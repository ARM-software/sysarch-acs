/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_common/val_mmu.h"

#include "aarch64/sysreg.h"
#include "val_common/val_status.h"
#include "val_common/val_mp.h"

/*
 * A very small identity-mapping MMU setup:
 * - 4KB granule, 39-bit VA space (TTBR0 points at an L1 table).
 * - L1 entries are 1GiB blocks.
 *
 * This is sufficient for early bring-up and simple tests. It is not meant to
 * replace a full xlat-tables implementation.
 */

#define PAGE_SIZE 4096UL
#define L1_ENTRIES 512U
#define L2_ENTRIES 512U
#define L3_ENTRIES 512U

/* Descriptor bits (Block at level 1). */
#define DESC_TYPE_BLOCK 0x1UL
#define DESC_TYPE_TABLE 0x3UL
#define DESC_TYPE_PAGE  0x3UL
#define DESC_AF (1UL << 10)
#define DESC_AMEC (UINT64_C(1) << 63)
#define DESC_ADDRESS_MASK UINT64_C(0x0000FFFFFFFFF000)

/* AttrIndx in [4:2]. */
#define DESC_ATTRINDX_SHIFT 2
#define DESC_ATTRINDX(n) ((uint64_t)(n) << DESC_ATTRINDX_SHIFT)

/* Shareability bits [9:8]. */
#define DESC_SH_SHIFT 8
#define DESC_SH_IS (3UL << DESC_SH_SHIFT)

/* Access permissions bits [7:6]. */
#define DESC_AP_SHIFT 6
#define DESC_AP_RW (0UL << DESC_AP_SHIFT) /* RW at ELx */

/* Execute-never bits. */
#define DESC_PXN (1UL << 53)
#define DESC_UXN (1UL << 54)

/* MAIR encodings. */
#define MAIR_ATTR_DEVICE_nGnRE 0x04UL
#define MAIR_ATTR_NORMAL_WBWA  0xFFUL
#define DESC_NS (UINT64_C(1) << 5)
#define VAL_NS_PROBE_ALIAS (UINT64_C(1) << 38)
#define VAL_NS_PROBE_TCR_RES1 ((UINT64_C(1) << 31) | (UINT64_C(1) << 23))
#define VAL_NS_PROBE_TCR_NC (UINT64_C(25) | (UINT64_C(3) << 12))

typedef struct {
    uint64_t root[L1_ENTRIES];
    uint64_t image_l2[L2_ENTRIES];
    uint64_t image_l3[L3_ENTRIES];
    uint64_t alias_l2[L2_ENTRIES];
    uint64_t alias_l3[L3_ENTRIES];
} val_ns_probe_tables_t;

static _Alignas(PAGE_SIZE) val_ns_probe_tables_t g_ns_probe_tables[PAL_PE_COUNT];
static uint32_t g_ns_probe_active[PAL_PE_COUNT];

int val_mmu_ns_read_scope_el2(uint64_t root, uint64_t tcr, uint64_t address,
                              uint64_t *value, val_fault_info_t *fault);
int val_mmu_native_read_scope_el2(uint64_t root, uint64_t tcr, uint64_t address,
                                  uint64_t *value, val_fault_info_t *fault,
                                  uint64_t expected_root);

_Alignas(PAGE_SIZE) uint64_t g_val_mmu_l1[L1_ENTRIES];
static _Alignas(PAGE_SIZE) uint64_t g_alias_l2[L2_ENTRIES];
static _Alignas(PAGE_SIZE) uint64_t g_alias_l3[L3_ENTRIES];
static uint64_t g_alias_va;
static uint32_t g_alias_active;
static uint64_t g_device_base;
static uint64_t g_device_tcr;
static uint32_t g_device_active;
static uint32_t g_mmu_controller_pe = PAL_PE_COUNT;

#define VAL_TCR_PS_SHIFT 16U
#define VAL_TCR_PS_MASK (UINT64_C(7) << VAL_TCR_PS_SHIFT)
#define VAL_TCR_PS_36 (UINT64_C(1) << VAL_TCR_PS_SHIFT)

/**
 *   @brief    - Build a 1GiB block descriptor for an identity mapping.
 *   @param    - pa   : Physical address (1GiB aligned).
 *   @param    - attr : AttrIndx/attributes bits for the block.
 *   @param    - xn   : Non-zero to mark the mapping execute-never.
 *   @return   - Descriptor value.
**/
static inline uint64_t make_block_desc(uint64_t pa, uint64_t attr, int xn)
{
    uint64_t descriptor = 0ULL;

    descriptor |= (pa & ~((1UL << 30) - 1)); /* 1GiB aligned */
    descriptor |= DESC_TYPE_BLOCK;
    descriptor |= DESC_AF;
    descriptor |= DESC_SH_IS;
    descriptor |= DESC_AP_RW;
    descriptor |= attr;
    if (xn) {
        descriptor |= DESC_PXN | DESC_UXN;
    }
    return descriptor;
}

/**
 *   @brief    - Build a normal-memory page descriptor using the alternate MECID.
 *   @param    - pa : Page-aligned physical address.
 *   @return   - Descriptor value.
 **/
static uint64_t make_amec_page_desc(uint64_t pa)
{
    uint64_t descriptor = 0ULL;

    descriptor = (pa & DESC_ADDRESS_MASK) | DESC_TYPE_PAGE | DESC_AF |
                 DESC_SH_IS | DESC_AP_RW | DESC_ATTRINDX(1) | DESC_PXN |
                 DESC_UXN | DESC_AMEC;
    return descriptor;
}

/**
 *   @brief    - Build a next-level translation-table descriptor.
 *   @param    - table : Page-aligned translation-table address.
 *   @return   - Descriptor value.
 **/
static uint64_t make_table_desc(const uint64_t *table)
{
    return ((uint64_t)(uintptr_t)table & DESC_ADDRESS_MASK) |
           DESC_TYPE_TABLE;
}

/**
 *   @brief    - Program EL2 MMU registers for TTBR0-based translation.
 *   @return   - void
**/
static void program_el2(void)
{
    const uint64_t tcr = VAL_MMU_EL2_BOOT_TCR;

    /*
     * MAIR_EL2:
     *  - Attr0: Device-nGnRE
     *  - Attr1: Normal WBWA
     */
    write_mair_el2(VAL_MMU_BOOT_MAIR);

    /*
     * TCR_EL2: 4KB granule, 39-bit VA space.
     *  - T0SZ = 25
     *  - IRGN0/ORGN0 = WBWA (01)
     *  - SH0 = Inner Shareable (11)
     */
    write_tcr_el2(tcr);

    write_ttbr0_el2((uint64_t)(uintptr_t)g_val_mmu_l1);
    dsb_sy();
    isb_sy();
}

/**
 *   @brief    - Program EL1 MMU registers for TTBR0-based translation.
 *   @return   - void
**/
static void program_el1(void)
{
    const uint64_t tcr =
        (25UL << 0) |        /* T0SZ */
        (1UL << 8) |         /* IRGN0 */
        (1UL << 10) |        /* ORGN0 */
        (3UL << 12) |        /* SH0 */
        (0UL << 14);         /* TG0=4KB */

    write_mair_el1((MAIR_ATTR_DEVICE_nGnRE << 0) | (MAIR_ATTR_NORMAL_WBWA << 8));

    write_tcr_el1(tcr);

    write_ttbr0_el1((uint64_t)(uintptr_t)g_val_mmu_l1);
    dsb_sy();
    isb_sy();
}

/**
 *   @brief    - Create a minimal identity map using 1GiB blocks.
 *   @param    - el        : EL to program (EL2 or EL1).
 *   @param    - dram_base : DRAM base address.
 *   @param    - dram_size : DRAM size in bytes.
 *   @return   - void
**/
void val_mmu_setup_identity_1g(mmu_el_t el, uint64_t dram_base, uint64_t dram_size)
{
    const unsigned max_gib = 4;
    uint64_t dram_end = 0ULL;
    uint64_t pa = 0ULL;
    unsigned int index = 0U;

    g_mmu_controller_pe = val_pe_index();
    for (index = 0U; index < L1_ENTRIES; index++)
        g_val_mmu_l1[index] = 0ULL;
    for (index = 0U; index < L2_ENTRIES; index++)
        g_alias_l2[index] = 0ULL;
    for (index = 0U; index < L3_ENTRIES; index++)
        g_alias_l3[index] = 0ULL;
    g_alias_va = 0ULL;
    g_alias_active = 0U;

    /*
     * Map low 4GiB as Device (safe default for peripherals) and then override
     * the DRAM window as Normal memory.
     */
    for (index = 0U; index < max_gib; index++) {
        pa = (uint64_t)index << 30;
        g_val_mmu_l1[index] = make_block_desc(pa, DESC_ATTRINDX(0), 1);
    }

    dram_end = dram_base + dram_size;
    pa = dram_base & ~((1UL << 30) - 1);
    for (; pa < dram_end; pa += (1UL << 30)) {
        index = (unsigned int)(pa >> 30);
        g_val_mmu_l1[index] = make_block_desc(pa, DESC_ATTRINDX(1), 0);
    }

    if (el == MMU_EL2) {
        program_el2();
    } else {
        program_el1();
    }
}

/**
 *   @brief    - Enable the MMU and caches for the given EL.
 *   @param    - el : EL to enable MMU for (EL2 or EL1).
 *   @return   - void
**/
void val_mmu_enable(mmu_el_t el)
{
    if (el == MMU_EL2) {
        uint64_t sctlr = read_sctlr_el2();
        sctlr |= (1UL << 0);  /* M */
        sctlr |= (1UL << 2);  /* C */
        sctlr |= (1UL << 12); /* I */
        write_sctlr_el2(sctlr);
    } else {
        uint64_t sctlr = read_sctlr_el1();
        sctlr |= (1UL << 0);  /* M */
        sctlr |= (1UL << 2);  /* C */
        sctlr |= (1UL << 12); /* I */
        write_sctlr_el1(sctlr);
    }
    dsb_sy();
    isb_sy();
}

/** @brief Publish boot tables before a cache-disabled secondary uses them. */
void val_mmu_publish_tables(void)
{
    val_cache_clean_poc((uintptr_t)&g_mmu_controller_pe, sizeof(g_mmu_controller_pe));
    val_cache_clean_poc((uintptr_t)g_val_mmu_l1, sizeof(g_val_mmu_l1));
    val_cache_clean_poc((uintptr_t)g_alias_l2, sizeof(g_alias_l2));
    val_cache_clean_poc((uintptr_t)g_alias_l3, sizeof(g_alias_l3));
}

/**
 * @brief Map an unused 1-GiB EL2 slot as non-executable Device memory.
 * @param base Aligned identity-mapped address below 64 GiB, on the primary PE.
 * @return VAL_SUCCESS on success; never replaces an existing mapping.
 */
int val_mmu_map_device_block_el2(uint64_t base)
{
    uint32_t index;
    uint64_t tcr;
    uint64_t physical_range;

    if (!val_mmu_is_controller_pe() || g_device_active || g_alias_active ||
        (base & ((UINT64_C(1) << 30) - 1U)) != 0ULL || (base >> 36) != 0ULL)
        return VAL_ERROR;
    index = (uint32_t)(base >> 30);
    if (g_val_mmu_l1[index] != 0ULL)
        return VAL_ERROR;
    tcr = val_read_tcr_el2();
    physical_range = val_read_id_aa64mmfr0_el1() & UINT64_C(0xf);
    if (base >= (UINT64_C(1) << 32) &&
        (physical_range == 0ULL || physical_range > 7ULL))
        return VAL_ERROR;
    g_device_tcr = tcr;
    g_device_base = base;
    g_device_active = VAL_TRUE;
    if (base >= (UINT64_C(1) << 32) && (tcr & VAL_TCR_PS_MASK) == 0ULL) {
        dsb_sy();
        write_tcr_el2(tcr | VAL_TCR_PS_36);
    }

    g_val_mmu_l1[index] = make_block_desc(base, DESC_ATTRINDX(0), 1);
    val_cache_clean_poc((uintptr_t)&g_val_mmu_l1[index], sizeof(uint64_t));
    val_tlbi_all_el2();
    return VAL_SUCCESS;
}

/**
 * @brief Remove only a matching Device mapping created for a temporary fixture.
 * @param base Address previously passed to the mapping helper.
 * @return VAL_SUCCESS after publishing the invalid descriptor and invalidating TLBs.
 */
int val_mmu_unmap_device_block_el2(uint64_t base)
{
    uint32_t index;

    if (!val_mmu_is_controller_pe() || !g_device_active || base != g_device_base)
        return VAL_ERROR;
    index = (uint32_t)(base >> 30);
    if (g_val_mmu_l1[index] != make_block_desc(base, DESC_ATTRINDX(0), 1))
        return VAL_ERROR;
    g_val_mmu_l1[index] = 0ULL;
    val_cache_clean_poc((uintptr_t)&g_val_mmu_l1[index], sizeof(uint64_t));
    val_tlbi_all_el2();
    write_tcr_el2(g_device_tcr);
    val_tlbi_all_el2();
    g_device_active = VAL_FALSE;
    return VAL_SUCCESS;
}

/**
 * @brief Identify the PE that constructed this world's shared boot tables.
 * @return VAL_TRUE only for the known controller PE after cold initialization.
 */
uint32_t val_mmu_is_controller_pe(void)
{
    return (g_mmu_controller_pe < PAL_PE_COUNT) &&
           (val_pe_index() == g_mmu_controller_pe);
}

/**
 *   @brief    - Map one EL2 page through the alternate MECID path.
 *   @param    - va : Page-aligned virtual address.
 *   @param    - pa : Page-aligned Realm physical address.
 *   @return   - VAL_SUCCESS on success, otherwise VAL_ERROR.
 **/
int val_mmu_map_alternate_mecid_page_el2(uint64_t va, uint64_t pa)
{
    uint32_t l1_index = 0U;
    uint32_t l2_index = 0U;
    uint32_t l3_index = 0U;

    if (((va | pa) & (PAGE_SIZE - 1UL)) != 0ULL)
        return VAL_ERROR;
    if ((va >> 39) != 0ULL)
        return VAL_ERROR;
    if (g_alias_active != 0U)
        return VAL_ERROR;

    l1_index = (uint32_t)((va >> 30) & UINT64_C(0x1FF));
    l2_index = (uint32_t)((va >> 21) & UINT64_C(0x1FF));
    l3_index = (uint32_t)((va >> 12) & UINT64_C(0x1FF));
    if (g_val_mmu_l1[l1_index] != 0ULL)
        return VAL_ERROR;

    g_alias_l3[l3_index] = make_amec_page_desc(pa);
    g_alias_l2[l2_index] = make_table_desc(g_alias_l3);
    dsb_sy();
    g_val_mmu_l1[l1_index] = make_table_desc(g_alias_l2);
    val_tlbi_all_el2();

    g_alias_va = va;
    g_alias_active = 1U;
    return VAL_SUCCESS;
}

/**
 *   @brief    - Remove the active EL2 alternate-MECID page mapping.
 *   @param    - va : Virtual address passed to the map operation.
 *   @return   - VAL_SUCCESS on success, otherwise VAL_ERROR.
 **/
int val_mmu_unmap_alternate_mecid_page_el2(uint64_t va)
{
    uint32_t l1_index = 0U;
    uint32_t l2_index = 0U;
    uint32_t l3_index = 0U;

    if ((g_alias_active == 0U) || (va != g_alias_va))
        return VAL_ERROR;

    l1_index = (uint32_t)((va >> 30) & UINT64_C(0x1FF));
    l2_index = (uint32_t)((va >> 21) & UINT64_C(0x1FF));
    l3_index = (uint32_t)((va >> 12) & UINT64_C(0x1FF));

    g_alias_l3[l3_index] = 0ULL;
    dsb_sy();
    val_tlbi_all_el2();
    g_alias_l2[l2_index] = 0ULL;
    g_val_mmu_l1[l1_index] = 0ULL;
    dsb_sy();
    val_tlbi_all_el2();

    g_alias_va = 0ULL;
    g_alias_active = 0U;
    return VAL_SUCCESS;
}

/** @brief Report whether this PE is inside the temporary Secure NS-PAS mapping. */
uint32_t val_mmu_ns_probe_active(void)
{
    uint32_t pe_index = val_pe_index();

    return (pe_index < PAL_PE_COUNT) ? g_ns_probe_active[pe_index] : VAL_FALSE;
}

/**
 * @brief Build private tables mapping only the Secure image and one NS data page.
 * @return VAL_SUCCESS for a representable, non-overlapping mapping.
 */
static int val_mmu_ns_probe_build(val_ns_probe_tables_t *tables,
                                  uint64_t image_start, uint64_t image_end,
                                  uint64_t pa)
{
    uint64_t page;
    uint64_t descriptor;
    uint32_t index;
    uint32_t image_root;
    uint32_t image_l2;

    if (!tables || !image_start || image_start >= image_end ||
        (image_start & (PAGE_SIZE - 1U)) ||
        (image_end > VAL_NS_PROBE_ALIAS) ||
        ((image_start >> 21) != ((image_end - 1U) >> 21)) ||
        (pa >> 48) || (pa & (sizeof(uint64_t) - 1U)))
        return VAL_ERROR;

    image_root = (uint32_t)(image_start >> 30);
    image_l2 = (uint32_t)((image_start >> 21) & UINT64_C(0x1ff));
    for (index = 0U; index < L1_ENTRIES; index++) {
        tables->root[index] = 0ULL;
        tables->image_l2[index] = 0ULL;
        tables->image_l3[index] = 0ULL;
        tables->alias_l2[index] = 0ULL;
        tables->alias_l3[index] = 0ULL;
    }

    descriptor = DESC_TYPE_PAGE | DESC_AF | DESC_SH_IS | DESC_AP_RW;
    for (page = image_start; page < image_end; page += PAGE_SIZE)
        tables->image_l3[(page >> 12) & UINT64_C(0x1ff)] = page | descriptor;

    tables->image_l2[image_l2] = make_table_desc(tables->image_l3);
    tables->root[image_root] = make_table_desc(tables->image_l2);
    tables->alias_l3[0] = (pa & DESC_ADDRESS_MASK) | descriptor | DESC_NS |
                         DESC_PXN | DESC_UXN;
    tables->alias_l2[0] = make_table_desc(tables->alias_l3);
    tables->root[VAL_NS_PROBE_ALIAS >> 30] = make_table_desc(tables->alias_l2);
    return VAL_SUCCESS;
}

/** @brief Probe with a private image map when the caller's MMU and cache are disabled. */
static int val_mmu_probe_uncached_el2(uint64_t pa, uint64_t image_start,
                                      uint64_t image_end, uint64_t *value,
                                      val_fault_info_t *fault, uint32_t nonsecure)
{
    uint32_t pe_index = val_pe_index();
    uint64_t physical_range;
    uint64_t tcr;
    uintptr_t table_base;
    val_ns_probe_tables_t *tables;
    int probe_status;

    if (!value || !fault || pe_index >= PAL_PE_COUNT)
        return VAL_ERROR;
    *value = 0ULL;
    *fault = (val_fault_info_t){0};
    if (g_ns_probe_active[pe_index] || read_currentel() != UINT64_C(8) ||
        (read_sctlr_el2() & UINT64_C(5)))
        return VAL_ERROR;

    tables = &g_ns_probe_tables[pe_index];
    table_base = (uintptr_t)tables;
    if (image_end <= image_start || image_end - image_start < sizeof(*tables) ||
        table_base < image_start || table_base > image_end - sizeof(*tables) ||
        (uintptr_t)value < image_start || (uintptr_t)value > image_end - sizeof(*value) ||
        (uintptr_t)fault < image_start || (uintptr_t)fault > image_end - sizeof(*fault))
        return VAL_ERROR;

    physical_range = val_read_id_aa64mmfr0_el1() & UINT64_C(0xf);
    if (physical_range > UINT64_C(6))
        return VAL_ERROR;
    if (physical_range > UINT64_C(5))
        physical_range = UINT64_C(5);
    if (val_mmu_ns_probe_build(tables, image_start, image_end, pa) != VAL_SUCCESS)
        return VAL_ERROR;
    if (!nonsecure)
        tables->alias_l3[0] &= ~DESC_NS;

    tcr = VAL_NS_PROBE_TCR_NC | VAL_NS_PROBE_TCR_RES1 |
          (physical_range << VAL_TCR_PS_SHIFT);
    g_ns_probe_active[pe_index] = VAL_TRUE;
    dsb_sy();
    probe_status = val_mmu_ns_read_scope_el2(table_base, tcr,
        VAL_NS_PROBE_ALIAS | (pa & (PAGE_SIZE - 1U)), value, fault);
    g_ns_probe_active[pe_index] = VAL_FALSE;
    if (probe_status != VAL_SUCCESS && !fault->faulted)
        return VAL_ERROR;

    return VAL_SUCCESS;
}

/** @brief Perform a scoped S-EL2 NS-output load without changing shared boot tables. */
int val_mmu_probe_ns_read64_el2(uint64_t pa, uint64_t image_start,
                               uint64_t image_end, uint64_t *value,
                               val_fault_info_t *fault)
{
    return val_mmu_probe_uncached_el2(pa, image_start, image_end, value,
                                     fault, VAL_TRUE);
}

/**
 * @brief Map one native-PAS page privately, retaining active identity mappings and caches.
 * @return VAL_SUCCESS for load/fault evidence, VAL_ERROR if safe setup is unavailable.
 */
int val_mmu_probe_native_read64_el2(uint64_t pa, uint64_t image_start,
                                    uint64_t image_end, uint64_t *value,
                                    val_fault_info_t *fault)
{
    uint32_t pe_index = val_pe_index();
    uint32_t index;
    uint64_t physical_range;
    uint64_t tcr;
    val_ns_probe_tables_t *tables;
    int probe_status;

    if (!value || !fault || pe_index >= PAL_PE_COUNT ||
        read_currentel() != UINT64_C(8) || g_ns_probe_active[pe_index] ||
        (pa >> 48) || (pa & (sizeof(uint64_t) - 1U)))
        return VAL_ERROR;
    *value = 0ULL;
    *fault = (val_fault_info_t){0};
    if (!(read_sctlr_el2() & UINT64_C(1)))
        return val_mmu_probe_uncached_el2(pa, image_start, image_end, value,
                                         fault, VAL_FALSE);

    tcr = val_read_tcr_el2();
    if ((tcr & UINT64_C(0x3f)) != UINT64_C(25) ||
        (tcr & (UINT64_C(3) << 14)) || g_alias_active ||
        g_val_mmu_l1[VAL_NS_PROBE_ALIAS >> 30])
        return VAL_ERROR;
    physical_range = val_read_id_aa64mmfr0_el1() & UINT64_C(0xf);
    if (physical_range > UINT64_C(6))
        return VAL_ERROR;
    if (physical_range > UINT64_C(5))
        physical_range = UINT64_C(5);
    tables = &g_ns_probe_tables[pe_index];
    for (index = 0U; index < L1_ENTRIES; index++) {
        tables->root[index] = g_val_mmu_l1[index];
        tables->alias_l2[index] = 0ULL;
        tables->alias_l3[index] = 0ULL;
    }

    tables->alias_l3[0] = (pa & DESC_ADDRESS_MASK) | DESC_TYPE_PAGE | DESC_AF |
                         DESC_SH_IS | DESC_AP_RW | DESC_ATTRINDX(7) | DESC_PXN | DESC_UXN;
    tables->alias_l2[0] = make_table_desc(tables->alias_l3);
    tables->root[VAL_NS_PROBE_ALIAS >> 30] = make_table_desc(tables->alias_l2);
    val_cache_clean_poc((uintptr_t)tables, sizeof(*tables));
    tcr = (tcr & ~VAL_TCR_PS_MASK) | (physical_range << VAL_TCR_PS_SHIFT);
    g_ns_probe_active[pe_index] = VAL_TRUE;
    probe_status = val_mmu_native_read_scope_el2((uint64_t)(uintptr_t)tables, tcr,
        VAL_NS_PROBE_ALIAS | (pa & (PAGE_SIZE - 1U)), value, fault,
        (uint64_t)(uintptr_t)g_val_mmu_l1);
    g_ns_probe_active[pe_index] = VAL_FALSE;
    if (probe_status != VAL_SUCCESS && !fault->faulted)
        return VAL_ERROR;

    return VAL_SUCCESS;
}
