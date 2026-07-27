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
                  "\n       PeTable ptr NULL");
    return;
  }

  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       ************PE TABLE DUMP************");
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       NUM PE %d",
                PeTable->header.num_of_pe);

  while (Index < PeTable->header.num_of_pe) {
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       PE NUM      :%x",
                  PeTable->pe_info[Index].pe_num);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       MPIDR       :%llx",
                  PeTable->pe_info[Index].mpidr);
//    pal_print_msg(ACS_PRINT_DEBUG,
//                  "    ATTR     :%x\n",
//                  PeTable->pe_info[Index].attr);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       PMU GSIV    :%x",
                  PeTable->pe_info[Index].pmu_gsiv);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       GIC MAINT GSIV    :%x",
                  PeTable->pe_info[Index].gmain_gsiv);
    Index++;
  }
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       *************************************");
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
                  "\n       GicTable ptr NULL");
    return;
  }

  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       ************GIC TABLE************");
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       GIC version %d",
                GicTable->header.gic_version);
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       GIC num D %d",
                GicTable->header.num_gicd);
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       GIC num GICC RD %d",
                GicTable->header.num_gicc_rd);
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       GIC num GICR RD %d",
                GicTable->header.num_gicr_rd);
//  pal_print_msg(ACS_PRINT_DEBUG,
//                " GIC num ITS %d\n",
//                GicTable->header.num_its);

  while (GicTable->gic_info[Index].type != 0xFF) {
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       GIC TYPE     :%x",
                  GicTable->gic_info[Index].type);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       BASE         :%x",
                  GicTable->gic_info[Index].base);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       LEN          :%x",
                  GicTable->gic_info[Index].length);
//    pal_print_msg(ACS_PRINT_DEBUG,
//                  "     ITS ID   :%x\n",
//                  GicTable->gic_info[Index].entry_id);
    Index++;
  }
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       *************************************");
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
                  "\n       WdTable ptr NULL");
    return;
  }

  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       ************WD TABLE************");
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       NUM WD %d",
                WdTable->header.num_wd);

  while (Index < WdTable->header.num_wd) {
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       REFRESH BASE  :%x",
                  WdTable->wd_info[Index].wd_refresh_base);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       CONTROL BASE  :%x",
                  WdTable->wd_info[Index].wd_ctrl_base);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       GSIV          :%x",
                  WdTable->wd_info[Index].wd_gsiv);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       FLAGS         :%x",
                  WdTable->wd_info[Index].wd_flags);
    Index++;
  }
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       *************************************");
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
                  "\n       PcieTable ptr NULL");
    return;
  }

  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       ************PCIE TABLE************");
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       NUM ECAM %d",
                PcieTable->num_entries);

  while (Index < PcieTable->num_entries) {
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       ECAM BASE          :%x",
                  PcieTable->block[Index].ecam_base);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       START BUS          :%x",
                  PcieTable->block[Index].start_bus_num);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       END BUS            :%x",
                  PcieTable->block[Index].end_bus_num);
//    pal_print_msg(ACS_PRINT_DEBUG,
//                  "      SEGMENT NUM   :%x\n",
//                  PcieTable->block[Index].segment_num);
    Index++;
  }
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       *************************************");
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
                  "\n       memoryInfoTable ptr NULL");
    return;
  }

  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       ************MEMORY TABLE************");
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       dram base  :%x",
                memoryInfoTable->dram_base);
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       dram size  :%x",
                memoryInfoTable->dram_size);

  while (memoryInfoTable->info[Index].type < 0x1004) {
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       Type      :%x",
                  memoryInfoTable->info[Index].type);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       PHY addr  :%x",
                  memoryInfoTable->info[Index].phy_addr);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       VIRT addr :%x",
                  memoryInfoTable->info[Index].virt_addr);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       size      :%x",
                  memoryInfoTable->info[Index].size);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       flags     :%x",
                  memoryInfoTable->info[Index].flags);
    Index++;
  }
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       *************************************");
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
                  "\n       TimerTable ptr NULL");
    return;
  }

  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       ************TIMER TABLE************");
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       Num of system timers %d",
                TimerTable->header.num_platform_timer);
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       s_el1_timer_flag %x",
                TimerTable->header.s_el1_timer_flag);
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       ns_el1_timer_flag %x",
                TimerTable->header.ns_el1_timer_flag);
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       el2_timer_flag %x",
                TimerTable->header.el2_timer_flag);
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       el2_virt_timer_flag %x",
                TimerTable->header.el2_virt_timer_flag);
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       s_el1_timer_gsiv %x",
                TimerTable->header.s_el1_timer_gsiv);
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       ns_el1_timer_gsiv %x",
                TimerTable->header.ns_el1_timer_gsiv);
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       el2_timer_gsiv %x",
                TimerTable->header.el2_timer_gsiv);
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       virtual_timer_flag %x",
                TimerTable->header.virtual_timer_flag);
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       virtual_timer_gsiv %x",
                TimerTable->header.virtual_timer_gsiv);
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       el2_virt_timer_gsiv %x",
                TimerTable->header.el2_virt_timer_gsiv);
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       CNTBase             %x",
                TimerTable->gt_info->block_cntl_base);

  while (Index < TimerTable->gt_info->timer_count) {
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       Frame num   :%x",
                  TimerTable->gt_info->frame_num[Index]);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       GtCntBase   :%x",
                  TimerTable->gt_info->GtCntBase[Index]);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       GtCntEl0Base:%x",
                  TimerTable->gt_info->GtCntEl0Base[Index]);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       gsiv        :%x",
                  TimerTable->gt_info->gsiv[Index]);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       virt_gsiv   :%x",
                  TimerTable->gt_info->virt_gsiv[Index]);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       flags       :%x",
                  TimerTable->gt_info->flags[Index]);
    Index++;
  }
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       *************************************");
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
                  "\n       peripheralInfoTable ptr NULL");
    return;
  }

  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       ************USB TABLE************");
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       NUM USB %d",
                peripheralInfoTable->header.num_usb);

  while (Index < peripheralInfoTable->header.num_usb) {
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       TYPE          :%x",
                  peripheralInfoTable->info[Index].type);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       CONTROL BASE  :%x",
                  peripheralInfoTable->info[Index].base0);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       GSIV          :%d",
                  peripheralInfoTable->info[Index].irq);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       FLAGS         :%x",
                  peripheralInfoTable->info[Index].flags);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       BDF           :%x",
                  peripheralInfoTable->info[Index].bdf);
    Index++;
  }

  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       ************SATA TABLE************");
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       NUM SATA %d",
                peripheralInfoTable->header.num_sata);

  while (Index < (peripheralInfoTable->header.num_sata + peripheralInfoTable->header.num_usb)) {
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       TYPE          :%x",
                  peripheralInfoTable->info[Index].type);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       CONTROL BASE  :%x",
                  peripheralInfoTable->info[Index].base0);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       GSIV          :%d",
                  peripheralInfoTable->info[Index].irq);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       FLAGS         :%x",
                  peripheralInfoTable->info[Index].flags);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       BDF           :%x",
                  peripheralInfoTable->info[Index].bdf);
    Index++;
  }

  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       ************UART TABLE************");
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       NUM UART %d",
                peripheralInfoTable->header.num_uart);

  while (Index < (peripheralInfoTable->header.num_sata + peripheralInfoTable->header.num_usb +
      peripheralInfoTable->header.num_uart)) {
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       TYPE          :%x",
                  peripheralInfoTable->info[Index].type);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       CONTROL BASE  :%x",
                  peripheralInfoTable->info[Index].base0);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       GSIV          :%d",
                  peripheralInfoTable->info[Index].irq);
    pal_print_msg(ACS_PRINT_DEBUG,
                  "\n       FLAGS         :%x",
                  peripheralInfoTable->info[Index].flags);
    Index++;
  }
  pal_print_msg(ACS_PRINT_DEBUG,
                "\n       *************************************");
}
