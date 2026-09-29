/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt_fixture.h"

#include "val_common/val_status.h"

#define GPT_FIXTURE_MAX_CALLS 32U
#define GPT_FIXTURE_GPI_ANY UINT64_C(0xf)

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
static int gpt_fixture_par(const val_firme_gpt_fixture_t *fixture,
                           uint64_t base, uint64_t expected_state)
{
    firme_res_t result = {0};

    val_firme_gm_gpt_info(base, base + fixture->range_size, &result);
    return (val_firme_status_to_i64(result.x0) == (int64_t)FIRME_SUCCESS &&
            result.x1 == base + fixture->range_size &&
            result.x2 == expected_state) ? VAL_SUCCESS : VAL_ERROR;
}

/** @brief Observe every occupied granule without conflating object bytes and ownership. */
static int gpt_fixture_l1_observe(const val_firme_gpt_fixture_t *fixture)
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
pal_firme_gpt_scenario_status_t val_firme_gpt_fixture_begin(
    val_firme_gpt_fixture_t *fixture, firme_acs_world_t world,
    pal_firme_gpt_memory_kind_t kind)
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
    if ((kind != PAL_FIRME_GPT_MEMORY_TRANSITION && world != FIRME_ACS_WORLD_REALM) ||
        (kind == PAL_FIRME_GPT_MEMORY_TRANSITION && world == FIRME_ACS_WORLD_NSEL2))
        return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
    required_bits = FIRME_GM_FEATURE_REG0_GPI_SET_BIT |
                    FIRME_GM_FEATURE_REG0_OP_CONTINUE_BIT;
    if (kind != PAL_FIRME_GPT_MEMORY_TRANSITION)
        required_bits = FIRME_GM_FEATURE_REG0_L1_CREATE_BIT |
                        FIRME_GM_FEATURE_REG0_L1_DESTROY_BIT |
                        FIRME_GM_FEATURE_REG0_GPT_INFO_BIT;
    val_firme_service_features(FIRME_SERVICE_ID_GRANULE_MGMT,
                               FIRME_GM_FEATURE_REG0_INDEX, &result);
    if (val_firme_status_to_i64(result.x0) != (int64_t)FIRME_SUCCESS)
        return PAL_FIRME_GPT_SCENARIO_ERROR;
    if ((result.x1 & required_bits) != required_bits)
        return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
    val_firme_service_features(FIRME_SERVICE_ID_GRANULE_MGMT,
                               FIRME_GM_FEATURE_REG1_INDEX, &result);
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
                          FIRME_GPI_REALM : FIRME_GPI_SECURE;
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

/** @brief Record only architecturally valid progress and retained cookies. */
int val_firme_gpt_fixture_gpi_record(val_firme_gpt_fixture_t *fixture,
                                      const firme_res_t *result)
{
    uint64_t remaining;
    int64_t status;

    if (!fixture || !result || !fixture->memory.token || fixture->poisoned ||
        fixture->kind != PAL_FIRME_GPT_MEMORY_TRANSITION ||
        fixture->completed >= fixture->count)
        return VAL_ERROR;
    remaining = fixture->count - fixture->completed;
    status = val_firme_status_to_i64(result->x0);
    fixture->last_status = status;
    if (val_firme_gpt_operation_count_valid(status, result->x1, remaining) != VAL_TRUE) {
        fixture->poisoned = VAL_TRUE;
        return VAL_ERROR;
    }
    if (status == (int64_t)FIRME_BUSY) {
        if (fixture->pending)
            fixture->cookie = result->x2;
        return VAL_SUCCESS;
    }
    fixture->completed += result->x1;
    fixture->pending = status == (int64_t)FIRME_INCOMPLETE ? VAL_TRUE : VAL_FALSE;
    fixture->cookie = fixture->pending ? result->x2 : 0ULL;
    if ((fixture->completed && gpt_fixture_observe(fixture,
            fixture->memory.range_base, fixture->completed * fixture->granule_size,
            fixture->target_gpi) != VAL_SUCCESS) ||
        (!fixture->restoring && fixture->completed < fixture->count && gpt_fixture_observe(fixture,
            fixture->memory.range_base + fixture->completed * fixture->granule_size,
            (fixture->count - fixture->completed) * fixture->granule_size,
            FIRME_GPI_NONSECURE) != VAL_SUCCESS))
        return VAL_ERROR;
    return VAL_SUCCESS;
}

/** @brief Issue the exact remaining prefix or current retained cookie. */
int val_firme_gpt_fixture_gpi_step(val_firme_gpt_fixture_t *fixture,
                                    firme_res_t *result)
{
    if (!fixture || !result || !fixture->memory.token || fixture->poisoned ||
        fixture->kind != PAL_FIRME_GPT_MEMORY_TRANSITION ||
        fixture->completed >= fixture->count)
        return VAL_ERROR;
    if (fixture->pending)
        val_firme_gm_gpi_op_continue(fixture->cookie, result);
    else
        val_firme_gm_gpi_set(fixture->memory.range_base +
            fixture->completed * fixture->granule_size,
            fixture->count - fixture->completed,
            FIRME_GPI_ATTR_TARGET(fixture->target_gpi), result);
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
        val_firme_gm_gpi_set(fixture->memory.range_base +
            fixture->completed * fixture->granule_size, 1ULL,
            FIRME_GPI_ATTR_TARGET(fixture->target_gpi), &result);
        status = val_firme_status_to_i64(result.x0);
        if (status == (int64_t)FIRME_BUSY)
            continue;
        if (status == (int64_t)FIRME_OP_CONFLICT && result.x1 == 0ULL)
            return VAL_SUCCESS;
        fixture->poisoned = VAL_TRUE;
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

    if (!fixture || destroy > VAL_TRUE || !fixture->memory.token || fixture->poisoned ||
        fixture->kind == PAL_FIRME_GPT_MEMORY_TRANSITION ||
        (fixture->pending && fixture->l1_destroy != destroy))
        return VAL_ERROR;
    for (attempt = 0U; attempt < GPT_FIXTURE_MAX_CALLS; attempt++) {
        if (destroy)
            val_firme_gm_l1_gpt_destroy(fixture->memory.range_base, &result);
        else
            val_firme_gm_l1_gpt_create(fixture->memory.range_base,
                fixture->pending ? 0ULL : fixture->memory.object_base, &result);
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
            fixture->target_gpi = FIRME_GPI_NONSECURE;
            fixture->completed = 0ULL;
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
