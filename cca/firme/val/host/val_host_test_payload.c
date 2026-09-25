/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_mp.h"
#include "val_log.h"

typedef struct {
    val_test_payload_t payload;
    firme_rule_res_t result;
} val_host_test_job_t;

static val_host_test_job_t g_test_job;
static uint32_t g_test_payload_busy;
static uint32_t g_test_payload_failed;

/** @brief Report whether test payload storage is safe to reuse after cleanup. */
int val_host_test_payload_healthy(void)
{
    return g_test_payload_failed == VAL_FALSE;
}

/** @brief Route a phase or result query without exposing cross-world pointers. */
static int val_host_test_command(uint32_t pe_index, const val_test_payload_t *payload,
                                 firme_acs_cmd_t cmd, uint32_t argument,
                                 firme_acs_call_res_t *response)
{
    if (payload->world == FIRME_ACS_WORLD_NSEL2) {
        val_firme_test_payload_command(payload->world, cmd, payload->rule_id,
                                        payload->phase, argument, pe_index, response);
        return VAL_SUCCESS;
    }

    return val_firme_world_call(payload->world, cmd, payload->rule_id,
                                payload->phase, argument, pe_index, response);
}

/** @brief Execute a phase and retrieve every result bit before powering down. */
static int val_host_test_worker(uint32_t pe_index, void *context)
{
    val_host_test_job_t *job = context;
    firme_acs_call_res_t response = {0};
    uint64_t words[VAL_TEST_RESULT_WORDS] = {0};
    uint64_t status = ACS_STATUS_ERR;
    uint32_t word_index = 0U;

    job->result = (firme_rule_res_t){.status = ACS_STATUS_ERR};
    if (val_host_test_command(pe_index, &job->payload, FIRME_ACS_CMD_RUN_TEST_PAYLOAD,
                              job->payload.argument, &response) != VAL_SUCCESS)
        return VAL_ERROR;

    if ((response.x1 != pe_index) || (response.x2 != job->payload.phase))
        return VAL_ERROR;

    status = response.x0;
    for (word_index = 0U; word_index < VAL_TEST_RESULT_WORDS; word_index++) {
        if ((val_host_test_command(pe_index, &job->payload,
                                    FIRME_ACS_CMD_GET_TEST_RESULT, word_index,
                                    &response) != VAL_SUCCESS) ||
            (response.x0 != status) || (response.x1 > UINT32_MAX) ||
            (response.x2 > UINT32_MAX))
            return VAL_ERROR;

        words[word_index] = response.x1 | (response.x2 << 32);
    }

    if ((words[0] != status) ||
        ((status != ACS_STATUS_PASS) && (status != ACS_STATUS_SKIP) &&
         (status != ACS_STATUS_FAIL) && (status != ACS_STATUS_ERR)))
        return VAL_ERROR;

    job->result.status = words[0];
    job->result.fail_step = words[1];
    job->result.last_x0 = words[2];
    job->result.last_x1 = words[3];
    return ((status == ACS_STATUS_PASS) || (status == ACS_STATUS_SKIP)) ?
           VAL_SUCCESS : VAL_ERROR;
}

/**
 * @brief Run a test-selected phase on num_pe PEs, including the controller.
 * @param num_pe Exact PE count, between one and the platform's available count.
 * @param payload Registered test, world, phase and argument to execute.
 * @param results Array of at least num_pe records, in execution order.
 * @return VAL_SUCCESS for completed PASS/SKIP payloads, VAL_ERROR otherwise.
 * Payloads run sequentially; tests needing simultaneous execution use the
 * lower-level MP worker API and explicitly synchronized, private contexts.
 */
int val_host_run_test_payload(uint32_t num_pe, const val_test_payload_t *payload,
                              val_test_pe_result_t *results)
{
    uint32_t primary_pe = val_pe_index();
    uint32_t pe_index = 0U;
    uint32_t slot = 0U;
    uint32_t next_pe = 0U;
    int result = VAL_SUCCESS;
    int callback_result = VAL_ERROR;
    int wait_result = VAL_ERROR;

    if ((primary_pe >= PAL_PE_COUNT) ||
        (primary_pe != val_host_mp_primary_index()) || (num_pe == 0U) ||
        (num_pe > PAL_PE_COUNT) || (payload == NULL) || (results == NULL) ||
        ((uint32_t)payload->world > FIRME_ACS_WORLD_REALM) || g_test_payload_busy ||
        g_test_payload_failed)
        return VAL_ERROR;

    g_test_payload_busy = VAL_TRUE;
    for (slot = 0U; slot < num_pe; slot++) {
        results[slot].pe_index = PAL_PE_COUNT;
        results[slot].result = (firme_rule_res_t){.status = ACS_STATUS_ERR};
    }

    for (slot = 0U; slot < num_pe; slot++) {
        pe_index = primary_pe;
        if (slot != 0U) {
            if (next_pe == primary_pe)
                next_pe++;

            pe_index = next_pe++;
        }

        results[slot].pe_index = pe_index;
        g_test_job.payload = *payload;
        g_test_job.result = (firme_rule_res_t){.status = ACS_STATUS_ERR};
        if (pe_index == primary_pe) {
            callback_result = val_host_test_worker(pe_index, &g_test_job);
        } else {
            if (val_host_mp_start(pe_index) != VAL_SUCCESS) {
                g_test_payload_failed = VAL_TRUE;
                result = VAL_ERROR;
                break;
            }

            wait_result = val_host_mp_dispatch(pe_index, val_host_test_worker, &g_test_job);
            callback_result = VAL_ERROR;
            if (wait_result == VAL_SUCCESS)
                wait_result = val_host_mp_wait(pe_index, &callback_result);

            if (val_host_mp_stop(pe_index) != VAL_SUCCESS) {
                g_test_payload_failed = VAL_TRUE;
                result = VAL_ERROR;
                break;
            }

            if (wait_result != VAL_SUCCESS)
                result = VAL_ERROR;
        }

        results[slot].result = g_test_job.result;
        if (callback_result != VAL_SUCCESS)
            result = VAL_ERROR;

        LOG_TEST(NULL, "MP payload: PE=%u phase=%u status=%llx step=%llu x0=%llx x1=%llx",
                 pe_index, payload->phase, (unsigned long long)g_test_job.result.status,
                 (unsigned long long)g_test_job.result.fail_step,
                 (unsigned long long)g_test_job.result.last_x0,
                 (unsigned long long)g_test_job.result.last_x1);
    }

    g_test_payload_busy = VAL_FALSE;
    return result;
}
