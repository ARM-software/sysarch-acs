/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt_l1.h"

#include <stddef.h>

#include "pal_firme_gpt.h"
#include "val_common/val_status.h"
#include "val_firme_abi.h"
#include "val_firme_gpt.h"
#include "val_firme_gpt_fixture.h"
#include "val_log.h"

#define VAL_FIRME_GPT_L1_MAX_CALLS UINT32_C(32)

#define VAL_FIRME_GPT_L1_STEP_DISCOVERY UINT64_C(1)
#define VAL_FIRME_GPT_L1_STEP_PREPARE   UINT64_C(2)
#define VAL_FIRME_GPT_L1_STEP_PRESTATE  UINT64_C(3)
#define VAL_FIRME_GPT_L1_STEP_INVOKE    UINT64_C(4)
#define VAL_FIRME_GPT_L1_STEP_OUTPUT    UINT64_C(5)
#define VAL_FIRME_GPT_L1_STEP_POSTSTATE UINT64_C(6)
#define VAL_FIRME_GPT_L1_STEP_MONITOR   UINT64_C(7)
#define VAL_FIRME_GPT_L1_STEP_CLEANUP   UINT64_C(8)

typedef enum {
    VAL_FIRME_GPT_L1_FLOW_PASS = 0,
    VAL_FIRME_GPT_L1_FLOW_SKIP,
    VAL_FIRME_GPT_L1_FLOW_FAIL,
    VAL_FIRME_GPT_L1_FLOW_ERROR,
} val_firme_gpt_l1_flow_t;

typedef struct {
    firme_res_t result[VAL_FIRME_GPT_L1_MAX_CALLS];
    uint64_t object_base_input[VAL_FIRME_GPT_L1_MAX_CALLS];
    uint32_t call_count;
    uint32_t saw_incomplete;
} val_firme_gpt_l1_trace_t;

typedef struct {
    pal_firme_gpt_l1_condition_t condition;
    int64_t expected_status;
} val_firme_gpt_l1_status_case_t;

static val_firme_gpt_fixture_t g_l1_fixture;
static uint32_t g_l1_fixture_lock;

/** @brief Prepare a positive lifecycle using an explicitly reserved aperture. */
static pal_firme_gpt_scenario_status_t val_firme_gpt_l1_owned_prepare(
    firme_acs_world_t world, pal_firme_gpt_l1_operation_t operation,
    pal_firme_gpt_l1_condition_t condition, pal_firme_gpt_l1_scenario_t *scenario)
{
    pal_firme_gpt_scenario_status_t status;
    pal_firme_gpt_memory_kind_t kind;
    uint32_t unlocked = VAL_FALSE;

    if (condition != PAL_FIRME_GPT_L1_CONDITION_VALID_SELF &&
        condition != PAL_FIRME_GPT_L1_CONDITION_VALID_NON_SELF)
        return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
    if (!__atomic_compare_exchange_n(&g_l1_fixture_lock, &unlocked, VAL_TRUE,
            0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
    kind = condition == PAL_FIRME_GPT_L1_CONDITION_VALID_SELF ?
           PAL_FIRME_GPT_MEMORY_L1_SELF : PAL_FIRME_GPT_MEMORY_L1_NON_SELF;
    status = val_firme_gpt_fixture_begin(&g_l1_fixture, world, kind);
    if (status == PAL_FIRME_GPT_SCENARIO_READY && operation == PAL_FIRME_GPT_L1_DESTROY &&
        val_firme_gpt_fixture_l1_complete(&g_l1_fixture, VAL_FALSE) != VAL_SUCCESS)
        status = PAL_FIRME_GPT_SCENARIO_ERROR;
    if (status != PAL_FIRME_GPT_SCENARIO_READY) {
        if (val_firme_gpt_fixture_cleanup(&g_l1_fixture) == VAL_SUCCESS)
            __atomic_store_n(&g_l1_fixture_lock, VAL_FALSE, __ATOMIC_RELEASE);
        else
            status = PAL_FIRME_GPT_SCENARIO_ERROR;
        return status;
    }
    *scenario = (pal_firme_gpt_l1_scenario_t){
        .range_base = g_l1_fixture.memory.range_base,
        .range_size = g_l1_fixture.range_size,
        .object_base = g_l1_fixture.memory.object_base,
        .object_size = g_l1_fixture.object_size,
        .granule_size = g_l1_fixture.granule_size,
        .range_initial_gpi = g_l1_fixture.memory.initial_gpi,
        .object_initial_gpi = FIRME_GPI_NONSECURE,
        .token = g_l1_fixture.memory.token,
        .layout = kind == PAL_FIRME_GPT_MEMORY_L1_SELF ?
            PAL_FIRME_GPT_L1_LAYOUT_SELF_DESCRIBING :
            PAL_FIRME_GPT_L1_LAYOUT_NON_SELF_DESCRIBING,
        .fixture_owned = VAL_TRUE,
    };
    return PAL_FIRME_GPT_SCENARIO_READY;
}

/** @brief Release only the exact owned lifecycle after verified restoration. */
static pal_firme_gpt_scenario_status_t val_firme_gpt_l1_release(
    firme_acs_world_t world, pal_firme_gpt_l1_operation_t operation,
    pal_firme_gpt_l1_condition_t condition,
    const pal_firme_gpt_l1_scenario_t *scenario)
{
    if (!scenario->fixture_owned)
        return pal_firme_gpt_l1_scenario_cleanup(world, operation, condition, scenario);
    if (!scenario->token || scenario->token != g_l1_fixture.memory.token ||
        world != g_l1_fixture.world ||
        val_firme_gpt_fixture_cleanup(&g_l1_fixture) != VAL_SUCCESS)
        return PAL_FIRME_GPT_SCENARIO_ERROR;
    __atomic_store_n(&g_l1_fixture_lock, VAL_FALSE, __ATOMIC_RELEASE);
    return PAL_FIRME_GPT_SCENARIO_READY;
}

static const val_firme_gpt_l1_status_case_t g_create_status_cases[] = {
    {PAL_FIRME_GPT_L1_CONDITION_VALID_SELF, FIRME_SUCCESS},
    {PAL_FIRME_GPT_L1_CONDITION_VALID_NON_SELF, FIRME_SUCCESS},
    {PAL_FIRME_GPT_L1_CONDITION_PA_OUTSIDE_PPS,
     FIRME_INVALID_PARAMETERS},
    {PAL_FIRME_GPT_L1_CONDITION_PA_MISALIGNED,
     FIRME_INVALID_PARAMETERS},
    {PAL_FIRME_GPT_L1_CONDITION_EL3_MANAGED,
     FIRME_INVALID_PARAMETERS},
    {PAL_FIRME_GPT_L1_CONDITION_OBJECT_OUTSIDE_PPS,
     FIRME_INVALID_PARAMETERS},
    {PAL_FIRME_GPT_L1_CONDITION_OBJECT_MISALIGNED,
     FIRME_INVALID_PARAMETERS},
    {PAL_FIRME_GPT_L1_CONDITION_OBJECT_UNSUITABLE,
     FIRME_INVALID_PARAMETERS},
    {PAL_FIRME_GPT_L1_CONDITION_CONTINUE_WITHOUT_PENDING,
     FIRME_INVALID_PARAMETERS},
    {PAL_FIRME_GPT_L1_CONDITION_AUXILIARY_L1_UNAVAILABLE,
     FIRME_NOT_FOUND},
    {PAL_FIRME_GPT_L1_CONDITION_ALREADY_PRESENT, FIRME_ALREADY_EXISTS},
    {PAL_FIRME_GPT_L1_CONDITION_CONFLICT, FIRME_OP_CONFLICT},
    {PAL_FIRME_GPT_L1_CONDITION_BUSY, FIRME_BUSY},
    {PAL_FIRME_GPT_L1_CONDITION_NO_MEMORY, FIRME_NO_MEMORY},
    {PAL_FIRME_GPT_L1_CONDITION_OBJECT_GPI_DENIED, FIRME_DENIED},
    {PAL_FIRME_GPT_L1_CONDITION_INCOMPLETE_NON_SELF, FIRME_INCOMPLETE},
};

static const val_firme_gpt_l1_status_case_t g_destroy_status_cases[] = {
    {PAL_FIRME_GPT_L1_CONDITION_VALID_SELF, FIRME_SUCCESS},
    {PAL_FIRME_GPT_L1_CONDITION_VALID_NON_SELF, FIRME_SUCCESS},
    {PAL_FIRME_GPT_L1_CONDITION_PA_OUTSIDE_PPS,
     FIRME_INVALID_PARAMETERS},
    {PAL_FIRME_GPT_L1_CONDITION_PA_MISALIGNED,
     FIRME_INVALID_PARAMETERS},
    {PAL_FIRME_GPT_L1_CONDITION_EL3_MANAGED,
     FIRME_INVALID_PARAMETERS},
    {PAL_FIRME_GPT_L1_CONDITION_NO_L1_OBJECT, FIRME_NOT_FOUND},
    {PAL_FIRME_GPT_L1_CONDITION_CONFLICT, FIRME_OP_CONFLICT},
    {PAL_FIRME_GPT_L1_CONDITION_MIXED_RANGE_GPI, FIRME_DENIED},
    {PAL_FIRME_GPT_L1_CONDITION_SELF_OBJECT_NOT_ROOT, FIRME_DENIED},
    {PAL_FIRME_GPT_L1_CONDITION_SELF_NON_OBJECT_NOT_NS, FIRME_DENIED},
    {PAL_FIRME_GPT_L1_CONDITION_NO_MEMORY, FIRME_NO_MEMORY},
    {PAL_FIRME_GPT_L1_CONDITION_BUSY, FIRME_BUSY},
    {PAL_FIRME_GPT_L1_CONDITION_INCOMPLETE_NON_SELF, FIRME_INCOMPLETE},
};

/**
 * @brief Record a diagnostic assertion failure for an L1 GPT rule.
 * @param rule_id Active rule identifier.
 * @param out Rule result.
 * @param step Failing algorithm step.
 * @param observed Primary observed value.
 * @param expected Expected or secondary diagnostic value.
 * @return None.
 */
static void val_firme_gpt_l1_fail(firme_rule_id_t rule_id,
                                  firme_rule_res_t *out,
                                  uint64_t step,
                                  uint64_t observed,
                                  uint64_t expected)
{
    LOG_ERROR(NULL,
              "l1_gpt: rule=R%04u step=%llu observed=%llx expected=%llx",
              (unsigned int)rule_id, (unsigned long long)step,
              (unsigned long long)observed,
              (unsigned long long)expected);
    if (step == VAL_FIRME_GPT_L1_STEP_CLEANUP && out &&
        out->status == (uint64_t)ACS_STATUS_FAIL && out->fail_step != 0ULL)
        return;
    val_firme_gpt_rule_result_fail(out, step, observed, expected);
}

/**
 * @brief Convert an internal flow result into an ACS execution result.
 * @param flow Internal rule flow result.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
static int val_firme_gpt_l1_complete(val_firme_gpt_l1_flow_t flow,
                                     firme_rule_res_t *out)
{
    if (!out)
        return VAL_ERROR;

    if (flow == VAL_FIRME_GPT_L1_FLOW_PASS)
        out->status = (uint64_t)ACS_STATUS_PASS;

    if ((flow == VAL_FIRME_GPT_L1_FLOW_ERROR) ||
        (flow == VAL_FIRME_GPT_L1_FLOW_FAIL)) {
        /* Step 1: Keep uncertain cleanup distinct from a completed assertion failure. */
        out->status = (flow == VAL_FIRME_GPT_L1_FLOW_ERROR) ?
                      (uint64_t)ACS_STATUS_ERR : (uint64_t)ACS_STATUS_FAIL;
        return VAL_ERROR;
    }

    return val_firme_gpt_rule_result_status(out);
}

/**
 * @brief Return whether two non-empty physical ranges overlap.
 * @param first_base First range base.
 * @param first_size First range size.
 * @param second_base Second range base.
 * @param second_size Second range size.
 * @return VAL_TRUE when ranges overlap or their geometry overflows.
 */
static uint32_t val_firme_gpt_l1_ranges_overlap(uint64_t first_base,
                                                uint64_t first_size,
                                                uint64_t second_base,
                                                uint64_t second_size)
{
    uint64_t first_top;
    uint64_t second_top;

    if ((first_size == 0ULL) || (second_size == 0ULL) ||
        (first_base > (UINT64_MAX - first_size)) ||
        (second_base > (UINT64_MAX - second_size)))
        return VAL_TRUE;

    first_top = first_base + first_size;
    second_top = second_base + second_size;

    return ((first_base < second_top) && (second_base < first_top)) ?
           VAL_TRUE : VAL_FALSE;
}

/**
 * @brief Derive the architected L1 GPT object size from feature register 1.
 * @param feature_reg1 Granule Management feature register 1.
 * @return Required object size and alignment, or zero for invalid geometry.
 */
static uint64_t val_firme_gpt_l1_object_size(uint64_t feature_reg1)
{
    uint64_t granule_size;
    uint64_t range_size;
    uint64_t granule_count;

    granule_size = val_firme_gpt_pgs_size_from_feature_reg1(feature_reg1);
    range_size = val_firme_gpt_range_size_from_feature_reg1(feature_reg1);
    if ((granule_size == 0ULL) || (range_size == 0ULL) ||
        ((range_size % granule_size) != 0ULL))
        return 0ULL;

    granule_count = range_size / granule_size;
    if ((granule_count == 0ULL) || ((granule_count % 2ULL) != 0ULL))
        return 0ULL;

    return granule_count / 2ULL;
}

/**
 * @brief Discover whether the optional L1 create/destroy ABI pair is present.
 * @param rule_id Active rule identifier.
 * @param out Rule result.
 * @return PASS when both exist, SKIP when neither exists, or FAIL on mismatch.
 */
static val_firme_gpt_l1_flow_t val_firme_gpt_l1_discover(
    firme_rule_id_t rule_id, firme_rule_res_t *out)
{
    firme_res_t result = {0};
    uint64_t pair_bits;

    val_firme_service_features(FIRME_SERVICE_ID_GRANULE_MGMT,
                               FIRME_GM_FEATURE_REG0_INDEX, &result);
    if (val_firme_status_to_i64(result.x0) != (int64_t)FIRME_SUCCESS) {
        val_firme_gpt_l1_fail(rule_id, out,
                              VAL_FIRME_GPT_L1_STEP_DISCOVERY,
                              result.x0, (uint64_t)FIRME_SUCCESS);
        return VAL_FIRME_GPT_L1_FLOW_FAIL;
    }

    pair_bits = result.x1 & (FIRME_GM_FEATURE_REG0_L1_CREATE_BIT |
                             FIRME_GM_FEATURE_REG0_L1_DESTROY_BIT);
    if (pair_bits == 0ULL) {
        out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_FIRME_GPT_L1_FLOW_SKIP;
    }

    if (pair_bits != (FIRME_GM_FEATURE_REG0_L1_CREATE_BIT |
                      FIRME_GM_FEATURE_REG0_L1_DESTROY_BIT)) {
        val_firme_gpt_l1_fail(
            rule_id, out, VAL_FIRME_GPT_L1_STEP_DISCOVERY, pair_bits,
            FIRME_GM_FEATURE_REG0_L1_CREATE_BIT |
                FIRME_GM_FEATURE_REG0_L1_DESTROY_BIT);
        return VAL_FIRME_GPT_L1_FLOW_FAIL;
    }

    return VAL_FIRME_GPT_L1_FLOW_PASS;
}

/**
 * @brief Validate PAL scenario geometry against advertised GPT geometry.
 * @param rule_id Active rule identifier.
 * @param condition Condition represented by the scenario.
 * @param scenario PAL scenario to validate.
 * @param out Rule result.
 * @return PASS for valid geometry, otherwise ERROR.
 */
static val_firme_gpt_l1_flow_t val_firme_gpt_l1_validate_scenario(
    firme_rule_id_t rule_id, pal_firme_gpt_l1_condition_t condition,
    const pal_firme_gpt_l1_scenario_t *scenario, firme_rule_res_t *out)
{
    uint64_t feature_reg1 = 0ULL;
    uint64_t expected_range_size;
    uint64_t expected_granule_size;
    uint64_t expected_object_size;
    uint64_t object_granule_base;
    uint64_t object_granule_size;
    uint32_t overlap;

    if (!scenario ||
        (val_firme_gpt_feature_reg1(
             &feature_reg1, out,
             VAL_FIRME_GPT_L1_STEP_PREPARE) == VAL_FALSE))
        return VAL_FIRME_GPT_L1_FLOW_ERROR;

    expected_range_size =
        val_firme_gpt_range_size_from_feature_reg1(feature_reg1);
    expected_granule_size =
        val_firme_gpt_pgs_size_from_feature_reg1(feature_reg1);
    expected_object_size = val_firme_gpt_l1_object_size(feature_reg1);

    if ((scenario->range_size != expected_range_size) ||
        (scenario->granule_size != expected_granule_size) ||
        (scenario->object_size != expected_object_size) ||
        (expected_object_size == 0ULL) ||
        ((scenario->layout != PAL_FIRME_GPT_L1_LAYOUT_SELF_DESCRIBING) &&
         (scenario->layout != PAL_FIRME_GPT_L1_LAYOUT_NON_SELF_DESCRIBING)) ||
        (scenario->range_base > (UINT64_MAX - scenario->range_size)) ||
        (scenario->object_base > (UINT64_MAX - scenario->object_size)) ||
        (scenario->range_initial_gpi > UINT64_C(0xf)) ||
        (scenario->object_initial_gpi > UINT64_C(0xf))) {
        val_firme_gpt_l1_fail(rule_id, out,
                              VAL_FIRME_GPT_L1_STEP_PREPARE,
                              scenario->object_size,
                              expected_object_size);
        return VAL_FIRME_GPT_L1_FLOW_ERROR;
    }

    if ((condition != PAL_FIRME_GPT_L1_CONDITION_PA_MISALIGNED) &&
        ((scenario->range_base % expected_range_size) != 0ULL)) {
        val_firme_gpt_l1_fail(rule_id, out,
                              VAL_FIRME_GPT_L1_STEP_PREPARE,
                              scenario->range_base, expected_range_size);
        return VAL_FIRME_GPT_L1_FLOW_ERROR;
    }

    if (condition ==
        PAL_FIRME_GPT_L1_CONDITION_CONTINUE_WITHOUT_PENDING) {
        if (scenario->object_base != 0ULL) {
            val_firme_gpt_l1_fail(rule_id, out,
                                  VAL_FIRME_GPT_L1_STEP_PREPARE,
                                  scenario->object_base, 0ULL);
            return VAL_FIRME_GPT_L1_FLOW_ERROR;
        }
        return VAL_FIRME_GPT_L1_FLOW_PASS;
    }

    if ((condition != PAL_FIRME_GPT_L1_CONDITION_OBJECT_MISALIGNED) &&
        ((scenario->object_base % expected_object_size) != 0ULL)) {
        val_firme_gpt_l1_fail(rule_id, out,
                              VAL_FIRME_GPT_L1_STEP_PREPARE,
                              scenario->object_base, expected_object_size);
        return VAL_FIRME_GPT_L1_FLOW_ERROR;
    }

    if (pal_firme_gpt_granule_envelope(
            scenario->object_base, scenario->object_size,
            scenario->granule_size, &object_granule_base,
            &object_granule_size) != PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_l1_fail(rule_id, out, VAL_FIRME_GPT_L1_STEP_PREPARE,
                              scenario->object_base, scenario->object_size);
        return VAL_FIRME_GPT_L1_FLOW_ERROR;
    }
    overlap = val_firme_gpt_l1_ranges_overlap(
        scenario->range_base, scenario->range_size,
        object_granule_base, object_granule_size);
    if (((scenario->layout ==
          PAL_FIRME_GPT_L1_LAYOUT_SELF_DESCRIBING) &&
         ((object_granule_base < scenario->range_base) ||
          ((object_granule_base + object_granule_size) >
           (scenario->range_base + scenario->range_size)))) ||
        ((scenario->layout ==
          PAL_FIRME_GPT_L1_LAYOUT_NON_SELF_DESCRIBING) &&
         (overlap == VAL_TRUE))) {
        val_firme_gpt_l1_fail(rule_id, out,
                              VAL_FIRME_GPT_L1_STEP_PREPARE,
                              overlap, scenario->layout);
        return VAL_FIRME_GPT_L1_FLOW_ERROR;
    }

    return VAL_FIRME_GPT_L1_FLOW_PASS;
}

/**
 * @brief Prepare one PAL-owned L1 GPT scenario.
 * @param rule_id Active rule identifier.
 * @param world Caller world.
 * @param operation ABI under test.
 * @param condition Condition to establish.
 * @param base_pa Default test address.
 * @param scenario Returned scenario.
 * @param out Rule result.
 * @return Internal setup result.
 */
static val_firme_gpt_l1_flow_t val_firme_gpt_l1_prepare(
    firme_rule_id_t rule_id, firme_acs_world_t world,
    pal_firme_gpt_l1_operation_t operation,
    pal_firme_gpt_l1_condition_t condition, uint64_t base_pa,
    pal_firme_gpt_l1_scenario_t *scenario, firme_rule_res_t *out)
{
    pal_firme_gpt_scenario_status_t status;
    val_firme_gpt_l1_flow_t flow;

    flow = val_firme_gpt_l1_discover(rule_id, out);
    if (flow != VAL_FIRME_GPT_L1_FLOW_PASS)
        return flow;

    status = pal_firme_gpt_l1_scenario_prepare(
        (uint32_t)world, operation, condition, base_pa, scenario);
    if (status == PAL_FIRME_GPT_SCENARIO_UNAVAILABLE)
        status = val_firme_gpt_l1_owned_prepare(world, operation, condition, scenario);
    if (status == PAL_FIRME_GPT_SCENARIO_UNAVAILABLE) {
        out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_FIRME_GPT_L1_FLOW_SKIP;
    }

    if (status != PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_l1_fail(rule_id, out,
                              VAL_FIRME_GPT_L1_STEP_PREPARE,
                              (uint64_t)status,
                              PAL_FIRME_GPT_SCENARIO_READY);
        return VAL_FIRME_GPT_L1_FLOW_ERROR;
    }

    flow = val_firme_gpt_l1_validate_scenario(
        rule_id, condition, scenario, out);
    if (flow == VAL_FIRME_GPT_L1_FLOW_PASS)
        return flow;

    status = val_firme_gpt_l1_release(
        (uint32_t)world, operation, condition, scenario);
    if (status != PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_l1_fail(rule_id, out,
                              VAL_FIRME_GPT_L1_STEP_CLEANUP,
                              (uint64_t)status,
                              PAL_FIRME_GPT_SCENARIO_READY);
    }

    return VAL_FIRME_GPT_L1_FLOW_ERROR;
}

/**
 * @brief Restore one PAL-owned L1 GPT scenario.
 * @param rule_id Active rule identifier.
 * @param world Caller world.
 * @param operation ABI under test.
 * @param condition Prepared condition.
 * @param scenario Prepared scenario.
 * @param out Rule result.
 * @return PASS on cleanup or ERROR on infrastructure failure.
 */
static val_firme_gpt_l1_flow_t val_firme_gpt_l1_cleanup(
    firme_rule_id_t rule_id, firme_acs_world_t world,
    pal_firme_gpt_l1_operation_t operation,
    pal_firme_gpt_l1_condition_t condition,
    const pal_firme_gpt_l1_scenario_t *scenario, firme_rule_res_t *out)
{
    pal_firme_gpt_scenario_status_t status;

    status = val_firme_gpt_l1_release(
        (uint32_t)world, operation, condition, scenario);
    if (status != PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_l1_fail(rule_id, out,
                              VAL_FIRME_GPT_L1_STEP_CLEANUP,
                              (uint64_t)status,
                              PAL_FIRME_GPT_SCENARIO_READY);
        return VAL_FIRME_GPT_L1_FLOW_ERROR;
    }

    return VAL_FIRME_GPT_L1_FLOW_PASS;
}

/**
 * @brief Return whether a status is valid for one L1 GPT ABI.
 * @param operation ABI operation.
 * @param status Signed FIRME status.
 * @return VAL_TRUE for an architected status, otherwise VAL_FALSE.
 */
static uint32_t val_firme_gpt_l1_status_valid(
    pal_firme_gpt_l1_operation_t operation, int64_t status)
{
    if ((status == (int64_t)FIRME_SUCCESS) ||
        (status == (int64_t)FIRME_NOT_SUPPORTED) ||
        (status == (int64_t)FIRME_INVALID_PARAMETERS) ||
        (status == (int64_t)FIRME_DENIED) ||
        (status == (int64_t)FIRME_BUSY) ||
        (status == (int64_t)FIRME_INCOMPLETE) ||
        (status == (int64_t)FIRME_OP_CONFLICT) ||
        (status == (int64_t)FIRME_NO_MEMORY) ||
        (status == (int64_t)FIRME_NOT_FOUND))
        return VAL_TRUE;

    if ((operation == PAL_FIRME_GPT_L1_CREATE) &&
        (status == (int64_t)FIRME_ALREADY_EXISTS))
        return VAL_TRUE;

    return VAL_FALSE;
}

/**
 * @brief Invoke one L1 GPT ABI once.
 * @param operation ABI operation.
 * @param scenario Prepared scenario.
 * @param continuation Whether CREATE must use a zero object base.
 * @param result Returned registers.
 * @param record_status Returned ownership/progress recording status.
 * @return Signed FIRME status.
 */
static int64_t val_firme_gpt_l1_invoke(
    pal_firme_gpt_l1_operation_t operation,
    const pal_firme_gpt_l1_scenario_t *scenario, uint32_t continuation,
    firme_res_t *result, int *record_status)
{
    uint64_t object_base;

    *record_status = VAL_SUCCESS;
    if (operation == PAL_FIRME_GPT_L1_CREATE) {
        object_base = (continuation == VAL_TRUE) ? 0ULL :
                      scenario->object_base;
        val_firme_gm_l1_gpt_create(
            scenario->range_base, object_base, result);
    } else {
        val_firme_gm_l1_gpt_destroy(scenario->range_base, result);
    }

    if (scenario->fixture_owned) {
        if (!scenario->token || scenario->token != g_l1_fixture.memory.token)
            *record_status = VAL_ERROR;
        else
            *record_status = val_firme_gpt_fixture_l1_record(&g_l1_fixture,
                operation == PAL_FIRME_GPT_L1_DESTROY ? VAL_TRUE : VAL_FALSE, result);
    }

    return val_firme_status_to_i64(result->x0);
}

/**
 * @brief Invoke one L1 GPT ABI and require an exact status.
 * @param rule_id Active rule identifier.
 * @param operation ABI operation.
 * @param scenario Prepared scenario.
 * @param expected_status Required signed status.
 * @param result Returned registers.
 * @param out Rule result.
 * @return PASS for the expected status, otherwise FAIL.
 */
static val_firme_gpt_l1_flow_t val_firme_gpt_l1_invoke_expected(
    firme_rule_id_t rule_id, pal_firme_gpt_l1_operation_t operation,
    const pal_firme_gpt_l1_scenario_t *scenario, int64_t expected_status,
    firme_res_t *result, firme_rule_res_t *out)
{
    uint32_t continuation;
    int64_t status;
    int record_status;

    continuation =
        ((operation == PAL_FIRME_GPT_L1_CREATE) &&
         (scenario->object_base == 0ULL)) ? VAL_TRUE : VAL_FALSE;
    status = val_firme_gpt_l1_invoke(
        operation, scenario, continuation, result, &record_status);

    if ((record_status != VAL_SUCCESS) ||
        (val_firme_gpt_l1_status_valid(operation, status) == VAL_FALSE) ||
        (status != expected_status)) {
        val_firme_gpt_l1_fail(rule_id, out,
                              VAL_FIRME_GPT_L1_STEP_INVOKE,
                              result->x0, (uint64_t)expected_status);
        return VAL_FIRME_GPT_L1_FLOW_FAIL;
    }

    return VAL_FIRME_GPT_L1_FLOW_PASS;
}

/**
 * @brief Run one L1 GPT operation through BUSY/INCOMPLETE to a terminal status.
 * @param rule_id Active rule identifier.
 * @param operation ABI operation.
 * @param scenario Prepared scenario.
 * @param expected_status Required terminal status.
 * @param trace Returned raw call trace.
 * @param out Rule result.
 * @return PASS for the expected terminal result, otherwise FAIL.
 */
static val_firme_gpt_l1_flow_t val_firme_gpt_l1_run_to_terminal(
    firme_rule_id_t rule_id, pal_firme_gpt_l1_operation_t operation,
    const pal_firme_gpt_l1_scenario_t *scenario, int64_t expected_status,
    val_firme_gpt_l1_trace_t *trace, firme_rule_res_t *out)
{
    firme_res_t *result;
    int64_t status = (int64_t)FIRME_BUSY;
    uint32_t continuation = VAL_FALSE;
    int record_status;

    if (!trace)
        return VAL_FIRME_GPT_L1_FLOW_ERROR;

    *trace = (val_firme_gpt_l1_trace_t){0};
    while (trace->call_count < VAL_FIRME_GPT_L1_MAX_CALLS) {
        result = &trace->result[trace->call_count];
        trace->object_base_input[trace->call_count] =
            ((operation == PAL_FIRME_GPT_L1_CREATE) &&
             (continuation == VAL_FALSE)) ? scenario->object_base : 0ULL;
        status = val_firme_gpt_l1_invoke(
            operation, scenario, continuation, result, &record_status);
        trace->call_count++;

        if (record_status != VAL_SUCCESS ||
            val_firme_gpt_l1_status_valid(operation, status) == VAL_FALSE) {
            val_firme_gpt_l1_fail(rule_id, out,
                                  VAL_FIRME_GPT_L1_STEP_INVOKE,
                                  result->x0, 0ULL);
            return VAL_FIRME_GPT_L1_FLOW_FAIL;
        }

        if (status == (int64_t)FIRME_BUSY)
            continue;

        if (status == (int64_t)FIRME_INCOMPLETE) {
            trace->saw_incomplete = VAL_TRUE;
            continuation = VAL_TRUE;
            continue;
        }

        if (status != expected_status) {
            val_firme_gpt_l1_fail(rule_id, out,
                                  VAL_FIRME_GPT_L1_STEP_INVOKE,
                                  result->x0,
                                  (uint64_t)expected_status);
            return VAL_FIRME_GPT_L1_FLOW_FAIL;
        }

        return VAL_FIRME_GPT_L1_FLOW_PASS;
    }

    val_firme_gpt_l1_fail(rule_id, out,
                          VAL_FIRME_GPT_L1_STEP_INVOKE,
                          trace->call_count,
                          VAL_FIRME_GPT_L1_MAX_CALLS);
    return VAL_FIRME_GPT_L1_FLOW_FAIL;
}

/**
 * @brief Check every granule in one range using independent PAL observation.
 * @param rule_id Active rule identifier.
 * @param scenario Prepared scenario.
 * @param range_base Range to check.
 * @param range_size Range size.
 * @param exclude_base Optional excluded subrange base.
 * @param exclude_size Optional excluded subrange size.
 * @param allowed_gpi_mask Accepted GPI encoding bits.
 * @param step Diagnostic step.
 * @param out Rule result.
 * @return PASS, SKIP, FAIL, or ERROR.
 */
static val_firme_gpt_l1_flow_t val_firme_gpt_l1_check_gpis(
    firme_rule_id_t rule_id, const pal_firme_gpt_l1_scenario_t *scenario,
    uint64_t range_base, uint64_t range_size, uint64_t exclude_base,
    uint64_t exclude_size, uint32_t allowed_gpi_mask, uint64_t step,
    firme_rule_res_t *out)
{
    pal_firme_gpt_scenario_status_t status;
    uint64_t range_top;
    uint64_t exclude_top;
    uint64_t expected_count;
    uint64_t checked_count = 0ULL;

    if (scenario && range_base == scenario->object_base &&
        range_size == scenario->object_size &&
        pal_firme_gpt_granule_envelope(range_base, range_size,
            scenario->granule_size, &range_base, &range_size) !=
            PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_l1_fail(rule_id, out, step, range_base, range_size);
        return VAL_FIRME_GPT_L1_FLOW_ERROR;
    }
    if (scenario && exclude_size && exclude_base == scenario->object_base &&
        exclude_size == scenario->object_size &&
        pal_firme_gpt_granule_envelope(exclude_base, exclude_size,
            scenario->granule_size, &exclude_base, &exclude_size) !=
            PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_l1_fail(rule_id, out, step, exclude_base, exclude_size);
        return VAL_FIRME_GPT_L1_FLOW_ERROR;
    }
    if (!scenario || (scenario->granule_size == 0ULL) ||
        (range_size == 0ULL) ||
        ((range_base % scenario->granule_size) != 0ULL) ||
        ((range_size % scenario->granule_size) != 0ULL) ||
        ((exclude_size % scenario->granule_size) != 0ULL) ||
        (exclude_size > range_size) || (allowed_gpi_mask == 0U) ||
        (range_base > (UINT64_MAX - range_size))) {
        val_firme_gpt_l1_fail(rule_id, out, step, range_size,
                              scenario ? scenario->granule_size : 0ULL);
        return VAL_FIRME_GPT_L1_FLOW_ERROR;
    }

    range_top = range_base + range_size;
    if (exclude_size != 0ULL) {
        if ((exclude_base > (UINT64_MAX - exclude_size)) ||
            ((exclude_base % scenario->granule_size) != 0ULL)) {
            val_firme_gpt_l1_fail(rule_id, out, step,
                                  exclude_base, exclude_size);
            return VAL_FIRME_GPT_L1_FLOW_ERROR;
        }
        exclude_top = exclude_base + exclude_size;
        if ((exclude_base < range_base) || (exclude_top > range_top)) {
            val_firme_gpt_l1_fail(rule_id, out, step,
                                  exclude_base, range_base);
            return VAL_FIRME_GPT_L1_FLOW_ERROR;
        }
    }

    expected_count = (range_size - exclude_size) / scenario->granule_size;
    status = pal_firme_gpt_l1_gpi_range_check(
        range_base, range_size, scenario->granule_size,
        exclude_base, exclude_size, allowed_gpi_mask, &checked_count);
    if (status == PAL_FIRME_GPT_SCENARIO_UNAVAILABLE) {
        out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_FIRME_GPT_L1_FLOW_SKIP;
    }

    if (status == PAL_FIRME_GPT_SCENARIO_MISMATCH) {
        val_firme_gpt_l1_fail(rule_id, out, step,
                              checked_count, expected_count);
        return VAL_FIRME_GPT_L1_FLOW_FAIL;
    }

    if ((status != PAL_FIRME_GPT_SCENARIO_READY) ||
        (checked_count != expected_count)) {
        val_firme_gpt_l1_fail(rule_id, out, step,
                              (uint64_t)status, checked_count);
        return VAL_FIRME_GPT_L1_FLOW_ERROR;
    }

    return VAL_FIRME_GPT_L1_FLOW_PASS;
}

/**
 * @brief Return a one-bit mask for an architectural GPI encoding.
 * @param gpi Architectural GPI encoding.
 * @return GPI bit mask, or zero for an invalid encoding.
 */
static uint32_t val_firme_gpt_l1_gpi_mask(uint64_t gpi)
{
    if (gpi > UINT64_C(0xf))
        return 0U;

    return UINT32_C(1) << (uint32_t)gpi;
}

/**
 * @brief Check the range portion that is not occupied by a self-describing object.
 * @param rule_id Active rule identifier.
 * @param scenario Prepared scenario.
 * @param step Diagnostic step.
 * @param out Rule result.
 * @return Internal observation result.
 */
static val_firme_gpt_l1_flow_t val_firme_gpt_l1_check_range_unchanged(
    firme_rule_id_t rule_id, const pal_firme_gpt_l1_scenario_t *scenario,
    uint64_t step, firme_rule_res_t *out)
{
    uint64_t exclude_base = 0ULL;
    uint64_t exclude_size = 0ULL;
    uint32_t allowed_mask;

    if (scenario->layout == PAL_FIRME_GPT_L1_LAYOUT_SELF_DESCRIBING) {
        exclude_base = scenario->object_base;
        exclude_size = scenario->object_size;
    }

    allowed_mask = val_firme_gpt_l1_gpi_mask(
        scenario->range_initial_gpi);
    return val_firme_gpt_l1_check_gpis(
        rule_id, scenario, scenario->range_base, scenario->range_size,
        exclude_base, exclude_size, allowed_mask, step, out);
}

/**
 * @brief Run one complete setup/invoke/cleanup status case.
 * @param rule_id Active rule identifier.
 * @param world Caller world.
 * @param operation ABI operation.
 * @param status_case Condition and exact expected status.
 * @param base_pa Default test address.
 * @param out Rule result.
 * @return Internal case result.
 */
static val_firme_gpt_l1_flow_t val_firme_gpt_l1_run_status_case(
    firme_rule_id_t rule_id, firme_acs_world_t world,
    pal_firme_gpt_l1_operation_t operation,
    const val_firme_gpt_l1_status_case_t *status_case,
    uint64_t base_pa, firme_rule_res_t *out)
{
    pal_firme_gpt_l1_scenario_t scenario = {0};
    firme_res_t result = {0};
    val_firme_gpt_l1_flow_t flow;
    val_firme_gpt_l1_flow_t cleanup_flow;

    flow = val_firme_gpt_l1_prepare(
        rule_id, world, operation, status_case->condition,
        base_pa, &scenario, out);
    if (flow != VAL_FIRME_GPT_L1_FLOW_PASS)
        return flow;

    flow = val_firme_gpt_l1_invoke_expected(
        rule_id, operation, &scenario, status_case->expected_status,
        &result, out);
    cleanup_flow = val_firme_gpt_l1_cleanup(
        rule_id, world, operation, status_case->condition,
        &scenario, out);
    if (cleanup_flow == VAL_FIRME_GPT_L1_FLOW_ERROR)
        return cleanup_flow;

    return flow;
}

/**
 * @brief Validate an L1 ABI is inaccessible from a non-Realm FIRME instance.
 * @param rule_id Active rule identifier.
 * @param operation ABI operation.
 * @param base_pa Default test address.
 * @param out Rule result.
 * @return PASS only for exact NOT_SUPPORTED.
 */
static val_firme_gpt_l1_flow_t val_firme_gpt_l1_disallowed_world(
    firme_rule_id_t rule_id, pal_firme_gpt_l1_operation_t operation,
    uint64_t base_pa, firme_rule_res_t *out)
{
    firme_res_t result = {0};
    int64_t status;

    if (operation == PAL_FIRME_GPT_L1_CREATE)
        val_firme_gm_l1_gpt_create(base_pa, base_pa, &result);
    else
        val_firme_gm_l1_gpt_destroy(base_pa, &result);

    status = val_firme_status_to_i64(result.x0);
    if (status != (int64_t)FIRME_NOT_SUPPORTED) {
        val_firme_gpt_l1_fail(rule_id, out,
                              VAL_FIRME_GPT_L1_STEP_INVOKE,
                              result.x0,
                              (uint64_t)FIRME_NOT_SUPPORTED);
        return VAL_FIRME_GPT_L1_FLOW_FAIL;
    }

    return VAL_FIRME_GPT_L1_FLOW_PASS;
}

/**
 * @brief Validate the complete status table for one L1 GPT ABI.
 * @param rule_id Active rule identifier.
 * @param world Caller world.
 * @param operation ABI operation.
 * @param base_pa Default test address.
 * @param out Rule result.
 * @return Internal rule result.
 */
static val_firme_gpt_l1_flow_t val_firme_gpt_l1_status_rule(
    firme_rule_id_t rule_id, firme_acs_world_t world,
    pal_firme_gpt_l1_operation_t operation, uint64_t base_pa,
    firme_rule_res_t *out)
{
    const val_firme_gpt_l1_status_case_t *cases;
    size_t case_count;
    size_t case_index;
    val_firme_gpt_l1_flow_t flow;

    if (world != FIRME_ACS_WORLD_REALM)
        return val_firme_gpt_l1_disallowed_world(
            rule_id, operation, base_pa, out);

    if (operation == PAL_FIRME_GPT_L1_CREATE) {
        cases = g_create_status_cases;
        case_count = sizeof(g_create_status_cases) /
                     sizeof(g_create_status_cases[0]);
    } else {
        cases = g_destroy_status_cases;
        case_count = sizeof(g_destroy_status_cases) /
                     sizeof(g_destroy_status_cases[0]);
    }

    for (case_index = 0U; case_index < case_count; case_index++) {
        flow = val_firme_gpt_l1_run_status_case(
            rule_id, world, operation, &cases[case_index], base_pa, out);
        if (flow != VAL_FIRME_GPT_L1_FLOW_PASS)
            return flow;
    }

    return VAL_FIRME_GPT_L1_FLOW_PASS;
}

/**
 * @brief Validate bridge-object ownership, Root GPI, geometry, and lifetime.
 * @param world Caller world.
 * @param base_pa Default test address.
 * @param out Rule result.
 * @return Internal rule result.
 */
static val_firme_gpt_l1_flow_t val_firme_gpt_l1_bridge_rule(
    firme_acs_world_t world, uint64_t base_pa, firme_rule_res_t *out)
{
    pal_firme_gpt_l1_scenario_t scenario = {0};
    pal_firme_gpt_l1_evidence_t evidence = {0};
    val_firme_gpt_l1_trace_t trace = {0};
    pal_firme_gpt_scenario_status_t monitor_status;
    val_firme_gpt_l1_flow_t flow;
    val_firme_gpt_l1_flow_t cleanup_flow;
    uint32_t bridge_overlap;
    uint32_t object_overlap;

    if (world != FIRME_ACS_WORLD_REALM)
        return val_firme_gpt_l1_disallowed_world(
            FIRME_RULE_R0156, PAL_FIRME_GPT_L1_CREATE, base_pa, out);

    flow = val_firme_gpt_l1_prepare(
        FIRME_RULE_R0156, world, PAL_FIRME_GPT_L1_CREATE,
        PAL_FIRME_GPT_L1_CONDITION_VALID_SELF, base_pa,
        &scenario, out);
    if (flow != VAL_FIRME_GPT_L1_FLOW_PASS)
        return flow;

    monitor_status = pal_firme_gpt_l1_monitor_begin(
        (uint32_t)world, PAL_FIRME_GPT_L1_CREATE,
        PAL_FIRME_GPT_L1_CONDITION_VALID_SELF, &scenario);
    if (monitor_status == PAL_FIRME_GPT_SCENARIO_UNAVAILABLE) {
        out->status = (uint64_t)ACS_STATUS_SKIP;
        flow = VAL_FIRME_GPT_L1_FLOW_SKIP;
        goto cleanup;
    }
    if (monitor_status != PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_l1_fail(FIRME_RULE_R0156, out,
                              VAL_FIRME_GPT_L1_STEP_MONITOR,
                              monitor_status,
                              PAL_FIRME_GPT_SCENARIO_READY);
        flow = VAL_FIRME_GPT_L1_FLOW_ERROR;
        goto cleanup;
    }

    flow = val_firme_gpt_l1_run_to_terminal(
        FIRME_RULE_R0156, PAL_FIRME_GPT_L1_CREATE,
        &scenario, (int64_t)FIRME_SUCCESS, &trace, out);
    monitor_status = pal_firme_gpt_l1_monitor_end(
        (uint32_t)world, PAL_FIRME_GPT_L1_CREATE,
        PAL_FIRME_GPT_L1_CONDITION_VALID_SELF, &scenario, &evidence);
    if ((flow == VAL_FIRME_GPT_L1_FLOW_PASS) &&
        (monitor_status != PAL_FIRME_GPT_SCENARIO_READY)) {
        val_firme_gpt_l1_fail(FIRME_RULE_R0156, out,
                              VAL_FIRME_GPT_L1_STEP_MONITOR,
                              monitor_status,
                              PAL_FIRME_GPT_SCENARIO_READY);
        flow = VAL_FIRME_GPT_L1_FLOW_ERROR;
    }

    bridge_overlap = val_firme_gpt_l1_ranges_overlap(
        evidence.bridge_base, evidence.bridge_size,
        scenario.range_base, scenario.range_size);
    object_overlap = val_firme_gpt_l1_ranges_overlap(
        evidence.bridge_base, evidence.bridge_size,
        scenario.object_base, scenario.object_size);
    if ((flow == VAL_FIRME_GPT_L1_FLOW_PASS) &&
        ((evidence.bridge_observed != VAL_TRUE) ||
         (evidence.bridge_owned_by_el3 != VAL_TRUE) ||
         (evidence.bridge_reclaimed != VAL_TRUE) ||
         (evidence.bridge_gpi != FIRME_GPI_ROOT) ||
         (evidence.bridge_size != scenario.object_size) ||
         ((evidence.bridge_base % scenario.object_size) != 0ULL) ||
         (bridge_overlap != VAL_FALSE) ||
         (object_overlap != VAL_FALSE))) {
        val_firme_gpt_l1_fail(FIRME_RULE_R0156, out,
                              VAL_FIRME_GPT_L1_STEP_POSTSTATE,
                              evidence.bridge_base,
                              scenario.object_size);
        flow = VAL_FIRME_GPT_L1_FLOW_FAIL;
    }

cleanup:
    cleanup_flow = val_firme_gpt_l1_cleanup(
        FIRME_RULE_R0156, world, PAL_FIRME_GPT_L1_CREATE,
        PAL_FIRME_GPT_L1_CONDITION_VALID_SELF, &scenario, out);
    return (cleanup_flow == VAL_FIRME_GPT_L1_FLOW_ERROR) ?
           cleanup_flow : flow;
}

/**
 * @brief Validate the permitted L1 object GPI transition for one ABI.
 * @param rule_id Active rule identifier.
 * @param world Caller world.
 * @param operation ABI operation.
 * @param base_pa Default test address.
 * @param out Rule result.
 * @return Internal rule result.
 */
static val_firme_gpt_l1_flow_t val_firme_gpt_l1_transition_policy_rule(
    firme_rule_id_t rule_id, firme_acs_world_t world,
    pal_firme_gpt_l1_operation_t operation, uint64_t base_pa,
    firme_rule_res_t *out)
{
    pal_firme_gpt_l1_scenario_t scenario = {0};
    val_firme_gpt_l1_trace_t trace = {0};
    val_firme_gpt_l1_flow_t flow;
    val_firme_gpt_l1_flow_t cleanup_flow;
    uint32_t before_mask;
    uint32_t after_mask;

    if (world != FIRME_ACS_WORLD_REALM)
        return val_firme_gpt_l1_disallowed_world(
            rule_id, operation, base_pa, out);

    flow = val_firme_gpt_l1_prepare(
        rule_id, world, operation,
        PAL_FIRME_GPT_L1_CONDITION_VALID_NON_SELF,
        base_pa, &scenario, out);
    if (flow != VAL_FIRME_GPT_L1_FLOW_PASS)
        return flow;

    before_mask = val_firme_gpt_l1_gpi_mask(
        (operation == PAL_FIRME_GPT_L1_CREATE) ?
        FIRME_GPI_NONSECURE : FIRME_GPI_ROOT);
    after_mask = val_firme_gpt_l1_gpi_mask(
        (operation == PAL_FIRME_GPT_L1_CREATE) ?
        FIRME_GPI_ROOT : FIRME_GPI_NONSECURE);

    flow = val_firme_gpt_l1_check_gpis(
        rule_id, &scenario, scenario.object_base, scenario.object_size,
        0ULL, 0ULL, before_mask, VAL_FIRME_GPT_L1_STEP_PRESTATE, out);
    if (flow == VAL_FIRME_GPT_L1_FLOW_PASS)
        flow = val_firme_gpt_l1_run_to_terminal(
            rule_id, operation, &scenario, (int64_t)FIRME_SUCCESS,
            &trace, out);
    if (flow == VAL_FIRME_GPT_L1_FLOW_PASS)
        flow = val_firme_gpt_l1_check_gpis(
            rule_id, &scenario, scenario.object_base, scenario.object_size,
            0ULL, 0ULL, after_mask,
            VAL_FIRME_GPT_L1_STEP_POSTSTATE, out);

    cleanup_flow = val_firme_gpt_l1_cleanup(
        rule_id, world, operation,
        PAL_FIRME_GPT_L1_CONDITION_VALID_NON_SELF, &scenario, out);
    return (cleanup_flow == VAL_FIRME_GPT_L1_FLOW_ERROR) ?
           cleanup_flow : flow;
}

/**
 * @brief Validate range GPI stability across SUCCESS and INCOMPLETE.
 * @param rule_id Active rule identifier.
 * @param world Caller world.
 * @param operation ABI operation.
 * @param base_pa Default test address.
 * @param out Rule result.
 * @return Internal rule result.
 */
static val_firme_gpt_l1_flow_t val_firme_gpt_l1_range_stability_rule(
    firme_rule_id_t rule_id, firme_acs_world_t world,
    pal_firme_gpt_l1_operation_t operation, uint64_t base_pa,
    firme_rule_res_t *out)
{
    static const pal_firme_gpt_l1_condition_t conditions[] = {
        PAL_FIRME_GPT_L1_CONDITION_VALID_SELF,
        PAL_FIRME_GPT_L1_CONDITION_VALID_NON_SELF,
        PAL_FIRME_GPT_L1_CONDITION_INCOMPLETE_SELF,
        PAL_FIRME_GPT_L1_CONDITION_INCOMPLETE_NON_SELF,
    };
    pal_firme_gpt_l1_scenario_t scenario = {0};
    firme_res_t result = {0};
    val_firme_gpt_l1_flow_t flow;
    val_firme_gpt_l1_flow_t cleanup_flow;
    int64_t expected_status;
    size_t condition_index;

    if (world != FIRME_ACS_WORLD_REALM)
        return val_firme_gpt_l1_disallowed_world(
            rule_id, operation, base_pa, out);

    for (condition_index = 0U;
         condition_index < (sizeof(conditions) / sizeof(conditions[0]));
         condition_index++) {
        flow = val_firme_gpt_l1_prepare(
            rule_id, world, operation, conditions[condition_index],
            base_pa, &scenario, out);
        if (flow != VAL_FIRME_GPT_L1_FLOW_PASS)
            return flow;

        flow = val_firme_gpt_l1_check_range_unchanged(
            rule_id, &scenario, VAL_FIRME_GPT_L1_STEP_PRESTATE, out);
        expected_status =
            ((conditions[condition_index] ==
              PAL_FIRME_GPT_L1_CONDITION_INCOMPLETE_SELF) ||
             (conditions[condition_index] ==
              PAL_FIRME_GPT_L1_CONDITION_INCOMPLETE_NON_SELF)) ?
            (int64_t)FIRME_INCOMPLETE : (int64_t)FIRME_SUCCESS;
        if (flow == VAL_FIRME_GPT_L1_FLOW_PASS)
            flow = val_firme_gpt_l1_invoke_expected(
                rule_id, operation, &scenario, expected_status,
                &result, out);
        if (flow == VAL_FIRME_GPT_L1_FLOW_PASS)
            flow = val_firme_gpt_l1_check_range_unchanged(
                rule_id, &scenario,
                VAL_FIRME_GPT_L1_STEP_POSTSTATE, out);

        cleanup_flow = val_firme_gpt_l1_cleanup(
            rule_id, world, operation, conditions[condition_index],
            &scenario, out);
        if (cleanup_flow == VAL_FIRME_GPT_L1_FLOW_ERROR)
            return cleanup_flow;
        if (flow != VAL_FIRME_GPT_L1_FLOW_PASS)
            return flow;
    }

    return VAL_FIRME_GPT_L1_FLOW_PASS;
}

/**
 * @brief Validate object-granule GPIs for terminal or interrupted operation.
 * @param rule_id Active rule identifier.
 * @param world Caller world.
 * @param operation ABI operation.
 * @param incomplete Whether the invocation must return INCOMPLETE.
 * @param base_pa Default test address.
 * @param out Rule result.
 * @return Internal rule result.
 */
static val_firme_gpt_l1_flow_t val_firme_gpt_l1_object_gpi_rule(
    firme_rule_id_t rule_id, firme_acs_world_t world,
    pal_firme_gpt_l1_operation_t operation, uint32_t incomplete,
    uint64_t base_pa, firme_rule_res_t *out)
{
    pal_firme_gpt_l1_condition_t conditions[2];
    pal_firme_gpt_l1_scenario_t scenario = {0};
    firme_res_t result = {0};
    val_firme_gpt_l1_trace_t trace = {0};
    val_firme_gpt_l1_flow_t flow;
    val_firme_gpt_l1_flow_t cleanup_flow;
    uint64_t initial_gpi;
    uint32_t before_mask;
    uint32_t after_mask;
    size_t condition_index;

    if (world != FIRME_ACS_WORLD_REALM)
        return val_firme_gpt_l1_disallowed_world(
            rule_id, operation, base_pa, out);

    conditions[0] = (incomplete == VAL_TRUE) ?
                    PAL_FIRME_GPT_L1_CONDITION_INCOMPLETE_SELF :
                    PAL_FIRME_GPT_L1_CONDITION_VALID_SELF;
    conditions[1] = (incomplete == VAL_TRUE) ?
                    PAL_FIRME_GPT_L1_CONDITION_INCOMPLETE_NON_SELF :
                    PAL_FIRME_GPT_L1_CONDITION_VALID_NON_SELF;

    initial_gpi = (operation == PAL_FIRME_GPT_L1_CREATE) ?
                  FIRME_GPI_NONSECURE : FIRME_GPI_ROOT;
    before_mask = val_firme_gpt_l1_gpi_mask(initial_gpi);
    if (incomplete == VAL_TRUE) {
        after_mask = val_firme_gpt_l1_gpi_mask(FIRME_GPI_ROOT) |
                     val_firme_gpt_l1_gpi_mask(FIRME_GPI_NONSECURE);
    } else {
        after_mask = val_firme_gpt_l1_gpi_mask(
            (operation == PAL_FIRME_GPT_L1_CREATE) ?
            FIRME_GPI_ROOT : FIRME_GPI_NONSECURE);
    }

    for (condition_index = 0U; condition_index < 2U; condition_index++) {
        flow = val_firme_gpt_l1_prepare(
            rule_id, world, operation, conditions[condition_index],
            base_pa, &scenario, out);
        if (flow != VAL_FIRME_GPT_L1_FLOW_PASS)
            return flow;

        flow = val_firme_gpt_l1_check_gpis(
            rule_id, &scenario, scenario.object_base, scenario.object_size,
            0ULL, 0ULL, before_mask,
            VAL_FIRME_GPT_L1_STEP_PRESTATE, out);
        if ((flow == VAL_FIRME_GPT_L1_FLOW_PASS) &&
            (incomplete == VAL_TRUE)) {
            flow = val_firme_gpt_l1_invoke_expected(
                rule_id, operation, &scenario,
                (int64_t)FIRME_INCOMPLETE, &result, out);
        } else if (flow == VAL_FIRME_GPT_L1_FLOW_PASS) {
            flow = val_firme_gpt_l1_run_to_terminal(
                rule_id, operation, &scenario,
                (int64_t)FIRME_SUCCESS, &trace, out);
        }
        if (flow == VAL_FIRME_GPT_L1_FLOW_PASS)
            flow = val_firme_gpt_l1_check_gpis(
                rule_id, &scenario, scenario.object_base,
                scenario.object_size, 0ULL, 0ULL, after_mask,
                VAL_FIRME_GPT_L1_STEP_POSTSTATE, out);

        cleanup_flow = val_firme_gpt_l1_cleanup(
            rule_id, world, operation, conditions[condition_index],
            &scenario, out);
        if (cleanup_flow == VAL_FIRME_GPT_L1_FLOW_ERROR)
            return cleanup_flow;
        if (flow != VAL_FIRME_GPT_L1_FLOW_PASS)
            return flow;
    }

    return VAL_FIRME_GPT_L1_FLOW_PASS;
}

/**
 * @brief Check one exact segment of a denied CREATE object.
 * @param scenario Prepared denied scenario.
 * @param rule_id Active rule identifier.
 * @param start_index First object granule index.
 * @param granule_count Number of granules in the segment.
 * @param gpi Expected GPI for every segment granule.
 * @param step Diagnostic step.
 * @param out Rule result.
 * @return Internal observation result.
 */
static val_firme_gpt_l1_flow_t val_firme_gpt_l1_check_object_segment(
    const pal_firme_gpt_l1_scenario_t *scenario, firme_rule_id_t rule_id,
    uint64_t start_index, uint64_t granule_count, uint64_t gpi,
    uint64_t step, firme_rule_res_t *out)
{
    uint64_t segment_base;
    uint64_t object_base;
    uint64_t object_size;

    if (granule_count == 0ULL)
        return VAL_FIRME_GPT_L1_FLOW_PASS;

    if (pal_firme_gpt_granule_envelope(
            scenario->object_base, scenario->object_size, scenario->granule_size,
            &object_base, &object_size) != PAL_FIRME_GPT_SCENARIO_READY ||
        start_index > object_size / scenario->granule_size ||
        granule_count > object_size / scenario->granule_size - start_index) {
        val_firme_gpt_l1_fail(rule_id, out, step, start_index, granule_count);
        return VAL_FIRME_GPT_L1_FLOW_ERROR;
    }
    segment_base = object_base +
                   (start_index * scenario->granule_size);
    return val_firme_gpt_l1_check_gpis(
        rule_id, scenario, segment_base,
        granule_count * scenario->granule_size,
        0ULL, 0ULL, val_firme_gpt_l1_gpi_mask(gpi), step, out);
}

/**
 * @brief Validate DENIED CREATE leaves every object granule unchanged.
 * @param world Caller world.
 * @param base_pa Default test address.
 * @param out Rule result.
 * @return Internal rule result.
 */
static val_firme_gpt_l1_flow_t val_firme_gpt_l1_create_denied_rule(
    firme_acs_world_t world, uint64_t base_pa, firme_rule_res_t *out)
{
    pal_firme_gpt_l1_scenario_t scenario = {0};
    val_firme_gpt_l1_trace_t trace = {0};
    val_firme_gpt_l1_flow_t flow;
    val_firme_gpt_l1_flow_t cleanup_flow;
    uint64_t object_granules;
    uint64_t object_base;
    uint64_t object_size;
    uint64_t suffix_count;
    uint32_t post_invocation = VAL_FALSE;

    if (world != FIRME_ACS_WORLD_REALM)
        return val_firme_gpt_l1_disallowed_world(
            FIRME_RULE_R0173, PAL_FIRME_GPT_L1_CREATE, base_pa, out);

    flow = val_firme_gpt_l1_prepare(
        FIRME_RULE_R0173, world, PAL_FIRME_GPT_L1_CREATE,
        PAL_FIRME_GPT_L1_CONDITION_OBJECT_GPI_DENIED,
        base_pa, &scenario, out);
    if (flow != VAL_FIRME_GPT_L1_FLOW_PASS)
        return flow;

    if (pal_firme_gpt_granule_envelope(
            scenario.object_base, scenario.object_size, scenario.granule_size,
            &object_base, &object_size) != PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_l1_fail(FIRME_RULE_R0173, out,
            VAL_FIRME_GPT_L1_STEP_PREPARE, scenario.object_base, scenario.object_size);
        flow = VAL_FIRME_GPT_L1_FLOW_ERROR;
        goto cleanup;
    }
    object_granules = object_size / scenario.granule_size;
    if ((scenario.denied_granule_index >= object_granules) ||
        (scenario.denied_granule_gpi == FIRME_GPI_NONSECURE) ||
        (scenario.denied_granule_gpi > UINT64_C(0xf))) {
        val_firme_gpt_l1_fail(FIRME_RULE_R0173, out,
                              VAL_FIRME_GPT_L1_STEP_PREPARE,
                              scenario.denied_granule_index,
                              object_granules);
        flow = VAL_FIRME_GPT_L1_FLOW_ERROR;
        goto cleanup;
    }

check_state:
    flow = val_firme_gpt_l1_check_object_segment(
        &scenario, FIRME_RULE_R0173, 0ULL,
        scenario.denied_granule_index, FIRME_GPI_NONSECURE,
        post_invocation ? VAL_FIRME_GPT_L1_STEP_POSTSTATE :
                          VAL_FIRME_GPT_L1_STEP_PRESTATE,
        out);
    if (flow != VAL_FIRME_GPT_L1_FLOW_PASS)
        goto cleanup;

    flow = val_firme_gpt_l1_check_object_segment(
        &scenario, FIRME_RULE_R0173, scenario.denied_granule_index,
        1ULL, scenario.denied_granule_gpi,
        post_invocation ? VAL_FIRME_GPT_L1_STEP_POSTSTATE :
                          VAL_FIRME_GPT_L1_STEP_PRESTATE,
        out);
    if (flow != VAL_FIRME_GPT_L1_FLOW_PASS)
        goto cleanup;

    suffix_count = object_granules - scenario.denied_granule_index - 1ULL;
    flow = val_firme_gpt_l1_check_object_segment(
        &scenario, FIRME_RULE_R0173,
        scenario.denied_granule_index + 1ULL, suffix_count,
        FIRME_GPI_NONSECURE,
        post_invocation ? VAL_FIRME_GPT_L1_STEP_POSTSTATE :
                          VAL_FIRME_GPT_L1_STEP_PRESTATE,
        out);
    if ((flow == VAL_FIRME_GPT_L1_FLOW_PASS) &&
        (post_invocation == VAL_FALSE)) {
        flow = val_firme_gpt_l1_run_to_terminal(
            FIRME_RULE_R0173, PAL_FIRME_GPT_L1_CREATE,
            &scenario, (int64_t)FIRME_DENIED, &trace, out);
        if (flow == VAL_FIRME_GPT_L1_FLOW_PASS) {
            post_invocation = VAL_TRUE;
            goto check_state;
        }
    }

cleanup:
    cleanup_flow = val_firme_gpt_l1_cleanup(
        FIRME_RULE_R0173, world, PAL_FIRME_GPT_L1_CREATE,
        PAL_FIRME_GPT_L1_CONDITION_OBJECT_GPI_DENIED,
        &scenario, out);
    return (cleanup_flow == VAL_FIRME_GPT_L1_FLOW_ERROR) ?
           cleanup_flow : flow;
}

/**
 * @brief Validate that DESTROY scrubs every occupied granule before NS release.
 * @param world Caller world.
 * @param base_pa Default test address.
 * @param out Rule result.
 * @return Internal rule result.
 */
static val_firme_gpt_l1_flow_t val_firme_gpt_l1_scrub_rule(
    firme_acs_world_t world, uint64_t base_pa, firme_rule_res_t *out)
{
    static const pal_firme_gpt_l1_condition_t conditions[] = {
        PAL_FIRME_GPT_L1_CONDITION_SCRUB_SELF,
        PAL_FIRME_GPT_L1_CONDITION_SCRUB_NON_SELF,
    };
    pal_firme_gpt_l1_scenario_t scenario = {0};
    pal_firme_gpt_l1_evidence_t evidence = {0};
    val_firme_gpt_l1_trace_t trace = {0};
    pal_firme_gpt_scenario_status_t monitor_status;
    val_firme_gpt_l1_flow_t flow;
    val_firme_gpt_l1_flow_t cleanup_flow;
    size_t condition_index;
    uint64_t object_granule_base;
    uint64_t object_granule_size;

    if (world != FIRME_ACS_WORLD_REALM)
        return val_firme_gpt_l1_disallowed_world(
            FIRME_RULE_R0180, PAL_FIRME_GPT_L1_DESTROY, base_pa, out);

    for (condition_index = 0U;
         condition_index < (sizeof(conditions) / sizeof(conditions[0]));
         condition_index++) {
        flow = val_firme_gpt_l1_prepare(
            FIRME_RULE_R0180, world, PAL_FIRME_GPT_L1_DESTROY,
            conditions[condition_index], base_pa, &scenario, out);
        if (flow != VAL_FIRME_GPT_L1_FLOW_PASS)
            return flow;

        /* Step 1: Derive the full occupied envelope without changing the object geometry. */
        if (pal_firme_gpt_granule_envelope(
                scenario.object_base, scenario.object_size, scenario.granule_size,
                &object_granule_base, &object_granule_size) != PAL_FIRME_GPT_SCENARIO_READY) {
            val_firme_gpt_l1_fail(FIRME_RULE_R0180, out,
                                  VAL_FIRME_GPT_L1_STEP_PREPARE,
                                  scenario.object_base, scenario.object_size);
            flow = VAL_FIRME_GPT_L1_FLOW_ERROR;
            goto cleanup;
        }

        /* Step 2: Require independent full-envelope scrubbing before Non-secure release. */
        monitor_status = pal_firme_gpt_l1_monitor_begin(
            (uint32_t)world, PAL_FIRME_GPT_L1_DESTROY,
            conditions[condition_index], &scenario);
        if (monitor_status == PAL_FIRME_GPT_SCENARIO_UNAVAILABLE) {
            out->status = (uint64_t)ACS_STATUS_SKIP;
            flow = VAL_FIRME_GPT_L1_FLOW_SKIP;
        } else if (monitor_status != PAL_FIRME_GPT_SCENARIO_READY) {
            val_firme_gpt_l1_fail(FIRME_RULE_R0180, out,
                                  VAL_FIRME_GPT_L1_STEP_MONITOR,
                                  monitor_status,
                                  PAL_FIRME_GPT_SCENARIO_READY);
            flow = VAL_FIRME_GPT_L1_FLOW_ERROR;
        } else {
            flow = val_firme_gpt_l1_run_to_terminal(
                FIRME_RULE_R0180, PAL_FIRME_GPT_L1_DESTROY,
                &scenario, (int64_t)FIRME_SUCCESS, &trace, out);
            monitor_status = pal_firme_gpt_l1_monitor_end(
                (uint32_t)world, PAL_FIRME_GPT_L1_DESTROY,
                conditions[condition_index], &scenario, &evidence);
            if ((flow == VAL_FIRME_GPT_L1_FLOW_PASS) &&
                ((monitor_status != PAL_FIRME_GPT_SCENARIO_READY) ||
                 (evidence.scrubbed_size != object_granule_size) ||
                 (evidence.scrub_value_independent != VAL_TRUE) ||
                 (evidence.scrub_completed_before_nonsecure != VAL_TRUE))) {
                val_firme_gpt_l1_fail(
                    FIRME_RULE_R0180, out,
                    VAL_FIRME_GPT_L1_STEP_MONITOR,
                    evidence.scrubbed_size, object_granule_size);
                flow = (monitor_status == PAL_FIRME_GPT_SCENARIO_READY) ?
                       VAL_FIRME_GPT_L1_FLOW_FAIL :
                       VAL_FIRME_GPT_L1_FLOW_ERROR;
            }
        }

cleanup:
        /* Step 3: Always clean up the prepared scenario, retaining uncertain state as ERR. */
        cleanup_flow = val_firme_gpt_l1_cleanup(
            FIRME_RULE_R0180, world, PAL_FIRME_GPT_L1_DESTROY,
            conditions[condition_index], &scenario, out);
        if (cleanup_flow == VAL_FIRME_GPT_L1_FLOW_ERROR)
            return cleanup_flow;
        if (flow != VAL_FIRME_GPT_L1_FLOW_PASS)
            return flow;
    }

    return VAL_FIRME_GPT_L1_FLOW_PASS;
}

/**
 * @brief Validate DESTROY returns the exact L1 object base only on SUCCESS.
 * @param world Caller world.
 * @param base_pa Default test address.
 * @param out Rule result.
 * @return Internal rule result.
 */
static val_firme_gpt_l1_flow_t val_firme_gpt_l1_destroy_output_rule(
    firme_acs_world_t world, uint64_t base_pa, firme_rule_res_t *out)
{
    static const pal_firme_gpt_l1_condition_t conditions[] = {
        PAL_FIRME_GPT_L1_CONDITION_VALID_SELF,
        PAL_FIRME_GPT_L1_CONDITION_VALID_NON_SELF,
    };
    pal_firme_gpt_l1_scenario_t scenario = {0};
    val_firme_gpt_l1_trace_t trace = {0};
    val_firme_gpt_l1_flow_t flow;
    val_firme_gpt_l1_flow_t cleanup_flow;
    const firme_res_t *terminal_result;
    size_t condition_index;

    if (world != FIRME_ACS_WORLD_REALM)
        return val_firme_gpt_l1_disallowed_world(
            FIRME_RULE_R0185, PAL_FIRME_GPT_L1_DESTROY, base_pa, out);

    for (condition_index = 0U;
         condition_index < (sizeof(conditions) / sizeof(conditions[0]));
         condition_index++) {
        flow = val_firme_gpt_l1_prepare(
            FIRME_RULE_R0185, world, PAL_FIRME_GPT_L1_DESTROY,
            conditions[condition_index], base_pa, &scenario, out);
        if (flow != VAL_FIRME_GPT_L1_FLOW_PASS)
            return flow;

        flow = val_firme_gpt_l1_run_to_terminal(
            FIRME_RULE_R0185, PAL_FIRME_GPT_L1_DESTROY,
            &scenario, (int64_t)FIRME_SUCCESS, &trace, out);
        if (flow == VAL_FIRME_GPT_L1_FLOW_PASS) {
            terminal_result = &trace.result[trace.call_count - 1U];
            if (terminal_result->x2 != scenario.object_base) {
                val_firme_gpt_l1_fail(
                    FIRME_RULE_R0185, out,
                    VAL_FIRME_GPT_L1_STEP_OUTPUT,
                    terminal_result->x2, scenario.object_base);
                flow = VAL_FIRME_GPT_L1_FLOW_FAIL;
            }
        }

        cleanup_flow = val_firme_gpt_l1_cleanup(
            FIRME_RULE_R0185, world, PAL_FIRME_GPT_L1_DESTROY,
            conditions[condition_index], &scenario, out);
        if (cleanup_flow == VAL_FIRME_GPT_L1_FLOW_ERROR)
            return cleanup_flow;
        if (flow != VAL_FIRME_GPT_L1_FLOW_PASS)
            return flow;
    }

    return VAL_FIRME_GPT_L1_FLOW_PASS;
}

/**
 * @brief Validate only one create or destroy operation can own a range.
 * @param world Caller world.
 * @param base_pa Default test address.
 * @param out Rule result.
 * @return Internal rule result.
 */
static val_firme_gpt_l1_flow_t val_firme_gpt_l1_single_operation_rule(
    firme_acs_world_t world, uint64_t base_pa, firme_rule_res_t *out)
{
    static const pal_firme_gpt_l1_operation_t operations[] = {
        PAL_FIRME_GPT_L1_CREATE,
        PAL_FIRME_GPT_L1_DESTROY,
    };
    pal_firme_gpt_l1_scenario_t scenario = {0};
    firme_res_t result = {0};
    val_firme_gpt_l1_trace_t trace = {0};
    val_firme_gpt_l1_flow_t flow;
    val_firme_gpt_l1_flow_t cleanup_flow;
    pal_firme_gpt_l1_operation_t operation;
    pal_firme_gpt_l1_operation_t competing_operation;
    uint64_t original_object_base;
    size_t operation_index;

    if (world != FIRME_ACS_WORLD_REALM)
        return val_firme_gpt_l1_disallowed_world(
            FIRME_RULE_R0191, PAL_FIRME_GPT_L1_CREATE, base_pa, out);

    for (operation_index = 0U;
         operation_index < (sizeof(operations) / sizeof(operations[0]));
         operation_index++) {
        operation = operations[operation_index];
        competing_operation =
            (operation == PAL_FIRME_GPT_L1_CREATE) ?
            PAL_FIRME_GPT_L1_DESTROY : PAL_FIRME_GPT_L1_CREATE;
        flow = val_firme_gpt_l1_prepare(
            FIRME_RULE_R0191, world, operation,
            PAL_FIRME_GPT_L1_CONDITION_INCOMPLETE_NON_SELF,
            base_pa, &scenario, out);
        if (flow != VAL_FIRME_GPT_L1_FLOW_PASS)
            return flow;
        original_object_base = scenario.object_base;

        flow = val_firme_gpt_l1_invoke_expected(
            FIRME_RULE_R0191, operation, &scenario,
            (int64_t)FIRME_INCOMPLETE, &result, out);
        if (flow == VAL_FIRME_GPT_L1_FLOW_PASS) {
            flow = val_firme_gpt_l1_invoke_expected(
                FIRME_RULE_R0191, competing_operation, &scenario,
                (int64_t)FIRME_OP_CONFLICT, &result, out);
        }
        if ((flow == VAL_FIRME_GPT_L1_FLOW_PASS) &&
            (operation == PAL_FIRME_GPT_L1_CREATE)) {
            flow = val_firme_gpt_l1_invoke_expected(
                FIRME_RULE_R0191, PAL_FIRME_GPT_L1_CREATE,
                &scenario, (int64_t)FIRME_OP_CONFLICT,
                &result, out);
        }
        if (flow == VAL_FIRME_GPT_L1_FLOW_PASS) {
            if (operation == PAL_FIRME_GPT_L1_CREATE)
                scenario.object_base = 0ULL;
            flow = val_firme_gpt_l1_run_to_terminal(
                FIRME_RULE_R0191, operation, &scenario,
                (int64_t)FIRME_SUCCESS, &trace, out);
            scenario.object_base = original_object_base;
        }

        cleanup_flow = val_firme_gpt_l1_cleanup(
            FIRME_RULE_R0191, world, operation,
            PAL_FIRME_GPT_L1_CONDITION_INCOMPLETE_NON_SELF,
            &scenario, out);
        if (cleanup_flow == VAL_FIRME_GPT_L1_FLOW_ERROR)
            return cleanup_flow;
        if (flow != VAL_FIRME_GPT_L1_FLOW_PASS)
            return flow;
    }

    return VAL_FIRME_GPT_L1_FLOW_PASS;
}

/**
 * @brief Execute one strict ALP4 L1 GPT lifecycle rule algorithm.
 * @param rule_id Rule implemented by the dedicated test source.
 * @param world FIRME caller world.
 * @param base_pa Default platform test address.
 * @param out Rule result and diagnostics.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
int val_firme_gpt_l1_rule_run(firme_rule_id_t rule_id,
                              firme_acs_world_t world,
                              uint64_t base_pa,
                              firme_rule_res_t *out)
{
    val_firme_gpt_l1_flow_t flow;

    if (!out)
        return VAL_ERROR;

    val_firme_gpt_rule_result_init(out);

    switch (rule_id) {
    case FIRME_RULE_R0156:
        flow = val_firme_gpt_l1_bridge_rule(world, base_pa, out);
        break;
    case FIRME_RULE_R0161:
        flow = val_firme_gpt_l1_transition_policy_rule(
            rule_id, world, PAL_FIRME_GPT_L1_CREATE, base_pa, out);
        break;
    case FIRME_RULE_R0167:
        flow = val_firme_gpt_l1_status_rule(
            rule_id, world, PAL_FIRME_GPT_L1_CREATE, base_pa, out);
        break;
    case FIRME_RULE_R0168:
        flow = val_firme_gpt_l1_range_stability_rule(
            rule_id, world, PAL_FIRME_GPT_L1_CREATE, base_pa, out);
        break;
    case FIRME_RULE_R0170:
        flow = val_firme_gpt_l1_object_gpi_rule(
            rule_id, world, PAL_FIRME_GPT_L1_CREATE,
            VAL_FALSE, base_pa, out);
        break;
    case FIRME_RULE_R0171:
        flow = val_firme_gpt_l1_object_gpi_rule(
            rule_id, world, PAL_FIRME_GPT_L1_CREATE,
            VAL_TRUE, base_pa, out);
        break;
    case FIRME_RULE_R0173:
        flow = val_firme_gpt_l1_create_denied_rule(world, base_pa, out);
        break;
    case FIRME_RULE_R0179:
        flow = val_firme_gpt_l1_transition_policy_rule(
            rule_id, world, PAL_FIRME_GPT_L1_DESTROY, base_pa, out);
        break;
    case FIRME_RULE_R0180:
        flow = val_firme_gpt_l1_scrub_rule(world, base_pa, out);
        break;
    case FIRME_RULE_R0184:
        flow = val_firme_gpt_l1_status_rule(
            rule_id, world, PAL_FIRME_GPT_L1_DESTROY, base_pa, out);
        break;
    case FIRME_RULE_R0185:
        flow = val_firme_gpt_l1_destroy_output_rule(world, base_pa, out);
        break;
    case FIRME_RULE_R0186:
        flow = val_firme_gpt_l1_range_stability_rule(
            rule_id, world, PAL_FIRME_GPT_L1_DESTROY, base_pa, out);
        break;
    case FIRME_RULE_R0188:
        flow = val_firme_gpt_l1_object_gpi_rule(
            rule_id, world, PAL_FIRME_GPT_L1_DESTROY,
            VAL_FALSE, base_pa, out);
        break;
    case FIRME_RULE_R0189:
        flow = val_firme_gpt_l1_object_gpi_rule(
            rule_id, world, PAL_FIRME_GPT_L1_DESTROY,
            VAL_TRUE, base_pa, out);
        break;
    case FIRME_RULE_R0191:
        flow = val_firme_gpt_l1_single_operation_rule(
            world, base_pa, out);
        break;
    default:
        val_firme_gpt_l1_fail(rule_id, out,
                              VAL_FIRME_GPT_L1_STEP_PREPARE,
                              (uint64_t)rule_id, 0ULL);
        flow = VAL_FIRME_GPT_L1_FLOW_ERROR;
        break;
    }

    return val_firme_gpt_l1_complete(flow, out);
}
