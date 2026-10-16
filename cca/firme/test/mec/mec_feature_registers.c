/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_firme_rules.h"
#include "val_log.h"

#define MEC_FEATURE_REGISTERS_TEST_NAME "mec_feature_registers"
#define MEC_FEATURE_REGISTERS_STEP_VERSION UINT64_C(1)
#define MEC_FEATURE_REGISTERS_STEP_REG0 UINT64_C(2)
#define MEC_FEATURE_REGISTERS_STEP_REG0_BITS UINT64_C(3)
#define MEC_FEATURE_REGISTERS_STEP_REG1 UINT64_C(4)
#define MEC_FEATURE_REGISTERS_STEP_UNALLOCATED UINT64_C(6)
#define MEC_FEATURE_REGISTERS_FIRST_UNALLOCATED_INDEX 2U

/**
 * @brief Record a failing MEC feature-register check.
 * @param out Rule result.
 * @param step Failing check number.
 * @param result DUT response.
 * @return VAL_ERROR.
 */
static int mec_feature_registers_fail(firme_rule_res_t *out, uint64_t step,
                                      const firme_res_t *result)
{
    out->status = (uint64_t)ACS_STATUS_FAIL;
    out->fail_step = step;
    out->last_x0 = result->x0;
    out->last_x1 = result->x1;
    LOG_ERROR(NULL, "%s: step=%llu x0=%llx x1=%llx",
              MEC_FEATURE_REGISTERS_TEST_NAME, (unsigned long long)step,
              (unsigned long long)result->x0, (unsigned long long)result->x1);
    return VAL_ERROR;
}

/**
 * @brief Validate ALP4 MEC feature-register indices, sizes and encodings.
 * @param base_pa Unused platform test address.
 * @param out Rule result.
 * @return VAL_SUCCESS on PASS, otherwise VAL_ERROR.
 */
int firme_mec_feature_registers_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    firme_res_t result = {0};
    uint32_t register_index;

    (void)base_pa;
    if (!out)
        return VAL_ERROR;

    out->status = (uint64_t)ACS_STATUS_FAIL;
    out->fail_step = 0ULL;
    out->last_x0 = 0ULL;
    out->last_x1 = 0ULL;

    /* Step 1: MECID management must have an implemented Realm version. */
    val_firme_service_version(FIRME_SERVICE_ID_MECID_MGMT, &result);
    if (val_firme_status_to_i64(result.x0) <= 0)
        return mec_feature_registers_fail(out, MEC_FEATURE_REGISTERS_STEP_VERSION,
                                          &result);

    /* Step 2: Feature register 0 must advertise the mandatory refresh ABI. */
    val_firme_service_features(FIRME_SERVICE_ID_MECID_MGMT,
                               FIRME_MECID_FEATURE_REG0_INDEX, &result);
    if (val_firme_status_to_i64(result.x0) != (int64_t)FIRME_SUCCESS)
        return mec_feature_registers_fail(out, MEC_FEATURE_REGISTERS_STEP_REG0,
                                          &result);
    if ((result.x1 & FIRME_MECID_FEATURE_REG0_MEC_REFRESH_BIT) == 0ULL)
        return mec_feature_registers_fail(out, MEC_FEATURE_REGISTERS_STEP_REG0_BITS,
                                          &result);

    /* Step 3: Register 1 is implemented; unused output bits are ignored (R0015). */
    val_firme_service_features(FIRME_SERVICE_ID_MECID_MGMT,
                               FIRME_MECID_FEATURE_REG1_INDEX, &result);
    if (val_firme_status_to_i64(result.x0) != (int64_t)FIRME_SUCCESS)
        return mec_feature_registers_fail(out, MEC_FEATURE_REGISTERS_STEP_REG1,
                                          &result);

    /* Step 4: Every unallocated register index must return NOT_SUPPORTED. */
    for (register_index = MEC_FEATURE_REGISTERS_FIRST_UNALLOCATED_INDEX;
         register_index <= FIRME_FEATURE_REG_INDEX_8BIT_MAX; register_index++) {
        val_firme_service_features(FIRME_SERVICE_ID_MECID_MGMT,
                                   register_index, &result);
        if (val_firme_status_to_i64(result.x0) != (int64_t)FIRME_NOT_SUPPORTED) {
            mec_feature_registers_fail(out, MEC_FEATURE_REGISTERS_STEP_UNALLOCATED,
                                       &result);
            out->last_x1 = register_index;
            return VAL_ERROR;
        }
    }

    out->status = (uint64_t)ACS_STATUS_PASS;
    return VAL_SUCCESS;
}
