/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_dispatch.h"

#include "val_firme_spmd_msg.h"
#include "val_firme_rmmd.h"
#include "val_common/val_smccc.h"
#include "val_common/val_status.h"
#include "pal_platform.h"

/**
 *   @brief    - Check whether a value is an ACS rule status.
 *   @param    - status : Candidate ACS_STATUS_* value.
 *   @return   - 1 if the value is an ACS status, 0 otherwise.
**/
static int val_firme_is_rule_status(uint64_t status)
{
    return (status == (uint64_t)ACS_STATUS_PASS) ||
           (status == (uint64_t)ACS_STATUS_FAIL) ||
           (status == (uint64_t)ACS_STATUS_SKIP) ||
           (status == (uint64_t)ACS_STATUS_ERR);
}

/**
 *   @brief    - Call into the Secure world shim using SPMD direct messaging.
 *   @param    - ns_id   : NS endpoint ID.
 *   @param    - dst_id  : Destination endpoint ID (Secure shim logical endpoint).
 *   @param    - cmd     : FIRME ACS command ID.
 *   @param    - a0      : Argument 0.
 *   @param    - a1      : Argument 1.
 *   @param    - a2      : Argument 2.
 *   @param    - a3      : Argument 3.
 *   @param    - out     : SMCCC result registers.
 *   @return   - 0 on success, non-zero otherwise.
**/
static int secure_transport_direct_msg(uint16_t ns_id, uint16_t dst_id,
                                       uint64_t cmd,
                                       uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3,
                                       smccc_res_t *out)
{
    uint32_t src_dst = ((uint32_t)ns_id << 16) | (uint32_t)dst_id;
    /*
     * Use SMC32 direct messaging for maximum TF-A SPMD compatibility.
     * We still carry 64-bit values in x3..x7 (AArch64 registers) but keep the
     * direct-messaging call convention as SMC32.
     */
    val_smccc_smc_call(FFA_MSG_SEND_DIRECT_REQ_SMC32, src_dst, 0, cmd, a0, a1, a2, a3, out);
    if ((uint32_t)out->x0 == (uint32_t)FFA_MSG_SEND_DIRECT_RESP_SMC32 ||
        (uint32_t)out->x0 == (uint32_t)FFA_MSG_SEND_DIRECT_RESP_SMC64) {
        return VAL_SUCCESS;
    }
    return VAL_ERROR;
}

/**
 *   @brief    - Invoke the Realm shim via a private RMI-range dispatch SMC (RMMD forwarded).
 *   @param    - cmd : FIRME ACS command ID.
 *   @param    - a0  : Argument 0.
 *   @param    - a1  : Argument 1.
 *   @param    - a2  : Argument 2.
 *   @param    - a3  : Argument 3.
 *   @param    - out : SMCCC result registers.
 *   @return   - 0 if the call was forwarded, non-zero otherwise.
**/
static int realm_transport_rmi_dispatch(uint64_t cmd,
                                       uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3,
                                       smccc_res_t *out)
{
    val_smccc_smc_call((uint64_t)FIRME_ACS_RMI_DISPATCH, cmd, a0, a1, a2, a3, 0, 0, out);
    if ((uint32_t)out->x0 == (uint32_t)SMCCC_SMC_UNK) {
        return VAL_ERROR;
    }
    return VAL_SUCCESS;
}

/**
 *   @brief    - Call into another world and return a normalized result.
 *   @param    - world : Target world (Secure or Realm).
 *   @param    - cmd   : FIRME ACS command to execute.
 *   @param    - arg0  : Argument 0.
 *   @param    - arg1  : Argument 1.
 *   @param    - arg2  : Argument 2.
 *   @param    - arg3  : Argument 3.
 *   @param    - res   : Normalized result (x0=status, x1..x4 payload).
 *   @return   - 0 on success, non-zero on failure/unimplemented.
**/
int val_firme_world_call(firme_acs_world_t world, firme_acs_cmd_t cmd,
                         uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                         firme_acs_call_res_t *res)
{
    smccc_res_t out = {0};
    uint16_t ns_id = 0;

    if (res) {
        volatile uint64_t *dst = (volatile uint64_t *)(void *)res;
        dst[0] = (uint64_t)VAL_ERROR;
        dst[1] = 0;
        dst[2] = 0;
        dst[3] = 0;
        dst[4] = 0;
    }

    if (world == FIRME_ACS_WORLD_SECURE) {
        ns_id = (uint16_t)PLAT_FFA_NS_ENDPOINT_ID;
        if (secure_transport_direct_msg(ns_id, (uint16_t)PLAT_FFA_ACS_SECURE_ENDPOINT_ID,
                                        (uint64_t)cmd,
                                        arg0, arg1, arg2, arg3, &out) != VAL_SUCCESS) {
            return VAL_ERROR;
        }

        if (res) {
            /* Normalize to: x0=status, x1..x4=payload results. */
            volatile uint64_t *dst = (volatile uint64_t *)(void *)res;
            dst[0] = out.x3;
            dst[1] = out.x4;
            dst[2] = out.x5;
            dst[3] = out.x6;
            dst[4] = out.x7;
        }
        return VAL_SUCCESS;
    }

    if (world == FIRME_ACS_WORLD_REALM) {
        if (realm_transport_rmi_dispatch((uint64_t)cmd, arg0, arg1, arg2, arg3, &out) !=
            VAL_SUCCESS) {
            return VAL_ERROR;
        }
        if (res) {
            /*
             * Normalize the RMMD-safe result layout to x0=status and
             * x1..x2=payload. In TF-A RMMD V1 compatibility mode, only Realm
             * x1-x5 are forwarded, so ACS must not reconstruct payload from
             * non-returned Realm x6/x7 values.
             */
            if ((out.x0 == 0ULL) && (out.x1 == 0ULL) &&
                val_firme_is_rule_status(out.x2)) {
                res->x0 = out.x2;
                res->x1 = out.x3;
                res->x2 = out.x4;
                res->x3 = 0;
                res->x4 = 0;
            } else {
                res->x0 = (uint64_t)ACS_STATUS_FAIL;
                res->x1 = out.x0;
                res->x2 = out.x1;
                res->x3 = out.x2;
                res->x4 = out.x3;
            }
        }
        return VAL_SUCCESS;
    }

    return VAL_ERROR;
}
