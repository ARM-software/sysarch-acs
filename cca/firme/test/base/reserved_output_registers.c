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

#define RESERVED_OUTPUT_REGS_TEST_NAME "reserved_output_registers"
#define RESERVED_OUTPUT_REGS_STEP_VERSION  UINT64_C(1)
#define RESERVED_OUTPUT_REGS_STEP_FEATURES UINT64_C(2)
#define RESERVED_OUTPUT_REGS_STEP_POISON_ENABLE UINT64_C(3)
#define RESERVED_OUTPUT_REGS_STEP_VERSION_POISON UINT64_C(4)
#define RESERVED_OUTPUT_REGS_STEP_FEATURES_POISON UINT64_C(5)
#define RESERVED_OUTPUT_REGS_POISON_COUNT 3U

static const firme_res_t g_reserved_output_version_register_mask = {
    .x1 = UINT64_MAX,
    .x2 = UINT64_MAX,
    .x3 = UINT64_MAX,
};

static const firme_res_t g_reserved_output_feature_register_mask = {
    .x2 = UINT64_MAX,
    .x3 = UINT64_MAX,
};

static const firme_res_t g_reserved_output_register_poison[
    RESERVED_OUTPUT_REGS_POISON_COUNT] = {
    {.x1 = 0ULL, .x2 = 0ULL, .x3 = 0ULL},
    {.x1 = UINT64_MAX, .x2 = UINT64_MAX, .x3 = UINT64_MAX},
    {.x1 = UINT64_C(0xa5a5a5a5a5a5a5a5),
     .x2 = UINT64_C(0x5a5a5a5a5a5a5a5a),
     .x3 = UINT64_C(0x3cc3c33cc33c3cc3)},
};

/**
 *   @brief    - Initialize a reserved output register result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void reserved_output_regs_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a reserved output register failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void.
 **/
static void reserved_output_regs_fail(firme_rule_res_t *out, uint64_t fail_step,
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
static void reserved_output_regs_error(firme_rule_res_t *out,
                                       uint64_t fail_step,
                                       uint64_t last_x0,
                                       uint64_t last_x1)
{
    reserved_output_regs_fail(out, fail_step, last_x0, last_x1);
    if (out)
        out->status = (uint64_t)ACS_STATUS_ERR;
}

/**
 *   @brief    - Validate callers consume only architected output registers.
 *   @param    - out : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
static int reserved_output_regs_run(firme_rule_res_t *out)
{
    firme_res_t version_result = {0};
    firme_res_t feature_result = {0};
    firme_res_t poisoned_result = {0};
    uint64_t baseline_feature_reg0 = 0ULL;
    uint64_t poisoned_feature_reg0 = 0ULL;
    uint32_t baseline_implemented = VAL_FALSE;
    uint32_t poisoned_implemented = VAL_FALSE;
    uint32_t baseline_advertised = VAL_FALSE;
    uint32_t poisoned_advertised = VAL_FALSE;
    uint32_t poison_index = 0U;
    int version_status = VAL_ERROR;
    int feature_status = VAL_ERROR;
    int poisoned_status = VAL_ERROR;

    reserved_output_regs_init_result(out);

    /* Step 1: Capture the real Base service discovery results. */
    version_status = val_firme_service_get_version(
        FIRME_SERVICE_ID_BASE, &baseline_implemented, &version_result);
    if ((version_status != VAL_SUCCESS) || (baseline_implemented != VAL_TRUE)) {
        LOG_ERROR(NULL, "%s: SERVICE_VERSION expected version x0=%llx",
                  RESERVED_OUTPUT_REGS_TEST_NAME,
                  (unsigned long long)version_result.x0);
        reserved_output_regs_fail(out, RESERVED_OUTPUT_REGS_STEP_VERSION,
                                  version_result.x0, version_result.x1);
        return VAL_SUCCESS;
    }

    feature_status = val_firme_service_abi_is_advertised(
        FIRME_SERVICE_ID_BASE, FIRME_BASE_FEATURE_REG0_MANDATORY_BITS,
        &baseline_advertised, &baseline_feature_reg0, &feature_result);
    if ((feature_status != VAL_SUCCESS) || (baseline_advertised != VAL_TRUE)) {
        LOG_ERROR(NULL, "%s: SERVICE_FEATURES reg0 expected SUCCESS x0=%llx",
                  RESERVED_OUTPUT_REGS_TEST_NAME,
                  (unsigned long long)feature_result.x0);
        reserved_output_regs_fail(out, RESERVED_OUTPUT_REGS_STEP_FEATURES,
                                  feature_result.x0, feature_result.x1);
        return VAL_SUCCESS;
    }

    /* Step 2: Poison unused VERSION registers before production decoding. */
    for (poison_index = 0U; poison_index < RESERVED_OUTPUT_REGS_POISON_COUNT;
         poison_index++) {
        if (val_firme_test_output_poison_enable(
                FIRME_FID_SERVICE_VERSION, &g_reserved_output_version_register_mask,
                &g_reserved_output_register_poison[poison_index]) != VAL_SUCCESS) {
            reserved_output_regs_error(
                out, RESERVED_OUTPUT_REGS_STEP_POISON_ENABLE,
                FIRME_FID_SERVICE_VERSION, poison_index);
            return VAL_ERROR;
        }

        poisoned_implemented = VAL_FALSE;
        poisoned_result = (firme_res_t){0};
        poisoned_status = val_firme_service_get_version(
            FIRME_SERVICE_ID_BASE, &poisoned_implemented, &poisoned_result);
        val_firme_test_output_poison_disable();
        if ((poisoned_status != version_status) ||
            (poisoned_implemented != baseline_implemented)) {
            LOG_ERROR(NULL, "%s: VERSION poison index=%u changed decoding",
                      RESERVED_OUTPUT_REGS_TEST_NAME, poison_index);
            reserved_output_regs_fail(out,
                                      RESERVED_OUTPUT_REGS_STEP_VERSION_POISON,
                                      poisoned_result.x1, poisoned_result.x2);
            return VAL_SUCCESS;
        }
    }

    /* Step 3: Poison unused FEATURES registers before production decoding. */
    for (poison_index = 0U; poison_index < RESERVED_OUTPUT_REGS_POISON_COUNT;
         poison_index++) {
        if (val_firme_test_output_poison_enable(
                FIRME_FID_SERVICE_FEATURES, &g_reserved_output_feature_register_mask,
                &g_reserved_output_register_poison[poison_index]) != VAL_SUCCESS) {
            reserved_output_regs_error(
                out, RESERVED_OUTPUT_REGS_STEP_POISON_ENABLE,
                FIRME_FID_SERVICE_FEATURES, poison_index);
            return VAL_ERROR;
        }

        poisoned_advertised = VAL_FALSE;
        poisoned_feature_reg0 = 0ULL;
        poisoned_result = (firme_res_t){0};
        poisoned_status = val_firme_service_abi_is_advertised(
            FIRME_SERVICE_ID_BASE, FIRME_BASE_FEATURE_REG0_MANDATORY_BITS,
            &poisoned_advertised, &poisoned_feature_reg0, &poisoned_result);
        val_firme_test_output_poison_disable();
        if ((poisoned_status != feature_status) ||
            (poisoned_advertised != baseline_advertised) ||
            (poisoned_feature_reg0 != baseline_feature_reg0)) {
            LOG_ERROR(NULL, "%s: FEATURES poison index=%u changed decoding",
                      RESERVED_OUTPUT_REGS_TEST_NAME, poison_index);
            reserved_output_regs_fail(out,
                                      RESERVED_OUTPUT_REGS_STEP_FEATURES_POISON,
                                      poisoned_result.x2, poisoned_result.x3);
            return VAL_SUCCESS;
        }
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Reserved output register rule body for NS caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_reserved_output_registers_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return reserved_output_regs_run(out);
}

/**
 *   @brief    - Reserved output register rule body for Secure caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_reserved_output_registers_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return reserved_output_regs_run(out);
}

/**
 *   @brief    - Reserved output register rule body for Realm caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_reserved_output_registers_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return reserved_output_regs_run(out);
}
