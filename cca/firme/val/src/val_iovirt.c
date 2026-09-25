/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_iovirt.h"
#include "val_common/val_status.h"
#include <stddef.h>

#define VAL_SMMU_REGISTER_PAGE_SIZE UINT64_C(0x10000)

/** @brief Validate the complete platform SMMU table before trusting its count. */
int val_iovirt_smmu_count(uint32_t *count)
{
    pal_smmu_info_t info = {0};
    pal_smmu_info_t previous = {0};
    uint32_t index = 0U;
    uint32_t earlier = 0U;
    uint32_t total = pal_iovirt_smmu_count();

    if (count == NULL)
        return VAL_ERROR;

    *count = 0U;
    for (index = 0U; index < total; index++) {
        if ((pal_iovirt_smmu_info(index, &info) != VAL_SUCCESS) ||
            (info.base == 0U) || (info.realm_base == 0U) ||
            (info.base == info.realm_base) ||
            ((info.base & (VAL_SMMU_REGISTER_PAGE_SIZE - 1U)) != 0U) ||
            ((info.realm_base & (VAL_SMMU_REGISTER_PAGE_SIZE - 1U)) != 0U) ||
            (info.realm_base > UINT64_MAX - VAL_SMMU_REGISTER_PAGE_SIZE))
            return VAL_ERROR;

        for (earlier = 0U; earlier < index; earlier++) {
            if ((pal_iovirt_smmu_info(earlier, &previous) != VAL_SUCCESS) ||
                (info.base == previous.base) || (info.realm_base == previous.realm_base))
                return VAL_ERROR;
        }
    }

    *count = total;
    return VAL_SUCCESS;
}
