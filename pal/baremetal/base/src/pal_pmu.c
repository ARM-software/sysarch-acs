/** @file
 * Copyright (c) 2024-2026, Arm Limited or its affiliates. All rights reserved.
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

#include "platform_override_struct.h"
#include "pal_common_support.h"

extern PLATFORM_OVERRIDE_PMU_INFO_TABLE platform_pmu_cfg;
extern PLATFORM_OVERRIDE_EVENT_DETAILS event_list[];

/**
  @brief  Display PMU info table details

  @param  PmuTable  - Address to the PMU information table.

  @return  None
**/
void
pal_pmu_dump_info_table(PMU_INFO_TABLE *PmuTable)
{
  uint32_t i;

  if (PmuTable == NULL) {
      return;
  }

  for (i = 0; i < PmuTable->pmu_count; i++) {
      print(ACS_PRINT_INFO, "\nPMU info Index      :%d ", i);
      print(ACS_PRINT_INFO, "\nPMU node type       :%02X ", PmuTable->info[i].type);
      print(ACS_PRINT_INFO, "\nDual page extension :%d ",
                 PmuTable->info[i].dual_page_extension);
      print(ACS_PRINT_INFO, "\nBase Address 0      :%llX ", PmuTable->info[i].base0);
      if(PmuTable->info[i].dual_page_extension)
          print(ACS_PRINT_INFO, "\nBase Address 1      :%llX ", PmuTable->info[i].base1);
      print(ACS_PRINT_INFO, "\nPrimary Instance    :%llX ",
                 PmuTable->info[i].primary_instance);
      print(ACS_PRINT_INFO, "\nSecondary Instance  :%08X ",
                 PmuTable->info[i].secondary_instance);

  }
}

/**
  @brief  This API fills in the PMU_INFO_TABLE with information about local and system
          timers in the system. this employs baremetal platform specific data.

  @param  PmuTable  - Address where the PMU information needs to be filled.

  @return  None
**/
void
pal_pmu_create_info_table(PMU_INFO_TABLE *PmuTable)
{

  uint32_t i;

  /* Check if memory for PMU info table allocated */
  if (PmuTable == NULL) {
      print(ACS_PRINT_ERR, "\n Input PMU Table Pointer is NULL");
      return;
  }

  /* Initialize PMU info table */
  PmuTable->pmu_count = 0;

  /* Populate PMU information */
  for (i = 0; i < platform_pmu_cfg.pmu_count; i++) {
      PmuTable->info[i].type = platform_pmu_cfg.pmu_info[i].type;
      PmuTable->info[i].dual_page_extension = platform_pmu_cfg.pmu_info[i].dual_page_extension;
      PmuTable->info[i].base0 = platform_pmu_cfg.pmu_info[i].base0;
      PmuTable->info[i].base1 = platform_pmu_cfg.pmu_info[i].base1;
      PmuTable->info[i].primary_instance = platform_pmu_cfg.pmu_info[i].primary_instance;
      PmuTable->info[i].secondary_instance = platform_pmu_cfg.pmu_info[i].secondary_instance;

      PmuTable->pmu_count++;
      if (PmuTable->pmu_count >= MAX_NUM_OF_PMU_SUPPORTED) {
          print(ACS_PRINT_WARN, "\n Number of PMUs greater than %d",
                MAX_NUM_OF_PMU_SUPPORTED);
          break;
      }

      /* Dump PMU info table */
      if (g_print_level <= ACS_PRINT_DEBUG)
          pal_pmu_dump_info_table(PmuTable);
  }
}

/**
  @brief  This API returns the event ID to be filled into PMEVTYPER register.
          Prerequisite - event_list array. This API should be called after
          filling the required event IDs into event_list array.

  @param  node_index  -  Index of PMU node.
  @param  event_type  -  Type of the event.
  @param  node_type   -  PMU Node type

  @return  Event ID

**/
uint32_t
pal_pmu_get_event_info(uint32_t node_index, PMU_EVENT_TYPE_e event_type,
                       PMU_NODE_INFO_TYPE node_type)
{
  uint32_t i = 0;
  while (event_list[i].node_index != node_index || event_list[i].node_type != node_type ||
         event_list[i].event_desc != event_type) {
    i++;
  }

  return event_list[i].event_id;
}
