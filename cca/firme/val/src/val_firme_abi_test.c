/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi_test.h"

#include <stddef.h>

#include "val_common/val_status.h"
#include "val_firme_abi.h"
#include "val_firme_service.h"
#include "val_log.h"

#define VAL_FIRME_ABI_ADVERTISEMENT_STEP UINT64_C(1)
#define VAL_FIRME_ABI_RETRY_LIMIT        16U

_Static_assert(VAL_SMCCC_REGISTER_COUNT == VAL_FIRME_ABI_REGISTER_COUNT,
               "SMCCC and FIRME ABI frame sizes differ");

/**
 * @brief Clear one SMC call frame without relying on a hosted C library.
 * @param frame Frame to clear.
 * @return None.
 */
static void val_firme_abi_frame_clear(smccc_call_frame_t *frame)
{
    uint32_t register_index;

    for (register_index = 0U;
         register_index < VAL_SMCCC_REGISTER_COUNT;
         register_index++)
        frame->registers[register_index] = 0ULL;
}

/**
 * @brief Clear per-case service-adapter scratch storage.
 * @param context Generated-plan execution context.
 * @return None.
 */
static void val_firme_abi_scratch_clear(
    val_firme_abi_test_context_t *context)
{
    uint32_t scratch_index;

    for (scratch_index = 0U;
         scratch_index < VAL_FIRME_ABI_SCRATCH_QWORDS;
         scratch_index++)
        context->scratch[scratch_index] = 0ULL;
}

/**
 * @brief Initialize a rule result before executing a generated plan.
 * @param out Result to initialize.
 * @return None.
 */
static void val_firme_abi_result_init(firme_rule_res_t *out)
{
    out->status = (uint64_t)ACS_STATUS_FAIL;
    out->fail_step = 0ULL;
    out->last_x0 = 0ULL;
    out->last_x1 = 0ULL;
}

/**
 * @brief Record a generated-plan failure.
 * @param out Rule result to update.
 * @param step Diagnostic step from the generated case.
 * @param context Current execution context.
 * @return None.
 */
static void val_firme_abi_result_fail(firme_rule_res_t *out, uint64_t step,
                                      const val_firme_abi_test_context_t *context)
{
    out->status = (uint64_t)ACS_STATUS_FAIL;
    out->fail_step = step;
    out->last_x0 = context->output.registers[0];
    out->last_x1 = context->output.registers[1];
}

/**
 * @brief Convert a dispatcher caller world into a catalog array index.
 * @param world Dispatcher caller world.
 * @param world_index Returned catalog index.
 * @return VAL_SUCCESS when the world is valid, otherwise VAL_ERROR.
 */
static int val_firme_abi_world_index(firme_acs_world_t world,
                                     uint32_t *world_index)
{
    if (!world_index)
        return VAL_ERROR;

    if (world == FIRME_ACS_WORLD_NSEL2) {
        *world_index = (uint32_t)VAL_FIRME_ABI_WORLD_NS;
        return VAL_SUCCESS;
    }

    if (world == FIRME_ACS_WORLD_SECURE) {
        *world_index = (uint32_t)VAL_FIRME_ABI_WORLD_SECURE;
        return VAL_SUCCESS;
    }

    if (world == FIRME_ACS_WORLD_REALM) {
        *world_index = (uint32_t)VAL_FIRME_ABI_WORLD_REALM;
        return VAL_SUCCESS;
    }

    return VAL_ERROR;
}

/**
 * @brief Check whether the plan ABI is advertised at the caller instance.
 * @param context Generated-plan execution context.
 * @param out Rule result populated when execution must stop.
 * @return VAL_TRUE when execution may continue, VAL_FALSE for a completed
 *         FAIL/SKIP result, or VAL_ERROR for a framework query failure.
 */
static int val_firme_abi_check_advertisement(
    const val_firme_abi_test_context_t *context, firme_rule_res_t *out)
{
    firme_res_t features_res = {0};
    firme_res_t version_res = {0};
    uint64_t feature_reg0 = 0ULL;
    uint32_t advertised = VAL_FALSE;
    uint32_t implemented = VAL_FALSE;
    uint32_t world_index = 0U;
    uint32_t world_mask;
    val_firme_abi_world_policy_t policy;
    int query_status;

    if (context->plan->require_advertised == 0U)
        return VAL_TRUE;

    if (val_firme_abi_world_index(context->world, &world_index) != VAL_SUCCESS) {
        LOG_ERROR(NULL, "%s: invalid caller world=%u",
                  context->plan->name, (unsigned int)context->world);
        return VAL_ERROR;
    }

    world_mask = UINT32_C(1) << world_index;
    query_status = val_firme_service_get_version(
        context->abi->service_id, &implemented, &version_res);
    if (query_status != VAL_SUCCESS) {
        LOG_ERROR(NULL,
                  "%s: invalid %s service version result x0=%llx",
                  context->plan->name, context->abi->name,
                  (unsigned long long)version_res.x0);
        out->fail_step = VAL_FIRME_ABI_ADVERTISEMENT_STEP;
        out->last_x0 = version_res.x0;
        out->last_x1 = version_res.x1;
        return VAL_ERROR;
    }

    if (implemented == VAL_FALSE) {
        out->fail_step = 0ULL;
        out->last_x0 = version_res.x0;
        out->last_x1 = version_res.x1;
        if ((context->plan->required_worlds_mask & world_mask) == 0U) {
            out->status = (uint64_t)ACS_STATUS_SKIP;
            return VAL_FALSE;
        }

        LOG_ERROR(NULL, "%s: required service %u is not implemented",
                  context->plan->name,
                  (unsigned int)context->abi->service_id);
        out->fail_step = VAL_FIRME_ABI_ADVERTISEMENT_STEP;
        return VAL_FALSE;
    }

    /* A positive SERVICE_VERSION result proves this ABI is implemented. */
    if (context->abi->id == VAL_FIRME_ABI_ID_SERVICE_VERSION)
        return VAL_TRUE;

    query_status = val_firme_service_abi_is_advertised(
        context->abi->service_id, context->abi->feature_bit, &advertised,
        &feature_reg0, &features_res);
    if (query_status != VAL_SUCCESS) {
        LOG_ERROR(NULL,
                  "%s: failed to query %s advertisement x0=%llx x1=%llx",
                  context->plan->name, context->abi->name,
                  (unsigned long long)features_res.x0,
                  (unsigned long long)features_res.x1);
        out->fail_step = VAL_FIRME_ABI_ADVERTISEMENT_STEP;
        out->last_x0 = features_res.x0;
        out->last_x1 = features_res.x1;
        return VAL_ERROR;
    }

    if (advertised == VAL_TRUE)
        return VAL_TRUE;

    policy = context->abi->world_policy[world_index];
    if (policy != VAL_FIRME_ABI_WORLD_MANDATORY) {
        out->status = (uint64_t)ACS_STATUS_SKIP;
        out->fail_step = 0ULL;
        out->last_x0 = features_res.x0;
        out->last_x1 = feature_reg0;
        return VAL_FALSE;
    }

    LOG_ERROR(NULL, "%s: mandatory ABI %s is not advertised reg0=%llx",
              context->plan->name, context->abi->name,
              (unsigned long long)feature_reg0);
    out->fail_step = VAL_FIRME_ABI_ADVERTISEMENT_STEP;
    out->last_x0 = features_res.x0;
    out->last_x1 = feature_reg0;
    return VAL_FALSE;
}

/**
 * @brief Invoke an ABI and apply its generic retry execution model.
 * @param context Generated-plan execution context.
 * @return None.
 */
static void val_firme_abi_invoke(val_firme_abi_test_context_t *context)
{
    uint32_t attempt;
    int64_t status;

    for (attempt = 0U; attempt < VAL_FIRME_ABI_RETRY_LIMIT; attempt++) {
        val_smccc_smc_call_frame(&context->input, &context->output);
        if (context->abi->execution_model !=
            VAL_FIRME_ABI_EXEC_RETRYABLE)
            return;

        status = val_firme_status_to_i64(
            context->output.registers[0]);
        if (status != (int64_t)FIRME_BUSY)
            return;
    }
}

/**
 * @brief Verify the adapter produced an input frame valid for this ABI.
 * @param context Current generated-plan execution context.
 * @param out Rule result populated on an invalid frame.
 * @return VAL_SUCCESS when all reserved input fields are zero, else VAL_ERROR.
 */
static int val_firme_abi_validate_input(
    const val_firme_abi_test_context_t *context, firme_rule_res_t *out)
{
    uint64_t invalid_bits;
    uint32_t register_index;

    if (context->input.registers[0] != context->abi->fid) {
        LOG_ERROR(NULL, "%s: %s input x0=%llx expected fid=%llx",
                  context->plan->name, context->abi->name,
                  (unsigned long long)context->input.registers[0],
                  (unsigned long long)context->abi->fid);
        out->fail_step = context->test_case->diagnostic_step;
        out->last_x0 = context->input.registers[0];
        out->last_x1 = context->abi->fid;
        return VAL_ERROR;
    }

    for (register_index = 0U;
         register_index < VAL_SMCCC_REGISTER_COUNT;
         register_index++) {
        invalid_bits = context->input.registers[register_index] &
                       ~context->abi->input_bit_masks[register_index];
        if (invalid_bits != 0ULL) {
            LOG_ERROR(NULL,
                      "%s: %s case=%s input x%u has reserved bits=%llx",
                      context->plan->name, context->abi->name,
                      context->test_case->name,
                      (unsigned int)register_index,
                      (unsigned long long)invalid_bits);
            out->fail_step = context->test_case->diagnostic_step;
            out->last_x0 = context->input.registers[register_index];
            out->last_x1 =
                context->abi->input_bit_masks[register_index];
            return VAL_ERROR;
        }
    }

    return VAL_SUCCESS;
}

/**
 * @brief Validate the generated status or positive-version expectation.
 * @param context Current generated-plan execution context.
 * @return VAL_TRUE when x0 matches the generated expectation, else VAL_FALSE.
 */
static uint32_t val_firme_abi_validate_expectation(
    const val_firme_abi_test_context_t *context)
{
    int64_t actual_status;
    uint32_t status_index;

    actual_status =
        val_firme_status_to_i64(context->output.registers[0]);

    if (context->test_case->expectation == VAL_FIRME_EXPECT_POSITIVE) {
        if (actual_status > 0)
            return VAL_TRUE;

        LOG_ERROR(NULL,
                  "%s: %s case=%s expected positive result x0=%llx",
                  context->plan->name, context->abi->name,
                  context->test_case->name,
                  (unsigned long long)context->output.registers[0]);
        return VAL_FALSE;
    }

    if (context->test_case->expectation ==
        VAL_FIRME_EXPECT_ARCHITECTED_STATUS) {
        if ((actual_status <= 0) && (actual_status >= -31)) {
            status_index = (uint32_t)-actual_status;
            if ((context->abi->allowed_status_mask &
                 (UINT32_C(1) << status_index)) != 0U)
                return VAL_TRUE;
        }

        LOG_ERROR(NULL,
                  "%s: %s case=%s returned non-architected status=%lld",
                  context->plan->name, context->abi->name,
                  context->test_case->name, (long long)actual_status);
        return VAL_FALSE;
    }

    if (actual_status == (int64_t)context->test_case->expected_status)
        return VAL_TRUE;

    LOG_ERROR(NULL,
              "%s: %s case=%s expected status=%lld actual=%lld x0=%llx",
              context->plan->name, context->abi->name,
              context->test_case->name,
              (long long)context->test_case->expected_status,
              (long long)actual_status,
              (unsigned long long)context->output.registers[0]);
    return VAL_FALSE;
}

/**
 * @brief Execute one generated case and all adapter lifecycle hooks.
 * @param context Mutable execution context for the case.
 * @param ops Service-specific adapter hooks.
 * @param out Rule result populated on failure.
 * @return VAL_SUCCESS for an architectural PASS/FAIL, otherwise VAL_ERROR.
 */
static int val_firme_abi_execute_case(
    val_firme_abi_test_context_t *context,
    const val_firme_abi_test_ops_t *ops,
    firme_rule_res_t *out)
{
    uint32_t case_pass = VAL_TRUE;
    uint32_t call_issued = VAL_FALSE;
    int adapter_status = VAL_SUCCESS;
    int cleanup_status = VAL_SUCCESS;

    val_firme_abi_frame_clear(&context->input);
    val_firme_abi_frame_clear(&context->output);
    val_firme_abi_scratch_clear(context);
    context->input.registers[0] = context->abi->fid;

    if (ops->prepare_state &&
        (ops->prepare_state(context) != VAL_SUCCESS))
        adapter_status = VAL_ERROR;

    if ((adapter_status == VAL_SUCCESS) &&
        (ops->build_valid_input(context) != VAL_SUCCESS))
        adapter_status = VAL_ERROR;

    if ((adapter_status == VAL_SUCCESS) &&
        (context->test_case->mutation != VAL_FIRME_MUTATION_NONE)) {
        if (!ops->apply_mutation ||
            (ops->apply_mutation(context) != VAL_SUCCESS))
            adapter_status = VAL_ERROR;
    }

    if ((adapter_status == VAL_SUCCESS) &&
        (val_firme_abi_validate_input(context, out) != VAL_SUCCESS))
        adapter_status = VAL_ERROR;

    if (adapter_status == VAL_SUCCESS) {
        val_firme_abi_invoke(context);
        call_issued = VAL_TRUE;
        case_pass = val_firme_abi_validate_expectation(context);

        if ((case_pass == VAL_TRUE) && ops->validate_output)
            case_pass = ops->validate_output(context);

        if ((case_pass == VAL_TRUE) &&
            (context->test_case->verify_no_effect != 0U)) {
            if (!ops->validate_effect)
                adapter_status = VAL_ERROR;
            else
                case_pass = ops->validate_effect(context);
        }
    }

    if (ops->cleanup)
        cleanup_status = ops->cleanup(context);

    if ((adapter_status != VAL_SUCCESS) ||
        (cleanup_status != VAL_SUCCESS)) {
        LOG_ERROR(NULL,
                  "%s: adapter error for %s case=%s issued=%u cleanup=%d",
                  context->plan->name, context->abi->name,
                  context->test_case->name, (unsigned int)call_issued,
                  cleanup_status);
        if (out->fail_step == 0ULL) {
            val_firme_abi_result_fail(
                out, context->test_case->diagnostic_step, context);
        } else {
            out->status = (uint64_t)ACS_STATUS_FAIL;
        }
        return VAL_ERROR;
    }

    if (case_pass == VAL_FALSE) {
        LOG_ERROR(NULL, "%s: validation failed for %s case=%s",
                  context->plan->name, context->abi->name,
                  context->test_case->name);
        val_firme_abi_result_fail(
            out, context->test_case->diagnostic_step, context);
    }

    return VAL_SUCCESS;
}

/**
 * @brief Execute a generated ABI test plan through service-specific adapters.
 * @param request Test-plan identifier, caller world, state and adapter hooks.
 * @param out Rule result populated with PASS, FAIL or SKIP.
 * @return VAL_SUCCESS when an architectural result is produced, otherwise
 *         VAL_ERROR for an ACS framework error.
 */
int val_firme_abi_test_execute(const val_firme_abi_test_request_t *request,
                               firme_rule_res_t *out)
{
    val_firme_abi_test_context_t context;
    const val_firme_abi_test_plan_t *plan;
    const val_firme_abi_descriptor_t *abi;
    uint32_t case_index;
    uint32_t plan_worlds;
    uint32_t world_mask;
    uint32_t world_index;
    int advertisement_status;
    int case_status;

    if (!out)
        return VAL_ERROR;

    val_firme_abi_result_init(out);

    if (!request || !request->ops ||
        !request->ops->build_valid_input)
        return VAL_ERROR;

    if (val_firme_abi_world_index(request->world, &world_index) !=
        VAL_SUCCESS)
        return VAL_ERROR;

    plan = val_firme_abi_test_plan_get(request->plan_id);
    if (!plan || (plan->enabled == 0U))
        return VAL_ERROR;

    world_mask = UINT32_C(1) << world_index;
    plan_worlds = plan->required_worlds_mask |
                  plan->optional_worlds_mask;
    if ((plan_worlds & world_mask) == 0U)
        return VAL_ERROR;

    abi = val_firme_abi_catalog_get(plan->abi_id);
    if (!abi)
        return VAL_ERROR;

    if ((plan->baseline_policy == VAL_FIRME_BASELINE_ADAPTER_VALIDATED) &&
        !request->ops->prepare_state)
        return VAL_ERROR;

    if ((plan->require_output_validation != 0U) &&
        !request->ops->validate_output)
        return VAL_ERROR;

    context.abi = abi;
    context.plan = plan;
    context.test_case = NULL;
    context.world = request->world;
    context.base_pa = request->base_pa;
    context.adapter_context = request->adapter_context;
    val_firme_abi_frame_clear(&context.input);
    val_firme_abi_frame_clear(&context.output);
    val_firme_abi_scratch_clear(&context);

    advertisement_status =
        val_firme_abi_check_advertisement(&context, out);
    if (advertisement_status == VAL_ERROR)
        return VAL_ERROR;

    if (advertisement_status == VAL_FALSE)
        return VAL_SUCCESS;

    out->status = (uint64_t)ACS_STATUS_PASS;

    for (case_index = 0U; case_index < plan->case_count; case_index++) {
        context.test_case = &plan->cases[case_index];
        case_status =
            val_firme_abi_execute_case(&context, request->ops, out);
        if (case_status != VAL_SUCCESS)
            return VAL_ERROR;

        if (out->status == (uint64_t)ACS_STATUS_FAIL)
            return VAL_SUCCESS;
    }

    out->status = (uint64_t)ACS_STATUS_PASS;
    return VAL_SUCCESS;
}
