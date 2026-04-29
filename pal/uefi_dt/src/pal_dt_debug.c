/** @file
 * Copyright (c) 2021, 2023-2026, Arm Limited or its affiliates. All rights reserved.
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

#include <Uefi.h>
#include <Library/UefiLib.h>
#include "pal_uefi.h"
#include "pal_dt.h"

/**
  @brief  This API is use to dump PE_INFO Table after filling from DT

  @param  PeTable  - Address where the PE information needs to be filled.

  @return  None
**/
VOID
dt_dump_pe_table(PE_INFO_TABLE *PeTable)
{
  UINT32 Index = 0;

  if (!PeTable) {
    pal_print_msg(ACS_PRINT_ERR,
                  " PeTable ptr NULL\n");
    return;
  }

  pal_print_msg(ACS_PRINT_DEBUG,
                " ************PE TABLE DUMP************\n");
  pal_print_msg(ACS_PRINT_DEBUG,
                " NUM PE %d\n",
                PeTable->header.num_of_pe);

  while (Index < PeTable->header.num_of_pe) {
    pal_print_msg(ACS_PRINT_DEBUG,
                  " PE NUM      :%x\n",
                  PeTable->pe_info[Index].pe_num);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " MPIDR       :%llx\n",
                  PeTable->pe_info[Index].mpidr);
//    pal_print_msg(ACS_PRINT_DEBUG,
//                  "    ATTR     :%x\n",
//                  PeTable->pe_info[Index].attr);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " PMU GSIV    :%x\n",
                  PeTable->pe_info[Index].pmu_gsiv);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " GIC MAINT GSIV    :%x\n",
                  PeTable->pe_info[Index].gmain_gsiv);
    Index++;
  }
  pal_print_msg(ACS_PRINT_DEBUG,
                " *************************************\n");
}

/**
  @brief  This API is use to dump GIC_INFO Table after filling from DT

  @param  GicTable  - Address where the GIC information needs to be filled.

  @return  None
**/
VOID
dt_dump_gic_table(GIC_INFO_TABLE *GicTable)
{
  UINT32 Index = 0;

  if (!GicTable) {
    pal_print_msg(ACS_PRINT_ERR,
                  " GicTable ptr NULL\n");
    return;
  }

  pal_print_msg(ACS_PRINT_DEBUG,
                " ************GIC TABLE************\n");
  pal_print_msg(ACS_PRINT_DEBUG,
                " GIC version %d\n",
                GicTable->header.gic_version);
  pal_print_msg(ACS_PRINT_DEBUG,
                " GIC num D %d\n",
                GicTable->header.num_gicd);
  pal_print_msg(ACS_PRINT_DEBUG,
                " GIC num GICC RD %d\n",
                GicTable->header.num_gicc_rd);
  pal_print_msg(ACS_PRINT_DEBUG,
                " GIC num GICR RD %d\n",
                GicTable->header.num_gicr_rd);
//  pal_print_msg(ACS_PRINT_DEBUG,
//                " GIC num ITS %d\n",
//                GicTable->header.num_its);

  while (GicTable->gic_info[Index].type != 0xFF) {
    pal_print_msg(ACS_PRINT_DEBUG,
                  " GIC TYPE     :%x\n",
                  GicTable->gic_info[Index].type);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " BASE         :%x\n",
                  GicTable->gic_info[Index].base);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " LEN          :%x\n",
                  GicTable->gic_info[Index].length);
//    pal_print_msg(ACS_PRINT_DEBUG,
//                  "     ITS ID   :%x\n",
//                  GicTable->gic_info[Index].entry_id);
    Index++;
  }
  pal_print_msg(ACS_PRINT_DEBUG,
                " *************************************\n");
}

/**
  @brief  This API is use to dump WD_INFO Table after filling from DT

  @param  WdTable  - Address where the WD information needs to be filled.

  @return  None
**/
VOID
dt_dump_wd_table(WD_INFO_TABLE *WdTable)
{
  UINT32 Index = 0;

  if (!WdTable) {
    pal_print_msg(ACS_PRINT_ERR,
                  " WdTable ptr NULL\n");
    return;
  }

  pal_print_msg(ACS_PRINT_DEBUG,
                " ************WD TABLE************\n");
  pal_print_msg(ACS_PRINT_DEBUG,
                " NUM WD %d\n",
                WdTable->header.num_wd);

  while (Index < WdTable->header.num_wd) {
    pal_print_msg(ACS_PRINT_DEBUG,
                  " REFRESH BASE  :%x\n",
                  WdTable->wd_info[Index].wd_refresh_base);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " CONTROL BASE  :%x\n",
                  WdTable->wd_info[Index].wd_ctrl_base);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " GSIV          :%x\n",
                  WdTable->wd_info[Index].wd_gsiv);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " FLAGS         :%x\n",
                  WdTable->wd_info[Index].wd_flags);
    Index++;
  }
  pal_print_msg(ACS_PRINT_DEBUG,
                " *************************************\n");
}

/**
=======
  @brief  This API is use to dump PCIE_INFO Table after filling from DT

  @param  PcieTable  - Address where the PCIE information needs to be filled.

  @return  None
**/
VOID
dt_dump_pcie_table(PCIE_INFO_TABLE *PcieTable)
{
  UINT32 Index = 0;

  if (!PcieTable) {
    pal_print_msg(ACS_PRINT_ERR,
                  " PcieTable ptr NULL\n");
    return;
  }

  pal_print_msg(ACS_PRINT_DEBUG,
                " ************PCIE TABLE************\n");
  pal_print_msg(ACS_PRINT_DEBUG,
                " NUM ECAM %d\n",
                PcieTable->num_entries);

  while (Index < PcieTable->num_entries) {
    pal_print_msg(ACS_PRINT_DEBUG,
                  " ECAM BASE          :%x\n",
                  PcieTable->block[Index].ecam_base);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " START BUS          :%x\n",
                  PcieTable->block[Index].start_bus_num);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " END BUS            :%x\n",
                  PcieTable->block[Index].end_bus_num);
//    pal_print_msg(ACS_PRINT_DEBUG,
//                  "      SEGMENT NUM   :%x\n",
//                  PcieTable->block[Index].segment_num);
    Index++;
  }
  pal_print_msg(ACS_PRINT_DEBUG,
                " *************************************\n");
}

/**
  @brief  This API is use to dump MEMORY_INFO Table after filling from DT

  @param  memoryInfoTable  - Address where the MEMORY information needs to be filled.

  @return  None
**/
VOID
dt_dump_memory_table(MEMORY_INFO_TABLE *memoryInfoTable)
{
  UINT32 Index = 0;

  if (!memoryInfoTable) {
    pal_print_msg(ACS_PRINT_ERR,
                  " memoryInfoTable ptr NULL\n");
    return;
  }

  pal_print_msg(ACS_PRINT_DEBUG,
                " ************MEMORY TABLE************\n");
  pal_print_msg(ACS_PRINT_DEBUG,
                " dram base  :%x\n",
                memoryInfoTable->dram_base);
  pal_print_msg(ACS_PRINT_DEBUG,
                " dram size  :%x\n",
                memoryInfoTable->dram_size);

  while (memoryInfoTable->info[Index].type < 0x1004) {
    pal_print_msg(ACS_PRINT_DEBUG,
                  " Type      :%x\n",
                  memoryInfoTable->info[Index].type);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " PHY addr  :%x\n",
                  memoryInfoTable->info[Index].phy_addr);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " VIRT addr :%x\n",
                  memoryInfoTable->info[Index].virt_addr);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " size      :%x\n",
                  memoryInfoTable->info[Index].size);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " flags     :%x\n",
                  memoryInfoTable->info[Index].flags);
    Index++;
  }
  pal_print_msg(ACS_PRINT_DEBUG,
                " *************************************\n");
}

/**
  @brief  This API is use to dump TIMER_INFO Table after filling from DT

  @param  TimerTable  - Address where the TIMER information needs to be filled.

  @return  None
**/
VOID
dt_dump_timer_table(TIMER_INFO_TABLE *TimerTable)
{
  UINT32 Index = 0;

  if (!TimerTable) {
    pal_print_msg(ACS_PRINT_ERR,
                  " TimerTable ptr NULL\n");
    return;
  }

  pal_print_msg(ACS_PRINT_DEBUG,
                " ************TIMER TABLE************\n");
  pal_print_msg(ACS_PRINT_DEBUG,
                " Num of system timers %d\n",
                TimerTable->header.num_platform_timer);
  pal_print_msg(ACS_PRINT_DEBUG,
                " s_el1_timer_flag %x\n",
                TimerTable->header.s_el1_timer_flag);
  pal_print_msg(ACS_PRINT_DEBUG,
                " ns_el1_timer_flag %x\n",
                TimerTable->header.ns_el1_timer_flag);
  pal_print_msg(ACS_PRINT_DEBUG,
                " el2_timer_flag %x\n",
                TimerTable->header.el2_timer_flag);
  pal_print_msg(ACS_PRINT_DEBUG,
                " el2_virt_timer_flag %x\n",
                TimerTable->header.el2_virt_timer_flag);
  pal_print_msg(ACS_PRINT_DEBUG,
                " s_el1_timer_gsiv %x\n",
                TimerTable->header.s_el1_timer_gsiv);
  pal_print_msg(ACS_PRINT_DEBUG,
                " ns_el1_timer_gsiv %x\n",
                TimerTable->header.ns_el1_timer_gsiv);
  pal_print_msg(ACS_PRINT_DEBUG,
                " el2_timer_gsiv %x\n",
                TimerTable->header.el2_timer_gsiv);
  pal_print_msg(ACS_PRINT_DEBUG,
                " virtual_timer_flag %x\n",
                TimerTable->header.virtual_timer_flag);
  pal_print_msg(ACS_PRINT_DEBUG,
                " virtual_timer_gsiv %x\n",
                TimerTable->header.virtual_timer_gsiv);
  pal_print_msg(ACS_PRINT_DEBUG,
                " el2_virt_timer_gsiv %x\n",
                TimerTable->header.el2_virt_timer_gsiv);
  pal_print_msg(ACS_PRINT_DEBUG,
                " CNTBase             %x\n",
                TimerTable->gt_info->block_cntl_base);

  while (Index < TimerTable->gt_info->timer_count) {
    pal_print_msg(ACS_PRINT_DEBUG,
                  " Frame num   :%x\n",
                  TimerTable->gt_info->frame_num[Index]);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " GtCntBase   :%x\n",
                  TimerTable->gt_info->GtCntBase[Index]);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " GtCntEl0Base:%x\n",
                  TimerTable->gt_info->GtCntEl0Base[Index]);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " gsiv        :%x\n",
                  TimerTable->gt_info->gsiv[Index]);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " virt_gsiv   :%x\n",
                  TimerTable->gt_info->virt_gsiv[Index]);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " flags       :%x\n",
                  TimerTable->gt_info->flags[Index]);
    Index++;
  }
  pal_print_msg(ACS_PRINT_DEBUG,
                " *************************************\n");
}

/**
  @brief  This API is use to dump peripheralInfo Table after filling from DT

  @param  peripheralInfoTable  - Address where the WD information needs to be filled.

  @return  None
**/
VOID
dt_dump_peripheral_table(PERIPHERAL_INFO_TABLE *peripheralInfoTable)
{
  UINT32 Index = 0;

  if (!peripheralInfoTable) {
    pal_print_msg(ACS_PRINT_ERR,
                  " peripheralInfoTable ptr NULL\n");
    return;
  }

  pal_print_msg(ACS_PRINT_DEBUG,
                " ************USB TABLE************\n");
  pal_print_msg(ACS_PRINT_DEBUG,
                " NUM USB %d\n",
                peripheralInfoTable->header.num_usb);

  while (Index < peripheralInfoTable->header.num_usb) {
    pal_print_msg(ACS_PRINT_DEBUG,
                  " TYPE          :%x\n",
                  peripheralInfoTable->info[Index].type);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " CONTROL BASE  :%x\n",
                  peripheralInfoTable->info[Index].base0);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " GSIV          :%d\n",
                  peripheralInfoTable->info[Index].irq);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " FLAGS         :%x\n",
                  peripheralInfoTable->info[Index].flags);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " BDF           :%x\n",
                  peripheralInfoTable->info[Index].bdf);
    Index++;
  }

  pal_print_msg(ACS_PRINT_DEBUG,
                " ************SATA TABLE************\n");
  pal_print_msg(ACS_PRINT_DEBUG,
                " NUM SATA %d\n",
                peripheralInfoTable->header.num_sata);

  while (Index < (peripheralInfoTable->header.num_sata + peripheralInfoTable->header.num_usb)) {
    pal_print_msg(ACS_PRINT_DEBUG,
                  " TYPE          :%x\n",
                  peripheralInfoTable->info[Index].type);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " CONTROL BASE  :%x\n",
                  peripheralInfoTable->info[Index].base0);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " GSIV          :%d\n",
                  peripheralInfoTable->info[Index].irq);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " FLAGS         :%x\n",
                  peripheralInfoTable->info[Index].flags);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " BDF           :%x\n",
                  peripheralInfoTable->info[Index].bdf);
    Index++;
  }

  pal_print_msg(ACS_PRINT_DEBUG,
                " ************UART TABLE************\n");
  pal_print_msg(ACS_PRINT_DEBUG,
                " NUM UART %d\n",
                peripheralInfoTable->header.num_uart);

  while (Index < (peripheralInfoTable->header.num_sata + peripheralInfoTable->header.num_usb +
      peripheralInfoTable->header.num_uart)) {
    pal_print_msg(ACS_PRINT_DEBUG,
                  " TYPE          :%x\n",
                  peripheralInfoTable->info[Index].type);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " CONTROL BASE  :%x\n",
                  peripheralInfoTable->info[Index].base0);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " GSIV          :%d\n",
                  peripheralInfoTable->info[Index].irq);
    pal_print_msg(ACS_PRINT_DEBUG,
                  " FLAGS         :%x\n",
                  peripheralInfoTable->info[Index].flags);
    Index++;
  }
  pal_print_msg(ACS_PRINT_DEBUG,
                " *************************************\n");
}
