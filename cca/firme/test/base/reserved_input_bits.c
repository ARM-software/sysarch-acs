/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_firme_ide.h"
#include "val_firme_rules.h"
#include "val_common/val_smccc.h"
#include "val_log.h"

#define RESERVED_INPUT_BITS_TEST_NAME "reserved_input_bits"
#define RESERVED_INPUT_BITS_DIRTY_VALUE UINT64_MAX
#define RESERVED_INPUT_BITS_STEP_SERVICE_VERSION  UINT64_C(1)
#define RESERVED_INPUT_BITS_STEP_SERVICE_FEATURES UINT64_C(2)
#define RESERVED_INPUT_BITS_STEP_GM_GPI_SET       UINT64_C(3)
#define RESERVED_INPUT_BITS_STEP_MEC_REFRESH      UINT64_C(4)
#define RESERVED_INPUT_BITS_STEP_IDE_PROG         UINT64_C(5)
#define RESERVED_INPUT_BITS_STEP_IDE_GO           UINT64_C(6)
#define RESERVED_INPUT_BITS_STEP_IDE_STOP         UINT64_C(7)
#define RESERVED_INPUT_BITS_STEP_IDE_POLL         UINT64_C(8)
#define RESERVED_INPUT_BITS_X0_INDEX 0U
#define RESERVED_INPUT_BITS_X1_INDEX 1U
#define RESERVED_INPUT_BITS_X2_INDEX 2U
#define RESERVED_INPUT_BITS_X3_INDEX 3U
#define RESERVED_INPUT_BITS_X4_INDEX 4U
#define RESERVED_INPUT_BITS_X5_INDEX 5U
#define RESERVED_INPUT_BITS_X6_INDEX 6U
#define RESERVED_INPUT_BITS_X7_INDEX 7U
#define RESERVED_INPUT_BITS_X8_INDEX 8U

/**
 *   @brief    - Initialize a reserved input bit result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void reserved_input_bits_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a reserved input bit failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void.
 **/
static void reserved_input_bits_fail(firme_rule_res_t *out, uint64_t fail_step,
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
static int reserved_input_bits_result(const firme_rule_res_t *out)
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
static int reserved_input_bits_capture_enable(uint64_t fail_step,
                                              firme_rule_res_t *out)
{
    if (val_smccc_test_input_capture_enable() != VAL_SUCCESS) {
        LOG_ERROR(NULL, "%s: input capture setup failed step=%llu",
                  RESERVED_INPUT_BITS_TEST_NAME,
                  (unsigned long long)fail_step);
        reserved_input_bits_fail(out, fail_step, 0, 0);
        if (out)
            out->status = (uint64_t)ACS_STATUS_ERR;
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/**
 *   @brief    - Compare the captured SMC input frame with its allocated encoding.
 *   @param    - expected  : Expected x0-x17 values after wrapper sanitization.
 *   @param    - fail_step : Test step associated with the ABI.
 *   @param    - out       : Optional result record.
 *   @return   - VAL_SUCCESS when the frame matches, otherwise VAL_ERROR.
 **/
static int reserved_input_bits_check_capture(
    const smccc_input_registers_t *expected, uint64_t fail_step,
    firme_rule_res_t *out)
{
    smccc_input_registers_t captured = {0};
    uint32_t register_index = 0U;
    int capture_status = VAL_ERROR;

    capture_status = val_smccc_test_input_capture_read(&captured);
    val_smccc_test_input_capture_disable();
    if (capture_status != VAL_SUCCESS) {
        LOG_ERROR(NULL, "%s: input capture read failed step=%llu",
                  RESERVED_INPUT_BITS_TEST_NAME,
                  (unsigned long long)fail_step);
        reserved_input_bits_fail(out, fail_step, 0, 0);
        if (out)
            out->status = (uint64_t)ACS_STATUS_ERR;
        return VAL_ERROR;
    }

    for (register_index = 0U;
         register_index < VAL_SMCCC_INPUT_REGISTER_COUNT;
         register_index++) {
        if (captured.registers[register_index] !=
            expected->registers[register_index]) {
            LOG_ERROR(NULL,
                      "%s: step=%llu x%u actual=%llx expected=%llx",
                      RESERVED_INPUT_BITS_TEST_NAME,
                      (unsigned long long)fail_step, register_index,
                      (unsigned long long)captured.registers[register_index],
                      (unsigned long long)expected->registers[register_index]);
            reserved_input_bits_fail(
                out, fail_step, captured.registers[register_index],
                expected->registers[register_index]);
            return VAL_ERROR;
        }
    }

    return VAL_SUCCESS;
}

/**
 *   @brief    - Validate production FIRME wrappers clear every reserved input bit.
 *   @param    - base_pa : Test physical address used for the captured GM request.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS on completion, otherwise VAL_ERROR.
 **/
static int reserved_input_bits_run(uint64_t base_pa, firme_rule_res_t *out)
{
    smccc_input_registers_t expected = {0};
    firme_res_t result = {0};
    val_firme_ide_prog_args_t prog_args = {0};

    reserved_input_bits_init_result(out);

    /* Step 1: SERVICE_VERSION must retain only the allocated service-ID bits. */
    expected.registers[RESERVED_INPUT_BITS_X0_INDEX] =
        FIRME_FID_SERVICE_VERSION;
    expected.registers[RESERVED_INPUT_BITS_X1_INDEX] =
        FIRME_SERVICE_ID_INPUT_MASK;
    if (reserved_input_bits_capture_enable(
            RESERVED_INPUT_BITS_STEP_SERVICE_VERSION, out) != VAL_SUCCESS)
        return reserved_input_bits_result(out);
    val_firme_service_version(RESERVED_INPUT_BITS_DIRTY_VALUE, &result);
    if (reserved_input_bits_check_capture(
            &expected, RESERVED_INPUT_BITS_STEP_SERVICE_VERSION,
            out) != VAL_SUCCESS)
        return reserved_input_bits_result(out);

    /* Step 2: SERVICE_FEATURES must retain only its two eight-bit fields. */
    expected = (smccc_input_registers_t){0};
    expected.registers[RESERVED_INPUT_BITS_X0_INDEX] =
        FIRME_FID_SERVICE_FEATURES;
    expected.registers[RESERVED_INPUT_BITS_X1_INDEX] =
        FIRME_SERVICE_ID_INPUT_MASK;
    expected.registers[RESERVED_INPUT_BITS_X2_INDEX] =
        FIRME_FEATURE_REG_INDEX_INPUT_MASK;
    if (reserved_input_bits_capture_enable(
            RESERVED_INPUT_BITS_STEP_SERVICE_FEATURES, out) != VAL_SUCCESS)
        return reserved_input_bits_result(out);
    val_firme_service_features(RESERVED_INPUT_BITS_DIRTY_VALUE,
                               RESERVED_INPUT_BITS_DIRTY_VALUE, &result);
    if (reserved_input_bits_check_capture(
            &expected, RESERVED_INPUT_BITS_STEP_SERVICE_FEATURES,
            out) != VAL_SUCCESS)
        return reserved_input_bits_result(out);

    /* Step 3: GM_GPI_SET must retain only the target-GPI attribute field. */
    expected = (smccc_input_registers_t){0};
    expected.registers[RESERVED_INPUT_BITS_X0_INDEX] = FIRME_FID_GM_GPI_SET;
    expected.registers[RESERVED_INPUT_BITS_X1_INDEX] = base_pa;
    expected.registers[RESERVED_INPUT_BITS_X2_INDEX] =
        RESERVED_INPUT_BITS_DIRTY_VALUE;
    expected.registers[RESERVED_INPUT_BITS_X3_INDEX] =
        FIRME_GM_GPI_SET_ATTRIBUTES_MASK;
    if (reserved_input_bits_capture_enable(
            RESERVED_INPUT_BITS_STEP_GM_GPI_SET, out) != VAL_SUCCESS)
        return reserved_input_bits_result(out);
    val_firme_gm_gpi_set(base_pa, RESERVED_INPUT_BITS_DIRTY_VALUE,
                         RESERVED_INPUT_BITS_DIRTY_VALUE, &result);
    if (reserved_input_bits_check_capture(
            &expected, RESERVED_INPUT_BITS_STEP_GM_GPI_SET,
            out) != VAL_SUCCESS)
        return reserved_input_bits_result(out);

    /* Step 4: MEC_REFRESH must retain only MECID and refresh-reason fields. */
    expected = (smccc_input_registers_t){0};
    expected.registers[RESERVED_INPUT_BITS_X0_INDEX] = FIRME_FID_MEC_REFRESH;
    expected.registers[RESERVED_INPUT_BITS_X1_INDEX] =
        FIRME_MEC_REFRESH_PARAMS_MASK;
    if (reserved_input_bits_capture_enable(
            RESERVED_INPUT_BITS_STEP_MEC_REFRESH, out) != VAL_SUCCESS)
        return reserved_input_bits_result(out);
    val_firme_mec_refresh(RESERVED_INPUT_BITS_DIRTY_VALUE, &result);
    if (reserved_input_bits_check_capture(
            &expected, RESERVED_INPUT_BITS_STEP_MEC_REFRESH,
            out) != VAL_SUCCESS)
        return reserved_input_bits_result(out);

    /* Step 5: IDE_KEYSET_PROG must retain only allocated flag and ID bits. */
    expected = (smccc_input_registers_t){0};
    expected.registers[RESERVED_INPUT_BITS_X0_INDEX] =
        FIRME_FID_IDE_KEYSET_PROG;
    expected.registers[RESERVED_INPUT_BITS_X1_INDEX] =
        RESERVED_INPUT_BITS_DIRTY_VALUE;
    expected.registers[RESERVED_INPUT_BITS_X2_INDEX] =
        VAL_FIRME_IDE_INPUT_FLAGS_MASK;
    expected.registers[RESERVED_INPUT_BITS_X3_INDEX] =
        VAL_FIRME_IDE_KEYSET_ID_INPUT_MASK;
    expected.registers[RESERVED_INPUT_BITS_X4_INDEX] =
        RESERVED_INPUT_BITS_DIRTY_VALUE;
    expected.registers[RESERVED_INPUT_BITS_X5_INDEX] =
        RESERVED_INPUT_BITS_DIRTY_VALUE;
    expected.registers[RESERVED_INPUT_BITS_X6_INDEX] =
        RESERVED_INPUT_BITS_DIRTY_VALUE;
    expected.registers[RESERVED_INPUT_BITS_X7_INDEX] =
        RESERVED_INPUT_BITS_DIRTY_VALUE;
    expected.registers[RESERVED_INPUT_BITS_X8_INDEX] =
        RESERVED_INPUT_BITS_DIRTY_VALUE;
    prog_args.ecam_address = RESERVED_INPUT_BITS_DIRTY_VALUE;
    prog_args.flags = RESERVED_INPUT_BITS_DIRTY_VALUE;
    prog_args.keyset_id = RESERVED_INPUT_BITS_DIRTY_VALUE;
    prog_args.keyqw0 = RESERVED_INPUT_BITS_DIRTY_VALUE;
    prog_args.keyqw1 = RESERVED_INPUT_BITS_DIRTY_VALUE;
    prog_args.keyqw2 = RESERVED_INPUT_BITS_DIRTY_VALUE;
    prog_args.keyqw3 = RESERVED_INPUT_BITS_DIRTY_VALUE;
    prog_args.handle = RESERVED_INPUT_BITS_DIRTY_VALUE;
    if (reserved_input_bits_capture_enable(
            RESERVED_INPUT_BITS_STEP_IDE_PROG, out) != VAL_SUCCESS)
        return reserved_input_bits_result(out);
    val_firme_ide_keyset_prog(&prog_args, &result);
    if (reserved_input_bits_check_capture(
            &expected, RESERVED_INPUT_BITS_STEP_IDE_PROG,
            out) != VAL_SUCCESS)
        return reserved_input_bits_result(out);

    /* Step 6: IDE_KEYSET_GO must retain only allocated flag and ID bits. */
    expected = (smccc_input_registers_t){0};
    expected.registers[RESERVED_INPUT_BITS_X0_INDEX] =
        FIRME_FID_IDE_KEYSET_GO;
    expected.registers[RESERVED_INPUT_BITS_X1_INDEX] =
        RESERVED_INPUT_BITS_DIRTY_VALUE;
    expected.registers[RESERVED_INPUT_BITS_X2_INDEX] =
        VAL_FIRME_IDE_INPUT_FLAGS_MASK;
    expected.registers[RESERVED_INPUT_BITS_X3_INDEX] =
        VAL_FIRME_IDE_KEYSET_ID_INPUT_MASK;
    expected.registers[RESERVED_INPUT_BITS_X4_INDEX] =
        RESERVED_INPUT_BITS_DIRTY_VALUE;
    if (reserved_input_bits_capture_enable(
            RESERVED_INPUT_BITS_STEP_IDE_GO, out) != VAL_SUCCESS)
        return reserved_input_bits_result(out);
    val_firme_ide_keyset_go(
        RESERVED_INPUT_BITS_DIRTY_VALUE, RESERVED_INPUT_BITS_DIRTY_VALUE,
        RESERVED_INPUT_BITS_DIRTY_VALUE, RESERVED_INPUT_BITS_DIRTY_VALUE,
        &result);
    if (reserved_input_bits_check_capture(
            &expected, RESERVED_INPUT_BITS_STEP_IDE_GO,
            out) != VAL_SUCCESS)
        return reserved_input_bits_result(out);

    /* Step 7: IDE_KEYSET_STOP must retain only allocated flag and ID bits. */
    expected.registers[RESERVED_INPUT_BITS_X0_INDEX] =
        FIRME_FID_IDE_KEYSET_STOP;
    if (reserved_input_bits_capture_enable(
            RESERVED_INPUT_BITS_STEP_IDE_STOP, out) != VAL_SUCCESS)
        return reserved_input_bits_result(out);
    val_firme_ide_keyset_stop(
        RESERVED_INPUT_BITS_DIRTY_VALUE, RESERVED_INPUT_BITS_DIRTY_VALUE,
        RESERVED_INPUT_BITS_DIRTY_VALUE, RESERVED_INPUT_BITS_DIRTY_VALUE,
        &result);
    if (reserved_input_bits_check_capture(
            &expected, RESERVED_INPUT_BITS_STEP_IDE_STOP,
            out) != VAL_SUCCESS)
        return reserved_input_bits_result(out);

    /* Step 8: IDE_KEYSET_POLL must retain only its one-bit flag and ID. */
    expected = (smccc_input_registers_t){0};
    expected.registers[RESERVED_INPUT_BITS_X0_INDEX] =
        FIRME_FID_IDE_KEYSET_POLL;
    expected.registers[RESERVED_INPUT_BITS_X1_INDEX] =
        RESERVED_INPUT_BITS_DIRTY_VALUE;
    expected.registers[RESERVED_INPUT_BITS_X2_INDEX] =
        VAL_FIRME_IDE_POLL_FLAGS_MASK;
    expected.registers[RESERVED_INPUT_BITS_X3_INDEX] =
        VAL_FIRME_IDE_KEYSET_ID_INPUT_MASK;
    if (reserved_input_bits_capture_enable(
            RESERVED_INPUT_BITS_STEP_IDE_POLL, out) != VAL_SUCCESS)
        return reserved_input_bits_result(out);
    val_firme_ide_keyset_poll(
        RESERVED_INPUT_BITS_DIRTY_VALUE, RESERVED_INPUT_BITS_DIRTY_VALUE,
        RESERVED_INPUT_BITS_DIRTY_VALUE, &result);
    if (reserved_input_bits_check_capture(
            &expected, RESERVED_INPUT_BITS_STEP_IDE_POLL,
            out) != VAL_SUCCESS)
        return reserved_input_bits_result(out);

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Reserved input bit rule body for NS caller execution.
 *   @param    - base_pa : Test physical address.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS on completion, otherwise VAL_ERROR.
 **/
int firme_base_reserved_input_bits_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    return reserved_input_bits_run(base_pa, out);
}

/**
 *   @brief    - Reserved input bit rule body for Secure caller execution.
 *   @param    - base_pa : Test physical address.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS on completion, otherwise VAL_ERROR.
 **/
int firme_base_reserved_input_bits_run_secure(uint64_t base_pa,
                                              firme_rule_res_t *out)
{
    return reserved_input_bits_run(base_pa, out);
}

/**
 *   @brief    - Reserved input bit rule body for Realm caller execution.
 *   @param    - base_pa : Test physical address.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS on completion, otherwise VAL_ERROR.
 **/
int firme_base_reserved_input_bits_run_realm(uint64_t base_pa,
                                             firme_rule_res_t *out)
{
    return reserved_input_bits_run(base_pa, out);
}
