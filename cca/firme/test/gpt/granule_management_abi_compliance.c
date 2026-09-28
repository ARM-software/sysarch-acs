/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_firme_gpt.h"
#include "val_firme_rules.h"
#include "val_firme_service.h"
#include "val_log.h"

#define GM_ABI_COMPLIANCE_TEST_NAME "granule_management_abi_compliance"

#define GM_ABI_COMPLIANCE_STEP_VERSION          UINT64_C(1)
#define GM_ABI_COMPLIANCE_STEP_REG0_STATUS      UINT64_C(2)
#define GM_ABI_COMPLIANCE_STEP_PROHIBITED_BITS  UINT64_C(4)
#define GM_ABI_COMPLIANCE_STEP_MANDATORY_BITS   UINT64_C(5)
#define GM_ABI_COMPLIANCE_STEP_L1_OPTIONAL_PAIR UINT64_C(6)

/**
 *   @brief    - Initialize a rule result for GM ABI-compliance checks.
 *   @param    - out : Optional result record.
 *   @return   - void
 **/
static void gm_abi_compliance_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a GM ABI-compliance failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void
 **/
static void gm_abi_compliance_fail(firme_rule_res_t *out, uint64_t fail_step,
                                   uint64_t last_x0, uint64_t last_x1)
{
    LOG_ERROR(NULL, "%s: failed step=%llu x0=%llx x1=%llx",
              GM_ABI_COMPLIANCE_TEST_NAME, (unsigned long long)fail_step,
              (unsigned long long)last_x0, (unsigned long long)last_x1);

    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Validate Table 3.10 ABI compliance for one FIRME instance.
 *   @param    - world : Current caller world.
 *   @param    - out   : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
static int gm_abi_compliance_run(firme_acs_world_t world, firme_rule_res_t *out)
{
    firme_res_t res = {0};
    uint64_t feature_reg0 = 0ULL;
    uint64_t allocated_bits = 0ULL;
    uint64_t supported_bits = 0ULL;
    uint64_t mandatory_bits = 0ULL;
    uint64_t create_bit = 0ULL;
    uint64_t destroy_bit = 0ULL;
    int64_t status = 0;

    if (!out)
        return VAL_ERROR;

    gm_abi_compliance_init_result(out);

    /* Step 1: Skip only when Granule Management is absent at this instance. */
    val_firme_service_version(FIRME_SERVICE_ID_GRANULE_MGMT, &res);
    status = val_firme_status_to_i64(res.x0);
    if (status == (int64_t)FIRME_NOT_SUPPORTED) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return val_firme_gpt_rule_result_status(out);
    }
    if (status < 0) {
        gm_abi_compliance_fail(out, GM_ABI_COMPLIANCE_STEP_VERSION, res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 2: Read Granule Management feature register 0. */
    val_firme_service_features(FIRME_SERVICE_ID_GRANULE_MGMT,
                               FIRME_GM_FEATURE_REG0_INDEX, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        gm_abi_compliance_fail(out, GM_ABI_COMPLIANCE_STEP_REG0_STATUS,
                               res.x0, res.x1);
        return VAL_ERROR;
    }
    feature_reg0 = res.x1;

    allocated_bits = val_firme_service_allocated_abi_mask(FIRME_SERVICE_ID_GRANULE_MGMT);
    supported_bits = val_firme_service_supported_abi_mask(world,
                                                          FIRME_SERVICE_ID_GRANULE_MGMT);
    mandatory_bits = val_firme_service_mandatory_abi_mask(world,
                                                          FIRME_SERVICE_ID_GRANULE_MGMT);

    /* Unused output bits have no required value and are ignored by the caller. */
    /* Step 4: Not-supported ABIs for this instance must not be advertised. */
    if ((feature_reg0 & (allocated_bits & ~supported_bits)) != 0ULL) {
        gm_abi_compliance_fail(out, GM_ABI_COMPLIANCE_STEP_PROHIBITED_BITS,
                               res.x0, feature_reg0);
        return VAL_ERROR;
    }

    /* Step 5: Mandatory ABIs for this instance must be advertised. */
    if ((feature_reg0 & mandatory_bits) != mandatory_bits) {
        gm_abi_compliance_fail(out, GM_ABI_COMPLIANCE_STEP_MANDATORY_BITS,
                               res.x0, feature_reg0);
        return VAL_ERROR;
    }

    /* Step 6: Realm optional L1 GPT create/destroy support is paired. */
    if (world == FIRME_ACS_WORLD_REALM) {
        create_bit = feature_reg0 & FIRME_GM_FEATURE_REG0_L1_CREATE_BIT;
        destroy_bit = feature_reg0 & FIRME_GM_FEATURE_REG0_L1_DESTROY_BIT;
        if ((create_bit == 0ULL) != (destroy_bit == 0ULL)) {
            gm_abi_compliance_fail(out, GM_ABI_COMPLIANCE_STEP_L1_OPTIONAL_PAIR,
                                   res.x0, feature_reg0);
            return VAL_ERROR;
        }
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return val_firme_gpt_rule_result_status(out);
}

/**
 *   @brief    - GM ABI-compliance rule body for NS caller execution.
 *   @param    - base_pa : Unused for compliance-discovery tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_gpt_granule_management_abi_compliance_run_ns(uint64_t base_pa,
                                                       firme_rule_res_t *out)
{
    (void)base_pa;
    return gm_abi_compliance_run(FIRME_ACS_WORLD_NSEL2, out);
}

/**
 *   @brief    - GM ABI-compliance rule body for Secure caller execution.
 *   @param    - base_pa : Unused for compliance-discovery tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_gpt_granule_management_abi_compliance_run_secure(uint64_t base_pa,
                                                           firme_rule_res_t *out)
{
    (void)base_pa;
    return gm_abi_compliance_run(FIRME_ACS_WORLD_SECURE, out);
}

/**
 *   @brief    - GM ABI-compliance rule body for Realm caller execution.
 *   @param    - base_pa : Unused for compliance-discovery tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_gpt_granule_management_abi_compliance_run_realm(uint64_t base_pa,
                                                          firme_rule_res_t *out)
{
    (void)base_pa;
    return gm_abi_compliance_run(FIRME_ACS_WORLD_REALM, out);
}
