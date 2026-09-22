/** @file
 * Copyright (c) 2019,2024-2026, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
**/

#include "pal_uefi.h"

/* This is a place-holder file. Need to be implemented if needed in later releases */

/**
  @brief   Begin monitoring DMA IOVAs for a device port
**/
VOID
pal_smmu_device_start_monitor_iova(VOID *port)
{
  (void)port;
  pal_warn_not_implemented(__func__);
}

/**
  @brief   Stop monitoring DMA IOVAs for a device port
**/
VOID
pal_smmu_device_stop_monitor_iova(VOID *port)
{
  (void)port;
  pal_warn_not_implemented(__func__);
}

/**
  @brief   Check if a DMA address is present in the SMMU IOVA table
  @return  PAL_STATUS_NOT_IMPLEMENTED to signal lack of support on UEFI PAL
**/
UINT32
pal_smmu_check_device_iova(VOID *port, UINT64 dma_addr)
{
  (void)port;
  (void)dma_addr;
  pal_warn_not_implemented(__func__);
  return PAL_STATUS_NOT_IMPLEMENTED;
}
