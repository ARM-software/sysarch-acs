/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi_test.h"

#include "val_common/val_status.h"
#include "val_firme_abi.h"

/** @brief Feed fixture register frames to existing native ABI models, not a DUT. */
int val_firme_abi_call(val_firme_abi_test_context_t *context,
                       const smccc_call_frame_t *input, smccc_call_frame_t *output,
                       uint32_t *issued)
{
    firme_res_t result = {0};
    uint64_t argument0 = input->registers[1];
    uint64_t argument1 = input->registers[2];
    uint64_t argument2 = input->registers[3];
    int call_status;

    if (context->invoke_frame) {
        call_status = context->invoke_frame(input, output, issued, context->invoke_cookie);
        if (call_status != VAL_SUCCESS && *issued)
            context->transport_uncertain = VAL_TRUE;

        return call_status;
    }

    *issued = VAL_TRUE;
    switch (input->registers[0]) {
    case FIRME_FID_SERVICE_FEATURES:
        val_firme_service_features(argument0, argument1, &result);
        break;
    case FIRME_FID_GM_GPT_INFO:
        val_firme_gm_gpt_info(argument0, argument1, &result);
        break;
    case FIRME_FID_GM_GPI_SET:
        val_firme_gm_gpi_set(argument0, argument1, argument2, &result);
        break;
    case FIRME_FID_GM_GPI_OP_CONTINUE:
        val_firme_gm_gpi_op_continue(argument0, &result);
        break;
    case FIRME_FID_GM_L1_GPT_CREATE:
        val_firme_gm_l1_gpt_create(argument0, argument1, &result);
        break;
    case FIRME_FID_GM_L1_GPT_DESTROY:
        val_firme_gm_l1_gpt_destroy(argument0, &result);
        break;
    default:
        *issued = VAL_FALSE;
        return VAL_ERROR;
    }

    *output = (smccc_call_frame_t){0};
    output->registers[0] = result.x0;
    output->registers[1] = result.x1;
    output->registers[2] = result.x2;
    output->registers[3] = result.x3;
    return VAL_SUCCESS;
}
