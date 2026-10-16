/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt_fixture.h"

#include <stddef.h>

#include "val_common/val_status.h"
#include "val_log.h"

#define GPT_FIXTURE_MAX_CALLS 32U
#define GPT_FIXTURE_GPI_ANY UINT64_C(0xf)

/** @brief Route every setup, operation and cleanup SMC through the selected caller. */
static int gpt_fixture_call(val_firme_gpt_fixture_t *fixture, uint64_t fid,
                            uint64_t argument0, uint64_t argument1,
                            uint64_t argument2, firme_res_t *result)
{
    smccc_call_frame_t input = {0};
    smccc_call_frame_t output = {0};
    uint32_t issued = VAL_FALSE;

    if (!fixture || !result)
        return VAL_ERROR;

    *result = (firme_res_t){0};
    if (fixture->has_abi_context) {
        input.registers[0] = fid;
        input.registers[1] = argument0;
        input.registers[2] = argument1;
        input.registers[3] = argument2;
        if (val_firme_abi_call(&fixture->abi_context, &input, &output, &issued) != VAL_SUCCESS) {
            if (issued)
                fixture->poisoned = VAL_TRUE;

            return VAL_ERROR;
        }

        result->x0 = output.registers[0];
        result->x1 = output.registers[1];
        result->x2 = output.registers[2];
        result->x3 = output.registers[3];
        return VAL_SUCCESS;
    }

    switch (fid) {
    case FIRME_FID_SERVICE_FEATURES:
        val_firme_service_features(argument0, argument1, result);
        break;
    case FIRME_FID_GM_GPT_INFO:
        val_firme_gm_gpt_info(argument0, argument1, result);
        break;
    case FIRME_FID_GM_GPI_SET:
        val_firme_gm_gpi_set(argument0, argument1, argument2, result);
        break;
    case FIRME_FID_GM_GPI_OP_CONTINUE:
        val_firme_gm_gpi_op_continue(argument0, result);
        break;
    case FIRME_FID_GM_L1_GPT_CREATE:
        val_firme_gm_l1_gpt_create(argument0, argument1, result);
        break;
    case FIRME_FID_GM_L1_GPT_DESTROY:
        val_firme_gm_l1_gpt_destroy(argument0, result);
        break;
    default:
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/** @brief Require complete independent GPI observation of an owned extent. */
static int gpt_fixture_observe(const val_firme_gpt_fixture_t *fixture,
                               uint64_t base, uint64_t size, uint64_t gpi)
{
    uint64_t checked = 0ULL;

    if (gpi > UINT64_C(0xf))
        return VAL_ERROR;
    return (pal_firme_gpt_l1_gpi_range_check(
                base, size, fixture->granule_size, 0ULL, 0ULL,
                UINT32_C(1) << gpi, &checked) == PAL_FIRME_GPT_SCENARIO_READY &&
            checked == size / fixture->granule_size) ? VAL_SUCCESS : VAL_ERROR;
}

/** @brief Require the exact observed state of one protected address range. */
static int gpt_fixture_par(val_firme_gpt_fixture_t *fixture,
                           uint64_t base, uint64_t expected_state)
{
    firme_res_t result = {0};

    if (gpt_fixture_call(fixture, FIRME_FID_GM_GPT_INFO, base,
                          base + fixture->range_size, 0ULL, &result) != VAL_SUCCESS)
        return VAL_ERROR;

    return (val_firme_status_to_i64(result.x0) == (int64_t)FIRME_SUCCESS &&
            result.x1 == base + fixture->range_size &&
            result.x2 == expected_state) ? VAL_SUCCESS : VAL_ERROR;
}

/** @brief Observe every occupied granule without conflating object bytes and ownership. */
static int gpt_fixture_l1_observe(val_firme_gpt_fixture_t *fixture)
{
    uint64_t checked = 0ULL;
    uint64_t exclude_base = 0ULL;
    uint64_t exclude_size = 0ULL;
    uint32_t object_mask;

    if (fixture->kind == PAL_FIRME_GPT_MEMORY_L1_SELF) {
        exclude_base = fixture->object_granule_base;
        exclude_size = fixture->object_granule_size;
    }
    object_mask = UINT32_C(1) << (fixture->l1_present ?
                                  FIRME_GPI_ROOT : FIRME_GPI_NONSECURE);
    if (fixture->pending)
        object_mask = (UINT32_C(1) << FIRME_GPI_ROOT) |
                      (UINT32_C(1) << FIRME_GPI_NONSECURE);
    if (pal_firme_gpt_l1_gpi_range_check(fixture->object_granule_base,
            fixture->object_granule_size, fixture->granule_size, 0ULL, 0ULL,
            object_mask, &checked) != PAL_FIRME_GPT_SCENARIO_READY ||
        checked != fixture->object_granule_size / fixture->granule_size)
        return VAL_ERROR;
    if (pal_firme_gpt_l1_gpi_range_check(fixture->memory.range_base,
            fixture->memory.range_size, fixture->granule_size,
            exclude_base, exclude_size,
            UINT32_C(1) << fixture->memory.initial_gpi, &checked) !=
                PAL_FIRME_GPT_SCENARIO_READY ||
        checked != (fixture->memory.range_size - exclude_size) / fixture->granule_size)
        return VAL_ERROR;
    if (!fixture->pending)
        return gpt_fixture_par(fixture, fixture->memory.range_base,
            fixture->l1_present ? FIRME_GPT_PAR_STATE_HOST_TABLE :
                                  FIRME_GPT_PAR_STATE_HOST_BLOCK);
    return VAL_SUCCESS;
}

/** @brief Validate complete allocation containment before any destructive call. */
static int gpt_fixture_geometry(val_firme_gpt_fixture_t *fixture, uint64_t pps)
{
    const pal_firme_gpt_memory_t *memory = &fixture->memory;
    uint64_t object_top;
    uint64_t range_top;

    if (!memory->token || !memory->range_size ||
        memory->range_base >= pps || memory->range_size > pps - memory->range_base ||
        memory->range_base % fixture->granule_size ||
        memory->range_size % fixture->granule_size)
        return VAL_ERROR;
    fixture->count = memory->range_size / fixture->granule_size;
    if (fixture->kind == PAL_FIRME_GPT_MEMORY_TRANSITION)
        return memory->initial_gpi == FIRME_GPI_NONSECURE ? VAL_SUCCESS : VAL_ERROR;
    if (memory->range_base % fixture->range_size ||
        memory->range_size != fixture->range_size || !memory->object_base ||
        memory->object_base % fixture->object_size ||
        !memory->pool_size || memory->pool_base >= pps ||
        memory->pool_size > pps - memory->pool_base ||
        memory->pool_base % fixture->granule_size ||
        memory->pool_size % fixture->granule_size ||
        pal_firme_gpt_granule_envelope(memory->object_base, fixture->object_size,
            fixture->granule_size, &fixture->object_granule_base,
            &fixture->object_granule_size) != PAL_FIRME_GPT_SCENARIO_READY)
        return VAL_ERROR;
    object_top = fixture->object_granule_base + fixture->object_granule_size;
    range_top = memory->range_base + memory->range_size;
    if (fixture->object_granule_base < memory->pool_base ||
        object_top > memory->pool_base + memory->pool_size)
        return VAL_ERROR;
    if (fixture->kind == PAL_FIRME_GPT_MEMORY_L1_SELF)
        return (memory->initial_gpi == FIRME_GPI_NONSECURE &&
                memory->pool_base >= memory->range_base &&
                memory->pool_base + memory->pool_size <= range_top) ?
               VAL_SUCCESS : VAL_ERROR;
    if (memory->pool_base < range_top &&
        memory->pool_base + memory->pool_size > memory->range_base)
        return VAL_ERROR;
    return (memory->initial_gpi == FIRME_GPI_NONSECURE ||
            memory->initial_gpi == GPT_FIXTURE_GPI_ANY) ? VAL_SUCCESS : VAL_ERROR;
}

/** @brief Discover geometry and validate independently reserved pre-state. */
static pal_firme_gpt_scenario_status_t gpt_fixture_begin(
    val_firme_gpt_fixture_t *fixture, firme_acs_world_t world,
    pal_firme_gpt_memory_kind_t kind, val_firme_abi_test_context_t *context)
{
    firme_res_t result = {0};
    pal_firme_gpt_scenario_status_t status;
    uint64_t required_bits;
    uint64_t feature;
    uint64_t pps;

    if (!fixture || fixture->memory.token ||
        (uint32_t)world > FIRME_ACS_WORLD_REALM ||
        (uint32_t)kind > PAL_FIRME_GPT_MEMORY_L1_NON_SELF)
        return PAL_FIRME_GPT_SCENARIO_ERROR;
    *fixture = (val_firme_gpt_fixture_t){0};
    fixture->world = world;
    fixture->kind = kind;
    if (context) {
        fixture->abi_context = *context;
        fixture->has_abi_context = VAL_TRUE;
    }

    if (kind != PAL_FIRME_GPT_MEMORY_TRANSITION && world != FIRME_ACS_WORLD_REALM)
        return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
    required_bits = FIRME_GM_FEATURE_REG0_GPI_SET_BIT |
                    FIRME_GM_FEATURE_REG0_OP_CONTINUE_BIT;
    if (kind == PAL_FIRME_GPT_MEMORY_TRANSITION && context && context->abi &&
        context->abi->id == VAL_FIRME_ABI_ID_GM_GPI_SET)
        required_bits = FIRME_GM_FEATURE_REG0_GPI_SET_BIT;
    if (kind != PAL_FIRME_GPT_MEMORY_TRANSITION)
        required_bits = FIRME_GM_FEATURE_REG0_L1_CREATE_BIT |
                        FIRME_GM_FEATURE_REG0_L1_DESTROY_BIT |
                        FIRME_GM_FEATURE_REG0_GPT_INFO_BIT;
    if (gpt_fixture_call(fixture, FIRME_FID_SERVICE_FEATURES,
            FIRME_SERVICE_ID_GRANULE_MGMT, FIRME_GM_FEATURE_REG0_INDEX,
            0ULL, &result) != VAL_SUCCESS)
        return PAL_FIRME_GPT_SCENARIO_ERROR;

    fixture->last_status = val_firme_status_to_i64(result.x0);
    if (val_firme_status_to_i64(result.x0) != (int64_t)FIRME_SUCCESS)
        return PAL_FIRME_GPT_SCENARIO_ERROR;
    fixture->continue_advertised =
        (result.x1 & FIRME_GM_FEATURE_REG0_OP_CONTINUE_BIT) ? VAL_TRUE : VAL_FALSE;
    if ((result.x1 & required_bits) != required_bits) {
        LOG_WARN(NULL, "GPT fixture: world=%u feature reg0=%llx missing ABI bits=%llx",
                 (unsigned int)world, (unsigned long long)result.x1,
                 (unsigned long long)(required_bits & ~result.x1));
        return (kind == PAL_FIRME_GPT_MEMORY_TRANSITION ||
                (result.x1 & (FIRME_GM_FEATURE_REG0_L1_CREATE_BIT |
                              FIRME_GM_FEATURE_REG0_L1_DESTROY_BIT))) ?
               PAL_FIRME_GPT_SCENARIO_ERROR : PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
    }

    if (gpt_fixture_call(fixture, FIRME_FID_SERVICE_FEATURES,
            FIRME_SERVICE_ID_GRANULE_MGMT, FIRME_GM_FEATURE_REG1_INDEX,
            0ULL, &result) != VAL_SUCCESS)
        return PAL_FIRME_GPT_SCENARIO_ERROR;

    fixture->last_status = val_firme_status_to_i64(result.x0);
    if (val_firme_status_to_i64(result.x0) != (int64_t)FIRME_SUCCESS)
        return PAL_FIRME_GPT_SCENARIO_ERROR;
    feature = result.x1;
    fixture->granule_size = val_firme_gpt_pgs_size_from_feature_reg1(feature);
    fixture->range_size = val_firme_gpt_range_size_from_feature_reg1(feature);
    pps = val_firme_gpt_pps_size_from_feature_reg1(feature);
    if (!fixture->granule_size || !fixture->range_size || !pps)
        return PAL_FIRME_GPT_SCENARIO_ERROR;
    fixture->object_size = fixture->range_size / fixture->granule_size / 2ULL;
    fixture->target_gpi = world == FIRME_ACS_WORLD_REALM ?
                          FIRME_GPI_REALM :
                          (world == FIRME_ACS_WORLD_SECURE ? FIRME_GPI_SECURE :
                                                           FIRME_GPI_NONSECURE);
    status = pal_firme_gpt_memory_acquire(world, kind, fixture->granule_size,
        fixture->range_size, fixture->object_size, &fixture->memory);
    if (status != PAL_FIRME_GPT_SCENARIO_READY) {
        if (fixture->memory.token)
            fixture->poisoned = VAL_TRUE;
        return fixture->memory.token ? PAL_FIRME_GPT_SCENARIO_ERROR : status;
    }
    fixture->poisoned = VAL_TRUE;
    if (gpt_fixture_geometry(fixture, pps) != VAL_SUCCESS ||
        gpt_fixture_observe(fixture, fixture->memory.range_base,
            fixture->memory.range_size, fixture->memory.initial_gpi) != VAL_SUCCESS)
        return PAL_FIRME_GPT_SCENARIO_ERROR;
    if (kind != PAL_FIRME_GPT_MEMORY_TRANSITION &&
        (gpt_fixture_par(fixture, fixture->memory.range_base,
             FIRME_GPT_PAR_STATE_HOST_BLOCK) != VAL_SUCCESS ||
         gpt_fixture_observe(fixture, fixture->memory.pool_base,
             fixture->memory.pool_size, FIRME_GPI_NONSECURE) != VAL_SUCCESS))
        return PAL_FIRME_GPT_SCENARIO_ERROR;
    fixture->poisoned = VAL_FALSE;
    fixture->last_status = FIRME_SUCCESS;
    return PAL_FIRME_GPT_SCENARIO_READY;
}

/** @brief Acquire an in-world fixture for a handwritten GPT lifecycle test. */
pal_firme_gpt_scenario_status_t val_firme_gpt_fixture_begin(
    val_firme_gpt_fixture_t *fixture, firme_acs_world_t world,
    pal_firme_gpt_memory_kind_t kind)
{
    return gpt_fixture_begin(fixture, world, kind, NULL);
}

/** @brief Keep generated setup and cleanup in the same caller as the tested ABI. */
pal_firme_gpt_scenario_status_t val_firme_gpt_fixture_begin_abi(
    val_firme_gpt_fixture_t *fixture, val_firme_abi_test_context_t *context,
    pal_firme_gpt_memory_kind_t kind)
{
    if (!context)
        return PAL_FIRME_GPT_SCENARIO_ERROR;

    return gpt_fixture_begin(fixture, context->world, kind, context);
}

/** @brief Prove each completed granule and untouched suffix independently of status. */
static int gpt_fixture_progress_observe(val_firme_gpt_fixture_t *fixture)
{
    if ((fixture->completed && gpt_fixture_observe(fixture,
            fixture->memory.range_base, fixture->completed * fixture->granule_size,
            fixture->target_gpi) != VAL_SUCCESS) ||
        (!fixture->restoring && fixture->completed < fixture->count &&
         gpt_fixture_observe(fixture,
            fixture->memory.range_base + fixture->completed * fixture->granule_size,
            (fixture->count - fixture->completed) * fixture->granule_size,
            FIRME_GPI_NONSECURE) != VAL_SUCCESS)) {
        fixture->poisoned = VAL_TRUE;
        return VAL_ERROR;
    }
    return VAL_SUCCESS;
}

/** @brief Record only architecturally valid progress and retained cookies. */
int val_firme_gpt_fixture_gpi_record(val_firme_gpt_fixture_t *fixture,
                                      const firme_res_t *result)
{
    uint64_t remaining;
    int64_t status;
    uint32_t rejected_start;

    if (!fixture || !result || !fixture->memory.token || fixture->poisoned ||
        fixture->kind != PAL_FIRME_GPT_MEMORY_TRANSITION ||
        fixture->completed >= fixture->count)
        return VAL_ERROR;
    remaining = fixture->count - fixture->completed;
    status = val_firme_status_to_i64(result->x0);
    fixture->last_status = status;
    rejected_start = !fixture->pending &&
        (status == (int64_t)FIRME_INVALID_PARAMETERS ||
         status == (int64_t)FIRME_NOT_SUPPORTED);
    if (!rejected_start &&
        val_firme_gpt_operation_count_valid(status, result->x1, remaining) != VAL_TRUE) {
        fixture->poisoned = VAL_TRUE;
        LOG_ERROR(NULL, "GPT fixture: invalid progress x0=%llx count=%llx "
                  "remaining=%llu cookie=%llx\n", (unsigned long long)result->x0,
                  (unsigned long long)result->x1, (unsigned long long)remaining,
                  (unsigned long long)result->x2);
        return VAL_ERROR;
    }
    if (status == (int64_t)FIRME_BUSY) {
        /* Step 1: BUSY consumes no count and only replaces an already-retained cookie. */
        if (fixture->pending)
            fixture->cookie = result->x2;
    } else {
        /* Step 2: Record terminal progress or the newly retained operation. */
        if (!rejected_start)
            fixture->completed += result->x1;

        fixture->pending = status == (int64_t)FIRME_INCOMPLETE ? VAL_TRUE : VAL_FALSE;
        fixture->cookie = fixture->pending ? result->x2 : 0ULL;
        if (fixture->pending && !fixture->continue_advertised) {
            fixture->poisoned = VAL_TRUE;
            LOG_ERROR(NULL, "GPT fixture: INCOMPLETE without advertised OP_CONTINUE; "
                      "retaining lease=%llx cookie=%llx\n",
                      (unsigned long long)fixture->memory.token,
                      (unsigned long long)fixture->cookie);
            return VAL_ERROR;
        }
    }

    /* Step 3: Observe the known prefix and untouched suffix even when BUSY claims no progress. */
    return gpt_fixture_progress_observe(fixture);
}

/** @brief Issue the exact remaining prefix or current retained cookie. */
int val_firme_gpt_fixture_gpi_step(val_firme_gpt_fixture_t *fixture,
                                    firme_res_t *result)
{
    int call_status;

    if (!fixture || !result || !fixture->memory.token || fixture->poisoned ||
        fixture->kind != PAL_FIRME_GPT_MEMORY_TRANSITION ||
        fixture->completed >= fixture->count)
        return VAL_ERROR;
    if (fixture->pending && !fixture->continue_advertised) {
        fixture->poisoned = VAL_TRUE;
        return VAL_ERROR;
    }
    if (fixture->pending)
        call_status = gpt_fixture_call(fixture, FIRME_FID_GM_GPI_OP_CONTINUE,
            fixture->cookie, 0ULL, 0ULL, result);
    else
        call_status = gpt_fixture_call(fixture, FIRME_FID_GM_GPI_SET,
            fixture->memory.range_base +
            fixture->completed * fixture->granule_size,
            fixture->count - fixture->completed,
            FIRME_GPI_ATTR_TARGET(fixture->target_gpi), result);

    if (call_status != VAL_SUCCESS)
        return VAL_ERROR;

    return val_firme_gpt_fixture_gpi_record(fixture, result);
}

/** @brief Bound retries while retaining the real cookie for caller observation. */
pal_firme_gpt_scenario_status_t val_firme_gpt_fixture_retain(
    val_firme_gpt_fixture_t *fixture)
{
    firme_res_t result = {0};
    uint32_t attempt;

    if (!fixture || !fixture->memory.token || fixture->poisoned)
        return PAL_FIRME_GPT_SCENARIO_ERROR;
    for (attempt = 0U; attempt < GPT_FIXTURE_MAX_CALLS; attempt++) {
        if (fixture->pending)
            return PAL_FIRME_GPT_SCENARIO_READY;
        if (fixture->completed == fixture->count)
            return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
        if (val_firme_gpt_fixture_gpi_step(fixture, &result) != VAL_SUCCESS)
            return PAL_FIRME_GPT_SCENARIO_ERROR;
        if (fixture->last_status != (int64_t)FIRME_SUCCESS &&
            fixture->last_status != (int64_t)FIRME_BUSY &&
            fixture->last_status != (int64_t)FIRME_INCOMPLETE)
            return PAL_FIRME_GPT_SCENARIO_ERROR;
    }
    return fixture->pending ? PAL_FIRME_GPT_SCENARIO_READY :
                              PAL_FIRME_GPT_SCENARIO_ERROR;
}

/** @brief Probe an actually retained granule without consuming its cookie. */
int val_firme_gpt_fixture_conflict(val_firme_gpt_fixture_t *fixture)
{
    firme_res_t result = {0};
    uint32_t attempt;
    int64_t status;

    if (!fixture || !fixture->memory.token || fixture->poisoned || !fixture->pending ||
        fixture->kind != PAL_FIRME_GPT_MEMORY_TRANSITION ||
        fixture->completed >= fixture->count)
        return VAL_ERROR;
    for (attempt = 0U; attempt < GPT_FIXTURE_MAX_CALLS; attempt++) {
        if (gpt_fixture_call(fixture, FIRME_FID_GM_GPI_SET,
            fixture->memory.range_base +
            fixture->completed * fixture->granule_size, 1ULL,
            FIRME_GPI_ATTR_TARGET(fixture->target_gpi), &result) != VAL_SUCCESS)
            return VAL_ERROR;

        status = val_firme_status_to_i64(result.x0);
        if (gpt_fixture_progress_observe(fixture) != VAL_SUCCESS)
            return VAL_ERROR;

        if (status == (int64_t)FIRME_BUSY)
            continue;
        if (status == (int64_t)FIRME_OP_CONFLICT && result.x1 == 0ULL)
            return VAL_SUCCESS;
        fixture->poisoned = VAL_TRUE;
        LOG_ERROR(NULL, "GPT fixture: conflicting request x0=%llx count=%llx; "
                  "retaining original cookie=%llx\n", (unsigned long long)result.x0,
                  (unsigned long long)result.x1, (unsigned long long)fixture->cookie);
        return VAL_ERROR;
    }
    return VAL_ERROR;
}

/** @brief Complete progress without treating a valid error row as success. */
static int gpt_fixture_drain(val_firme_gpt_fixture_t *fixture)
{
    firme_res_t result = {0};
    uint32_t attempt;

    for (attempt = 0U; attempt < GPT_FIXTURE_MAX_CALLS; attempt++) {
        if (fixture->completed == fixture->count && !fixture->pending)
            return VAL_SUCCESS;
        if (val_firme_gpt_fixture_gpi_step(fixture, &result) != VAL_SUCCESS)
            return VAL_ERROR;
        if (fixture->last_status != (int64_t)FIRME_SUCCESS &&
            fixture->last_status != (int64_t)FIRME_INCOMPLETE &&
            fixture->last_status != (int64_t)FIRME_BUSY)
            return VAL_ERROR;
    }
    return fixture->completed == fixture->count ? VAL_SUCCESS : VAL_ERROR;
}

/** @brief Preserve the tested transition result separately from restoration. */
int val_firme_gpt_fixture_complete_transition(val_firme_gpt_fixture_t *fixture)
{
    if (!fixture || !fixture->memory.token || fixture->poisoned || fixture->restoring ||
        fixture->kind != PAL_FIRME_GPT_MEMORY_TRANSITION)
        return VAL_ERROR;
    return gpt_fixture_drain(fixture);
}

/** @brief Preserve L1 pending operation identity and correlate returned storage. */
int val_firme_gpt_fixture_l1_record(val_firme_gpt_fixture_t *fixture,
                                     uint32_t destroy, const firme_res_t *result)
{
    int64_t status;

    if (!fixture || !result || destroy > VAL_TRUE ||
        !fixture->memory.token || fixture->poisoned ||
        fixture->kind == PAL_FIRME_GPT_MEMORY_TRANSITION ||
        (fixture->pending && destroy != fixture->l1_destroy))
        return VAL_ERROR;
    status = val_firme_status_to_i64(result->x0);
    fixture->last_status = status;
    fixture->l1_destroy = destroy;
    if (status == (int64_t)FIRME_BUSY)
        return gpt_fixture_l1_observe(fixture);
    if (status == (int64_t)FIRME_INCOMPLETE) {
        fixture->pending = VAL_TRUE;
        return gpt_fixture_l1_observe(fixture);
    }
    if (status != (int64_t)FIRME_SUCCESS ||
        (destroy && result->x2 != fixture->memory.object_base)) {
        fixture->poisoned = VAL_TRUE;
        return VAL_ERROR;
    }
    fixture->pending = VAL_FALSE;
    fixture->l1_present = destroy ? VAL_FALSE : VAL_TRUE;
    return gpt_fixture_l1_observe(fixture);
}

/** @brief Resume CREATE only with object_base zero after real INCOMPLETE. */
int val_firme_gpt_fixture_l1_complete(val_firme_gpt_fixture_t *fixture,
                                       uint32_t destroy)
{
    firme_res_t result = {0};
    uint32_t attempt;
    int call_status;

    if (!fixture || destroy > VAL_TRUE || !fixture->memory.token || fixture->poisoned ||
        fixture->kind == PAL_FIRME_GPT_MEMORY_TRANSITION ||
        (fixture->pending && fixture->l1_destroy != destroy))
        return VAL_ERROR;
    for (attempt = 0U; attempt < GPT_FIXTURE_MAX_CALLS; attempt++) {
        if (destroy)
            call_status = gpt_fixture_call(fixture, FIRME_FID_GM_L1_GPT_DESTROY,
                fixture->memory.range_base, 0ULL, 0ULL, &result);
        else
            call_status = gpt_fixture_call(fixture, FIRME_FID_GM_L1_GPT_CREATE,
                fixture->memory.range_base,
                fixture->pending ? 0ULL : fixture->memory.object_base, 0ULL, &result);

        if (call_status != VAL_SUCCESS)
            return VAL_ERROR;

        if (val_firme_gpt_fixture_l1_record(fixture, destroy, &result) != VAL_SUCCESS)
            return VAL_ERROR;
        if (fixture->last_status == (int64_t)FIRME_SUCCESS)
            return VAL_SUCCESS;
    }
    return VAL_ERROR;
}

/** @brief Restore state before release; uncertainty deliberately retains ownership. */
int val_firme_gpt_fixture_cleanup(val_firme_gpt_fixture_t *fixture)
{
    firme_rule_res_t scrub = {.status = ACS_STATUS_PASS};
    uint64_t index;
    int drain_status;

    if (!fixture)
        return VAL_ERROR;
    if (!fixture->memory.token)
        return VAL_SUCCESS;
    if (fixture->poisoned)
        return VAL_ERROR;
    if (fixture->kind == PAL_FIRME_GPT_MEMORY_TRANSITION) {
        if (fixture->pending) {
            drain_status = gpt_fixture_drain(fixture);
            if (drain_status != VAL_SUCCESS && (fixture->pending || fixture->poisoned))
                return VAL_ERROR;
        }
        if (!fixture->restoring) {
            if (fixture->target_gpi == FIRME_GPI_NSO) {
                for (index = 0ULL; index < fixture->completed; index++) {
                    if (!val_firme_gpt_scrub_ns(fixture->memory.range_base +
                            index * fixture->granule_size, &scrub, index + 1ULL))
                        return VAL_ERROR;
                }
            }
            if (fixture->target_gpi == FIRME_GPI_NONSECURE || !fixture->completed) {
                if (gpt_fixture_observe(fixture, fixture->memory.range_base,
                        fixture->memory.range_size, FIRME_GPI_NONSECURE) != VAL_SUCCESS)
                    return VAL_ERROR;

                fixture->completed = fixture->count;
            } else {
                fixture->count = fixture->completed;
                fixture->completed = 0ULL;
            }

            fixture->target_gpi = FIRME_GPI_NONSECURE;
            fixture->cookie = 0ULL;
            fixture->restoring = VAL_TRUE;
        }
        if (gpt_fixture_drain(fixture) != VAL_SUCCESS)
            return VAL_ERROR;
    } else {
        if (fixture->pending &&
            val_firme_gpt_fixture_l1_complete(fixture, fixture->l1_destroy) != VAL_SUCCESS)
            return VAL_ERROR;
        if (fixture->l1_present &&
            val_firme_gpt_fixture_l1_complete(fixture, VAL_TRUE) != VAL_SUCCESS)
            return VAL_ERROR;
        if (gpt_fixture_par(fixture, fixture->memory.range_base,
                FIRME_GPT_PAR_STATE_HOST_BLOCK) != VAL_SUCCESS ||
            gpt_fixture_observe(fixture, fixture->memory.pool_base,
                fixture->memory.pool_size, FIRME_GPI_NONSECURE) != VAL_SUCCESS)
            return VAL_ERROR;
    }
    if (gpt_fixture_observe(fixture, fixture->memory.range_base,
            fixture->memory.range_size, fixture->memory.initial_gpi) != VAL_SUCCESS ||
        pal_firme_gpt_memory_release(fixture->memory.token) != PAL_FIRME_GPT_SCENARIO_READY)
        return VAL_ERROR;
    *fixture = (val_firme_gpt_fixture_t){0};
    return VAL_SUCCESS;
}
