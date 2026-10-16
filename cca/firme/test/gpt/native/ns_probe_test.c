/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>

#include "../../../val_common/src/val_mmu.c"

#define TEST_IMAGE_BASE UINT64_C(0x06000000)
#define TEST_IMAGE_END (TEST_IMAGE_BASE + UINT64_C(0x100000))
#define TEST_GRANULE UINT64_C(0xfb000000)

static _Alignas(PAGE_SIZE) val_ns_probe_tables_t g_test_tables;
static uint32_t g_test_pe;
static uint64_t g_test_sctlr = UINT64_C(5);
static uint64_t g_test_tcr = VAL_MMU_EL2_BOOT_TCR;
static uint64_t g_test_parange = UINT64_C(2);
static uint32_t g_test_fault;
static uint32_t g_test_scope_error;
static uint32_t g_test_clean_count;

/** @brief Model an ordinary non-VHE EL2 caller with caches and MMU enabled. */
uint64_t read_currentel(void)
{
    return UINT64_C(8);
}

/** @brief Select the enabled or disabled-MMU probe implementation. */
uint64_t read_sctlr_el2(void)
{
    return g_test_sctlr;
}

/** @brief Provide the current translation geometry without hardware. */
uint64_t val_read_tcr_el2(void)
{
    return g_test_tcr;
}

/** @brief Model a 40-bit physical address implementation. */
uint64_t val_read_id_aa64mmfr0_el1(void)
{
    return g_test_parange;
}

/** @brief No-op barrier in the native table-building model. */
void dsb_sy(void)
{
}

/** @brief Require publishing exactly the current PE's private translation tables. */
void val_cache_clean_poc(uintptr_t base, size_t size)
{
    assert(base == (uintptr_t)&g_ns_probe_tables[g_test_pe]);
    assert(size == sizeof(val_ns_probe_tables_t));
    g_test_clean_count++;
}

/** @brief Reject uncached architectural execution in this native-only fixture. */
int val_mmu_ns_read_scope_el2(uint64_t root, uint64_t tcr, uint64_t address,
                              uint64_t *value, val_fault_info_t *fault)
{
    return VAL_ERROR;
}

/** @brief Inspect the private native-PAS mapping before modeling a load or fault. */
int val_mmu_native_read_scope_el2(uint64_t root, uint64_t tcr, uint64_t address,
                                  uint64_t *value, val_fault_info_t *fault,
                                  uint64_t expected_root)
{
    const val_ns_probe_tables_t *tables = (const val_ns_probe_tables_t *)(uintptr_t)root;

    assert(tables == &g_ns_probe_tables[g_test_pe]);
    assert(expected_root == (uint64_t)(uintptr_t)g_val_mmu_l1);
    assert(address == VAL_NS_PROBE_ALIAS);
    assert((tcr & VAL_TCR_PS_MASK) == (g_test_parange << VAL_TCR_PS_SHIFT));
    assert(tables->root[2] == g_val_mmu_l1[2]);
    assert((tables->alias_l3[0] & DESC_ADDRESS_MASK) == UINT64_C(0x900000000));
    assert((tables->alias_l3[0] & DESC_NS) == 0ULL);
    assert((tables->alias_l3[0] & DESC_ATTRINDX(7)) == DESC_ATTRINDX(7));
    assert(g_test_clean_count && val_mmu_ns_probe_active());
    if (g_test_scope_error)
        return VAL_ERROR;
    if (g_test_fault) {
        fault->faulted = VAL_TRUE;
        fault->esr = (UINT64_C(0x25) << 26) | UINT64_C(0x28);
        fault->far = VAL_NS_PROBE_ALIAS;
        return VAL_ERROR;
    }

    *value = UINT64_C(0xabc);
    return VAL_SUCCESS;
}

/** @brief Select a synthetic PE without executing architectural instructions. */
uint32_t val_pe_index(void)
{
    return g_test_pe;
}

/** @brief Verify exact page mappings, NS selection and untouched virtual holes. */
static void test_tables(void)
{
    uint32_t index;
    uint64_t descriptor;

    assert(val_mmu_ns_probe_build(&g_test_tables, TEST_IMAGE_BASE,
                                  TEST_IMAGE_END, TEST_GRANULE) == VAL_SUCCESS);
    assert(g_test_tables.root[0] == make_table_desc(g_test_tables.image_l2));
    assert(g_test_tables.image_l2[TEST_IMAGE_BASE >> 21] ==
           make_table_desc(g_test_tables.image_l3));
    for (index = 0U; index < 256U; index++) {
        descriptor = g_test_tables.image_l3[index];
        assert((descriptor & DESC_ADDRESS_MASK) == TEST_IMAGE_BASE + index * PAGE_SIZE);
        assert((descriptor & DESC_NS) == 0ULL);
    }

    for (index = 256U; index < L3_ENTRIES; index++)
        assert(g_test_tables.image_l3[index] == 0ULL);
    descriptor = g_test_tables.alias_l3[0];
    assert((descriptor & DESC_ADDRESS_MASK) == TEST_GRANULE);
    assert((descriptor & (DESC_NS | DESC_PXN | DESC_UXN)) ==
           (DESC_NS | DESC_PXN | DESC_UXN));
    assert((VAL_NS_PROBE_TCR_NC & UINT64_C(0xf00)) == 0ULL);
    for (index = 1U; index < L1_ENTRIES; index++) {
        if (index != (VAL_NS_PROBE_ALIAS >> 30))
            assert(g_test_tables.root[index] == 0ULL);
        assert(g_test_tables.alias_l2[index] == 0ULL);
        assert(g_test_tables.alias_l3[index] == 0ULL);
    }

    assert(val_mmu_ns_probe_build(&g_test_tables, TEST_IMAGE_BASE,
        TEST_IMAGE_BASE + PAGE_SIZE, TEST_GRANULE + sizeof(uint64_t)) == VAL_SUCCESS);
    assert(g_test_tables.image_l3[1] == 0ULL);
    assert((g_test_tables.alias_l3[0] & DESC_ADDRESS_MASK) == TEST_GRANULE);
}

/** @brief Reject malformed image/PA geometry without guessing accessible memory. */
static void test_invalid_geometry(void)
{
    assert(val_mmu_ns_probe_build(NULL, TEST_IMAGE_BASE,
                                  TEST_IMAGE_END, TEST_GRANULE) == VAL_ERROR);
    assert(val_mmu_ns_probe_build(&g_test_tables, 0ULL,
                                  TEST_IMAGE_END, TEST_GRANULE) == VAL_ERROR);
    assert(val_mmu_ns_probe_build(&g_test_tables, TEST_IMAGE_BASE + 1U,
                                  TEST_IMAGE_END, TEST_GRANULE) == VAL_ERROR);
    assert(val_mmu_ns_probe_build(&g_test_tables, TEST_IMAGE_BASE,
                                  TEST_IMAGE_BASE, TEST_GRANULE) == VAL_ERROR);
    assert(val_mmu_ns_probe_build(&g_test_tables, TEST_IMAGE_BASE,
        TEST_IMAGE_BASE + (UINT64_C(1) << 21) + PAGE_SIZE, TEST_GRANULE) == VAL_ERROR);
    assert(val_mmu_ns_probe_build(&g_test_tables, VAL_NS_PROBE_ALIAS,
        VAL_NS_PROBE_ALIAS + PAGE_SIZE, TEST_GRANULE) == VAL_ERROR);
    assert(val_mmu_ns_probe_build(&g_test_tables, TEST_IMAGE_BASE,
        TEST_IMAGE_END, UINT64_C(1) << 48) == VAL_ERROR);
    assert(val_mmu_ns_probe_build(&g_test_tables, TEST_IMAGE_BASE,
                                  TEST_IMAGE_END, TEST_GRANULE + 1U) == VAL_ERROR);
}

/** @brief Do not share fault-handler suppression across PEs or unknown affinities. */
static void test_scope_visibility(void)
{
    g_ns_probe_active[0] = VAL_TRUE;
    g_test_pe = 0U;
    assert(val_mmu_ns_probe_active() == VAL_TRUE);
    g_test_pe = PAL_PE_COUNT;
    assert(val_mmu_ns_probe_active() == VAL_FALSE);
    g_ns_probe_active[0] = VAL_FALSE;
    g_test_pe = 0U;
    assert(val_mmu_ns_probe_active() == VAL_FALSE);
}

/** @brief Keep shared boot tables intact while mapping high DRAM on any known PE. */
static void test_native_scope(void)
{
    uint64_t value = 0ULL;
    val_fault_info_t fault = {0};

    g_val_mmu_l1[2] = make_block_desc(UINT64_C(0x80000000), DESC_ATTRINDX(1), 0);
    assert(val_mmu_probe_native_read64_el2(UINT64_C(0x900000000), 0ULL, 0ULL,
                                          &value, &fault) == VAL_SUCCESS);
    assert(value == UINT64_C(0xabc) && !fault.faulted && !val_mmu_ns_probe_active());
    assert(g_val_mmu_l1[VAL_NS_PROBE_ALIAS >> 30] == 0ULL);
    g_test_pe = PAL_PE_COUNT - 1U;
    g_test_fault = VAL_TRUE;
    assert(val_mmu_probe_native_read64_el2(UINT64_C(0x900000000), 0ULL, 0ULL,
                                          &value, &fault) == VAL_SUCCESS);
    assert(fault.faulted && val_fault_is_gpf(fault.esr) && !val_mmu_ns_probe_active());
    g_test_fault = VAL_FALSE;
    g_test_scope_error = VAL_TRUE;
    assert(val_mmu_probe_native_read64_el2(UINT64_C(0x900000000), 0ULL, 0ULL,
                                          &value, &fault) == VAL_ERROR);
    assert(!fault.faulted && !val_mmu_ns_probe_active());
    g_test_scope_error = VAL_FALSE;
    g_alias_active = VAL_TRUE;
    assert(val_mmu_probe_native_read64_el2(UINT64_C(0x900000000), 0ULL, 0ULL,
                                          &value, &fault) == VAL_ERROR);
    g_alias_active = VAL_FALSE;
    g_val_mmu_l1[VAL_NS_PROBE_ALIAS >> 30] = DESC_TYPE_BLOCK;
    assert(val_mmu_probe_native_read64_el2(UINT64_C(0x900000000), 0ULL, 0ULL,
                                          &value, &fault) == VAL_ERROR);
    g_val_mmu_l1[VAL_NS_PROBE_ALIAS >> 30] = 0ULL;
    g_test_tcr = 0ULL;
    assert(val_mmu_probe_native_read64_el2(UINT64_C(0x900000000), 0ULL, 0ULL,
                                          &value, &fault) == VAL_ERROR);
    g_test_tcr = VAL_MMU_EL2_BOOT_TCR;
    g_test_parange = UINT64_C(0xf);
    assert(val_mmu_probe_native_read64_el2(UINT64_C(0x900000000), 0ULL, 0ULL,
                                          &value, &fault) == VAL_ERROR);
    g_test_parange = UINT64_C(2);
    g_test_pe = PAL_PE_COUNT;
    assert(val_mmu_probe_native_read64_el2(UINT64_C(0x900000000), 0ULL, 0ULL,
                                          &value, &fault) == VAL_ERROR);
    g_test_pe = 0U;
}

/** @brief Run translation geometry and per-PE recovery-scope checks natively. */
int main(void)
{
    test_tables();
    test_invalid_geometry();
    test_scope_visibility();
    test_native_scope();
    puts("NS-PAS probe table/scope native tests: PASS");
    return VAL_SUCCESS;
}
