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

/* This is a place-holder file. Need to be implemented if needed in later releases */
#include "include/pal_uefi.h"

/**
  @brief   This API converts physical address to IO virtual address
  @param   SmmuBase       - Physical addr of the SMMU for pa to iova conversion
  @param   Pa             - Physical address to use in conversion
  @param   dram_buf_iova  - IOVA addresses for DMA purposes

  @return
  - 0               : Success
  - NOT_IMPLEMENTED : Feature not implemented
  - non-zero        : Failure (implementation-specific error code)
*/
UINT64
pal_smmu_pa2iova(
  UINT64 SmmuBase,
  UINT64 Pa, UINT64 *dram_buf_iova
  )
{
  return NOT_IMPLEMENTED;
}
