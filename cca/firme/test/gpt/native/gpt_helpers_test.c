/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../../val/src/val_firme_gpt.c"
#include "../../../val/src/val_firme_gpt_l1.c"
#include "../../../val/src/val_firme_gpt_fixture.c"
#include "../../../val/src/val_firme_gpt_retained.c"
#define pal_firme_gpt_scenario_prepare native_unused_scenario_prepare
#define pal_firme_gpt_scenario_cleanup native_unused_scenario_cleanup
#define pal_firme_gpt_concurrent_info_prepare native_unused_concurrent_prepare
#define pal_firme_gpt_concurrent_info_begin native_unused_concurrent_begin
#define pal_firme_gpt_concurrent_info_finish native_unused_concurrent_finish
#define pal_firme_gpt_concurrent_info_cleanup native_unused_concurrent_cleanup
#include "../../../platform/pal_baremetal/targets/fvp/src/pal_firme_gpt.c"
#undef pal_firme_gpt_scenario_prepare
#undef pal_firme_gpt_scenario_cleanup
#undef pal_firme_gpt_concurrent_info_prepare
#undef pal_firme_gpt_concurrent_info_begin
#undef pal_firme_gpt_concurrent_info_finish
#undef pal_firme_gpt_concurrent_info_cleanup
#include "../gpt_op_continue_progress.c"
#include "../gpt_busy_retry.c"
#include "../gpt_gpi_set_progress.c"
#include "../gpt_same_gpi_transition.c"
#include "../gpt_base_alignment.c"
#include "../gpt_transition_feat_rme.c"
#include "../gpt_transition_feat_gpc2.c"
#include "../gpt_transition_feat_gdi.c"
#include "../granule_management_feature_registers.c"
#include "../granule_management_abi_compliance.c"
#include "../gpt_l1_allocation_not_found.c"
#include "../gpt_management_owner_stability.c"
#include "../gpt_operation_conflict.c"
#include "../gpt_success_count_nonzero.c"
#include "../gpt_success_granule_count.c"
#include "../gpt_granule_size.c"
#include "../gpt_start_granule.c"
#include "../gpt_denied_status.c"
#include "../gpt_invalid_parameters_status.c"
#include "../gpt_not_supported_status.c"
#include "../gpt_info_success_outputs.c"
#include "../gpt_info_output_validity.c"
#include "../gpt_info_status.c"
#include "../gpt_par_state_encoding.c"
#include "../gpt_protected_range_base_alignment.c"
#include "../gpt_protected_range_size.c"
#include "../gpt_info_concurrent_state.c"
#include "../gpt_op_continue_start_granule.c"
#include "../gpt_op_continue_granule_count.c"
#include "../gpt_stateful_incomplete.c"

#define TEST_BASE UINT64_C(0x40000000)
#define TEST_PGS UINT64_C(0x1000)
#define TEST_COOKIE UINT64_C(0x1234)
#define TEST_MAX_CALLS 64U
#define TEST_GPI_ANY UINT64_C(0xf)

typedef enum {
    TEST_SET,
    TEST_CONTINUE,
    TEST_COMPLETE,
    TEST_CREATE,
    TEST_DESTROY,
    TEST_INFO,
} test_abi_t;

typedef struct {
    test_abi_t abi;
    uint64_t base;
    uint64_t count;
    uint64_t target;
} test_call_t;

static firme_res_t responses[TEST_MAX_CALLS];
static test_call_t calls[TEST_MAX_CALLS];
static size_t response_count;
static size_t call_count;
static uint32_t cleanup_count;
static uint32_t probe_count;
static uint32_t probe_fault;
static uint32_t probe_infrastructure_error;
static int complete_result;
static uint64_t probe_accessible_count;
static uint64_t active_target_gpi;
static uint64_t feature_reg1;
static uint64_t feature_reg0;
static int64_t feature_status;
static uint32_t feature_gpc2;
static uint32_t feature_gdi;
static uint32_t allow_protected_access;
static uint32_t ignore_nso_transition;
static uint32_t change_gpi_on_error;
static uint32_t model_rme_state;
static uint32_t remote_body_active;
static uint32_t remote_rule_calls;
static uint32_t remote_probe_calls;
static uint64_t remote_result_gpi;
static uint64_t remote_result_second_gpi;
static firme_acs_world_t native_world;
static uint32_t ns_probe_transport_failure;
static uint64_t ns_probe_fault_esr;
static uint64_t current_gpi;
static uint64_t second_gpi;
static uint64_t active_base;
static uint64_t active_remaining;
static uint64_t granule_data[2U * TEST_PGS / sizeof(uint64_t)];
static size_t scrub_writes;
static size_t cleaned_writes;
static size_t clean_count;
static size_t failed_write_index;
static pal_firme_gpt_scenario_status_t fixture_status;
static pal_firme_gpt_scenario_t fixture;

/** @brief Keep unrelated Host-observer tests separate from the dedicated retained fixture. */
int val_host_gpt_retained_run(firme_rule_id_t rule, firme_acs_world_t world,
    firme_rule_res_t *out)
{
    return val_firme_gpt_retained_run(rule, world, out);
}

/** @brief Reset the scripted DUT and fixture. */
static void test_reset(void)
{
    memset(responses, 0, sizeof(responses));
    memset(calls, 0, sizeof(calls));
    memset(g_gpt_not_supported_retained, 0, sizeof(g_gpt_not_supported_retained));
    memset(g_retained_fixture, 0, sizeof(g_retained_fixture));
    memset(g_retained_lock, 0, sizeof(g_retained_lock));
    response_count = 0U;
    call_count = 0U;
    cleanup_count = 0U;
    probe_count = 0U;
    probe_fault = VAL_FALSE;
    probe_infrastructure_error = VAL_FALSE;
    complete_result = VAL_SUCCESS;
    probe_accessible_count = 0ULL;
    active_target_gpi = FIRME_GPI_NONSECURE;
    feature_reg1 = 0ULL;
    feature_reg0 = FIRME_GM_FEATURE_REG0_ALLOCATED_BITS | (UINT64_C(1) << 63);
    feature_status = FIRME_SUCCESS;
    feature_gpc2 = VAL_FALSE;
    feature_gdi = VAL_FALSE;
    allow_protected_access = VAL_FALSE;
    ignore_nso_transition = VAL_FALSE;
    change_gpi_on_error = VAL_FALSE;
    model_rme_state = VAL_FALSE;
    remote_body_active = VAL_FALSE;
    remote_rule_calls = 0U;
    remote_probe_calls = 0U;
    remote_result_gpi = UINT64_MAX;
    remote_result_second_gpi = UINT64_MAX;
    native_world = FIRME_ACS_WORLD_NSEL2;
    ns_probe_transport_failure = VAL_FALSE;
    ns_probe_fault_esr = (UINT64_C(0x25) << 26) | UINT64_C(0x28);
    current_gpi = FIRME_GPI_NONSECURE;
    second_gpi = FIRME_GPI_NONSECURE;
    active_base = TEST_BASE;
    active_remaining = 0ULL;
    memset(granule_data, 0, sizeof(granule_data));
    scrub_writes = 0U;
    cleaned_writes = 0U;
    clean_count = 0U;
    failed_write_index = SIZE_MAX;
    fixture_status = PAL_FIRME_GPT_SCENARIO_READY;
    fixture = (pal_firme_gpt_scenario_t){
        .base_pa = TEST_BASE,
        .granule_count = 4ULL,
        .target_gpi = FIRME_GPI_REALM,
        .cookie = TEST_COOKIE,
        .initial_gpi = FIRME_GPI_NONSECURE,
        .completed_count = 1ULL,
    };
}

/** @brief Queue a raw W0 status and outputs, with nonzero upper X0 bits. */
static void test_reply(int64_t status, uint64_t count, uint64_t cookie)
{
    assert(response_count < TEST_MAX_CALLS);
    responses[response_count++] = (firme_res_t){
        .x0 = UINT64_C(0xa5a5a5a500000000) | (uint32_t)status,
        .x1 = count,
        .x2 = cookie,
    };
}

/** @brief Capture an ABI call and return its scripted result. */
static void test_invoke(test_abi_t abi, uint64_t base, uint64_t count,
                        uint64_t target, firme_res_t *result)
{
    assert(call_count < response_count);
    calls[call_count] = (test_call_t){abi, base, count, target};
    *result = responses[call_count++];
}

/** @brief Stub the signed W0 transport decoder. */
int64_t val_firme_status_to_i64(uint64_t value)
{
    return (int64_t)(int32_t)(uint32_t)value;
}

/** @brief Suppress firmware logging during native assertions. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag,
                  const char *format, ...)
{
    return 0U;
}

/** @brief Provide configurable geometry and arbitrary unused output bits. */
void val_firme_service_features(uint64_t service_id, uint64_t register_index,
                                 firme_res_t *result)
{
    *result = (firme_res_t){.x0 = (uint64_t)feature_status};
    if (feature_status != FIRME_SUCCESS)
        return;
    if (register_index == FIRME_GM_FEATURE_REG0_INDEX)
        result->x1 = feature_reg0;
    else if (register_index == FIRME_GM_FEATURE_REG1_INDEX)
        result->x1 = feature_reg1;
    else
        result->x0 = (uint64_t)FIRME_NOT_SUPPORTED;
}

/** @brief Advertise a supported GM service version. */
void val_firme_service_version(uint64_t service_id, firme_res_t *result)
{
    *result = (firme_res_t){.x0 = UINT64_C(0x10000)};
}

/** @brief Supply the dispatcher-owned native GPT test window. */
uint64_t val_firme_gpt_base_for_world(firme_acs_world_t world)
{
    assert((uint32_t)world < GPT_NOT_SUPPORTED_WORLD_COUNT);
    return TEST_BASE;
}

/** @brief Advertise all requested Granule Management ABIs. */
int val_firme_service_abi_is_advertised(uint32_t service_id, uint64_t abi_mask,
                                        uint32_t *advertised, uint64_t *reg0,
                                        firme_res_t *result)
{
    *advertised = VAL_TRUE;
    *reg0 = FIRME_GM_FEATURE_REG0_ALLOCATED_BITS;
    *result = (firme_res_t){.x0 = (uint64_t)feature_status, .x1 = *reg0};
    return (feature_status == FIRME_SUCCESS) ? VAL_SUCCESS : VAL_ERROR;
}

/** @brief Model the allocated Granule Management feature bits. */
uint64_t val_firme_service_allocated_abi_mask(uint32_t service_id)
{
    return FIRME_GM_FEATURE_REG0_ALLOCATED_BITS;
}

/** @brief Model the mandatory Granule Management ABI set. */
uint64_t val_firme_service_mandatory_abi_mask(firme_acs_world_t world,
                                             uint32_t service_id)
{
    return FIRME_GM_FEATURE_REG0_GPI_SET_BIT |
           FIRME_GM_FEATURE_REG0_OP_CONTINUE_BIT |
           FIRME_GM_FEATURE_REG0_GPT_INFO_BIT;
}

/** @brief Model the Realm-only optional L1 ABI pair. */
uint64_t val_firme_service_supported_abi_mask(firme_acs_world_t world,
                                             uint32_t service_id)
{
    if (world == FIRME_ACS_WORLD_REALM)
        return FIRME_GM_FEATURE_REG0_ALLOCATED_BITS;
    return val_firme_service_mandatory_abi_mask(world, service_id);
}

/** @brief Advertise baseline RME for native rule execution. */
uint32_t val_firme_feat_rme_present(void)
{
    return VAL_TRUE;
}

/** @brief Select the GPC2 feature-present or feature-absent path. */
uint32_t val_firme_feat_rme_gpc2_present(void)
{
    return feature_gpc2;
}

/** @brief Select the GDI feature-present or feature-absent path. */
uint32_t val_firme_feat_rme_gdi_present(void)
{
    return feature_gdi;
}

/** @brief Model native PAS access independently of the observation helper's expectations. */
static uint32_t test_native_access(firme_acs_world_t world, uint64_t address)
{
    uint64_t gpi = address >= TEST_BASE + TEST_PGS ? second_gpi : current_gpi;

    return gpi == TEST_GPI_ANY ||
           (world == FIRME_ACS_WORLD_NSEL2 &&
            (gpi == FIRME_GPI_NONSECURE || gpi == FIRME_GPI_NSO)) ||
           (world == FIRME_ACS_WORLD_SECURE && gpi == FIRME_GPI_SECURE) ||
           (world == FIRME_ACS_WORLD_REALM && gpi == FIRME_GPI_REALM);
}

/** @brief Dispatch native Host bodies through the same rule entry contract as firmware. */
int val_firme_rule_run_ns(firme_rule_id_t rule, uint64_t base_pa, firme_rule_res_t *out)
{
    switch (rule) {
    case FIRME_RULE_R0092:
        return firme_gpt_same_gpi_transition_run_ns(base_pa, out);
    case FIRME_RULE_R0094:
        return firme_gpt_transition_feat_rme_run_ns(base_pa, out);
    case FIRME_RULE_R0111:
        return firme_gpt_start_granule_run_ns(base_pa, out);
    case FIRME_RULE_R0127:
        return firme_gpt_denied_status_run_ns(base_pa, out);
    default:
        return VAL_ERROR;
    }
}

/** @brief Execute protected rule bodies while rejecting nested cross-world transport. */
int val_firme_world_call(firme_acs_world_t world, firme_acs_cmd_t command,
                         uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                         firme_acs_call_res_t *result)
{
    firme_rule_res_t rule_result = {0};
    int rule_status;

    assert(!remote_body_active);
    if (command == FIRME_ACS_CMD_RUN_RULE) {
        assert(arg1 == TEST_BASE);
        assert(world == FIRME_ACS_WORLD_SECURE || world == FIRME_ACS_WORLD_REALM);
        remote_rule_calls++;
        remote_body_active = VAL_TRUE;
        native_world = world;
        switch (arg0) {
        case FIRME_RULE_R0090:
            rule_status = gpt_base_alignment_run(arg1,
                world == FIRME_ACS_WORLD_SECURE ? FIRME_GPI_SECURE : FIRME_GPI_REALM,
                &rule_result);
            break;
        case FIRME_RULE_R0092:
            rule_status = gpt_same_gpi_run(arg1,
                world == FIRME_ACS_WORLD_SECURE ? FIRME_GPI_SECURE : FIRME_GPI_REALM,
                &rule_result);
            break;
        case FIRME_RULE_R0094:
            rule_status = world == FIRME_ACS_WORLD_SECURE ?
                firme_gpt_transition_feat_rme_run_secure(arg1, &rule_result) :
                firme_gpt_transition_feat_rme_run_realm(arg1, &rule_result);
            break;
        case FIRME_RULE_R0111:
            rule_status = gpt_start_granule_run(arg1,
                world == FIRME_ACS_WORLD_SECURE ? FIRME_GPI_SECURE : FIRME_GPI_REALM,
                &rule_result);
            break;
        case FIRME_RULE_R0113:
            rule_status = gpt_success_count_run(arg1,
                world == FIRME_ACS_WORLD_SECURE ? FIRME_GPI_SECURE : FIRME_GPI_REALM,
                &rule_result);
            break;
        case FIRME_RULE_R0117:
            rule_status = gpt_gpi_set_progress_run(arg1,
                world == FIRME_ACS_WORLD_SECURE ? FIRME_GPI_SECURE : FIRME_GPI_REALM,
                &rule_result);
            break;
        case FIRME_RULE_R0120:
            rule_status = gpt_success_nonzero_run(arg1,
                world == FIRME_ACS_WORLD_SECURE ? FIRME_GPI_SECURE : FIRME_GPI_REALM,
                &rule_result);
            break;
        case FIRME_RULE_R0134:
            rule_status = gpt_management_owner_run(arg1,
                world == FIRME_ACS_WORLD_SECURE ? FIRME_GPI_SECURE : FIRME_GPI_REALM,
                &rule_result);
            break;
        default:
            assert(VAL_FALSE);
            return VAL_ERROR;
        }
        assert(rule_status == val_firme_gpt_rule_result_status(&rule_result));
        native_world = FIRME_ACS_WORLD_NSEL2;
        remote_body_active = VAL_FALSE;
        if (remote_result_gpi != UINT64_MAX)
            current_gpi = remote_result_gpi;
        if (remote_result_second_gpi != UINT64_MAX)
            second_gpi = remote_result_second_gpi;

        *result = (firme_acs_call_res_t){.x0 = rule_result.status,
            .x1 = rule_result.fail_step, .x2 = rule_result.last_x0, .x3 = rule_result.last_x1};
        return VAL_SUCCESS;
    }

    remote_probe_calls++;
    *result = (firme_acs_call_res_t){
        .x0 = ACS_STATUS_PASS,
        .x1 = 1ULL,
        .x2 = (UINT64_C(0x25) << 26) | UINT64_C(0x28),
    };
    if (model_rme_state && command == FIRME_ACS_CMD_GPT_PROBE) {
        result->x1 = !test_native_access(world, arg0);
        result->x2 = result->x1 ? ns_probe_fault_esr : granule_data[0];
    }

    if (command == FIRME_ACS_CMD_RUN_DIAG && arg0 == FIRME_DIAG_READ_NS_PAS) {
        assert(world == FIRME_ACS_WORLD_SECURE &&
               (arg1 == TEST_BASE || arg1 == TEST_BASE + TEST_PGS));
        if (ns_probe_transport_failure)
            return VAL_ERROR;
        result->x1 = ((arg1 == TEST_BASE ? current_gpi : second_gpi) != FIRME_GPI_NONSECURE);
        result->x2 = result->x1 ? ns_probe_fault_esr : granule_data[0];
    }

    return VAL_SUCCESS;
}

/** @brief Model bounded native-PAS reads without converting infrastructure errors into access. */
void val_firme_gpt_probe_command(uint64_t address, firme_acs_call_res_t *response)
{
    val_fault_info_t fault = {0};
    uint64_t value = 0ULL;
    int status;

    status = val_probe_read64(address, &value, &fault);
    *response = (firme_acs_call_res_t){
        .x0 = (status == VAL_SUCCESS || fault.faulted) ? ACS_STATUS_PASS : ACS_STATUS_ERR,
        .x1 = fault.faulted,
        .x2 = fault.faulted ? fault.esr : value,
    };
}

/** @brief Model GPI effects separately from firmware return status, including a no-op bug. */
static void test_apply_gpi_result(uint64_t base, uint64_t count, uint64_t target,
    const firme_res_t *result)
{
    uint64_t index;
    int64_t status = val_firme_status_to_i64(result->x0);

    if (model_rme_state) {
        if (val_firme_gpt_operation_status_valid(status) && status != FIRME_BUSY &&
            result->x1 <= count) {
            for (index = 0ULL; index < result->x1; index++) {
                if (base + index * TEST_PGS == TEST_BASE)
                    current_gpi = target;
                else if (base + index * TEST_PGS == TEST_BASE + TEST_PGS)
                    second_gpi = target;
            }
        }

        return;
    }

    if (change_gpi_on_error && val_firme_status_to_i64(result->x0) == FIRME_DENIED) {
        current_gpi = target;
        return;
    }

    if (!(feature_gpc2 || feature_gdi || model_rme_state) ||
        !val_firme_status_success(result->x0, result->x1, count))
        return;

    if (current_gpi == FIRME_GPI_NSO && target == FIRME_GPI_NONSECURE) {
        assert(clean_count != 0U && cleaned_writes == scrub_writes);
        assert(scrub_writes >= TEST_PGS / sizeof(uint64_t));
    }

    if (!(ignore_nso_transition && target == FIRME_GPI_NSO))
        current_gpi = target;
}

/** @brief Return one scripted GPI_SET response. */
void val_firme_gm_gpi_set_do(uint64_t base, uint64_t count, uint64_t target,
                             firme_res_t *result)
{
    test_invoke(TEST_SET, base, count, target, result);
    test_apply_gpi_result(base, count, target, result);
    active_target_gpi = target;
    active_base = base;
    active_remaining = count;
    if (val_firme_gpt_operation_status_valid(val_firme_status_to_i64(result->x0)) &&
        val_firme_status_to_i64(result->x0) != FIRME_BUSY && result->x1 <= count) {
        active_base += result->x1 * TEST_PGS;
        active_remaining -= result->x1;
    }
    if (target == FIRME_GPI_NONSECURE && val_firme_status_to_i64(result->x0) == FIRME_SUCCESS)
        probe_accessible_count = 0ULL;
    if ((target == FIRME_GPI_REALM) &&
        (val_firme_status_to_i64(result->x0) == FIRME_SUCCESS ||
         val_firme_status_to_i64(result->x0) == FIRME_INCOMPLETE))
        probe_accessible_count += result->x1;
}

/** @brief Forward raw fixture calls to the existing scripted native transport. */
void val_firme_gm_gpi_set(uint64_t base, uint64_t count, uint64_t attributes,
                          firme_res_t *result)
{
    val_firme_gm_gpi_set_do(base, count, attributes, result);
}

/** @brief Return one scripted continuation response. */
void val_firme_gm_gpi_op_continue(uint64_t cookie, firme_res_t *result)
{
    test_invoke(TEST_CONTINUE, cookie, 0ULL, 0ULL, result);
    test_apply_gpi_result(active_base, active_remaining, active_target_gpi, result);
    if (val_firme_gpt_operation_status_valid(val_firme_status_to_i64(result->x0)) &&
        val_firme_status_to_i64(result->x0) != FIRME_BUSY && result->x1 <= active_remaining) {
        active_base += result->x1 * TEST_PGS;
        active_remaining -= result->x1;
    }
    if (active_target_gpi == FIRME_GPI_REALM &&
        (val_firme_status_to_i64(result->x0) == FIRME_SUCCESS ||
         val_firme_status_to_i64(result->x0) == FIRME_INCOMPLETE))
        probe_accessible_count += result->x1;
}

/** @brief Return a scripted composite result and update local probe state. */
int val_firme_gm_gpi_set_complete(uint64_t base, uint64_t count,
                                  uint64_t target, firme_res_t *result)
{
    test_invoke(TEST_COMPLETE, base, count, target, result);
    if (complete_result != VAL_SUCCESS)
        return complete_result;
    test_apply_gpi_result(base, count, target, result);
    if ((target == FIRME_GPI_NONSECURE) &&
        (val_firme_status_to_i64(result->x0) == FIRME_SUCCESS))
        probe_accessible_count = 0ULL;
    if ((target == FIRME_GPI_REALM) &&
        (val_firme_status_to_i64(result->x0) == FIRME_SUCCESS))
        probe_accessible_count = result->x1;
    return VAL_SUCCESS;
}

/** @brief Return a scripted L1 create response. */
void val_firme_gm_l1_gpt_create(uint64_t base, uint64_t object,
                                firme_res_t *result)
{
    test_invoke(TEST_CREATE, base, object, 0ULL, result);
}

/** @brief Return a scripted L1 destroy response. */
void val_firme_gm_l1_gpt_destroy(uint64_t base, firme_res_t *result)
{
    test_invoke(TEST_DESTROY, base, 0ULL, 0ULL, result);
}

/** @brief Return a scripted range-query response. */
void val_firme_gm_gpt_info(uint64_t base, uint64_t top, firme_res_t *result)
{
    test_invoke(TEST_INFO, base, top, 0ULL, result);
}

/** @brief Accept test-output poisoning without a firmware transport. */
int val_firme_test_output_poison_enable(uint64_t fid, const firme_res_t *mask,
                                        const firme_res_t *value)
{
    return VAL_SUCCESS;
}

/** @brief End native test-output poisoning. */
void val_firme_test_output_poison_disable(void)
{
}

/** @brief Report no concurrent GPT_INFO fixture in the native harness. */
pal_firme_gpt_scenario_status_t pal_firme_gpt_concurrent_info_prepare(
    uint32_t world, uint64_t base, pal_firme_gpt_concurrent_info_t *evidence)
{
    return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
}

/** @brief Reject starting an unprepared native concurrency fixture. */
pal_firme_gpt_scenario_status_t pal_firme_gpt_concurrent_info_begin(
    uint32_t world, pal_firme_gpt_concurrent_info_t *evidence)
{
    return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
}

/** @brief Reject finishing an unprepared native concurrency fixture. */
pal_firme_gpt_scenario_status_t pal_firme_gpt_concurrent_info_finish(
    uint32_t world, pal_firme_gpt_concurrent_info_t *evidence)
{
    return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
}

/** @brief Reject releasing an unprepared native concurrency fixture. */
pal_firme_gpt_scenario_status_t pal_firme_gpt_concurrent_info_cleanup(
    uint32_t world, const pal_firme_gpt_concurrent_info_t *evidence)
{
    return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
}

/** @brief Model a local read and a controllable GPF prefix. */
int val_probe_read64(uint64_t address, uint64_t *value, val_fault_info_t *fault)
{
    probe_count++;
    *value = FIRME_TEST_DATA_PATTERN;
    *fault = (val_fault_info_t){0};
    if (probe_infrastructure_error)
        return VAL_ERROR;
    if ((feature_gpc2 || feature_gdi || model_rme_state) &&
        (address >= TEST_BASE) && (address < TEST_BASE + 2U * TEST_PGS))
        *value = granule_data[(address - TEST_BASE) / sizeof(uint64_t)];
    if ((model_rme_state && !test_native_access(native_world, address)) ||
        ((probe_fault == VAL_TRUE) &&
         (address >= TEST_BASE + probe_accessible_count * TEST_PGS)) ||
        (!allow_protected_access && (address == TEST_BASE) &&
         ((current_gpi == FIRME_GPI_NSP) || (current_gpi == FIRME_GPI_SA)))) {
        fault->faulted = 1ULL;
        fault->esr = (UINT64_C(0x25) << 26) | UINT64_C(0x28);
        fault->far = address;
        return VAL_ERROR;
    }
    return VAL_SUCCESS;
}

/** @brief Model successful local data writes. */
int val_probe_write64(uint64_t address, uint64_t value, val_fault_info_t *fault)
{
    *fault = (val_fault_info_t){0};
    if (scrub_writes == failed_write_index ||
        (model_rme_state && !test_native_access(native_world, address))) {
        fault->faulted = VAL_TRUE;
        fault->esr = (UINT64_C(0x25) << 26) | UINT64_C(0x28);
        fault->far = address;
        return VAL_ERROR;
    }

    if ((address >= TEST_BASE) && (address < TEST_BASE + 2U * TEST_PGS))
        granule_data[(address - TEST_BASE) / sizeof(uint64_t)] = value;
    if (value == 0ULL)
        scrub_writes++;
    return VAL_SUCCESS;
}

/** @brief Require a whole-granule scrub before the caller performs its PoC clean. */
void val_cache_clean_poc(uintptr_t address, size_t size)
{
    if ((address == TEST_BASE || address == TEST_BASE + TEST_PGS) && size == sizeof(uint64_t))
        return;
    assert((address == TEST_BASE || address == TEST_BASE + TEST_PGS) && size == TEST_PGS);
    assert(scrub_writes >= TEST_PGS / sizeof(uint64_t));
    cleaned_writes = scrub_writes;
    clean_count++;
}

/** @brief Supply a prepared fixture without manipulating real memory. */
pal_firme_gpt_scenario_status_t pal_firme_gpt_scenario_prepare(
    uint32_t world, pal_firme_gpt_abi_t abi, int64_t expected_status,
    uint64_t default_base, pal_firme_gpt_scenario_t *scenario)
{
    *scenario = fixture;
    if (abi == PAL_FIRME_GPT_ABI_GPI_SET)
        scenario->completed_count = 0ULL;
    return fixture_status;
}

/** @brief Record that a prepared fixture was released. */
pal_firme_gpt_scenario_status_t pal_firme_gpt_scenario_cleanup(
    uint32_t world, pal_firme_gpt_abi_t abi, int64_t expected_status,
    const pal_firme_gpt_scenario_t *scenario)
{
    cleanup_count++;
    return PAL_FIRME_GPT_SCENARIO_READY;
}

/** @brief Check BUSY cookies, per-call progress, and stateful/stateless handoff. */
static void test_trace(void)
{
    val_firme_gpt_trace_t trace;
    firme_rule_res_t out;
    int64_t failures[] = {FIRME_DENIED, FIRME_OP_CONFLICT, FIRME_NOT_FOUND};
    size_t status_index;

    test_reset();
    test_reply(FIRME_BUSY, UINT64_MAX, UINT64_MAX);
    test_reply(FIRME_INCOMPLETE, 1ULL, 11ULL);
    test_reply(FIRME_BUSY, UINT64_MAX, 12ULL);
    test_reply(FIRME_INCOMPLETE, 1ULL, 13ULL);
    test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
    test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
    assert(val_firme_gpt_trace_transition(TEST_BASE, 4ULL, FIRME_GPI_REALM,
        VAL_FALSE, &out, 1ULL, &trace) == VAL_SUCCESS);
    assert(!trace.malformed && !trace.exhausted && !trace.unexpected_status);
    assert(trace.completed_count == 4ULL && call_count == response_count);
    assert(calls[0].base == TEST_BASE && calls[1].base == TEST_BASE);
    assert(calls[1].count == 4ULL);
    assert(calls[2].abi == TEST_CONTINUE && calls[2].base == 11ULL);
    assert(calls[3].base == 12ULL && calls[4].base == 13ULL);
    assert(trace.phases[2].start_granule == 1ULL);
    assert(trace.phases[3].start_granule == 1ULL);
    assert(calls[5].abi == TEST_SET && calls[5].count == 1ULL);
    assert(calls[5].base == TEST_BASE + 3ULL * TEST_PGS);

    for (status_index = 0U; status_index < 3U; status_index++) {
        test_reset();
        test_reply(failures[status_index], 4ULL, UINT64_MAX);
        assert(val_firme_gpt_trace_transition(TEST_BASE, 4ULL, FIRME_GPI_REALM,
            VAL_FALSE, &out, 1ULL, &trace) == VAL_SUCCESS);
        assert(trace.malformed == VAL_TRUE && trace.completed_count == 0ULL);
        test_reset();
        test_reply(failures[status_index], 2ULL, UINT64_MAX);
        assert(val_firme_gpt_trace_transition(TEST_BASE, 4ULL, FIRME_GPI_REALM,
            VAL_FALSE, &out, 1ULL, &trace) == VAL_SUCCESS);
        assert(trace.malformed == VAL_FALSE && trace.completed_count == 2ULL);
    }

    test_reset();
    assert(val_firme_gpt_trace_transition(TEST_BASE, UINT64_MAX, FIRME_GPI_REALM,
        VAL_FALSE, &out, 1ULL, &trace) == VAL_ERROR);
    assert(val_firme_gpt_trace_transition(TEST_BASE + 1ULL, 1ULL, FIRME_GPI_REALM,
        VAL_FALSE, &out, 1ULL, &trace) == VAL_ERROR);
    assert(call_count == 0U);
    trace.phase_count = VAL_FIRME_GPT_TRACE_MAX_PHASES + 1U;
    assert(val_firme_gpt_trace_has_status(&trace, VAL_FIRME_GPT_PHASE_GPI_SET,
                                        FIRME_BUSY) == VAL_FALSE);
}

/** @brief Accept BUSY/INCOMPLETE progress and drain a terminal error before rollback. */
static void test_success_count_continuation(void)
{
    firme_rule_res_t out;

    test_reset();
    probe_fault = VAL_TRUE;
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_BUSY, UINT64_MAX, UINT64_MAX);
    test_reply(FIRME_INCOMPLETE, 0ULL, 11ULL);
    test_reply(FIRME_BUSY, UINT64_MAX, 12ULL);
    test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
    test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_success_granule_count_run_realm(TEST_BASE, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_PASS && call_count == response_count);
    assert(calls[2].abi == TEST_SET && calls[3].abi == TEST_SET);
    assert(calls[4].abi == TEST_CONTINUE && calls[4].base == 11ULL);
    assert(calls[5].abi == TEST_CONTINUE && calls[5].base == 12ULL);
    assert(calls[6].abi == TEST_SET && calls[6].base == TEST_BASE + TEST_PGS);

    test_reset();
    probe_fault = VAL_TRUE;
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_INCOMPLETE, 0ULL, 21ULL);
    test_reply(FIRME_DENIED, 0ULL, UINT64_MAX);
    assert(firme_gpt_success_granule_count_run_realm(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL && out.fail_step == GPT_SUCCESS_COUNT_STEP_RESULT);
    assert(call_count == response_count && calls[3].base == 21ULL);

    test_reset();
    probe_fault = VAL_TRUE;
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_INCOMPLETE, 0ULL, 31ULL);
    test_reply(FIRME_NOT_SUPPORTED, 0ULL, UINT64_MAX);
    assert(firme_gpt_success_granule_count_run_realm(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_ERR && out.fail_step == GPT_SUCCESS_COUNT_STEP_RESULT);
    assert(call_count == response_count && calls[3].base == 31ULL);
}

/** @brief Check GPT_INFO bounds without requiring a maximal prefix. */
static void test_info(void)
{
    val_firme_gpt_info_window_t window = {
        .range_base = TEST_BASE,
        .range_top = 3ULL * TEST_BASE,
        .range_size = TEST_BASE,
        .granule_size = TEST_PGS,
        .protected_pa_size = 4ULL * TEST_BASE,
    };
    firme_res_t result = {.x0 = FIRME_SUCCESS, .x1 = 2ULL * TEST_BASE, .x2 = 3ULL};
    firme_rule_res_t out;

    assert(val_firme_gpt_info_success_valid(&window, &result, &out, 1ULL));
    result.x1 = window.range_base;
    assert(!val_firme_gpt_info_success_valid(&window, &result, &out, 1ULL));
    result.x1 = 4ULL * TEST_BASE;
    assert(!val_firme_gpt_info_success_valid(&window, &result, &out, 1ULL));
    result.x1 = 2ULL * TEST_BASE + TEST_PGS;
    assert(!val_firme_gpt_info_success_valid(&window, &result, &out, 1ULL));
    result.x1 = 2ULL * TEST_BASE;
    result.x2 = FIRME_GPT_PAR_STATE_MAX + 1ULL;
    assert(!val_firme_gpt_info_success_valid(&window, &result, &out, 1ULL));
    result.x2 = 3ULL;
    window.range_size = 0ULL;
    assert(!val_firme_gpt_info_success_valid(&window, &result, &out, 1ULL));
}

/** @brief Check retained suffix bounds and UNKNOWN BUSY count handling. */
static void test_continue(void)
{
    firme_rule_res_t out;

    test_reset();
    test_reply(FIRME_SUCCESS, 4ULL, UINT64_MAX);
    assert(gpt_op_continue_progress_run(FIRME_ACS_WORLD_NSEL2,
                                        TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL && cleanup_count == 1U);

    test_reset();
    test_reply(FIRME_SUCCESS, 3ULL, UINT64_MAX);
    test_reply(FIRME_DENIED, 2ULL, UINT64_MAX);
    test_reply(FIRME_OP_CONFLICT, 0ULL, UINT64_MAX);
    test_reply(FIRME_NOT_FOUND, 1ULL, UINT64_MAX);
    test_reply(FIRME_INCOMPLETE, 1ULL, 55ULL);
    test_reply(FIRME_SUCCESS, 2ULL, UINT64_MAX);
    test_reply(FIRME_BUSY, UINT64_MAX, 66ULL);
    test_reply(FIRME_SUCCESS, 3ULL, UINT64_MAX);
    assert(gpt_op_continue_progress_run(FIRME_ACS_WORLD_NSEL2,
                                        TEST_BASE, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_PASS && cleanup_count == 6U);
    assert(calls[5].base == 55ULL && calls[7].base == 66ULL);

    test_reset();
    test_reply(FIRME_BUSY, UINT64_MAX, UINT64_MAX);
    test_reply(FIRME_SUCCESS, 4ULL, UINT64_MAX);
    test_reply(FIRME_BUSY, UINT64_MAX, 77ULL);
    test_reply(FIRME_SUCCESS, 3ULL, UINT64_MAX);
    assert(gpt_busy_retry_run(FIRME_ACS_WORLD_NSEL2, TEST_BASE, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_PASS && cleanup_count == 2U);
    assert(calls[1].base == TEST_BASE && calls[1].count == 4ULL);
    assert(calls[3].abi == TEST_CONTINUE && calls[3].base == 77ULL);

    assert(!val_firme_gpt_operation_count_valid(FIRME_SUCCESS, 0ULL, 4ULL));
    assert(!val_firme_gpt_operation_count_valid(FIRME_INCOMPLETE, 4ULL, 4ULL));
    assert(val_firme_gpt_operation_count_valid(FIRME_INCOMPLETE, 0ULL, 4ULL));
    assert(val_firme_gpt_operation_count_valid(FIRME_BUSY, UINT64_MAX, 4ULL));
}

/** @brief Check L1 input geometry and zero-base continuation across BUSY. */
static void test_l1(void)
{
    pal_firme_gpt_l1_scenario_t scenario = {
        .range_base = TEST_BASE,
        .range_size = TEST_BASE,
        .object_base = TEST_BASE,
        .object_size = TEST_BASE / TEST_PGS / 2ULL,
        .granule_size = TEST_PGS,
        .range_initial_gpi = FIRME_GPI_NONSECURE,
        .object_initial_gpi = FIRME_GPI_NONSECURE,
        .layout = PAL_FIRME_GPT_L1_LAYOUT_SELF_DESCRIBING,
    };
    val_firme_gpt_l1_trace_t trace;
    firme_rule_res_t out;

    test_reset();
    assert(val_firme_gpt_l1_validate_scenario(FIRME_RULE_R0170,
        PAL_FIRME_GPT_L1_CONDITION_VALID_SELF, &scenario, &out) ==
        VAL_FIRME_GPT_L1_FLOW_PASS);
    scenario.layout = (pal_firme_gpt_l1_layout_t)2;
    assert(val_firme_gpt_l1_validate_scenario(FIRME_RULE_R0170,
        PAL_FIRME_GPT_L1_CONDITION_VALID_SELF, &scenario, &out) ==
        VAL_FIRME_GPT_L1_FLOW_ERROR);
    scenario.layout = PAL_FIRME_GPT_L1_LAYOUT_SELF_DESCRIBING;
    scenario.object_base = 2ULL * TEST_BASE - scenario.object_size / 2ULL;
    assert(val_firme_gpt_l1_validate_scenario(FIRME_RULE_R0167,
        PAL_FIRME_GPT_L1_CONDITION_OBJECT_MISALIGNED, &scenario, &out) ==
        VAL_FIRME_GPT_L1_FLOW_ERROR);
    scenario.object_base = UINT64_MAX - scenario.object_size + 1ULL;
    assert(val_firme_gpt_l1_validate_scenario(FIRME_RULE_R0167,
        PAL_FIRME_GPT_L1_CONDITION_OBJECT_OUTSIDE_PPS, &scenario, &out) ==
        VAL_FIRME_GPT_L1_FLOW_ERROR);

    scenario.object_base = TEST_BASE;
    test_reply(FIRME_BUSY, UINT64_MAX, UINT64_MAX);
    test_reply(FIRME_INCOMPLETE, UINT64_MAX, UINT64_MAX);
    test_reply(FIRME_BUSY, UINT64_MAX, UINT64_MAX);
    test_reply(FIRME_SUCCESS, UINT64_MAX, UINT64_MAX);
    assert(val_firme_gpt_l1_run_to_terminal(FIRME_RULE_R0170,
        PAL_FIRME_GPT_L1_CREATE, &scenario, FIRME_SUCCESS, &trace, &out) ==
        VAL_FIRME_GPT_L1_FLOW_PASS);
    assert(calls[0].count == TEST_BASE && calls[1].count == TEST_BASE);
    assert(calls[2].count == 0ULL && calls[3].count == 0ULL);
    assert(trace.saw_incomplete == VAL_TRUE);
}

/** @brief Check cleanup failures and cleanup after a malformed transition. */
static void test_cleanup(void)
{
    firme_rule_res_t out;

    test_reset();
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    assert(firme_gpt_same_gpi_transition_run_realm(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL && out.fail_step == GPT_SAME_GPI_STEP_CLEANUP);

    test_reset();
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_base_alignment_run_realm(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_ERR && call_count == 1U);

    test_reset();
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    assert(firme_gpt_transition_feat_rme_run_realm(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL && out.fail_step == 4ULL);
    assert(call_count == 4U && calls[3].target == FIRME_GPI_REALM);

    test_reset();
    failed_write_index = 2U;
    test_reply(FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
    assert(firme_gpt_transition_feat_gpc2_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_ERR);
    assert(out.fail_step == GPT_TRANS_FEAT_GPC2_CLEANUP_STEP_BASE);

    test_reset();
    failed_write_index = 2U;
    test_reply(FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
    test_reply(FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
    assert(firme_gpt_transition_feat_gdi_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_ERR);
    assert(out.fail_step == GPT_TRANS_FEAT_GDI_CLEANUP_STEP);

    test_reset();
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    assert(firme_gpt_transition_feat_rme_run_secure(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL && out.fail_step == 1ULL);
    assert(val_firme_status_to_i64(out.last_x0) == FIRME_DENIED);
    assert(call_count == response_count);

    /* Step 1: A second cleanup failure preserves the first error's diagnostics. */
    test_reset();
    test_reply(FIRME_INVALID_PARAMETERS, 0ULL, 0ULL);
    assert(!val_firme_gpt_restore_ns(TEST_BASE, &out, GPT_SAME_GPI_STEP_CLEANUP));
    assert(out.status == ACS_STATUS_ERR && out.fail_step == 1ULL);
    assert(val_firme_status_to_i64(out.last_x0) == FIRME_DENIED);

    /* Step 2: Successful cleanup cannot erase an existing quarantine result. */
    test_reset();
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(val_firme_gpt_restore_ns(TEST_BASE, &out, GPT_SAME_GPI_STEP_CLEANUP));
    assert(out.status == ACS_STATUS_ERR && out.fail_step == 1ULL);

    /* Step 3: An optional result pointer must not hide restoration failure. */
    test_reset();
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    assert(!val_firme_gpt_restore_ns(TEST_BASE, NULL, GPT_SAME_GPI_STEP_CLEANUP));
}

/** @brief Accept MAX_CONCURRENT_OP_CNT and ignore unused feature outputs. */
static void test_features(void)
{
    firme_rule_res_t out;

    test_reset();
    feature_reg1 = (UINT64_C(1) << 63) | (UINT64_C(2) << 9);
    assert(gm_feature_registers_run(FIRME_ACS_WORLD_REALM, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_PASS);
    feature_reg1 |= UINT64_C(1) << FIRME_GM_FEATURE_REG1_L0GPTSZ_SHIFT;
    assert(gm_feature_registers_run(FIRME_ACS_WORLD_REALM, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL);
    assert(out.fail_step == GM_FEATURE_REGISTERS_STEP_REG1_RANGE);
    feature_reg1 = UINT64_C(7) << FIRME_GM_FEATURE_REG1_PPS_SHIFT;
    assert(gm_feature_registers_run(FIRME_ACS_WORLD_REALM, &out) == VAL_ERROR);
    assert(out.fail_step == GM_FEATURE_REGISTERS_STEP_REG1_PPS);
    assert(gm_abi_compliance_run(FIRME_ACS_WORLD_REALM, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_PASS);
    feature_reg0 &= ~FIRME_GM_FEATURE_REG0_GPT_INFO_BIT;
    assert(gm_feature_registers_run(FIRME_ACS_WORLD_REALM, &out) == VAL_ERROR);
    assert(out.fail_step == GM_FEATURE_REGISTERS_STEP_REG0_BITS);
    assert(gm_abi_compliance_run(FIRME_ACS_WORLD_REALM, &out) == VAL_ERROR);
    assert(out.fail_step == GM_ABI_COMPLIANCE_STEP_MANDATORY_BITS);

    test_reset();
    assert(gm_feature_registers_run(FIRME_ACS_WORLD_NSEL2, &out) == VAL_ERROR);
    assert(out.fail_step == GM_FEATURE_REGISTERS_STEP_REG0_BITS);
    feature_reg0 &= ~(FIRME_GM_FEATURE_REG0_L1_CREATE_BIT |
                      FIRME_GM_FEATURE_REG0_L1_DESTROY_BIT);
    assert(gm_feature_registers_run(FIRME_ACS_WORLD_NSEL2, &out) == VAL_SUCCESS);
}

/** @brief Require observed setup, denied zero progress and safe cleanup for R0127. */
static void test_denied_preconditions(void)
{
    firme_rule_res_t out;

    test_reset();
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    assert(firme_gpt_denied_status_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.fail_step == GPT_DENIED_STATUS_STEP_SETUP);
    assert(call_count == response_count);
    assert(calls[0].target == FIRME_GPI_NONSECURE);

    test_reset();
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_DENIED, 1ULL, 0ULL);
    assert(firme_gpt_denied_status_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.fail_step == GPT_DENIED_STATUS_STEP_RESULT);
    assert(call_count == response_count);

    test_reset();
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    assert(firme_gpt_denied_status_run_ns(TEST_BASE, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_PASS && probe_count == 3U);
    assert(call_count == response_count);

    test_reset();
    ns_probe_transport_failure = feature_gpc2 = VAL_TRUE;
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_denied_status_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_ERR);
}

/** @brief Check bounded progress, forbidden transitions, and unknown-state safety. */
static void test_denied_progress(void)
{
    firme_rule_res_t out;

    test_reset();
    probe_fault = VAL_TRUE;
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 2ULL, 0ULL);
    test_reply(FIRME_DENIED, 1ULL, UINT64_MAX);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_gpi_set_progress_run_realm(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL);
    assert(out.fail_step == GPT_GPI_SET_PROGRESS_STEP_DENIED);
    assert(call_count == response_count);

    test_reset();
    probe_fault = VAL_TRUE;
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 2ULL, 0ULL);
    test_reply(FIRME_DENIED, 0ULL, UINT64_MAX);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_gpi_set_progress_run_realm(TEST_BASE, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_PASS && call_count == response_count);

    test_reset();
    probe_fault = VAL_TRUE;
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_BUSY, UINT64_MAX, UINT64_MAX);
    test_reply(FIRME_INCOMPLETE, 1ULL, TEST_COOKIE);
    test_reply(FIRME_BUSY, UINT64_MAX, TEST_COOKIE + 1ULL);
    test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
    test_reply(FIRME_DENIED, 0ULL, UINT64_MAX);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_gpi_set_progress_run_realm(TEST_BASE, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_PASS && call_count == response_count);
    assert(calls[4].abi == TEST_CONTINUE && calls[4].base == TEST_COOKIE);
    assert(calls[5].abi == TEST_CONTINUE && calls[5].base == TEST_COOKIE + 1ULL);

    test_reset();
    probe_fault = VAL_TRUE;
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_INCOMPLETE, 0ULL, TEST_COOKIE);
    test_reply(FIRME_NOT_SUPPORTED, UINT64_MAX, UINT64_MAX);
    assert(firme_gpt_gpi_set_progress_run_realm(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_ERR && call_count == response_count);

    test_reset();
    probe_fault = VAL_TRUE;
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    assert(gpt_gpi_set_progress_prepare_ns(TEST_BASE, &out, 1ULL) == VAL_FALSE);
}

/** @brief Check every touched rule entry rejects null output and discovery failure. */
static void test_rule_return_contract(void)
{
    int (*const entries[])(uint64_t, firme_rule_res_t *) = {
        firme_gpt_base_alignment_run_realm,
        firme_gpt_busy_retry_run_ns,
        firme_gpt_gpi_set_progress_run_realm,
        firme_gpt_l1_allocation_not_found_run_ns,
        firme_gpt_management_owner_stability_run_realm,
        firme_gpt_op_continue_progress_run_ns,
        firme_gpt_operation_conflict_run_ns,
        firme_gpt_same_gpi_transition_run_realm,
        firme_gpt_success_count_nonzero_run_realm,
        firme_gpt_success_granule_count_run_realm,
        firme_gpt_transition_feat_gdi_run_ns,
        firme_gpt_transition_feat_gpc2_run_ns,
        firme_gpt_transition_feat_rme_run_ns,
        firme_gpt_transition_feat_rme_run_secure,
        firme_gpt_transition_feat_rme_run_realm,
        firme_gpt_granule_management_abi_compliance_run_realm,
        firme_gpt_granule_management_feature_registers_run_realm,
        firme_gpt_granule_size_run_ns,
        firme_gpt_start_granule_run_realm,
        firme_gpt_denied_status_run_ns,
        firme_gpt_invalid_parameters_status_run_ns,
        firme_gpt_not_supported_status_run_ns,
        firme_gpt_info_success_outputs_run_ns,
        firme_gpt_info_output_validity_run_ns,
        firme_gpt_info_status_run_ns,
        firme_gpt_par_state_encoding_run_ns,
        firme_gpt_protected_range_base_alignment_run_ns,
        firme_gpt_protected_range_size_run_ns,
        firme_gpt_info_concurrent_state_run_ns,
        firme_gpt_op_continue_start_granule_run_ns,
        firme_gpt_op_continue_granule_count_run_ns,
        firme_gpt_stateful_incomplete_run_ns,
    };
    firme_rule_res_t out;
    size_t entry_index;

    for (entry_index = 0U; entry_index < sizeof(entries) / sizeof(entries[0]);
         entry_index++) {
        test_reset();
        assert(entries[entry_index](TEST_BASE, NULL) == VAL_ERROR);
        assert(call_count == 0U);
        feature_status = FIRME_INVALID_PARAMETERS;
        out = (firme_rule_res_t){.status = ACS_STATUS_PASS};
        assert(entries[entry_index](TEST_BASE, &out) == VAL_ERROR);
        assert(out.status == ACS_STATUS_FAIL && call_count == 0U);
    }

    test_reset();
    assert(firme_gpt_base_alignment_run_realm(TEST_BASE + 1ULL, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL && call_count == 0U);
    assert(firme_gpt_base_alignment_run_realm(0ULL, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_SKIP);
    fixture_status = PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
    assert(firme_gpt_busy_retry_run_ns(TEST_BASE, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_SKIP && call_count == 0U);
    assert(firme_gpt_start_granule_run_ns(TEST_BASE, NULL) == VAL_ERROR);
    assert(firme_gpt_start_granule_run_ns(TEST_BASE, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_SKIP);

    assert(val_firme_gpt_l1_complete(VAL_FIRME_GPT_L1_FLOW_PASS, NULL) == VAL_ERROR);
    assert(val_firme_gpt_l1_complete(VAL_FIRME_GPT_L1_FLOW_PASS, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_PASS);
    assert(val_firme_gpt_l1_complete(VAL_FIRME_GPT_L1_FLOW_FAIL, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL);
    assert(val_firme_gpt_l1_complete(VAL_FIRME_GPT_L1_FLOW_ERROR, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_ERR);
    out.status = ACS_STATUS_SKIP;
    assert(val_firme_gpt_l1_complete(VAL_FIRME_GPT_L1_FLOW_SKIP, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_SKIP);
}

/** @brief Require INVALID_PARAMETERS for GPIs unallocated without GPC2/GDI. */
static void test_absent_feature_status(void)
{
    firme_rule_res_t out;

    test_reset();
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    test_reply(FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
    test_reply(FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
    assert(firme_gpt_transition_feat_rme_run_ns(TEST_BASE, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_PASS && call_count == response_count);

    test_reset();
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    test_reply(FIRME_NOT_SUPPORTED, UINT64_MAX, UINT64_MAX);
    assert(firme_gpt_transition_feat_rme_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL && out.fail_step == 4ULL);
    assert(call_count == response_count);

    test_reset();
    test_reply(FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
    assert(firme_gpt_transition_feat_gpc2_run_ns(TEST_BASE, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_PASS && call_count == response_count);
    test_reset();
    test_reply(FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
    test_reply(FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
    assert(firme_gpt_transition_feat_gdi_run_ns(TEST_BASE, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_PASS && call_count == response_count);
}

/** @brief Exercise every positive GDI edge and reject a failure at any transition. */
static void test_present_feature_transitions(void)
{
    const size_t transition_count = 9U;
    firme_rule_res_t out;
    size_t failed_transition;
    size_t transition_index;
    size_t word_index;

    test_reset();
    feature_gpc2 = VAL_TRUE;
    for (transition_index = 0U; transition_index < 2U; transition_index++)
        test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_transition_feat_gpc2_run_ns(TEST_BASE, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_PASS && call_count == response_count);
    assert(clean_count == 2U);
    for (word_index = 0U; word_index < TEST_PGS / sizeof(uint64_t); word_index++)
        assert(granule_data[word_index] == 0ULL);

    for (failed_transition = 0U; failed_transition <= transition_count; failed_transition++) {
        test_reset();
        feature_gpc2 = VAL_TRUE;
        feature_gdi = VAL_TRUE;
        for (transition_index = 0U; transition_index < failed_transition; transition_index++)
            test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
        if (failed_transition < transition_count)
            test_reply(FIRME_DENIED, 0ULL, 0ULL);
        if (failed_transition != 0U && failed_transition != 2U && failed_transition != 4U)
            test_reply(FIRME_SUCCESS, 1ULL, 0ULL);

        if (failed_transition < transition_count) {
            assert(firme_gpt_transition_feat_gdi_run_ns(TEST_BASE, &out) == VAL_ERROR);
            assert(out.status == ACS_STATUS_FAIL);
            assert(out.fail_step == GPT_TRANS_FEAT_GDI_TRANSITION_STEP + failed_transition);
        } else {
            assert(firme_gpt_transition_feat_gdi_run_ns(TEST_BASE, &out) == VAL_SUCCESS);
            assert(out.status == ACS_STATUS_PASS && clean_count == 2U);
        }

        assert(call_count == response_count);
        assert(current_gpi == FIRME_GPI_NONSECURE);
    }

    test_reset();
    feature_gpc2 = VAL_TRUE;
    feature_gdi = VAL_TRUE;
    allow_protected_access = VAL_TRUE;
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_transition_feat_gdi_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.fail_step == GPT_TRANS_FEAT_GDI_MATRIX_STEP);

    test_reset();
    failed_write_index = 2U;
    val_firme_gpt_rule_result_init(&out);
    assert(!val_firme_gpt_scrub_ns(TEST_BASE, &out, 1ULL));
    assert(out.status == ACS_STATUS_FAIL && clean_count == 0U);
    assert(!val_firme_gpt_scrub_ns(TEST_BASE + 1ULL, &out, 2ULL));
    assert(out.fail_step == 1ULL);
    assert(!val_firme_gpt_scrub_ns(TEST_BASE, NULL, 3ULL));
}

/** @brief Reject terminal setup errors and exercise the GDI-aware NS start-granule check. */
static void test_start_granule_setup(void)
{
    firme_rule_res_t out;
    size_t call_index;

    test_reset();
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    assert(firme_gpt_start_granule_run_realm(TEST_BASE, &out) == VAL_ERROR);
    assert(out.fail_step == GPT_START_GRANULE_STEP_SET);
    assert(val_firme_status_to_i64(out.last_x0) == FIRME_DENIED);
    assert(call_count == 1U && response_count == 1U);

    test_reset();
    feature_gpc2 = VAL_TRUE;
    feature_gdi = VAL_TRUE;
    for (call_index = 0U; call_index < 4U; call_index++)
        test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_start_granule_run_ns(TEST_BASE, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_PASS && call_count == response_count);
    assert(calls[2].target == FIRME_GPI_NSP);
    assert(calls[3].base == TEST_BASE);

    test_reset();
    probe_fault = VAL_TRUE;
    for (call_index = 0U; call_index < 4U; call_index++)
        test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_start_granule_run_realm(TEST_BASE, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_PASS && call_count == response_count);
}

/** @brief Check failure returns from GPT_INFO assertions and retained-operation cleanup. */
static void test_additional_body_failures(void)
{
    int (*const info_entries[])(uint64_t, firme_rule_res_t *) = {
        firme_gpt_info_success_outputs_run_ns,
        firme_gpt_info_output_validity_run_ns,
        firme_gpt_info_status_run_ns,
        firme_gpt_par_state_encoding_run_ns,
        firme_gpt_protected_range_base_alignment_run_ns,
        firme_gpt_protected_range_size_run_ns,
    };
    int (*const retained_entries[])(uint64_t, firme_rule_res_t *) = {
        firme_gpt_op_continue_start_granule_run_ns,
        firme_gpt_op_continue_granule_count_run_ns,
        firme_gpt_stateful_incomplete_run_ns,
    };
    firme_rule_res_t out;
    size_t entry_index;

    for (entry_index = 0U;
         entry_index < sizeof(info_entries) / sizeof(info_entries[0]);
         entry_index++) {
        test_reset();
        feature_reg1 = (uint64_t)FIRME_GM_PPS_1TB << FIRME_GM_FEATURE_REG1_PPS_SHIFT;
        test_reply(FIRME_SUCCESS, TEST_BASE, PLAT_FIRME_GPT_INFO_PAR_STATE);
        assert(info_entries[entry_index](TEST_BASE, &out) == VAL_ERROR);
        assert(out.status == ACS_STATUS_FAIL && call_count == response_count);
    }

    for (entry_index = 0U;
         entry_index < sizeof(retained_entries) / sizeof(retained_entries[0]);
         entry_index++) {
        test_reset();
        feature_reg0 &= ~FIRME_GM_FEATURE_REG0_OP_CONTINUE_BIT;
        assert(retained_entries[entry_index](TEST_BASE, &out) == VAL_ERROR);
        assert(out.status == ACS_STATUS_FAIL && cleanup_count == 0U && call_count == 0U);
    }

    test_reset();
    feature_reg1 = (uint64_t)FIRME_GM_PGS_16KB << FIRME_GM_FEATURE_REG1_PGS_SHIFT;
    assert(firme_gpt_granule_size_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL);

    test_reset();
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_denied_status_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL);

    test_reset();
    test_reply(FIRME_SUCCESS, 0ULL, 0ULL);
    assert(firme_gpt_invalid_parameters_status_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL);

    test_reset();
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_not_supported_status_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_ERR && cleanup_count == 0U);

    test_reset();
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_start_granule_run_realm(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL && call_count == response_count);
    assert(out.fail_step == GPT_START_GRANULE_STEP_ADJACENT_ACCESS);

    test_reset();
    out.status = ACS_STATUS_FAIL;
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(gpt_start_granule_transition(TEST_BASE, FIRME_GPI_REALM,
        &out, 1ULL) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_FAIL);
    assert(firme_gpt_info_concurrent_state_run_ns(TEST_BASE, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_SKIP);
}

/** @brief A rejected probe scope is not evidence of successful memory access. */
static void test_probe_infrastructure_error(void)
{
    firme_rule_res_t out = {.status = ACS_STATUS_FAIL};

    test_reset();
    probe_infrastructure_error = VAL_TRUE;
    assert(gpt_gpi_set_progress_expect_access(TEST_BASE, VAL_TRUE, &out, 1ULL) == VAL_FALSE);
    assert(gpt_success_count_expect_access(TEST_BASE, VAL_TRUE, &out, 1ULL) == VAL_FALSE);
    assert(gpt_start_granule_expect_access(TEST_BASE, &out, 1ULL) == VAL_FALSE);
    assert(gpt_gpi_set_progress_expect_access(TEST_BASE, VAL_FALSE, &out, 1ULL) == VAL_FALSE);

    test_reset();
    test_reply(FIRME_INCOMPLETE, 0ULL, TEST_COOKIE);
    test_reply(FIRME_NOT_SUPPORTED, UINT64_MAX, UINT64_MAX);
    assert(firme_gpt_success_granule_count_run_realm(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_ERR && call_count == 2U);

    test_reset();
    test_reply(FIRME_INCOMPLETE, 0ULL, TEST_COOKIE);
    test_reply(FIRME_NOT_SUPPORTED, UINT64_MAX, UINT64_MAX);
    assert(firme_gpt_gpi_set_progress_run_realm(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_ERR && call_count == 2U);
}

/** @brief Reject no-op NSO transitions and unrelated faults without unsafe cleanup. */
static void test_gpc2_independent_observation(void)
{
    firme_rule_res_t out;
    size_t phase_index;
    size_t failed_phase;

    test_reset();
    feature_gpc2 = VAL_TRUE;
    ignore_nso_transition = VAL_TRUE;
    for (phase_index = 0U; phase_index < 2U; phase_index++)
        test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_transition_feat_gpc2_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL && call_count == response_count);
    assert(out.fail_step == GPT_TRANS_FEAT_GPC2_MATRIX_NSO_STEP_BASE + 3ULL);
    assert(current_gpi == FIRME_GPI_NONSECURE && clean_count == 2U);
    assert(scrub_writes >= TEST_PGS / sizeof(uint64_t));

    test_reset();
    feature_gpc2 = VAL_TRUE;
    test_reply(FIRME_INCOMPLETE, 0ULL, TEST_COOKIE);
    test_reply(FIRME_BUSY, UINT64_MAX, TEST_COOKIE + 1ULL);
    test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
    test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
    assert(firme_gpt_transition_feat_gpc2_run_ns(TEST_BASE, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_PASS && call_count == response_count);
    assert(calls[1].abi == TEST_CONTINUE && calls[1].base == TEST_COOKIE);
    assert(calls[2].abi == TEST_CONTINUE && calls[2].base == TEST_COOKIE + 1ULL);
    assert(current_gpi == FIRME_GPI_NONSECURE && clean_count == 2U);

    test_reset();
    feature_gpc2 = VAL_TRUE;
    ns_probe_fault_esr = (UINT64_C(0x25) << 26) | UINT64_C(0x04);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_transition_feat_gpc2_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL);
    assert(out.fail_step == GPT_TRANS_FEAT_GPC2_MATRIX_NSO_STEP_BASE + 3ULL);

    test_reset();
    feature_gpc2 = VAL_TRUE;
    ns_probe_transport_failure = VAL_TRUE;
    assert(firme_gpt_transition_feat_gpc2_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_ERR && call_count == 0U);
    assert(out.fail_step == GPT_TRANS_FEAT_GPC2_MATRIX_BASELINE_STEP_BASE + 3ULL);

    for (failed_phase = 0U; failed_phase < 2U; failed_phase++) {
        test_reset();
        feature_gpc2 = VAL_TRUE;
        for (phase_index = 0U; phase_index < failed_phase; phase_index++)
            test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
        test_reply(FIRME_INCOMPLETE, 0ULL, TEST_COOKIE);
        test_reply(FIRME_NOT_SUPPORTED, UINT64_MAX, UINT64_MAX);
        assert(firme_gpt_transition_feat_gpc2_run_ns(TEST_BASE, &out) == VAL_ERROR);
        assert(out.status == ACS_STATUS_ERR && call_count == response_count);
        assert(calls[failed_phase + 1U].abi == TEST_CONTINUE);
        assert(calls[failed_phase + 1U].base == TEST_COOKIE);
        assert(clean_count == (failed_phase == 0U ? 0U : 1U));
    }

    test_reset();
    feature_gpc2 = VAL_TRUE;
    test_reply(FIRME_INCOMPLETE, 0ULL, TEST_COOKIE);
    for (phase_index = 1U; phase_index < VAL_FIRME_GPT_TRACE_MAX_PHASES; phase_index++)
        test_reply(FIRME_BUSY, UINT64_MAX, TEST_COOKIE + phase_index);
    assert(firme_gpt_transition_feat_gpc2_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_ERR && call_count == response_count);
    assert(clean_count == 0U && scrub_writes == 0U);

    test_reset();
    feature_gpc2 = VAL_TRUE;
    test_reply(FIRME_SUCCESS, 0ULL, UINT64_MAX);
    assert(firme_gpt_transition_feat_gpc2_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_ERR && call_count == 1U && clean_count == 0U);

    for (failed_phase = 0U; failed_phase < 2U; failed_phase++) {
        test_reset();
        feature_gpc2 = VAL_TRUE;
        for (phase_index = 0U; phase_index < failed_phase; phase_index++)
            test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
        test_reply(FIRME_DENIED, 0ULL, UINT64_MAX);
        if (failed_phase == 1U)
            test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
        assert(firme_gpt_transition_feat_gpc2_run_ns(TEST_BASE, &out) == VAL_ERROR);
        assert(out.status == ACS_STATUS_FAIL);
        assert(call_count == response_count);
    }

    test_reset();
    feature_gpc2 = VAL_TRUE;
    failed_write_index = 2U;
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_transition_feat_gpc2_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL && call_count == 1U && clean_count == 0U);
}

/** @brief Separate a strict transition failure from independently restored scratch. */
static void test_observed_ns_cleanup(void)
{
    firme_rule_res_t out = {0};

    /* Step 1: Reject the DUT's transition error without issuing same-GPI cleanup. */
    test_reset();
    feature_gpc2 = VAL_TRUE;
    test_reply(FIRME_DENIED, 0ULL, UINT64_MAX);
    assert(firme_gpt_transition_feat_gpc2_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL && out.fail_step == 1ULL);
    assert(call_count == 1U && clean_count == 1U);

    /* Step 2: The GDI caller retains the same independent cleanup discipline. */
    test_reset();
    feature_gpc2 = feature_gdi = VAL_TRUE;
    test_reply(FIRME_DENIED, 0ULL, UINT64_MAX);
    assert(firme_gpt_transition_feat_gdi_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL && out.fail_step == GPT_TRANS_FEAT_GDI_TRANSITION_STEP);
    assert(call_count == 1U && clean_count == 1U);

    /* Step 3: A terminal rejection is not evidence that its granule remained NS. */
    test_reset();
    feature_gpc2 = VAL_TRUE;
    change_gpi_on_error = VAL_TRUE;
    test_reply(FIRME_DENIED, 0ULL, UINT64_MAX);
    assert(firme_gpt_transition_feat_gpc2_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_ERR && current_gpi == FIRME_GPI_NSO);
    assert(call_count == 1U && clean_count == 0U);

    /* Step 4: A failed continuation never permits competing GDI restoration. */
    test_reset();
    feature_gpc2 = feature_gdi = VAL_TRUE;
    test_reply(FIRME_INCOMPLETE, 0ULL, TEST_COOKIE);
    test_reply(FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
    assert(firme_gpt_transition_feat_gdi_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_ERR && call_count == 2U && clean_count == 0U);

    /* Step 5: Even accessible memory cannot clear an already quarantined operation. */
    test_reset();
    assert(!val_firme_gpt_confirm_ns_restored(TEST_BASE, &out, UINT64_C(1)));
    assert(out.status == ACS_STATUS_ERR && probe_count == 0U && scrub_writes == 0U);
}

/** @brief Keep R0094 failures strict and verify restoration only after remote completion. */
static void test_rme_host_observation(void)
{
    const firme_acs_world_t worlds[] = {FIRME_ACS_WORLD_SECURE, FIRME_ACS_WORLD_REALM};
    const uint64_t unsafe_gpis[] = {FIRME_GPI_NSO, FIRME_GPI_NSP, TEST_GPI_ANY,
        FIRME_GPI_SECURE, FIRME_GPI_REALM, FIRME_GPI_ROOT, FIRME_GPI_SA};
    firme_acs_call_res_t response = {0};
    size_t world_index;
    size_t gpi_index;

    for (world_index = 0U; world_index < sizeof(worlds) / sizeof(worlds[0]); world_index++) {
        test_reset();
        model_rme_state = feature_gpc2 = VAL_TRUE;
        test_reply(FIRME_DENIED, 0ULL, UINT64_MAX);
        assert(val_firme_gpt_run_rme_observed(worlds[world_index], TEST_BASE,
                                              &response) == VAL_SUCCESS);
        assert(response.x0 == ACS_STATUS_FAIL && response.x1 == 1ULL);
        assert(val_firme_status_to_i64(response.x2) == FIRME_DENIED && response.x3 == 0ULL);
        assert(call_count == 1U && remote_rule_calls == 1U);
        assert(remote_probe_calls == 6U && clean_count == 2U);

        test_reset();
        model_rme_state = feature_gpc2 = VAL_TRUE;
        test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
        test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
        test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
        test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
        test_reply(FIRME_DENIED, 0ULL, UINT64_MAX);
        test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
        assert(val_firme_gpt_run_rme_observed(worlds[world_index], TEST_BASE,
                                              &response) == VAL_SUCCESS);
        assert(response.x0 == ACS_STATUS_PASS && call_count == response_count);
        assert(current_gpi == FIRME_GPI_NONSECURE && remote_rule_calls == 1U);
        assert(remote_probe_calls == 6U && clean_count == 2U);

        test_reset();
        model_rme_state = feature_gpc2 = VAL_TRUE;
        test_reply(FIRME_INCOMPLETE, 0ULL, TEST_COOKIE);
        test_reply(FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
        assert(val_firme_gpt_run_rme_observed(worlds[world_index], TEST_BASE,
                                              &response) == VAL_SUCCESS);
        assert(response.x0 == ACS_STATUS_ERR && response.x1 == 1ULL);
        assert(call_count == 2U && calls[1].abi == TEST_CONTINUE);
        assert(remote_probe_calls == 3U && clean_count == 1U);

        for (gpi_index = 0U; gpi_index < sizeof(unsafe_gpis) / sizeof(unsafe_gpis[0]);
             gpi_index++) {
            test_reset();
            model_rme_state = feature_gpc2 = VAL_TRUE;
            remote_result_gpi = unsafe_gpis[gpi_index];
            test_reply(FIRME_DENIED, 0ULL, UINT64_MAX);
            assert(val_firme_gpt_run_rme_observed(worlds[world_index], TEST_BASE,
                                                  &response) == VAL_SUCCESS);
            assert(response.x0 == ACS_STATUS_ERR && response.x1 == 1ULL);
            assert(val_firme_status_to_i64(response.x2) == FIRME_DENIED);
            assert(call_count == 1U && remote_rule_calls == 1U && clean_count == 1U);

            test_reset();
            model_rme_state = feature_gpc2 = VAL_TRUE;
            current_gpi = unsafe_gpis[gpi_index];
            assert(val_firme_gpt_run_rme_observed(worlds[world_index], TEST_BASE,
                                                  &response) == VAL_SUCCESS);
            assert(response.x0 == ACS_STATUS_ERR);
            assert(call_count == 0U && remote_rule_calls == 0U && clean_count == 0U);
        }
    }
}

/** @brief Verify whole-range Host evidence and changed-prefix cleanup for adjacent rules. */
static void test_shared_range_observation(void)
{
    const firme_rule_id_t setup_rules[] = {FIRME_RULE_R0111, FIRME_RULE_R0113,
        FIRME_RULE_R0117, FIRME_RULE_R0120};
    const firme_rule_id_t range_rules[] = {FIRME_RULE_R0113, FIRME_RULE_R0117};
    const firme_acs_world_t worlds[] = {FIRME_ACS_WORLD_SECURE, FIRME_ACS_WORLD_REALM};
    firme_acs_call_res_t response = {0};
    uint64_t count;
    size_t world_index;
    size_t rule_index;

    for (world_index = 0U; world_index < sizeof(worlds) / sizeof(worlds[0]); world_index++) {
        for (rule_index = 0U; rule_index < sizeof(setup_rules) / sizeof(setup_rules[0]);
             rule_index++) {
            count = val_firme_gpt_observed_rule_granules(setup_rules[rule_index]);
            test_reset();
            model_rme_state = feature_gpc2 = VAL_TRUE;
            test_reply(FIRME_DENIED, 0ULL, UINT64_MAX);
            assert(val_firme_gpt_run_observed(setup_rules[rule_index], worlds[world_index],
                                              TEST_BASE, &response) == VAL_SUCCESS);
            assert(response.x0 == ACS_STATUS_FAIL && call_count == 1U);
            assert(val_firme_status_to_i64(response.x2) == FIRME_DENIED);
            assert(remote_rule_calls == 1U && clean_count == count * 2U);

            test_reset();
            model_rme_state = feature_gpc2 = VAL_TRUE;
            test_reply(FIRME_INCOMPLETE, 0ULL, TEST_COOKIE);
            test_reply(FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
            assert(val_firme_gpt_run_observed(setup_rules[rule_index], worlds[world_index],
                                              TEST_BASE, &response) == VAL_SUCCESS);
            assert(response.x0 == ACS_STATUS_ERR && call_count == 2U);
            assert(clean_count == count && remote_probe_calls == count * 3U);
        }

        for (rule_index = 0U; rule_index < sizeof(range_rules) / sizeof(range_rules[0]);
             rule_index++) {
            test_reset();
            model_rme_state = feature_gpc2 = VAL_TRUE;
            test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
            test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
            test_reply(FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
            assert(val_firme_gpt_run_observed(range_rules[rule_index], worlds[world_index],
                                              TEST_BASE, &response) == VAL_SUCCESS);
            assert(response.x0 == ACS_STATUS_FAIL && call_count == 3U);
            assert(val_firme_status_to_i64(response.x2) == FIRME_INVALID_PARAMETERS);
            assert(clean_count == 4U && remote_probe_calls == 12U);

            test_reset();
            model_rme_state = feature_gpc2 = VAL_TRUE;
            test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
            test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
            test_reply(FIRME_DENIED, 1ULL, UINT64_MAX);
            test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
            assert(val_firme_gpt_run_observed(range_rules[rule_index], worlds[world_index],
                                              TEST_BASE, &response) == VAL_SUCCESS);
            assert(response.x0 == ACS_STATUS_FAIL && call_count == 4U);
            assert(val_firme_status_to_i64(response.x2) == FIRME_DENIED && response.x3 == 1ULL);
            assert(calls[3].base == TEST_BASE && calls[3].target == FIRME_GPI_NONSECURE);
            assert(current_gpi == FIRME_GPI_NONSECURE && second_gpi == FIRME_GPI_NONSECURE);
            assert(clean_count == 4U);

            test_reset();
            model_rme_state = feature_gpc2 = VAL_TRUE;
            test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
            test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
            test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
            test_reply(FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
            test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
            assert(val_firme_gpt_run_observed(range_rules[rule_index], worlds[world_index],
                                              TEST_BASE, &response) == VAL_SUCCESS);
            assert(response.x0 == ACS_STATUS_FAIL && call_count == 5U);
            assert(calls[3].base == TEST_BASE + TEST_PGS && calls[4].base == TEST_BASE);
            assert(current_gpi == FIRME_GPI_NONSECURE && second_gpi == FIRME_GPI_NONSECURE);

            test_reset();
            model_rme_state = feature_gpc2 = VAL_TRUE;
            test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
            test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
            test_reply(FIRME_INCOMPLETE, 1ULL, TEST_COOKIE);
            test_reply(FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
            assert(val_firme_gpt_run_observed(range_rules[rule_index], worlds[world_index],
                                              TEST_BASE, &response) == VAL_SUCCESS);
            assert(response.x0 == ACS_STATUS_ERR && call_count == 4U);
            assert(calls[3].abi == TEST_CONTINUE && calls[3].base == TEST_COOKIE);
            assert(clean_count == 2U && remote_probe_calls == 6U);

            test_reset();
            model_rme_state = feature_gpc2 = VAL_TRUE;
            remote_result_second_gpi = TEST_GPI_ANY;
            test_reply(FIRME_DENIED, 0ULL, UINT64_MAX);
            assert(val_firme_gpt_run_observed(range_rules[rule_index], worlds[world_index],
                                              TEST_BASE, &response) == VAL_SUCCESS);
            assert(response.x0 == ACS_STATUS_ERR && call_count == 1U);
            assert(val_firme_status_to_i64(response.x2) == FIRME_DENIED);
        }

        test_reset();
        model_rme_state = feature_gpc2 = VAL_TRUE;
        test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
        test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
        test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
        test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
        assert(val_firme_gpt_run_observed(FIRME_RULE_R0111, worlds[world_index],
                                          TEST_BASE, &response) == VAL_SUCCESS);
        assert(response.x0 == ACS_STATUS_PASS && call_count == 4U && clean_count == 4U);
        assert(calls[3].base == TEST_BASE && calls[3].target == FIRME_GPI_NONSECURE);
    }

    test_reset();
    model_rme_state = feature_gpc2 = VAL_TRUE;
    test_reply(FIRME_DENIED, 0ULL, UINT64_MAX);
    assert(val_firme_gpt_run_observed(FIRME_RULE_R0127, FIRME_ACS_WORLD_NSEL2,
                                      TEST_BASE, &response) == VAL_SUCCESS);
    assert(response.x0 == ACS_STATUS_FAIL && call_count == 1U);

    test_reset();
    model_rme_state = feature_gpc2 = VAL_TRUE;
    test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
    test_reply(FIRME_DENIED, 0ULL, UINT64_MAX);
    test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
    assert(val_firme_gpt_run_observed(FIRME_RULE_R0092, FIRME_ACS_WORLD_REALM,
                                      TEST_BASE, &response) == VAL_SUCCESS);
    assert(response.x0 == ACS_STATUS_FAIL && response.x1 == GPT_SAME_GPI_STEP_RESULT);
    assert(call_count == 3U && current_gpi == FIRME_GPI_NONSECURE);

    test_reset();
    model_rme_state = feature_gpc2 = VAL_TRUE;
    test_reply(FIRME_SUCCESS, 1ULL, UINT64_MAX);
    test_reply(FIRME_DENIED, 0ULL, UINT64_MAX);
    test_reply(FIRME_DENIED, 0ULL, UINT64_MAX);
    assert(val_firme_gpt_run_observed(FIRME_RULE_R0092, FIRME_ACS_WORLD_REALM,
                                      TEST_BASE, &response) == VAL_SUCCESS);
    assert(response.x0 == ACS_STATUS_ERR && response.x1 == GPT_SAME_GPI_STEP_RESULT);
    assert(call_count == 3U && current_gpi == FIRME_GPI_REALM);
}

/** @brief Route retained rules to their owned coordinator before generic scratch probing. */
static void test_retained_dispatch_route(void)
{
    const firme_rule_id_t rules[] = {
        FIRME_RULE_R0114, FIRME_RULE_R0115, FIRME_RULE_R0129, FIRME_RULE_R0130,
    };
    firme_acs_call_res_t response;
    firme_acs_world_t world;
    size_t rule_index;

    for (world = FIRME_ACS_WORLD_NSEL2; world <= FIRME_ACS_WORLD_REALM; world++) {
        for (rule_index = 0U; rule_index < sizeof(rules) / sizeof(rules[0]); rule_index++) {
            test_reset();
            feature_reg0 &= ~FIRME_GM_FEATURE_REG0_OP_CONTINUE_BIT;
            assert(val_firme_gpt_observed_rule_granules(rules[rule_index]) != 0ULL);
            assert(val_firme_gpt_run_observed(rules[rule_index], world,
                                             TEST_BASE, &response) == VAL_SUCCESS);
            assert(response.x0 == ACS_STATUS_FAIL);
            assert(response.x1 == GPT_RETAINED_STEP_PREPARE);
            assert(!call_count && !probe_count && !cleanup_count && !remote_rule_calls);
        }
    }
}

/** @brief Run focused GPT helper and rule regressions without firmware. */
int main(void)
{
    test_trace();
    test_success_count_continuation();
    test_info();
    test_continue();
    test_l1();
    test_cleanup();
    test_denied_progress();
    test_features();
    test_denied_preconditions();
    test_rule_return_contract();
    test_absent_feature_status();
    test_present_feature_transitions();
    test_start_granule_setup();
    test_additional_body_failures();
    test_probe_infrastructure_error();
    test_gpc2_independent_observation();
    test_observed_ns_cleanup();
    test_rme_host_observation();
    test_shared_range_observation();
    test_retained_dispatch_route();
    puts("GPT native regressions: PASS");
    return VAL_SUCCESS;
}
