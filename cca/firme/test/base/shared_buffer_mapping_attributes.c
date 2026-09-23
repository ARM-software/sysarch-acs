/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "shared_buffer_test_common.h"

#include "val_firme_abi.h"
#include "val_firme_shared_buffer.h"

/**
 *   @brief    - Validate R0070 shared-buffer mapping attributes.
 *   @param    - world   : FIRME caller world.
 *   @param    - base_pa : Unused for shared-buffer concept tests.
 *   @param    - out     : Result record.
 *   @return   - VAL_SUCCESS when the rule produced an outcome, else VAL_ERROR.
 **/
static int shared_buffer_mapping_attributes_run(
    firme_acs_world_t world, uint64_t base_pa, firme_rule_res_t *out)
{
    shared_buffer_limits_t limits = {0};
    val_firme_shared_buffer_evidence_t evidence = {0};
    uint32_t required_fields =
        VAL_FIRME_SHARED_BUFFER_EVIDENCE_CALLER_ATTRS |
        VAL_FIRME_SHARED_BUFFER_EVIDENCE_EL3_ATTRS;
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
        out->status = (uint64_t)ACS_STATUS_ERR;
        firme_shared_buffer_test_fail(out, SHARED_BUFFER_STEP_EVIDENCE,
                           evidence.valid_fields, required_fields);
        return VAL_ERROR;
    }

    if (val_firme_shared_buffer_status_accepted(evidence.abi_status) == VAL_FALSE) {
        firme_shared_buffer_test_fail(out, SHARED_BUFFER_STEP_STATUS,
                           (uint64_t)evidence.abi_status, 0ULL);
        return VAL_SUCCESS;
    }

    if ((evidence.caller_attributes & VAL_FIRME_SHARED_BUFFER_REQUIRED_ATTRS) !=
        VAL_FIRME_SHARED_BUFFER_REQUIRED_ATTRS) {
        firme_shared_buffer_test_fail(out, SHARED_BUFFER_STEP_CALLER_ATTRIBUTES,
                           evidence.caller_attributes,
                           VAL_FIRME_SHARED_BUFFER_REQUIRED_ATTRS);
        return VAL_SUCCESS;
    }

    if ((evidence.el3_attributes & VAL_FIRME_SHARED_BUFFER_REQUIRED_ATTRS) !=
        VAL_FIRME_SHARED_BUFFER_REQUIRED_ATTRS) {
        firme_shared_buffer_test_fail(out, SHARED_BUFFER_STEP_EL3_ATTRIBUTES,
                           evidence.el3_attributes,
                           VAL_FIRME_SHARED_BUFFER_REQUIRED_ATTRS);
        return VAL_SUCCESS;
    }

    out->status = (uint64_t)ACS_STATUS_PASS;
    return VAL_SUCCESS;
}

/**
 *   @brief    - Execute R0070 from the NS FIRME instance.
 *   @param    - base_pa : Unused for shared-buffer concept tests.
 *   @param    - out     : Result record.
 *   @return   - VAL_SUCCESS when the rule produced an outcome, else VAL_ERROR.
 **/
int firme_base_shared_buffer_mapping_attributes_run_ns(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return shared_buffer_mapping_attributes_run(
        FIRME_ACS_WORLD_NSEL2, base_pa, out);
}

/**
 *   @brief    - Execute R0070 from the Secure FIRME instance.
 *   @param    - base_pa : Unused for shared-buffer concept tests.
 *   @param    - out     : Result record.
 *   @return   - VAL_SUCCESS when the rule produced an outcome, else VAL_ERROR.
 **/
int firme_base_shared_buffer_mapping_attributes_run_secure(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return shared_buffer_mapping_attributes_run(
        FIRME_ACS_WORLD_SECURE, base_pa, out);
}

/**
 *   @brief    - Execute R0070 from the Realm FIRME instance.
 *   @param    - base_pa : Unused for shared-buffer concept tests.
 *   @param    - out     : Result record.
 *   @return   - VAL_SUCCESS when the rule produced an outcome, else VAL_ERROR.
 **/
int firme_base_shared_buffer_mapping_attributes_run_realm(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return shared_buffer_mapping_attributes_run(
        FIRME_ACS_WORLD_REALM, base_pa, out);
}
