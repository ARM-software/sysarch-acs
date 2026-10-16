/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#define main native_existing_fixture_tests
#include "gpt_fixture_test.c"
#undef main

/** @brief Invoke exactly the generated frame and retain its architected outputs. */
static void native_generated_call(val_firme_abi_test_context_t *context)
{
    firme_res_t result = {0};

    val_firme_gm_gpi_set(context->input.registers[1], context->input.registers[2],
                         context->input.registers[3], &result);
    context->call_issued = VAL_TRUE;
    context->output.registers[0] = result.x0;
    context->output.registers[1] = result.x1;
    context->output.registers[2] = result.x2;
}

/** @brief Simple GPI_SET cases must not depend on an unused continuation ABI. */
static void native_set_without_continue_test(void)
{
    val_firme_abi_descriptor_t abi = {.id = VAL_FIRME_ABI_ID_GM_GPI_SET};
    val_firme_abi_case_t test_case = {.mutation = VAL_FIRME_MUTATION_NONE};
    val_firme_abi_test_context_t context = {
        .abi = &abi, .test_case = &test_case, .world = FIRME_ACS_WORLD_REALM,
    };
    const val_firme_abi_test_ops_t *ops = val_firme_gpt_abi_adapter_get(abi.id);

    native_reset();
    native_abis = FIRME_GM_FEATURE_REG0_GPI_SET_BIT;
    assert(ops && ops->prepare_state(&context) == VAL_SUCCESS);
    assert(ops->build_valid_input(&context) == VAL_SUCCESS);
    assert(native_acquires == 1U && native_issued == 0U);
    native_reply(NATIVE_SET, FIRME_SUCCESS, 1ULL, UINT64_MAX);
    native_generated_call(&context);
    assert(ops->validate_output(&context) == VAL_TRUE);
    native_reply(NATIVE_SET, FIRME_SUCCESS, NATIVE_COUNT - 1U, UINT64_MAX);
    native_reply(NATIVE_SET, FIRME_SUCCESS, NATIVE_COUNT, UINT64_MAX);
    assert(ops->cleanup(&context) == VAL_SUCCESS);
    assert(native_calls[1].argument == NATIVE_BASE + native_pgs);
    assert(native_calls[1].count == NATIVE_COUNT - 1U);
    assert(native_releases == 1U && !native_lease_live);
    assert(native_issued == native_queued);
}

/** @brief Both independent invalid-input checks still require no observed GPI change. */
static void native_set_negative_without_continue_test(void)
{
    static const val_firme_abi_mutation_t mutations[] = {
        VAL_FIRME_MUTATION_GM_UNALIGNED_BASE, VAL_FIRME_MUTATION_GM_ZERO_COUNT,
    };
    val_firme_abi_descriptor_t abi = {.id = VAL_FIRME_ABI_ID_GM_GPI_SET};
    val_firme_abi_case_t test_case = {0};
    val_firme_abi_test_context_t context;
    const val_firme_abi_test_ops_t *ops = val_firme_gpt_abi_adapter_get(abi.id);
    uint32_t mutation_index;

    for (mutation_index = 0U; mutation_index < sizeof(mutations) / sizeof(mutations[0]);
         mutation_index++) {
        native_reset();
        native_abis = FIRME_GM_FEATURE_REG0_GPI_SET_BIT;
        test_case.mutation = mutations[mutation_index];
        context = (val_firme_abi_test_context_t){
            .abi = &abi, .test_case = &test_case, .world = FIRME_ACS_WORLD_REALM,
        };
        assert(ops->prepare_state(&context) == VAL_SUCCESS);
        assert(ops->build_valid_input(&context) == VAL_SUCCESS);
        assert(ops->apply_mutation(&context) == VAL_SUCCESS);
        native_reply(NATIVE_SET, FIRME_INVALID_PARAMETERS, UINT64_MAX, UINT64_MAX);
        native_generated_call(&context);
        assert(ops->validate_output(&context) == VAL_TRUE);
        assert(ops->cleanup(&context) == VAL_SUCCESS);
        assert(native_releases == 1U && !native_lease_live);
        assert(native_issued == native_queued);
    }
}

/** @brief Unexpected retained state is quarantined, even with an opaque zero cookie. */
static void native_unadvertised_continue_quarantine_test(void)
{
    val_firme_abi_descriptor_t abi = {.id = VAL_FIRME_ABI_ID_GM_GPI_SET};
    val_firme_abi_case_t test_case = {.mutation = VAL_FIRME_MUTATION_NONE};
    val_firme_abi_test_context_t context;
    val_firme_abi_test_context_t next_context;
    const val_firme_abi_test_ops_t *ops = val_firme_gpt_abi_adapter_get(abi.id);
    val_firme_gpt_fixture_t *fixture;
    uint64_t token;
    uint32_t during_cleanup;

    for (during_cleanup = VAL_FALSE; during_cleanup <= VAL_TRUE; during_cleanup++) {
        native_reset();
        native_abis = FIRME_GM_FEATURE_REG0_GPI_SET_BIT;
        context = (val_firme_abi_test_context_t){
            .abi = &abi, .test_case = &test_case, .world = FIRME_ACS_WORLD_REALM,
        };
        assert(ops->prepare_state(&context) == VAL_SUCCESS);
        assert(ops->build_valid_input(&context) == VAL_SUCCESS);
        fixture = &g_gpt_adapter_fixture[context.world];
        token = fixture->memory.token;
        assert(token && !fixture->continue_advertised);
        native_reply(NATIVE_SET, during_cleanup ? FIRME_SUCCESS : FIRME_INCOMPLETE,
                     during_cleanup ? NATIVE_COUNT : 1ULL, 0ULL);
        native_generated_call(&context);
        assert(ops->validate_output(&context) == during_cleanup);
        if (during_cleanup)
            native_reply(NATIVE_SET, FIRME_INCOMPLETE, 1ULL, 0ULL);

        assert(ops->cleanup(&context) == VAL_ERROR);
        assert(fixture->pending && fixture->poisoned && fixture->cookie == 0ULL);
        assert(fixture->completed == 1ULL && fixture->memory.token == token);
        assert(native_lease_live && !native_releases);
        assert(native_issued == native_queued);
        next_context = (val_firme_abi_test_context_t){
            .abi = &abi, .test_case = &test_case, .world = FIRME_ACS_WORLD_REALM,
        };
        assert(ops->prepare_state(&next_context) == VAL_FIRME_ABI_SCENARIO_UNAVAILABLE);
        assert(ops->cleanup(&context) == VAL_ERROR);
        assert(native_issued == native_queued && !native_releases);
    }
}

/** @brief Retained-operation fixtures still require their continuation entry point. */
static void native_continue_prerequisite_test(void)
{
    val_firme_abi_descriptor_t abi = {.id = VAL_FIRME_ABI_ID_GM_GPI_OP_CONTINUE};
    val_firme_abi_case_t test_case = {.mutation = VAL_FIRME_MUTATION_NONE};
    val_firme_abi_test_context_t context = {
        .abi = &abi, .test_case = &test_case, .world = FIRME_ACS_WORLD_REALM,
    };
    const val_firme_abi_test_ops_t *ops = val_firme_gpt_abi_adapter_get(abi.id);

    native_reset();
    native_abis = FIRME_GM_FEATURE_REG0_GPI_SET_BIT;
    assert(ops->prepare_state(&context) == VAL_ERROR);
    assert(ops->cleanup(&context) == VAL_SUCCESS);
    assert(!native_issued && !native_acquires && !native_releases);
}

/** @brief Keep rejected requests failed while releasing proven-unchanged storage. */
static void native_rejected_set_cleanup_test(void)
{
    static const int64_t errors[] = {FIRME_INVALID_PARAMETERS, FIRME_NOT_SUPPORTED, FIRME_DENIED};
    val_firme_abi_descriptor_t abi = {.id = VAL_FIRME_ABI_ID_GM_GPI_SET};
    val_firme_abi_case_t test_case = {.mutation = VAL_FIRME_MUTATION_NONE};
    val_firme_abi_test_context_t context;
    const val_firme_abi_test_ops_t *ops = val_firme_gpt_abi_adapter_get(abi.id);
    uint32_t error_index;
    uint32_t broken_observation;

    for (error_index = 0U; error_index < sizeof(errors) / sizeof(errors[0]); error_index++) {
        for (broken_observation = VAL_FALSE; broken_observation <= VAL_TRUE;
             broken_observation++) {
            native_reset();
            context = (val_firme_abi_test_context_t){
                .abi = &abi, .test_case = &test_case, .world = FIRME_ACS_WORLD_REALM,
            };

            /* Step 1: Issue a positive request and retain its actual rejected status. */
            assert(ops->prepare_state(&context) == VAL_SUCCESS);
            assert(ops->build_valid_input(&context) == VAL_SUCCESS);
            native_reply(NATIVE_SET, errors[error_index],
                         errors[error_index] == FIRME_DENIED ? 0ULL : UINT64_MAX, UINT64_MAX);
            native_generated_call(&context);

            /* Step 2: Independent state, not the terminal error, determines reusability. */
            if (broken_observation)
                native_gpis[NATIVE_COUNT - 1U] = FIRME_GPI_REALM;

            assert(ops->validate_output(&context) == VAL_FALSE);
            assert(!context.scratch[GPT_ADAPTER_POSITIVE]);
            assert(ops->cleanup(&context) == (broken_observation ? VAL_ERROR : VAL_SUCCESS));
            assert(native_releases == (broken_observation ? 0U : 1U));
            assert(native_issued == 1U && native_issued == native_queued);
        }
    }
}

/** @brief Run production-fixture regressions; this is not live DUT qualification. */
int main(void)
{
    assert(native_existing_fixture_tests() == VAL_SUCCESS);
    native_set_without_continue_test();
    native_set_negative_without_continue_test();
    native_unadvertised_continue_quarantine_test();
    native_continue_prerequisite_test();
    native_rejected_set_cleanup_test();
    puts("GPT generated GPI_SET prerequisite/quarantine regressions: PASS");
    return VAL_SUCCESS;
}
