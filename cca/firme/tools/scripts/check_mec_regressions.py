#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

"""Native MEC caller/evidence regressions, not hardware compliance tests."""

import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


SOURCE = r"""
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "aarch64/sysreg.h"
#include "pal_platform.h"
#include "val_common/val_fault.h"
#include "val_common/val_mmu.h"
#include "val_common/val_smccc.h"
#include "val_firme_mec.h"
#include "val_firme_mp_test.h"
#include "val_common/val_mp.h"
#include "val_iovirt.h"
#include "val_log.h"
#include "mec_semantic_test_common.h"

#define TEST_LINE_SIZE 64U
#define TEST_WORDS 512U
#define TEST_ABORT ((UINT64_C(0x25) << 26) | UINT64_C(0x10))

uint8_t __mec_test_pa__[4096];
static uint64_t g_words[TEST_WORDS];
static uint32_t g_generations[TEST_WORDS];
static uint32_t g_dirty[4096 / TEST_LINE_SIZE];
static uint32_t g_creates;
static uint32_t g_generation;
static uint32_t g_destroy_noop;
static uint32_t g_writes;
static uint32_t g_fail_write_at;
static uint32_t g_bad_baseline;
static uint32_t g_fault_baseline;
static uint32_t g_fault_stale;
static uint64_t g_stale_fault_esr;
static uint32_t g_stale_fault_count;
static uint32_t g_stale_fault_creates;
static uint32_t g_bad_round_trip;
static uint32_t g_no_refresh;
static uint32_t g_mapped;
static uint32_t g_destroy_dirty;
static uint32_t g_destroy_calls;
static int32_t g_destroy_status;
static int32_t g_refresh_status = FIRME_SUCCESS;
static uint64_t g_tcr2;
static uint64_t g_sctlr2;
static uint64_t g_mecid;
static uint64_t g_poison;
static uint32_t g_refresh_present = VAL_TRUE;
static uint32_t g_advertised_width = 4U;
static uint32_t g_pe_widths[PAL_PE_COUNT];
static uint32_t g_current_pe;
static uint32_t g_smmu_widths[2] = {6U, 5U};
static uint32_t g_smmu_reads;
static uint32_t g_mmio_fail;
static uint32_t g_duplicate_smmu;
static uint32_t g_use_evidence;
static val_firme_mec_observation_evidence_t g_evidence;

int val_realm_mec_observation_probe(val_firme_mec_observation_case_t test_case,
                                   val_firme_mec_observation_evidence_t *evidence);
int firme_mec_feature_registers_run_realm(uint64_t base_pa, firme_rule_res_t *out);
int firme_mec_refresh_abi_run_ns(uint64_t base_pa, firme_rule_res_t *out);
int firme_mec_refresh_abi_run_secure(uint64_t base_pa, firme_rule_res_t *out);
int firme_mec_refresh_abi_run_realm(uint64_t base_pa, firme_rule_res_t *out);
int firme_mec_context_refresh_run_realm(uint64_t base_pa, firme_rule_res_t *out);
int firme_mec_ciphertext_decorrelation_run_realm(uint64_t base_pa, firme_rule_res_t *out);
int firme_mec_common_mecid_width_payload(firme_acs_world_t world, uint32_t phase,
                                         uint32_t argument, firme_rule_res_t *out);
int firme_mec_common_mecid_width_entry(firme_acs_world_t world, uint32_t num_pe,
                                       val_test_run_payload_t run_payload,
                                       firme_rule_res_t *out);

/** @brief Suppress production diagnostics during expected-failure checks. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *format, ...)
{
    (void)verbosity;
    (void)tag;
    (void)format;
    return 0U;
}

/** @brief Provide the controller identity to production ABI wrappers. */
uint64_t read_mpidr_el1(void)
{
    return 0U;
}

/** @brief Model FIRME responses while checking unused wrapper inputs. */
void val_smccc_smc_call(uint64_t fid, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                        uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7,
                        smccc_res_t *result)
{
    uint32_t line_index;

    assert((arg3 | arg4 | arg5 | arg6 | arg7) == 0U);
    *result = (smccc_res_t){.x0 = FIRME_SUCCESS};
    if (fid == FIRME_FID_SERVICE_VERSION) {
        assert(arg1 == FIRME_SERVICE_ID_MECID_MGMT && arg2 == 0U);
        result->x0 = UINT64_C(0x10000);
    } else if (fid == FIRME_FID_SERVICE_FEATURES) {
        assert(arg1 == FIRME_SERVICE_ID_MECID_MGMT);
        if (arg2 == FIRME_MECID_FEATURE_REG0_INDEX)
            result->x1 = (g_refresh_present ? FIRME_MECID_FEATURE_REG0_MEC_REFRESH_BIT : 0U) |
                         (g_poison & ~FIRME_MECID_FEATURE_REG0_MEC_REFRESH_BIT);
        else if (arg2 == FIRME_MECID_FEATURE_REG1_INDEX)
            result->x1 = (g_advertised_width - 1U) |
                         (g_poison & ~FIRME_MECID_FEATURE_REG1_ALLOCATED_MASK);
        else
            result->x0 = (uint32_t)FIRME_NOT_SUPPORTED;
    } else {
        assert(fid == FIRME_FID_MEC_REFRESH && arg2 == 0U);
        if (g_refresh_status != FIRME_SUCCESS) {
            result->x0 = (uint32_t)g_refresh_status;
        } else if (((arg1 >> FIRME_MEC_REFRESH_MECID_SHIFT) & FIRME_MEC_REFRESH_MECID_MASK) >=
            (UINT64_C(1) << g_advertised_width)) {
            result->x0 = (uint32_t)FIRME_INVALID_PARAMETERS;
        } else if ((arg1 & FIRME_MEC_REFRESH_REASON_MASK) == FIRME_MEC_REFRESH_REASON_CREATE) {
            g_creates++;
            g_generation++;
        } else {
            g_destroy_calls++;
            for (line_index = 0U; line_index < 4096 / TEST_LINE_SIZE; line_index++)
                g_destroy_dirty |= g_dirty[line_index];
            result->x0 = (uint32_t)g_destroy_status;
            if (g_destroy_status == FIRME_SUCCESS && !g_destroy_noop)
                g_generation++;
        }
    }
    result->x0 |= g_poison & UINT64_C(0xffffffff00000000);
}

/** @brief Return a supported local architectural MEC field. */
uint64_t read_id_aa64mmfr3_el1(void)
{
    return UINT64_C(1) << 28;
}

/** @brief Keep the independent local width different from the advertised width. */
uint64_t read_mecidr_el2(void)
{
    return (g_pe_widths[g_current_pe] ? g_pe_widths[g_current_pe] : 8U) - 1U;
}

/** @brief Provide two independent SMMUs to the production inventory validator. */
uint32_t pal_iovirt_smmu_count(void)
{
    return 2U;
}

/** @brief Supply controller addresses, optionally injecting a duplicate. */
int pal_iovirt_smmu_info(uint32_t index, pal_smmu_info_t *info)
{
    if (index >= 2U || info == NULL)
        return VAL_ERROR;
    info->base = UINT64_C(0x10000000) + (g_duplicate_smmu ? 0U : index * UINT64_C(0x100000));
    info->realm_base = info->base + UINT64_C(0x40000);
    return VAL_SUCCESS;
}

/** @brief Model checked reads of each controller's capability and independent width. */
int val_mmio_read32_checked(uint64_t address, uint32_t *value)
{
    uint32_t index;
    pal_smmu_info_t info;

    g_smmu_reads++;
    if (g_mmio_fail)
        return VAL_ERROR;
    for (index = 0U; index < 2U; index++) {
        assert(pal_iovirt_smmu_info(index, &info) == VAL_SUCCESS);
        if (address == info.realm_base + UINT64_C(0xc)) {
            *value = g_smmu_widths[index] ? UINT32_C(1) << 16 : 0U;
            return VAL_SUCCESS;
        }
        if (address == info.realm_base + UINT64_C(0x220)) {
            assert(g_smmu_widths[index] != 0U);
            *value = g_smmu_widths[index] - 1U;
            return VAL_SUCCESS;
        }
    }
    return VAL_ERROR;
}

/** @brief Execute real per-PE phases without claiming native PSCI coverage. */
static int run_payload(uint32_t num_pe, const val_test_payload_t *payload,
                        val_test_pe_result_t *results)
{
    uint32_t slot;
    int result = VAL_SUCCESS;

    for (slot = 0U; slot < num_pe; slot++) {
        g_current_pe = slot;
        results[slot].pe_index = slot;
        results[slot].result = (firme_rule_res_t){.status = ACS_STATUS_ERR};
        if (firme_mec_common_mecid_width_payload(payload->world, payload->phase,
                                                payload->argument,
                                                &results[slot].result) != VAL_SUCCESS)
            result = VAL_ERROR;
    }
    g_current_pe = 0U;
    return result;
}

/** @brief Preserve all-PE/SMMU reduction, incomplete coverage, and read failures. */
static void check_common_width(void)
{
    firme_rule_res_t result;

    g_pe_widths[PAL_PE_COUNT - 1U] = 4U;
    g_smmu_reads = 0U;
    assert(firme_mec_common_mecid_width_entry(FIRME_ACS_WORLD_REALM,
             PAL_PE_COUNT, run_payload, &result) ==
           VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS && g_smmu_reads == 4U);
    g_advertised_width = 5U;
    assert(firme_mec_common_mecid_width_entry(FIRME_ACS_WORLD_REALM,
             PAL_PE_COUNT, run_payload, &result) ==
           VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    g_advertised_width = 4U;
    g_smmu_widths[1] = 3U;
    assert(firme_mec_common_mecid_width_entry(FIRME_ACS_WORLD_REALM,
             PAL_PE_COUNT, run_payload, &result) ==
           VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    g_advertised_width = 3U;
    assert(firme_mec_common_mecid_width_entry(FIRME_ACS_WORLD_REALM,
             PAL_PE_COUNT, run_payload, &result) ==
           VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    g_smmu_widths[1] = 0U;
    assert(firme_mec_common_mecid_width_entry(FIRME_ACS_WORLD_REALM,
             PAL_PE_COUNT, run_payload, &result) ==
           VAL_SUCCESS);
    assert(result.status == ACS_STATUS_SKIP);
    g_mmio_fail = VAL_TRUE;
    assert(firme_mec_common_mecid_width_entry(FIRME_ACS_WORLD_REALM,
             PAL_PE_COUNT, run_payload, &result) ==
           VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    g_mmio_fail = VAL_FALSE;
    g_duplicate_smmu = VAL_TRUE;
    assert(firme_mec_common_mecid_width_entry(FIRME_ACS_WORLD_REALM,
             PAL_PE_COUNT, run_payload, &result) ==
           VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
}

/** @brief Model 64-byte cache lines. */
uint64_t read_ctr_el0(void)
{
    return UINT64_C(4) << 16;
}

/** @brief Read saved translation configuration. */
uint64_t read_tcr2_el2(void)
{
    return g_tcr2;
}

/** @brief Track translation configuration writes. */
void write_tcr2_el2(uint64_t value)
{
    g_tcr2 = value;
}

/** @brief Read saved MEC control configuration. */
uint64_t read_sctlr2_el2(void)
{
    return g_sctlr2;
}

/** @brief Track MEC control configuration writes. */
void write_sctlr2_el2(uint64_t value)
{
    g_sctlr2 = value;
}

/** @brief Read the saved alternate MECID. */
uint64_t read_mecid_a0_el2(void)
{
    return g_mecid;
}

/** @brief Track alternate MECID programming and restoration. */
void write_mecid_a0_el2(uint64_t value)
{
    g_mecid = value;
}

/** @brief Leave real TLB ordering to target execution. */
void val_tlbi_all_el2(void)
{
}

/** @brief Model installation of the probe-owned alias. */
int val_mmu_map_alternate_mecid_page_el2(uint64_t address, uint64_t physical)
{
    assert(!g_mapped && address == PLAT_FIRME_MEC_TEST_VA);
    assert(physical == (uintptr_t)__mec_test_pa__);
    g_mapped = VAL_TRUE;
    return VAL_SUCCESS;
}

/** @brief Require alias cleanup on both successful and failed observations. */
int val_mmu_unmap_alternate_mecid_page_el2(uint64_t address)
{
    assert(g_mapped && address == PLAT_FIRME_MEC_TEST_VA);
    g_mapped = VAL_FALSE;
    return VAL_SUCCESS;
}

/** @brief Model eviction of a dirty line before a MEC refresh. */
void val_dcache_clean_invalidate_poe_realm(uint64_t physical)
{
    uint64_t offset = physical - (uintptr_t)__mec_test_pa__;

    assert(offset < 4096 && offset % TEST_LINE_SIZE == 0U);
    g_dirty[offset / TEST_LINE_SIZE] = VAL_FALSE;
}

/** @brief Fail an optional mid-line store after preceding words became dirty. */
int val_probe_write64(uint64_t address, uint64_t value, val_fault_info_t *fault)
{
    uint64_t offset = address - PLAT_FIRME_MEC_TEST_VA;

    assert(g_mapped && offset < 4096);
    *fault = (val_fault_info_t){0};
    g_writes++;
    if (g_writes == g_fail_write_at) {
        *fault = (val_fault_info_t){.faulted = VAL_TRUE, .esr = TEST_ABORT};
        return VAL_ERROR;
    }
    g_words[offset / 8U] = value;
    g_generations[offset / 8U] = g_generation;
    g_dirty[offset / TEST_LINE_SIZE] = VAL_TRUE;
    return VAL_SUCCESS;
}

/** @brief Model stale ciphertext, baseline failures, and a new-context control. */
int val_probe_read64(uint64_t address, uint64_t *value, val_fault_info_t *fault)
{
    uint64_t offset = address - PLAT_FIRME_MEC_TEST_VA;
    uint32_t stale;

    assert(g_mapped && offset < 4096);
    stale = g_generations[offset / 8U] != g_generation;
    *fault = (val_fault_info_t){0};
    if (g_fault_baseline && g_creates == 1U) {
        *fault = (val_fault_info_t){.faulted = VAL_TRUE, .esr = TEST_ABORT};
        return VAL_ERROR;
    }
    if (g_fault_stale && stale) {
        g_stale_fault_count++;
        g_stale_fault_creates = g_creates;
        *fault = (val_fault_info_t){.faulted = VAL_TRUE, .esr = g_stale_fault_esr};
        return VAL_ERROR;
    }
    *value = g_words[offset / 8U];
    if ((stale && !g_no_refresh) || (g_bad_baseline && g_creates == 1U) ||
        (g_bad_round_trip && g_creates == 2U && !stale))
        *value = ~*value;
    return VAL_SUCCESS;
}

/** @brief Optionally inject malformed evidence into the production evaluators. */
int val_firme_mec_observation_probe(val_firme_mec_observation_case_t test_case,
                                   val_firme_mec_observation_evidence_t *evidence)
{
    if (g_use_evidence) {
        *evidence = g_evidence;
        return VAL_FIRME_MEC_OBSERVATION_PROBE_AVAILABLE;
    }
    return val_realm_mec_observation_probe(test_case, evidence);
}

/** @brief Reset the modeled memory system between independent scenarios. */
static void reset_model(void)
{
    memset(g_words, 0, sizeof(g_words));
    memset(g_generations, 0, sizeof(g_generations));
    memset(g_dirty, 0, sizeof(g_dirty));
    g_creates = 0U;
    g_generation = 0U;
    g_destroy_noop = VAL_FALSE;
    g_writes = 0U;
    g_fail_write_at = 0U;
    g_bad_baseline = VAL_FALSE;
    g_fault_baseline = VAL_FALSE;
    g_fault_stale = VAL_FALSE;
    g_stale_fault_esr = TEST_ABORT;
    g_stale_fault_count = 0U;
    g_stale_fault_creates = 0U;
    g_bad_round_trip = VAL_FALSE;
    g_no_refresh = VAL_FALSE;
    g_mapped = VAL_FALSE;
    g_destroy_dirty = VAL_FALSE;
    g_destroy_calls = 0U;
    g_destroy_status = FIRME_SUCCESS;
    g_refresh_status = FIRME_SUCCESS;
    g_tcr2 = UINT64_C(0x40);
    g_sctlr2 = UINT64_C(0x80);
    g_mecid = UINT64_C(0x20);
    g_use_evidence = VAL_FALSE;
}

/** @brief Require cache eviction, retirement, unmapping, and register restoration. */
static void check_restored_count(uint32_t destroy_count)
{
    assert(!g_mapped && !g_destroy_dirty && g_destroy_calls == destroy_count);
    assert(g_tcr2 == UINT64_C(0x40) && g_sctlr2 == UINT64_C(0x80));
    assert(g_mecid == UINT64_C(0x20));
}

/** @brief Require one retirement after a single-context scenario. */
static void check_restored(void)
{
    check_restored_count(1U);
}

/** @brief Keep ABI test return codes consistent with the recorded rule outcome. */
static void check_refresh_returns(void)
{
    firme_rule_res_t result;

    reset_model();
    assert(firme_mec_refresh_abi_run_ns(0U, NULL) == VAL_ERROR);
    assert(firme_mec_refresh_abi_run_secure(0U, NULL) == VAL_ERROR);
    assert(firme_mec_refresh_abi_run_realm(0U, NULL) == VAL_ERROR);
    assert(g_creates == 0U);
    assert(firme_mec_refresh_abi_run_ns(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && result.fail_step == 1U);
    assert(firme_mec_refresh_abi_run_secure(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && result.fail_step == 2U);
    g_refresh_status = FIRME_NOT_SUPPORTED;
    assert(firme_mec_refresh_abi_run_ns(0U, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    assert(firme_mec_refresh_abi_run_secure(0U, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    assert(firme_mec_refresh_abi_run_realm(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && result.fail_step == 8U);
    reset_model();
    g_destroy_status = FIRME_DENIED;
    assert(firme_mec_refresh_abi_run_realm(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && result.fail_step == MEC_SEMANTIC_STEP_CLEANUP);
    reset_model();
    assert(firme_mec_refresh_abi_run_realm(0U, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    check_restored_count(3U);
    reset_model();
    g_destroy_noop = VAL_TRUE;
    assert(firme_mec_refresh_abi_run_realm(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && result.fail_step == MEC_SEMANTIC_STEP_STALE_READ);
    check_restored_count(3U);
    reset_model();
    g_no_refresh = VAL_TRUE;
    assert(firme_mec_refresh_abi_run_realm(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && result.fail_step == MEC_SEMANTIC_STEP_STALE_READ);
    check_restored();
}

/** @brief Observe DESTROY faults before re-enabling and require complete restoration. */
static void check_destroy_faults(void)
{
    firme_rule_res_t result;

    /* Step 1: An unclassified integrity-like abort is incomplete evidence, not PASS. */
    reset_model();
    g_fault_stale = VAL_TRUE;
    assert(firme_mec_semantic_test_context_refresh(
               VAL_FIRME_MEC_OBSERVE_CONTEXT_DESTROY, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_SKIP);
    assert(result.fail_step == MEC_SEMANTIC_STEP_INTEGRITY_ABORT);
    assert(result.last_x1 == TEST_ABORT);
    assert(g_stale_fault_count == VAL_FIRME_MEC_OBSERVATION_MAX_SAMPLES);
    assert(g_stale_fault_creates == 1U && g_creates == 2U);
    check_restored_count(2U);

    /* Step 2: A translation abort must fail, even after re-enable and successful cleanup. */
    reset_model();
    g_fault_stale = VAL_TRUE;
    g_stale_fault_esr = (UINT64_C(0x25) << 26) | UINT64_C(0x04);
    assert(firme_mec_semantic_test_context_refresh(
               VAL_FIRME_MEC_OBSERVE_CONTEXT_DESTROY, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    assert(result.fail_step == MEC_SEMANTIC_STEP_INTEGRITY_ABORT);
    assert(result.last_x0 == g_stale_fault_esr);
    assert(g_stale_fault_count == VAL_FIRME_MEC_OBSERVATION_MAX_SAMPLES);
    assert(g_stale_fault_creates == 1U && g_creates == 2U);
    check_restored_count(2U);
}

/**
 * @brief Require failures to dominate unclassified stale-read faults.
 * @param run_rule Context-refresh or ciphertext-decorrelation production rule.
 */
static void check_semantic_faults(int (*run_rule)(uint64_t, firme_rule_res_t *))
{
    const uint64_t inconclusive[] = {
        TEST_ABORT, (UINT64_C(0x25) << 26) | UINT64_C(0x18),
        (UINT64_C(0x25) << 26) | UINT64_C(0x3f), UINT64_C(0x2f) << 26,
    };
    const uint32_t unrelated_status[] = {
        0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U,
        0x08U, 0x09U, 0x0aU, 0x0bU, 0x0cU, 0x0dU, 0x0eU, 0x0fU,
        0x11U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U,
        0x1bU, 0x1cU, 0x1dU, 0x1eU, 0x1fU, 0x21U,
        0x23U, 0x24U, 0x25U, 0x26U, 0x27U, 0x28U, 0x29U, 0x2bU,
        0x30U, 0x31U, 0x35U,
    };
    const uint32_t unrelated_classes[] = {0x00U, 0x18U, 0x20U, 0x21U, 0x24U, 0x26U};
    val_firme_mec_observation_evidence_t baseline;
    firme_rule_res_t result;
    uint32_t index;

    reset_model();
    assert(val_realm_mec_observation_probe(VAL_FIRME_MEC_OBSERVE_CONTEXT_REFRESH,
                                           &baseline) ==
           VAL_FIRME_MEC_OBSERVATION_PROBE_AVAILABLE);
    check_restored();
    g_use_evidence = VAL_TRUE;
    baseline.stale_read_completed_mask &= ~UINT32_C(1);
    baseline.stale_read_fault_mask = UINT32_C(1);
    for (index = 0U; index < sizeof(inconclusive) / sizeof(inconclusive[0]); index++) {
        baseline.stale_read_fault_esr[0] = inconclusive[index];
        g_evidence = baseline;
        assert(run_rule(0U, &result) == VAL_SUCCESS);
        assert(result.status == ACS_STATUS_SKIP);
        assert(result.fail_step == MEC_SEMANTIC_STEP_INTEGRITY_ABORT);
        assert(result.last_x0 == UINT32_C(1) && result.last_x1 == inconclusive[index]);

        g_evidence.cleanup_status = FIRME_DENIED;
        assert(run_rule(0U, &result) == VAL_ERROR);
        assert(result.status == ACS_STATUS_FAIL && result.fail_step == MEC_SEMANTIC_STEP_CLEANUP);
        g_evidence = baseline;
        g_evidence.round_trip[1] = ~g_evidence.plaintext;
        assert(run_rule(0U, &result) == VAL_ERROR);
        assert(result.status == ACS_STATUS_FAIL && result.fail_step == MEC_SEMANTIC_STEP_ROUND_TRIP);
        g_evidence = baseline;
        g_evidence.round_trip_completed_mask &= ~UINT32_C(2);
        assert(run_rule(0U, &result) == VAL_ERROR);
        assert(result.status == ACS_STATUS_FAIL && result.fail_step == MEC_SEMANTIC_STEP_ROUND_TRIP);
        g_evidence = baseline;
        g_evidence.stale_read[1] = g_evidence.plaintext;
        assert(run_rule(0U, &result) == VAL_ERROR);
        assert(result.status == ACS_STATUS_FAIL && result.fail_step == MEC_SEMANTIC_STEP_STALE_READ);
        g_evidence = baseline;
        g_evidence.stale_read_completed_mask &= ~UINT32_C(2);
        g_evidence.stale_read_fault_mask |= UINT32_C(2);
        g_evidence.stale_read_fault_esr[1] = (UINT64_C(0x25) << 26) | UINT64_C(0x04);
        assert(run_rule(0U, &result) == VAL_ERROR);
        assert(result.status == ACS_STATUS_FAIL && result.fail_step == MEC_SEMANTIC_STEP_INTEGRITY_ABORT);
    }

    for (index = 0U; index < sizeof(unrelated_status) / sizeof(unrelated_status[0]); index++) {
        g_evidence = baseline;
        g_evidence.stale_read_fault_esr[0] = (UINT64_C(0x25) << 26) | unrelated_status[index];
        assert(run_rule(0U, &result) == VAL_ERROR);
        assert(result.status == ACS_STATUS_FAIL && result.fail_step == MEC_SEMANTIC_STEP_INTEGRITY_ABORT);
    }
    for (index = 0U; index < sizeof(unrelated_classes) / sizeof(unrelated_classes[0]); index++) {
        g_evidence = baseline;
        g_evidence.stale_read_fault_esr[0] = (uint64_t)unrelated_classes[index] << 26;
        assert(run_rule(0U, &result) == VAL_ERROR);
        assert(result.status == ACS_STATUS_FAIL && result.fail_step == MEC_SEMANTIC_STEP_INTEGRITY_ABORT);
    }
    reset_model();
    g_fault_stale = VAL_TRUE;
    assert(run_rule(0U, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_SKIP);
    check_restored();

    reset_model();
    g_fault_stale = VAL_TRUE;
    g_stale_fault_esr = (UINT64_C(0x25) << 26) | UINT64_C(0x04);
    assert(run_rule(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    assert(result.fail_step == MEC_SEMANTIC_STEP_INTEGRITY_ABORT);
    assert(result.last_x0 == g_stale_fault_esr && result.last_x1 == 0U);
    check_restored();
}

/** @brief Exercise production callers, probe cleanup, and evidence classification. */
int main(void)
{
    const uint64_t poisons[] = {0U, UINT64_MAX, UINT64_C(0xa5a5a5a55a5a5a5a)};
    firme_rule_res_t result;
    uint32_t index;
    uint32_t width;

    for (index = 0U; index < sizeof(poisons) / sizeof(poisons[0]); index++) {
        g_poison = poisons[index];
        assert(firme_mec_feature_registers_run_realm(0U, &result) == VAL_SUCCESS);
        assert(result.status == ACS_STATUS_PASS);
        assert(val_firme_mec_advertised_common_mecid_width(&width, NULL) == VAL_SUCCESS);
        assert(width == 4U);
        assert(val_firme_mec_local_pe_mecid_width(&width) == VAL_SUCCESS);
        assert(width == 8U);
        g_refresh_present = VAL_FALSE;
        assert(firme_mec_feature_registers_run_realm(0U, &result) == VAL_ERROR);
        assert(result.status == ACS_STATUS_FAIL);
        g_refresh_present = VAL_TRUE;
    }

    reset_model();
    assert(firme_mec_ciphertext_decorrelation_run_realm(0U, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    check_restored();
    reset_model();
    assert(firme_mec_context_refresh_run_realm(0U, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    check_restored();

    reset_model();
    g_no_refresh = VAL_TRUE;
    assert(firme_mec_context_refresh_run_realm(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && result.fail_step == MEC_SEMANTIC_STEP_STALE_READ);
    check_restored();

    reset_model();
    g_bad_baseline = VAL_TRUE;
    assert(firme_mec_context_refresh_run_realm(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && result.fail_step == MEC_SEMANTIC_STEP_SETUP);
    assert(g_creates == 1U);
    check_restored();
    reset_model();
    g_fault_baseline = VAL_TRUE;
    assert(firme_mec_ciphertext_decorrelation_run_realm(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && result.fail_step == MEC_SEMANTIC_STEP_SETUP);
    check_restored();

    reset_model();
    g_fail_write_at = 2U;
    assert(firme_mec_context_refresh_run_realm(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && result.fail_step == MEC_SEMANTIC_STEP_SETUP);
    check_restored();

    reset_model();
    g_fault_stale = VAL_TRUE;
    assert(firme_mec_ciphertext_decorrelation_run_realm(0U, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_SKIP);
    check_restored();
    reset_model();
    g_fault_stale = VAL_TRUE;
    g_destroy_status = FIRME_DENIED;
    assert(firme_mec_ciphertext_decorrelation_run_realm(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && result.fail_step == MEC_SEMANTIC_STEP_CLEANUP);
    check_restored();
    reset_model();
    g_fault_stale = VAL_TRUE;
    g_bad_round_trip = VAL_TRUE;
    assert(firme_mec_ciphertext_decorrelation_run_realm(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && result.fail_step == MEC_SEMANTIC_STEP_ROUND_TRIP);
    check_restored();

    reset_model();
    assert(val_realm_mec_observation_probe(VAL_FIRME_MEC_OBSERVE_CONTEXT_REFRESH,
                                           &g_evidence) ==
           VAL_FIRME_MEC_OBSERVATION_PROBE_AVAILABLE);
    check_restored();
    g_use_evidence = VAL_TRUE;
    g_evidence.stale_read_fault_mask = UINT32_C(1) << g_evidence.sample_count;
    assert(firme_mec_ciphertext_decorrelation_run_realm(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_ERR && result.fail_step == MEC_SEMANTIC_STEP_EVIDENCE);
    assert(firme_mec_context_refresh_run_realm(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_ERR);
    g_evidence.stale_read_fault_mask = UINT32_C(1);
    assert(firme_mec_ciphertext_decorrelation_run_realm(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_ERR);
    assert(firme_mec_context_refresh_run_realm(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_ERR);
    g_evidence.stale_read_completed_mask &= ~UINT32_C(1);
    g_evidence.stale_read_fault_esr[0] = TEST_ABORT;
    g_evidence.stale_read[1] = g_evidence.plaintext;
    assert(firme_mec_ciphertext_decorrelation_run_realm(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && result.fail_step == MEC_SEMANTIC_STEP_STALE_READ);
    g_evidence.stale_read_fault_mask = 0U;
    assert(firme_mec_context_refresh_run_realm(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_ERR);
    check_refresh_returns();
    check_destroy_faults();
    check_semantic_faults(firme_mec_context_refresh_run_realm);
    check_semantic_faults(firme_mec_ciphertext_decorrelation_run_realm);
    check_common_width();
    puts("PASS: MEC unused outputs, all-PE/SMMU width, baseline, cleanup and evidence checks");
    return VAL_SUCCESS;
}
"""


def check_mmio(root, temporary):
    """Execute assembled MMIO code with injected recovery, not hardware exceptions."""
    from unicorn import Uc, UC_ARCH_ARM64, UC_MODE_ARM, UC_HOOK_CODE, arm64_const

    cross = os.environ.get("CROSS_COMPILE", "aarch64-linux-gnu-")
    stub = temporary / "fault_stubs.S"
    executable = temporary / "mmio_checks.elf"
    binary = temporary / "mmio_checks.bin"
    stub.write_text(r"""
    .text
    .global val_fault_begin
/** @brief Emulator hook point for arming a recovery scope. */
val_fault_begin:
    ret
    .global val_fault_end
/** @brief Emulator hook point returning an injected ESR independently of recovery. */
val_fault_end:
    ret
""")
    subprocess.run([cross + "gcc", "-nostdlib", "-static", "-Wl,--build-id=none",
                    "-Wl,-z,noexecstack", "-Wl,-Ttext=0x100000",
                    "-Wl,-e,val_mmio_read32_checked", "-I", str(root / "val_common/include"),
                    str(root / "val_common/src/val_mmio_probe.S"), str(stub),
                    "-o", str(executable)], check=True)
    subprocess.run([cross + "objcopy", "-O", "binary", "--only-section=.text",
                    str(executable), str(binary)], check=True)
    symbols = {}
    for line in subprocess.check_output([cross + "nm", str(executable)], text=True).splitlines():
        fields = line.split()
        if len(fields) == 3:
            symbols[fields[2]] = int(fields[0], 16)
    disassembly = subprocess.check_output([cross + "objdump", "-d", str(executable)], text=True)
    load = re.search(r"^\s*([0-9a-f]+):\s+[0-9a-f]+\s+ldr\s+w21,\s*\[x19\]",
                     disassembly, re.MULTILINE)
    barrier = re.search(r"^\s*([0-9a-f]+):\s+d5033f9f\s+dsb\s+sy",
                        disassembly, re.MULTILINE)
    if load is None or barrier is None:
        raise AssertionError("Cannot locate the production MMIO load and completion barrier")
    load_address = int(load.group(1), 16)
    barrier_address = int(barrier.group(1), 16)
    code = binary.read_bytes()
    val_success = 0
    val_error = -1

    def run_case(name, fault_address=None, syndrome=0, busy=False, null_output=False,
                 unaligned=False):
        """Verify result publication, recovery ownership, and callee-saved state."""
        emulator = Uc(UC_ARCH_ARM64, UC_MODE_ARM)
        text_base, memory_base, stack_base = 0x100000, 0x200000, 0x300000
        stop = text_base + 0xffc
        stack = stack_base + 0xfff0
        sentinel = bytes.fromhex("11223344")
        value = bytes.fromhex("aabbccdd")
        emulator.mem_map(text_base, 0x1000)
        emulator.mem_map(memory_base, 0x1000)
        emulator.mem_map(stack_base, 0x10000)
        emulator.mem_write(text_base, code)
        emulator.mem_write(memory_base, value)
        emulator.mem_write(memory_base + 8, sentinel)
        saved = {getattr(arm64_const, f"UC_ARM64_REG_X{index}"): 0xdead0000 + index
                 for index in range(19, 30)}
        for register, original in saved.items():
            emulator.reg_write(register, original)
        emulator.reg_write(arm64_const.UC_ARM64_REG_SP, stack)
        emulator.reg_write(arm64_const.UC_ARM64_REG_LR, stop)
        emulator.reg_write(arm64_const.UC_ARM64_REG_X0, memory_base + int(unaligned))
        emulator.reg_write(arm64_const.UC_ARM64_REG_X1, 0 if null_output else memory_base + 8)
        state = {"begin": 0, "end": 0, "resume": 0, "faulted": False, "active": busy}

        def hook(machine, address, size, user_data):
            """Redirect only injected faults to the real ADR-selected recovery PC."""
            if address == symbols["val_fault_begin"]:
                state["begin"] += 1
                state["resume"] = machine.reg_read(arm64_const.UC_ARM64_REG_X0)
                if not busy:
                    state["active"] = True
                machine.reg_write(arm64_const.UC_ARM64_REG_W0,
                                  (val_error if busy else val_success) & 0xffffffff)
            elif address == symbols["val_fault_end"]:
                assert state["active"], name
                state["end"] += 1
                state["active"] = False
                machine.reg_write(arm64_const.UC_ARM64_REG_X0, syndrome)
            elif address == fault_address and not state["faulted"]:
                assert state["active"] and state["resume"], name
                state["faulted"] = True
                machine.reg_write(arm64_const.UC_ARM64_REG_PC, state["resume"])

        emulator.hook_add(UC_HOOK_CODE, hook)
        emulator.emu_start(symbols["val_mmio_read32_checked"], stop, count=100)
        assert emulator.reg_read(arm64_const.UC_ARM64_REG_PC) == stop, name
        status = emulator.reg_read(arm64_const.UC_ARM64_REG_W0)
        if status & (1 << 31):
            status -= 1 << 32
        rejected = busy or null_output or unaligned
        failed = rejected or fault_address is not None
        assert status == (val_error if failed else val_success), name
        assert emulator.mem_read(memory_base + 8, 4) == (sentinel if failed else value), name
        assert state["begin"] == (0 if null_output or unaligned else 1), name
        assert state["end"] == (0 if rejected else 1), name
        assert state["active"] == busy, name
        assert state["faulted"] == (fault_address is not None), name
        assert emulator.reg_read(arm64_const.UC_ARM64_REG_SP) == stack, name
        for register, original in saved.items():
            assert emulator.reg_read(register) == original, name

    run_case("normal completion")
    for syndrome in (0, 0x96000010):
        run_case(f"load recovery ESR={syndrome:x}", load_address, syndrome)
        run_case(f"barrier recovery ESR={syndrome:x}", barrier_address, syndrome)
    run_case("ESR alone is not occurrence", syndrome=0x96000010)
    run_case("scope already owned", busy=True)
    run_case("null output", null_output=True)
    run_case("unaligned MMIO address", unaligned=True)
    print("PASS: assembled AArch64 MMIO completion, ESR-zero recovery, output and scope checks")


def main():
    """Compile the production MEC paths with an instrumented native memory model."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mmio", action="store_true",
                        help="also emulate AArch64 MMIO code (requires Unicorn and cross-binutils)")
    arguments = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    command = [os.environ.get("HOST_CC", "cc"), "-std=c11", "-Wall", "-Wextra", "-Werror",
               "-ffunction-sections", "-fdata-sections", "-fsanitize=undefined",
               "-fno-sanitize-recover=all"]
    for include in [root / "val/include", root / "val_common/include", root / "test/mec",
                    root / "platform/pal_baremetal/targets/fvp/include"]:
        command.extend(["-I", str(include)])
    with tempfile.TemporaryDirectory(prefix="build-mec-native-", dir=root) as directory:
        temporary = Path(directory)
        source = temporary / "mec_checks.c"
        probe = temporary / "probe.o"
        binary = temporary / "mec_checks"
        source.write_text(SOURCE)
        subprocess.run(command + [
            "-Dval_firme_mec_observation_probe=val_realm_mec_observation_probe",
            "-c", str(root / "val/realm/val_realm_mec_observation.c"), "-o", str(probe)],
            check=True)
        production = ["val/src/val_firme_abi.c", "val/src/val_firme_mec.c",
                      "val/src/val_iovirt.c", "val/src/val_smmu.c",
                      "test/mec/mec_semantic_test_common.c", "test/mec/mec_context_refresh.c",
                      "test/mec/mec_ciphertext_decorrelation.c", "test/mec/mec_feature_registers.c",
                      "test/mec/mec_common_mecid_width.c", "test/mec/mec_refresh_abi.c"]
        subprocess.run(command + [str(source), str(probe)] +
                       [str(root / path) for path in production] +
                       ["-Wl,--gc-sections", "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
        if arguments.mmio:
            check_mmio(root, temporary)


if __name__ == "__main__":
    main()
