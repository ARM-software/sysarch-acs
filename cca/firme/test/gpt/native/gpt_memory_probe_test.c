/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>

#define PLAT_FIRME_GPT_L1_BACKED_RESERVED 1U
#define PLAT_FIRME_GPT_L1_RANGE_BASE UINT64_C(0x900000000)
#define PLAT_FIRME_GPT_L1_RANGE_SIZE UINT64_C(0x40000000)
#define PLAT_FIRME_GPT_L1_POOL_BASE UINT64_C(0x880100000)
#define PLAT_FIRME_GPT_L1_POOL_SIZE UINT64_C(0x100000)

#include "../../../platform/pal_baremetal/targets/fvp/src/pal_firme_gpt.c"
#include "../../../platform/pal_baremetal/targets/fvp/src/pal_firme_gpt_memory.c"
#include "../../../val/src/val_firme_gpt_probe.c"

#define NATIVE_GPF_ESR ((UINT64_C(0x25) << 26) | UINT64_C(0x28))

static uint32_t g_fault;
static uint32_t g_probe_error;
static uint32_t g_probe_calls;
uint8_t __text_start__[1];
uint8_t __stack_end__[1];

/** @brief Model the separately tested MMU probe without confusing setup errors and GPFs. */
int val_mmu_probe_native_read64_el2(uint64_t address, uint64_t image_start,
    uint64_t image_end, uint64_t *value, val_fault_info_t *fault)
{
    assert(pal_firme_gpt_memory_probe_allowed(address));
    assert(image_start == (uint64_t)(uintptr_t)__text_start__);
    assert(image_end == (uint64_t)(uintptr_t)__stack_end__);
    g_probe_calls++;
    *value = UINT64_C(0xfeedbeef);
    *fault = (val_fault_info_t){.faulted = g_fault, .esr = NATIVE_GPF_ESR};
    return g_probe_error ? VAL_ERROR : VAL_SUCCESS;
}

/** @brief Verify explicit reservation geometry and bounded mapping/fault lifetime. */
int main(void)
{
    pal_firme_gpt_memory_t memory = {0};
    firme_acs_call_res_t response = {0};
    uint64_t granule = UINT64_C(0x10000);
    uint64_t range = PLAT_FIRME_GPT_L1_RANGE_SIZE;
    uint64_t object = range / granule / 2ULL;
    uint32_t calls;

    assert(pal_firme_gpt_memory_layout(FIRME_ACS_WORLD_REALM,
        PAL_FIRME_GPT_MEMORY_L1_NON_SELF, granule, range, object, &memory) ==
            PAL_FIRME_GPT_SCENARIO_READY);
    assert(memory.range_base == PLAT_FIRME_GPT_L1_RANGE_BASE);
    assert(memory.pool_base == PLAT_FIRME_GPT_L1_POOL_BASE && memory.pool_size == granule);
    assert(memory.object_base % object == 0ULL && !memory.token);
    assert(pal_firme_gpt_memory_layout(FIRME_ACS_WORLD_REALM,
        PAL_FIRME_GPT_MEMORY_L1_SELF, granule, range, object, &memory) ==
            PAL_FIRME_GPT_SCENARIO_UNAVAILABLE);
    assert(pal_firme_gpt_memory_layout(FIRME_ACS_WORLD_SECURE,
        PAL_FIRME_GPT_MEMORY_L1_NON_SELF, granule, range, object, &memory) ==
            PAL_FIRME_GPT_SCENARIO_UNAVAILABLE);
    assert(pal_firme_gpt_memory_layout(FIRME_ACS_WORLD_REALM,
        PAL_FIRME_GPT_MEMORY_L1_NON_SELF, granule, range * 2ULL, object, &memory) ==
            PAL_FIRME_GPT_SCENARIO_ERROR);
    assert(pal_firme_gpt_memory_layout(FIRME_ACS_WORLD_REALM,
        PAL_FIRME_GPT_MEMORY_L1_NON_SELF, granule, range, 0ULL, &memory) ==
            PAL_FIRME_GPT_SCENARIO_ERROR);
    assert(!pal_firme_gpt_memory_probe_allowed(PLAT_FIRME_GPT_L1_RANGE_BASE - 8ULL));
    assert(!pal_firme_gpt_memory_probe_allowed(PLAT_FIRME_GPT_L1_RANGE_BASE + range));
    assert(!pal_firme_gpt_memory_probe_allowed(PLAT_FIRME_GPT_L1_POOL_BASE + 1ULL));
    assert(!pal_firme_gpt_memory_probe_allowed(UINT64_MAX - 7ULL));

    val_firme_gpt_probe_command(PLAT_FIRME_GPT_L1_RANGE_BASE, &response);
    assert(response.x0 == ACS_STATUS_PASS && !response.x1 &&
           response.x2 == UINT64_C(0xfeedbeef));
    g_fault = VAL_TRUE;
    val_firme_gpt_probe_command(PLAT_FIRME_GPT_L1_POOL_BASE, &response);
    assert(response.x0 == ACS_STATUS_PASS && response.x1 == VAL_TRUE &&
           response.x2 == NATIVE_GPF_ESR);
    g_fault = VAL_FALSE;
    g_probe_error = VAL_TRUE;
    val_firme_gpt_probe_command(PLAT_FIRME_GPT_L1_RANGE_BASE, &response);
    assert(response.x0 == ACS_STATUS_ERR);
    g_probe_error = VAL_FALSE;
    calls = g_probe_calls;
    val_firme_gpt_probe_command(UINT64_C(0x800000000), &response);
    assert(response.x0 == ACS_STATUS_ERR && g_probe_calls == calls);
    puts("GPT PAL reservation and scoped memory probe: PASS");
    return VAL_SUCCESS;
}
