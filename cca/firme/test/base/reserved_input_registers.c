/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_firme_rules.h"
#include "val_common/val_smccc.h"
#include "val_log.h"

#define RESERVED_INPUT_REGS_TEST_NAME "reserved_input_registers"
#define RESERVED_INPUT_REGS_STEP_SERVICE_VERSION  UINT64_C(1)
#define RESERVED_INPUT_REGS_STEP_SERVICE_FEATURES UINT64_C(2)
#define RESERVED_INPUT_REGS_STEP_GM_GPI_SET       UINT64_C(3)
#define RESERVED_INPUT_REGS_STEP_MEC_REFRESH      UINT64_C(4)
#define RESERVED_INPUT_REGS_VERSION_FIRST_UNUSED  2U
#define RESERVED_INPUT_REGS_FEATURES_FIRST_UNUSED 3U
#define RESERVED_INPUT_REGS_GPI_SET_FIRST_UNUSED  4U
#define RESERVED_INPUT_REGS_MEC_FIRST_UNUSED      2U
#define RESERVED_INPUT_REGS_GRANULE_COUNT         UINT64_C(1)

/**
 *   @brief    - Initialize a reserved input register result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void reserved_input_regs_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a reserved input register failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void.
 **/
static void reserved_input_regs_fail(firme_rule_res_t *out, uint64_t fail_step,
                                     uint64_t last_x0, uint64_t last_x1)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Translate an internal helper stop into the rule-body contract.
 *   @param    - out : Result record populated by the helper.
 *   @return   - VAL_SUCCESS for a recorded DUT failure, otherwise VAL_ERROR.
 **/
static int reserved_input_regs_result(const firme_rule_res_t *out)
{
    if ((out != 0) && (out->status == (uint64_t)ACS_STATUS_FAIL))
        return VAL_SUCCESS;

    return VAL_ERROR;
}

/**
 *   @brief    - Enable capture-only mode for one production FIRME wrapper call.
 *   @param    - fail_step : Test step to report if capture cannot be enabled.
 *   @param    - out       : Optional result record.
 *   @return   - VAL_SUCCESS when enabled, otherwise VAL_ERROR.
 **/
static int reserved_input_regs_capture_enable(uint64_t fail_step,
                                              firme_rule_res_t *out)
{
    if (val_smccc_test_input_capture_enable() != VAL_SUCCESS) {
        LOG_ERROR(NULL, "%s: input capture setup failed step=%llu",
                  RESERVED_INPUT_REGS_TEST_NAME,
                  (unsigned long long)fail_step);
        reserved_input_regs_fail(out, fail_step, 0, 0);
        if (out)
            out->status = (uint64_t)ACS_STATUS_ERR;
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/**
 *   @brief    - Verify every wholly unused input register is zero.
 *   @param    - first_unused : First input register unused by the ABI.
 *   @param    - fail_step    : Test step associated with the ABI.
 *   @param    - out          : Optional result record.
 *   @return   - VAL_SUCCESS when unused registers are zero, otherwise VAL_ERROR.
 **/
static int reserved_input_regs_check_capture(uint32_t first_unused,
                                             uint64_t fail_step,
                                             firme_rule_res_t *out)
{
    smccc_input_registers_t captured = {0};
    uint32_t register_index = 0U;
    int capture_status = VAL_ERROR;

    capture_status = val_smccc_test_input_capture_read(&captured);
    val_smccc_test_input_capture_disable();
    if (capture_status != VAL_SUCCESS) {
        LOG_ERROR(NULL, "%s: input capture read failed step=%llu",
                  RESERVED_INPUT_REGS_TEST_NAME,
                  (unsigned long long)fail_step);
        reserved_input_regs_fail(out, fail_step, 0, 0);
        if (out)
            out->status = (uint64_t)ACS_STATUS_ERR;
        return VAL_ERROR;
    }

    for (register_index = first_unused;
         register_index < VAL_SMCCC_INPUT_REGISTER_COUNT;
         register_index++) {
        if (captured.registers[register_index] != 0ULL) {
            LOG_ERROR(NULL, "%s: step=%llu x%u expected SBZ, actual=%llx",
                      RESERVED_INPUT_REGS_TEST_NAME,
                      (unsigned long long)fail_step, register_index,
                      (unsigned long long)captured.registers[register_index]);
            reserved_input_regs_fail(
                out, fail_step, register_index,
                captured.registers[register_index]);
            return VAL_ERROR;
        }
    }

    return VAL_SUCCESS;
}

/**
 *   @brief    - Validate production FIRME wrappers zero unused input registers.
 *   @param    - base_pa : Test physical address used for the captured GM request.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS on completion, otherwise VAL_ERROR.
 **/
static int reserved_input_regs_run(uint64_t base_pa, firme_rule_res_t *out)
{
    firme_res_t result = {0};

    reserved_input_regs_init_result(out);

    /* Step 1: Capture SERVICE_VERSION and verify x2-x17 are zero. */
    if (reserved_input_regs_capture_enable(
            RESERVED_INPUT_REGS_STEP_SERVICE_VERSION, out) != VAL_SUCCESS)
        return reserved_input_regs_result(out);
    val_firme_service_version(FIRME_SERVICE_ID_BASE, &result);
    if (reserved_input_regs_check_capture(
            RESERVED_INPUT_REGS_VERSION_FIRST_UNUSED,
            RESERVED_INPUT_REGS_STEP_SERVICE_VERSION, out) != VAL_SUCCESS)
        return reserved_input_regs_result(out);

    /* Step 2: Capture SERVICE_FEATURES and verify x3-x17 are zero. */
    if (reserved_input_regs_capture_enable(
            RESERVED_INPUT_REGS_STEP_SERVICE_FEATURES, out) != VAL_SUCCESS)
        return reserved_input_regs_result(out);
    val_firme_service_features(FIRME_SERVICE_ID_BASE,
                               FIRME_BASE_FEATURE_REG0_INDEX, &result);
    if (reserved_input_regs_check_capture(
            RESERVED_INPUT_REGS_FEATURES_FIRST_UNUSED,
            RESERVED_INPUT_REGS_STEP_SERVICE_FEATURES, out) != VAL_SUCCESS)
        return reserved_input_regs_result(out);

    /* Step 3: Capture GM_GPI_SET and verify x4-x17 are zero. */
    if (reserved_input_regs_capture_enable(
            RESERVED_INPUT_REGS_STEP_GM_GPI_SET, out) != VAL_SUCCESS)
        return reserved_input_regs_result(out);
    val_firme_gm_gpi_set(base_pa, RESERVED_INPUT_REGS_GRANULE_COUNT,
                         FIRME_GM_GPI_SET_ATTRIBUTES_MASK, &result);
    if (reserved_input_regs_check_capture(
            RESERVED_INPUT_REGS_GPI_SET_FIRST_UNUSED,
            RESERVED_INPUT_REGS_STEP_GM_GPI_SET, out) != VAL_SUCCESS)
        return reserved_input_regs_result(out);

    /* Step 4: Capture MEC_REFRESH and verify x2-x17 are zero. */
    if (reserved_input_regs_capture_enable(
            RESERVED_INPUT_REGS_STEP_MEC_REFRESH, out) != VAL_SUCCESS)
        return reserved_input_regs_result(out);
    val_firme_mec_refresh(FIRME_MEC_REFRESH_REASON_CREATE, &result);
    if (reserved_input_regs_check_capture(
            RESERVED_INPUT_REGS_MEC_FIRST_UNUSED,
            RESERVED_INPUT_REGS_STEP_MEC_REFRESH, out) != VAL_SUCCESS)
        return reserved_input_regs_result(out);

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Reserved input register rule body for NS caller execution.
 *   @param    - base_pa : Test physical address.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS on completion, otherwise VAL_ERROR.
 **/
int firme_base_reserved_input_registers_run_ns(uint64_t base_pa,
                                               firme_rule_res_t *out)
{
    return reserved_input_regs_run(base_pa, out);
}

/**
 *   @brief    - Reserved input register rule body for Secure caller execution.
 *   @param    - base_pa : Test physical address.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS on completion, otherwise VAL_ERROR.
 **/
int firme_base_reserved_input_registers_run_secure(uint64_t base_pa,
                                                   firme_rule_res_t *out)
{
    return reserved_input_regs_run(base_pa, out);
}

/**
 *   @brief    - Reserved input register rule body for Realm caller execution.
 *   @param    - base_pa : Test physical address.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS on completion, otherwise VAL_ERROR.
 **/
int firme_base_reserved_input_registers_run_realm(uint64_t base_pa,
                                                  firme_rule_res_t *out)
{
    return reserved_input_regs_run(base_pa, out);
}
