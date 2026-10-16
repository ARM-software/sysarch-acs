/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../../val/src/val_firme_gpt_abi_adapter.c"

#define NATIVE_BASE UINT64_C(0x40000000)
#define NATIVE_GRANULE UINT64_C(0x1000)
#define NATIVE_COUNT UINT64_C(4)

static uint32_t g_l1_calls;
static uint32_t g_last_destroy;
static uint32_t g_releases;
static uint32_t g_transition_completions;
static int g_finish_status;
static uint64_t g_observed_gpi;
static uint64_t g_owned_count = NATIVE_COUNT;

/** @brief Supply a real callback reference whose retained lifetime can be checked. */
static int native_invoke(const smccc_call_frame_t *input, smccc_call_frame_t *output,
                          uint32_t *issued, void *cookie)
{
    *issued = VAL_FALSE;
    return VAL_ERROR;
}

/** @brief Reset ownership and the independent fixture outputs between cases. */
static void native_reset(val_firme_abi_test_context_t *context)
{
    memset(context->scratch, 0, sizeof(context->scratch));
    memset(g_gpt_adapter_fixture, 0, sizeof(g_gpt_adapter_fixture));
    memset(g_gpt_adapter_lock, 0, sizeof(g_gpt_adapter_lock));
    context->call_issued = VAL_FALSE;
    context->transport_uncertain = VAL_FALSE;
    g_l1_calls = g_last_destroy = g_releases = g_transition_completions = 0U;
    g_finish_status = VAL_SUCCESS;
    g_observed_gpi = FIRME_GPI_NONSECURE;
    g_owned_count = NATIVE_COUNT;
}

/** @brief Decode W0 and ignore unused upper X0 bits. */
int64_t val_firme_status_to_i64(uint64_t value)
{
    return (int64_t)(int32_t)(uint32_t)value;
}

/** @brief Supply a known owned allocation without issuing a DUT call. */
pal_firme_gpt_scenario_status_t val_firme_gpt_fixture_begin_abi(
    val_firme_gpt_fixture_t *fixture, val_firme_abi_test_context_t *context,
    pal_firme_gpt_memory_kind_t kind)
{
    *fixture = (val_firme_gpt_fixture_t){
        .memory = {.range_base = NATIVE_BASE, .range_size = NATIVE_GRANULE * g_owned_count,
                   .object_base = NATIVE_BASE + NATIVE_GRANULE,
                   .initial_gpi = FIRME_GPI_NONSECURE, .token = 1ULL},
        .world = context->world, .kind = kind, .granule_size = NATIVE_GRANULE,
        .count = g_owned_count, .target_gpi = FIRME_GPI_REALM,
        .has_abi_context = VAL_TRUE, .abi_context = *context,
    };
    return PAL_FIRME_GPT_SCENARIO_READY;
}

/** @brief No asynchronous GPI setup is needed by these adapter-contract cases. */
pal_firme_gpt_scenario_status_t val_firme_gpt_fixture_retain(val_firme_gpt_fixture_t *fixture)
{
    return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
}

/** @brief Record the returned status independently of terminal completion. */
int val_firme_gpt_fixture_gpi_record(val_firme_gpt_fixture_t *fixture,
                                     const firme_res_t *result)
{
    fixture->last_status = val_firme_status_to_i64(result->x0);
    return VAL_SUCCESS;
}

/** @brief Model L1 BUSY without claiming that the operation has started. */
int val_firme_gpt_fixture_l1_record(val_firme_gpt_fixture_t *fixture,
                                    uint32_t destroy, const firme_res_t *result)
{
    fixture->last_status = val_firme_status_to_i64(result->x0);
    fixture->l1_destroy = destroy;
    return VAL_SUCCESS;
}

/** @brief Count terminal-completion attempts separately from rollback. */
int val_firme_gpt_fixture_l1_complete(val_firme_gpt_fixture_t *fixture, uint32_t destroy)
{
    g_l1_calls++;
    g_last_destroy = destroy;
    return g_finish_status;
}

/** @brief Detect accidental positive-transition completion for rejected inputs. */
int val_firme_gpt_fixture_complete_transition(val_firme_gpt_fixture_t *fixture)
{
    g_transition_completions++;
    return g_finish_status;
}

/** @brief Never release a fixture whose operation ownership is uncertain. */
int val_firme_gpt_fixture_cleanup(val_firme_gpt_fixture_t *fixture)
{
    if (fixture->poisoned)
        return VAL_ERROR;
    g_releases++;
    return VAL_SUCCESS;
}

/** @brief Independently check the whole reserved extent, not the return count. */
pal_firme_gpt_scenario_status_t pal_firme_gpt_l1_gpi_range_check(
    uint64_t base, uint64_t size, uint64_t granule, uint64_t exclude_base,
    uint64_t exclude_size, uint32_t allowed, uint64_t *checked)
{
    assert(base == NATIVE_BASE && granule == NATIVE_GRANULE);
    assert(size == NATIVE_COUNT * NATIVE_GRANULE && exclude_size == 0ULL);
    *checked = NATIVE_COUNT;
    return (allowed & (UINT32_C(1) << g_observed_gpi)) ?
           PAL_FIRME_GPT_SCENARIO_READY : PAL_FIRME_GPT_SCENARIO_ERROR;
}

/** @brief Preserve failed completion and independently verify rejected requests' effects. */
int main(void)
{
    val_firme_abi_descriptor_t abi = {.id = VAL_FIRME_ABI_ID_GM_L1_GPT_CREATE};
    val_firme_abi_case_t test_case = {.mutation = VAL_FIRME_MUTATION_NONE};
    val_firme_abi_test_context_t context = {
        .abi = &abi, .test_case = &test_case, .world = FIRME_ACS_WORLD_REALM,
        .invoke_frame = native_invoke, .invoke_cookie = &abi,
    };

    native_reset(&context);
    assert(gpt_adapter_prepare(&context) == VAL_SUCCESS);
    assert(gpt_adapter_build(&context) == VAL_SUCCESS);
    context.call_issued = VAL_TRUE;
    context.output.registers[0] = (uint64_t)FIRME_BUSY;
    assert(gpt_adapter_validate(&context) == VAL_TRUE);
    assert(gpt_adapter_cleanup(&context) == VAL_SUCCESS);
    assert(g_l1_calls == 1U && g_last_destroy == VAL_FALSE && g_releases == 1U);

    native_reset(&context);
    assert(gpt_adapter_prepare(&context) == VAL_SUCCESS);
    assert(gpt_adapter_build(&context) == VAL_SUCCESS);
    context.call_issued = VAL_TRUE;
    assert(gpt_adapter_validate(&context) == VAL_TRUE);
    g_finish_status = VAL_ERROR;
    assert(gpt_adapter_cleanup(&context) == VAL_ERROR);
    assert(g_l1_calls == 1U && g_releases == 1U);

    native_reset(&context);
    abi.id = VAL_FIRME_ABI_ID_GM_L1_GPT_DESTROY;
    assert(gpt_adapter_prepare(&context) == VAL_SUCCESS && g_l1_calls == 1U);
    assert(gpt_adapter_build(&context) == VAL_SUCCESS);
    context.call_issued = VAL_TRUE;
    assert(gpt_adapter_validate(&context) == VAL_TRUE);
    assert(gpt_adapter_cleanup(&context) == VAL_SUCCESS);
    assert(g_l1_calls == 2U && g_last_destroy == VAL_TRUE);

    native_reset(&context);
    abi.id = VAL_FIRME_ABI_ID_GM_GPI_SET;
    test_case.mutation = VAL_FIRME_MUTATION_GM_ZERO_COUNT;
    assert(gpt_adapter_prepare(&context) == VAL_SUCCESS);
    assert(gpt_adapter_build(&context) == VAL_SUCCESS);
    assert(gpt_adapter_mutate(&context) == VAL_SUCCESS && context.input.registers[2] == 0ULL);
    context.call_issued = VAL_TRUE;
    context.output.registers[0] = (uint64_t)FIRME_INVALID_PARAMETERS;
    context.output.registers[1] = 0ULL;
    assert(gpt_adapter_validate(&context) == VAL_TRUE);
    assert(gpt_adapter_cleanup(&context) == VAL_SUCCESS);
    assert(!g_transition_completions && g_releases == 1U);

    native_reset(&context);
    test_case.mutation = VAL_FIRME_MUTATION_GM_UNALIGNED_BASE;
    g_owned_count = 1ULL;
    assert(gpt_adapter_prepare(&context) == VAL_FIRME_ABI_SCENARIO_UNAVAILABLE);
    assert(!context.call_issued);
    assert(gpt_adapter_cleanup(&context) == VAL_SUCCESS && g_releases == 1U);

    native_reset(&context);
    assert(gpt_adapter_prepare(&context) == VAL_SUCCESS);
    assert(gpt_adapter_build(&context) == VAL_SUCCESS);
    assert(gpt_adapter_mutate(&context) == VAL_SUCCESS);
    assert(context.input.registers[1] == NATIVE_BASE + 1ULL &&
           context.input.registers[2] == 1ULL);
    context.call_issued = VAL_TRUE;
    context.output.registers[1] = UINT64_MAX;
    assert(gpt_adapter_validate(&context) == VAL_TRUE);
    assert(gpt_adapter_cleanup(&context) == VAL_SUCCESS && g_releases == 1U);

    native_reset(&context);
    assert(gpt_adapter_prepare(&context) == VAL_SUCCESS);
    assert(gpt_adapter_build(&context) == VAL_SUCCESS);
    context.call_issued = VAL_TRUE;
    context.output.registers[1] = 0ULL;
    g_observed_gpi = FIRME_GPI_REALM;
    assert(gpt_adapter_validate(&context) == VAL_FALSE);
    assert(gpt_adapter_cleanup(&context) == VAL_ERROR && !g_releases);
    assert(!g_gpt_adapter_fixture[context.world].abi_context.invoke_frame &&
           !g_gpt_adapter_fixture[context.world].abi_context.invoke_cookie);

    native_reset(&context);
    assert(gpt_adapter_prepare(&context) == VAL_SUCCESS);
    assert(gpt_adapter_build(&context) == VAL_SUCCESS);
    context.call_issued = VAL_TRUE;
    context.output.registers[0] = FIRME_SUCCESS;
    assert(gpt_adapter_validate(&context) == VAL_FALSE);
    assert(gpt_adapter_cleanup(&context) == VAL_ERROR && !g_releases);

    native_reset(&context);
    test_case.mutation = VAL_FIRME_MUTATION_NONE;
    assert(gpt_adapter_prepare(&context) == VAL_SUCCESS);
    assert(gpt_adapter_build(&context) == VAL_SUCCESS);
    assert(g_gpt_adapter_fixture[context.world].abi_context.invoke_frame == native_invoke &&
           g_gpt_adapter_fixture[context.world].abi_context.invoke_cookie == &abi);
    context.call_issued = VAL_TRUE;
    context.transport_uncertain = VAL_TRUE;
    assert(gpt_adapter_cleanup(&context) == VAL_ERROR && !g_releases);
    assert(g_gpt_adapter_fixture[context.world].poisoned);
    assert(!g_gpt_adapter_fixture[context.world].abi_context.invoke_frame &&
           !g_gpt_adapter_fixture[context.world].abi_context.invoke_cookie);

    native_reset(&context);
    assert(gpt_adapter_prepare(&context) == VAL_SUCCESS);
    context.call_issued = VAL_TRUE;
    assert(gpt_adapter_cleanup(&context) == VAL_ERROR && !g_releases);
    assert(g_gpt_adapter_fixture[context.world].poisoned);
    assert(!g_gpt_adapter_fixture[context.world].abi_context.invoke_frame &&
           !g_gpt_adapter_fixture[context.world].abi_context.invoke_cookie);
    puts("Generated GPT ABI validation: PASS");
    return VAL_SUCCESS;
}
