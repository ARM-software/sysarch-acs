/** @file
 * Copyright (c) 2025-2026, Arm Limited or its affiliates. All rights reserved.
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

#include "acs_val.h"
#include "acs_common.h"
#include "val_interface.h"
#include "acs_tpm.h"


#ifndef TARGET_LINUX

static TPM2_INFO_TABLE *g_tpm2_info_table;

#define TPM2_INFO_INVALID 0xFFFFFFFF

/**
  @brief   This API is the single entry point to return all TPM2 related information
           1. Caller       -  Test Suite
           2. Prerequisite -  val_tpm2_create_info_table

  @param   info_type  - Type of the TPM2 information to be returned

  @return  64-bit data pertaining to the requested input type
**/

uint64_t
val_tpm2_get_info(TPM2_INFO_e info_type)
{

  if (g_tpm2_info_table == NULL)
      return 0;

  switch (info_type) {
  case TPM2_INFO_IS_PRESENT:
      return g_tpm2_info_table->tpm_presence;
  case TPM2_INFO_BASE_ADDR:
      return g_tpm2_info_table->base;
  case TPM2_INFO_INTERFACE_TYPE:
      return g_tpm2_info_table->tpm_interface;
  default:
      val_print(ACS_PRINT_ERR, " Invalid TPM2 info_type: %d\n", info_type);
      return TPM2_INFO_INVALID;
  }

}


/**
  @brief   This API will call PAL layer to fill in the TPM2 table information
           into the g_tpm2_info_table pointer.
           1. Caller       -  Application layer.
           2. Prerequisite -  Memory allocated and passed as argument.
  @param   tpm2_info_table  pre-allocated memory pointer for cache info.
  @return  Error if Input parameter is NULL
**/
void
val_tpm2_create_info_table(uint64_t *tpm2_info_table)
{
  if (tpm2_info_table == NULL) {
    val_print(ACS_PRINT_ERR, "\n Pre-allocated memory pointer is NULL\n", 0);
    return;
  }
  val_print(ACS_PRINT_INFO, " Creating TPM2 INFO table\n", 0);

  g_tpm2_info_table = (TPM2_INFO_TABLE *)tpm2_info_table;

  pal_tpm2_create_info_table(g_tpm2_info_table);
  return;
}

/**
  @brief   Returns the TPM2 version from the PAL layer.

  @return  64-bit TPM2 version value or 0 on failure
**/

uint64_t
val_tpm2_get_version(void)
{
  return pal_tpm2_get_version();
}

/**
  @brief   This API frees the memory allocated for TPM2 info table.
  @param   None
  @return  None
**/
void
val_tpm2_free_info_table(void)
{
    if (g_tpm2_info_table != NULL) {
        pal_mem_free_aligned((void *)g_tpm2_info_table);
        g_tpm2_info_table = NULL;
    }
    else {
      val_print(ACS_PRINT_ERR,
                  "\n WARNING: g_tpm2_info_table pointer is already NULL",
        0);
    }
}

#endif