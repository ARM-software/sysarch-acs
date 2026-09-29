/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdint.h>

#include "pal_platform.h"
#include "val_common/val_smccc.h"

#define PAL_PSCI_SYSTEM_OFF UINT64_C(0x84000008)

/** @brief Shut down FVP through PSCI; a successful request does not return. */
void pal_system_off(void)
{
    smccc_res_t result = {0};

    val_smccc_smc_call(PAL_PSCI_SYSTEM_OFF, 0U, 0U, 0U, 0U, 0U, 0U, 0U, &result);
}
