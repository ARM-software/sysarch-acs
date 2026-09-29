/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>

#include "val_common/val_status.h"

#include "../../../platform/pal_baremetal/targets/fvp/src/pal_power.c"

static uint32_t g_shutdown_calls;

/** @brief Check the real PAL shutdown frame without shutting down the native host. */
void val_smccc_smc_call(uint64_t fid,
    uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
    uint64_t arg5, uint64_t arg6, uint64_t arg7, smccc_res_t *result)
{
    assert(fid == UINT64_C(0x84000008));
    assert(arg1 == 0U && arg2 == 0U && arg3 == 0U && arg4 == 0U);
    assert(arg5 == 0U && arg6 == 0U && arg7 == 0U && result != NULL);
    g_shutdown_calls++;
    result->x0 = UINT64_MAX;
}

/** @brief Exercise the PAL entrypoint and its unexpected-return path. */
int main(void)
{
    pal_system_off();
    assert(g_shutdown_calls == 1U);
    puts("FVP platform shutdown frame: PASS");
    return VAL_SUCCESS;
}
