/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#define main native_existing_fixture_main
#include "gpt_fixture_test.c"
#undef main

#include "../../../val/src/val_firme_gpt_retained.c"
#include "../../../val/host/val_host_gpt.c"
#include "../gpt_op_continue_start_granule.c"
#include "../gpt_op_continue_granule_count.c"
#include "../gpt_operation_conflict.c"
#include "../gpt_stateful_incomplete.c"

static const firme_rule_id_t native_retained_rules[] = {
    FIRME_RULE_R0114, FIRME_RULE_R0115, FIRME_RULE_R0129, FIRME_RULE_R0130,
};
static firme_acs_world_t native_expected_world;
static uint32_t native_remote_calls;
static uint64_t native_fail_fid;
static uint32_t native_fail_issued;
static uint32_t native_host_index;

/** @brief Native tests install explicit fixture reservations, not a guessed production range. */
pal_firme_gpt_scenario_status_t pal_firme_gpt_memory_layout(
    uint32_t world, pal_firme_gpt_memory_kind_t kind, uint64_t granule_size,
    uint64_t range_size, uint64_t object_size, pal_firme_gpt_memory_t *memory)
{
    assert(VAL_FALSE);
    return PAL_FIRME_GPT_SCENARIO_ERROR;
}

/** @brief Reject bypasses of the explicitly selected native frame transport. */
int val_firme_world_call(firme_acs_world_t world, firme_acs_cmd_t command,
    uint64_t argument0, uint64_t argument1, uint64_t argument2, uint64_t argument3,
    firme_acs_call_res_t *response)
{
    assert(VAL_FALSE);
    return VAL_ERROR;
}

/** @brief Native GPI evidence comes from the separate map, never uncontrolled memory loads. */
void val_firme_gpt_probe_command(uint64_t address, firme_acs_call_res_t *response)
{
    assert(VAL_FALSE);
}

/** @brief Bind the coordinator and its frame transport to the same native PE. */
uint32_t val_pe_index(void)
{
    return native_host_index;
}

/** @brief The production coordinator must reject a secondary PE without issuing an ABI. */
uint32_t val_host_mp_primary_index(void)
{
    return 0U;
}

/** @brief Initialize a selected caller transport without substituting a Host invocation. */
int val_host_abi_frame_init_world(val_host_abi_frame_context_t *context,
    firme_acs_world_t world)
{
    assert(context && world == native_expected_world && world != FIRME_ACS_WORLD_NSEL2);
    *context = (val_host_abi_frame_context_t){
        .world = world, .owner_pe = native_host_index, .initialized = VAL_TRUE,
    };
    return VAL_SUCCESS;
}

/** @brief Model the selected caller, full-width cookies and uncertain execution separately. */
int val_host_abi_frame_invoke(const smccc_call_frame_t *input,
    smccc_call_frame_t *output, uint32_t *issued, void *cookie)
{
    val_host_abi_frame_context_t *transport = cookie;
    val_firme_abi_test_context_t local_context = {0};
    uint32_t index;

    assert(transport && transport->initialized &&
           transport->world == native_expected_world &&
           transport->owner_pe == native_host_index);
    native_remote_calls++;
    for (index = 4U; index < VAL_SMCCC_REGISTER_COUNT; index++)
        assert(input->registers[index] == 0ULL);

    if (native_fail_fid == input->registers[0]) {
        native_fail_fid = 0ULL;
        *issued = native_fail_issued;
        return VAL_ERROR;
    }
    return val_firme_abi_call(&local_context, input, output, issued);
}

/** @brief Reset only the native model between independent process-equivalent cases. */
static void native_retained_reset(firme_acs_world_t world)
{
    native_reset();
    memset(g_retained_fixture, 0, sizeof(g_retained_fixture));
    memset(g_retained_lock, 0, sizeof(g_retained_lock));
    memset(g_host_gpt_retained_transport, 0, sizeof(g_host_gpt_retained_transport));
    native_expected_world = world;
    native_remote_calls = native_host_index = 0U;
    native_fail_fid = 0ULL;
    native_fail_issued = VAL_FALSE;
    native_feature = FIRME_GM_PGS_4KB;
    native_pgs = PLAT_FIRME_GPT_GRANULE_SIZE;
}

/** @brief Exercise real rule bodies through production Host coordination in every world. */
static void native_retained_success(void)
{
    firme_rule_res_t out;
    firme_acs_world_t world;
    size_t rule_index;
    uint32_t continue_index;
    uint64_t next_cookie = UINT64_C(0xa5a5b6b6c7c7d8d8);

    for (world = FIRME_ACS_WORLD_NSEL2; world <= FIRME_ACS_WORLD_REALM; world++) {
        for (rule_index = 0U; rule_index < sizeof(native_retained_rules) /
             sizeof(native_retained_rules[0]); rule_index++) {
            native_retained_reset(world);

            /* Step 1: Partial stateless progress precedes an actual stateful cookie of zero. */
            native_reply(NATIVE_SET, FIRME_SUCCESS, 1ULL, UINT64_MAX);
            native_reply(NATIVE_SET, FIRME_INCOMPLETE, 1ULL, 0ULL);
            if (native_retained_rules[rule_index] == FIRME_RULE_R0129) {
                native_reply(NATIVE_SET, FIRME_BUSY, UINT64_MAX, UINT64_MAX);
                native_reply(NATIVE_SET, FIRME_OP_CONFLICT, 0ULL, UINT64_MAX);
            }
            continue_index = native_queued;
            native_reply(NATIVE_CONTINUE, FIRME_BUSY, UINT64_MAX, next_cookie);
            native_reply(NATIVE_CONTINUE, FIRME_INCOMPLETE, 1ULL, 0ULL);
            native_reply(NATIVE_CONTINUE, FIRME_SUCCESS, 1ULL, UINT64_MAX);
            native_reply(NATIVE_SET, FIRME_SUCCESS, NATIVE_COUNT, UINT64_MAX);

            /* Step 2: Full-width replacement cookies and per-call progress must be preserved. */
            assert(val_host_gpt_retained_run(native_retained_rules[rule_index], world,
                                             &out) == VAL_SUCCESS);
            assert(out.status == ACS_STATUS_PASS);
            assert(native_calls[1].argument == NATIVE_BASE + native_pgs);
            assert(native_calls[1].count == NATIVE_COUNT - 1ULL);
            assert(native_calls[continue_index].argument == 0ULL);
            assert(native_calls[continue_index + 1U].argument == next_cookie);
            assert(native_calls[continue_index + 2U].argument == 0ULL);
            assert(native_issued == native_queued && native_releases == 1U);
            assert(!native_lease_live && !g_retained_lock[world]);
            assert((native_remote_calls != 0U) == (world != FIRME_ACS_WORLD_NSEL2));

            /* Step 3: Normal-world NSO granules must be fully scrubbed before returning to NS. */
            assert(native_scrub_cleans == (world == FIRME_ACS_WORLD_NSEL2 ?
                                           NATIVE_COUNT : 0ULL));
            assert(native_scrub_writes == native_scrub_cleans * native_pgs / sizeof(uint64_t));
        }
    }
}

/** @brief Synchronous DUT completion and missing observations never become retained PASS. */
static void native_retained_unavailable(void)
{
    firme_rule_res_t out;
    size_t rule_index;

    for (rule_index = 0U; rule_index < sizeof(native_retained_rules) /
         sizeof(native_retained_rules[0]); rule_index++) {
        native_retained_reset(FIRME_ACS_WORLD_REALM);
        native_reply(NATIVE_SET, FIRME_SUCCESS, NATIVE_COUNT, UINT64_MAX);
        native_reply(NATIVE_SET, FIRME_SUCCESS, NATIVE_COUNT, UINT64_MAX);
        assert(val_host_gpt_retained_run(native_retained_rules[rule_index],
            FIRME_ACS_WORLD_REALM, &out) == VAL_SUCCESS);
        assert(out.status == ACS_STATUS_SKIP && native_releases == 1U);
        assert(native_issued == native_queued);

        native_retained_reset(FIRME_ACS_WORLD_REALM);
        native_abis &= ~FIRME_GM_FEATURE_REG0_OP_CONTINUE_BIT;
        assert(val_host_gpt_retained_run(native_retained_rules[rule_index],
            FIRME_ACS_WORLD_REALM, &out) == VAL_ERROR);
        assert(out.status == ACS_STATUS_FAIL && !native_acquires && !native_issued);
    }

    native_retained_reset(FIRME_ACS_WORLD_NSEL2);
    native_gpc2 = VAL_FALSE;
    assert(firme_gpt_op_continue_granule_count_run_ns(NATIVE_BASE, &out) == VAL_SUCCESS);
    assert(out.status == ACS_STATUS_SKIP && !native_issued && native_releases == 1U);

    native_retained_reset(FIRME_ACS_WORLD_REALM);
    native_host_index = 1U;
    assert(val_host_gpt_retained_run(FIRME_RULE_R0130, FIRME_ACS_WORLD_REALM,
                                     &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_ERR && !native_acquires && !native_remote_calls);
    assert(val_host_gpt_retained_run(FIRME_RULE_R0130, FIRME_ACS_WORLD_REALM,
                                     NULL) == VAL_ERROR);
}

/** @brief Wrong physical progress, impossible counts and conflicting side effects quarantine. */
static void native_retained_bad_progress(void)
{
    firme_rule_res_t out;
    uint32_t bad_case;
    uint32_t calls_before;
    firme_rule_id_t rule;

    for (bad_case = 0U; bad_case < 3U; bad_case++) {
        native_retained_reset(FIRME_ACS_WORLD_REALM);
        rule = bad_case == 2U ? FIRME_RULE_R0129 : FIRME_RULE_R0115;
        native_reply(NATIVE_SET, FIRME_INCOMPLETE, 1ULL, 0ULL);
        if (bad_case == 2U) {
            native_conflict_corruption = VAL_TRUE;
            native_reply(NATIVE_SET, FIRME_OP_CONFLICT, 0ULL, UINT64_MAX);
        } else {
            native_continue_corruption = bad_case == 1U;
            native_reply(NATIVE_CONTINUE, FIRME_INCOMPLETE,
                         bad_case == 0U ? NATIVE_COUNT : 1ULL, 0ULL);
        }
        assert(val_host_gpt_retained_run(rule, FIRME_ACS_WORLD_REALM, &out) == VAL_ERROR);
        assert(out.status == ACS_STATUS_ERR && native_lease_live && !native_releases);
        assert(g_retained_fixture[FIRME_ACS_WORLD_REALM].poisoned);
        assert(g_retained_lock[FIRME_ACS_WORLD_REALM]);
        calls_before = native_remote_calls;
        assert(val_host_gpt_retained_run(rule, FIRME_ACS_WORLD_REALM, &out) == VAL_ERROR);
        assert(out.status == ACS_STATUS_ERR && native_remote_calls == calls_before);
        assert(native_issued == native_queued);
    }
}

/** @brief Failed continuations stay failed even when known-prefix cleanup succeeds. */
static void native_retained_failure_cleanup(void)
{
    firme_rule_res_t out;
    uint32_t issued;

    native_retained_reset(FIRME_ACS_WORLD_REALM);
    native_reply(NATIVE_SET, FIRME_INCOMPLETE, 1ULL, 0ULL);
    native_reply(NATIVE_CONTINUE, FIRME_DENIED, 0ULL, UINT64_MAX);
    native_reply(NATIVE_SET, FIRME_SUCCESS, 1ULL, UINT64_MAX);
    assert(firme_gpt_stateful_incomplete_run_realm(NATIVE_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_FAIL && out.fail_step == GPT_RETAINED_STEP_CONTINUE);
    assert(native_releases == 1U && native_issued == native_queued);
    assert(native_calls[2].count == 1ULL);

    for (issued = VAL_FALSE; issued <= VAL_TRUE; issued++) {
        native_retained_reset(FIRME_ACS_WORLD_SECURE);
        native_reply(NATIVE_SET, FIRME_INCOMPLETE, 1ULL, 0ULL);
        native_fail_fid = FIRME_FID_GM_GPI_OP_CONTINUE;
        native_fail_issued = issued;
        if (!issued) {
            native_reply(NATIVE_CONTINUE, FIRME_SUCCESS, NATIVE_COUNT - 1U, UINT64_MAX);
            native_reply(NATIVE_SET, FIRME_SUCCESS, NATIVE_COUNT, UINT64_MAX);
        }
        assert(val_host_gpt_retained_run(FIRME_RULE_R0114, FIRME_ACS_WORLD_SECURE,
                                         &out) == VAL_ERROR);
        assert(out.status == (issued ? ACS_STATUS_ERR : ACS_STATUS_FAIL));
        assert(native_releases == (issued ? 0U : 1U));
        assert(native_issued == native_queued);
    }

    native_retained_reset(FIRME_ACS_WORLD_NSEL2);
    native_reply(NATIVE_SET, FIRME_INCOMPLETE, 1ULL, 0ULL);
    native_reply(NATIVE_CONTINUE, FIRME_SUCCESS, NATIVE_COUNT - 1U, UINT64_MAX);
    native_scrub_failure = VAL_TRUE;
    assert(firme_gpt_op_continue_start_granule_run_ns(NATIVE_BASE, &out) == VAL_ERROR);
    assert(out.status == ACS_STATUS_ERR && !native_releases && native_lease_live);
    assert(native_issued == native_queued && !native_scrub_cleans);
}

/** @brief Bounded BUSY retry exhaustion cannot turn retained ownership into SKIP or PASS. */
static void native_retained_busy_exhaustion(void)
{
    firme_rule_res_t out;
    uint32_t attempt;
    uint32_t retained;

    for (retained = VAL_FALSE; retained <= VAL_TRUE; retained++) {
        native_retained_reset(FIRME_ACS_WORLD_REALM);
        if (retained)
            native_reply(NATIVE_SET, FIRME_INCOMPLETE, 1ULL, 0ULL);

        for (attempt = 0U; attempt < (retained ? 2U : 1U) * GPT_FIXTURE_MAX_CALLS;
             attempt++)
            native_reply(retained ? NATIVE_CONTINUE : NATIVE_SET,
                         FIRME_BUSY, UINT64_MAX, 0ULL);

        assert(val_host_gpt_retained_run(FIRME_RULE_R0130, FIRME_ACS_WORLD_REALM,
                                         &out) == VAL_ERROR);
        assert(out.status == (retained ? ACS_STATUS_ERR : ACS_STATUS_FAIL));
        assert(native_releases == (retained ? 0U : 1U));
        assert(native_lease_live == retained);
        assert(native_issued == native_queued);
    }
}

/** @brief Validate production retained rules with independent native state, not DUT evidence. */
int main(void)
{
    assert(pal_firme_gpt_memory_register(&native_memory_ops) == PAL_FIRME_GPT_SCENARIO_READY);
    native_retained_success();
    native_retained_unavailable();
    native_retained_bad_progress();
    native_retained_failure_cleanup();
    native_retained_busy_exhaustion();
    puts("GPT handwritten retained-operation and Host coordinator regressions: PASS");
    return VAL_SUCCESS;
}
