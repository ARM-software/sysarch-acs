/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"

#include "aarch64/sysreg.h"
#include "pal_platform.h"
#include "pal_firme_gpt_memory.h"
#include "val_common/val_mp.h"
#include "val_common/val_status.h"
#include "val_firme_service.h"
#include "val_firme_diag.h"
#include "val_firme_gpt_probe.h"
#include "val_host_gpt.h"
#include "val_log.h"

#define VAL_FIRME_GPT_PGS_SIZE_4KB  UINT64_C(0x1000)
#define VAL_FIRME_GPT_PGS_SIZE_16KB UINT64_C(0x4000)
#define VAL_FIRME_GPT_PGS_SIZE_64KB UINT64_C(0x10000)

#define VAL_FIRME_GPT_L0GPTSZ_30_SHIFT 30U
#define VAL_FIRME_GPT_L0GPTSZ_34_SHIFT 34U
#define VAL_FIRME_GPT_L0GPTSZ_36_SHIFT 36U
#define VAL_FIRME_GPT_L0GPTSZ_39_SHIFT 39U

/**
 *   @brief    - Record a GPT helper failure in the rule result.
 *   @param    - out       : Optional rule result.
 *   @param    - fail_step : Failing algorithm step.
 *   @param    - last_x0   : Primary diagnostic value.
 *   @param    - last_x1   : Secondary diagnostic value.
 *   @return   - void.
 **/
static void val_firme_gpt_record_failure(firme_rule_res_t *out,
                                         uint64_t fail_step,
                                         uint64_t last_x0,
                                         uint64_t last_x1)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Probe a 64-bit load from a given world and return fault details.
 *   @param    - world     : Target world (NS-EL2/Secure/Realm).
 *   @param    - addr      : Address to read.
 *   @param    - out_value : Optional value read (valid when no fault).
 *   @param    - out_fault : Optional fault info (faulted/esr/far).
 *   @return   - VAL_SUCCESS on dispatch, VAL_ERROR on transport failure.
**/
int val_firme_mem_read64_world(firme_acs_world_t world,
                               uint64_t addr,
                               uint64_t *out_value,
                               val_fault_info_t *out_fault)
{
    firme_acs_call_res_t res = {0};

    if (out_fault) {
        volatile uint64_t *dst = (volatile uint64_t *)(void *)out_fault;
        dst[0] = 0;
        dst[1] = 0;
        dst[2] = 0;
    }
    if (out_value) {
        *out_value = 0;
    }

    if (world == FIRME_ACS_WORLD_NSEL2) {
        val_firme_gpt_probe_command(addr, &res);
    } else if (val_firme_world_call(world, FIRME_ACS_CMD_GPT_PROBE,
                                   addr, 0ULL, 0ULL, 0ULL, &res) != VAL_SUCCESS) {
        return VAL_ERROR;
    }

    if (res.x0 != (uint64_t)ACS_STATUS_PASS || res.x1 > VAL_TRUE) {
        return VAL_ERROR;
    }

    if (out_fault) {
        /*
         * Use volatile stores to avoid paired stores (stp) to potentially
         * misaligned addresses when alignment checking is enabled.
         * Cross-world memory probes use a compact payload format:
         * x1=faulted, x2=ESR when faulted or read value when not faulted.
         * Realm transport only guarantees two payload words through RMMD.
         */
        volatile uint64_t *dst = (volatile uint64_t *)(void *)out_fault;
        dst[0] = res.x1; /* faulted */
        dst[1] = (res.x1 != 0ULL) ? res.x2 : 0; /* esr */
        dst[2] = 0; /* far is not part of the compact transport payload */
    }
    if (out_value) {
        *out_value = (res.x1 == 0ULL) ? res.x2 : 0; /* value */
    }
    return VAL_SUCCESS;
}

/**
 * @brief Read an owned PA through the Secure observer's Non-secure output PAS.
 * @return VAL_SUCCESS for completed access/fault evidence, VAL_ERROR on transport/setup errors.
 */
int val_firme_mem_read64_ns_world(firme_acs_world_t world, uint64_t address,
                                  uint64_t *value, val_fault_info_t *fault)
{
    firme_acs_call_res_t response = {0};

    if (!value || !fault || world != FIRME_ACS_WORLD_SECURE)
        return VAL_ERROR;
    *value = 0ULL;
    *fault = (val_fault_info_t){0};
    if (val_firme_world_call(world, FIRME_ACS_CMD_RUN_DIAG,
        FIRME_DIAG_READ_NS_PAS, address, 0ULL, 0ULL, &response) != VAL_SUCCESS ||
        response.x0 != ACS_STATUS_PASS || response.x1 > VAL_TRUE)
        return VAL_ERROR;

    fault->faulted = response.x1;
    fault->esr = response.x1 ? response.x2 : 0ULL;
    fault->far = response.x3;
    *value = response.x1 ? 0ULL : response.x2;
    return VAL_SUCCESS;
}

/**
 *   @brief    - Perform an NS write followed by a DSB to commit the store.
 *   @param    - addr  : Target address.
 *   @param    - value : Value to store.
 *   @return   - void
**/
void val_firme_ns_write64(uint64_t addr, uint64_t value)
{
    *(volatile uint64_t *)(uintptr_t)addr = value;
    dsb_sy();
}

/**
 *   @brief    - Convert an ABI return value into a signed W0 status.
 *   @param    - v : Raw x0 return.
 *   @return   - Signed FIRME status value.
**/
int64_t val_firme_as_i64(uint64_t v)
{
    return val_firme_status_to_i64(v);
}

/**
 *   @brief    - Check whether a FIRME return indicates success with an expected count.
 *   @param    - x0      : FIRME status.
 *   @param    - x1      : FIRME completion count.
 *   @param    - exp_cnt : Expected completion count.
 *   @return   - 1 if success and count matches, 0 otherwise.
**/
uint32_t val_firme_status_success(uint64_t x0, uint64_t x1, uint64_t exp_cnt)
{
    return (uint32_t)((val_firme_as_i64(x0) == (int64_t)FIRME_SUCCESS) && (x1 == exp_cnt));
}

/**
 *   @brief    - Check whether a FIRME return indicates a denied access.
 *   @param    - x0 : FIRME status.
 *   @return   - 1 if denied, 0 otherwise.
**/
uint32_t val_firme_status_deniedish(uint64_t x0)
{
    return (uint32_t)(val_firme_as_i64(x0) == (int64_t)FIRME_DENIED);
}

/**
 *   @brief    - Check whether a FIRME return indicates the ABI is not supported.
 *   @param    - x0 : FIRME status.
 *   @return   - 1 if not supported, 0 otherwise.
**/
uint32_t val_firme_status_not_supported(uint64_t x0)
{
    return (uint32_t)(val_firme_as_i64(x0) == (int64_t)FIRME_NOT_SUPPORTED);
}

/**
 *   @brief    - Check whether a FIRME return indicates invalid parameters.
 *   @param    - x0 : FIRME status.
 *   @return   - 1 if invalid parameters, 0 otherwise.
**/
uint32_t val_firme_status_invalid_parameters(uint64_t x0)
{
    return (uint32_t)(val_firme_as_i64(x0) == (int64_t)FIRME_INVALID_PARAMETERS);
}

/**
 * @brief Restore one test granule to NS while preserving primary diagnostics.
 * @param base_pa Test granule physical address.
 * @param out Optional rule result marked ERR on failure, retaining primary diagnostics.
 * @param fail_step Cleanup diagnostic step.
 * @return VAL_TRUE only for complete, successful restoration.
 */
uint32_t val_firme_gpt_restore_ns(uint64_t base_pa, firme_rule_res_t *out,
                                   uint64_t fail_step)
{
    firme_res_t result = {0};

    if ((val_firme_gm_gpi_set_complete(base_pa, UINT64_C(1),
                                      FIRME_GPI_NONSECURE,
                                      &result) != VAL_SUCCESS) ||
        (val_firme_status_success(result.x0, result.x1,
                                  UINT64_C(1)) == VAL_FALSE)) {
        LOG_ERROR(NULL, "gpt: NS restoration failed step=%llu x0=%llx x1=%llx",
                  (unsigned long long)fail_step,
                  (unsigned long long)result.x0,
                  (unsigned long long)result.x1);
        if (!out || ((out->status != (uint64_t)ACS_STATUS_FAIL) &&
                     (out->status != (uint64_t)ACS_STATUS_ERR)) ||
            ((out->fail_step == 0ULL) && (out->last_x0 == 0ULL) &&
             (out->last_x1 == 0ULL)))
            val_firme_gpt_record_failure(out, fail_step, result.x0, result.x1);

        if (out)
            out->status = (uint64_t)ACS_STATUS_ERR;

        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 * @brief Resolve one granule without mistaking a failed continuation for termination.
 * @param base_pa Owned granule physical address.
 * @param target_gpi Requested GPI encoding.
 * @param result Last ABI result, including valid progress and retained cookie.
 * @param out Rule result quarantined on malformed or unresolved operation state.
 * @param fail_step Transition diagnostic step.
 * @return VAL_SUCCESS for a known terminal result, not necessarily a successful transition.
 */
int val_firme_gpt_transition_terminal(uint64_t base_pa, uint64_t target_gpi,
    firme_res_t *result, firme_rule_res_t *out, uint64_t fail_step)
{
    val_firme_gpt_trace_t trace = {0};
    const val_firme_gpt_phase_t *last = NULL;
    int trace_status;

    if (!result || !out || out->status == ACS_STATUS_ERR)
        return VAL_ERROR;

    *result = (firme_res_t){0};
    trace_status = val_firme_gpt_trace_transition(base_pa, UINT64_C(1), target_gpi,
                                                 VAL_FALSE, out, fail_step, &trace);
    if (trace.phase_count != 0U && trace.phase_count <= VAL_FIRME_GPT_TRACE_MAX_PHASES) {
        last = &trace.phases[trace.phase_count - 1U];
        result->x0 = (uint64_t)last->status;
        result->x1 = last->granule_count;
        result->x2 = last->output_cookie;
    }

    if (trace_status == VAL_SUCCESS && !trace.observation_failed &&
        val_firme_gpt_trace_terminal(&trace))
        return VAL_SUCCESS;

    LOG_ERROR(NULL, "gpt: operation unresolved; no competing cleanup "
              "step=%llu x0=%llx x1=%llx cookie=%llx\n",
              (unsigned long long)fail_step, (unsigned long long)result->x0,
              (unsigned long long)result->x1, (unsigned long long)result->x2);
    val_firme_gpt_rule_result_fail(out, fail_step, result->x0, result->x1);
    out->status = ACS_STATUS_ERR;
    return VAL_ERROR;
}

/**
 * @brief Prove Host-owned scratch is NS without trusting a terminal ABI error.
 * @param base_pa Owned granule; no retained or unresolved operation may reference it.
 * @param out Rule result retained on successful restoration, quarantined on uncertainty.
 * @param fail_step Restoration diagnostic step.
 * @return VAL_TRUE only for independent NS evidence and successful scrubbing.
 */
uint32_t val_firme_gpt_confirm_ns_restored(uint64_t base_pa,
    firme_rule_res_t *out, uint64_t fail_step)
{
    firme_rule_res_t observation = {0};

    if (!out || out->status == ACS_STATUS_ERR)
        return VAL_FALSE;

    if (!val_firme_check_access_matrix(base_pa, FIRME_GPI_NONSECURE,
            0ULL, VAL_FALSE, &observation, fail_step)) {
        if (out->status != ACS_STATUS_FAIL || out->fail_step == 0ULL)
            *out = observation;

        out->status = ACS_STATUS_ERR;
        return VAL_FALSE;
    }

    if (!val_firme_gpt_scrub_ns(base_pa, out, fail_step)) {
        out->status = ACS_STATUS_ERR;
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 * @brief Surround a protected caller's R0094 body with independent Host PAS observations.
 * @param world Secure or Realm caller; Host owns the supplied scratch granule.
 * @param base_pa Scratch whose prior test completed without unresolved operation state.
 * @param response Original diagnostics, upgraded to ERR if restoration cannot be proved.
 * @return VAL_SUCCESS for a delivered result, VAL_ERROR for transport or argument errors.
 */
int val_firme_gpt_run_rme_observed(firme_acs_world_t world, uint64_t base_pa,
    firme_acs_call_res_t *response)
{
    return val_firme_gpt_run_observed(FIRME_RULE_R0094, world, base_pa, response);
}

/** @brief Select only rules whose bodies preserve unresolved operation state as ERR. */
uint64_t val_firme_gpt_observed_rule_granules(firme_rule_id_t rule)
{
    switch (rule) {
    case FIRME_RULE_R0111:
    case FIRME_RULE_R0113:
    case FIRME_RULE_R0117:
        return UINT64_C(2);
    case FIRME_RULE_R0114:
    case FIRME_RULE_R0115:
    case FIRME_RULE_R0129:
    case FIRME_RULE_R0130:
        return PLAT_FIRME_GPT_TRANSITION_SIZE / PLAT_FIRME_GPT_GRANULE_SIZE;
    case FIRME_RULE_R0090:
    case FIRME_RULE_R0092:
    case FIRME_RULE_R0094:
    case FIRME_RULE_R0120:
    case FIRME_RULE_R0127:
    case FIRME_RULE_R0134:
        return UINT64_C(1);
    default:
        return 0ULL;
    }
}

/**
 * @brief Prove a bounded scratch range is NS from Host before permitting reuse.
 * @param base_pa Owned scratch range base.
 * @param count Number of granules in the rule's complete lease.
 * @param granule_size Validated architectural granule size.
 * @param out Original rule result, never downgraded from ERR.
 * @param step Observation diagnostic step.
 * @return VAL_TRUE only if every granule passes exact NS checks and scrubbing.
 */
static uint32_t val_firme_gpt_confirm_ns_range(uint64_t base_pa, uint64_t count,
    uint64_t granule_size, firme_rule_res_t *out, uint64_t step)
{
    uint64_t index;

    for (index = 0ULL; index < count; index++) {
        if (!val_firme_gpt_confirm_ns_restored(base_pa + index * granule_size,
                                               out, step))
            return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 * @brief Run an audited rule with Host-owned, independently checked scratch.
 * @param rule Audited rule whose body never hides unresolved work.
 * @param world Caller world; protected bodies execute without nested transport.
 * @param base_pa Owned scratch base, not referenced by an earlier retained operation.
 * @param response Original rule result, quarantined if whole-range restoration is unproved.
 * @return VAL_SUCCESS for a delivered result, VAL_ERROR for transport or argument errors.
 */
int val_firme_gpt_run_observed(firme_rule_id_t rule, firme_acs_world_t world,
    uint64_t base_pa, firme_acs_call_res_t *response)
{
    firme_rule_res_t observation = {.status = ACS_STATUS_PASS};
    firme_rule_res_t result = {.status = ACS_STATUS_ERR};
    uint64_t count = val_firme_gpt_observed_rule_granules(rule);
    uint64_t feature_reg1 = 0ULL;
    uint64_t granule_size;
    int call_status;

    if (!response || count == 0ULL ||
        (world != FIRME_ACS_WORLD_SECURE && world != FIRME_ACS_WORLD_REALM &&
         world != FIRME_ACS_WORLD_NSEL2))
        return VAL_ERROR;

    *response = (firme_acs_call_res_t){.x0 = ACS_STATUS_ERR};
    if (rule == FIRME_RULE_R0114 || rule == FIRME_RULE_R0115 ||
        rule == FIRME_RULE_R0129 || rule == FIRME_RULE_R0130) {
        call_status = val_host_gpt_retained_run(rule, world, &result);
        if (call_status != VAL_SUCCESS &&
            (result.status == ACS_STATUS_PASS || result.status == ACS_STATUS_SKIP))
            result.status = ACS_STATUS_ERR;

        *response = (firme_acs_call_res_t){.x0 = result.status, .x1 = result.fail_step,
            .x2 = result.last_x0, .x3 = result.last_x1};
        return VAL_SUCCESS;
    }

    if (!val_firme_gpt_feature_reg1(&feature_reg1, &observation, UINT64_C(1)))
        goto failed;

    granule_size = val_firme_gpt_pgs_size_from_feature_reg1(feature_reg1);
    if (!base_pa || !granule_size || base_pa % granule_size != 0ULL ||
        count > (UINT64_MAX - base_pa) / granule_size) {
        val_firme_gpt_rule_result_fail(&observation, UINT64_C(1), base_pa, granule_size);
        goto failed;
    }

    if (!val_firme_gpt_confirm_ns_range(base_pa, count, granule_size,
                                        &observation, UINT64_C(1)))
        goto failed;

    if (world == FIRME_ACS_WORLD_NSEL2) {
        call_status = val_firme_rule_run_ns(rule, base_pa, &result);
        if (call_status != VAL_SUCCESS &&
            (result.status == ACS_STATUS_PASS || result.status == ACS_STATUS_SKIP))
            result.status = ACS_STATUS_ERR;

        *response = (firme_acs_call_res_t){.x0 = result.status, .x1 = result.fail_step,
            .x2 = result.last_x0, .x3 = result.last_x1};
    } else {
        call_status = val_firme_world_call(world, FIRME_ACS_CMD_RUN_RULE,
            rule, base_pa, 0ULL, 0ULL, response);
        if (call_status != VAL_SUCCESS) {
            response->x0 = ACS_STATUS_ERR;
            return VAL_ERROR;
        }
    }

    if (response->x0 == ACS_STATUS_ERR)
        return VAL_SUCCESS;

    if (response->x0 != ACS_STATUS_PASS && response->x0 != ACS_STATUS_FAIL &&
        response->x0 != ACS_STATUS_SKIP) {
        response->x0 = ACS_STATUS_ERR;
        return VAL_SUCCESS;
    }

    observation = (firme_rule_res_t){.status = response->x0, .fail_step = response->x1,
                                    .last_x0 = response->x2, .last_x1 = response->x3};
    if (!val_firme_gpt_confirm_ns_range(base_pa, count, granule_size,
                                        &observation, UINT64_C(30)))
        goto failed;

    return VAL_SUCCESS;

failed:
    *response = (firme_acs_call_res_t){.x0 = ACS_STATUS_ERR,
        .x1 = observation.fail_step, .x2 = observation.last_x0, .x3 = observation.last_x1};
    return VAL_SUCCESS;
}

/** @brief Accept only bounded terminal traces; unknown continuation errors retain quarantine. */
uint32_t val_firme_gpt_trace_terminal(const val_firme_gpt_trace_t *trace)
{
    const val_firme_gpt_phase_t *last;

    if (!trace || !trace->phase_count || trace->phase_count > VAL_FIRME_GPT_TRACE_MAX_PHASES ||
        trace->malformed || trace->exhausted)
        return VAL_FALSE;

    last = &trace->phases[trace->phase_count - 1U];
    if (last->status == FIRME_INCOMPLETE || last->status == FIRME_BUSY)
        return VAL_FALSE;

    if (trace->unexpected_status)
        return last->abi == VAL_FIRME_GPT_PHASE_GPI_SET &&
               (last->status == FIRME_INVALID_PARAMETERS || last->status == FIRME_NOT_SUPPORTED);

    return VAL_TRUE;
}

/**
 * @brief Restore a known changed prefix without issuing same-GPI requests for its suffix.
 * @param base_pa Start of the exclusively owned range.
 * @param count Known changed prefix; zero defers the entire range to Host observation.
 * @param granule_size Validated granule size.
 * @param out Original rule result; existing ERR forbids further calls.
 * @param fail_step Cleanup diagnostic step.
 * @return VAL_TRUE for successful cleanup calls; whole-range Host proof is still required.
 */
uint32_t val_firme_gpt_restore_prefix_ns(uint64_t base_pa, uint64_t count,
    uint64_t granule_size, firme_rule_res_t *out, uint64_t fail_step)
{
    firme_rule_res_t cleanup = {.status = ACS_STATUS_PASS};
    firme_res_t result = {0};
    uint64_t index;
    uint32_t restored = VAL_TRUE;

    if (!out || out->status == ACS_STATUS_ERR)
        return VAL_FALSE;

    if (!granule_size || count > (UINT64_MAX - base_pa) / granule_size) {
        out->status = ACS_STATUS_ERR;
        return VAL_FALSE;
    }

    for (index = 0ULL; index < count; index++) {
        if (val_firme_gpt_transition_terminal(base_pa + index * granule_size,
                FIRME_GPI_NONSECURE, &result, &cleanup, fail_step + index) != VAL_SUCCESS) {
            if (out->status != ACS_STATUS_FAIL || out->fail_step == 0ULL)
                *out = cleanup;

            out->status = ACS_STATUS_ERR;
            return VAL_FALSE;
        }

        if (!val_firme_status_success(result.x0, result.x1, UINT64_C(1))) {
            LOG_ERROR(NULL, "gpt: changed-prefix cleanup rejected PA=%llx x0=%llx x1=%llx\n",
                (unsigned long long)(base_pa + index * granule_size),
                (unsigned long long)result.x0, (unsigned long long)result.x1);
            if (out->status != ACS_STATUS_FAIL || out->fail_step == 0ULL)
                val_firme_gpt_rule_result_fail(out, fail_step + index, result.x0, result.x1);

            restored = VAL_FALSE;
        }
    }

    return restored;
}

/**
 * @brief Scrub an NS-accessible test granule and clean it to PoC before NSO release.
 * @param base_pa Owned, identity-mapped test granule physical address.
 * @param out Rule result, preserving an already recorded primary failure.
 * @param fail_step Cleanup diagnostic step.
 * @return VAL_TRUE only after every store and the PoC clean complete.
 */
uint32_t val_firme_gpt_scrub_ns(uint64_t base_pa, firme_rule_res_t *out,
                                 uint64_t fail_step)
{
    firme_rule_res_t scrub_result = {0};
    val_fault_info_t fault_info = {0};
    uint64_t feature_reg1 = 0ULL;
    uint64_t granule_size;
    uint64_t offset;

    if (!out)
        return VAL_FALSE;

    if (val_firme_gpt_feature_reg1(&feature_reg1, &scrub_result,
                                   fail_step) == VAL_FALSE)
        goto failed;

    granule_size = val_firme_gpt_pgs_size_from_feature_reg1(feature_reg1);
    if ((granule_size == 0ULL) ||
        (granule_size != PLAT_FIRME_GPT_GRANULE_SIZE) || (base_pa == 0ULL) ||
        ((base_pa % granule_size) != 0ULL) ||
        (base_pa > UINT64_MAX - granule_size)) {
        val_firme_gpt_record_failure(&scrub_result, fail_step, base_pa,
                                     granule_size);
        goto failed;
    }

    for (offset = 0ULL; offset < granule_size; offset += sizeof(uint64_t)) {
        if (val_probe_write64(base_pa + offset, 0ULL, &fault_info) != VAL_SUCCESS) {
            val_firme_gpt_record_failure(&scrub_result, fail_step,
                                         fault_info.esr, fault_info.far);
            goto failed;
        }
    }

    val_cache_clean_poc((uintptr_t)base_pa, (size_t)granule_size);
    return VAL_TRUE;

failed:
    LOG_ERROR(NULL, "gpt: scrub/PoC preparation failed step=%llu x0=%llx x1=%llx",
              (unsigned long long)fail_step,
              (unsigned long long)scrub_result.last_x0,
              (unsigned long long)scrub_result.last_x1);
    if ((out->status != ACS_STATUS_FAIL) ||
        ((out->fail_step == 0ULL) && (out->last_x0 == 0ULL) &&
         (out->last_x1 == 0ULL)))
        *out = scrub_result;
    return VAL_FALSE;
}

/**
 *   @brief    - Check whether FIRME_GM_GPI_SET is advertised by the DUT.
 *   @param    - out       : Optional rule result updated on skip/failure.
 *   @param    - fail_step : Step ID to report on service-query failure.
 *   @return   - VAL_TRUE if advertised, otherwise VAL_FALSE.
**/
uint32_t val_firme_gpt_gpi_set_advertised(firme_rule_res_t *out, uint64_t fail_step)
{
    firme_res_t features_res = {0};
    uint64_t feature_reg0 = 0ULL;
    uint32_t advertised = VAL_FALSE;

    if (val_firme_service_abi_is_advertised(FIRME_SERVICE_ID_GRANULE_MGMT,
                                            FIRME_GM_FEATURE_REG0_GPI_SET_BIT,
                                            &advertised, &feature_reg0,
                                            &features_res) != VAL_SUCCESS) {
        LOG_ERROR(NULL, "gpt: failed to query GM_GPI_SET advertisement x0=%llx x1=%llx",
                  (unsigned long long)features_res.x0,
                  (unsigned long long)features_res.x1);
        if (out) {
            out->status = (uint64_t)ACS_STATUS_FAIL;
            out->fail_step = fail_step;
            out->last_x0 = features_res.x0;
            out->last_x1 = features_res.x1;
        }
        return VAL_FALSE;
    }

    if (advertised == VAL_FALSE) {
        if (val_firme_status_to_i64(features_res.x0) ==
            (int64_t)FIRME_NOT_SUPPORTED) {
            if (out) {
                out->status = (uint64_t)ACS_STATUS_SKIP;
                out->fail_step = 0;
                out->last_x0 = features_res.x0;
                out->last_x1 = 0;
            }
            return VAL_FALSE;
        }

        LOG_ERROR(NULL, "gpt: GM service omits mandatory GPI_SET bit reg0=%llx",
                  (unsigned long long)feature_reg0);
        if (out) {
            out->status = (uint64_t)ACS_STATUS_FAIL;
            out->fail_step = fail_step;
            out->last_x0 = features_res.x0;
            out->last_x1 = feature_reg0;
        }
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 *   @brief    - Read Granule Management feature register 1.
 *   @param    - feature_reg1 : Returned feature register value.
 *   @param    - out          : Optional rule result updated on skip/failure.
 *   @param    - fail_step    : Step ID to report on service-query failure.
 *   @return   - VAL_TRUE if feature register 1 is available, otherwise VAL_FALSE.
**/
uint32_t val_firme_gpt_feature_reg1(uint64_t *feature_reg1, firme_rule_res_t *out,
                                    uint64_t fail_step)
{
    firme_res_t res = {0};

    if (feature_reg1)
        *feature_reg1 = 0ULL;

    val_firme_service_features(FIRME_SERVICE_ID_GRANULE_MGMT,
                               FIRME_GM_FEATURE_REG1_INDEX, &res);

    if (val_firme_status_not_supported(res.x0)) {
        LOG_ERROR(NULL,
                  "gpt: mandatory GM feature register 1 is not supported");
        val_firme_gpt_record_failure(out, fail_step, res.x0, res.x1);
        return VAL_FALSE;
    }

    if (val_firme_as_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        LOG_ERROR(NULL, "gpt: failed to read GM feature register 1 x0=%llx x1=%llx",
                  (unsigned long long)res.x0, (unsigned long long)res.x1);
        if (out) {
            out->status = (uint64_t)ACS_STATUS_FAIL;
            out->fail_step = fail_step;
            out->last_x0 = res.x0;
            out->last_x1 = res.x1;
        }
        return VAL_FALSE;
    }

    if (feature_reg1)
        *feature_reg1 = res.x1;

    return VAL_TRUE;
}

/**
 *   @brief    - Decode the PGS field in Granule Management feature register 1.
 *   @param    - feature_reg1 : Raw feature register 1 value.
 *   @return   - Granule size in bytes, or 0 for an unknown encoding.
**/
uint64_t val_firme_gpt_pgs_size_from_feature_reg1(uint64_t feature_reg1)
{
    uint64_t pgs = (feature_reg1 >> FIRME_GM_FEATURE_REG1_PGS_SHIFT) &
                   FIRME_GM_FEATURE_REG1_PGS_MASK;

    if (pgs == FIRME_GM_PGS_4KB)
        return VAL_FIRME_GPT_PGS_SIZE_4KB;

    if (pgs == FIRME_GM_PGS_16KB)
        return VAL_FIRME_GPT_PGS_SIZE_16KB;

    if (pgs == FIRME_GM_PGS_64KB)
        return VAL_FIRME_GPT_PGS_SIZE_64KB;

    return 0ULL;
}

/**
 *   @brief    - Decode protected-address-range size from GM feature register 1.
 *   @param    - feature_reg1 : Raw feature register 1 value.
 *   @return   - Protected-address-range size in bytes, or 0 for a reserved encoding.
 **/
uint64_t val_firme_gpt_range_size_from_feature_reg1(uint64_t feature_reg1)
{
    uint64_t l0gptsz;
    uint32_t address_shift;

    l0gptsz = (feature_reg1 >> FIRME_GM_FEATURE_REG1_L0GPTSZ_SHIFT) &
              FIRME_GM_FEATURE_REG1_L0GPTSZ_MASK;

    switch (l0gptsz) {
    case FIRME_GM_L0GPTSZ_30BITS:
        address_shift = VAL_FIRME_GPT_L0GPTSZ_30_SHIFT;
        break;
    case FIRME_GM_L0GPTSZ_34BITS:
        address_shift = VAL_FIRME_GPT_L0GPTSZ_34_SHIFT;
        break;
    case FIRME_GM_L0GPTSZ_36BITS:
        address_shift = VAL_FIRME_GPT_L0GPTSZ_36_SHIFT;
        break;
    case FIRME_GM_L0GPTSZ_39BITS:
        address_shift = VAL_FIRME_GPT_L0GPTSZ_39_SHIFT;
        break;
    default:
        return 0ULL;
    }

    return UINT64_C(1) << address_shift;
}

/**
 *   @brief    - Decode protected physical address size from GM feature register 1.
 *   @param    - feature_reg1 : Raw feature register 1 value.
 *   @return   - Protected physical address size in bytes, or 0 for a reserved encoding.
 **/
uint64_t val_firme_gpt_pps_size_from_feature_reg1(uint64_t feature_reg1)
{
    uint64_t pps;
    uint32_t address_shift;

    pps = (feature_reg1 >> FIRME_GM_FEATURE_REG1_PPS_SHIFT) &
          FIRME_GM_FEATURE_REG1_PPS_MASK;

    switch (pps) {
    case FIRME_GM_PPS_4GB:
        address_shift = 32U;
        break;
    case FIRME_GM_PPS_64GB:
        address_shift = 36U;
        break;
    case FIRME_GM_PPS_1TB:
        address_shift = 40U;
        break;
    case FIRME_GM_PPS_4TB:
        address_shift = 42U;
        break;
    case FIRME_GM_PPS_16TB:
        address_shift = 44U;
        break;
    case FIRME_GM_PPS_256TB:
        address_shift = 48U;
        break;
    case FIRME_GM_PPS_4PB:
        address_shift = 52U;
        break;
    default:
        return 0ULL;
    }

    return UINT64_C(1) << address_shift;
}

/**
 *   @brief    - Check whether mandatory FIRME_GM_GPT_INFO is advertised.
 *   @param    - out       : Optional rule result updated on failure.
 *   @param    - fail_step : Step ID to report on discovery failure.
 *   @return   - VAL_TRUE if advertised, otherwise VAL_FALSE.
 **/
uint32_t val_firme_gpt_info_advertised(firme_rule_res_t *out,
                                       uint64_t fail_step)
{
    firme_res_t features_res = {0};
    uint64_t feature_reg0 = 0ULL;
    uint32_t advertised = VAL_FALSE;

    if (val_firme_service_abi_is_advertised(
            FIRME_SERVICE_ID_GRANULE_MGMT,
            FIRME_GM_FEATURE_REG0_GPT_INFO_BIT, &advertised,
            &feature_reg0, &features_res) != VAL_SUCCESS) {
        LOG_ERROR(NULL,
                  "gpt_info: feature discovery failed x0=%llx x1=%llx",
                  (unsigned long long)features_res.x0,
                  (unsigned long long)features_res.x1);
        val_firme_gpt_record_failure(out, fail_step, features_res.x0,
                                     features_res.x1);
        return VAL_FALSE;
    }

    if (advertised == VAL_FALSE) {
        LOG_ERROR(NULL,
                  "gpt_info: mandatory ABI is not advertised reg0=%llx",
                  (unsigned long long)feature_reg0);
        val_firme_gpt_record_failure(out, fail_step, features_res.x0,
                                     feature_reg0);
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 * @brief Check whether mandatory FIRME_GM_GPI_OP_CONTINUE is advertised.
 * @param out Optional rule result updated on failure.
 * @param fail_step Step ID to report on discovery failure.
 * @return VAL_TRUE if advertised, otherwise VAL_FALSE.
 */
uint32_t val_firme_gpt_op_continue_advertised(firme_rule_res_t *out,
                                              uint64_t fail_step)
{
    firme_res_t features_res = {0};
    uint64_t feature_reg0 = 0ULL;
    uint32_t advertised = VAL_FALSE;

    if (val_firme_service_abi_is_advertised(
            FIRME_SERVICE_ID_GRANULE_MGMT,
            FIRME_GM_FEATURE_REG0_OP_CONTINUE_BIT, &advertised,
            &feature_reg0, &features_res) != VAL_SUCCESS) {
        LOG_ERROR(NULL,
                  "gpi_op_continue: feature discovery failed x0=%llx x1=%llx",
                  (unsigned long long)features_res.x0,
                  (unsigned long long)features_res.x1);
        val_firme_gpt_record_failure(out, fail_step, features_res.x0,
                                     features_res.x1);
        return VAL_FALSE;
    }

    if (advertised == VAL_FALSE) {
        LOG_ERROR(NULL,
                  "gpi_op_continue: mandatory ABI is not advertised reg0=%llx",
                  (unsigned long long)feature_reg0);
        val_firme_gpt_record_failure(out, fail_step, features_res.x0,
                                     feature_reg0);
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 * @brief Return whether a status belongs to a valid GPI operation row.
 * @param status Signed FIRME status.
 * @return VAL_TRUE for an architected operation result, otherwise VAL_FALSE.
 */
uint32_t val_firme_gpt_operation_status_valid(int64_t status)
{
    if ((status == (int64_t)FIRME_SUCCESS) ||
        (status == (int64_t)FIRME_DENIED) ||
        (status == (int64_t)FIRME_OP_CONFLICT) ||
        (status == (int64_t)FIRME_NOT_FOUND) ||
        (status == (int64_t)FIRME_INCOMPLETE) ||
        (status == (int64_t)FIRME_BUSY))
        return VAL_TRUE;

    return VAL_FALSE;
}

/**
 * @brief Validate per-call progress without consuming UNKNOWN count outputs.
 * @param status Signed FIRME operation status.
 * @param count Raw count output, ignored for BUSY.
 * @param remaining Granules remaining before this invocation.
 * @return VAL_TRUE for valid progress, otherwise VAL_FALSE.
 */
uint32_t val_firme_gpt_operation_count_valid(int64_t status,
                                              uint64_t count,
                                              uint64_t remaining)
{
    if ((remaining == 0ULL) ||
        (val_firme_gpt_operation_status_valid(status) == VAL_FALSE))
        return VAL_FALSE;

    if (status == (int64_t)FIRME_BUSY)
        return VAL_TRUE;

    if (status == (int64_t)FIRME_SUCCESS)
        return (uint32_t)((count != 0ULL) && (count <= remaining));

    return (uint32_t)(count < remaining);
}

/**
 * @brief Trace one GPI transition across stateless and stateful phases.
 * @param base_pa Base physical address of the first granule.
 * @param granule_count Number of granules requested by the caller.
 * @param target_gpi Target GPI encoding.
 * @param observe_prefix Whether to probe the transitioned prefix after each phase.
 * @param out Optional rule result updated on an observation mismatch.
 * @param fail_step Diagnostic step for a prefix mismatch.
 * @param trace Returned raw phase trace.
 * @return VAL_SUCCESS when a trace is produced, otherwise VAL_ERROR.
 */
int val_firme_gpt_trace_transition(uint64_t base_pa,
                                   uint64_t granule_count,
                                   uint64_t target_gpi,
                                   uint32_t observe_prefix,
                                   firme_rule_res_t *out,
                                   uint64_t fail_step,
                                   val_firme_gpt_trace_t *trace)
{
    firme_res_t result = {0};
    val_firme_gpt_phase_t *phase;
    uint64_t current_base = base_pa;
    uint64_t remaining = granule_count;
    uint64_t feature_reg1 = 0ULL;
    uint64_t granule_size = 0ULL;
    uint64_t cookie = 0ULL;
    uint64_t offset;
    int64_t status;
    uint32_t use_continue = VAL_FALSE;

    if (!trace || (base_pa == 0ULL) || (granule_count == 0ULL) ||
        ((observe_prefix != VAL_FALSE) && !out))
        return VAL_ERROR;

    *trace = (val_firme_gpt_trace_t){0};
    trace->requested_count = granule_count;

    if (val_firme_gpt_feature_reg1(
            &feature_reg1, NULL, 0ULL) == VAL_FALSE)
        return VAL_ERROR;

    granule_size =
        val_firme_gpt_pgs_size_from_feature_reg1(feature_reg1);
    if ((granule_size == 0ULL) ||
        ((base_pa % granule_size) != 0ULL) ||
        (granule_count > ((UINT64_MAX - base_pa) / granule_size))) {
        val_firme_gpt_record_failure(out, fail_step, base_pa, granule_count);
        return VAL_ERROR;
    }

    while ((remaining != 0ULL) &&
           (trace->phase_count < VAL_FIRME_GPT_TRACE_MAX_PHASES)) {
        phase = &trace->phases[trace->phase_count];
        phase->abi = (use_continue == VAL_TRUE) ?
                     VAL_FIRME_GPT_PHASE_OP_CONTINUE :
                     VAL_FIRME_GPT_PHASE_GPI_SET;
        phase->start_granule = trace->completed_count;
        phase->remaining_before = remaining;
        phase->input_cookie = cookie;

        if (use_continue == VAL_TRUE)
            val_firme_gm_gpi_op_continue(cookie, &result);
        else
            val_firme_gm_gpi_set_do(current_base, remaining,
                                    target_gpi, &result);

        status = val_firme_status_to_i64(result.x0);
        phase->status = status;
        phase->granule_count = result.x1;
        phase->output_cookie = result.x2;
        trace->phase_count++;

        if (status == (int64_t)FIRME_BUSY) {
            if ((observe_prefix != VAL_FALSE) &&
                (val_firme_gpt_local_prefix_matches(
                     base_pa, granule_count, trace->completed_count,
                     out, fail_step + trace->phase_count) == VAL_FALSE)) {
                trace->observation_failed = VAL_TRUE;
                return VAL_SUCCESS;
            }
            if (use_continue == VAL_TRUE)
                cookie = result.x2;
            continue;
        }

        if (val_firme_gpt_operation_status_valid(status) == VAL_FALSE) {
            trace->unexpected_status = VAL_TRUE;
            return VAL_SUCCESS;
        }

        if (val_firme_gpt_operation_count_valid(
                status, result.x1, remaining) == VAL_FALSE) {
            trace->malformed = VAL_TRUE;
            return VAL_SUCCESS;
        }

        trace->completed_count += result.x1;
        remaining -= result.x1;
        if (result.x1 > (UINT64_MAX / granule_size)) {
            trace->malformed = VAL_TRUE;
            return VAL_SUCCESS;
        }
        offset = result.x1 * granule_size;
        if (current_base > (UINT64_MAX - offset)) {
            trace->malformed = VAL_TRUE;
            return VAL_SUCCESS;
        }
        current_base += offset;

        if ((observe_prefix != VAL_FALSE) &&
            (val_firme_gpt_local_prefix_matches(
                 base_pa, granule_count, trace->completed_count,
                 out, fail_step + trace->phase_count) == VAL_FALSE)) {
            trace->observation_failed = VAL_TRUE;
            return VAL_SUCCESS;
        }

        if (status == (int64_t)FIRME_INCOMPLETE) {
            cookie = result.x2;
            use_continue = VAL_TRUE;
            continue;
        }

        if (status == (int64_t)FIRME_SUCCESS) {
            use_continue = VAL_FALSE;
            continue;
        }

        return VAL_SUCCESS;
    }

    if (remaining != 0ULL)
        trace->exhausted = VAL_TRUE;

    return VAL_SUCCESS;
}

/**
 * @brief Initialize a GPT rule result to FAIL with empty diagnostics.
 * @param out Optional rule result.
 * @return None.
 */
void val_firme_gpt_rule_result_init(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0ULL;
        out->last_x0 = 0ULL;
        out->last_x1 = 0ULL;
    }
}

/**
 * @brief Return the execution status of a completed GPT rule result.
 * @param out Required result record.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
int val_firme_gpt_rule_result_status(const firme_rule_res_t *out)
{
    if (out && ((out->status == (uint64_t)ACS_STATUS_PASS) ||
                (out->status == (uint64_t)ACS_STATUS_SKIP)))
        return VAL_SUCCESS;

    return VAL_ERROR;
}

/**
 * @brief Record a GPT rule assertion failure.
 * @param out Optional rule result.
 * @param fail_step Failing algorithm step.
 * @param last_x0 Primary diagnostic value.
 * @param last_x1 Secondary diagnostic value.
 * @return None.
 */
void val_firme_gpt_rule_result_fail(firme_rule_res_t *out,
                                    uint64_t fail_step,
                                    uint64_t last_x0,
                                    uint64_t last_x1)
{
    val_firme_gpt_record_failure(out, fail_step, last_x0, last_x1);
}

/**
 * @brief Find a status returned by a particular ABI in a phase trace.
 * @param trace GPI operation trace.
 * @param abi ABI phase to match.
 * @param status Signed FIRME status to match.
 * @return VAL_TRUE when a matching phase exists, otherwise VAL_FALSE.
 */
uint32_t val_firme_gpt_trace_has_status(
    const val_firme_gpt_trace_t *trace, val_firme_gpt_phase_abi_t abi,
    int64_t status)
{
    uint32_t phase_index;

    if (!trace || (trace->phase_count > VAL_FIRME_GPT_TRACE_MAX_PHASES))
        return VAL_FALSE;

    for (phase_index = 0U; phase_index < trace->phase_count;
         phase_index++) {
        if ((trace->phases[phase_index].abi == abi) &&
            (trace->phases[phase_index].status == status))
            return VAL_TRUE;
    }

    return VAL_FALSE;
}

/**
 * @brief Validate the locally observable transitioned prefix.
 * @param base_pa First granule in the requested range.
 * @param requested_count Number of granules in the requested range.
 * @param completed_count Number reported as transitioned.
 * @param out Optional rule result updated on mismatch.
 * @param fail_step Diagnostic step.
 * @return VAL_TRUE when access matches one contiguous prefix.
 */
uint32_t val_firme_gpt_local_prefix_matches(
    uint64_t base_pa, uint64_t requested_count, uint64_t completed_count,
    firme_rule_res_t *out, uint64_t fail_step)
{
    val_fault_info_t fault_info = {0};
    uint64_t feature_reg1 = 0ULL;
    uint64_t granule_size;
    uint64_t address;
    uint64_t value = 0ULL;
    uint64_t index;
    uint32_t expect_access;

    if ((base_pa == 0ULL) || (requested_count == 0ULL) ||
        (completed_count > requested_count) ||
        (val_firme_gpt_feature_reg1(&feature_reg1, out, fail_step) ==
         VAL_FALSE))
        return VAL_FALSE;

    granule_size = val_firme_gpt_pgs_size_from_feature_reg1(feature_reg1);
    if ((granule_size == 0ULL) ||
        (requested_count > ((UINT64_MAX - base_pa) / granule_size))) {
        val_firme_gpt_record_failure(out, fail_step, base_pa,
                                     requested_count);
        return VAL_FALSE;
    }

    for (index = 0ULL; index < requested_count; index++) {
        address = base_pa + (index * granule_size);
        fault_info = (val_fault_info_t){0};
        value = 0ULL;
        val_probe_read64(address, &value, &fault_info);
        expect_access = (uint32_t)(index < completed_count);

        if (((expect_access == VAL_TRUE) &&
             (fault_info.faulted != 0ULL)) ||
            ((expect_access == VAL_FALSE) &&
             ((fault_info.faulted == 0ULL) ||
              !val_fault_is_gpf(fault_info.esr)))) {
            LOG_ERROR(NULL,
                      "gpt prefix mismatch index=%llu completed=%llu "
                      "faulted=%llu esr=%llx",
                      (unsigned long long)index,
                      (unsigned long long)completed_count,
                      (unsigned long long)fault_info.faulted,
                      (unsigned long long)fault_info.esr);
            val_firme_gpt_record_failure(out, fail_step + index,
                                         fault_info.esr, address);
            return VAL_FALSE;
        }
    }

    return VAL_TRUE;
}

/**
 *   @brief    - Derive the protected range containing a platform test address.
 *   @param    - test_pa   : Platform test address.
 *   @param    - window    : Returned protected-address-range window.
 *   @param    - out       : Optional rule result updated on failure.
 *   @param    - fail_step : Step ID to report on invalid discovery data.
 *   @return   - VAL_TRUE for a valid window, otherwise VAL_FALSE.
 **/
uint32_t val_firme_gpt_info_prepare_window(
    uint64_t test_pa, val_firme_gpt_info_window_t *window,
    firme_rule_res_t *out, uint64_t fail_step)
{
    uint64_t feature_reg1 = 0ULL;
    uint64_t advertised_granule_size;
    uint64_t advertised_range_size;
    uint64_t advertised_pa_size;
    uint64_t range_mask;

    if (!window) {
        val_firme_gpt_record_failure(out, fail_step, test_pa, 0ULL);
        return VAL_FALSE;
    }

    *window = (val_firme_gpt_info_window_t){0};
    if (val_firme_gpt_feature_reg1(&feature_reg1, out, fail_step) ==
        VAL_FALSE)
        return VAL_FALSE;

    advertised_granule_size =
        val_firme_gpt_pgs_size_from_feature_reg1(feature_reg1);
    advertised_range_size =
        val_firme_gpt_range_size_from_feature_reg1(feature_reg1);
    advertised_pa_size =
        val_firme_gpt_pps_size_from_feature_reg1(feature_reg1);

    window->granule_size = PLAT_FIRME_GPT_GRANULE_SIZE;
    window->range_size = PLAT_FIRME_GPT_PROTECTED_RANGE_SIZE;
    window->protected_pa_size = PLAT_FIRME_GPT_PROTECTED_PA_SIZE;

    if ((test_pa == 0ULL) || (window->granule_size == 0ULL) ||
        (advertised_granule_size != window->granule_size) ||
        (advertised_range_size != window->range_size) ||
        (advertised_pa_size != window->protected_pa_size)) {
        LOG_ERROR(NULL,
                  "gpt_info: geometry mismatch reg1=%llx pgs=%llx/%llx "
                  "range=%llx/%llx pps=%llx/%llx",
                  (unsigned long long)feature_reg1,
                  (unsigned long long)advertised_granule_size,
                  (unsigned long long)window->granule_size,
                  (unsigned long long)advertised_range_size,
                  (unsigned long long)window->range_size,
                  (unsigned long long)advertised_pa_size,
                  (unsigned long long)window->protected_pa_size);
        val_firme_gpt_record_failure(out, fail_step, feature_reg1, test_pa);
        return VAL_FALSE;
    }

    range_mask = window->range_size - UINT64_C(1);
    window->range_base = test_pa & ~range_mask;
    if ((window->range_base >= window->protected_pa_size) ||
        (window->range_size >
         (window->protected_pa_size - window->range_base))) {
        LOG_ERROR(NULL,
                  "gpt_info: test range is outside PPS base=%llx size=%llx",
                  (unsigned long long)window->range_base,
                  (unsigned long long)window->range_size);
        val_firme_gpt_record_failure(out, fail_step, window->range_base,
                                     window->protected_pa_size);
        return VAL_FALSE;
    }

    window->range_top = window->range_base + window->range_size;
    return VAL_TRUE;
}

/**
 *   @brief    - Validate the common SUCCESS outputs of FIRME_GM_GPT_INFO.
 *   @param    - window    : Input query window.
 *   @param    - result    : ABI result.
 *   @param    - out       : Optional rule result updated on failure.
 *   @param    - fail_step : Step ID to report on malformed output.
 *   @return   - VAL_TRUE for valid SUCCESS outputs, otherwise VAL_FALSE.
 **/
uint32_t val_firme_gpt_info_success_valid(
    const val_firme_gpt_info_window_t *window, const firme_res_t *result,
    firme_rule_res_t *out, uint64_t fail_step)
{
    if (!window || !result || (window->range_size == 0ULL) ||
        (window->range_base >= window->range_top) ||
        ((window->range_base % window->range_size) != 0ULL) ||
        ((window->range_top % window->range_size) != 0ULL) ||
        (window->range_top > window->protected_pa_size)) {
        val_firme_gpt_record_failure(out, fail_step, 0ULL, 0ULL);
        return VAL_FALSE;
    }

    if ((val_firme_status_to_i64(result->x0) !=
         (int64_t)FIRME_SUCCESS) ||
        (result->x1 <= window->range_base) ||
        (result->x1 > window->range_top) ||
        ((result->x1 % window->range_size) != 0ULL) ||
        (result->x2 > FIRME_GPT_PAR_STATE_MAX)) {
        LOG_ERROR(NULL,
                  "gpt_info: invalid success result x0=%llx x1=%llx x2=%llx",
                  (unsigned long long)result->x0,
                  (unsigned long long)result->x1,
                  (unsigned long long)result->x2);
        val_firme_gpt_record_failure(out, fail_step, result->x0,
                                     result->x1);
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 *   @brief    - Determine which world is expected to access memory in a given GPI.
 *   @param    - gpi   : GPI ID (FIRME_GPI_*).
 *   @param    - world : World being validated.
 *   @return   - 1 if access should be permitted, 0 otherwise.
**/
uint32_t val_firme_expect_access_world(uint64_t gpi, firme_acs_world_t world)
{
    switch ((uint32_t)gpi) {
    case (uint32_t)FIRME_GPI_SECURE:
        return (uint32_t)(world == FIRME_ACS_WORLD_SECURE);
    case (uint32_t)FIRME_GPI_REALM:
        return (uint32_t)(world == FIRME_ACS_WORLD_REALM);
    case (uint32_t)FIRME_GPI_ROOT:
    case (uint32_t)FIRME_GPI_NSP:
    case (uint32_t)FIRME_GPI_SA:
        return VAL_FALSE;
    case (uint32_t)FIRME_GPI_NONSECURE:
    case (uint32_t)FIRME_GPI_NSO:
        return (uint32_t)(world == FIRME_ACS_WORLD_NSEL2);
    default:
        return VAL_FALSE;
    }
}

/**
 *   @brief    - Validate access enforcement from all worlds for a single address/GPI.
 *   @param    - addr           : Address to probe.
 *   @param    - gpi            : GPI the granule is expected to be in.
 *   @param    - expected_value : Expected 64-bit value when access is permitted.
 *   @param    - check_value    : If non-zero, validate the returned value as well.
 *   @param    - out            : Optional rule result (filled on failure).
 *   @param    - fail_step_base : Base step index to encode which world failed.
 *   @return   - 1 if matrix matches expectations, 0 otherwise.
**/
uint32_t val_firme_check_access_matrix(uint64_t addr,
                                       uint64_t gpi,
                                       uint64_t expected_value,
                                       uint32_t check_value,
                                       firme_rule_res_t *out,
                                       uint64_t fail_step_base)
{
    /* Transport failure tag for out->last_x0. */
    const uint64_t transport_fail_magic = UINT64_C(0xFFFF0000);

    const firme_acs_world_t worlds[3] = {
        FIRME_ACS_WORLD_NSEL2,
        FIRME_ACS_WORLD_SECURE,
        FIRME_ACS_WORLD_REALM,
    };
    firme_acs_call_res_t ns_pas_result = {0};
    val_fault_info_t fault_info;
    firme_acs_world_t world;
    uint64_t value;
    uint64_t ns_pas_step = fail_step_base + UINT64_C(3);
    uint32_t ns_pas_access;
    uint32_t world_index;
    uint32_t expected_ok;

    if (gpi == FIRME_GPI_NSO && !val_firme_feat_rme_gpc2_present()) {
        val_firme_gpt_record_failure(out, ns_pas_step, gpi, VAL_FALSE);
        return VAL_FALSE;
    }

    if (gpi == FIRME_GPI_NONSECURE || gpi == FIRME_GPI_NSO)
        val_cache_clean_poc((uintptr_t)addr, sizeof(uint64_t));

    for (world_index = 0U; world_index < 3U; world_index++) {
        world = worlds[world_index];
        fault_info = (val_fault_info_t){0};
        value = 0ULL;

        if (val_firme_mem_read64_world(world, addr, &value, &fault_info) != VAL_SUCCESS) {
            if (out) {
                out->fail_step = fail_step_base + world_index;
                out->last_x0 = transport_fail_magic | world_index;
                out->last_x1 = 0;
            }
            return 0U;
        }

        expected_ok = val_firme_expect_access_world(gpi, world);
        if (expected_ok != 0U) {
            if (fault_info.faulted != 0U) {
                if (out) {
                    out->fail_step = fail_step_base + world_index;
                    out->last_x0 = fault_info.esr;
                    out->last_x1 = fault_info.far;
                }
                return 0U;
            }
            if (check_value != 0U && value != expected_value) {
                if (out) {
                    out->fail_step = fail_step_base + world_index;
                    out->last_x0 = value;
                    out->last_x1 = expected_value;
                }
                return 0U;
            }
        } else {
            if (fault_info.faulted == 0U) {
                if (out) {
                    out->fail_step = fail_step_base + world_index;
                    out->last_x0 = value;
                    out->last_x1 = 0;
                }
                return 0U;
            }
            if (!val_fault_is_gpf(fault_info.esr)) {
                if (out) {
                    out->fail_step = fail_step_base + world_index;
                    out->last_x0 = fault_info.esr;
                    out->last_x1 = fault_info.far;
                }
                return 0U;
            }
        }
    }

    if (val_firme_feat_rme_gpc2_present() &&
        ((gpi == FIRME_GPI_NONSECURE) || (gpi == FIRME_GPI_NSO))) {
        if (val_firme_world_call(FIRME_ACS_WORLD_SECURE, FIRME_ACS_CMD_RUN_DIAG,
            FIRME_DIAG_READ_NS_PAS, addr, 0ULL, 0ULL, &ns_pas_result) != VAL_SUCCESS ||
            ns_pas_result.x0 != ACS_STATUS_PASS || ns_pas_result.x1 > VAL_TRUE) {
            LOG_ERROR(NULL, "gpt access: Secure NS-PAS probe setup failed at %llx",
                      (unsigned long long)addr);
            val_firme_gpt_record_failure(out, ns_pas_step,
                transport_fail_magic | UINT64_C(3), ns_pas_result.x0);
            return VAL_FALSE;
        }

        ns_pas_access = (gpi == FIRME_GPI_NONSECURE);
        if ((ns_pas_access && (ns_pas_result.x1 != 0ULL ||
             (check_value && ns_pas_result.x2 != expected_value))) ||
            (!ns_pas_access && (ns_pas_result.x1 == 0ULL ||
             !val_fault_is_gpf(ns_pas_result.x2)))) {
            LOG_ERROR(NULL, "gpt access: Secure NS-PAS mismatch gpi=%llx "
                      "faulted=%llu value/esr=%llx expected=%llx",
                      (unsigned long long)gpi, (unsigned long long)ns_pas_result.x1,
                      (unsigned long long)ns_pas_result.x2,
                      (unsigned long long)expected_value);
            val_firme_gpt_record_failure(out, ns_pas_step,
                ns_pas_result.x2, expected_value);
            return VAL_FALSE;
        }
    }

    return VAL_TRUE;
}
