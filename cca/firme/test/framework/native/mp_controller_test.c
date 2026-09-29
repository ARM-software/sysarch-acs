/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>

#include "../../../val_common/src/val_mmu.c"
#include "../../../val/src/val_firme_ide_link.c"

#define REVIEW_DEVICE_BASE (UINT64_C(8) << 30)
#define REVIEW_CONTROLLER 1U
#define REVIEW_SECONDARY 0U

static uint32_t g_review_pe;
static uint32_t g_review_clean_count;
static uint32_t g_review_tlbi_count;
static uint64_t g_review_tcr;

/** @brief Simulate a boot PE at an arbitrary valid topology index. */
uint32_t val_pe_index(void)
{
    return g_review_pe;
}

/** @brief Record publication of modified descriptors. */
void val_cache_clean_poc(uintptr_t start, size_t size)
{
    assert(size != 0U);
    g_review_clean_count++;
}

/** @brief Record required TLB invalidations. */
void val_tlbi_all_el2(void)
{
    g_review_tlbi_count++;
}

/** @brief Supply an implementation with sufficient PA width. */
uint64_t val_read_id_aa64mmfr0_el1(void)
{
    return UINT64_C(2);
}

/** @brief Return the simulated translation control register. */
uint64_t val_read_tcr_el2(void)
{
    return g_review_tcr;
}

/** @brief Record translation control changes and restoration. */
void write_tcr_el2(uint64_t value)
{
    g_review_tcr = value;
}

/** @brief Replace hardware barriers in this native control-flow check. */
void dsb_sy(void) {}

/** @brief Replace hardware instruction synchronization in the native check. */
void isb_sy(void) {}

/** @brief Stub the unexercised EL1 boot path. */
void write_tcr_el1(uint64_t value) {}

/** @brief Stub memory attribute programming without changing the C path. */
void write_mair_el1(uint64_t value) {}

/** @brief Stub memory attribute programming without changing the C path. */
void write_mair_el2(uint64_t value) {}

/** @brief Stub the EL1 table register. */
void write_ttbr0_el1(uint64_t value) {}

/** @brief Stub the EL2 table register. */
void write_ttbr0_el2(uint64_t value) {}

/** @brief Decode synthetic architectural status values. */
int64_t val_firme_status_to_i64(uint64_t value)
{
    return (int64_t)value;
}

/** @brief Supply a service only if the controller guard permits discovery. */
void val_firme_service_version(uint64_t service, firme_res_t *result)
{
    *result = (firme_res_t){.x0 = UINT64_C(1)};
}

/** @brief Supply a stable fixture ECAM address. */
uint64_t val_pcie_ecam_base(void)
{
    return REVIEW_DEVICE_BASE;
}

/** @brief Resolve the single synthetic Root Port used by this ownership check. */
uint32_t val_pcie_bdf_from_requester_id(uint32_t segment, uint32_t requester_id)
{
    assert(segment == 0U && requester_id == 0U);
    return 0U;
}

/** @brief Return the ECAM aperture for the synthetic Root Port. */
int val_pcie_ecam_base_for_bdf(uint32_t bdf, uint64_t *ecam_base)
{
    assert(bdf == 0U && ecam_base);
    *ecam_base = REVIEW_DEVICE_BASE;
    return VAL_SUCCESS;
}

/** @brief Supply fixed flags for the dispatcher ownership test only. */
uint64_t val_firme_ide_flags(uint32_t ide_type)
{
    return ide_type;
}

/** @brief Supply fixed poll flags for the unused operation path. */
uint64_t val_firme_ide_poll_flags(uint32_t any_pending)
{
    return any_pending;
}

/** @brief Reject unexpected programming in the ownership-only fixture. */
void val_firme_ide_keyset_prog_do_retry(const val_firme_ide_prog_args_t *args,
    firme_res_t *result)
{
    assert(VAL_FALSE);
}

/** @brief Reject unexpected GO in the ownership-only fixture. */
void val_firme_ide_keyset_go_do_retry(uint64_t ecam, uint64_t flags,
    uint64_t keyset, uint64_t handle, firme_res_t *result)
{
    assert(VAL_FALSE);
}

/** @brief Reject unexpected STOP in the ownership-only fixture. */
void val_firme_ide_keyset_stop_do_retry(uint64_t ecam, uint64_t flags,
    uint64_t keyset, uint64_t handle, firme_res_t *result)
{
    assert(VAL_FALSE);
}

/** @brief Reject unexpected POLL in the ownership-only fixture. */
void val_firme_ide_keyset_poll_do_retry(uint64_t ecam, uint64_t flags,
    uint64_t keyset, firme_res_t *result)
{
    assert(VAL_FALSE);
}

/** @brief Exercise the production MMU and IDE guards for a nonzero boot index. */
int main(void)
{
    firme_acs_call_res_t result;
    uint64_t keyset = (uint64_t)VAL_FIRME_IDE_CXL_SUBSTREAM_ID <<
                     VAL_FIRME_IDE_KEYSET_SUBSTREAM_SHIFT;
    uint64_t initial_tcr;
    uint32_t clean_before;
    uint32_t tlbi_before;

    g_review_pe = REVIEW_CONTROLLER;
    assert(val_mmu_is_controller_pe() == VAL_FALSE);
    assert(val_mmu_map_device_block_el2(REVIEW_DEVICE_BASE) == VAL_ERROR);
    val_mmu_setup_identity_1g(MMU_EL2, UINT64_C(0x80000000), UINT64_C(0x100000));
    assert(val_mmu_is_controller_pe() == VAL_TRUE);
    initial_tcr = g_review_tcr;
    assert(val_mmu_map_device_block_el2(REVIEW_DEVICE_BASE) == VAL_SUCCESS);
    assert(g_review_clean_count > 0U && g_review_tlbi_count > 0U);
    assert(val_mmu_map_device_block_el2(REVIEW_DEVICE_BASE) == VAL_ERROR);
    val_firme_ide_link_command(VAL_IDE_LINK_BEGIN, keyset, REVIEW_DEVICE_BASE, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_PASS);

    clean_before = g_review_clean_count;
    tlbi_before = g_review_tlbi_count;
    g_review_pe = REVIEW_SECONDARY;
    assert(val_mmu_is_controller_pe() == VAL_FALSE);
    assert(val_mmu_unmap_device_block_el2(REVIEW_DEVICE_BASE) == VAL_ERROR);
    val_firme_ide_link_command(VAL_IDE_LINK_RELEASE, keyset, 0ULL, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_FAIL);
    assert(g_review_clean_count == clean_before && g_review_tlbi_count == tlbi_before);

    g_review_pe = PAL_PE_COUNT;
    assert(val_mmu_is_controller_pe() == VAL_FALSE);
    assert(val_mmu_unmap_device_block_el2(REVIEW_DEVICE_BASE) == VAL_ERROR);
    g_review_pe = REVIEW_CONTROLLER;
    val_firme_ide_link_command(VAL_IDE_LINK_RELEASE, keyset, 0ULL, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_PASS);
    assert(val_mmu_unmap_device_block_el2(REVIEW_DEVICE_BASE) == VAL_SUCCESS);
    assert(g_val_mmu_l1[REVIEW_DEVICE_BASE >> 30] == 0ULL && g_review_tcr == initial_tcr);
    assert(val_mmu_unmap_device_block_el2(REVIEW_DEVICE_BASE) == VAL_ERROR);
    g_review_pe = REVIEW_SECONDARY;
    assert(val_mmu_map_device_block_el2(REVIEW_DEVICE_BASE) == VAL_ERROR);
    val_firme_ide_link_command(VAL_IDE_LINK_BEGIN, keyset, REVIEW_DEVICE_BASE, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_FAIL);
    puts("MP controller review regression: PASS");
    return VAL_SUCCESS;
}
