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

#include <stddef.h>

#define RMI_SUCCESS UINT64_C(0)

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
    } else if (cmd == (uint64_t)FIRME_ACS_CMD_RUN_RULE) {
        firme_rule_res_t rr = {0};
        val_firme_rule_run_realm((firme_rule_id_t)a0, a1, &rr);
        status = rr.status;
        out0 = rr.fail_step;
        out1 = rr.last_x0;
    } else {
        status = (uint64_t)ACS_STATUS_ERR;
        out0 = cmd;
        out1 = a0;
    }

    rmi_complete(args, status, out0, out1);
}
