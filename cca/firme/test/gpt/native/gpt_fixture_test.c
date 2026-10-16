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
#include "../../../val/src/val_firme_gpt_fixture.c"
#include "../../../val/src/val_firme_gpt_abi_adapter.c"
#include "../../../val/src/val_firme_gpt_l1.c"
#include "../../../platform/pal_baremetal/targets/fvp/src/pal_firme_gpt.c"

#define NATIVE_BASE UINT64_C(0x40000000)
#define NATIVE_POOL UINT64_C(0x80000000)
#define NATIVE_COUNT 4U
#define NATIVE_CALLS 128U

typedef enum { NATIVE_SET, NATIVE_CONTINUE, NATIVE_CREATE, NATIVE_DESTROY } native_abi_t;
typedef struct {
    native_abi_t abi;
    uint64_t argument;
    uint64_t count;
    firme_res_t result;
} native_call_t;

static native_call_t native_calls[NATIVE_CALLS];
static uint32_t native_queued;
static uint32_t native_issued;
static uint64_t native_feature;
static uint64_t native_abis;
static uint64_t native_pgs;
static pal_firme_gpt_memory_t native_memory;
static pal_firme_gpt_memory_kind_t native_kind;
static uint64_t native_gpis[NATIVE_COUNT];
static uint64_t native_object_gpi;
static uint64_t native_par;
static uint64_t native_progress;
static uint64_t native_target;
static uint32_t native_acquires;
static uint32_t native_releases;
static uint32_t native_short_pool;
static uint32_t native_short_observation;
static uint32_t native_observer_error;
static uint32_t native_release_error;
static uint32_t native_lease_live;
static uint32_t native_create_mismatch;
static uint32_t native_gpc2 = VAL_TRUE;
static uint32_t native_scrub_failure;
static uint64_t native_scrub_writes;
static uint64_t native_scrub_cleans;
static uint32_t native_continue_corruption;
static uint32_t native_conflict_corruption;

typedef struct {
    firme_acs_world_t world;
    uint32_t calls;
    uint32_t fail_next;
    uint32_t issued_on_failure;
    uint64_t fid_mask;
} native_frame_route_t;

/** @brief Model a caller-specific transport without silently invoking setup in Host. */
static int native_frame_invoke(const smccc_call_frame_t *input,
    smccc_call_frame_t *output, uint32_t *issued, void *cookie)
{
    native_frame_route_t *route = cookie;
    val_firme_abi_test_context_t local_context = {0};
    uint32_t register_index;

    assert(route && route->world == FIRME_ACS_WORLD_REALM);
    route->calls++;
    assert(input->registers[0] >= FIRME_FID_SERVICE_VERSION &&
           input->registers[0] <= FIRME_FID_GM_GPT_INFO);
    route->fid_mask |= UINT64_C(1) << (input->registers[0] - FIRME_FID_SERVICE_VERSION);
    for (register_index = 4U; register_index < VAL_SMCCC_REGISTER_COUNT; register_index++)
        assert(input->registers[register_index] == 0ULL);

    if (route->fail_next) {
        route->fail_next = VAL_FALSE;
        *issued = route->issued_on_failure;
        return VAL_ERROR;
    }

    return val_firme_abi_call(&local_context, input, output, issued);
}

/** @brief Reset the native model, including quarantines as on platform reset. */
static void native_reset(void)
{
    uint32_t index;

    memset(native_calls, 0, sizeof(native_calls));
    memset(g_gpt_adapter_fixture, 0, sizeof(g_gpt_adapter_fixture));
    memset(g_gpt_adapter_lock, 0, sizeof(g_gpt_adapter_lock));
    g_l1_fixture = (val_firme_gpt_fixture_t){0};
    g_l1_fixture_lock = VAL_FALSE;
    native_queued = native_issued = 0U;
    native_acquires = native_releases = 0U;
    native_short_pool = native_short_observation = VAL_FALSE;
    native_observer_error = native_release_error = VAL_FALSE;
    native_lease_live = VAL_FALSE;
    native_create_mismatch = VAL_FALSE;
    native_gpc2 = VAL_TRUE;
    native_scrub_failure = VAL_FALSE;
    native_scrub_writes = native_scrub_cleans = 0ULL;
    native_continue_corruption = native_conflict_corruption = VAL_FALSE;
    native_abis = FIRME_GM_FEATURE_REG0_ALLOCATED_BITS;
    native_feature = FIRME_GM_PGS_64KB;
    native_pgs = UINT64_C(0x10000);
    native_object_gpi = FIRME_GPI_NONSECURE;
    native_par = FIRME_GPT_PAR_STATE_HOST_BLOCK;
    native_progress = 0ULL;
    native_memory = (pal_firme_gpt_memory_t){0};
    for (index = 0U; index < NATIVE_COUNT; index++)
        native_gpis[index] = FIRME_GPI_NONSECURE;
}

/** @brief Queue real-response-shaped registers, including poisoned unused bits. */
static void native_reply(native_abi_t abi, int64_t status, uint64_t count, uint64_t cookie)
{
    assert(native_queued < NATIVE_CALLS);
    native_calls[native_queued++] = (native_call_t){
        .abi = abi,
        .result = {.x0 = UINT64_C(0xa5a5a5a500000000) | (uint32_t)status,
                   .x1 = count, .x2 = cookie, .x3 = UINT64_MAX},
    };
}

/** @brief Record the actual issued ABI and consume its scripted response. */
static void native_invoke(native_abi_t abi, uint64_t argument, uint64_t count,
                           firme_res_t *result)
{
    assert(native_issued < native_queued);
    assert(native_calls[native_issued].abi == abi);
    native_calls[native_issued].argument = argument;
    native_calls[native_issued].count = count;
    *result = native_calls[native_issued++].result;
}

/** @brief Decode the architected signed W0 result, not arbitrary upper X0 bits. */
int64_t val_firme_status_to_i64(uint64_t value)
{
    return (int64_t)(int32_t)(uint32_t)value;
}

/** @brief Suppress native firmware logging. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *format, ...)
{
    return VAL_SUCCESS;
}

/** @brief Model the PE feature needed for an observable Normal-world transition. */
uint32_t val_firme_feat_rme_gpc2_present(void)
{
    return native_gpc2;
}

/** @brief Count real scrub-helper writes without dereferencing synthetic native addresses. */
int val_probe_write64(uint64_t address, uint64_t value, val_fault_info_t *fault)
{
    assert(address >= NATIVE_BASE && address < NATIVE_BASE + NATIVE_COUNT * native_pgs);
    assert(value == 0ULL && fault);
    *fault = (val_fault_info_t){0};
    if (native_scrub_failure) {
        fault->faulted = VAL_TRUE;
        return VAL_ERROR;
    }
    native_scrub_writes++;
    return VAL_SUCCESS;
}

/** @brief Record that each complete granule is cleaned before returning NS ownership. */
void val_cache_clean_poc(uintptr_t address, size_t size)
{
    assert(address >= NATIVE_BASE && size == native_pgs);
    native_scrub_cleans++;
}

/** @brief Advertise independently configurable ABI presence and geometry. */
void val_firme_service_features(uint64_t service, uint64_t index, firme_res_t *result)
{
    assert(service == FIRME_SERVICE_ID_GRANULE_MGMT);
    *result = (firme_res_t){.x0 = FIRME_SUCCESS,
        .x1 = index == FIRME_GM_FEATURE_REG0_INDEX ? native_abis : native_feature};
}

/** @brief Reflect progress in a separate native GPI map, never in observer results. */
static void native_apply(const firme_res_t *result)
{
    uint64_t index;
    int64_t status = val_firme_status_to_i64(result->x0);

    if (status == FIRME_BUSY || !val_firme_gpt_operation_status_valid(status) ||
        result->x1 > NATIVE_COUNT - native_progress)
        return;
    for (index = 0ULL; index < result->x1; index++)
        native_gpis[native_progress + index] = native_target;
    native_progress += result->x1;
}

/** @brief Model an actual GPI_SET invocation and its successful prefix. */
void val_firme_gm_gpi_set(uint64_t base, uint64_t count, uint64_t attributes,
                          firme_res_t *result)
{
    native_invoke(NATIVE_SET, base, count, result);
    if (val_firme_status_to_i64(result->x0) == FIRME_OP_CONFLICT && result->x1 == 0ULL) {
        if (native_conflict_corruption)
            native_gpis[(base - NATIVE_BASE) / native_pgs] = native_target;

        return;
    }
    if (val_firme_status_to_i64(result->x0) == FIRME_BUSY)
        return;
    native_progress = (base - NATIVE_BASE) / native_pgs;
    native_target = attributes;
    native_apply(result);
}

/** @brief Model continuation without replacing its original target or range. */
void val_firme_gm_gpi_op_continue(uint64_t cookie, firme_res_t *result)
{
    native_invoke(NATIVE_CONTINUE, cookie, 0ULL, result);
    native_apply(result);
    if (native_continue_corruption && native_progress > 1ULL &&
        native_progress < NATIVE_COUNT) {
        native_gpis[native_progress - 1ULL] = FIRME_GPI_NONSECURE;
        native_gpis[native_progress] = native_target;
    }
}

/** @brief Model creation including whole-granule Root ownership and table state. */
void val_firme_gm_l1_gpt_create(uint64_t base, uint64_t object, firme_res_t *result)
{
    int64_t status;

    native_invoke(NATIVE_CREATE, object, base, result);
    status = val_firme_status_to_i64(result->x0);
    if (status == FIRME_SUCCESS || status == FIRME_INCOMPLETE) {
        native_object_gpi = native_create_mismatch ? FIRME_GPI_NONSECURE : FIRME_GPI_ROOT;
        native_par = FIRME_GPT_PAR_STATE_HOST_TABLE;
    }
}

/** @brief Model destruction returning the exact object base in X2 only. */
void val_firme_gm_l1_gpt_destroy(uint64_t base, firme_res_t *result)
{
    native_invoke(NATIVE_DESTROY, base, 0ULL, result);
    if (val_firme_status_to_i64(result->x0) == FIRME_SUCCESS) {
        native_object_gpi = FIRME_GPI_NONSECURE;
        native_par = FIRME_GPT_PAR_STATE_HOST_BLOCK;
    }
}

/** @brief Model actual GPT_INFO state independently from queued ABI outputs. */
void val_firme_gm_gpt_info(uint64_t base, uint64_t top, firme_res_t *result)
{
    *result = (firme_res_t){.x0 = FIRME_SUCCESS, .x1 = top, .x2 = native_par};
}

/** @brief Reserve a native disposable extent including a sub-granule object. */
static pal_firme_gpt_scenario_status_t native_acquire(
    uint32_t world, pal_firme_gpt_memory_kind_t kind, uint64_t pgs,
    uint64_t range_size, uint64_t object_size, pal_firme_gpt_memory_t *memory)
{
    uint64_t pool_size;

    assert(!native_lease_live);
    native_acquires++;
    native_lease_live = VAL_TRUE;
    native_kind = kind;
    native_pgs = pgs;
    pool_size = object_size > pgs ? object_size : pgs;
    native_memory = (pal_firme_gpt_memory_t){
        .range_base = NATIVE_BASE,
        .range_size = kind == PAL_FIRME_GPT_MEMORY_TRANSITION ? NATIVE_COUNT * pgs : range_size,
        .initial_gpi = kind == PAL_FIRME_GPT_MEMORY_L1_NON_SELF ?
                       GPT_FIXTURE_GPI_ANY : FIRME_GPI_NONSECURE,
        .pool_base = kind == PAL_FIRME_GPT_MEMORY_L1_SELF ? NATIVE_BASE : NATIVE_POOL,
        .pool_size = native_short_pool ? object_size : pool_size,
        .token = 1ULL,
    };
    native_memory.object_base = native_memory.pool_base +
                               (object_size < pgs ? object_size : 0ULL);
    *memory = native_memory;
    return PAL_FIRME_GPT_SCENARIO_READY;
}

/** @brief Observe each granule from model state, with configurable observer faults. */
static pal_firme_gpt_scenario_status_t native_observe(
    uint64_t base, uint64_t size, uint64_t pgs, uint32_t mask, uint64_t *checked)
{
    uint64_t address;
    uint64_t gpi;

    *checked = 0ULL;
    if (native_observer_error)
        return PAL_FIRME_GPT_SCENARIO_ERROR;
    for (address = base; address < base + size; address += pgs) {
        if (native_kind == PAL_FIRME_GPT_MEMORY_TRANSITION) {
            assert(address >= NATIVE_BASE && address < NATIVE_BASE + NATIVE_COUNT * pgs);
            gpi = native_gpis[(address - NATIVE_BASE) / pgs];
        } else if (address >= native_memory.pool_base &&
                   address < native_memory.pool_base + native_memory.pool_size) {
            gpi = native_object_gpi;
        } else {
            gpi = native_memory.initial_gpi;
        }
        if (!(mask & (UINT32_C(1) << gpi)))
            return PAL_FIRME_GPT_SCENARIO_MISMATCH;
        (*checked)++;
    }
    if (native_short_observation && *checked)
        (*checked)--;
    return PAL_FIRME_GPT_SCENARIO_READY;
}

/** @brief Return storage only after independent state is restored. */
static pal_firme_gpt_scenario_status_t native_release(uint64_t token)
{
    uint32_t index;

    assert(token == native_memory.token && native_lease_live);
    if (native_release_error)
        return PAL_FIRME_GPT_SCENARIO_ERROR;
    for (index = 0U; index < NATIVE_COUNT; index++)
        assert(native_gpis[index] == FIRME_GPI_NONSECURE);
    assert(native_object_gpi == FIRME_GPI_NONSECURE);
    assert(native_par == FIRME_GPT_PAR_STATE_HOST_BLOCK);
    native_lease_live = VAL_FALSE;
    native_releases++;
    return PAL_FIRME_GPT_SCENARIO_READY;
}

static const pal_firme_gpt_memory_ops_t native_memory_ops = {
    .acquire = native_acquire, .observe = native_observe, .release = native_release,
};

/** @brief Exercise BUSY count/cookie rules, valid-prefix failure, and restoration. */
static void native_progress_test(void)
{
    val_firme_gpt_fixture_t fixture = {0};
    firme_res_t result;

    native_reset();
    assert(val_firme_gpt_fixture_begin(&fixture, FIRME_ACS_WORLD_REALM,
        PAL_FIRME_GPT_MEMORY_TRANSITION) == PAL_FIRME_GPT_SCENARIO_READY);
    native_reply(NATIVE_SET, FIRME_BUSY, UINT64_MAX, UINT64_MAX);
    native_reply(NATIVE_SET, FIRME_INCOMPLETE, 1ULL, 11ULL);
    assert(val_firme_gpt_fixture_retain(&fixture) == PAL_FIRME_GPT_SCENARIO_READY);
    assert(fixture.completed == 1ULL && fixture.cookie == 11ULL);
    assert(native_calls[0].argument == native_calls[1].argument);
    native_reply(NATIVE_CONTINUE, FIRME_BUSY, UINT64_MAX, 12ULL);
    assert(val_firme_gpt_fixture_gpi_step(&fixture, &result) == VAL_SUCCESS);
    assert(fixture.completed == 1ULL && fixture.cookie == 12ULL);
    native_reply(NATIVE_SET, FIRME_BUSY, UINT64_MAX, UINT64_MAX);
    native_reply(NATIVE_SET, FIRME_OP_CONFLICT, 0ULL, UINT64_MAX);
    assert(val_firme_gpt_fixture_conflict(&fixture) == VAL_SUCCESS);
    assert(fixture.cookie == 12ULL && fixture.pending);
    assert(native_calls[3].argument == NATIVE_BASE + native_pgs);
    native_reply(NATIVE_CONTINUE, FIRME_DENIED, 1ULL, UINT64_MAX);
    assert(val_firme_gpt_fixture_gpi_step(&fixture, &result) == VAL_SUCCESS);
    assert(fixture.completed == 2ULL && !fixture.pending && !fixture.cookie);
    native_reply(NATIVE_SET, FIRME_SUCCESS, 2ULL, UINT64_MAX);
    assert(val_firme_gpt_fixture_cleanup(&fixture) == VAL_SUCCESS);
    assert(native_calls[3].argument == native_calls[4].argument);
    assert(native_calls[5].argument == 12ULL && native_releases == 1U);
    assert(!fixture.memory.token && native_issued == native_queued);
}

/** @brief Observe unchanged BUSY state independently and quarantine uncertain leases. */
static void native_busy_observation_test(void)
{
    static const struct {
        uint32_t pending;
        uint32_t corrupt_index;
        uint32_t short_observation;
        uint32_t observer_error;
    } cases[] = {
        {VAL_FALSE, 0U, VAL_FALSE, VAL_FALSE},
        {VAL_TRUE, 0U, VAL_FALSE, VAL_FALSE},
        {VAL_TRUE, 1U, VAL_FALSE, VAL_FALSE},
        {VAL_FALSE, UINT32_MAX, VAL_TRUE, VAL_FALSE},
        {VAL_TRUE, UINT32_MAX, VAL_TRUE, VAL_FALSE},
        {VAL_FALSE, UINT32_MAX, VAL_FALSE, VAL_TRUE},
        {VAL_TRUE, UINT32_MAX, VAL_FALSE, VAL_TRUE},
    };
    val_firme_gpt_fixture_t fixture = {0};
    firme_res_t result;
    size_t case_index;
    uint32_t issued;
    uint32_t is_pending;
    uint64_t corrupt_gpi;

    /* Step 1: A stateless BUSY must ignore unknown count and cookie outputs. */
    native_reset();
    assert(val_firme_gpt_fixture_begin(&fixture, FIRME_ACS_WORLD_REALM,
        PAL_FIRME_GPT_MEMORY_TRANSITION) == PAL_FIRME_GPT_SCENARIO_READY);
    native_reply(NATIVE_SET, FIRME_BUSY, UINT64_MAX, UINT64_MAX);
    assert(val_firme_gpt_fixture_gpi_step(&fixture, &result) == VAL_SUCCESS);
    assert(fixture.completed == 0ULL && !fixture.pending && fixture.cookie == 0ULL);
    assert(val_firme_gpt_fixture_cleanup(&fixture) == VAL_SUCCESS);
    assert(native_releases == 1U && native_issued == 1U);

    /* Step 2: Inject independent prefix/suffix corruption or incomplete observation. */
    for (case_index = 0U; case_index < sizeof(cases) / sizeof(cases[0]); case_index++) {
        is_pending = cases[case_index].pending;
        native_reset();
        fixture = (val_firme_gpt_fixture_t){0};
        assert(val_firme_gpt_fixture_begin(&fixture, FIRME_ACS_WORLD_REALM,
            PAL_FIRME_GPT_MEMORY_TRANSITION) == PAL_FIRME_GPT_SCENARIO_READY);
        if (is_pending) {
            native_reply(NATIVE_SET, FIRME_INCOMPLETE, 1ULL, 11ULL);
            assert(val_firme_gpt_fixture_retain(&fixture) == PAL_FIRME_GPT_SCENARIO_READY);
        }
        if (cases[case_index].corrupt_index != UINT32_MAX) {
            corrupt_gpi = fixture.target_gpi;
            if (is_pending && cases[case_index].corrupt_index == 0U)
                corrupt_gpi = FIRME_GPI_NONSECURE;
            native_gpis[cases[case_index].corrupt_index] = corrupt_gpi;
        }
        native_short_observation = cases[case_index].short_observation;
        native_observer_error = cases[case_index].observer_error;
        native_reply(is_pending ? NATIVE_CONTINUE : NATIVE_SET,
            FIRME_BUSY, UINT64_MAX, 12ULL);
        assert(val_firme_gpt_fixture_gpi_step(&fixture, &result) == VAL_ERROR);
        assert(fixture.completed == (is_pending ? 1ULL : 0ULL));
        assert(fixture.pending == cases[case_index].pending);
        assert(fixture.cookie == (is_pending ? 12ULL : 0ULL));

        /* Step 3: Preserve the lease and latest cookie without attempting rollback. */
        issued = native_issued;
        assert(fixture.poisoned && fixture.memory.token);
        assert(val_firme_gpt_fixture_cleanup(&fixture) == VAL_ERROR);
        assert(native_lease_live && !native_releases && native_issued == issued);
        assert(native_issued == native_queued);
    }
}

/** @brief Restore a retained request across partial SUCCESS and a stateful rollback. */
static void native_cleanup_test(void)
{
    val_firme_gpt_fixture_t fixture = {0};

    native_reset();
    assert(val_firme_gpt_fixture_begin(&fixture, FIRME_ACS_WORLD_SECURE,
        PAL_FIRME_GPT_MEMORY_TRANSITION) == PAL_FIRME_GPT_SCENARIO_READY);
    native_reply(NATIVE_SET, FIRME_INCOMPLETE, 1ULL, 21ULL);
    assert(val_firme_gpt_fixture_retain(&fixture) == PAL_FIRME_GPT_SCENARIO_READY);
    native_reply(NATIVE_CONTINUE, FIRME_SUCCESS, 1ULL, UINT64_MAX);
    native_reply(NATIVE_SET, FIRME_SUCCESS, 2ULL, UINT64_MAX);
    native_reply(NATIVE_SET, FIRME_INCOMPLETE, 2ULL, 22ULL);
    native_reply(NATIVE_CONTINUE, FIRME_BUSY, UINT64_MAX, 23ULL);
    native_reply(NATIVE_CONTINUE, FIRME_SUCCESS, 2ULL, UINT64_MAX);
    assert(val_firme_gpt_fixture_cleanup(&fixture) == VAL_SUCCESS);
    assert(native_calls[2].argument == NATIVE_BASE + 2ULL * native_pgs);
    assert(native_calls[2].count == 2ULL);
    assert(native_calls[4].argument == 22ULL && native_calls[5].argument == 23ULL);
    assert(native_releases == 1U && native_issued == native_queued);
}

/** @brief Reject malformed progress and retain reservations on cleanup failures. */
static void native_quarantine_test(void)
{
    val_firme_gpt_fixture_t fixture = {0};
    firme_res_t result;
    uint32_t attempt;

    native_reset();
    assert(val_firme_gpt_fixture_begin(&fixture, FIRME_ACS_WORLD_REALM,
        PAL_FIRME_GPT_MEMORY_TRANSITION) == PAL_FIRME_GPT_SCENARIO_READY);
    native_reply(NATIVE_SET, FIRME_INCOMPLETE, NATIVE_COUNT, 31ULL);
    assert(val_firme_gpt_fixture_gpi_step(&fixture, &result) == VAL_ERROR);
    assert(val_firme_gpt_fixture_cleanup(&fixture) == VAL_ERROR);
    assert(fixture.memory.token && native_releases == 0U && native_issued == 1U);

    native_reset();
    fixture = (val_firme_gpt_fixture_t){0};
    assert(val_firme_gpt_fixture_begin(&fixture, FIRME_ACS_WORLD_REALM,
        PAL_FIRME_GPT_MEMORY_TRANSITION) == PAL_FIRME_GPT_SCENARIO_READY);
    native_release_error = VAL_TRUE;
    assert(val_firme_gpt_fixture_cleanup(&fixture) == VAL_ERROR);
    assert(fixture.memory.token && native_releases == 0U);
    native_release_error = VAL_FALSE;
    assert(val_firme_gpt_fixture_cleanup(&fixture) == VAL_SUCCESS);
    assert(native_issued == 0U && native_releases == 1U);

    native_reset();
    assert(val_firme_gpt_fixture_begin(&fixture, FIRME_ACS_WORLD_REALM,
        PAL_FIRME_GPT_MEMORY_TRANSITION) == PAL_FIRME_GPT_SCENARIO_READY);
    native_reply(NATIVE_SET, FIRME_INCOMPLETE, 1ULL, 31ULL);
    assert(val_firme_gpt_fixture_retain(&fixture) == PAL_FIRME_GPT_SCENARIO_READY);
    for (attempt = 0U; attempt < GPT_FIXTURE_MAX_CALLS; attempt++)
        native_reply(NATIVE_CONTINUE, FIRME_BUSY, UINT64_MAX, 32ULL + attempt);
    assert(val_firme_gpt_fixture_cleanup(&fixture) == VAL_ERROR);
    assert(fixture.pending && fixture.memory.token && !fixture.poisoned);
    assert(fixture.cookie == 31ULL + GPT_FIXTURE_MAX_CALLS && !native_releases);
    native_reply(NATIVE_CONTINUE, FIRME_SUCCESS, 3ULL, UINT64_MAX);
    native_reply(NATIVE_SET, FIRME_SUCCESS, NATIVE_COUNT, UINT64_MAX);
    assert(val_firme_gpt_fixture_cleanup(&fixture) == VAL_SUCCESS);
    assert(native_releases == 1U && native_issued == native_queued);
}

/** @brief Check full-granule envelopes, exclusions, containment, and lifecycle. */
static void native_l1_test(void)
{
    val_firme_gpt_fixture_t fixture = {0};
    pal_firme_gpt_l1_scenario_t scenario = {0};
    firme_rule_res_t out = {0};
    uint64_t base;
    uint64_t size;
    uint32_t kind;

    native_reset();
    assert(pal_firme_gpt_granule_envelope(NATIVE_POOL + 0x2000ULL, 0x2000ULL,
        0x10000ULL, &base, &size) == PAL_FIRME_GPT_SCENARIO_READY);
    assert(base == NATIVE_POOL && size == 0x10000ULL);
    assert(pal_firme_gpt_granule_envelope(UINT64_MAX - 1ULL, 1ULL,
        0x10000ULL, &base, &size) == PAL_FIRME_GPT_SCENARIO_ERROR);
    assert(pal_firme_gpt_granule_envelope(NATIVE_POOL, 0ULL,
        0x10000ULL, &base, &size) == PAL_FIRME_GPT_SCENARIO_ERROR);
    assert(pal_firme_gpt_granule_envelope(NATIVE_POOL, 0x2000ULL,
        3ULL, &base, &size) == PAL_FIRME_GPT_SCENARIO_ERROR);
    for (kind = PAL_FIRME_GPT_MEMORY_L1_SELF; kind <= PAL_FIRME_GPT_MEMORY_L1_NON_SELF; kind++) {
        native_reset();
        assert(val_firme_gpt_fixture_begin(&fixture, FIRME_ACS_WORLD_REALM,
            (pal_firme_gpt_memory_kind_t)kind) == PAL_FIRME_GPT_SCENARIO_READY);
        assert(fixture.object_size == 0x2000ULL && fixture.object_granule_size == 0x10000ULL);
        scenario = (pal_firme_gpt_l1_scenario_t){
            .range_base = fixture.memory.range_base, .range_size = fixture.range_size,
            .object_base = fixture.memory.object_base, .object_size = fixture.object_size,
            .granule_size = fixture.granule_size,
            .range_initial_gpi = fixture.memory.initial_gpi,
            .object_initial_gpi = FIRME_GPI_NONSECURE,
            .layout = kind == PAL_FIRME_GPT_MEMORY_L1_SELF ?
                PAL_FIRME_GPT_L1_LAYOUT_SELF_DESCRIBING :
                PAL_FIRME_GPT_L1_LAYOUT_NON_SELF_DESCRIBING,
        };
        assert(val_firme_gpt_l1_validate_scenario(FIRME_RULE_R0170,
            PAL_FIRME_GPT_L1_CONDITION_VALID_SELF, &scenario, &out) == VAL_FIRME_GPT_L1_FLOW_PASS);
        assert(val_firme_gpt_l1_check_object_segment(&scenario, FIRME_RULE_R0173,
            0ULL, 1ULL, FIRME_GPI_NONSECURE, 1ULL, &out) == VAL_FIRME_GPT_L1_FLOW_PASS);
        assert(val_firme_gpt_l1_check_object_segment(&scenario, FIRME_RULE_R0173,
            1ULL, 1ULL, FIRME_GPI_NONSECURE, 1ULL, &out) == VAL_FIRME_GPT_L1_FLOW_ERROR);
        native_reply(NATIVE_CREATE, FIRME_BUSY, UINT64_MAX, UINT64_MAX);
        native_reply(NATIVE_CREATE, FIRME_INCOMPLETE, UINT64_MAX, UINT64_MAX);
        native_reply(NATIVE_CREATE, FIRME_BUSY, UINT64_MAX, UINT64_MAX);
        native_reply(NATIVE_CREATE, FIRME_SUCCESS, UINT64_MAX, UINT64_MAX);
        assert(val_firme_gpt_fixture_l1_complete(&fixture, VAL_FALSE) == VAL_SUCCESS);
        assert(native_calls[0].argument == fixture.memory.object_base);
        assert(native_calls[1].argument == fixture.memory.object_base);
        assert(!native_calls[2].argument && !native_calls[3].argument);
        assert(val_firme_gpt_l1_check_gpis(FIRME_RULE_R0170, &scenario,
            scenario.object_base, scenario.object_size, 0ULL, 0ULL,
            UINT32_C(1) << FIRME_GPI_ROOT, 1ULL, &out) == VAL_FIRME_GPT_L1_FLOW_PASS);
        assert(val_firme_gpt_l1_check_range_unchanged(FIRME_RULE_R0168,
            &scenario, 1ULL, &out) == VAL_FIRME_GPT_L1_FLOW_PASS);
        native_reply(NATIVE_DESTROY, FIRME_INCOMPLETE, UINT64_MAX, UINT64_MAX);
        native_reply(NATIVE_DESTROY, FIRME_SUCCESS, UINT64_MAX, fixture.memory.object_base);
        assert(val_firme_gpt_fixture_cleanup(&fixture) == VAL_SUCCESS);
        assert(native_releases == 1U && native_issued == native_queued);
    }
    native_reset();
    native_short_pool = VAL_TRUE;
    assert(val_firme_gpt_fixture_begin(&fixture, FIRME_ACS_WORLD_REALM,
        PAL_FIRME_GPT_MEMORY_L1_NON_SELF) == PAL_FIRME_GPT_SCENARIO_ERROR);
    assert(val_firme_gpt_fixture_cleanup(&fixture) == VAL_ERROR);
    assert(!native_issued && !native_releases);
}

/** @brief Exercise generated continuation setup, failed output cleanup, and L1 correlation. */
static void native_adapter_test(void)
{
    val_firme_abi_descriptor_t abi = {.id = VAL_FIRME_ABI_ID_GM_GPI_OP_CONTINUE};
    val_firme_abi_case_t test_case = {.mutation = VAL_FIRME_MUTATION_NONE};
    val_firme_abi_test_context_t context = {
        .abi = &abi, .test_case = &test_case, .world = FIRME_ACS_WORLD_REALM,
    };
    const val_firme_abi_test_ops_t *ops = val_firme_gpt_abi_adapter_get(abi.id);
    firme_res_t result;

    native_reset();
    native_reply(NATIVE_SET, FIRME_INCOMPLETE, 1ULL, 41ULL);
    assert(ops && ops->prepare_state(&context) == VAL_SUCCESS);
    assert(ops->build_valid_input(&context) == VAL_SUCCESS);
    assert(context.input.registers[1] == 41ULL);
    native_reply(NATIVE_CONTINUE, FIRME_DENIED, 1ULL, UINT64_MAX);
    context.call_issued = VAL_TRUE;
    val_firme_gm_gpi_op_continue(41ULL, &result);
    context.output.registers[0] = result.x0;
    context.output.registers[1] = result.x1;
    context.output.registers[2] = result.x2;
    assert(ops->validate_output(&context) == VAL_FALSE);
    native_reply(NATIVE_SET, FIRME_SUCCESS, 2ULL, UINT64_MAX);
    assert(ops->cleanup(&context) == VAL_SUCCESS && native_releases == 1U);

    native_reset();
    memset(context.scratch, 0, sizeof(context.scratch));
    context.call_issued = VAL_FALSE;
    abi.id = VAL_FIRME_ABI_ID_GM_GPI_SET;
    assert(ops->prepare_state(&context) == VAL_SUCCESS);
    assert(ops->build_valid_input(&context) == VAL_SUCCESS);
    native_reply(NATIVE_SET, FIRME_INCOMPLETE, 1ULL, 41ULL);
    context.call_issued = VAL_TRUE;
    val_firme_gm_gpi_set(NATIVE_BASE, NATIVE_COUNT, FIRME_GPI_REALM, &result);
    context.output.registers[0] = result.x0;
    context.output.registers[1] = result.x1;
    context.output.registers[2] = result.x2;
    assert(ops->validate_output(&context) == VAL_TRUE);
    native_reply(NATIVE_CONTINUE, FIRME_DENIED, 0ULL, UINT64_MAX);
    native_reply(NATIVE_SET, FIRME_SUCCESS, 1ULL, UINT64_MAX);
    assert(ops->cleanup(&context) == VAL_ERROR);
    assert(native_releases == 1U && native_issued == native_queued);

    native_reset();
    memset(context.scratch, 0, sizeof(context.scratch));
    context.call_issued = VAL_FALSE;
    assert(ops->prepare_state(&context) == VAL_SUCCESS);
    assert(ops->build_valid_input(&context) == VAL_SUCCESS);
    native_reply(NATIVE_SET, FIRME_INCOMPLETE, 1ULL, 41ULL);
    context.call_issued = VAL_TRUE;
    val_firme_gm_gpi_set(NATIVE_BASE, NATIVE_COUNT, FIRME_GPI_REALM, &result);
    context.output.registers[0] = result.x0;
    context.output.registers[1] = result.x1;
    context.output.registers[2] = result.x2;
    assert(ops->validate_output(&context) == VAL_TRUE);
    native_reply(NATIVE_CONTINUE, FIRME_SUCCESS, NATIVE_COUNT - 1U, UINT64_MAX);
    native_reply(NATIVE_SET, FIRME_SUCCESS, NATIVE_COUNT, UINT64_MAX);
    assert(ops->cleanup(&context) == VAL_SUCCESS);
    assert(native_releases == 1U && native_issued == native_queued);

    native_reset();
    memset(context.scratch, 0, sizeof(context.scratch));
    context.call_issued = VAL_FALSE;
    abi.id = VAL_FIRME_ABI_ID_GM_GPI_OP_CONTINUE;
    native_reply(NATIVE_SET, FIRME_SUCCESS, NATIVE_COUNT, UINT64_MAX);
    assert(ops->prepare_state(&context) == VAL_FIRME_ABI_SCENARIO_UNAVAILABLE);
    native_reply(NATIVE_SET, FIRME_SUCCESS, NATIVE_COUNT, UINT64_MAX);
    assert(ops->cleanup(&context) == VAL_SUCCESS && native_releases == 1U);

    native_reset();
    memset(context.scratch, 0, sizeof(context.scratch));
    abi.id = VAL_FIRME_ABI_ID_GM_L1_GPT_DESTROY;
    native_reply(NATIVE_CREATE, FIRME_SUCCESS, UINT64_MAX, UINT64_MAX);
    assert(ops->prepare_state(&context) == VAL_SUCCESS);
    assert(ops->build_valid_input(&context) == VAL_SUCCESS);
    native_reply(NATIVE_DESTROY, FIRME_SUCCESS, UINT64_MAX, native_memory.object_base);
    context.call_issued = VAL_TRUE;
    val_firme_gm_l1_gpt_destroy(NATIVE_BASE, &result);
    context.output.registers[0] = result.x0;
    context.output.registers[1] = result.x1;
    context.output.registers[2] = result.x2;
    assert(ops->validate_output(&context) == VAL_TRUE);
    assert(ops->cleanup(&context) == VAL_SUCCESS && native_releases == 1U);

    native_reset();
    memset(context.scratch, 0, sizeof(context.scratch));
    context.call_issued = VAL_FALSE;
    native_reply(NATIVE_CREATE, FIRME_SUCCESS, UINT64_MAX, UINT64_MAX);
    assert(ops->prepare_state(&context) == VAL_SUCCESS);
    assert(ops->build_valid_input(&context) == VAL_SUCCESS);
    native_reply(NATIVE_DESTROY, FIRME_SUCCESS, native_memory.object_base, UINT64_MAX);
    context.call_issued = VAL_TRUE;
    val_firme_gm_l1_gpt_destroy(NATIVE_BASE, &result);
    context.output.registers[0] = result.x0;
    context.output.registers[1] = result.x1;
    context.output.registers[2] = result.x2;
    assert(ops->validate_output(&context) == VAL_FALSE);
    assert(ops->cleanup(&context) == VAL_ERROR && !native_releases);

    native_reset();
    memset(context.scratch, 0, sizeof(context.scratch));
    context.call_issued = VAL_FALSE;
    abi.id = VAL_FIRME_ABI_ID_GM_L1_GPT_CREATE;
    assert(ops->prepare_state(&context) == VAL_SUCCESS);
    assert(ops->build_valid_input(&context) == VAL_SUCCESS);
    memset(&context.output, 0, sizeof(context.output));
    assert(ops->validate_output(&context) == VAL_FALSE);
    assert(!context.scratch[GPT_ADAPTER_RECORDED]);
    assert(ops->cleanup(&context) == VAL_SUCCESS);
    assert(!native_issued && native_releases == 1U && !native_lease_live);
}

/** @brief Record rejected real outputs once; never consume outputs of unissued calls. */
static void native_adapter_cleanup_test(void)
{
    val_firme_abi_descriptor_t abi = {.id = VAL_FIRME_ABI_ID_GM_GPI_OP_CONTINUE};
    val_firme_abi_case_t test_case = {.mutation = VAL_FIRME_MUTATION_NONE};
    val_firme_abi_test_context_t context = {
        .abi = &abi, .test_case = &test_case, .world = FIRME_ACS_WORLD_REALM,
    };
    const val_firme_abi_test_ops_t *ops = val_firme_gpt_abi_adapter_get(abi.id);
    firme_res_t result;

    native_reset();
    native_reply(NATIVE_SET, FIRME_INCOMPLETE, 1ULL, 41ULL);
    assert(ops->prepare_state(&context) == VAL_SUCCESS);
    assert(ops->build_valid_input(&context) == VAL_SUCCESS);
    native_reply(NATIVE_CONTINUE, FIRME_BUSY, UINT64_MAX, 42ULL);
    context.call_issued = VAL_TRUE;
    val_firme_gm_gpi_op_continue(41ULL, &result);
    context.output.registers[0] = result.x0;
    context.output.registers[1] = result.x1;
    context.output.registers[2] = result.x2;
    native_reply(NATIVE_CONTINUE, FIRME_SUCCESS, 3ULL, UINT64_MAX);
    native_reply(NATIVE_SET, FIRME_SUCCESS, NATIVE_COUNT, UINT64_MAX);
    assert(ops->cleanup(&context) == VAL_SUCCESS);
    assert(context.scratch[GPT_ADAPTER_RECORDED] == VAL_TRUE);
    assert(native_calls[2].argument == 42ULL && native_releases == 1U);
    assert(native_issued == native_queued);
    assert(ops->cleanup(&context) == VAL_SUCCESS);
    assert(native_issued == native_queued && native_releases == 1U);

    native_reset();
    memset(context.scratch, 0, sizeof(context.scratch));
    context.call_issued = VAL_FALSE;
    native_reply(NATIVE_SET, FIRME_INCOMPLETE, 1ULL, 51ULL);
    assert(ops->prepare_state(&context) == VAL_SUCCESS);
    assert(ops->build_valid_input(&context) == VAL_SUCCESS);
    context.output.registers[0] = (uint64_t)FIRME_INCOMPLETE;
    context.output.registers[1] = UINT64_MAX;
    context.output.registers[2] = UINT64_MAX;
    native_reply(NATIVE_CONTINUE, FIRME_SUCCESS, 3ULL, UINT64_MAX);
    native_reply(NATIVE_SET, FIRME_SUCCESS, NATIVE_COUNT, UINT64_MAX);
    assert(ops->cleanup(&context) == VAL_SUCCESS);
    assert(!context.scratch[GPT_ADAPTER_RECORDED]);
    assert(native_calls[1].argument == 51ULL && native_releases == 1U);
    assert(native_issued == native_queued);

    native_reset();
    memset(context.scratch, 0, sizeof(context.scratch));
    context.call_issued = VAL_FALSE;
    abi.id = VAL_FIRME_ABI_ID_GM_L1_GPT_DESTROY;
    native_reply(NATIVE_CREATE, FIRME_SUCCESS, UINT64_MAX, UINT64_MAX);
    assert(ops->prepare_state(&context) == VAL_SUCCESS);
    assert(ops->build_valid_input(&context) == VAL_SUCCESS);
    native_reply(NATIVE_DESTROY, FIRME_SUCCESS, UINT64_MAX, native_memory.object_base);
    assert(ops->cleanup(&context) == VAL_SUCCESS);
    assert(!context.scratch[GPT_ADAPTER_RECORDED]);
    assert(native_issued == 2U && native_releases == 1U);

    native_reset();
    memset(context.scratch, 0, sizeof(context.scratch));
    context.call_issued = VAL_FALSE;
    abi.id = VAL_FIRME_ABI_ID_GM_L1_GPT_CREATE;
    assert(ops->prepare_state(&context) == VAL_SUCCESS);
    assert(ops->build_valid_input(&context) == VAL_SUCCESS);
    native_reply(NATIVE_CREATE, FIRME_INCOMPLETE, UINT64_MAX, UINT64_MAX);
    context.call_issued = VAL_TRUE;
    val_firme_gm_l1_gpt_create(NATIVE_BASE, native_memory.object_base, &result);
    context.output.registers[0] = result.x0;
    context.output.registers[1] = result.x1;
    context.output.registers[2] = result.x2;
    native_reply(NATIVE_CREATE, FIRME_SUCCESS, UINT64_MAX, UINT64_MAX);
    native_reply(NATIVE_DESTROY, FIRME_SUCCESS, UINT64_MAX, native_memory.object_base);
    assert(ops->cleanup(&context) == VAL_SUCCESS);
    assert(context.scratch[GPT_ADAPTER_RECORDED] == VAL_TRUE);
    assert(native_calls[1].argument == 0ULL && native_releases == 1U);
    assert(native_issued == native_queued);

    native_reset();
    memset(context.scratch, 0, sizeof(context.scratch));
    context.call_issued = VAL_FALSE;
    abi.id = VAL_FIRME_ABI_ID_GM_GPI_SET;
    assert(ops->prepare_state(&context) == VAL_SUCCESS);
    assert(ops->build_valid_input(&context) == VAL_SUCCESS);
    native_reply(NATIVE_SET, FIRME_INCOMPLETE, NATIVE_COUNT, UINT64_MAX);
    context.call_issued = VAL_TRUE;
    val_firme_gm_gpi_set(NATIVE_BASE, NATIVE_COUNT, FIRME_GPI_REALM, &result);
    context.output.registers[0] = result.x0;
    context.output.registers[1] = result.x1;
    context.output.registers[2] = result.x2;
    assert(ops->cleanup(&context) == VAL_ERROR);
    assert(context.scratch[GPT_ADAPTER_RECORDED] == VAL_TRUE);
    assert(g_gpt_adapter_fixture[FIRME_ACS_WORLD_REALM].poisoned == VAL_TRUE);
    assert(native_issued == 1U && !native_releases && native_lease_live);

    native_reset();
    memset(context.scratch, 0, sizeof(context.scratch));
    context.call_issued = VAL_FALSE;
    assert(ops->prepare_state(&context) == VAL_SUCCESS);
    assert(ops->build_valid_input(&context) == VAL_SUCCESS);
    native_reply(NATIVE_SET, FIRME_SUCCESS, 1ULL, UINT64_MAX);
    context.call_issued = VAL_TRUE;
    val_firme_gm_gpi_set(NATIVE_BASE, NATIVE_COUNT, FIRME_GPI_REALM, &result);
    context.output.registers[0] = result.x0;
    context.output.registers[1] = result.x1;
    context.output.registers[2] = result.x2;
    native_gpis[0] = FIRME_GPI_NONSECURE;
    assert(ops->cleanup(&context) == VAL_ERROR);
    assert(g_gpt_adapter_fixture[FIRME_ACS_WORLD_REALM].poisoned == VAL_TRUE);
    assert(native_issued == 1U && !native_releases && native_lease_live);

    native_reset();
    memset(context.scratch, 0, sizeof(context.scratch));
    context.call_issued = VAL_FALSE;
    abi.id = VAL_FIRME_ABI_ID_GM_GPI_OP_CONTINUE;
    native_reply(NATIVE_SET, FIRME_INCOMPLETE, 1ULL, 61ULL);
    assert(ops->prepare_state(&context) == VAL_SUCCESS);
    assert(ops->build_valid_input(&context) == VAL_SUCCESS);
    native_reply(NATIVE_CONTINUE, FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
    context.call_issued = VAL_TRUE;
    val_firme_gm_gpi_op_continue(61ULL, &result);
    context.output.registers[0] = result.x0;
    context.output.registers[1] = result.x1;
    context.output.registers[2] = result.x2;
    assert(ops->cleanup(&context) == VAL_ERROR);
    assert(native_issued == 2U && !native_releases && native_lease_live);
    assert(g_gpt_adapter_fixture[FIRME_ACS_WORLD_REALM].poisoned == VAL_TRUE);
}

/** @brief Require matching return/out on owned L1 PASS, failed checks, and failed cleanup. */
static void native_l1_rule_test(void)
{
    firme_rule_res_t out;

    /* Step 1: Successful operations and verified release remain PASS. */
    native_reset();
    native_reply(NATIVE_CREATE, FIRME_SUCCESS, UINT64_MAX, UINT64_MAX);
    native_reply(NATIVE_DESTROY, FIRME_SUCCESS, UINT64_MAX, NATIVE_BASE + 0x2000ULL);
    native_reply(NATIVE_CREATE, FIRME_SUCCESS, UINT64_MAX, UINT64_MAX);
    native_reply(NATIVE_DESTROY, FIRME_SUCCESS, UINT64_MAX, NATIVE_POOL + 0x2000ULL);
    assert(val_firme_gpt_l1_rule_run(FIRME_RULE_R0170, FIRME_ACS_WORLD_REALM,
        NATIVE_BASE, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_PASS && native_releases == 2U);
    assert(native_issued == native_queued);

    /* Step 2: A failed assertion with verified cleanup remains FAIL. */
    native_reset();
    native_create_mismatch = VAL_TRUE;
    native_reply(NATIVE_CREATE, FIRME_SUCCESS, UINT64_MAX, UINT64_MAX);
    native_reply(NATIVE_DESTROY, FIRME_SUCCESS, UINT64_MAX, NATIVE_BASE + 0x2000ULL);
    assert(val_firme_gpt_l1_rule_run(FIRME_RULE_R0170, FIRME_ACS_WORLD_REALM,
        NATIVE_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL && native_releases == 1U);
    assert(out.fail_step == VAL_FIRME_GPT_L1_STEP_INVOKE);

    /* Step 3: Uncertain DESTROY correlation retains the lease and original diagnostic. */
    native_reset();
    native_reply(NATIVE_CREATE, FIRME_SUCCESS, UINT64_MAX, UINT64_MAX);
    native_reply(NATIVE_DESTROY, FIRME_SUCCESS, NATIVE_BASE + 0x2000ULL, UINT64_MAX);
    assert(val_firme_gpt_l1_rule_run(FIRME_RULE_R0185, FIRME_ACS_WORLD_REALM,
        NATIVE_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_ERR && !native_releases);
    assert(out.fail_step == VAL_FIRME_GPT_L1_STEP_INVOKE);
    assert(g_l1_fixture.memory.token && g_l1_fixture_lock && native_lease_live);

    /* Step 4: Even a correct operation cannot PASS when release remains uncertain. */
    native_reset();
    native_release_error = VAL_TRUE;
    native_reply(NATIVE_CREATE, FIRME_SUCCESS, UINT64_MAX, UINT64_MAX);
    native_reply(NATIVE_DESTROY, FIRME_SUCCESS, UINT64_MAX, NATIVE_BASE + 0x2000ULL);
    assert(val_firme_gpt_l1_rule_run(FIRME_RULE_R0170, FIRME_ACS_WORLD_REALM,
        NATIVE_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_ERR && !native_releases);
    assert(out.fail_step == VAL_FIRME_GPT_L1_STEP_CLEANUP);
    assert(g_l1_fixture.memory.token && g_l1_fixture_lock && native_lease_live);
    assert(native_issued == native_queued);
}

/** @brief Check allocation/observation geometry for the other advertised PGS values. */
static void native_l1_geometry_test(void)
{
    const uint64_t features[] = {FIRME_GM_PGS_4KB, FIRME_GM_PGS_16KB};
    val_firme_gpt_fixture_t fixture = {0};
    size_t index;

    for (index = 0U; index < sizeof(features) / sizeof(features[0]); index++) {
        native_reset();
        native_feature = features[index];
        assert(val_firme_gpt_fixture_begin(&fixture, FIRME_ACS_WORLD_REALM,
            PAL_FIRME_GPT_MEMORY_L1_NON_SELF) == PAL_FIRME_GPT_SCENARIO_READY);
        assert(fixture.object_size == fixture.range_size / fixture.granule_size / 2ULL);
        assert(fixture.object_size == fixture.object_granule_size);
        native_reply(NATIVE_CREATE, FIRME_SUCCESS, UINT64_MAX, UINT64_MAX);
        assert(val_firme_gpt_fixture_l1_complete(&fixture, VAL_FALSE) == VAL_SUCCESS);
        native_reply(NATIVE_DESTROY, FIRME_SUCCESS, UINT64_MAX, fixture.memory.object_base);
        assert(val_firme_gpt_fixture_cleanup(&fixture) == VAL_SUCCESS);
        assert(native_releases == 1U && native_issued == native_queued);
    }
}

/** @brief Reject absent services and observers before fixture mutations. */
static void native_prerequisite_test(void)
{
    val_firme_gpt_fixture_t fixture = {0};

    native_reset();
    assert(val_firme_gpt_fixture_begin(&fixture, FIRME_ACS_WORLD_REALM,
        PAL_FIRME_GPT_MEMORY_TRANSITION) == PAL_FIRME_GPT_SCENARIO_UNAVAILABLE);
    assert(!native_acquires && !native_issued);
    assert(pal_firme_gpt_memory_register(&native_memory_ops) == PAL_FIRME_GPT_SCENARIO_READY);
    assert(pal_firme_gpt_memory_register(&native_memory_ops) == PAL_FIRME_GPT_SCENARIO_ERROR);
    native_abis = FIRME_GM_FEATURE_REG0_GPI_SET_BIT;
    assert(val_firme_gpt_fixture_begin(&fixture, FIRME_ACS_WORLD_REALM,
        PAL_FIRME_GPT_MEMORY_L1_NON_SELF) == PAL_FIRME_GPT_SCENARIO_UNAVAILABLE);
    assert(!native_acquires && !native_issued);
    native_abis = FIRME_GM_FEATURE_REG0_L1_CREATE_BIT;
    assert(val_firme_gpt_fixture_begin(&fixture, FIRME_ACS_WORLD_REALM,
        PAL_FIRME_GPT_MEMORY_L1_NON_SELF) == PAL_FIRME_GPT_SCENARIO_ERROR);
    assert(!native_acquires && !native_issued);
    native_abis = FIRME_GM_FEATURE_REG0_ALLOCATED_BITS;
    native_par = FIRME_GPT_PAR_STATE_EL3_BLOCK;
    assert(val_firme_gpt_fixture_begin(&fixture, FIRME_ACS_WORLD_REALM,
        PAL_FIRME_GPT_MEMORY_L1_NON_SELF) == PAL_FIRME_GPT_SCENARIO_ERROR);
    assert(!native_issued && !native_releases && fixture.memory.token);
    native_reset();
    fixture = (val_firme_gpt_fixture_t){0};
    native_short_observation = VAL_TRUE;
    assert(val_firme_gpt_fixture_begin(&fixture, FIRME_ACS_WORLD_REALM,
        PAL_FIRME_GPT_MEMORY_TRANSITION) == PAL_FIRME_GPT_SCENARIO_ERROR);
    assert(!native_issued && !native_releases);
}

/** @brief Exercise NS-owned same-GPI requests without inventing a continuation cookie. */
static void native_ns_fixture_test(void)
{
    val_firme_gpt_fixture_t fixture = {0};
    firme_res_t result = {0};

    native_reset();
    native_abis = FIRME_GM_FEATURE_REG0_GPI_SET_BIT;
    assert(val_firme_gpt_fixture_begin(&fixture, FIRME_ACS_WORLD_NSEL2,
        PAL_FIRME_GPT_MEMORY_TRANSITION) == PAL_FIRME_GPT_SCENARIO_ERROR);
    assert(!native_acquires && !native_issued);

    native_abis = FIRME_GM_FEATURE_REG0_ALLOCATED_BITS;
    assert(val_firme_gpt_fixture_begin(&fixture, FIRME_ACS_WORLD_NSEL2,
        PAL_FIRME_GPT_MEMORY_TRANSITION) == PAL_FIRME_GPT_SCENARIO_READY);
    assert(fixture.target_gpi == FIRME_GPI_NONSECURE);
    native_reply(NATIVE_SET, FIRME_INCOMPLETE, 1ULL, 67ULL);
    assert(val_firme_gpt_fixture_retain(&fixture) == PAL_FIRME_GPT_SCENARIO_READY);
    assert(fixture.pending && fixture.cookie == 67ULL);
    native_reply(NATIVE_CONTINUE, FIRME_BUSY, UINT64_MAX, 68ULL);
    assert(val_firme_gpt_fixture_gpi_step(&fixture, &result) == VAL_SUCCESS);
    assert(fixture.pending && fixture.cookie == 68ULL && fixture.completed == 1ULL);
    native_reply(NATIVE_CONTINUE, FIRME_SUCCESS, NATIVE_COUNT - 1U, UINT64_MAX);
    assert(val_firme_gpt_fixture_complete_transition(&fixture) == VAL_SUCCESS);
    assert(native_calls[2].argument == 68ULL);
    assert(val_firme_gpt_fixture_cleanup(&fixture) == VAL_SUCCESS);
    assert(native_releases == 1U && native_issued == native_queued);
}

/** @brief Forward all fixture calls, preserve their caller and quarantine lost completions. */
static void native_remote_fixture_test(void)
{
    val_firme_gpt_fixture_t fixture = {0};
    native_frame_route_t route = {.world = FIRME_ACS_WORLD_REALM};
    val_firme_abi_test_context_t context = {
        .world = FIRME_ACS_WORLD_REALM,
        .invoke_frame = native_frame_invoke,
        .invoke_cookie = &route,
    };
    firme_res_t result = {0};
    uint64_t l1_fids = (UINT64_C(1) << (FIRME_FID_SERVICE_FEATURES - FIRME_FID_SERVICE_VERSION)) |
                      (UINT64_C(1) << (FIRME_FID_GM_GPT_INFO - FIRME_FID_SERVICE_VERSION)) |
                      (UINT64_C(1) << (FIRME_FID_GM_L1_GPT_CREATE - FIRME_FID_SERVICE_VERSION)) |
                      (UINT64_C(1) << (FIRME_FID_GM_L1_GPT_DESTROY - FIRME_FID_SERVICE_VERSION));

    native_reset();
    assert(val_firme_gpt_fixture_begin_abi(&fixture, &context,
        PAL_FIRME_GPT_MEMORY_TRANSITION) == PAL_FIRME_GPT_SCENARIO_READY);
    assert(route.calls == 2U && fixture.has_abi_context);
    context.invoke_frame = NULL;
    native_reply(NATIVE_SET, FIRME_SUCCESS, NATIVE_COUNT, UINT64_MAX);
    assert(val_firme_gpt_fixture_gpi_step(&fixture, &result) == VAL_SUCCESS);
    assert(route.calls == 3U && fixture.completed == NATIVE_COUNT);
    native_reply(NATIVE_SET, FIRME_SUCCESS, NATIVE_COUNT, UINT64_MAX);
    assert(val_firme_gpt_fixture_cleanup(&fixture) == VAL_SUCCESS);
    assert(route.calls == 4U && native_releases == 1U);

    native_reset();
    route = (native_frame_route_t){.world = FIRME_ACS_WORLD_REALM};
    context.invoke_frame = native_frame_invoke;
    assert(val_firme_gpt_fixture_begin_abi(&fixture, &context,
        PAL_FIRME_GPT_MEMORY_L1_NON_SELF) == PAL_FIRME_GPT_SCENARIO_READY);
    native_reply(NATIVE_CREATE, FIRME_SUCCESS, UINT64_MAX, UINT64_MAX);
    assert(val_firme_gpt_fixture_l1_complete(&fixture, VAL_FALSE) == VAL_SUCCESS);
    native_reply(NATIVE_DESTROY, FIRME_SUCCESS, UINT64_MAX, fixture.memory.object_base);
    assert(val_firme_gpt_fixture_cleanup(&fixture) == VAL_SUCCESS);
    assert(route.fid_mask == l1_fids && native_releases == 1U);

    native_reset();
    route = (native_frame_route_t){.world = FIRME_ACS_WORLD_REALM};
    assert(val_firme_gpt_fixture_begin_abi(&fixture, &context,
        PAL_FIRME_GPT_MEMORY_TRANSITION) == PAL_FIRME_GPT_SCENARIO_READY);
    route.fail_next = route.issued_on_failure = VAL_TRUE;
    assert(val_firme_gpt_fixture_gpi_step(&fixture, &result) == VAL_ERROR);
    assert(fixture.poisoned && fixture.abi_context.transport_uncertain);
    assert(val_firme_gpt_fixture_cleanup(&fixture) == VAL_ERROR);
    assert(native_releases == 0U && native_issued == 0U);
}

/** @brief Run native controlled-session regressions; not evidence of DUT support. */
int main(void)
{
    native_prerequisite_test();
    native_ns_fixture_test();
    native_progress_test();
    native_busy_observation_test();
    native_cleanup_test();
    native_quarantine_test();
    native_l1_test();
    native_adapter_test();
    native_adapter_cleanup_test();
    native_l1_rule_test();
    native_l1_geometry_test();
    native_remote_fixture_test();
    puts("GPT fixture/adapter regressions: PASS");
    return VAL_SUCCESS;
}
