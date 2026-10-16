/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "../../../val/host/val_host_dispatcher.c"
#include "../../../val/src/val_firme_transport.c"
#include "../../../val/realm/val_realm_rmi_dispatch.c"

static uint32_t g_pe_index;
static uint32_t g_unavailable_worlds;
static uint32_t g_bad_ping_worlds;
static uint32_t g_corrupt_word = UINT32_MAX;
static uint32_t g_entry_calls;
static uint32_t g_post_calls;
static uint32_t g_worker_healthy = VAL_TRUE;
static int g_entry_return = VAL_SUCCESS;
static int g_post_return = VAL_SUCCESS;
static firme_rule_res_t g_results[3];
static firme_rule_res_t g_rule_result;
static firme_test_case_t g_cases[5];
static uint32_t g_case_calls[5];
static uint64_t g_first_status;
static char g_log[16384];
static size_t g_log_length;
static uint32_t g_capture_log;
static uint32_t g_observed_calls[3];
static uint32_t g_observed_transport_error;

/** @brief Capture dispatcher evidence without printing expected fixture failures. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *format, ...)
{
    va_list arguments;
    int written;

    if (!g_capture_log)
        return 0U;

    va_start(arguments, format);
    written = vsnprintf(g_log + g_log_length, sizeof(g_log) - g_log_length,
                        format, arguments);
    va_end(arguments);
    assert(written >= 0 && (size_t)written < sizeof(g_log) - g_log_length);
    g_log_length += (size_t)written;
    return 0U;
}

/** @brief Supply a fixed registry with dependent and independent tests interleaved. */
const firme_test_case_t *val_firme_suite_cases(uint32_t *count)
{
    *count = sizeof(g_cases) / sizeof(g_cases[0]);
    return g_cases;
}

/** @brief Use a stable chapter label in captured native logs. */
const char *val_firme_chapter_str(firme_chapter_id_t chapter)
{
    return "native";
}

/** @brief Return an opaque scratch address that native tests must never dereference. */
uint64_t val_firme_gpt_base_for_world(firme_acs_world_t world)
{
    return PLAT_FIRME_GPT_BASE_NS;
}

/** @brief Supply the dispatcher initialization response. */
void val_firme_version(firme_res_t *out)
{
    *out = (firme_res_t){.x0 = UINT64_C(0x10000)};
}

/** @brief Supply the dispatcher initialization feature response. */
void val_firme_features(uint64_t feature_id, firme_res_t *out)
{
    *out = (firme_res_t){.x0 = FIRME_SUCCESS};
}

/** @brief Record dispatches and inject the first test's cleanup outcome. */
int val_firme_rule_run_ns(firme_rule_id_t rule, uint64_t base_pa, firme_rule_res_t *out)
{
    uint32_t case_index;

    for (case_index = 0U; case_index < sizeof(g_cases) / sizeof(g_cases[0]); case_index++) {
        if (g_cases[case_index].rule_id != rule)
            continue;

        g_case_calls[case_index]++;
        *out = (firme_rule_res_t){.status = ACS_STATUS_PASS};
        if (case_index == 0U) {
            out->status = g_first_status;
            out->fail_step = UINT64_C(3);
            out->last_x0 = (uint64_t)FIRME_DENIED;
        }

        return is_pass_or_skip(out->status) ? VAL_SUCCESS : VAL_ERROR;
    }

    assert(0 && "unexpected native rule dispatch");
    return VAL_ERROR;
}

/** @brief Select the GPT rule used to exercise dispatcher isolation. */
uint64_t val_firme_gpt_observed_rule_granules(firme_rule_id_t rule)
{
    return rule == FIRME_RULE_R0127 ? UINT64_C(1) : 0ULL;
}

/** @brief Inject observer outcomes without dereferencing native fixture addresses. */
int val_firme_gpt_run_observed(firme_rule_id_t rule, firme_acs_world_t world,
    uint64_t base_pa, firme_acs_call_res_t *response)
{
    firme_rule_res_t result = {.status = ACS_STATUS_ERR};

    assert(rule == FIRME_RULE_R0127);
    assert(world <= FIRME_ACS_WORLD_REALM && response != NULL);
    g_observed_calls[world]++;
    *response = (firme_acs_call_res_t){.x0 = ACS_STATUS_ERR};
    if (g_observed_transport_error)
        return VAL_ERROR;

    if (world != FIRME_ACS_WORLD_NSEL2)
        return val_firme_world_call(world, FIRME_ACS_CMD_RUN_RULE,
            rule, base_pa, 0ULL, 0ULL, response);

    val_firme_rule_run_ns(rule, base_pa, &result);
    *response = (firme_acs_call_res_t){.x0 = result.status, .x1 = result.fail_step,
        .x2 = result.last_x0, .x3 = result.last_x1};
    return VAL_SUCCESS;
}

/** @brief Mark the NS body present; dispatch uses val_firme_rule_run_ns instead. */
static int test_ns_body(uint64_t base_pa, firme_rule_res_t *out)
{
    assert(0 && "unexpected direct native body invocation");
    return VAL_ERROR;
}

/** @brief Identify the emulated physical PE. */
uint32_t val_pe_index(void)
{
    return g_pe_index;
}

/** @brief Expose worker quarantine independently of the entry's result. */
int val_host_test_payload_healthy(void)
{
    return g_worker_healthy;
}

/** @brief Provide the runner address passed to the test-owned MP entry. */
int val_host_run_test_payload(uint32_t num_pe, const val_test_payload_t *payload,
                              val_test_pe_result_t *results)
{
    return VAL_SUCCESS;
}

/** @brief Supply a full-width result to the production Realm cache. */
int val_firme_rule_run_realm(firme_rule_id_t rule, uint64_t base_pa,
                             firme_rule_res_t *out)
{
    *out = g_rule_result;
    return is_pass_or_skip(out->status) ? VAL_SUCCESS : VAL_ERROR;
}

/** @brief Reject unrelated diagnostic commands in this fixture. */
int val_firme_diag_run(uint64_t test_id, uint64_t arg0, uint64_t arg1,
                        uint64_t arg2, uint64_t arg3, firme_diag_out_t *out)
{
    return ACS_STATUS_ERR;
}

/** @brief Reject unrelated IDE commands in this fixture. */
void val_firme_ide_link_command(uint64_t operation, uint64_t keyset_id,
                                uint64_t data0, uint64_t data1,
                                firme_acs_call_res_t *out)
{
    out->x0 = ACS_STATUS_ERR;
}

/** @brief Reject unrelated full-frame commands; dedicated tests cover that transport. */
void val_firme_abi_frame_command(firme_acs_world_t world, uint64_t control, uint64_t sequence,
                                uint64_t low, uint64_t high,
                                firme_acs_call_res_t *response)
{
    (void)world;
    (void)control;
    (void)sequence;
    (void)low;
    (void)high;
    *response = (firme_acs_call_res_t){.x0 = ACS_STATUS_ERR};
}

/** @brief Reject memory probes outside this dispatcher-only fixture. */
void val_firme_gpt_probe_command(uint64_t address, firme_acs_call_res_t *response)
{
    *response = (firme_acs_call_res_t){.x0 = ACS_STATUS_ERR};
}

/** @brief Reject unrelated MP payload commands in this fixture. */
void val_firme_test_payload_command(firme_acs_world_t world, firme_acs_cmd_t cmd,
                                     uint64_t rule_id, uint64_t phase,
                                     uint64_t argument, uint64_t pe_index,
                                     firme_acs_call_res_t *response)
{
    response->x0 = ACS_STATUS_ERR;
}

/** @brief Emulate only the documented SPMD and legacy RMMD forwarding window. */
void val_smccc_smc_call(uint64_t fid, uint64_t arg1, uint64_t arg2,
                        uint64_t arg3, uint64_t arg4, uint64_t arg5,
                        uint64_t arg6, uint64_t arg7, smccc_res_t *out)
{
    smc_args_t frame = {
        .x = {fid, arg1, arg2, arg3, arg4, arg5, arg6, arg7}
    };

    *out = (smccc_res_t){.x0 = SMCCC_SMC_UNK};
    if (fid == FIRME_ACS_RMI_DISPATCH) {
        if ((g_unavailable_worlds & FIRME_WORLD_MASK_REALM) != 0U)
            return;

        rel2_handle_smc(&frame);
        out->x0 = frame.x[1];
        out->x1 = frame.x[2];
        out->x2 = frame.x[3];
        out->x3 = frame.x[4];
        out->x4 = frame.x[5];
        out->x5 = UINT64_C(0xbad0000000000005);
        out->x6 = UINT64_C(0xbad0000000000006);
        out->x7 = UINT64_C(0xbad0000000000007);
        if ((arg1 == FIRME_ACS_CMD_PING) &&
            ((g_bad_ping_worlds & FIRME_WORLD_MASK_REALM) != 0U))
            out->x2 = ACS_STATUS_ERR;

        if ((arg1 == FIRME_ACS_CMD_GET_RULE_RESULT) && (arg4 == g_corrupt_word))
            out->x4 = UINT64_C(1) << 32;
    } else if (fid == FFA_MSG_SEND_DIRECT_REQ_SMC32) {
        if ((g_unavailable_worlds & FIRME_WORLD_MASK_SECURE) != 0U)
            return;

        out->x0 = FFA_MSG_SEND_DIRECT_RESP_SMC32;
        out->x3 = ((g_bad_ping_worlds & FIRME_WORLD_MASK_SECURE) != 0U) ?
                  ACS_STATUS_ERR : ACS_STATUS_PASS;
    }
}

/** @brief Return independently controlled per-world MP outcomes. */
static int test_entry(firme_acs_world_t world, uint32_t num_pe,
                       val_test_run_payload_t run_payload, firme_rule_res_t *out)
{
    assert(num_pe == PAL_PE_COUNT);
    assert(run_payload == val_host_run_test_payload);
    g_entry_calls |= UINT32_C(1) << world;
    *out = g_results[world];
    return g_entry_return;
}

/** @brief Satisfy the registry's payload-presence contract. */
static int test_payload(firme_acs_world_t world, uint32_t phase,
                         uint32_t argument, firme_rule_res_t *out)
{
    out->status = ACS_STATUS_PASS;
    return VAL_SUCCESS;
}

/** @brief Check that MP post-run receives every world's untruncated evidence. */
static int test_post(const firme_acs_call_res_t *normal,
                      const firme_acs_call_res_t *secure,
                      const firme_acs_call_res_t *realm,
                      firme_acs_call_res_t *overall)
{
    g_post_calls++;
    assert(normal->x2 == g_results[FIRME_ACS_WORLD_NSEL2].last_x0);
    assert(secure->x3 == g_results[FIRME_ACS_WORLD_SECURE].last_x1);
    assert(realm->x3 == g_results[FIRME_ACS_WORLD_REALM].last_x1);
    return g_post_return;
}

/** @brief Exercise required/optional world enforcement and post-run reduction. */
static void test_mp_contract(void)
{
    firme_test_case_t entry = {
        .num_pe = VAL_TEST_ALL_PES, .mp_entry = test_entry,
        .mp_payload = test_payload, .post_run = test_post,
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
    };
    firme_acs_call_res_t result;
    uint32_t world;

    for (world = 0U; world <= FIRME_ACS_WORLD_REALM; world++) {
        g_results[world] = (firme_rule_res_t){
            .status = ACS_STATUS_PASS, .last_x0 = UINT64_C(0x100000000) + world,
            .last_x1 = UINT64_C(0x200000000) + world,
        };
    }

    assert(val_host_run_mp_case(&entry, &result) == VAL_SUCCESS);
    assert(result.x0 == ACS_STATUS_PASS && g_entry_calls == 7U && g_post_calls == 1U);
    entry.chapter = FIRME_CHAPTER_GPT;
    g_entry_calls = 0U;
    g_results[FIRME_ACS_WORLD_NSEL2].status = ACS_STATUS_ERR;
    assert(val_host_run_mp_case(&entry, &result) == VAL_ERROR);
    assert(result.x0 == ACS_STATUS_ERR && g_entry_calls == FIRME_WORLD_MASK_NS);
    assert(g_post_calls == 1U);
    g_results[FIRME_ACS_WORLD_NSEL2].status = ACS_STATUS_PASS;
    entry.chapter = FIRME_CHAPTER_OTHER;
    g_unavailable_worlds = FIRME_WORLD_MASK_REALM;
    assert(val_host_run_mp_case(&entry, &result) == VAL_ERROR);
    assert(result.x0 == ACS_STATUS_ERR && g_post_calls == 1U);
    entry.required_worlds_mask &= ~FIRME_WORLD_MASK_REALM;
    entry.optional_worlds_mask = FIRME_WORLD_MASK_REALM;
    entry.post_run = NULL;
    assert(val_host_run_mp_case(&entry, &result) == VAL_SUCCESS);
    assert(result.x0 == ACS_STATUS_PASS);
    g_unavailable_worlds = 0U;
    g_bad_ping_worlds = FIRME_WORLD_MASK_REALM;
    assert(val_host_run_mp_case(&entry, &result) == VAL_ERROR);
    g_bad_ping_worlds = 0U;
    entry.required_worlds_mask |= FIRME_WORLD_MASK_REALM;
    entry.optional_worlds_mask = 0U;
    entry.post_run = test_post;
    g_results[FIRME_ACS_WORLD_REALM].status = ACS_STATUS_SKIP;
    assert(val_host_run_mp_case(&entry, &result) == VAL_SUCCESS);
    assert(result.x0 == ACS_STATUS_SKIP && g_post_calls == 1U);
    g_results[FIRME_ACS_WORLD_REALM].status = ACS_STATUS_PASS;
    g_post_return = VAL_ERROR;
    assert(val_host_run_mp_case(&entry, &result) == VAL_ERROR);
    assert(result.x0 == ACS_STATUS_ERR);
    g_post_return = VAL_SUCCESS;
    g_entry_return = VAL_ERROR;
    assert(val_host_run_mp_case(&entry, &result) == VAL_ERROR);
    g_entry_return = VAL_SUCCESS;
    g_worker_healthy = VAL_FALSE;
    assert(val_host_run_mp_case(&entry, &result) == VAL_ERROR);
    g_worker_healthy = VAL_TRUE;
    entry.required_worlds_mask = 0U;
    assert(val_host_run_mp_case(&entry, &result) == VAL_ERROR);
}

/** @brief Preserve all result bits and reject stale, cross-PE or corrupt chunks. */
static void test_realm_result(void)
{
    firme_acs_call_res_t result;
    uint32_t sequence;

    g_rule_result = (firme_rule_res_t){
        .status = ACS_STATUS_FAIL, .fail_step = UINT64_C(0x123456789abcdef0),
        .last_x0 = UINT64_C(0xfffffffffffffffb),
        .last_x1 = UINT64_C(0xfedcba9876543210),
    };
    assert(val_firme_world_call(FIRME_ACS_WORLD_REALM, FIRME_ACS_CMD_RUN_RULE,
                                FIRME_RULE_R0064, 0U, 0U, 0U, &result) == VAL_SUCCESS);
    assert(result.x0 == g_rule_result.status && result.x1 == g_rule_result.fail_step);
    assert(result.x2 == g_rule_result.last_x0 && result.x3 == g_rule_result.last_x1);
    sequence = g_realm_rule_sequence[0];
    val_realm_rule_command(FIRME_ACS_CMD_GET_RULE_RESULT, FIRME_RULE_R0064,
                            sequence - 1U, 3U, 0U, &result);
    assert(result.x0 == ACS_STATUS_ERR);
    val_realm_rule_command(FIRME_ACS_CMD_GET_RULE_RESULT, FIRME_RULE_R0064,
                            sequence, 3U, 1U, &result);
    assert(result.x0 == ACS_STATUS_ERR);
    g_corrupt_word = 3U;
    assert(val_firme_world_call(FIRME_ACS_WORLD_REALM, FIRME_ACS_CMD_RUN_RULE,
                                FIRME_RULE_R0064, 0U, 0U, 0U, &result) == VAL_SUCCESS);
    assert(result.x0 == ACS_STATUS_ERR);
    g_corrupt_word = UINT32_MAX;
    g_pe_index = PAL_PE_COUNT;
    assert(val_firme_world_call(FIRME_ACS_WORLD_REALM, FIRME_ACS_CMD_RUN_RULE,
                                FIRME_RULE_R0064, 0U, 0U, 0U, &result) == VAL_ERROR);
    g_pe_index = 0U;
}

/** @brief Retain the unsafe-world result when subsequent outcomes are combined. */
static void test_error_precedence(void)
{
    const uint64_t statuses[] = {ACS_STATUS_PASS, ACS_STATUS_SKIP, ACS_STATUS_FAIL};
    firme_acs_call_res_t overall;
    firme_acs_call_res_t candidate;
    uint32_t have_result;
    uint32_t status_index;

    for (status_index = 0U; status_index < sizeof(statuses) / sizeof(statuses[0]);
         status_index++) {
        overall = (firme_acs_call_res_t){.x0 = ACS_STATUS_ERR, .x1 = UINT64_C(3)};
        candidate = (firme_acs_call_res_t){.x0 = statuses[status_index]};
        have_result = VAL_TRUE;
        combine_rule_result(&overall, &candidate, &have_result);
        assert(overall.x0 == ACS_STATUS_ERR && overall.x1 == UINT64_C(3));
    }
}

/** @brief Quarantine unsafe GPT storage without hiding failures or stopping other services. */
static void test_dispatcher_quarantine(void)
{
    const firme_chapter_id_t chapters[] = {
        FIRME_CHAPTER_GPT, FIRME_CHAPTER_BASE, FIRME_CHAPTER_GPT,
        FIRME_CHAPTER_MEC, FIRME_CHAPTER_IDE,
    };
    const firme_rule_id_t rules[] = {
        FIRME_RULE_R0127, FIRME_RULE_R0344, FIRME_RULE_R0348,
        FIRME_RULE_R0358, FIRME_RULE_R0360,
    };
    const char *const rule_names[] = {"R0127", "R0344", "R0348", "R0358", "R0360"};
    uint32_t case_index;

    for (case_index = 0U; case_index < sizeof(g_cases) / sizeof(g_cases[0]); case_index++) {
        g_cases[case_index] = (firme_test_case_t){
            .chapter = chapters[case_index], .rule_id = rules[case_index],
            .rule_str = rule_names[case_index], .suite = "native", .test_name = "isolation",
            .required_worlds_mask = FIRME_WORLD_MASK_NS, .run_ns = test_ns_body,
        };
    }

    /* Step 1: An unsafe GPT result blocks later GPT bodies, not other services. */
    g_capture_log = VAL_TRUE;
    g_first_status = ACS_STATUS_ERR;
    memset(g_observed_calls, 0, sizeof(g_observed_calls));
    dispatcher_main();
    assert(g_observed_calls[FIRME_ACS_WORLD_NSEL2] == 1U);
    assert(g_case_calls[0] == 1U && g_case_calls[2] == 0U);
    assert(g_case_calls[1] == 1U && g_case_calls[3] == 1U && g_case_calls[4] == 1U);
    assert(strstr(g_log, "Result: FAIL") != NULL);
    assert(strstr(g_log, "fail_step=3 last_x0=fffffffffffffffb") != NULL);
    assert(strstr(g_log, "Blocked: GPT scratch quarantined after R0127; "
                        "body not executed") != NULL);
    assert(strstr(g_log, "Total Tests run  = 5 ; Tests Passed  = 3 ; Tests Failed = 1") != NULL);
    assert(strstr(g_log, "Tests Skipped    = 1") != NULL);
    assert(strstr(g_log, "FIRME tests complete.") != NULL);

    /* Step 2: A new run with an ordinary mismatch still executes every test. */
    memset(g_case_calls, 0, sizeof(g_case_calls));
    memset(g_observed_calls, 0, sizeof(g_observed_calls));
    g_log_length = 0U;
    g_first_status = ACS_STATUS_FAIL;
    dispatcher_main();
    assert(g_observed_calls[FIRME_ACS_WORLD_NSEL2] == 1U);
    for (case_index = 0U; case_index < sizeof(g_cases) / sizeof(g_cases[0]); case_index++)
        assert(g_case_calls[case_index] == 1U);

    assert(strstr(g_log, "Total Tests run  = 5 ; Tests Passed  = 4 ; Tests Failed = 1") != NULL);
    assert(strstr(g_log, "Blocked:") == NULL);

    /* Step 3: An unknown live worker still stops the run before any more dispatches. */
    memset(g_case_calls, 0, sizeof(g_case_calls));
    g_log_length = 0U;
    g_first_status = ACS_STATUS_ERR;
    g_worker_healthy = VAL_FALSE;
    dispatcher_main();
    assert(g_case_calls[0] == 1U);
    for (case_index = 1U; case_index < sizeof(g_cases) / sizeof(g_cases[0]); case_index++)
        assert(g_case_calls[case_index] == 0U);

    assert(strstr(g_log, "MP cleanup unconfirmed") != NULL);
    assert(strstr(g_log, "FIRME tests complete.") == NULL);
    g_worker_healthy = VAL_TRUE;
    g_capture_log = VAL_FALSE;
}

/** @brief Supply a host-coordinated remote body's architectural result. */
static int test_host_remote_body(uint64_t base_pa, firme_rule_res_t *out)
{
    assert(base_pa == UINT64_C(0x90000000));
    *out = (firme_rule_res_t){.status = ACS_STATUS_PASS, .last_x1 = UINT64_MAX};
    return VAL_SUCCESS;
}

/** @brief Ensure an erroneous coordinator return cannot manufacture a PASS. */
static int test_host_remote_error(uint64_t base_pa, firme_rule_res_t *out)
{
    *out = (firme_rule_res_t){.status = ACS_STATUS_PASS};
    return VAL_ERROR;
}

/** @brief Route only explicitly coordinated worlds to their host-owned fixtures. */
static void test_host_remote_dispatch(void)
{
    firme_test_case_t test_case = {.rule_id = FIRME_RULE_R0127,
        .run_host_secure = test_host_remote_body,
        .run_host_realm = test_host_remote_error};
    firme_acs_call_res_t result = {0};

    /* Step 1: Explicit coordinators take precedence over the generic observer. */
    memset(g_observed_calls, 0, sizeof(g_observed_calls));
    assert(val_host_run_remote_case(&test_case, FIRME_ACS_WORLD_SECURE,
        UINT64_C(0x90000000), &result) == VAL_SUCCESS);
    assert(result.x0 == ACS_STATUS_PASS && result.x3 == UINT64_MAX);
    assert(val_host_run_remote_case(&test_case, FIRME_ACS_WORLD_REALM,
        UINT64_C(0x90000000), &result) == VAL_SUCCESS);
    assert(result.x0 == ACS_STATUS_ERR);
    assert(g_observed_calls[FIRME_ACS_WORLD_SECURE] == 0U);
    assert(g_observed_calls[FIRME_ACS_WORLD_REALM] == 0U);

    /* Step 2: Uncoordinated audited rules retain the observed Realm verdict. */
    test_case.run_host_realm = NULL;
    g_rule_result = (firme_rule_res_t){.status = ACS_STATUS_FAIL,
        .fail_step = UINT64_C(7), .last_x0 = (uint64_t)FIRME_DENIED};
    assert(val_host_run_remote_case(&test_case, FIRME_ACS_WORLD_REALM,
        UINT64_C(0x90000000), &result) == VAL_SUCCESS);
    assert(g_observed_calls[FIRME_ACS_WORLD_REALM] == 1U);
    assert(result.x0 == ACS_STATUS_FAIL && result.x1 == UINT64_C(7));
    assert(result.x2 == (uint64_t)FIRME_DENIED);

    /* Step 3: An observer transport error remains an error, never a PASS. */
    g_observed_transport_error = VAL_TRUE;
    assert(val_host_run_remote_case(&test_case, FIRME_ACS_WORLD_REALM,
        UINT64_C(0x90000000), &result) == VAL_ERROR);
    assert(result.x0 == ACS_STATUS_ERR);
    g_observed_transport_error = VAL_FALSE;
}

/** @brief Run native framework checks; these are not DUT compliance results. */
int main(void)
{
    assert(val_host_gpt_window_reusable(FIRME_CHAPTER_GPT, ACS_STATUS_ERR) == VAL_FALSE);
    assert(val_host_gpt_window_reusable(FIRME_CHAPTER_GPT, ACS_STATUS_FAIL) == VAL_TRUE);
    assert(val_host_gpt_window_reusable(FIRME_CHAPTER_GPT, ACS_STATUS_SKIP) == VAL_TRUE);
    assert(val_host_gpt_window_reusable(FIRME_CHAPTER_GPT, ACS_STATUS_PASS) == VAL_TRUE);
    assert(val_host_gpt_window_reusable(FIRME_CHAPTER_BASE, ACS_STATUS_ERR) == VAL_TRUE);
    test_mp_contract();
    test_realm_result();
    test_error_precedence();
    test_dispatcher_quarantine();
    test_host_remote_dispatch();
    puts("dispatcher and Realm transport native checks: PASS");
    return 0;
}
