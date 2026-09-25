/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_smmu.h"
#include "val_iovirt.h"
#include "val_common/val_status.h"
#include <stddef.h>

#define VAL_SMMU_R_IDR3_OFFSET UINT64_C(0x000c)
#define VAL_SMMU_R_MECIDR_OFFSET UINT64_C(0x0220)
#define VAL_SMMU_R_IDR3_MEC (UINT32_C(1) << 16)
#define VAL_SMMU_R_MECIDR_WIDTH_MASK UINT32_C(0xf)

int val_mmio_read32_checked(uint64_t address, uint32_t *value);

/** @brief Read each SMMU's actual Realm MEC capability and width, without writes. */
int val_smmu_mecid_width(uint32_t index, uint32_t *width)
{
    pal_smmu_info_t info = {0};
    uint32_t count = 0U;
    uint32_t idr3 = 0U;
    uint32_t mecidr = 0U;

    if (width == NULL)
        return VAL_ERROR;

    *width = 0U;
    if ((val_iovirt_smmu_count(&count) != VAL_SUCCESS) || (index >= count) ||
        (pal_iovirt_smmu_info(index, &info) != VAL_SUCCESS))
        return VAL_ERROR;

    if (val_mmio_read32_checked(info.realm_base + VAL_SMMU_R_IDR3_OFFSET, &idr3) !=
        VAL_SUCCESS)
        return VAL_ERROR;

    if ((idr3 & VAL_SMMU_R_IDR3_MEC) == 0U)
        return VAL_SMMU_MEC_UNAVAILABLE;

    if (val_mmio_read32_checked(info.realm_base + VAL_SMMU_R_MECIDR_OFFSET, &mecidr) !=
        VAL_SUCCESS)
        return VAL_ERROR;

    *width = (mecidr & VAL_SMMU_R_MECIDR_WIDTH_MASK) + 1U;
    return VAL_SUCCESS;
}
