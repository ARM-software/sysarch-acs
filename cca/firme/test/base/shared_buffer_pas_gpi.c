/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "shared_buffer_test_common.h"

#include "val_firme_abi.h"
#include "val_firme_shared_buffer.h"
#include "val_log.h"

/**
 *   @brief    - Validate R0071 shared-buffer PAS and GPI correspondence.
 *   @param    - world   : FIRME caller world.
 *   @param    - base_pa : Unused for shared-buffer concept tests.
 *   @param    - out     : Result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
static int shared_buffer_pas_gpi_run(
    firme_acs_world_t world, uint64_t base_pa, firme_rule_res_t *out)
{
    shared_buffer_limits_t limits = {0};
    val_firme_shared_buffer_evidence_t evidence = {0};
    uint32_t expected_gpi = 0U;
    uint32_t required_fields =
        VAL_FIRME_SHARED_BUFFER_EVIDENCE_PAS |
        VAL_FIRME_SHARED_BUFFER_EVIDENCE_GPI;
    int result = VAL_ERROR;

    (void)base_pa;
    result = firme_shared_buffer_test_setup(world, out, &limits);
    if ((result != VAL_SUCCESS) ||
        (out->status != (uint64_t)ACS_STATUS_FAIL))
        return result;

    result = firme_shared_buffer_test_collect(world, VAL_FIRME_SHARED_BUFFER_CASE_VALID,
                                   &limits, out, &evidence);
    if ((result != VAL_SUCCESS) ||
        (out->status != (uint64_t)ACS_STATUS_FAIL))
        return result;

    if ((evidence.valid_fields & required_fields) != required_fields) {
        if ((evidence.valid_fields & VAL_FIRME_SHARED_BUFFER_OBSERVATION_UNAVAILABLE) != 0U) {
            LOG_WARN(NULL, "shared_buffer: independent PAS/GPT evidence unavailable");
            out->status = (uint64_t)ACS_STATUS_SKIP;
            out->fail_step = SHARED_BUFFER_STEP_EVIDENCE;
            return VAL_SUCCESS;
        }
        out->status = (uint64_t)ACS_STATUS_ERR;
        firme_shared_buffer_test_fail(out, SHARED_BUFFER_STEP_EVIDENCE,
                           evidence.valid_fields, required_fields);
        return VAL_ERROR;
    }

    if (val_firme_shared_buffer_status_accepted(evidence.abi_status) == VAL_FALSE) {
        firme_shared_buffer_test_fail(out, SHARED_BUFFER_STEP_STATUS,
                           (uint64_t)evidence.abi_status, 0ULL);
        return VAL_ERROR;
    }

    if (evidence.pas_world != world) {
        firme_shared_buffer_test_fail(out, SHARED_BUFFER_STEP_PAS,
                           (uint64_t)evidence.pas_world, (uint64_t)world);
        return VAL_ERROR;
    }

    expected_gpi = val_firme_shared_buffer_expected_gpi(world);
    if ((expected_gpi == 0U) || (evidence.gpi != expected_gpi)) {
        firme_shared_buffer_test_fail(out, SHARED_BUFFER_STEP_GPI,
                           (uint64_t)evidence.gpi, (uint64_t)expected_gpi);
        return VAL_ERROR;
    }

    out->status = (uint64_t)ACS_STATUS_PASS;
    return VAL_SUCCESS;
}

/**
 *   @brief    - Execute R0071 from the NS FIRME instance.
 *   @param    - base_pa : Unused for shared-buffer concept tests.
 *   @param    - out     : Result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_base_shared_buffer_pas_gpi_run_ns(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return shared_buffer_pas_gpi_run(FIRME_ACS_WORLD_NSEL2, base_pa, out);
}

/**
 *   @brief    - Execute R0071 from the Secure FIRME instance.
 *   @param    - base_pa : Unused for shared-buffer concept tests.
 *   @param    - out     : Result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_base_shared_buffer_pas_gpi_run_secure(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return shared_buffer_pas_gpi_run(FIRME_ACS_WORLD_SECURE, base_pa, out);
}

/**
 *   @brief    - Execute R0071 from the Realm FIRME instance.
 *   @param    - base_pa : Unused for shared-buffer concept tests.
 *   @param    - out     : Result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_base_shared_buffer_pas_gpi_run_realm(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return shared_buffer_pas_gpi_run(FIRME_ACS_WORLD_REALM, base_pa, out);
}
