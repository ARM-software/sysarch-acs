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
static uint64_t probe_accessible_count;
static uint64_t feature_reg1;
static uint64_t feature_reg0;
static int64_t feature_status;
static pal_firme_gpt_scenario_status_t fixture_status;
static pal_firme_gpt_scenario_t fixture;

/** @brief Reset the scripted DUT and fixture. */
static void test_reset(void)
{
    memset(responses, 0, sizeof(responses));
    memset(calls, 0, sizeof(calls));
    response_count = 0U;
    call_count = 0U;
    cleanup_count = 0U;
    probe_count = 0U;
    probe_fault = VAL_FALSE;
    probe_accessible_count = 0ULL;
    feature_reg1 = 0ULL;
    feature_reg0 = FIRME_GM_FEATURE_REG0_ALLOCATED_BITS | (UINT64_C(1) << 63);
    feature_status = FIRME_SUCCESS;
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

/** @brief Select the baseline-RME path for GPC2 regressions. */
uint32_t val_firme_feat_rme_gpc2_present(void)
{
    return VAL_FALSE;
}

/** @brief Select the baseline-RME path for GDI regressions. */
uint32_t val_firme_feat_rme_gdi_present(void)
{
    return VAL_FALSE;
}

/** @brief Model the other worlds' GPFs against an NS granule. */
int val_firme_world_call(firme_acs_world_t world, firme_acs_cmd_t command,
                         uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                         firme_acs_call_res_t *result)
{
    *result = (firme_acs_call_res_t){
        .x0 = ACS_STATUS_PASS,
        .x1 = 1ULL,
        .x2 = (UINT64_C(0x25) << 26) | UINT64_C(0x28),
    };
    return VAL_SUCCESS;
}

/** @brief Return one scripted GPI_SET response. */
void val_firme_gm_gpi_set_do(uint64_t base, uint64_t count, uint64_t target,
                             firme_res_t *result)
{
    test_invoke(TEST_SET, base, count, target, result);
    if ((target == FIRME_GPI_REALM) &&
        (val_firme_status_to_i64(result->x0) == FIRME_SUCCESS))
        probe_accessible_count += result->x1;
}

/** @brief Return one scripted continuation response. */
void val_firme_gm_gpi_op_continue(uint64_t cookie, firme_res_t *result)
{
    test_invoke(TEST_CONTINUE, cookie, 0ULL, 0ULL, result);
}

/** @brief Return a scripted composite result and update local probe state. */
int val_firme_gm_gpi_set_complete(uint64_t base, uint64_t count,
                                  uint64_t target, firme_res_t *result)
{
    test_invoke(TEST_COMPLETE, base, count, target, result);
    if ((target == FIRME_GPI_NONSECURE) &&
        (val_firme_status_to_i64(result->x0) == FIRME_SUCCESS))
        probe_accessible_count = 0ULL;
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
    if ((probe_fault == VAL_TRUE) &&
        (address >= TEST_BASE + probe_accessible_count * TEST_PGS)) {
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
    return VAL_SUCCESS;
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
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_base_alignment_run_realm(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL && call_count == 2U);
    assert(calls[1].target == FIRME_GPI_NONSECURE);

    test_reset();
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_transition_feat_rme_run_realm(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL && out.fail_step == 4ULL);
    assert(call_count == 5U && calls[4].target == FIRME_GPI_NONSECURE);

    test_reset();
    test_reply(FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    assert(firme_gpt_transition_feat_gpc2_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL);
    assert(out.fail_step == GPT_TRANS_FEAT_GPC2_CLEANUP_STEP_BASE);

    test_reset();
    test_reply(FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
    test_reply(FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    assert(firme_gpt_transition_feat_gdi_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL);
    assert(out.fail_step == GPT_TRANS_FEAT_GDI_CLEANUP_STEP_BASE);

    test_reset();
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    assert(firme_gpt_transition_feat_rme_run_secure(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL && out.fail_step == 1ULL);
    assert(val_firme_status_to_i64(out.last_x0) == FIRME_DENIED);
    assert(call_count == response_count);
}

/** @brief Accept MAX_CONCURRENT_OP_CNT and ignore unused feature outputs. */
static void test_features(void)
{
    firme_rule_res_t out;

    test_reset();
    feature_reg1 = (UINT64_C(1) << 63) | (UINT64_C(2) << 9);
    assert(gm_feature_registers_run(&out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_PASS);
    feature_reg1 |= UINT64_C(1) << FIRME_GM_FEATURE_REG1_L0GPTSZ_SHIFT;
    assert(gm_feature_registers_run(&out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL);
    assert(out.fail_step == GM_FEATURE_REGISTERS_STEP_REG1_RANGE);
    feature_reg1 = UINT64_C(7) << FIRME_GM_FEATURE_REG1_PPS_SHIFT;
    assert(gm_feature_registers_run(&out) == VAL_ERROR);
    assert(out.fail_step == GM_FEATURE_REGISTERS_STEP_REG1_PPS);
    assert(gm_abi_compliance_run(FIRME_ACS_WORLD_REALM, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_PASS);
    feature_reg0 &= ~FIRME_GM_FEATURE_REG0_GPT_INFO_BIT;
    assert(gm_abi_compliance_run(FIRME_ACS_WORLD_REALM, &out) == VAL_ERROR);
    assert(out.fail_step == GM_ABI_COMPLIANCE_STEP_MANDATORY_BITS);
}

/** @brief Reject fabricated progress for a wholly forbidden Root transition. */
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
    assert(out.status == ACS_STATUS_FAIL);
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
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_transition_feat_rme_run_ns(TEST_BASE, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_PASS && call_count == response_count);

    test_reset();
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    test_reply(FIRME_DENIED, 0ULL, 0ULL);
    test_reply(FIRME_NOT_SUPPORTED, UINT64_MAX, UINT64_MAX);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_transition_feat_rme_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL && out.fail_step == 4ULL);
    assert(call_count == response_count);

    test_reset();
    test_reply(FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_transition_feat_gpc2_run_ns(TEST_BASE, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_PASS && call_count == response_count);
    test_reset();
    test_reply(FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
    test_reply(FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
    test_reply(FIRME_SUCCESS, 1ULL, 0ULL);
    assert(firme_gpt_transition_feat_gdi_run_ns(TEST_BASE, &out) == VAL_SUCCESS);
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
        test_reply(FIRME_SUCCESS, 0ULL, UINT64_MAX);
        assert(retained_entries[entry_index](TEST_BASE, &out) == VAL_ERROR);
        assert(out.status == ACS_STATUS_FAIL && cleanup_count == 1U);
    }

    test_reset();
    feature_reg1 = (uint64_t)FIRME_GM_PGS_16KB << FIRME_GM_FEATURE_REG1_PGS_SHIFT;
    assert(firme_gpt_granule_size_run_ns(TEST_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL);

    test_reset();
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
    assert(out.status == ACS_STATUS_FAIL && cleanup_count == 1U);

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
        VAL_TRUE, &out, 1ULL) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_FAIL);
    assert(firme_gpt_info_concurrent_state_run_ns(TEST_BASE, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_SKIP);
}

/** @brief Run focused GPT helper and rule regressions without firmware. */
int main(void)
{
    test_trace();
    test_info();
    test_continue();
    test_l1();
    test_cleanup();
    test_denied_progress();
    test_features();
    test_rule_return_contract();
    test_absent_feature_status();
    test_additional_body_failures();
    puts("GPT native regressions: PASS");
    return VAL_SUCCESS;
}
