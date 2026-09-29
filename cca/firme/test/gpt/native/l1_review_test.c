/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>

#include "../../../val/src/val_firme_gpt.c"
#include "../../../val/src/val_firme_gpt_l1.c"
#define pal_firme_gpt_l1_scenario_prepare native_unused_l1_prepare
#define pal_firme_gpt_l1_scenario_cleanup native_unused_l1_cleanup
#define pal_firme_gpt_l1_gpi_range_check native_unused_l1_check
#include "../../../platform/pal_baremetal/targets/fvp/src/pal_firme_gpt.c"
#undef pal_firme_gpt_l1_scenario_prepare
#undef pal_firme_gpt_l1_scenario_cleanup
#undef pal_firme_gpt_l1_gpi_range_check

#define REVIEW_RANGE_BASE UINT64_C(0x40000000)
#define REVIEW_RANGE_SIZE UINT64_C(0x40000000)
#define REVIEW_GRANULE UINT64_C(0x10000)
#define REVIEW_OBJECT_SIZE UINT64_C(0x2000)
#define REVIEW_OBJECT_BASE (REVIEW_RANGE_BASE + REVIEW_OBJECT_SIZE)

static uint32_t g_l1_calls;
static uint32_t g_l1_checks;
static uint32_t g_l1_cleanup;
static uint32_t g_l1_poststate_bad;
static uint32_t g_l1_resume;
static uint32_t g_l1_sequence_count;
static int64_t g_l1_sequence[4];
static uint64_t g_l1_checked_base;
static uint64_t g_l1_checked_size;
static uint64_t g_l1_excluded_size;

/** @brief This fixture supplies PAL scenarios and must not acquire an owned lease. */
pal_firme_gpt_scenario_status_t val_firme_gpt_fixture_begin(
    val_firme_gpt_fixture_t *fixture, firme_acs_world_t world,
    pal_firme_gpt_memory_kind_t kind)
{
    assert(VAL_FALSE);
    return PAL_FIRME_GPT_SCENARIO_ERROR;
}

/** @brief Reject unexpected owned-session operation recording. */
int val_firme_gpt_fixture_l1_record(val_firme_gpt_fixture_t *fixture,
    uint32_t destroy, const firme_res_t *result)
{
    assert(VAL_FALSE);
    return VAL_ERROR;
}

/** @brief Reject unexpected owned-session continuation. */
int val_firme_gpt_fixture_l1_complete(val_firme_gpt_fixture_t *fixture,
    uint32_t destroy)
{
    assert(VAL_FALSE);
    return VAL_ERROR;
}

/** @brief Require cleanup through the prepared PAL fixture instead. */
int val_firme_gpt_fixture_cleanup(val_firme_gpt_fixture_t *fixture)
{
    assert(VAL_FALSE);
    return VAL_ERROR;
}

/** @brief Suppress diagnostics from intentional negative cases. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *fmt, ...)
{
    return 0U;
}

/** @brief Decode mocked architectural statuses. */
int64_t val_firme_status_to_i64(uint64_t value)
{
    return (int64_t)value;
}

/** @brief Advertise the optional pair and the 64-KiB/30-bit geometry. */
void val_firme_service_features(uint64_t service, uint64_t reg, firme_res_t *result)
{
    *result = (firme_res_t){.x0 = FIRME_SUCCESS};
    result->x1 = reg == FIRME_GM_FEATURE_REG0_INDEX ?
        FIRME_GM_FEATURE_REG0_L1_CREATE_BIT | FIRME_GM_FEATURE_REG0_L1_DESTROY_BIT :
        (FIRME_GM_PGS_64KB << FIRME_GM_FEATURE_REG1_PGS_SHIFT) |
        (FIRME_GM_L0GPTSZ_30BITS << FIRME_GM_FEATURE_REG1_L0GPTSZ_SHIFT) |
        (FIRME_GM_PPS_1TB << FIRME_GM_FEATURE_REG1_PPS_SHIFT);
}

/** @brief Prepare an 8-KiB table inside one 64-KiB Root granule. */
pal_firme_gpt_scenario_status_t pal_firme_gpt_l1_scenario_prepare(
    uint32_t world, pal_firme_gpt_l1_operation_t operation,
    pal_firme_gpt_l1_condition_t condition, uint64_t base,
    pal_firme_gpt_l1_scenario_t *scenario)
{
    *scenario = (pal_firme_gpt_l1_scenario_t){
        .range_base = REVIEW_RANGE_BASE, .range_size = REVIEW_RANGE_SIZE,
        .object_base = REVIEW_OBJECT_BASE, .object_size = REVIEW_OBJECT_SIZE,
        .granule_size = REVIEW_GRANULE, .range_initial_gpi = FIRME_GPI_NONSECURE,
        .object_initial_gpi = FIRME_GPI_NONSECURE, .denied_granule_index = 0ULL,
        .denied_granule_gpi = FIRME_GPI_ROOT,
        .layout = PAL_FIRME_GPT_L1_LAYOUT_SELF_DESCRIBING};
    return PAL_FIRME_GPT_SCENARIO_READY;
}

/** @brief Count cleanup even when rollback validation fails. */
pal_firme_gpt_scenario_status_t pal_firme_gpt_l1_scenario_cleanup(
    uint32_t world, pal_firme_gpt_l1_operation_t operation,
    pal_firme_gpt_l1_condition_t condition,
    const pal_firme_gpt_l1_scenario_t *scenario)
{
    g_l1_cleanup++;
    return PAL_FIRME_GPT_SCENARIO_READY;
}

/** @brief Require complete granule observation and optionally corrupt rollback. */
pal_firme_gpt_scenario_status_t pal_firme_gpt_l1_gpi_range_check(
    uint64_t base, uint64_t size, uint64_t granule_size,
    uint64_t exclude_base, uint64_t exclude_size,
    uint32_t mask, uint64_t *checked_count)
{
    assert(granule_size == REVIEW_GRANULE);
    assert(base % REVIEW_GRANULE == 0ULL && size % REVIEW_GRANULE == 0ULL);
    assert(exclude_size % REVIEW_GRANULE == 0ULL);
    g_l1_checked_base = base;
    g_l1_checked_size = size;
    g_l1_excluded_size = exclude_size;
    g_l1_checks++;
    *checked_count = (size - exclude_size) / granule_size;
    return g_l1_calls && g_l1_poststate_bad ? PAL_FIRME_GPT_SCENARIO_MISMATCH :
                                           PAL_FIRME_GPT_SCENARIO_READY;
}

/** @brief Check continuation inputs while replaying an architectural sequence. */
void val_firme_gm_l1_gpt_create(uint64_t base, uint64_t object, firme_res_t *result)
{
    int64_t status;

    assert(base == REVIEW_RANGE_BASE);
    assert(object == (g_l1_resume ? 0ULL : REVIEW_OBJECT_BASE));
    status = g_l1_calls < g_l1_sequence_count ? g_l1_sequence[g_l1_calls] : FIRME_BUSY;
    g_l1_calls++;
    if (status == (int64_t)FIRME_INCOMPLETE)
        g_l1_resume = VAL_TRUE;
    *result = (firme_res_t){.x0 = (uint64_t)status};
}

/** @brief Fail if a CREATE rollback accidentally dispatches DESTROY. */
void val_firme_gm_l1_gpt_destroy(uint64_t base, firme_res_t *result)
{
    assert(VAL_FALSE);
}

/** @brief Reset fixture evidence before a new rule invocation. */
static void l1_review_reset(void)
{
    g_l1_calls = 0U;
    g_l1_checks = 0U;
    g_l1_cleanup = 0U;
    g_l1_poststate_bad = VAL_FALSE;
    g_l1_resume = VAL_FALSE;
    g_l1_sequence_count = 1U;
    g_l1_sequence[0] = FIRME_DENIED;
}

/** @brief Exercise sub-granule objects, exclusions and terminal rollback checks. */
int main(void)
{
    pal_firme_gpt_l1_scenario_t scenario;
    firme_rule_res_t result = {0};
    uint64_t covered_base;
    uint64_t covered_size;

    l1_review_reset();
    pal_firme_gpt_l1_scenario_prepare(FIRME_ACS_WORLD_REALM, PAL_FIRME_GPT_L1_CREATE,
        PAL_FIRME_GPT_L1_CONDITION_OBJECT_GPI_DENIED, REVIEW_RANGE_BASE, &scenario);
    assert(val_firme_gpt_l1_validate_scenario(FIRME_RULE_R0173,
        PAL_FIRME_GPT_L1_CONDITION_OBJECT_GPI_DENIED, &scenario, &result) ==
        VAL_FIRME_GPT_L1_FLOW_PASS);
    assert(val_firme_gpt_l1_check_gpis(FIRME_RULE_R0173, &scenario,
        scenario.object_base, scenario.object_size, 0ULL, 0ULL,
        val_firme_gpt_l1_gpi_mask(FIRME_GPI_ROOT), 1ULL, &result) ==
        VAL_FIRME_GPT_L1_FLOW_PASS);
    assert(g_l1_checked_base == REVIEW_RANGE_BASE && g_l1_checked_size == REVIEW_GRANULE);
    assert(val_firme_gpt_l1_check_range_unchanged(FIRME_RULE_R0173,
        &scenario, 1ULL, &result) == VAL_FIRME_GPT_L1_FLOW_PASS);
    assert(g_l1_excluded_size == REVIEW_GRANULE);
    assert(scenario.object_size == REVIEW_OBJECT_SIZE);
    scenario.object_size++;
    assert(val_firme_gpt_l1_validate_scenario(FIRME_RULE_R0173,
        PAL_FIRME_GPT_L1_CONDITION_OBJECT_GPI_DENIED, &scenario, &result) ==
        VAL_FIRME_GPT_L1_FLOW_ERROR);
    assert(pal_firme_gpt_granule_envelope(UINT64_MAX - 1ULL, 2ULL, REVIEW_GRANULE,
        &covered_base, &covered_size) == PAL_FIRME_GPT_SCENARIO_ERROR);
    assert(pal_firme_gpt_granule_envelope(REVIEW_OBJECT_BASE, 0ULL, REVIEW_GRANULE,
        &covered_base, &covered_size) == PAL_FIRME_GPT_SCENARIO_ERROR);

    l1_review_reset();
    assert(val_firme_gpt_l1_create_denied_rule(FIRME_ACS_WORLD_REALM,
        REVIEW_RANGE_BASE, &result) == VAL_FIRME_GPT_L1_FLOW_PASS);
    assert(g_l1_calls == 1U && g_l1_checks == 2U && g_l1_cleanup == 1U);

    l1_review_reset();
    g_l1_sequence_count = 4U;
    g_l1_sequence[0] = FIRME_BUSY;
    g_l1_sequence[1] = FIRME_INCOMPLETE;
    g_l1_sequence[2] = FIRME_BUSY;
    g_l1_sequence[3] = FIRME_DENIED;
    assert(val_firme_gpt_l1_create_denied_rule(FIRME_ACS_WORLD_REALM,
        REVIEW_RANGE_BASE, &result) == VAL_FIRME_GPT_L1_FLOW_PASS);
    assert(g_l1_calls == 4U && g_l1_checks == 2U && g_l1_cleanup == 1U);

    l1_review_reset();
    g_l1_poststate_bad = VAL_TRUE;
    assert(val_firme_gpt_l1_create_denied_rule(FIRME_ACS_WORLD_REALM,
        REVIEW_RANGE_BASE, &result) == VAL_FIRME_GPT_L1_FLOW_FAIL);
    assert(g_l1_cleanup == 1U && result.fail_step == VAL_FIRME_GPT_L1_STEP_POSTSTATE);

    l1_review_reset();
    g_l1_sequence[0] = FIRME_SUCCESS;
    assert(val_firme_gpt_l1_create_denied_rule(FIRME_ACS_WORLD_REALM,
        REVIEW_RANGE_BASE, &result) == VAL_FIRME_GPT_L1_FLOW_FAIL);
    assert(g_l1_checks == 1U && g_l1_cleanup == 1U);

    l1_review_reset();
    g_l1_sequence_count = 0U;
    assert(val_firme_gpt_l1_create_denied_rule(FIRME_ACS_WORLD_REALM,
        REVIEW_RANGE_BASE, &result) == VAL_FIRME_GPT_L1_FLOW_FAIL);
    assert(g_l1_calls == VAL_FIRME_GPT_L1_MAX_CALLS && g_l1_cleanup == 1U);
    puts("L1 review regression: PASS");
    return VAL_SUCCESS;
}
