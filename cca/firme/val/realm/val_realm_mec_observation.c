/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_mec_observation.h"

#include "aarch64/sysreg.h"
#include "pal_platform.h"
#include "val_common/val_fault.h"
#include "val_common/val_mmu.h"
#include "val_common/val_status.h"
#include "val_firme_abi.h"
#include "val_firme_mec.h"

#define MEC_TEST_PAGE_SIZE        UINT64_C(4096)
#define MEC_TEST_PLAINTEXT        UINT64_C(0xA5C33CA55AC39669)
#define MEC_TEST_CTR_DMIN_SHIFT   16U
#define MEC_TEST_CTR_DMIN_MASK    UINT64_C(0xF)
#define MEC_TEST_WORD_SIZE        UINT64_C(8)
#define MEC_TEST_TCR2_AMEC0       (UINT64_C(1) << 12)
#define MEC_TEST_SCTLR2_EMEC      (UINT64_C(1) << 1)

/**
 *   @brief    - Reset an indirect MEC observation record.
 *   @param    - evidence : Evidence record to reset.
 *   @return   - void.
 **/
static void mec_observation_clear(
    val_firme_mec_observation_evidence_t *evidence)
{
    uint32_t index = 0U;

    evidence->abi_status = (int64_t)FIRME_NOT_SUPPORTED;
    evidence->mecid = 0ULL;
    evidence->test_pa = 0ULL;
    evidence->plaintext = 0ULL;
    evidence->setup_fault_esr = 0ULL;
    evidence->cleanup_status = (int64_t)FIRME_NOT_SUPPORTED;
    evidence->stale_read_completed_mask = 0U;
    evidence->stale_read_fault_mask = 0U;
    evidence->round_trip_completed_mask = 0U;
    evidence->setup_completed = VAL_FALSE;
    evidence->cache_maintenance_completed = VAL_FALSE;
    evidence->sample_count = 0U;
    evidence->valid_fields = 0U;

    for (index = 0U;
         index < VAL_FIRME_MEC_OBSERVATION_MAX_SAMPLES; index++) {
        evidence->stale_read[index] = 0ULL;
        evidence->round_trip[index] = 0ULL;
        evidence->stale_read_fault_esr[index] = 0ULL;
        evidence->round_trip_fault_esr[index] = 0ULL;
    }
}

/**
 *   @brief    - Fill one cache line through the alternate-MECID mapping.
 *   @param    - va        : Virtual address of the cache line.
 *   @param    - line_size : Cache-line size in bytes.
 *   @param    - value     : Repeated 64-bit plaintext value.
 *   @param    - fault_esr : Returned ESR if a store faults.
 *   @return   - VAL_SUCCESS on success, otherwise VAL_ERROR.
 **/
static int mec_write_pattern_line(uint64_t va, uint64_t line_size,
                                  uint64_t value, uint64_t *fault_esr)
{
    val_fault_info_t fault = {0};
    uint64_t offset = 0ULL;
    int result = VAL_ERROR;

    if (fault_esr == 0)
        return VAL_ERROR;

    *fault_esr = 0ULL;
    for (offset = 0ULL; offset < line_size; offset += MEC_TEST_WORD_SIZE) {
        result = val_probe_write64(va + offset, value, &fault);
        if (result != VAL_SUCCESS) {
            *fault_esr = fault.esr;
            return VAL_ERROR;
        }
    }

    return VAL_SUCCESS;
}

/**
 *   @brief    - Invoke FIRME_MEC_REFRESH and return its signed status.
 *   @param    - mecid  : MECID to refresh.
 *   @param    - reason : Architected refresh reason.
 *   @return   - Signed FIRME status returned by the DUT.
 **/
static int64_t mec_refresh(uint64_t mecid, uint64_t reason)
{
    firme_res_t result = {0};
    uint64_t params = 0ULL;

    params = val_firme_mec_params(mecid, reason);
    val_firme_mec_refresh_do_retry(params, &result);
    return val_firme_status_to_i64(result.x0);
}

/**
 *   @brief    - Report Realm support for indirect MEC refresh observation.
 *   @return   - VAL_TRUE because the Realm payload owns the required adapter.
 **/
uint32_t val_firme_mec_observation_probe_available(void)
{
    return VAL_TRUE;
}

/**
 *   @brief    - Exercise a same-MECID refresh using indirect memory evidence.
 *   @param    - test_case : MEC semantic scenario to exercise.
 *   @param    - evidence  : Returned refresh and memory-observation evidence.
 *   @return   - VAL_FIRME_MEC_OBSERVATION_PROBE_*.
 **/
int val_firme_mec_observation_probe(
    val_firme_mec_observation_case_t test_case,
    val_firme_mec_observation_evidence_t *evidence)
{
    firme_res_t service_result = {0};
    val_fault_info_t fault = {0};
    uint64_t original_tcr2 = 0ULL;
    uint64_t original_sctlr2 = 0ULL;
    uint64_t original_mecid_a0 = 0ULL;
    uint64_t test_pa = (uint64_t)(uintptr_t)__mec_test_pa__;
    uint64_t test_va = PLAT_FIRME_MEC_TEST_VA;
    uint64_t test_mecid = PLAT_FIRME_MEC_TEST_MECID;
    uint64_t line_size = 0ULL;
    uint64_t sample_va = 0ULL;
    uint64_t sample_pa = 0ULL;
    uint64_t max_mecid = 0ULL;
    uint64_t value = 0ULL;
    uint64_t fault_esr = 0ULL;
    uint32_t common_width = 0U;
    uint32_t index = 0U;
    uint32_t mapping_active = VAL_FALSE;
    uint32_t context_created = VAL_FALSE;
    uint32_t registers_saved = VAL_FALSE;
    int probe_result = VAL_FIRME_MEC_OBSERVATION_PROBE_ERROR;
    int access_result = VAL_ERROR;
    int unmap_result = VAL_ERROR;

    if ((evidence == 0) ||
        ((test_case != VAL_FIRME_MEC_OBSERVE_CONTEXT_REFRESH) &&
         (test_case != VAL_FIRME_MEC_OBSERVE_CIPHERTEXT_DECORRELATION)))
        return VAL_FIRME_MEC_OBSERVATION_PROBE_ERROR;

    mec_observation_clear(evidence);
    evidence->mecid = test_mecid;
    evidence->test_pa = test_pa;
    evidence->plaintext = MEC_TEST_PLAINTEXT;
    evidence->valid_fields = VAL_FIRME_MEC_EVIDENCE_MECID |
                             VAL_FIRME_MEC_EVIDENCE_TEST_ADDRESS |
                             VAL_FIRME_MEC_EVIDENCE_PLAINTEXT;

    /* Step 1: Run only when the Realm MEC service is implemented. */
    val_firme_service_version(FIRME_SERVICE_ID_MECID_MGMT, &service_result);
    if (val_firme_status_to_i64(service_result.x0) ==
        (int64_t)FIRME_NOT_SUPPORTED)
        return VAL_FIRME_MEC_OBSERVATION_PROBE_UNAVAILABLE;
    if (val_firme_status_to_i64(service_result.x0) < 0) {
        evidence->abi_status = val_firme_status_to_i64(service_result.x0);
        evidence->valid_fields |= VAL_FIRME_MEC_EVIDENCE_STATUS;
        return VAL_FIRME_MEC_OBSERVATION_PROBE_AVAILABLE;
    }

    /* Step 2: Ensure the platform-reserved MECID fits the advertised width. */
    if (val_firme_mec_advertised_common_mecid_width(
            &common_width, &service_result) != VAL_SUCCESS) {
        evidence->abi_status = val_firme_status_to_i64(service_result.x0);
        evidence->valid_fields |= VAL_FIRME_MEC_EVIDENCE_STATUS;
        return VAL_FIRME_MEC_OBSERVATION_PROBE_AVAILABLE;
    }
    max_mecid = (UINT64_C(1) << common_width) - 1ULL;
    if (test_mecid > max_mecid)
        return VAL_FIRME_MEC_OBSERVATION_PROBE_UNAVAILABLE;

    /* Step 3: Derive independent cache-line samples within the owned page. */
    line_size = UINT64_C(4) <<
                ((read_ctr_el0() >> MEC_TEST_CTR_DMIN_SHIFT) &
                 MEC_TEST_CTR_DMIN_MASK);
    if ((line_size < MEC_TEST_WORD_SIZE) ||
        (line_size > MEC_TEST_PAGE_SIZE))
        return VAL_FIRME_MEC_OBSERVATION_PROBE_ERROR;

    evidence->sample_count = (uint32_t)(MEC_TEST_PAGE_SIZE / line_size);
    if (evidence->sample_count > VAL_FIRME_MEC_OBSERVATION_MAX_SAMPLES)
        evidence->sample_count = VAL_FIRME_MEC_OBSERVATION_MAX_SAMPLES;
    if (evidence->sample_count == 0U)
        return VAL_FIRME_MEC_OBSERVATION_PROBE_ERROR;
    evidence->valid_fields |= VAL_FIRME_MEC_EVIDENCE_SAMPLE_COUNT;

    /* Step 4: Establish the pre-refresh context for the reserved MECID. */
    evidence->abi_status = mec_refresh(
        test_mecid, FIRME_MEC_REFRESH_REASON_CREATE);
    evidence->valid_fields |= VAL_FIRME_MEC_EVIDENCE_STATUS;
    probe_result = VAL_FIRME_MEC_OBSERVATION_PROBE_AVAILABLE;
    if (evidence->abi_status != (int64_t)FIRME_SUCCESS)
        goto cleanup;
    context_created = VAL_TRUE;

    /* Step 5: Install one alternate-MECID mapping to the reserved page. */
    original_tcr2 = read_tcr2_el2();
    original_sctlr2 = read_sctlr2_el2();
    original_mecid_a0 = read_mecid_a0_el2();
    registers_saved = VAL_TRUE;
    if (val_mmu_map_alternate_mecid_page_el2(test_va, test_pa) !=
        VAL_SUCCESS) {
        probe_result = VAL_FIRME_MEC_OBSERVATION_PROBE_ERROR;
        goto cleanup;
    }
    mapping_active = VAL_TRUE;

    write_mecid_a0_el2(test_mecid);
    write_sctlr2_el2(original_sctlr2 | MEC_TEST_SCTLR2_EMEC);
    write_tcr2_el2(original_tcr2 | MEC_TEST_TCR2_AMEC0);
    val_tlbi_all_el2();

    /* Step 6: Store equal plaintext samples under the old MEC context. */
    for (index = 0U; index < evidence->sample_count; index++) {
        sample_va = test_va + ((uint64_t)index * line_size);
        sample_pa = test_pa + ((uint64_t)index * line_size);
        access_result = mec_write_pattern_line(
            sample_va, line_size, MEC_TEST_PLAINTEXT, &fault_esr);
        if (access_result != VAL_SUCCESS) {
            evidence->setup_fault_esr = fault_esr;
            goto cleanup;
        }
        val_dcache_clean_invalidate_poe_realm(sample_pa);
    }
    evidence->setup_completed = VAL_TRUE;
    evidence->cache_maintenance_completed = VAL_TRUE;
    evidence->valid_fields |= VAL_FIRME_MEC_EVIDENCE_CACHE_MAINTENANCE;

    /* Step 7: Refresh the same MECID to create the replacement context. */
    evidence->abi_status = mec_refresh(
        test_mecid, FIRME_MEC_REFRESH_REASON_CREATE);
    if (evidence->abi_status != (int64_t)FIRME_SUCCESS)
        goto cleanup;

    /* Step 8: Read old memory through the replacement context. */
    for (index = 0U; index < evidence->sample_count; index++) {
        sample_va = test_va + ((uint64_t)index * line_size);
        access_result = val_probe_read64(sample_va, &value, &fault);
        if (access_result == VAL_SUCCESS) {
            evidence->stale_read[index] = value;
            evidence->stale_read_completed_mask |= UINT32_C(1) << index;
        } else {
            evidence->stale_read_fault_esr[index] = fault.esr;
            evidence->stale_read_fault_mask |= UINT32_C(1) << index;
        }
    }
    evidence->valid_fields |= VAL_FIRME_MEC_EVIDENCE_STALE_READ;

    /* Step 9: Rewrite and verify the same plaintext under the new context. */
    for (index = 0U; index < evidence->sample_count; index++) {
        sample_va = test_va + ((uint64_t)index * line_size);
        sample_pa = test_pa + ((uint64_t)index * line_size);
        access_result = mec_write_pattern_line(
            sample_va, line_size, MEC_TEST_PLAINTEXT, &fault_esr);
        if (access_result != VAL_SUCCESS) {
            evidence->round_trip_fault_esr[index] = fault_esr;
            continue;
        }

        val_dcache_clean_invalidate_poe_realm(sample_pa);
        access_result = val_probe_read64(sample_va, &value, &fault);
        if (access_result == VAL_SUCCESS) {
            evidence->round_trip[index] = value;
            evidence->round_trip_completed_mask |= UINT32_C(1) << index;
        } else {
            evidence->round_trip_fault_esr[index] = fault.esr;
        }
    }
    evidence->valid_fields |= VAL_FIRME_MEC_EVIDENCE_ROUND_TRIP;

cleanup:
    /* Step 10: Evict new-context data before retiring the test MECID. */
    if ((mapping_active == VAL_TRUE) &&
        (evidence->setup_completed == VAL_TRUE)) {
        for (index = 0U; index < evidence->sample_count; index++) {
            sample_pa = test_pa + ((uint64_t)index * line_size);
            val_dcache_clean_invalidate_poe_realm(sample_pa);
        }
    }

    if (context_created == VAL_TRUE) {
        evidence->cleanup_status = mec_refresh(
            test_mecid, FIRME_MEC_REFRESH_REASON_DESTROY);
    }

    if (mapping_active == VAL_TRUE) {
        unmap_result = val_mmu_unmap_alternate_mecid_page_el2(test_va);
        if (unmap_result != VAL_SUCCESS)
            probe_result = VAL_FIRME_MEC_OBSERVATION_PROBE_ERROR;
    }

    if (registers_saved == VAL_TRUE) {
        write_tcr2_el2(original_tcr2);
        write_sctlr2_el2(original_sctlr2);
        write_mecid_a0_el2(original_mecid_a0);
        val_tlbi_all_el2();
    }

    return probe_result;
}
