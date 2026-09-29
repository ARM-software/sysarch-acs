/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi_adapter.h"

#include <stddef.h>

#include "val_common/val_status.h"
#include "val_firme_abi.h"
#include "val_firme_gpt.h"
#include "val_firme_gpt_fixture.h"
#include "val_firme_mec.h"
#include "val_log.h"

#define VAL_FIRME_ALPHA1_MAJOR_VERSION 1U
#define VAL_FIRME_ABI_TEST_MECID       UINT64_C(1)
#define VAL_FIRME_GPT_INFO_SCRATCH_BASE       0U
#define VAL_FIRME_GPT_INFO_SCRATCH_TOP        1U
#define VAL_FIRME_GPT_INFO_SCRATCH_RANGE_SIZE 2U
#define VAL_FIRME_GPT_INFO_SCRATCH_GRANULE    3U
#define VAL_FIRME_GPT_INFO_SCRATCH_PPS_SIZE   4U

/**
 * @brief Build a valid SERVICE_VERSION input frame for the Base service.
 * @param context Generated test execution context.
 * @return VAL_SUCCESS on success, otherwise VAL_ERROR.
 */
static int val_firme_abi_service_version_build(
    val_firme_abi_test_context_t *context)
{
    if (!context)
        return VAL_ERROR;

    context->input.registers[1] = FIRME_SERVICE_ID_BASE;
    return VAL_SUCCESS;
}

/**
 * @brief Apply a SERVICE_VERSION scenario mutation.
 * @param context Generated test execution context.
 * @return VAL_SUCCESS for a supported mutation, otherwise VAL_ERROR.
 */
static int val_firme_abi_service_version_mutate(
    val_firme_abi_test_context_t *context)
{
    if (!context || !context->test_case)
        return VAL_ERROR;

    if (context->test_case->mutation ==
        VAL_FIRME_MUTATION_INVALID_SERVICE_ID) {
        context->input.registers[1] = FIRME_SERVICE_ID_MAX;
        return VAL_SUCCESS;
    }

    return VAL_ERROR;
}

/**
 * @brief Validate SERVICE_VERSION output fields for the active scenario.
 * @param context Generated test execution context.
 * @return VAL_TRUE when the output follows ALP4, otherwise VAL_FALSE.
 */
static uint32_t val_firme_abi_service_version_validate(
    val_firme_abi_test_context_t *context)
{
    uint32_t major_version;

    if (!context || !context->test_case)
        return VAL_FALSE;

    if (context->test_case->mutation ==
        VAL_FIRME_MUTATION_INVALID_SERVICE_ID) {
        return (val_firme_status_to_i64(context->output.registers[0]) ==
                (int64_t)FIRME_NOT_SUPPORTED) ? VAL_TRUE : VAL_FALSE;
    }

    major_version =
        FIRME_SERVICE_VERSION_MAJOR(context->output.registers[0]);
    if (major_version != VAL_FIRME_ALPHA1_MAJOR_VERSION) {
        LOG_ERROR(NULL,
                  "%s: SERVICE_VERSION major=%u expected=%u",
                  context->plan->name, (unsigned int)major_version,
                  (unsigned int)VAL_FIRME_ALPHA1_MAJOR_VERSION);
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 * @brief Build a valid SERVICE_FEATURES input for Base feature register 0.
 * @param context Generated test execution context.
 * @return VAL_SUCCESS on success, otherwise VAL_ERROR.
 */
static int val_firme_abi_service_features_build(
    val_firme_abi_test_context_t *context)
{
    if (!context)
        return VAL_ERROR;

    context->input.registers[1] = FIRME_SERVICE_ID_BASE;
    context->input.registers[2] = FIRME_BASE_FEATURE_REG0_INDEX;
    return VAL_SUCCESS;
}

/**
 * @brief Apply a SERVICE_FEATURES scenario mutation.
 * @param context Generated test execution context.
 * @return VAL_SUCCESS for a supported mutation, otherwise VAL_ERROR.
 */
static int val_firme_abi_service_features_mutate(
    val_firme_abi_test_context_t *context)
{
    if (!context || !context->test_case)
        return VAL_ERROR;

    if (context->test_case->mutation ==
        VAL_FIRME_MUTATION_INVALID_SERVICE_ID) {
        context->input.registers[1] = FIRME_SERVICE_ID_MAX;
        return VAL_SUCCESS;
    }

    if (context->test_case->mutation ==
        VAL_FIRME_MUTATION_INVALID_FEATURE_INDEX) {
        context->input.registers[2] = FIRME_FEATURE_REG_INDEX_8BIT_MAX;
        return VAL_SUCCESS;
    }

    return VAL_ERROR;
}

/**
 * @brief Validate SERVICE_FEATURES output fields for the active scenario.
 * @param context Generated test execution context.
 * @return VAL_TRUE when the output follows ALP4, otherwise VAL_FALSE.
 */
static uint32_t val_firme_abi_service_features_validate(
    val_firme_abi_test_context_t *context)
{
    uint64_t mandatory_bits;

    if (!context || !context->test_case)
        return VAL_FALSE;

    if (context->test_case->mutation != VAL_FIRME_MUTATION_NONE) {
        return (val_firme_status_to_i64(context->output.registers[0]) ==
                (int64_t)FIRME_NOT_SUPPORTED) ? VAL_TRUE : VAL_FALSE;
    }

    mandatory_bits = context->output.registers[1] &
                     FIRME_BASE_FEATURE_REG0_MANDATORY_BITS;
    if (mandatory_bits != FIRME_BASE_FEATURE_REG0_MANDATORY_BITS) {
        LOG_ERROR(NULL,
                  "%s: Base feature reg0=%llx missing mandatory bits=%llx",
                  context->plan->name,
                  (unsigned long long)context->output.registers[1],
                  (unsigned long long)FIRME_BASE_FEATURE_REG0_MANDATORY_BITS);
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 * @brief Build a valid MEC_REFRESH input for Realm creation.
 * @param context Generated test execution context.
 * @return VAL_SUCCESS on success, otherwise VAL_ERROR.
 */
static int val_firme_abi_mec_refresh_build(
    val_firme_abi_test_context_t *context)
{
    if (!context)
        return VAL_ERROR;

    context->input.registers[1] = val_firme_mec_params(
        VAL_FIRME_ABI_TEST_MECID, FIRME_MEC_REFRESH_REASON_CREATE);
    return VAL_SUCCESS;
}

/**
 * @brief Select the Realm-destruction reason for MEC_REFRESH.
 * @param context Generated test execution context.
 * @return VAL_SUCCESS for the destruction mutation, otherwise VAL_ERROR.
 */
static int val_firme_abi_mec_refresh_mutate(
    val_firme_abi_test_context_t *context)
{
    if (!context || !context->test_case)
        return VAL_ERROR;

    if (context->test_case->mutation !=
        VAL_FIRME_MUTATION_DESTROY_REASON)
        return VAL_ERROR;

    context->input.registers[1] = val_firme_mec_params(
        VAL_FIRME_ABI_TEST_MECID, FIRME_MEC_REFRESH_REASON_DESTROY);
    return VAL_SUCCESS;
}

/**
 * @brief Validate the terminal MEC_REFRESH result.
 * @param context Generated test execution context.
 * @return VAL_TRUE for SUCCESS, otherwise VAL_FALSE.
 */
static uint32_t val_firme_abi_mec_refresh_validate(
    val_firme_abi_test_context_t *context)
{
    if (!context)
        return VAL_FALSE;

    return (val_firme_status_to_i64(context->output.registers[0]) ==
            (int64_t)FIRME_SUCCESS) ? VAL_TRUE : VAL_FALSE;
}

/**
 * @brief Discover a valid protected-address-range query window.
 * @param context Generated test execution context.
 * @return VAL_SUCCESS for a valid query window, otherwise VAL_ERROR.
 */
static int val_firme_abi_gpt_info_prepare(
    val_firme_abi_test_context_t *context)
{
    val_firme_gpt_info_window_t window;

    if (!context)
        return VAL_ERROR;

    if (val_firme_gpt_info_prepare_window(
            context->base_pa, &window, NULL, 0ULL) == VAL_FALSE)
        return VAL_ERROR;

    context->scratch[VAL_FIRME_GPT_INFO_SCRATCH_BASE] =
        window.range_base;
    context->scratch[VAL_FIRME_GPT_INFO_SCRATCH_TOP] =
        window.range_top;
    context->scratch[VAL_FIRME_GPT_INFO_SCRATCH_RANGE_SIZE] =
        window.range_size;
    context->scratch[VAL_FIRME_GPT_INFO_SCRATCH_GRANULE] =
        window.granule_size;
    context->scratch[VAL_FIRME_GPT_INFO_SCRATCH_PPS_SIZE] =
        window.protected_pa_size;

    return VAL_SUCCESS;
}

/**
 * @brief Build a valid FIRME_GM_GPT_INFO input frame.
 * @param context Generated test execution context.
 * @return VAL_SUCCESS on success, otherwise VAL_ERROR.
 */
static int val_firme_abi_gpt_info_build(
    val_firme_abi_test_context_t *context)
{
    if (!context)
        return VAL_ERROR;

    context->input.registers[1] =
        context->scratch[VAL_FIRME_GPT_INFO_SCRATCH_BASE];
    context->input.registers[2] =
        context->scratch[VAL_FIRME_GPT_INFO_SCRATCH_TOP];

    return VAL_SUCCESS;
}

/**
 * @brief Validate FIRME_GM_GPT_INFO status, returned top and PAR state.
 * @param context Generated test execution context.
 * @return VAL_TRUE when the output follows ALP4, otherwise VAL_FALSE.
 */
static uint32_t val_firme_abi_gpt_info_validate(
    val_firme_abi_test_context_t *context)
{
    val_firme_gpt_info_window_t window;
    firme_res_t result;

    if (!context)
        return VAL_FALSE;

    window.range_base =
        context->scratch[VAL_FIRME_GPT_INFO_SCRATCH_BASE];
    window.range_top =
        context->scratch[VAL_FIRME_GPT_INFO_SCRATCH_TOP];
    window.range_size =
        context->scratch[VAL_FIRME_GPT_INFO_SCRATCH_RANGE_SIZE];
    window.granule_size =
        context->scratch[VAL_FIRME_GPT_INFO_SCRATCH_GRANULE];
    window.protected_pa_size =
        context->scratch[VAL_FIRME_GPT_INFO_SCRATCH_PPS_SIZE];

    result.x0 = context->output.registers[0];
    result.x1 = context->output.registers[1];
    result.x2 = context->output.registers[2];
    result.x3 = context->output.registers[3];

    return val_firme_gpt_info_success_valid(
        &window, &result, NULL, 0ULL);
}

static const val_firme_abi_test_ops_t g_service_version_ops = {
    .build_valid_input = val_firme_abi_service_version_build,
    .apply_mutation = val_firme_abi_service_version_mutate,
    .validate_output = val_firme_abi_service_version_validate,
};

static const val_firme_abi_test_ops_t g_service_features_ops = {
    .build_valid_input = val_firme_abi_service_features_build,
    .apply_mutation = val_firme_abi_service_features_mutate,
    .validate_output = val_firme_abi_service_features_validate,
};

static const val_firme_abi_test_ops_t g_mec_refresh_ops = {
    .build_valid_input = val_firme_abi_mec_refresh_build,
    .apply_mutation = val_firme_abi_mec_refresh_mutate,
    .validate_output = val_firme_abi_mec_refresh_validate,
};

static const val_firme_abi_test_ops_t g_gpt_info_ops = {
    .prepare_state = val_firme_abi_gpt_info_prepare,
    .build_valid_input = val_firme_abi_gpt_info_build,
    .validate_output = val_firme_abi_gpt_info_validate,
};

/**
 * @brief Return the service-specific operations for an Alpha1 ABI.
 * @param abi_id Generated Alpha1 ABI identifier.
 * @return Adapter operations, or NULL when the ABI has not been enabled.
 */
const val_firme_abi_test_ops_t *val_firme_abi_adapter_get(
    val_firme_abi_id_t abi_id)
{
    if (abi_id == VAL_FIRME_ABI_ID_SERVICE_VERSION)
        return &g_service_version_ops;

    if (abi_id == VAL_FIRME_ABI_ID_SERVICE_FEATURES)
        return &g_service_features_ops;

    if (abi_id == VAL_FIRME_ABI_ID_MEC_REFRESH)
        return &g_mec_refresh_ops;

    if (abi_id == VAL_FIRME_ABI_ID_GM_GPT_INFO)
        return &g_gpt_info_ops;

    if ((abi_id == VAL_FIRME_ABI_ID_GM_GPI_SET) ||
        (abi_id == VAL_FIRME_ABI_ID_GM_GPI_OP_CONTINUE) ||
        (abi_id == VAL_FIRME_ABI_ID_GM_L1_GPT_CREATE) ||
        (abi_id == VAL_FIRME_ABI_ID_GM_L1_GPT_DESTROY))
        return val_firme_gpt_abi_adapter_get(abi_id);

    if ((abi_id == VAL_FIRME_ABI_ID_IDE_KEYSET_PROG) ||
        (abi_id == VAL_FIRME_ABI_ID_IDE_KEYSET_GO) ||
        (abi_id == VAL_FIRME_ABI_ID_IDE_KEYSET_STOP) ||
        (abi_id == VAL_FIRME_ABI_ID_IDE_KEYSET_POLL))
        return val_firme_ide_abi_adapter_get(abi_id);

    return NULL;
}
