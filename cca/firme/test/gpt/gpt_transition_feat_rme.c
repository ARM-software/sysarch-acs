/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"
#include "val_firme_rules.h"
#include "val_log.h"

#define GPT_TRANS_FEAT_RME_FAILSTEP_PROBE_WRITE UINT64_C(20)
#define GPT_TRANS_FEAT_RME_FAILSTEP_PROBE_READ  UINT64_C(21)
#define GPT_TRANS_FEAT_RME_FAILSTEP_PROBE_VALUE UINT64_C(22)
#define GPT_TRANS_FEAT_RME_FAILSTEP_CLEANUP     UINT64_C(30)
#define GPT_TRANS_FEAT_RME_TEST_NAME "gpt_transition_feat_rme"

/**
 * @brief Check one permitted transition without confusing rejection with retained work.
 * @param base_pa Exclusively owned scratch granule.
 * @param target_gpi Required destination GPI.
 * @param out Rule result.
 * @param step Diagnostic step.
 * @return VAL_TRUE only for terminal SUCCESS with exactly one granule completed.
 */
static uint32_t gpt_rme_permitted(uint64_t base_pa, uint64_t target_gpi,
    firme_rule_res_t *out, uint64_t step)
{
    firme_res_t result = {0};

    if (val_firme_gpt_transition_terminal(base_pa, target_gpi, &result, out, step) !=
        VAL_SUCCESS)
        return VAL_FALSE;

    if (!val_firme_status_success(result.x0, result.x1, UINT64_C(1))) {
        LOG_ERROR(NULL, "%s: target=%llx expected SUCCESS/count=1 x0=%llx x1=%llx\n",
                  GPT_TRANS_FEAT_RME_TEST_NAME, (unsigned long long)target_gpi,
                  (unsigned long long)result.x0, (unsigned long long)result.x1);
        val_firme_gpt_rule_result_fail(out, step, result.x0, result.x1);
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 * @brief Validate protected caller transitions with Host-coordinated NS observation.
 * @param base_pa Scratch checked independently by Host before and after dispatch.
 * @param caller_gpi Native caller GPI.
 * @param other_gpi Other protected-world GPI for the forbidden transition.
 * @param out Rule result; ERR prohibits reuse or competing cleanup.
 * @return VAL_SUCCESS only for PASS/SKIP; terminal failures remain VAL_ERROR.
 */
static int gpt_rme_protected_run(uint64_t base_pa, uint64_t caller_gpi,
    uint64_t other_gpi, firme_rule_res_t *out)
{
    firme_res_t result = {0};
    firme_rule_res_t cleanup = {.status = ACS_STATUS_PASS};
    val_fault_info_t fault = {0};
    uint64_t value = 0ULL;
    uint32_t caller_owned = VAL_FALSE;

    if (!out)
        return VAL_ERROR;

    val_firme_gpt_rule_result_init(out);
    if (!val_firme_feat_rme_present() || !base_pa) {
        out->status = ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }

    if (!val_firme_gpt_gpi_set_advertised(out, UINT64_C(1000)))
        return val_firme_gpt_rule_result_status(out);

    /* Step 1: Require successful NS initialization; Host separately proves its final state. */
    if (!gpt_rme_permitted(base_pa, FIRME_GPI_NONSECURE, out, UINT64_C(1)))
        goto cleanup;

    /* Step 2: Require the permitted NS-to-caller transition and native read/write access. */
    if (!gpt_rme_permitted(base_pa, caller_gpi, out, UINT64_C(2)))
        goto cleanup;

    caller_owned = VAL_TRUE;
    if (val_probe_write64(base_pa, FIRME_TEST_DATA_PATTERN, &fault) != VAL_SUCCESS) {
        val_firme_gpt_rule_result_fail(out, GPT_TRANS_FEAT_RME_FAILSTEP_PROBE_WRITE,
                                       fault.esr, fault.far);
        goto cleanup;
    }

    if (val_probe_read64(base_pa, &value, &fault) != VAL_SUCCESS) {
        val_firme_gpt_rule_result_fail(out, GPT_TRANS_FEAT_RME_FAILSTEP_PROBE_READ,
                                       fault.esr, fault.far);
        goto cleanup;
    }

    if (value != FIRME_TEST_DATA_PATTERN) {
        val_firme_gpt_rule_result_fail(out, GPT_TRANS_FEAT_RME_FAILSTEP_PROBE_VALUE,
                                       value, FIRME_TEST_DATA_PATTERN);
        goto cleanup;
    }

    /* Step 3: Require the permitted caller-to-NS transition before the negative case. */
    if (!gpt_rme_permitted(base_pa, FIRME_GPI_NONSECURE, out, UINT64_C(3)))
        goto cleanup;

    caller_owned = VAL_FALSE;

    /* Step 4: Re-establish the caller GPI and reject its transition to the other world. */
    if (!gpt_rme_permitted(base_pa, caller_gpi, out, UINT64_C(4)))
        goto cleanup;

    caller_owned = VAL_TRUE;
    if (val_firme_gpt_transition_terminal(base_pa, other_gpi, &result, out,
                                          UINT64_C(4)) != VAL_SUCCESS)
        return VAL_ERROR;

    if (!val_firme_status_deniedish(result.x0) || result.x1 != 0ULL) {
        val_firme_gpt_rule_result_fail(out, UINT64_C(4), result.x0, result.x1);
        goto cleanup;
    }

    out->status = ACS_STATUS_PASS;

cleanup:
    /* Step 5: Retained or unknown work must never be replaced with a cleanup request. */
    if (out->status == ACS_STATUS_ERR)
        return VAL_ERROR;

    /* Step 6: Restore changed storage; Host independently confirms NS before reuse. */
    if (caller_owned && !gpt_rme_permitted(base_pa, FIRME_GPI_NONSECURE,
                                           &cleanup, GPT_TRANS_FEAT_RME_FAILSTEP_CLEANUP)) {
        if (out->status == ACS_STATUS_PASS)
            *out = cleanup;

        out->status = ACS_STATUS_ERR;
        return VAL_ERROR;
    }

    return val_firme_gpt_rule_result_status(out);
}

/** @brief Validate Secure caller transitions; Host supplies and verifies NS scratch. */
int firme_gpt_transition_feat_rme_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_rme_protected_run(base_pa, FIRME_GPI_SECURE, FIRME_GPI_REALM, out);
}

/** @brief Validate Realm caller transitions; Host supplies and verifies NS scratch. */
int firme_gpt_transition_feat_rme_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_rme_protected_run(base_pa, FIRME_GPI_REALM, FIRME_GPI_SECURE, out);
}

/**
 * @brief Validate NS caller rejections and independently confirm unchanged ownership.
 * @param base_pa Owned NS scratch granule.
 * @param out Rule result preserving failures and quarantining unknown state.
 * @return VAL_SUCCESS only for PASS/SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_transition_feat_rme_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    const uint64_t targets[] = {
        FIRME_GPI_SECURE, FIRME_GPI_REALM, FIRME_GPI_ROOT, FIRME_GPI_NSO, FIRME_GPI_NSP,
    };
    firme_res_t result = {0};
    uint32_t target_index;
    uint32_t accepted;

    if (!out)
        return VAL_ERROR;

    val_firme_gpt_rule_result_init(out);
    if (!val_firme_feat_rme_present() || !base_pa) {
        out->status = ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }

    if (!val_firme_gpt_gpi_set_advertised(out, UINT64_C(1000)))
        return val_firme_gpt_rule_result_status(out);

    /* Step 1: Verify actual NS ownership rather than assigning the same GPI again. */
    if (!val_firme_gpt_confirm_ns_restored(base_pa, out, UINT64_C(10)))
        return VAL_ERROR;

    /* Step 2: Check forbidden targets and unallocated feature-dependent encodings. */
    for (target_index = 0U; target_index < sizeof(targets) / sizeof(targets[0]);
         target_index++) {
        if ((targets[target_index] == FIRME_GPI_NSO && val_firme_feat_rme_gpc2_present()) ||
            (targets[target_index] == FIRME_GPI_NSP && val_firme_feat_rme_gdi_present()))
            continue;

        if (val_firme_gpt_transition_terminal(base_pa, targets[target_index], &result,
                out, target_index + UINT64_C(1)) != VAL_SUCCESS)
            return VAL_ERROR;

        accepted = (targets[target_index] == FIRME_GPI_NSO ||
                    targets[target_index] == FIRME_GPI_NSP) ?
            val_firme_status_invalid_parameters(result.x0) :
            val_firme_status_deniedish(result.x0) && result.x1 == 0ULL;
        if (!accepted) {
            val_firme_gpt_rule_result_fail(out, target_index + UINT64_C(1),
                                           result.x0, result.x1);
            goto cleanup;
        }

        if (!val_firme_gpt_confirm_ns_restored(base_pa, out, UINT64_C(10)))
            return VAL_ERROR;
    }

    out->status = ACS_STATUS_PASS;

cleanup:
    /* Step 3: A rejected request never substitutes for independent NS restoration evidence. */
    if (!val_firme_gpt_confirm_ns_restored(base_pa, out, GPT_TRANS_FEAT_RME_FAILSTEP_CLEANUP))
        return VAL_ERROR;

    return val_firme_gpt_rule_result_status(out);
}
