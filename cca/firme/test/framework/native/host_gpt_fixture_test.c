/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>

#include "../../../val/host/val_host_gpt.c"
#include "../../../platform/pal_baremetal/targets/fvp/src/pal_firme_gpt.c"
#include "../../../platform/pal_baremetal/targets/fvp/src/pal_firme_gpt_memory.c"

#define NATIVE_GRANULE_COUNT 2U
#define NATIVE_GPF_ESR ((UINT64_C(0x25) << 26) | UINT64_C(0x28))
#define NATIVE_TRANSLATION_ESR ((UINT64_C(0x25) << 26) | UINT64_C(0x4))

static uint32_t g_pe_index;
static uint32_t g_gpc2 = VAL_TRUE;
static uint32_t g_bad_abort;
static uint32_t g_probe_count;
static uint32_t g_ping_count;
static uint64_t g_gpis[NATIVE_GRANULE_COUNT] = {FIRME_GPI_NONSECURE, FIRME_GPI_NONSECURE};

/** @brief Keep expected native negative-case logs quiet. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *format, ...)
{
    return VAL_SUCCESS;
}

/** @brief Model the physical owner of the fixture. */
uint32_t val_pe_index(void)
{
    return g_pe_index;
}

/** @brief Control whether NSO is implemented, not whether the test should pass. */
uint32_t val_firme_feat_rme_gpc2_present(void)
{
    return g_gpc2;
}

/** @brief Decode the native PAS access policy independently of generated outputs. */
uint32_t val_firme_expect_access_world(uint64_t gpi, firme_acs_world_t world)
{
    return (gpi == FIRME_GPI_REALM && world == FIRME_ACS_WORLD_REALM) ||
           (gpi == FIRME_GPI_SECURE && world == FIRME_ACS_WORLD_SECURE) ||
           ((gpi == FIRME_GPI_NONSECURE || gpi == FIRME_GPI_NSO) &&
            world == FIRME_ACS_WORLD_NSEL2);
}

/** @brief Probe a separate memory-protection model rather than FIRME return values. */
static void native_probe(firme_acs_world_t world, uint64_t address,
                           firme_acs_call_res_t *result)
{
    uint64_t index = (address - PLAT_FIRME_GPT_BASE_NS) / PLAT_FIRME_GPT_GRANULE_SIZE;
    uint32_t accessible;

    assert(pal_firme_gpt_memory_probe_allowed(address));
    assert(index < NATIVE_GRANULE_COUNT);
    g_probe_count++;
    accessible = val_firme_expect_access_world(g_gpis[index], world);
    *result = (firme_acs_call_res_t){
        .x0 = ACS_STATUS_PASS,
        .x1 = accessible ? VAL_FALSE : VAL_TRUE,
        .x2 = accessible ? UINT64_C(0x1234) :
              (g_bad_abort ? NATIVE_TRANSLATION_ESR : NATIVE_GPF_ESR),
    };
}

/** @brief Keep Host probing local while the other observers use transport. */
void val_firme_gpt_probe_command(uint64_t address, firme_acs_call_res_t *result)
{
    native_probe(FIRME_ACS_WORLD_NSEL2, address, result);
}

/** @brief Exercise both preflight and compact, per-world independent observation. */
int val_firme_world_call(firme_acs_world_t world, firme_acs_cmd_t cmd,
    uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3,
    firme_acs_call_res_t *result)
{
    uint64_t index;

    assert(world == FIRME_ACS_WORLD_SECURE || world == FIRME_ACS_WORLD_REALM);
    if (cmd == FIRME_ACS_CMD_PING) {
        g_ping_count++;
        *result = (firme_acs_call_res_t){.x0 = ACS_STATUS_PASS};
    } else if (cmd == FIRME_ACS_CMD_RUN_DIAG) {
        assert(world == FIRME_ACS_WORLD_SECURE && arg0 == FIRME_DIAG_READ_NS_PAS);
        index = (arg1 - PLAT_FIRME_GPT_BASE_NS) / PLAT_FIRME_GPT_GRANULE_SIZE;
        assert(index < NATIVE_GRANULE_COUNT);
        *result = (firme_acs_call_res_t){
            .x0 = ACS_STATUS_PASS,
            .x1 = g_gpis[index] == FIRME_GPI_NONSECURE ? VAL_FALSE : VAL_TRUE,
            .x2 = NATIVE_GPF_ESR,
        };
    } else {
        assert(cmd == FIRME_ACS_CMD_GPT_PROBE);
        native_probe(world, arg0, result);
    }

    return VAL_SUCCESS;
}

/** @brief Cover cross-world ownership, full-granule observation, NSO distinction and quarantine. */
int main(void)
{
    pal_firme_gpt_memory_t memory = {0};
    pal_firme_gpt_memory_t other = {0};
    uint64_t checked = 0ULL;
    uint64_t token;
    uint64_t granule_size = PLAT_FIRME_GPT_GRANULE_SIZE;
    uint32_t world;
    uint32_t probes;

    assert(val_host_gpt_init() == VAL_SUCCESS);
    assert(val_host_gpt_acquire(FIRME_ACS_WORLD_REALM, PAL_FIRME_GPT_MEMORY_L1_NON_SELF,
        granule_size, UINT64_C(1) << 30, UINT64_C(0x20000), &memory) ==
            PAL_FIRME_GPT_SCENARIO_UNAVAILABLE);
    assert(!g_host_gpt_owned);
    for (world = FIRME_ACS_WORLD_NSEL2; world <= FIRME_ACS_WORLD_REALM; world++) {
        assert(val_host_gpt_acquire(world, PAL_FIRME_GPT_MEMORY_TRANSITION,
            granule_size, 0ULL, 0ULL, &memory) == PAL_FIRME_GPT_SCENARIO_READY);
        assert(memory.range_size == NATIVE_GRANULE_COUNT * granule_size);
        assert(val_host_gpt_release(memory.token) == PAL_FIRME_GPT_SCENARIO_READY);
    }

    assert(g_ping_count == 6U);
    assert(val_host_gpt_acquire(FIRME_ACS_WORLD_REALM, PAL_FIRME_GPT_MEMORY_TRANSITION,
        granule_size, 0ULL, 0ULL, &memory) == PAL_FIRME_GPT_SCENARIO_READY);
    token = memory.token;
    assert(val_host_gpt_acquire(FIRME_ACS_WORLD_SECURE, PAL_FIRME_GPT_MEMORY_TRANSITION,
        granule_size, 0ULL, 0ULL, &other) == PAL_FIRME_GPT_SCENARIO_ERROR);
    assert(val_host_gpt_observe(memory.range_base, memory.range_size, granule_size,
        UINT32_C(1) << FIRME_GPI_NONSECURE, &checked) == PAL_FIRME_GPT_SCENARIO_READY);
    assert(checked == NATIVE_GRANULE_COUNT);
    probes = g_probe_count;
    assert(val_host_gpt_observe(memory.range_base, memory.range_size + granule_size,
        granule_size, UINT32_C(1) << FIRME_GPI_NONSECURE, &checked) ==
            PAL_FIRME_GPT_SCENARIO_ERROR);
    assert(!checked && g_probe_count == probes);

    g_gpis[1] = FIRME_GPI_NSO;
    assert(val_host_gpt_observe(memory.range_base, memory.range_size, granule_size,
        UINT32_C(1) << FIRME_GPI_NONSECURE, &checked) == PAL_FIRME_GPT_SCENARIO_ERROR);
    assert(checked == 1ULL);
    assert(val_host_gpt_observe(memory.range_base + granule_size, granule_size,
        granule_size, UINT32_C(1) << FIRME_GPI_NSO, &checked) == PAL_FIRME_GPT_SCENARIO_READY);
    assert(val_host_gpt_release(token) == PAL_FIRME_GPT_SCENARIO_ERROR && g_host_gpt_owned);

    g_gpis[1] = FIRME_GPI_REALM;
    assert(val_host_gpt_observe(memory.range_base + granule_size, granule_size,
        granule_size, UINT32_C(1) << FIRME_GPI_REALM, &checked) == PAL_FIRME_GPT_SCENARIO_READY);
    g_gpis[1] = FIRME_GPI_NONSECURE;
    g_bad_abort = VAL_TRUE;
    assert(val_host_gpt_release(token) == PAL_FIRME_GPT_SCENARIO_ERROR && g_host_gpt_owned);
    g_bad_abort = VAL_FALSE;
    assert(val_host_gpt_release(token + 1ULL) == PAL_FIRME_GPT_SCENARIO_ERROR);
    g_pe_index = 1U;
    assert(val_host_gpt_release(token) == PAL_FIRME_GPT_SCENARIO_ERROR);
    g_pe_index = 0U;
    assert(val_host_gpt_release(token) == PAL_FIRME_GPT_SCENARIO_READY);
    assert(!g_host_gpt_owned);
    assert(val_host_gpt_release(token) == PAL_FIRME_GPT_SCENARIO_ERROR);
    g_host_gpt_token = UINT64_MAX;
    assert(val_host_gpt_acquire(FIRME_ACS_WORLD_REALM, PAL_FIRME_GPT_MEMORY_TRANSITION,
        granule_size, 0ULL, 0ULL, &memory) == PAL_FIRME_GPT_SCENARIO_ERROR);
    assert(!g_host_gpt_owned && !memory.token && g_host_gpt_token == UINT64_MAX);
    puts("Host GPT owned multi-granule/caller fixture: PASS");
    return VAL_SUCCESS;
}
