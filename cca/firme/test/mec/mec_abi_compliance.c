/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_firme_mec.h"
#include "val_firme_rules.h"
#include "val_log.h"

#define MEC_ABI_COMPLIANCE_TEST_NAME "mec_abi_compliance"
#define MEC_ABI_COMPLIANCE_STEP_VERSION UINT64_C(1)
#define MEC_ABI_COMPLIANCE_STEP_FEATURE UINT64_C(2)
#define MEC_ABI_COMPLIANCE_STEP_CALL UINT64_C(3)
#define MEC_ABI_COMPLIANCE_STEP_CLEANUP UINT64_C(4)
#define MEC_ABI_COMPLIANCE_TEST_MECID UINT64_C(1)

/**
 * @brief Record a failing MEC ABI compliance check.
 * @param out Rule result.
 * @param step Failing check number.
 * @param result DUT response.
 * @return VAL_ERROR.
 */
static int mec_abi_compliance_fail(firme_rule_res_t *out, uint64_t step,
                                   const firme_res_t *result)
{
    out->status = (uint64_t)ACS_STATUS_FAIL;
    out->fail_step = step;
    out->last_x0 = result->x0;
    out->last_x1 = result->x1;
    LOG_ERROR(NULL, "%s: step=%llu x0=%llx x1=%llx",
              MEC_ABI_COMPLIANCE_TEST_NAME, (unsigned long long)step,
              (unsigned long long)result->x0, (unsigned long long)result->x1);
    return VAL_ERROR;
}

/**
 * @brief Verify that a non-Realm FIRME instance has no MEC service or ABI.
 * @param out Rule result.
 * @return VAL_SUCCESS on PASS, otherwise VAL_ERROR.
 */
static int mec_abi_compliance_disallowed_world(firme_rule_res_t *out)
{
    firme_res_t result = {0};
    uint64_t params = val_firme_mec_params(MEC_ABI_COMPLIANCE_TEST_MECID,
                                            FIRME_MEC_REFRESH_REASON_CREATE);

    if (!out)
        return VAL_ERROR;

    out->status = (uint64_t)ACS_STATUS_FAIL;
    out->fail_step = 0ULL;
    out->last_x0 = 0ULL;
    out->last_x1 = 0ULL;

    /* Step 1: The MECID management service is absent from this instance. */
    val_firme_service_version(FIRME_SERVICE_ID_MECID_MGMT, &result);
    if (val_firme_status_to_i64(result.x0) != (int64_t)FIRME_NOT_SUPPORTED)
        return mec_abi_compliance_fail(out, MEC_ABI_COMPLIANCE_STEP_VERSION,
                                       &result);

    /* Step 2: A direct MEC refresh request must be rejected. */
    val_firme_mec_refresh(params, &result);
    if (val_firme_status_to_i64(result.x0) != (int64_t)FIRME_NOT_SUPPORTED)
        return mec_abi_compliance_fail(out, MEC_ABI_COMPLIANCE_STEP_CALL,
                                       &result);

    out->status = (uint64_t)ACS_STATUS_PASS;
    return VAL_SUCCESS;
}

/**
 * @brief Check MEC service compliance from the Normal world.
 * @param base_pa Unused platform test address.
 * @param out Rule result.
 * @return VAL_SUCCESS on PASS, otherwise VAL_ERROR.
 */
int firme_mec_abi_compliance_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return mec_abi_compliance_disallowed_world(out);
}

/**
 * @brief Check MEC service compliance from the Secure world.
 * @param base_pa Unused platform test address.
 * @param out Rule result.
 * @return VAL_SUCCESS on PASS, otherwise VAL_ERROR.
 */
int firme_mec_abi_compliance_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return mec_abi_compliance_disallowed_world(out);
}

/**
 * @brief Check mandatory MEC_REFRESH implementation from the Realm world.
 * @param base_pa Unused platform test address.
 * @param out Rule result.
 * @return VAL_SUCCESS on PASS, otherwise VAL_ERROR.
 */
int firme_mec_abi_compliance_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    firme_res_t result = {0};
    uint64_t params = val_firme_mec_params(MEC_ABI_COMPLIANCE_TEST_MECID,
                                            FIRME_MEC_REFRESH_REASON_CREATE);

    (void)base_pa;
    if (!out)
        return VAL_ERROR;

    out->status = (uint64_t)ACS_STATUS_FAIL;
    out->fail_step = 0ULL;
    out->last_x0 = 0ULL;
    out->last_x1 = 0ULL;

    /* Step 1: Realm service must report an implemented version. */
    val_firme_service_version(FIRME_SERVICE_ID_MECID_MGMT, &result);
    if (val_firme_status_to_i64(result.x0) <= 0)
        return mec_abi_compliance_fail(out, MEC_ABI_COMPLIANCE_STEP_VERSION,
                                       &result);

    /* Step 2: The mandatory MEC_REFRESH ABI bit must be advertised. */
    val_firme_service_features(FIRME_SERVICE_ID_MECID_MGMT,
                               FIRME_MECID_FEATURE_REG0_INDEX, &result);
    if ((val_firme_status_to_i64(result.x0) != (int64_t)FIRME_SUCCESS) ||
        ((result.x1 & FIRME_MECID_FEATURE_REG0_MEC_REFRESH_BIT) == 0ULL))
        return mec_abi_compliance_fail(out, MEC_ABI_COMPLIANCE_STEP_FEATURE,
                                       &result);

    /* Step 3: Invoke a valid refresh to prove the ABI is implemented. */
    val_firme_mec_refresh_do_retry(params, &result);
    if (val_firme_status_to_i64(result.x0) != (int64_t)FIRME_SUCCESS)
        return mec_abi_compliance_fail(out, MEC_ABI_COMPLIANCE_STEP_CALL,
                                       &result);

    /* Step 4: Retire the test context before another rule uses this MECID. */
    params = val_firme_mec_params(MEC_ABI_COMPLIANCE_TEST_MECID,
                                   FIRME_MEC_REFRESH_REASON_DESTROY);
    val_firme_mec_refresh_do_retry(params, &result);
    if (val_firme_status_to_i64(result.x0) != (int64_t)FIRME_SUCCESS)
        return mec_abi_compliance_fail(out, MEC_ABI_COMPLIANCE_STEP_CLEANUP,
                                       &result);

    out->status = (uint64_t)ACS_STATUS_PASS;
    return VAL_SUCCESS;
}
