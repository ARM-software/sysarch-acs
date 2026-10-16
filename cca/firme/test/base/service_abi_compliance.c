/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_firme_rules.h"
#include "val_firme_service.h"
#include "val_log.h"

#define SERVICE_ABI_COMPLIANCE_TEST_NAME "service_abi_compliance"

#define SERVICE_ABI_COMPLIANCE_STEP_VERSION_STATUS    UINT64_C(1)
#define SERVICE_ABI_COMPLIANCE_STEP_FEATURES_STATUS   UINT64_C(2)
#define SERVICE_ABI_COMPLIANCE_STEP_PROHIBITED_BITS   UINT64_C(3)
#define SERVICE_ABI_COMPLIANCE_STEP_SERVICE_FORBIDDEN UINT64_C(5)
#define SERVICE_ABI_COMPLIANCE_STEP_MANDATORY_BITS    UINT64_C(6)
#define SERVICE_ABI_COMPLIANCE_STEP_OPTIONAL_GROUP    UINT64_C(7)
#define SERVICE_ABI_COMPLIANCE_STEP_ABI_DEPENDENCY    UINT64_C(8)

/**
 *   @brief    - Initialize a service ABI compliance result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void service_abi_compliance_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a service ABI compliance failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void.
 **/
static void service_abi_compliance_fail(firme_rule_res_t *out, uint64_t fail_step,
                                        uint64_t last_x0, uint64_t last_x1)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Validate one advertised service against ABI classifications.
 *   @param    - world      : Caller world represented by the current payload.
 *   @param    - service_id : FIRME service ID.
 *   @param    - out        : Optional result record.
 *   @return   - VAL_TRUE if the service classification is valid, otherwise VAL_FALSE.
 **/
static uint32_t service_abi_compliance_check_service(firme_acs_world_t world,
                                                     uint32_t service_id,
                                                     firme_rule_res_t *out)
{
    firme_res_t res = {0};
    uint64_t allocated_mask = 0ULL;
    uint64_t mandatory_mask = 0ULL;
    uint64_t supported_mask = 0ULL;
    uint64_t feature_reg0 = 0ULL;
    uint64_t optional_group = 0ULL;
    uint32_t advertised = VAL_FALSE;

    if (val_firme_service_get_feature_reg0(service_id, &advertised, &feature_reg0, &res) !=
        VAL_SUCCESS) {
        LOG_ERROR(NULL, "%s: service_id=%u reg0 unexpected x0=%llx",
                  SERVICE_ABI_COMPLIANCE_TEST_NAME, (unsigned int)service_id,
                  (unsigned long long)res.x0);
        service_abi_compliance_fail(out, SERVICE_ABI_COMPLIANCE_STEP_FEATURES_STATUS,
                                    res.x0, (uint64_t)service_id);
        return VAL_FALSE;
    }

    if (advertised == VAL_FALSE) {
        LOG_ERROR(NULL, "%s: implemented service_id=%u has no feature reg0",
                  SERVICE_ABI_COMPLIANCE_TEST_NAME, (unsigned int)service_id);
        service_abi_compliance_fail(out, SERVICE_ABI_COMPLIANCE_STEP_FEATURES_STATUS,
                                    res.x0, (uint64_t)service_id);
        return VAL_FALSE;
    }

    allocated_mask = val_firme_service_allocated_abi_mask(service_id);
    mandatory_mask = val_firme_service_mandatory_abi_mask(world, service_id);
    supported_mask = val_firme_service_supported_abi_mask(world, service_id);

    if ((feature_reg0 & mandatory_mask) != mandatory_mask) {
        LOG_ERROR(NULL, "%s: service_id=%u mandatory bits missing value=%llx mask=%llx",
                  SERVICE_ABI_COMPLIANCE_TEST_NAME, (unsigned int)service_id,
                  (unsigned long long)feature_reg0, (unsigned long long)mandatory_mask);
        service_abi_compliance_fail(out, SERVICE_ABI_COMPLIANCE_STEP_MANDATORY_BITS,
                                    feature_reg0, (uint64_t)service_id);
        return VAL_FALSE;
    }

    if ((feature_reg0 & (allocated_mask & ~supported_mask)) != 0ULL) {
        LOG_ERROR(NULL, "%s: service_id=%u prohibited bits set value=%llx mask=%llx",
                  SERVICE_ABI_COMPLIANCE_TEST_NAME, (unsigned int)service_id,
                  (unsigned long long)feature_reg0,
                  (unsigned long long)(allocated_mask & ~supported_mask));
        service_abi_compliance_fail(out, SERVICE_ABI_COMPLIANCE_STEP_PROHIBITED_BITS,
                                    feature_reg0, (uint64_t)service_id);
        return VAL_FALSE;
    }

    if ((service_id == FIRME_SERVICE_ID_ATTESTATION) &&
        ((feature_reg0 & FIRME_ATTEST_FEATURE_REG0_RAT_SIGN_BIT) != 0ULL) &&
        ((feature_reg0 & FIRME_ATTEST_FEATURE_REG0_RAK_GET_BIT) == 0ULL)) {
        LOG_ERROR(NULL, "%s: service_id=%u RAT_SIGN requires RAK_GET value=%llx",
                  SERVICE_ABI_COMPLIANCE_TEST_NAME, (unsigned int)service_id,
                  (unsigned long long)feature_reg0);
        service_abi_compliance_fail(out, SERVICE_ABI_COMPLIANCE_STEP_ABI_DEPENDENCY,
                                    feature_reg0, (uint64_t)service_id);
        return VAL_FALSE;
    }

    /* Optional ABI groups must be implemented together, not independently. */
    if (service_id == FIRME_SERVICE_ID_GRANULE_MGMT)
        optional_group = FIRME_GM_FEATURE_REG0_L1_CREATE_BIT |
                         FIRME_GM_FEATURE_REG0_L1_DESTROY_BIT;
    else if (service_id == FIRME_SERVICE_ID_ATTESTATION)
        optional_group = FIRME_ATTEST_FEATURE_REG0_EXT_CLAIMS_STAGE_BIT |
                         FIRME_ATTEST_FEATURE_REG0_EXT_CLAIMS_FINAL_BIT |
                         FIRME_ATTEST_FEATURE_REG0_EXT_CLAIMS_CLEAR_BIT;

    if ((feature_reg0 & optional_group) != 0ULL &&
        (feature_reg0 & optional_group) != optional_group) {
        LOG_ERROR(NULL, "%s: service_id=%u partial optional group value=%llx mask=%llx",
                  SERVICE_ABI_COMPLIANCE_TEST_NAME, (unsigned int)service_id,
                  (unsigned long long)feature_reg0, (unsigned long long)optional_group);
        service_abi_compliance_fail(out, SERVICE_ABI_COMPLIANCE_STEP_OPTIONAL_GROUP,
                                    feature_reg0, (uint64_t)service_id);
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 *   @brief    - Validate service ABI compliance classifications for one FIRME instance.
 *   @param    - world : Caller world represented by the current payload.
 *   @param    - out   : Optional result record.
 *   @return   - VAL_SUCCESS for PASS, otherwise VAL_ERROR.
 **/
static int service_abi_compliance_run(firme_acs_world_t world, firme_rule_res_t *out)
{
    firme_res_t res = {0};
    uint32_t service_id = 0U;
    int64_t status = 0;
    uint32_t implemented = VAL_FALSE;

    service_abi_compliance_init_result(out);

    /* Step 1-5: Advertised services must obey per-world ABI classifications. */
    for (service_id = FIRME_SERVICE_ID_BASE; service_id < FIRME_SERVICE_ID_MAX; service_id++) {
        implemented = VAL_FALSE;
        if (val_firme_service_get_version(service_id, &implemented, &res) != VAL_SUCCESS) {
            status = val_firme_status_to_i64(res.x0);
            LOG_ERROR(NULL, "%s: service_id=%u version unexpected status=%lld",
                      SERVICE_ABI_COMPLIANCE_TEST_NAME, (unsigned int)service_id,
                      (long long)status);
            service_abi_compliance_fail(out, SERVICE_ABI_COMPLIANCE_STEP_VERSION_STATUS,
                                        res.x0, (uint64_t)service_id);
            return VAL_ERROR;
        }

        if (implemented == VAL_FALSE)
            continue;

        if (val_firme_service_is_allowed_at_world(world, service_id) == VAL_FALSE) {
            LOG_ERROR(NULL, "%s: service_id=%u implemented at forbidden instance",
                      SERVICE_ABI_COMPLIANCE_TEST_NAME, (unsigned int)service_id);
            service_abi_compliance_fail(out, SERVICE_ABI_COMPLIANCE_STEP_SERVICE_FORBIDDEN,
                                        res.x0, (uint64_t)service_id);
            return VAL_ERROR;
        }

        if (service_abi_compliance_check_service(world, service_id, out) == VAL_FALSE)
            return VAL_ERROR;
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Service ABI compliance rule body for NS caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS, otherwise VAL_ERROR.
 **/
int firme_base_service_abi_compliance_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_abi_compliance_run(FIRME_ACS_WORLD_NSEL2, out);
}

/**
 *   @brief    - Service ABI compliance rule body for Secure caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS, otherwise VAL_ERROR.
 **/
int firme_base_service_abi_compliance_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_abi_compliance_run(FIRME_ACS_WORLD_SECURE, out);
}

/**
 *   @brief    - Service ABI compliance rule body for Realm caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS, otherwise VAL_ERROR.
 **/
int firme_base_service_abi_compliance_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_abi_compliance_run(FIRME_ACS_WORLD_REALM, out);
}
