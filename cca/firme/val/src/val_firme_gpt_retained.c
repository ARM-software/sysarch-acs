/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt_retained.h"

#include "val_firme_gpt_fixture.h"
#include "val_log.h"

#define GPT_RETAINED_WORLD_COUNT 3U
#define GPT_RETAINED_STEP_PREPARE UINT64_C(1)
#define GPT_RETAINED_STEP_RETAIN UINT64_C(2)
#define GPT_RETAINED_STEP_CONFLICT UINT64_C(3)
#define GPT_RETAINED_STEP_CONTINUE UINT64_C(4)
#define GPT_RETAINED_STEP_CLEANUP UINT64_C(5)

static val_firme_gpt_fixture_t g_retained_fixture[GPT_RETAINED_WORLD_COUNT];
static uint32_t g_retained_lock[GPT_RETAINED_WORLD_COUNT];

/** @brief Record the first failure without losing retained-operation diagnostics. */
static void val_firme_gpt_retained_fail(firme_rule_id_t rule,
    const val_firme_gpt_fixture_t *fixture, firme_rule_res_t *out, uint64_t step)
{
    LOG_ERROR(NULL, "gpt retained: R%04u step=%llu status=%lld completed=%llu "
              "pending=%u cookie=%llx poisoned=%u\n", (unsigned int)rule,
              (unsigned long long)step, (long long)fixture->last_status,
              (unsigned long long)fixture->completed, fixture->pending,
              (unsigned long long)fixture->cookie, fixture->poisoned);
    if (!out->fail_step)
        val_firme_gpt_rule_result_fail(out, step,
            (uint64_t)fixture->last_status, fixture->completed);
}

/** @brief Keep setup, real continuation, observation and cleanup in one owned session. */
int val_firme_gpt_retained_execute(firme_rule_id_t rule,
    val_firme_abi_test_context_t *context, firme_rule_res_t *out)
{
    val_firme_gpt_fixture_t *fixture;
    pal_firme_gpt_scenario_status_t status;
    uint32_t unlocked = VAL_FALSE;
    uint32_t world;

    if (!out)
        return VAL_ERROR;

    val_firme_gpt_rule_result_init(out);
    if (!context || (uint32_t)context->world >= GPT_RETAINED_WORLD_COUNT ||
        (rule != FIRME_RULE_R0114 && rule != FIRME_RULE_R0115 &&
         rule != FIRME_RULE_R0129 && rule != FIRME_RULE_R0130))
        return VAL_ERROR;

    world = (uint32_t)context->world;
    fixture = &g_retained_fixture[world];
    if (!__atomic_compare_exchange_n(&g_retained_lock[world], &unlocked, VAL_TRUE,
                                    0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        val_firme_gpt_retained_fail(rule, fixture, out, GPT_RETAINED_STEP_PREPARE);
        out->status = ACS_STATUS_ERR;
        return VAL_ERROR;
    }

    /* Step 1: Require real ABI support and independently checked, exclusive NS storage. */
    status = val_firme_gpt_fixture_begin_abi(fixture, context,
                                            PAL_FIRME_GPT_MEMORY_TRANSITION);
    if (status != PAL_FIRME_GPT_SCENARIO_READY) {
        if (status == PAL_FIRME_GPT_SCENARIO_UNAVAILABLE) {
            LOG_WARN(NULL, "gpt retained: R%04u world=%u owned observer unavailable\n",
                     (unsigned int)rule, world);
            out->status = ACS_STATUS_SKIP;
        } else {
            val_firme_gpt_retained_fail(rule, fixture, out, GPT_RETAINED_STEP_PREPARE);
        }
        goto cleanup;
    }

    /* Step 2: Use a changing GPI, never an unobservable NS-to-NS progress assertion. */
    if (context->world == FIRME_ACS_WORLD_NSEL2) {
        if (!val_firme_feat_rme_gpc2_present()) {
            LOG_WARN(NULL, "gpt retained: R%04u NS progress needs FEAT_RME_GPC2\n",
                     (unsigned int)rule);
            out->status = ACS_STATUS_SKIP;
            goto cleanup;
        }
        fixture->target_gpi = FIRME_GPI_NSO;
    }

    /* Step 3: Seek a real cookie; synchronous completion does not cover retained work. */
    status = val_firme_gpt_fixture_retain(fixture);
    if (status == PAL_FIRME_GPT_SCENARIO_UNAVAILABLE) {
        LOG_WARN(NULL, "gpt retained: R%04u world=%u DUT completed synchronously; "
                 "no INCOMPLETE operation observed\n", (unsigned int)rule, world);
        out->status = ACS_STATUS_SKIP;
        goto cleanup;
    }
    if (status != PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_retained_fail(rule, fixture, out, GPT_RETAINED_STEP_RETAIN);
        goto cleanup;
    }

    /* Step 4: R0129 probes the unfinished granule without consuming the original cookie. */
    if (rule == FIRME_RULE_R0129 &&
        val_firme_gpt_fixture_conflict(fixture) != VAL_SUCCESS) {
        val_firme_gpt_retained_fail(rule, fixture, out, GPT_RETAINED_STEP_CONFLICT);
        goto cleanup;
    }

    /* Step 5: Every CONTINUE proves its changed prefix and untouched suffix independently. */
    if (val_firme_gpt_fixture_complete_transition(fixture) != VAL_SUCCESS) {
        val_firme_gpt_retained_fail(rule, fixture, out, GPT_RETAINED_STEP_CONTINUE);
        goto cleanup;
    }
    out->status = ACS_STATUS_PASS;

cleanup:
    /* Step 6: Drain, scrub NSO before release, restore NS and retain every uncertain lease. */
    if (val_firme_gpt_fixture_cleanup(fixture) != VAL_SUCCESS) {
        val_firme_gpt_retained_fail(rule, fixture, out, GPT_RETAINED_STEP_CLEANUP);
        out->status = ACS_STATUS_ERR;
        return VAL_ERROR;
    }
    __atomic_store_n(&g_retained_lock[world], VAL_FALSE, __ATOMIC_RELEASE);
    return val_firme_gpt_rule_result_status(out);
}

/** @brief Execute locally only where an independent memory provider is installed. */
int val_firme_gpt_retained_run(firme_rule_id_t rule, firme_acs_world_t world,
    firme_rule_res_t *out)
{
    val_firme_abi_test_context_t context = {.world = world};

    return val_firme_gpt_retained_execute(rule, &context, out);
}
