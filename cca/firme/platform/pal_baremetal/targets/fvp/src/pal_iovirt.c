/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_iovirt.h"
#include "val_common/val_status.h"
#include <stddef.h>

/* FVP PCI SMMUv3; Realm page 0 follows ROOT_IDR0.BA_REALM=2. */
static const pal_smmu_info_t g_pal_smmus[] = {
    {.base = UINT64_C(0x2b400000), .realm_base = UINT64_C(0x2b440000)},
};

/** @brief Return the complete target SMMU inventory, not the accessible subset. */
uint32_t pal_iovirt_smmu_count(void)
{
    return sizeof(g_pal_smmus) / sizeof(g_pal_smmus[0]);
}

/** @brief Look up independently supplied platform addresses, never MEC widths. */
int pal_iovirt_smmu_info(uint32_t index, pal_smmu_info_t *info)
{
    if ((info == NULL) || (index >= pal_iovirt_smmu_count()))
        return VAL_ERROR;

    *info = g_pal_smmus[index];
    return VAL_SUCCESS;
}
