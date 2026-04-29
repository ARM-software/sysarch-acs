/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
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

#include "pal_pcie_enum.h"
#include "pal_common_support.h"
#include "platform_override_struct.h"

extern CXL_INFO_TABLE platform_cxl_cfg;

void
pal_cxl_create_info_table(CXL_INFO_TABLE *CxlTable)
{

  uint32_t i = 0, k = 0;
  uint32_t host_count, window_count;
  uint32_t max_windows;

  if (CxlTable == NULL) {
    pal_print_msg(ACS_PRINT_ERR,
                  "Input CXL Table Pointer is NULL. Cannot create CXL INFO Table\n");
    return;
  }

  CxlTable->num_entries = 0;

  if (platform_cxl_cfg.num_entries == 0) {
    pal_print_msg(ACS_PRINT_ERR,
                  "Number of HB is 0. Cannot create CXL INFO\n");
    return;
  }

  host_count = platform_cxl_cfg.num_entries;

  max_windows = CXL_MAX_CFMWS_WINDOWS;
  for (i = 0; i < host_count; i++) {
    CxlTable->device[i].cxl_struct_type = platform_cxl_cfg.device[i].cxl_struct_type;
    CxlTable->device[i].uid = platform_cxl_cfg.device[i].uid;
    CxlTable->device[i].component_reg_type = platform_cxl_cfg.device[i].component_reg_type;
    CxlTable->device[i].component_reg_base = platform_cxl_cfg.device[i].component_reg_base;
    CxlTable->device[i].component_reg_length = platform_cxl_cfg.device[i].component_reg_length;
    CxlTable->device[i].cxl_version = platform_cxl_cfg.device[i].cxl_version;

    window_count = platform_cxl_cfg.device[i].cfmws_count;
    if (window_count > max_windows) {
      window_count = max_windows;
      pal_print_msg(ACS_PRINT_WARN,
                    "CFMWS count exceeds per-host capacity. Truncating\n");
    }
    CxlTable->device[i].cfmws_count = window_count;

    for (k = 0; k < window_count; k++) {
      CxlTable->device[i].cfmws_base[k] = platform_cxl_cfg.device[i].cfmws_base[k];
      CxlTable->device[i].cfmws_length[k] = platform_cxl_cfg.device[i].cfmws_length[k];
      CxlTable->device[i].cfmws_window[k] = platform_cxl_cfg.device[i].cfmws_window[k];
    }
  }
  CxlTable->num_entries = host_count;
  return;
}

/**
  @brief   Return the CEDT host bridge UID for the given root port BDF.

  @param  bdf  Root port Bus/Device/Function identifier.
  @param  uid  Output pointer for the host bridge UID.

  @return 0 on success, non-zero when mapping is unavailable.
**/
uint32_t
pal_cxl_get_host_bridge_uid(uint32_t bdf, uint32_t *uid)
{
  (void)bdf;
  if (uid != NULL)
    *uid = 0;
  else
    pal_print_msg(ACS_PRINT_ERR,
                  " %s UID pointer NULL ",
                  __func__);
  return 1;
}
