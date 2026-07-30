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

#define RESERVED_OUTPUT_BITS_TEST_NAME "reserved_output_bits"
#define RESERVED_OUTPUT_BITS_STEP_VERSION   UINT64_C(1)
#define RESERVED_OUTPUT_BITS_STEP_REG0_BITS UINT64_C(2)
#define RESERVED_OUTPUT_BITS_STEP_REG1_BITS UINT64_C(3)
#define RESERVED_OUTPUT_BITS_STEP_POISON_ENABLE UINT64_C(4)
#define RESERVED_OUTPUT_BITS_STEP_VERSION_POISON UINT64_C(5)
#define RESERVED_OUTPUT_BITS_STEP_REG0_POISON    UINT64_C(6)
#define RESERVED_OUTPUT_BITS_STEP_REG1_POISON    UINT64_C(7)
#define RESERVED_OUTPUT_BITS_POISON_COUNT 3U
#define RESERVED_OUTPUT_BITS_X0_ALLOCATED_MASK UINT64_C(0xffffffff)
#define RESERVED_OUTPUT_BITS_REG0_ALLOCATED_MASK \
    (FIRME_BASE_FEATURE_REG0_MANDATORY_BITS | \
     (FIRME_BASE_SERVICE_LIST_MASK << FIRME_BASE_SERVICE_LIST_SHIFT))
#define RESERVED_OUTPUT_BITS_REG1_ALLOCATED_MASK \
    ((FIRME_BASE_MIN_SH_BUF_SZ_MASK << FIRME_BASE_MIN_SH_BUF_SZ_SHIFT) | \
     (FIRME_BASE_MAX_SH_BUF_PG_CNT_MASK << FIRME_BASE_MAX_SH_BUF_PG_CNT_SHIFT))

static const firme_res_t g_reserved_output_version_mask = {
    .x0 = ~RESERVED_OUTPUT_BITS_X0_ALLOCATED_MASK,
};

static const firme_res_t g_reserved_output_reg0_mask = {
    .x0 = ~RESERVED_OUTPUT_BITS_X0_ALLOCATED_MASK,
    .x1 = ~RESERVED_OUTPUT_BITS_REG0_ALLOCATED_MASK,
};

static const firme_res_t g_reserved_output_reg1_mask = {
    .x0 = ~RESERVED_OUTPUT_BITS_X0_ALLOCATED_MASK,
    .x1 = ~RESERVED_OUTPUT_BITS_REG1_ALLOCATED_MASK,
};

static const firme_res_t g_reserved_output_bit_poison[
    RESERVED_OUTPUT_BITS_POISON_COUNT] = {
    {.x0 = 0ULL, .x1 = 0ULL},
    {.x0 = UINT64_MAX, .x1 = UINT64_MAX},
    {.x0 = UINT64_C(0xa5a55a5ac33c3cc3),
     .x1 = UINT64_C(0x5a5aa5a53cc3c33c)},
};

/**
 *   @brief    - Initialize a reserved output bit result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void reserved_output_bits_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a reserved output bit failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void.
 **/
static void reserved_output_bits_fail(firme_rule_res_t *out, uint64_t fail_step,
                                      uint64_t last_x0, uint64_t last_x1)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Record an ACS output-poisoning infrastructure error.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void.
 **/
static void reserved_output_bits_error(firme_rule_res_t *out,
                                       uint64_t fail_step,
                                       uint64_t last_x0,
                                       uint64_t last_x1)
{
    reserved_output_bits_fail(out, fail_step, last_x0, last_x1);
    if (out)
        out->status = (uint64_t)ACS_STATUS_ERR;
}

/**
 *   @brief    - Validate callers ignore unused Base service output bits.
 *   @param    - out : Optional result record.
 *   @return   - VAL_SUCCESS on completion, otherwise VAL_ERROR.
 **/
static int reserved_output_bits_run(firme_rule_res_t *out)
{
    firme_res_t version_result = {0};
    firme_res_t reg0_result = {0};
    firme_res_t reg1_result = {0};
    firme_res_t poisoned_result = {0};
    uint64_t baseline_reg0 = 0ULL;
    uint64_t poisoned_reg0 = 0ULL;
    uint64_t baseline_reg1 = 0ULL;
    uint64_t poisoned_reg1 = 0ULL;
    uint64_t baseline_min_size = 0ULL;
    uint32_t baseline_implemented = VAL_FALSE;
    uint32_t poisoned_implemented = VAL_FALSE;
    uint32_t baseline_advertised = VAL_FALSE;
    uint32_t poisoned_advertised = VAL_FALSE;
    uint32_t poison_index = 0U;
    int version_status = VAL_ERROR;
    int reg0_status = VAL_ERROR;
    int reg1_status = VAL_ERROR;
    int poisoned_status = VAL_ERROR;

    reserved_output_bits_init_result(out);

    /* Step 1: Capture the real VERSION and Base feature-register responses. */
    version_status = val_firme_service_get_version(
        FIRME_SERVICE_ID_BASE, &baseline_implemented, &version_result);
    if ((version_status != VAL_SUCCESS) || (baseline_implemented != VAL_TRUE)) {
        LOG_ERROR(NULL, "%s: SERVICE_VERSION expected version x0=%llx",
                  RESERVED_OUTPUT_BITS_TEST_NAME,
                  (unsigned long long)version_result.x0);
        reserved_output_bits_fail(out, RESERVED_OUTPUT_BITS_STEP_VERSION,
                                  version_result.x0, version_result.x1);
        return VAL_SUCCESS;
    }

    reg0_status = val_firme_service_abi_is_advertised(
        FIRME_SERVICE_ID_BASE, FIRME_BASE_FEATURE_REG0_MANDATORY_BITS,
        &baseline_advertised, &baseline_reg0, &reg0_result);
    if ((reg0_status != VAL_SUCCESS) || (baseline_advertised != VAL_TRUE)) {
        LOG_ERROR(NULL, "%s: reg0 allocated bits invalid x0=%llx x1=%llx",
                  RESERVED_OUTPUT_BITS_TEST_NAME,
                  (unsigned long long)reg0_result.x0,
                  (unsigned long long)reg0_result.x1);
        reserved_output_bits_fail(out, RESERVED_OUTPUT_BITS_STEP_REG0_BITS,
                                  reg0_result.x0, reg0_result.x1);
        return VAL_SUCCESS;
    }

    val_firme_service_features(FIRME_SERVICE_ID_BASE,
                               FIRME_BASE_FEATURE_REG1_INDEX, &reg1_result);
    reg1_status = (val_firme_status_to_i64(reg1_result.x0) ==
                   (int64_t)FIRME_SUCCESS) ? VAL_SUCCESS : VAL_ERROR;
    baseline_reg1 = reg1_result.x1 & RESERVED_OUTPUT_BITS_REG1_ALLOCATED_MASK;
    baseline_min_size = (baseline_reg1 >> FIRME_BASE_MIN_SH_BUF_SZ_SHIFT) &
                        FIRME_BASE_MIN_SH_BUF_SZ_MASK;
    if ((reg1_status != VAL_SUCCESS) ||
        (baseline_min_size > FIRME_BASE_MIN_SH_BUF_SZ_MAX_ALLOCATED)) {
        LOG_ERROR(NULL, "%s: reg1 allocated bits invalid x0=%llx x1=%llx",
                  RESERVED_OUTPUT_BITS_TEST_NAME,
                  (unsigned long long)reg1_result.x0,
                  (unsigned long long)reg1_result.x1);
        reserved_output_bits_fail(out, RESERVED_OUTPUT_BITS_STEP_REG1_BITS,
                                  reg1_result.x0, reg1_result.x1);
        return VAL_SUCCESS;
    }

    /* Step 2: Poison unused VERSION bits before production decoding. */
    for (poison_index = 0U; poison_index < RESERVED_OUTPUT_BITS_POISON_COUNT;
         poison_index++) {
        if (val_firme_test_output_poison_enable(
                FIRME_FID_SERVICE_VERSION, &g_reserved_output_version_mask,
                &g_reserved_output_bit_poison[poison_index]) != VAL_SUCCESS) {
            reserved_output_bits_error(
                out, RESERVED_OUTPUT_BITS_STEP_POISON_ENABLE,
                FIRME_FID_SERVICE_VERSION, poison_index);
            return VAL_ERROR;
        }

        poisoned_implemented = VAL_FALSE;
        poisoned_result = (firme_res_t){0};
        poisoned_status = val_firme_service_get_version(
            FIRME_SERVICE_ID_BASE, &poisoned_implemented, &poisoned_result);
        val_firme_test_output_poison_disable();
        if ((poisoned_status != version_status) ||
            (poisoned_implemented != baseline_implemented) ||
            (FIRME_SERVICE_VERSION_MAJOR(poisoned_result.x0) !=
             FIRME_SERVICE_VERSION_MAJOR(version_result.x0)) ||
            (FIRME_SERVICE_VERSION_MINOR(poisoned_result.x0) !=
             FIRME_SERVICE_VERSION_MINOR(version_result.x0))) {
            LOG_ERROR(NULL, "%s: VERSION poison index=%u changed decoding",
                      RESERVED_OUTPUT_BITS_TEST_NAME, poison_index);
            reserved_output_bits_fail(out,
                                      RESERVED_OUTPUT_BITS_STEP_VERSION_POISON,
                                      poisoned_result.x0, version_result.x0);
            return VAL_SUCCESS;
        }
    }

    /* Step 3: Poison unused feature-register-0 bits. */
    for (poison_index = 0U; poison_index < RESERVED_OUTPUT_BITS_POISON_COUNT;
         poison_index++) {
        if (val_firme_test_output_poison_enable(
                FIRME_FID_SERVICE_FEATURES, &g_reserved_output_reg0_mask,
                &g_reserved_output_bit_poison[poison_index]) != VAL_SUCCESS) {
            reserved_output_bits_error(
                out, RESERVED_OUTPUT_BITS_STEP_POISON_ENABLE,
                FIRME_FID_SERVICE_FEATURES, poison_index);
            return VAL_ERROR;
        }

        poisoned_advertised = VAL_FALSE;
        poisoned_reg0 = 0ULL;
        poisoned_result = (firme_res_t){0};
        poisoned_status = val_firme_service_abi_is_advertised(
            FIRME_SERVICE_ID_BASE, FIRME_BASE_FEATURE_REG0_MANDATORY_BITS,
            &poisoned_advertised, &poisoned_reg0, &poisoned_result);
        val_firme_test_output_poison_disable();
        if ((poisoned_status != reg0_status) ||
            (poisoned_advertised != baseline_advertised) ||
            ((poisoned_reg0 & RESERVED_OUTPUT_BITS_REG0_ALLOCATED_MASK) !=
             (baseline_reg0 & RESERVED_OUTPUT_BITS_REG0_ALLOCATED_MASK))) {
            LOG_ERROR(NULL, "%s: reg0 poison index=%u changed decoding",
                      RESERVED_OUTPUT_BITS_TEST_NAME, poison_index);
            reserved_output_bits_fail(out,
                                      RESERVED_OUTPUT_BITS_STEP_REG0_POISON,
                                      poisoned_result.x0, poisoned_result.x1);
            return VAL_SUCCESS;
        }
    }

    /* Step 4: Poison unused feature-register-1 bits. */
    for (poison_index = 0U; poison_index < RESERVED_OUTPUT_BITS_POISON_COUNT;
         poison_index++) {
        if (val_firme_test_output_poison_enable(
                FIRME_FID_SERVICE_FEATURES, &g_reserved_output_reg1_mask,
                &g_reserved_output_bit_poison[poison_index]) != VAL_SUCCESS) {
            reserved_output_bits_error(
                out, RESERVED_OUTPUT_BITS_STEP_POISON_ENABLE,
                FIRME_FID_SERVICE_FEATURES, poison_index);
            return VAL_ERROR;
        }

        poisoned_result = (firme_res_t){0};
        val_firme_service_features(FIRME_SERVICE_ID_BASE,
                                   FIRME_BASE_FEATURE_REG1_INDEX,
                                   &poisoned_result);
        val_firme_test_output_poison_disable();
        poisoned_status = (val_firme_status_to_i64(poisoned_result.x0) ==
                           (int64_t)FIRME_SUCCESS) ? VAL_SUCCESS : VAL_ERROR;
        poisoned_reg1 = poisoned_result.x1 &
                        RESERVED_OUTPUT_BITS_REG1_ALLOCATED_MASK;
        if ((poisoned_status != reg1_status) ||
            (poisoned_reg1 != baseline_reg1)) {
            LOG_ERROR(NULL, "%s: reg1 poison index=%u changed decoding",
                      RESERVED_OUTPUT_BITS_TEST_NAME, poison_index);
            reserved_output_bits_fail(out,
                                      RESERVED_OUTPUT_BITS_STEP_REG1_POISON,
                                      poisoned_result.x0, poisoned_result.x1);
            return VAL_SUCCESS;
        }
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Reserved output bit rule body for NS caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_reserved_output_bits_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return reserved_output_bits_run(out);
}

/**
 *   @brief    - Reserved output bit rule body for Secure caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_reserved_output_bits_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return reserved_output_bits_run(out);
}

/**
 *   @brief    - Reserved output bit rule body for Realm caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_reserved_output_bits_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return reserved_output_bits_run(out);
}
