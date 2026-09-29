#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

"""Native regressions for FIRME caller checks, ABI execution and SMCCC probes."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest

import check_abi_callers


ROOT = Path(__file__).resolve().parents[2]
COMMON = r"""
#include <assert.h>
#include <string.h>
#include "val_firme_abi.h"
#include "val_firme_abi_test.h"
#include "val_firme_smccc_probe.h"
#include "val_firme_smccc_probe_private.h"
#include "val_log.h"

/** @brief Replace the architectural barrier for native-only fixtures. */
void dsb_sy(void)
{
}

/** @brief Supply a stable PE identity to production FIRME wrappers. */
uint64_t read_mpidr_el1(void)
{
    return 0;
}

/** @brief Replace UART logging in focused native tests. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *fmt, ...)
{
    (void)verbosity;
    (void)tag;
    (void)fmt;
    return VAL_SUCCESS;
}
"""

SMCCC_SOURCE = COMMON + r"""
int firme_base_smccc_version_run_ns(uint64_t base_pa, firme_rule_res_t *out);
int firme_base_smccc_sve_run_ns(uint64_t base_pa, firme_rule_res_t *out);
int firme_base_smccc_sme_run_ns(uint64_t base_pa, firme_rule_res_t *out);

static uint64_t g_smccc_version = UINT64_C(0x10004);
static uint32_t g_absent_service = FIRME_SERVICE_ID_MAX;
static uint32_t g_calls;
static uint32_t g_corrupt_register = 31U;
static uint64_t g_status_fid;
static int32_t g_status;

/** @brief Advertise SVE so both ordinary and SVE-specific tests run. */
uint64_t read_id_aa64pfr0_el1(void)
{
    return UINT64_C(1) << 32;
}

/** @brief Advertise SME for version and ZA-preservation checks. */
uint64_t read_id_aa64pfr1_el1(void)
{
    return UINT64_C(1) << 24;
}

/** @brief Simulate discovery without executing firmware or changing state. */
void val_smccc_smc_call(uint64_t fid, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                       uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7,
                       smccc_res_t *result)
{
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    (void)arg6;
    (void)arg7;
    *result = (smccc_res_t){0};
    if (fid == UINT64_C(0x80000000)) {
        result->x0 = g_smccc_version;
        return;
    }
    assert(fid == FIRME_FID_SERVICE_VERSION || fid == FIRME_FID_SERVICE_FEATURES);
    result->x0 = fid == FIRME_FID_SERVICE_VERSION ? UINT64_C(0x10000) : FIRME_SUCCESS;
    if (arg1 == g_absent_service)
        result->x0 = (uint32_t)FIRME_NOT_SUPPORTED;
    result->x1 = UINT64_MAX;
}

/** @brief Return preserved architectural state and arbitrary parameter outputs. */
int val_smccc_probe_state(uint64_t fid, uint64_t modes,
                          val_smccc_probe_context_t *context)
{
    uint32_t index;

    assert(fid >= FIRME_FID_SERVICE_VERSION && fid <= FIRME_FID_GM_GPT_INFO);
    g_calls |= UINT32_C(1) << (fid - FIRME_FID_SERVICE_VERSION);
    memcpy(context->returned, context->seed, VAL_SMCCC_VECTOR_BYTES);
    context->vector_length = 16U;
    context->returned_length = 16U;
    context->streaming_length = 16U;
    context->returned_svcr = (modes & VAL_SMCCC_PROBE_STREAMING) != 0U ? 3U : 2U;
    context->registers[0] = fid == FIRME_FID_SERVICE_VERSION ?
                            UINT64_C(0xffffffff00010000) : FIRME_SUCCESS;
    if (fid == g_status_fid)
        context->registers[0] = (uint32_t)g_status;
    for (index = 1U; index < 18U; index++)
        context->registers[index] = UINT64_MAX - index;
    for (index = 18U; index < 31U; index++)
        context->registers[index] = index;
    if (g_corrupt_register < 31U)
        context->registers[g_corrupt_register] = UINT64_MAX;
    return VAL_SUCCESS;
}

/** @brief Reject preservation/status corruption without rejecting unused outputs. */
int main(void)
{
    firme_rule_res_t result = {.status = ACS_STATUS_PASS};

    assert(val_firme_smccc_check_state(0U, &result) == VAL_SUCCESS);
    assert(g_calls == ((UINT32_C(1) << 20) - 1U));
    g_status_fid = FIRME_FID_GM_L1_GPT_DESTROY;
    g_status = FIRME_NO_MEMORY;
    assert(val_firme_smccc_check_service(FIRME_SERVICE_ID_GRANULE_MGMT,
                                        0U, &result) == VAL_SUCCESS);
    for (g_status_fid = FIRME_FID_ATTEST_PAT_GET;
         g_status_fid <= FIRME_FID_ATTEST_PAT_EXT_CLAIMS_STAGE; g_status_fid++)
        assert(val_firme_smccc_check_service(FIRME_SERVICE_ID_ATTESTATION,
                                            0U, &result) == VAL_SUCCESS);
    g_status_fid = FIRME_FID_ATTEST_PAT_GET;
    g_status = FIRME_DENIED;
    assert(val_firme_smccc_check_service(FIRME_SERVICE_ID_ATTESTATION,
                                        0U, &result) == VAL_SUCCESS);
    g_status_fid = FIRME_FID_ATTEST_RAK_GET;
    assert(val_firme_smccc_check_service(FIRME_SERVICE_ID_ATTESTATION,
                                        0U, &result) == VAL_ERROR);
    g_status_fid = FIRME_FID_GM_GPT_INFO;
    g_status = FIRME_BUSY;
    assert(val_firme_smccc_check_state(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    g_status_fid = 0;
    g_corrupt_register = 18U;
    assert(val_firme_smccc_check_state(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && result.fail_step == 18U);
    g_corrupt_register = 31U;
    g_absent_service = FIRME_SERVICE_ID_BASE;
    result.status = ACS_STATUS_PASS;
    assert(val_firme_smccc_check_state(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    g_absent_service = FIRME_SERVICE_ID_GRANULE_MGMT;
    assert(val_firme_smccc_check_state(0U, &result) == VAL_SUCCESS);
    result.status = ACS_STATUS_PASS;
    assert(val_firme_smccc_check_service(g_absent_service, 0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    result.status = ACS_STATUS_PASS;
    assert(val_firme_smccc_check_service(FIRME_SERVICE_ID_MAX, 0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    g_absent_service = FIRME_SERVICE_ID_MAX;
    g_calls = 0U;
    g_smccc_version = UINT64_C(0xffffffff00010001);
    assert(firme_base_smccc_version_run_ns(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && g_calls == 0U);
    g_smccc_version = UINT64_C(0xffffffff00010002);
    assert(firme_base_smccc_sve_run_ns(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && g_calls == 0U);
    g_smccc_version = UINT64_C(0xffffffff00010003);
    assert(firme_base_smccc_sme_run_ns(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && g_calls == 0U);
    g_smccc_version = UINT64_C(0xffffffff00010004);
    assert(firme_base_smccc_sme_run_ns(0, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    g_corrupt_register = 18U;
    assert(firme_base_smccc_version_run_ns(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    assert(firme_base_smccc_sve_run_ns(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    assert(firme_base_smccc_sme_run_ns(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    return VAL_SUCCESS;
}
"""

FRAMEWORK_SOURCE = COMMON + r"""
static val_firme_abi_descriptor_t g_abi = {
    .id = VAL_FIRME_ABI_ID_GM_L1_GPT_CREATE,
    .name = "optional ABI fixture",
    .fid = FIRME_FID_GM_L1_GPT_CREATE,
    .service_id = FIRME_SERVICE_ID_GRANULE_MGMT,
    .feature_bit = FIRME_GM_FEATURE_L1_CREATE_BIT,
    .input_bit_masks = {[0] = UINT32_MAX},
    .world_policy = {VAL_FIRME_ABI_WORLD_OPTIONAL,
                     VAL_FIRME_ABI_WORLD_OPTIONAL, VAL_FIRME_ABI_WORLD_OPTIONAL},
};
static val_firme_abi_case_t g_case = {
    .name = "success fixture",
    .expectation = VAL_FIRME_EXPECT_STATUS,
    .expected_status = FIRME_SUCCESS,
    .diagnostic_step = 7U,
};
static val_firme_abi_test_plan_t g_plan = {
    .name = "native fixture, not a registry plan",
    .enabled = VAL_TRUE,
    .abi_id = VAL_FIRME_ABI_ID_GM_L1_GPT_CREATE,
    .optional_worlds_mask = VAL_FIRME_ABI_WORLD_MASK_NS,
    .require_advertised = VAL_TRUE,
    .cases = &g_case,
    .case_count = 1U,
};
static uint32_t g_advertised = VAL_TRUE;
static uint32_t g_implemented = VAL_TRUE;
static uint32_t g_validation = VAL_TRUE;
static uint32_t g_effect = VAL_TRUE;
static uint32_t g_cleanup_count;
static uint32_t g_call_count;
static uint32_t g_dirty_input;
static int g_cleanup_status = VAL_SUCCESS;
static int g_prepare_status = VAL_SUCCESS;

/** @brief Supply a private optional-ABI fixture without changing registry gates. */
const val_firme_abi_descriptor_t *val_firme_abi_catalog_get(val_firme_abi_id_t id)
{
    assert(id == g_abi.id);
    return &g_abi;
}

/** @brief Supply a private plan to exercise executor validation paths. */
const val_firme_abi_test_plan_t *val_firme_abi_test_plan_get(val_firme_abi_test_plan_id_t id)
{
    (void)id;
    return &g_plan;
}

/** @brief Simulate service and ABI advertisement through production decoders. */
void val_smccc_smc_call(uint64_t fid, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                       uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7,
                       smccc_res_t *result)
{
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    (void)arg6;
    (void)arg7;
    *result = (smccc_res_t){0};
    result->x0 = fid == FIRME_FID_SERVICE_VERSION ? UINT64_C(0x10000) : FIRME_SUCCESS;
    if (g_implemented == VAL_FALSE)
        result->x0 = (uint32_t)FIRME_NOT_SUPPORTED;
    result->x1 = g_advertised == VAL_TRUE ? g_abi.feature_bit : 0;
}

/** @brief Return a successful ABI result after the executor validates inputs. */
void val_smccc_smc_call_frame(const smccc_call_frame_t *input, smccc_call_frame_t *output)
{
    assert(input->registers[0] == g_abi.fid);
    g_call_count++;
    *output = (smccc_call_frame_t){0};
    output->registers[0] = FIRME_SUCCESS;
}

/** @brief Select successful or unavailable preparation. */
static int prepare(val_firme_abi_test_context_t *context)
{
    (void)context;
    return g_prepare_status;
}

/** @brief Optionally inject an SBZ violation into the adapter frame. */
static int build(val_firme_abi_test_context_t *context)
{
    context->input.registers[17] = g_dirty_input;
    return VAL_SUCCESS;
}

/** @brief Model output validators, including a wrongly returned error code. */
static uint32_t validate(val_firme_abi_test_context_t *context)
{
    (void)context;
    return g_validation;
}

/** @brief Model effect validators independently of output validation. */
static uint32_t effect(val_firme_abi_test_context_t *context)
{
    (void)context;
    return g_effect;
}

/** @brief Count cleanup calls and optionally report cleanup failure. */
static int cleanup(val_firme_abi_test_context_t *context)
{
    (void)context;
    g_cleanup_count++;
    return g_cleanup_status;
}

/** @brief Exercise required-world, fail-closed validation and lifecycle behavior. */
int main(void)
{
    const val_firme_abi_test_ops_t ops = {
        .prepare_state = prepare, .build_valid_input = build,
        .validate_output = validate, .validate_effect = effect, .cleanup = cleanup,
    };
    val_firme_abi_test_request_t request = {.world = FIRME_ACS_WORLD_NSEL2, .ops = &ops};
    firme_rule_res_t result = {.status = ACS_STATUS_PASS};

    assert(val_firme_abi_test_execute(&request, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS && g_call_count == 1U && g_cleanup_count == 1U);
    g_validation = (uint32_t)VAL_ERROR;
    assert(val_firme_abi_test_execute(&request, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && result.fail_step == g_case.diagnostic_step);
    assert(g_cleanup_count == 2U);
    g_validation = VAL_TRUE;
    g_case.verify_no_effect = VAL_TRUE;
    g_effect = (uint32_t)VAL_ERROR;
    assert(val_firme_abi_test_execute(&request, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && g_cleanup_count == 3U);
    g_effect = VAL_TRUE;
    g_advertised = VAL_FALSE;
    assert(val_firme_abi_test_execute(&request, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_SKIP);
    g_plan.required_worlds_mask = VAL_FIRME_ABI_WORLD_MASK_NS;
    g_plan.optional_worlds_mask = 0U;
    assert(val_firme_abi_test_execute(&request, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && g_call_count == 3U);
    g_implemented = VAL_FALSE;
    assert(val_firme_abi_test_execute(&request, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    g_implemented = VAL_TRUE;
    g_advertised = VAL_TRUE;
    g_dirty_input = VAL_TRUE;
    assert(val_firme_abi_test_execute(&request, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && g_call_count == 3U && g_cleanup_count == 4U);
    g_dirty_input = VAL_FALSE;
    g_prepare_status = VAL_FIRME_ABI_SCENARIO_UNAVAILABLE;
    assert(val_firme_abi_test_execute(&request, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_SKIP && g_cleanup_count == 5U);
    g_cleanup_status = VAL_ERROR;
    assert(val_firme_abi_test_execute(&request, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && g_cleanup_count == 6U);
    g_prepare_status = VAL_SUCCESS;
    assert(val_firme_abi_test_execute(&request, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && g_cleanup_count == 7U);
    assert(val_firme_abi_test_execute(NULL, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    assert(val_firme_abi_test_execute(&request, NULL) == VAL_ERROR);
    g_plan.case_count = 0U;
    assert(val_firme_abi_test_execute(&request, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    g_plan.case_count = 1U;
    g_plan.cases = NULL;
    assert(val_firme_abi_test_execute(&request, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    return VAL_SUCCESS;
}
"""


CONCEPT_SOURCE = COMMON + r"""
#include "partial_retrieval_test_common.h"
#include "shared_buffer_test_common.h"

int firme_base_partial_retrieval_last_chunk_run_realm(uint64_t base_pa, firme_rule_res_t *out);
int firme_base_partial_retrieval_intermediate_chunk_run_realm(uint64_t base_pa,
                                                            firme_rule_res_t *out);
int firme_base_partial_retrieval_no_chunk_run_realm(uint64_t base_pa, firme_rule_res_t *out);
int firme_base_shared_buffer_size_bounds_run_ns(uint64_t base_pa, firme_rule_res_t *out);
int firme_base_shared_buffer_alignment_run_ns(uint64_t base_pa, firme_rule_res_t *out);

static int64_t g_expected_status = FIRME_SUCCESS;
static int64_t g_observed_status = FIRME_SUCCESS;
static uint64_t g_written = 1U;
static uint64_t g_remaining;
static uint64_t g_maximum_pages = 7U;
static uint32_t g_unwritten;
static uint32_t g_oversized;
static uint32_t g_above_maximum_calls;

/** @brief Keep the legacy evidence fixtures separate from real PAT driver tests. */
void val_smccc_smc_call_frame(const smccc_call_frame_t *input, smccc_call_frame_t *output)
{
    (void)input;
    (void)output;
    assert(VAL_FALSE);
}

/** @brief Return Base buffer limits to the real production decoder. */
void val_smccc_smc_call(uint64_t fid, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                       uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7,
                       smccc_res_t *result)
{
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    (void)arg6;
    (void)arg7;
    assert(fid == FIRME_FID_SERVICE_FEATURES);
    *result = (smccc_res_t){0};
    result->x0 = FIRME_SUCCESS;
    result->x1 = g_maximum_pages << FIRME_BASE_MAX_SH_BUF_PG_CNT_SHIFT;
}

/** @brief Advertise a native fixture without enabling any registry entry. */
uint32_t val_firme_partial_retrieval_probe_available(firme_acs_world_t world)
{
    (void)world;
    return VAL_TRUE;
}

/** @brief Return controlled chunk evidence independently of its expected status. */
int val_firme_partial_retrieval_probe(firme_acs_world_t world,
    val_firme_partial_retrieval_case_t test_case, val_firme_partial_retrieval_evidence_t *evidence)
{
    (void)world;
    (void)test_case;
    if (g_unwritten == VAL_FALSE) {
        evidence->valid_fields = VAL_FIRME_PARTIAL_RETRIEVAL_EVIDENCE_STATUS |
            VAL_FIRME_PARTIAL_RETRIEVAL_EVIDENCE_WRITTEN |
            VAL_FIRME_PARTIAL_RETRIEVAL_EVIDENCE_REMAINING |
            VAL_FIRME_PARTIAL_RETRIEVAL_EVIDENCE_SERVICE;
        evidence->service_id = FIRME_SERVICE_ID_INTEGRATED_DEVICE;
        evidence->abi_status = g_observed_status;
        evidence->written_size = g_written;
        evidence->remaining_size = g_remaining;
    }
    return VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_AVAILABLE;
}

#ifndef OMIT_STATUS_CONTRACT
/** @brief Model the selected adapter's independent status/size-validity contract. */
int firme_partial_retrieval_test_expected_status(firme_acs_world_t world,
    val_firme_partial_retrieval_case_t test_case,
    const val_firme_partial_retrieval_evidence_t *evidence, int64_t *expected_status)
{
    (void)world;
    (void)test_case;
    assert(evidence->service_id == FIRME_SERVICE_ID_INTEGRATED_DEVICE);
    *expected_status = g_expected_status;
    return VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_AVAILABLE;
}
#endif

/** @brief Advertise a controlled shared-buffer adapter for native tests. */
uint32_t val_firme_shared_buffer_probe_available(firme_acs_world_t world)
{
    (void)world;
    return VAL_TRUE;
}

/** @brief Return aligned, misaligned and size-bound input evidence. */
int val_firme_shared_buffer_probe(firme_acs_world_t world,
    val_firme_shared_buffer_case_t test_case, uint64_t minimum_size_bytes,
    uint64_t maximum_page_count, val_firme_shared_buffer_evidence_t *evidence)
{
    (void)world;
    if (g_unwritten == VAL_TRUE)
        return VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE;
    evidence->valid_fields = VAL_FIRME_SHARED_BUFFER_EVIDENCE_STATUS |
        VAL_FIRME_SHARED_BUFFER_EVIDENCE_SERVICE | VAL_FIRME_SHARED_BUFFER_EVIDENCE_BASE |
        VAL_FIRME_SHARED_BUFFER_EVIDENCE_PAGE_COUNT;
    evidence->service_id = FIRME_SERVICE_ID_ATTESTATION;
    evidence->base_pa = minimum_size_bytes;
    evidence->abi_status = FIRME_SUCCESS;
    if (test_case == VAL_FIRME_SHARED_BUFFER_CASE_MAXIMUM)
        evidence->page_count = maximum_page_count;
    if (test_case == VAL_FIRME_SHARED_BUFFER_CASE_ABOVE_MAXIMUM) {
        g_above_maximum_calls++;
        evidence->page_count = maximum_page_count + 1ULL;
        evidence->abi_status = FIRME_INVALID_PARAMETERS;
    }
    if (test_case == VAL_FIRME_SHARED_BUFFER_CASE_MISALIGNED) {
        evidence->base_pa++;
        evidence->abi_status = FIRME_INVALID_PARAMETERS;
    }
    if (g_oversized == VAL_TRUE)
        evidence->page_count = UINT64_MAX;
    return VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE;
}

/** @brief Exercise adapter-derived status, stale evidence and SBZ-safe bounds. */
int main(void)
{
    firme_rule_res_t result;
    val_firme_partial_retrieval_evidence_t chunk = {0};
    val_firme_shared_buffer_evidence_t buffer = {0};
    shared_buffer_limits_t limits = {4096U, 7U, 32768U};

    assert(firme_base_partial_retrieval_last_chunk_run_realm(0, &result) == VAL_SUCCESS);
#ifdef OMIT_STATUS_CONTRACT
    assert(result.status == ACS_STATUS_SKIP);
    g_expected_status = FIRME_SUCCESS;
#else
    assert(result.status == ACS_STATUS_PASS);
    g_remaining = 1U;
    assert(firme_base_partial_retrieval_intermediate_chunk_run_realm(0, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    g_written = 0U;
    g_remaining = 0U;
    assert(firme_base_partial_retrieval_no_chunk_run_realm(0, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    g_expected_status = FIRME_INCOMPLETE;
    assert(firme_base_partial_retrieval_no_chunk_run_realm(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && result.fail_step == PARTIAL_RETRIEVAL_STEP_STATUS);
    g_observed_status = FIRME_INCOMPLETE;
    assert(firme_base_partial_retrieval_no_chunk_run_realm(0, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    g_written = 1U;
    assert(firme_base_partial_retrieval_no_chunk_run_realm(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    g_remaining = 1U;
    assert(firme_base_partial_retrieval_last_chunk_run_realm(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    g_expected_status = INT64_MAX;
    assert(firme_base_partial_retrieval_intermediate_chunk_run_realm(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_ERR);
#endif
    assert(firme_base_shared_buffer_size_bounds_run_ns(0, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS && g_above_maximum_calls == 1U);
    assert(firme_base_shared_buffer_alignment_run_ns(0, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    g_maximum_pages = FIRME_BASE_MAX_SH_BUF_PG_CNT_MASK;
    assert(firme_base_shared_buffer_size_bounds_run_ns(0, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_SKIP && g_above_maximum_calls == 1U);
    g_oversized = VAL_TRUE;
    assert(firme_base_shared_buffer_alignment_run_ns(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_ERR);
    g_unwritten = VAL_TRUE;
    chunk.valid_fields = UINT32_MAX;
    assert(firme_partial_retrieval_test_collect(FIRME_ACS_WORLD_REALM,
        VAL_FIRME_PARTIAL_RETRIEVAL_LAST_CHUNK, &result, &chunk) == VAL_ERROR);
    assert(result.status == ACS_STATUS_ERR);
    buffer.valid_fields = UINT32_MAX;
    assert(firme_shared_buffer_test_collect(FIRME_ACS_WORLD_NSEL2,
        VAL_FIRME_SHARED_BUFFER_CASE_ALIGNED, &limits, &result, &buffer) == VAL_ERROR);
    assert(result.status == ACS_STATUS_ERR);
    return VAL_SUCCESS;
}
"""


FEATURE_SOURCE = COMMON + r"""
#include "feature_register_abi_presence.c"
#include "service_abi_compliance.c"
#include "service_feature_registers.c"
#include "feature_register_result_encoding.c"

static uint32_t g_missing_register;
static uint32_t g_missing_mandatory;
static uint32_t g_forbidden_service;

/** @brief Supply legal allocated fields plus nonzero unused feature bits. */
void val_smccc_smc_call(uint64_t fid, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                       uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7,
                       smccc_res_t *result)
{
    (void)arg3;
    (void)arg4;
    (void)arg5;
    (void)arg6;
    (void)arg7;
    assert(fid == FIRME_FID_SERVICE_FEATURES || fid == FIRME_FID_SERVICE_VERSION);
    *result = (smccc_res_t){0};
    if (fid == FIRME_FID_SERVICE_VERSION) {
        result->x0 = arg1 <= FIRME_SERVICE_ID_GRANULE_MGMT ?
                      UINT64_C(0x10000) : (uint32_t)FIRME_NOT_SUPPORTED;
        return;
    }
    result->x0 = g_missing_register == VAL_TRUE ? (uint32_t)FIRME_NOT_SUPPORTED : FIRME_SUCCESS;
    result->x1 = val_firme_service_mandatory_abi_mask(FIRME_ACS_WORLD_NSEL2, arg1) |
                  (UINT64_C(1) << 63);
    if (g_missing_mandatory == VAL_TRUE && arg1 == FIRME_SERVICE_ID_GRANULE_MGMT)
        result->x1 &= ~FIRME_GM_FEATURE_GPI_SET_BIT;
    if (arg2 == FIRME_BASE_FEATURE_REG1_INDEX) {
        result->x1 = val_firme_base_service_list_bit(FIRME_SERVICE_ID_GRANULE_MGMT) |
                      UINT64_C(0xffffffffffe00000);
        if (g_forbidden_service == VAL_TRUE)
            result->x1 |= val_firme_base_service_list_bit(FIRME_SERVICE_ID_MECID_MGMT);
    }
}

/** @brief Accept unused outputs without accepting missing or prohibited features. */
int main(void)
{
    firme_rule_res_t result;

    feature_register_abi_init_result(&result);
    assert(feature_register_abi_check_service(FIRME_ACS_WORLD_NSEL2,
        FIRME_SERVICE_ID_GRANULE_MGMT, &result) == VAL_TRUE);
    assert(service_abi_compliance_check_service(FIRME_ACS_WORLD_NSEL2,
        FIRME_SERVICE_ID_GRANULE_MGMT, &result) == VAL_TRUE);
    g_missing_register = VAL_TRUE;
    assert(feature_register_abi_check_service(FIRME_ACS_WORLD_NSEL2,
        FIRME_SERVICE_ID_GRANULE_MGMT, &result) == VAL_FALSE);
    assert(service_abi_compliance_check_service(FIRME_ACS_WORLD_NSEL2,
        FIRME_SERVICE_ID_GRANULE_MGMT, &result) == VAL_FALSE);
    g_missing_register = VAL_FALSE;
    g_missing_mandatory = VAL_TRUE;
    assert(feature_register_abi_check_service(FIRME_ACS_WORLD_NSEL2,
        FIRME_SERVICE_ID_GRANULE_MGMT, &result) == VAL_FALSE);
    assert(service_abi_compliance_check_service(FIRME_ACS_WORLD_NSEL2,
        FIRME_SERVICE_ID_GRANULE_MGMT, &result) == VAL_FALSE);
    assert(firme_base_feature_register_abi_presence_run_ns(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    assert(firme_base_service_abi_compliance_run_ns(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    g_missing_mandatory = VAL_FALSE;
    assert(firme_base_service_feature_registers_run_ns(0, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    assert(firme_base_feature_register_result_encoding_run_ns(0, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    g_forbidden_service = VAL_TRUE;
    assert(firme_base_service_feature_registers_run_ns(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    assert(firme_base_feature_register_result_encoding_run_ns(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    return VAL_SUCCESS;
}
"""


RETURN_SOURCE = COMMON + r"""
#include "smc64_calling_convention.c"
#include "status_codes.c"
#include "service_version_compatibility.c"
#include "reserved_input_registers.c"
#include "reserved_input_bits.c"
#include "max_concurrent_operations.c"

static int g_fid_return = VAL_SUCCESS;
static int g_state_return = VAL_SUCCESS;
static int g_capture_enable_return = VAL_SUCCESS;
static int g_capture_read_return = VAL_SUCCESS;
static uint32_t g_capture_enabled;
static uint32_t g_capture_disable_count;
static smccc_input_registers_t g_captured;
static uint64_t g_base_version = UINT64_C(0x10000);
static int32_t g_gm_version_status;
static int32_t g_reg1_status = FIRME_SUCCESS;
static uint32_t g_baseline_available = VAL_TRUE;
static uint64_t g_baseline_status = ACS_STATUS_PASS;
static int g_baseline_return = VAL_SUCCESS;

/** @brief Inject encoding failures without changing the rule result initializer. */
int val_firme_smccc_check_fid_encodings(firme_rule_res_t *out)
{
    assert(out);
    return g_fid_return;
}

/** @brief Inject preservation failures through the production Base rule bodies. */
int val_firme_smccc_check_state(uint64_t modes, firme_rule_res_t *out)
{
    (void)modes;
    assert(out);
    return g_state_return;
}

/** @brief Supply discovery results and capture real Base/GPT/MEC wrapper inputs. */
void val_smccc_smc_call(uint64_t fid, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                       uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7,
                       smccc_res_t *result)
{
    g_captured = (smccc_input_registers_t){.registers = {
        fid, arg1, arg2, arg3, arg4, arg5, arg6, arg7
    }};
    *result = (smccc_res_t){0};
    result->x0 = FIRME_SUCCESS;
    if (fid == FIRME_FID_SERVICE_VERSION) {
        result->x0 = (uint32_t)FIRME_NOT_SUPPORTED;
        if (arg1 == FIRME_SERVICE_ID_BASE)
            result->x0 = g_base_version;
        else if (arg1 == FIRME_SERVICE_ID_GRANULE_MGMT)
            result->x0 = g_gm_version_status != FIRME_SUCCESS ?
                          (uint32_t)g_gm_version_status : UINT64_C(0x10000);
    } else if (fid == FIRME_FID_SERVICE_FEATURES) {
        if (arg1 >= FIRME_SERVICE_ID_MAX)
            result->x0 = (uint32_t)FIRME_NOT_SUPPORTED;
        else if (arg2 == FIRME_BASE_FEATURE_REG1_INDEX)
            result->x0 = (uint32_t)g_reg1_status;
    }
}

/** @brief Allow independent capture setup failures. */
int val_smccc_test_input_capture_enable(void)
{
    if (g_capture_enable_return == VAL_SUCCESS)
        g_capture_enabled = VAL_TRUE;
    return g_capture_enable_return;
}

/** @brief Count cleanup after both bad captured frames and capture read errors. */
void val_smccc_test_input_capture_disable(void)
{
    g_capture_enabled = VAL_FALSE;
    g_capture_disable_count++;
}

/** @brief Inject an unused-register violation in the first captured wrapper call. */
int val_smccc_test_input_capture_read(smccc_input_registers_t *input)
{
    assert(g_capture_enabled == VAL_TRUE);
    *input = g_captured;
    input->registers[17] = UINT64_MAX;
    return g_capture_read_return;
}

/** @brief Ensure failed first-call capture never reaches IDE programming. */
void val_firme_ide_keyset_prog(const val_firme_ide_prog_args_t *args, firme_res_t *out)
{
    (void)args;
    (void)out;
    assert(VAL_FALSE);
}

/** @brief Ensure failed first-call capture never reaches IDE activation. */
void val_firme_ide_keyset_go(uint64_t ecam, uint64_t flags, uint64_t keyset,
                             uint64_t handle, firme_res_t *out)
{
    (void)ecam;
    (void)flags;
    (void)keyset;
    (void)handle;
    (void)out;
    assert(VAL_FALSE);
}

/** @brief Ensure failed first-call capture never reaches IDE stop. */
void val_firme_ide_keyset_stop(uint64_t ecam, uint64_t flags, uint64_t keyset,
                               uint64_t handle, firme_res_t *out)
{
    (void)ecam;
    (void)flags;
    (void)keyset;
    (void)handle;
    (void)out;
    assert(VAL_FALSE);
}

/** @brief Ensure failed first-call capture never reaches IDE polling. */
void val_firme_ide_keyset_poll(uint64_t ecam, uint64_t flags, uint64_t keyset,
                               firme_res_t *out)
{
    (void)ecam;
    (void)flags;
    (void)keyset;
    (void)out;
    assert(VAL_FALSE);
}

/** @brief Model legacy and current baseline rule return/status combinations. */
static int baseline_body(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    *out = (firme_rule_res_t){.status = g_baseline_status, .fail_step = 9U};
    return g_baseline_return;
}

/** @brief Supply a private baseline fixture without modifying registry gates. */
const firme_test_case_t *val_firme_suite_find(firme_rule_id_t rule)
{
    static const firme_test_case_t baseline = {
        .run_ns = baseline_body, .run_secure = baseline_body, .run_realm = baseline_body
    };

    (void)rule;
    return g_baseline_available == VAL_TRUE ? &baseline : NULL;
}

/** @brief Check the function return and recorded architectural result together. */
static void check_result(firme_rule_fn_t body, int expected_return, uint64_t expected_status)
{
    firme_rule_res_t result = {.status = ACS_STATUS_PASS};

    assert(body(0, &result) == expected_return);
    assert(result.status == expected_status);
    if (expected_status == ACS_STATUS_FAIL || expected_status == ACS_STATUS_ERR)
        assert(result.fail_step != 0U);
    assert(body(0, NULL) == VAL_ERROR);
}

/** @brief Regress every affected caller world and the concurrency post-run path. */
int main(void)
{
    const firme_rule_fn_t smc64[] = {
        firme_base_smc64_calling_convention_run_ns,
        firme_base_smc64_calling_convention_run_secure,
        firme_base_smc64_calling_convention_run_realm
    };
    const firme_rule_fn_t status_codes[] = {
        firme_base_status_codes_run_ns, firme_base_status_codes_run_secure,
        firme_base_status_codes_run_realm
    };
    const firme_rule_fn_t compatibility[] = {
        firme_base_service_version_compatibility_run_ns,
        firme_base_service_version_compatibility_run_secure,
        firme_base_service_version_compatibility_run_realm
    };
    const firme_rule_fn_t input_audits[] = {
        firme_base_reserved_input_registers_run_ns, firme_base_reserved_input_registers_run_secure,
        firme_base_reserved_input_registers_run_realm, firme_base_reserved_input_bits_run_ns,
        firme_base_reserved_input_bits_run_secure, firme_base_reserved_input_bits_run_realm
    };
    const firme_rule_fn_t concurrency[] = {
        firme_base_max_concurrent_r0064_run_ns, firme_base_max_concurrent_r0064_run_secure,
        firme_base_max_concurrent_r0064_run_realm, firme_base_max_concurrent_r0065_run_ns,
        firme_base_max_concurrent_r0065_run_secure, firme_base_max_concurrent_r0065_run_realm
    };
    firme_acs_call_res_t first = {.x0 = ACS_STATUS_PASS, .x1 = 1U, .x2 = UINT64_C(1) << 16};
    firme_acs_call_res_t second = {.x0 = ACS_STATUS_PASS, .x1 = 2U, .x2 = UINT64_C(1) << 16};
    firme_acs_call_res_t absent = {.x0 = ACS_STATUS_SKIP};
    firme_acs_call_res_t overall = {.x0 = ACS_STATUS_PASS};
    uint32_t index;

    for (index = 0U; index < 3U; index++) {
        g_fid_return = VAL_ERROR;
        check_result(smc64[index], VAL_ERROR, ACS_STATUS_FAIL);
        g_fid_return = VAL_SUCCESS;
        g_state_return = VAL_ERROR;
        check_result(smc64[index], VAL_ERROR, ACS_STATUS_FAIL);
        check_result(status_codes[index], VAL_ERROR, ACS_STATUS_FAIL);
        g_state_return = VAL_SUCCESS;
        check_result(smc64[index], VAL_SUCCESS, ACS_STATUS_PASS);
        check_result(status_codes[index], VAL_SUCCESS, ACS_STATUS_PASS);
        g_base_version = FIRME_SUCCESS;
        check_result(status_codes[index], VAL_ERROR, ACS_STATUS_FAIL);
        g_base_version = UINT64_C(0x10000);
        g_gm_version_status = FIRME_BUSY;
        check_result(status_codes[index], VAL_ERROR, ACS_STATUS_FAIL);
        g_gm_version_status = FIRME_SUCCESS;
        g_reg1_status = FIRME_NO_MEMORY;
        check_result(status_codes[index], VAL_ERROR, ACS_STATUS_FAIL);
        g_reg1_status = FIRME_SUCCESS;
        g_base_version = UINT64_C(0x10001);
        g_baseline_status = ACS_STATUS_FAIL;
        check_result(compatibility[index], VAL_ERROR, ACS_STATUS_FAIL);
        g_baseline_return = VAL_ERROR;
        check_result(compatibility[index], VAL_ERROR, ACS_STATUS_FAIL);
        g_baseline_status = ACS_STATUS_ERR;
        check_result(compatibility[index], VAL_ERROR, ACS_STATUS_ERR);
        g_baseline_return = VAL_SUCCESS;
        g_baseline_status = ACS_STATUS_SKIP;
        check_result(compatibility[index], VAL_SUCCESS, ACS_STATUS_SKIP);
        g_baseline_status = ACS_STATUS_PASS;
        check_result(compatibility[index], VAL_SUCCESS, ACS_STATUS_PASS);
        g_baseline_available = VAL_FALSE;
        check_result(compatibility[index], VAL_SUCCESS, ACS_STATUS_SKIP);
        g_baseline_available = VAL_TRUE;
    }
    g_base_version = UINT64_C(0x10000);
    for (index = 0U; index < 6U; index++) {
        g_capture_disable_count = 0U;
        check_result(input_audits[index], VAL_ERROR, ACS_STATUS_FAIL);
        assert(g_capture_enabled == VAL_FALSE && g_capture_disable_count == 1U);
        g_capture_read_return = VAL_ERROR;
        check_result(input_audits[index], VAL_ERROR, ACS_STATUS_ERR);
        assert(g_capture_enabled == VAL_FALSE && g_capture_disable_count == 2U);
        g_capture_read_return = VAL_SUCCESS;
        g_capture_enable_return = VAL_ERROR;
        check_result(input_audits[index], VAL_ERROR, ACS_STATUS_ERR);
        assert(g_capture_enabled == VAL_FALSE && g_capture_disable_count == 2U);
        g_capture_enable_return = VAL_SUCCESS;
        g_gm_version_status = FIRME_INVALID_PARAMETERS;
        check_result(concurrency[index], VAL_ERROR, ACS_STATUS_FAIL);
        g_gm_version_status = FIRME_SUCCESS;
        g_reg1_status = FIRME_NOT_SUPPORTED;
        check_result(concurrency[index], VAL_ERROR, ACS_STATUS_FAIL);
        g_reg1_status = FIRME_SUCCESS;
        check_result(concurrency[index], VAL_SUCCESS, ACS_STATUS_PASS);
    }
    assert(firme_base_max_concurrent_r0065_post(&first, &second, &absent, &overall) == VAL_ERROR);
    assert(overall.x0 == ACS_STATUS_FAIL);
    second.x1 = first.x1;
    assert(firme_base_max_concurrent_r0065_post(&first, &second, &absent, &overall) == VAL_SUCCESS);
    assert(overall.x0 == ACS_STATUS_SKIP);
    first.x1 = 0U;
    second.x1 = 0U;
    assert(firme_base_max_concurrent_r0065_post(&first, &second, &absent, &overall) == VAL_SUCCESS);
    assert(overall.x0 == ACS_STATUS_PASS);
    assert(firme_base_max_concurrent_r0065_post(NULL, &second, &absent, &overall) == VAL_ERROR);
    assert(overall.x0 == ACS_STATUS_ERR);
    assert(firme_base_max_concurrent_r0065_post(&first, &second, &absent, NULL) == VAL_ERROR);
    return VAL_SUCCESS;
}
"""


PAL_PAT_SOURCE = COMMON + r"""
#include "pal_firme_shared_buffer.h"
#include "val_common/val_mmu.h"
#include "val_firme_shared_buffer.h"

uint64_t g_val_mmu_l1[512];
static uint64_t g_current_el = 8ULL;
static uint64_t g_sctlr = 5ULL;
static uint64_t g_tcr = VAL_MMU_EL2_BOOT_TCR;
static uint64_t g_mair = VAL_MMU_BOOT_MAIR;
static uint64_t g_ttbr;
static uint64_t g_hcr;

/** @brief Supply the caller EL to the production precondition checker. */
uint64_t read_currentel(void)
{
    return g_current_el;
}

/** @brief Supply native SCTLR fixtures without executing privileged instructions. */
uint64_t read_sctlr_el2(void)
{
    return g_sctlr;
}

/** @brief Supply native TCR fixtures to the actual boot-map validator. */
uint64_t val_read_tcr_el2(void)
{
    return g_tcr;
}

/** @brief Model the assembly MAIR read for native precondition checks. */
uint64_t pal_firme_shared_buffer_read_mair_el2(void)
{
    return g_mair;
}

/** @brief Supply HCR fixtures to distinguish non-VHE from VHE translation. */
uint64_t pal_firme_shared_buffer_read_hcr_el2(void)
{
    return g_hcr;
}

/** @brief Model the assembly TTBR read for native precondition checks. */
uint64_t pal_firme_shared_buffer_read_ttbr0_el2(void)
{
    return g_ttbr;
}

#ifndef CHECK_BOOT_MAPPING
static uint32_t g_mapping_ready = VAL_TRUE;

/** @brief Test ownership separately from hardware mapping, not as DUT evidence. */
uint32_t pal_firme_shared_buffer_ns_mapping_ready(uint64_t base, uint64_t size)
{
    assert(base % PAL_FIRME_PAT_ALIGNMENT == 0ULL);
    assert(size == PAL_FIRME_PAT_BUFFER_SIZE);
    return g_mapping_ready;
}
#endif

/** @brief Regress the real mapping preconditions or the real PAL lease lifetime. */
int main(void)
{
#ifdef CHECK_BOOT_MAPPING
    const uint64_t base = UINT64_C(0x88001000);
    const uint64_t descriptor = UINT64_C(0x80000705);
    const uint64_t bad_bits[] = {UINT64_C(1), UINT64_C(4), UINT64_C(0x100),
                                UINT64_C(0x400), UINT64_C(0x80), UINT64_C(1) << 63,
                                UINT64_C(1) << 30};
    uint32_t index = 0U;

    g_ttbr = (uint64_t)(uintptr_t)g_val_mmu_l1;
    g_val_mmu_l1[2] = descriptor;
    assert(pal_firme_shared_buffer_ns_mapping_ready(base, 8192ULL) == VAL_TRUE);
    g_tcr |= UINT64_C(1) << 31;
    assert(pal_firme_shared_buffer_ns_mapping_ready(base, 8192ULL) == VAL_TRUE);
    g_tcr |= UINT64_C(1) << 23;
    assert(pal_firme_shared_buffer_ns_mapping_ready(base, 8192ULL) == VAL_TRUE);
    g_hcr = UINT64_C(1) << 34;
    assert(pal_firme_shared_buffer_ns_mapping_ready(base, 8192ULL) == VAL_FALSE);
    g_hcr = 0ULL;
    g_tcr |= UINT64_C(1) << 20;
    assert(pal_firme_shared_buffer_ns_mapping_ready(base, 8192ULL) == VAL_FALSE);
    g_tcr = VAL_MMU_EL2_BOOT_TCR;
    for (index = 0U; index < sizeof(bad_bits) / sizeof(bad_bits[0]); index++) {
        g_val_mmu_l1[2] = descriptor ^ bad_bits[index];
        assert(pal_firme_shared_buffer_ns_mapping_ready(base, 8192ULL) == VAL_FALSE);
    }
    g_val_mmu_l1[2] = descriptor;
    g_current_el = 4ULL;
    assert(pal_firme_shared_buffer_ns_mapping_ready(base, 8192ULL) == VAL_FALSE);
    g_current_el = 8ULL;
    g_sctlr = 1ULL;
    assert(pal_firme_shared_buffer_ns_mapping_ready(base, 8192ULL) == VAL_FALSE);
    g_sctlr = 5ULL;
    g_tcr++;
    assert(pal_firme_shared_buffer_ns_mapping_ready(base, 8192ULL) == VAL_FALSE);
    g_tcr = VAL_MMU_EL2_BOOT_TCR;
    g_mair = 0ULL;
    assert(pal_firme_shared_buffer_ns_mapping_ready(base, 8192ULL) == VAL_FALSE);
    g_mair = VAL_MMU_BOOT_MAIR;
    g_ttbr++;
    assert(pal_firme_shared_buffer_ns_mapping_ready(base, 8192ULL) == VAL_FALSE);
    g_ttbr = (uint64_t)(uintptr_t)g_val_mmu_l1;
    assert(pal_firme_shared_buffer_ns_mapping_ready(UINT64_MAX, 8192ULL) == VAL_FALSE);
    assert(pal_firme_shared_buffer_ns_mapping_ready(base, UINT64_MAX) == VAL_FALSE);
    assert(pal_firme_shared_buffer_ns_mapping_ready(base, 0ULL) == VAL_FALSE);
    assert(pal_firme_shared_buffer_ns_mapping_ready(UINT64_C(0x881ff000), 8192ULL) == VAL_FALSE);
#else
    val_firme_shared_buffer_lease_t first = {0};
    val_firme_shared_buffer_lease_t second = {0};
    val_firme_shared_buffer_lease_t stale = {0};

    assert(val_firme_shared_buffer_acquire_ns_pat(4096ULL, 4096ULL, NULL) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    assert(val_firme_shared_buffer_acquire_ns_pat(0ULL, 4096ULL, &first) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    assert(val_firme_shared_buffer_acquire_ns_pat(4096ULL, 3ULL, &first) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    assert(val_firme_shared_buffer_acquire_ns_pat(8193ULL, 4096ULL, &first) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
    assert(val_firme_shared_buffer_acquire_ns_pat(4096ULL, 16384ULL, &first) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
    g_mapping_ready = VAL_FALSE;
    assert(val_firme_shared_buffer_acquire_ns_pat(4096ULL, 4096ULL, &first) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
    assert(first.base_va == NULL && first.owner_token == 0ULL);
    g_mapping_ready = VAL_TRUE;
    assert(val_firme_shared_buffer_acquire_ns_pat(8192ULL, 4096ULL, &first) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE);
    assert(first.base_pa == (uint64_t)(uintptr_t)first.base_va && first.capacity == 8192ULL);
    assert(first.challenge_size == 32U && first.challenge != first.base_va);
    assert(first.owner_token != 0ULL);
    assert(val_firme_shared_buffer_acquire_ns_pat(4096ULL, 4096ULL, &second) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
    assert(val_firme_shared_buffer_release_ns_pat(&second, VAL_FALSE) == VAL_ERROR);
    assert(val_firme_shared_buffer_release_ns_pat(&first, 2U) == VAL_ERROR);
    assert(val_firme_shared_buffer_release_ns_pat(NULL, VAL_FALSE) == VAL_ERROR);
    stale = first;
    assert(val_firme_shared_buffer_release_ns_pat(&first, VAL_FALSE) == VAL_SUCCESS);
    assert(val_firme_shared_buffer_release_ns_pat(&first, VAL_FALSE) == VAL_ERROR);
    assert(val_firme_shared_buffer_acquire_ns_pat(4097ULL, 4096ULL, &second) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE);
    assert(stale.owner_token != second.owner_token);
    assert(val_firme_shared_buffer_release_ns_pat(&stale, VAL_FALSE) == VAL_ERROR);
    assert(val_firme_shared_buffer_release_ns_pat(&second, VAL_TRUE) == VAL_SUCCESS);
    assert(val_firme_shared_buffer_acquire_ns_pat(4096ULL, 4096ULL, &first) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
    assert(val_firme_shared_buffer_release_ns_pat(&second, VAL_FALSE) == VAL_ERROR);
#endif
    return VAL_SUCCESS;
}
"""


class CallerChecksTests(unittest.TestCase):
    """Exercise production callers against controlled responses and failures."""

    def run_native(self, source, files, replacements=None):
        """Compile production C with controlled native firmware substitutes."""
        with tempfile.TemporaryDirectory(prefix="firme-abi-tests-") as directory:
            temporary = Path(directory)
            source_path = temporary / "test.c"
            binary = temporary / "test"
            source_path.write_text(source)
            command = [os.environ.get("HOST_CC", "cc"), "-std=c11", "-Wall",
                       "-Wextra", "-Werror", "-fsanitize=undefined"]
            for include in ("val/include", "val/src", "val_common/include", "test/base",
                            "platform/pal_baremetal/targets/fvp/include"):
                command.extend(["-I", str(ROOT / include)])
            command.append(str(source_path))
            for filename in files:
                original = ROOT / filename
                if replacements and filename in replacements:
                    original = temporary / Path(filename).name
                    original.write_text(replacements[filename])
                command.append(str(original))
            command.extend(["-o", str(binary)])
            compiled = subprocess.run(command, cwd=temporary, capture_output=True,
                                      text=True, check=False)
            self.assertEqual(compiled.returncode, 0, compiled.stderr)
            return subprocess.run([str(binary)], cwd=temporary, capture_output=True,
                                  text=True, check=False)

    def test_smccc_probe_and_versions(self):
        """Check preservation, permitted statuses and W0 version decoding."""
        result = self.run_native(SMCCC_SOURCE, [
            "val/src/val_firme_abi.c", "val/src/val_firme_smccc_probe.c",
            "test/base/smccc_version.c", "test/base/smccc_sve.c", "test/base/smccc_sme.c",
        ])
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_framework_lifecycle(self):
        """Reject missing mandatory coverage, invalid evidence and cleanup failures."""
        result = self.run_native(FRAMEWORK_SOURCE, [
            "val/src/val_firme_abi.c", "val/src/val_firme_service.c",
            "val/src/val_firme_abi_test.c",
        ])
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_real_callers_and_poisoning(self):
        """Inject unused outputs into the actual production wrapper consumers."""
        result = self.run_native(check_abi_callers.SOURCE, self.caller_files())
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_pat_driver_and_cleanup(self):
        """Exercise real PAT frames, status-dependent outputs and durable ownership."""
        result = self.run_native(check_abi_callers.PAT_SOURCE, check_abi_callers.PAT_FILES)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_fvp_pat_lease_and_mapping(self):
        """Check actual PAL lease ownership and live-map validation independently."""
        for prefix in ("", "#define CHECK_BOOT_MAPPING\n"):
            with self.subTest(mapping=bool(prefix)):
                result = self.run_native(prefix + PAL_PAT_SOURCE, [
                    "platform/pal_baremetal/targets/fvp/src/pal_firme_shared_buffer.c",
                ])
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_pat_poisoning_catches_decoder(self):
        """Reject a production decoder mutated to consume unused SUCCESS X2."""
        filename = "val/src/val_firme_shared_buffer.c"
        original = (ROOT / filename).read_text()
        mutated = original.replace(
            "return response->written_size != 0ULL ? VAL_TRUE : VAL_FALSE;",
            "return (response->written_size != 0ULL && response->remaining_size == 0ULL) "
            "? VAL_TRUE : VAL_FALSE;").replace(
                "if (response->abi_status == (int64_t)FIRME_INCOMPLETE) {",
                "if (val_firme_shared_buffer_status_accepted(response->abi_status) == VAL_TRUE) {")
        self.assertNotEqual(original, mutated)
        result = self.run_native(check_abi_callers.PAT_SOURCE, check_abi_callers.PAT_FILES,
                                 {filename: mutated})
        self.assertNotEqual(result.returncode, 0)

    def test_poisoning_catches_decoder(self):
        """Prove the caller check detects a deliberately corrupted decoder."""
        filename = "val/src/val_firme_shared_buffer.c"
        original = (ROOT / filename).read_text()
        mutated = original.replace("FIRME_BASE_MAX_SH_BUF_PG_CNT_MASK;", "UINT64_MAX;")
        self.assertNotEqual(original, mutated)
        result = self.run_native(check_abi_callers.SOURCE, self.caller_files(), {filename: mutated})
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("firme_base_reserved_output_bits_run_ns", result.stderr)

    def test_chunk_and_buffer_evidence(self):
        """Require valid service-specific chunk contracts and buffer boundaries."""
        files = ["val/src/val_firme_abi.c", "val/src/val_firme_service.c",
                 "val/src/val_firme_shared_buffer.c", "test/base/shared_buffer_test_common.c",
                 "test/base/shared_buffer_size_bounds.c", "test/base/shared_buffer_alignment.c",
                 "test/base/partial_retrieval_test_common.c",
                 "test/base/partial_retrieval_last_chunk.c",
                 "test/base/partial_retrieval_intermediate_chunk.c",
                 "test/base/partial_retrieval_no_chunk.c"]
        for prefix in ("", "#define OMIT_STATUS_CONTRACT\n"):
            with self.subTest(status_contract=not prefix):
                result = self.run_native(prefix + CONCEPT_SOURCE, files)
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_feature_register_fields(self):
        """Ignore unused bits without ignoring missing mandatory feature fields."""
        result = self.run_native(FEATURE_SOURCE, [
            "val/src/val_firme_abi.c", "val/src/val_firme_service.c",
        ])
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_return_and_capture_cleanup(self):
        """Keep body returns, recorded results and capture cleanup consistent."""
        result = self.run_native(RETURN_SOURCE, [
            "val/src/val_firme_abi.c", "val/src/val_firme_service.c",
            "val/src/val_firme_abi_catalog.c",
        ])
        self.assertEqual(result.returncode, 0, result.stderr)

    @staticmethod
    def caller_files():
        """Production sources needed by the caller audit."""
        return ["val/src/val_firme_abi.c", "val/src/val_firme_service.c",
                "val/src/val_firme_shared_buffer.c", "test/base/reserved_output_bits.c",
                "test/base/reserved_output_registers.c"]


if __name__ == "__main__":
    unittest.main()
