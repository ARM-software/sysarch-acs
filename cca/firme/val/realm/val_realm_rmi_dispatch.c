/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_diag.h"
#include "val_firme_rules.h"
#include "val_firme_dispatch.h"
#include "val_firme_rmmd.h"
#include "val_common/val_smc_args.h"
#include "val_common/val_fault.h"
#include "val_firme_test.h"
#include "val_firme_ide_link.h"
#include "val_firme_mp_test.h"
#include "val_common/val_mp.h"

#include <stddef.h>

#define RMI_SUCCESS UINT64_C(0)

typedef struct {
    firme_rule_res_t result;
    uint32_t rule_id;
    uint32_t sequence;
    uint32_t valid;
} val_realm_rule_record_t;

static val_realm_rule_record_t g_realm_rule_records[PAL_PE_COUNT];

/**
 * @brief Retain ordinary rule diagnostics and return each full-width word safely.
 * @param cmd Execute or retrieve the current PE's result.
 * @param rule_id Registered rule identifier.
 * @param base_or_sequence Base address for execution, sequence for retrieval.
 * @param sequence_or_word Execution sequence, or result word index.
 * @param expected_pe Physical PE index expected by the host.
 * @param response ACS status and payload, restricted to RMMD-forwarded words.
 */
static void val_realm_rule_command(uint64_t cmd, uint64_t rule_id,
                                    uint64_t base_or_sequence,
                                    uint64_t sequence_or_word, uint64_t expected_pe,
                                    firme_acs_call_res_t *response)
{
    uint32_t pe_index = val_pe_index();
    val_realm_rule_record_t *record;
    uint64_t value;

    if (response == NULL)
        return;

    *response = (firme_acs_call_res_t){.x0 = ACS_STATUS_ERR};
    if ((pe_index >= PAL_PE_COUNT) || (expected_pe != pe_index) ||
        (rule_id > UINT32_MAX))
        return;

    record = &g_realm_rule_records[pe_index];
    if (cmd == FIRME_ACS_CMD_RUN_RULE) {
        record->valid = VAL_FALSE;
        if ((sequence_or_word == 0U) || (sequence_or_word > UINT32_MAX))
            return;

        record->rule_id = (uint32_t)rule_id;
        record->sequence = (uint32_t)sequence_or_word;
        record->result = (firme_rule_res_t){.status = ACS_STATUS_ERR};
        val_firme_rule_run_realm((firme_rule_id_t)rule_id, base_or_sequence,
                                 &record->result);
        record->valid = VAL_TRUE;
        response->x0 = record->result.status;
        response->x1 = record->sequence;
        response->x2 = rule_id;
        return;
    }

    if ((cmd != FIRME_ACS_CMD_GET_RULE_RESULT) || !record->valid ||
        (record->rule_id != rule_id) || (record->sequence != base_or_sequence))
        return;

    switch (sequence_or_word) {
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

/**
 *   @brief    - Complete an ACS RMI dispatch request and return to RMMD.
 *   @param    - args   : In/out SMC argument frame.
 *   @param    - status : ACS command completion status.
 *   @param    - out0   : Output word 0.
 *   @param    - out1   : Output word 1.
 *   @return   - void
**/
static void rmi_complete(smc_args_t *args,
                         uint64_t status,
                         uint64_t out0, uint64_t out1)
{
    /*
     * RMM returns to RMMD using RMM_RMI_REQ_COMPLETE. In TF-A RMMD V1
     * compatibility mode, only Realm x1-x5 are forwarded back to the original
     * NS caller. Keep x1/x2 as RMI transport-success words and carry ACS-owned
     * data only in x3-x5: status plus two payload words.
     */
    if (args == NULL) {
        return;
    }

    args->x[0] = (uint64_t)RMM_RMI_REQ_COMPLETE;
    args->x[1] = RMI_SUCCESS;
    args->x[2] = RMI_SUCCESS;
    args->x[3] = status;
    args->x[4] = out0;
    args->x[5] = out1;
    args->x[6] = 0;
    args->x[7] = 0;
}

/**
 *   @brief    - Realm payload SMC entrypoint called by TF-A RMMD.
 *   @param    - args : In/out SMC argument frame (x0..x7).
 *   @return   - void
**/
void rel2_handle_smc(smc_args_t *args)
{
    uint64_t fid = 0;
    uint64_t cmd = 0;
    uint64_t a0 = 0;
    uint64_t a1 = 0;
    uint64_t a2 = 0;
    uint64_t a3 = 0;
    uint64_t status = (uint64_t)ACS_STATUS_PASS;
    uint64_t out0 = 0;
    uint64_t out1 = 0;
    firme_acs_call_res_t link_response = {0};
    firme_acs_call_res_t mp_response = {0};

    if (args == NULL) {
        return;
    }

    fid = args->x[0];

    if ((uint32_t)fid != (uint32_t)FIRME_ACS_RMI_DISPATCH) {
        rmi_complete(args, (uint64_t)ACS_STATUS_ERR, fid, 0);
        return;
    }

    cmd = args->x[1];
    a0 = args->x[2];
    a1 = args->x[3];
    a2 = args->x[4];
    a3 = args->x[5];

    if (cmd == (uint64_t)FIRME_ACS_CMD_PING) {
        out0 = 0x504f4e47ULL; /* 'PONG' */
    } else if (cmd == (uint64_t)FIRME_ACS_CMD_RUN_DIAG) {
        firme_diag_out_t out = { 0 };

        status = (uint64_t)val_firme_diag_run(a0, a1, a2, a3, 0, &out);
        out0 = out.x0;
        out1 = out.x1;
    } else if (cmd == FIRME_ACS_CMD_IDE_LINK) {
        val_firme_ide_link_command(a0, a1, a2, a3, &link_response);
        status = link_response.x0;
        out0 = link_response.x1;
        out1 = link_response.x2;
    } else if ((cmd == FIRME_ACS_CMD_RUN_TEST_PAYLOAD) ||
               (cmd == FIRME_ACS_CMD_GET_TEST_RESULT)) {
        val_firme_test_payload_command(FIRME_ACS_WORLD_REALM, (firme_acs_cmd_t)cmd,
                                        a0, a1, a2, a3, &mp_response);
        status = mp_response.x0;
        out0 = mp_response.x1;
        out1 = mp_response.x2;
    } else if ((cmd == FIRME_ACS_CMD_RUN_RULE) ||
               (cmd == FIRME_ACS_CMD_GET_RULE_RESULT)) {
        val_realm_rule_command(cmd, a0, a1, a2, a3, &mp_response);
        status = mp_response.x0;
        out0 = mp_response.x1;
        out1 = mp_response.x2;
    } else {
        status = (uint64_t)ACS_STATUS_ERR;
        out0 = cmd;
        out1 = a0;
    }

    rmi_complete(args, status, out0, out1);
}
