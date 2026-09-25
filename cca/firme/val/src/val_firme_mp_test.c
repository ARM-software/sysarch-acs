/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_mp_test.h"
#include "val_firme_suite.h"
#include "val_common/val_mp.h"

typedef struct {
    firme_rule_res_t result;
    uint32_t rule_id;
    uint32_t phase;
    uint32_t valid;
} val_test_payload_record_t;

static val_test_payload_record_t g_test_payload_records[PAL_PE_COUNT];

/**
 * @brief Execute a registered per-PE payload or read its full-width result.
 * @param world Calling security state.
 * @param cmd Execute or result-read command.
 * @param rule_id Registered rule whose payload/result is requested.
 * @param phase Test-defined phase; result reads must match the executed phase.
 * @param argument Payload argument, or result word index for a result read.
 * @param pe_index Expected physical PE's index in the PAL affinity table.
 * @param response Status and two 32-bit halves, safe on both world transports.
 */
void val_firme_test_payload_command(firme_acs_world_t world, firme_acs_cmd_t cmd,
                                     uint64_t rule_id, uint64_t phase,
                                     uint64_t argument, uint64_t pe_index,
                                     firme_acs_call_res_t *response)
{
    const firme_test_case_t *test_case = NULL;
    val_test_payload_record_t *record = NULL;
    uint32_t current_pe = val_pe_index();
    uint32_t world_mask = 0U;
    uint64_t value = 0U;
    int payload_status = VAL_ERROR;

    if (response == NULL)
        return;

    *response = (firme_acs_call_res_t){.x0 = ACS_STATUS_ERR};
    if ((current_pe >= PAL_PE_COUNT) || (pe_index != current_pe) ||
        (rule_id > UINT32_MAX) || (phase > UINT32_MAX) ||
        (argument > UINT32_MAX) || ((uint32_t)world > FIRME_ACS_WORLD_REALM))
        return;

    test_case = val_firme_suite_find((firme_rule_id_t)rule_id);
    world_mask = UINT32_C(1) << world;
    if ((test_case == NULL) || (test_case->mp_payload == NULL) ||
        (((test_case->required_worlds_mask | test_case->optional_worlds_mask) &
          world_mask) == 0U))
        return;

    record = &g_test_payload_records[current_pe];
    if (cmd == FIRME_ACS_CMD_RUN_TEST_PAYLOAD) {
        record->valid = VAL_FALSE;
        record->rule_id = (uint32_t)rule_id;
        record->phase = (uint32_t)phase;
        record->result = (firme_rule_res_t){.status = ACS_STATUS_ERR};
        payload_status = test_case->mp_payload(world, (uint32_t)phase,
                                               (uint32_t)argument, &record->result);
        if ((payload_status != VAL_SUCCESS) &&
            ((record->result.status == ACS_STATUS_PASS) ||
             (record->result.status == ACS_STATUS_SKIP)))
            record->result.status = ACS_STATUS_ERR;

        if ((record->result.status != ACS_STATUS_PASS) &&
            (record->result.status != ACS_STATUS_SKIP) &&
            (record->result.status != ACS_STATUS_FAIL) &&
            (record->result.status != ACS_STATUS_ERR))
            record->result.status = ACS_STATUS_ERR;

        record->valid = VAL_TRUE;
        response->x0 = record->result.status;
        response->x1 = current_pe;
        response->x2 = phase;
        return;
    }

    if ((cmd != FIRME_ACS_CMD_GET_TEST_RESULT) || !record->valid ||
        (record->rule_id != rule_id) || (record->phase != phase))
        return;

    switch (argument) {
    case 0U:
        value = record->result.status;
        break;
    case 1U:
        value = record->result.fail_step;
        break;
    case 2U:
        value = record->result.last_x0;
        break;
    case 3U:
        value = record->result.last_x1;
        break;
    default:
        return;
    }

    response->x0 = record->result.status;
    response->x1 = (uint32_t)value;
    response->x2 = (uint32_t)(value >> 32);
}
