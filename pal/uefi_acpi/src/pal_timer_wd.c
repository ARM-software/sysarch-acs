/** @file
 * Copyright (c) 2018-2026, Arm Limited or its affiliates. All rights reserved.
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

#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/AcpiTable.h>
#include "Include/IndustryStandard/Acpi61.h"

#include "pal_uefi.h"
#include "platform_override.h"

static EFI_ACPI_6_1_GENERIC_TIMER_DESCRIPTION_TABLE *gGtdtHdr;

UINT64
pal_get_gtdt_ptr();

UINT64
pal_get_platform_time_us (
  VOID
  )
{
  EFI_STATUS Status;
  EFI_TIME Time;
  UINT64 TimeUs;

  Status = gRT->GetTime(&Time, NULL);
  if (EFI_ERROR(Status)) {
    pal_print_msg(ACS_PRINT_WARN, " GetTime failed: %x\n", Status);
    return ~0ULL;
  }

  TimeUs = (((UINT64)Time.Hour * 60 + Time.Minute) * 60 + Time.Second) * 1000000ULL;
  TimeUs += (UINT64)Time.Nanosecond / 1000ULL;

  return TimeUs;
}

/**
  @brief This API overrides the timer specified by TimerTable
         Note: Information about only one timer can be mentioned as an Override

  @param TimerTable Pointer to timer info table

  @return None
**/
static
VOID
pal_timer_platform_override(TIMER_INFO_TABLE *TimerTable)
{
  if (PLATFORM_OVERRIDE_PLATFORM_TIMER) {
      TimerTable->header.num_platform_timer = 1;
      TimerTable->gt_info[0].block_cntl_base = PLATFORM_OVERRIDE_CNTCTL_BASE;
      TimerTable->gt_info[0].timer_count = 1;
      TimerTable->gt_info[0].GtCntBase[0]  = PLATFORM_OVERRIDE_CNTBASE_N;
      TimerTable->gt_info[0].gsiv[0] = PLATFORM_OVERRIDE_PLATFORM_TIMER_GSIV;
  }

}

/**
  @brief  This API fills in the TIMER_INFO_TABLE with information about local and system
          timers in the system. This is achieved by parsing the ACPI - GTDT table.

  @param  TimerTable  - Address where the Timer information needs to be filled.

  @return  None
**/
VOID
pal_timer_create_info_table(TIMER_INFO_TABLE *TimerTable)
{
  EFI_ACPI_6_1_GTDT_GT_BLOCK_STRUCTURE       *Entry = NULL;
  EFI_ACPI_6_1_GTDT_GT_BLOCK_TIMER_STRUCTURE *GtBlockTimer = NULL;
  TIMER_INFO_GTBLOCK         *GtEntry = NULL;
  UINT32                      Offset= 0;
  UINT32                      i;
  UINT32                      num_of_entries;
  UINT32                      revision = 0;
  UINT8                       *gtdt_ptr = NULL;
  UINTN                       el2_virt_timer_offset;

  if (TimerTable == NULL) {
    pal_print_msg(ACS_PRINT_ERR,
                  " Input Timer Table Pointer is NULL. Cannot create Timer INFO\n");
    return;
  }

  GtEntry = TimerTable->gt_info;
  TimerTable->header.num_platform_timer = 0;

  gGtdtHdr = (EFI_ACPI_6_1_GENERIC_TIMER_DESCRIPTION_TABLE *) pal_get_gtdt_ptr();

  if (gGtdtHdr == NULL) {
    pal_print_msg(ACS_PRINT_ERR,
                  " GTDT not found\n");
    return;
  }
  gtdt_ptr = (UINT8 *)gGtdtHdr;
  pal_print_msg(ACS_PRINT_INFO,
                "  GTDT is at %x and length is %x\n",
                gGtdtHdr,
                gGtdtHdr->Header.Length);

  revision = gGtdtHdr->Header.Revision;
  pal_print_msg(ACS_PRINT_INFO,
                "  GTDT revision is at %d\n",
                revision);

  //Fill in our internal table
  TimerTable->header.s_el1_timer_flag  = gGtdtHdr->SecurePL1TimerFlags;
  TimerTable->header.ns_el1_timer_flag = gGtdtHdr->NonSecurePL1TimerFlags;
  TimerTable->header.el2_timer_flag    = gGtdtHdr->NonSecurePL2TimerFlags;
  TimerTable->header.s_el1_timer_gsiv  = gGtdtHdr->SecurePL1TimerGSIV;
  TimerTable->header.ns_el1_timer_gsiv = gGtdtHdr->NonSecurePL1TimerGSIV;
  TimerTable->header.el2_timer_gsiv    = gGtdtHdr->NonSecurePL2TimerGSIV;
  TimerTable->header.virtual_timer_flag = gGtdtHdr->VirtualTimerFlags;
  TimerTable->header.virtual_timer_gsiv = gGtdtHdr->VirtualTimerGSIV;

  /* If table Rev is  0x01 it will have above information only */
  if (revision < 2)
      return;

  Offset         = gGtdtHdr->PlatformTimerOffset;
  Entry          = (EFI_ACPI_6_1_GTDT_GT_BLOCK_STRUCTURE *) ((UINT8 *)gGtdtHdr + Offset);
  num_of_entries = gGtdtHdr->PlatformTimerCount;

  while(num_of_entries) {

    if (Entry->Type == EFI_ACPI_6_1_GTDT_GT_BLOCK) {
      pal_print_msg(ACS_PRINT_INFO,
                    "  Found block entry\n");
      GtEntry->type = TIMER_TYPE_SYS_TIMER;
      GtEntry->block_cntl_base = Entry->CntCtlBase;
      GtEntry->timer_count     = Entry->GTBlockTimerCount;
      pal_print_msg(ACS_PRINT_DEBUG,
                    "  CNTCTLBase = %llx\n",
                    GtEntry->block_cntl_base);
      GtBlockTimer = (EFI_ACPI_6_1_GTDT_GT_BLOCK_TIMER_STRUCTURE *)
                        (((UINT8 *)Entry) + Entry->GTBlockTimerOffset);
      for (i = 0; i < GtEntry->timer_count; i++) {
        pal_print_msg(ACS_PRINT_INFO,
                      "  Found timer entry\n");
        GtEntry->frame_num[i]    = GtBlockTimer->GTFrameNumber;
        GtEntry->GtCntBase[i]    = GtBlockTimer->CntBaseX;
        GtEntry->GtCntEl0Base[i] = GtBlockTimer->CntEL0BaseX;
        GtEntry->gsiv[i]         = GtBlockTimer->GTxPhysicalTimerGSIV;
        GtEntry->virt_gsiv[i]    = GtBlockTimer->GTxVirtualTimerGSIV;
        GtEntry->flags[i]        = GtBlockTimer->GTxPhysicalTimerFlags |
                                   (GtBlockTimer->GTxVirtualTimerFlags << 8) |
                                   (GtBlockTimer->GTxCommonFlags << 16);
        pal_print_msg(ACS_PRINT_DEBUG,
                      "  CNTBaseN = %llx for sys counter = %d\n",
                      GtEntry->GtCntBase[i],
                      i);
        GtBlockTimer++;
        TimerTable->header.num_platform_timer++;
      }
      GtEntry++;
    }

    Entry = (EFI_ACPI_6_1_GTDT_GT_BLOCK_STRUCTURE *) ((UINT8 *)Entry + (Entry->Length));
    num_of_entries--;

  };

  if (revision == 3) {
      el2_virt_timer_offset = (UINTN)((UINT8 *)&gGtdtHdr->PlatformTimerOffset -
                              gtdt_ptr) + sizeof(UINT32);
      if (gGtdtHdr->Header.Length >= (el2_virt_timer_offset + (2 * sizeof(UINT32)))) {
        CopyMem(&TimerTable->header.el2_virt_timer_gsiv,
            gtdt_ptr + el2_virt_timer_offset,
            sizeof(UINT32));
        CopyMem(&TimerTable->header.el2_virt_timer_flag,
            gtdt_ptr + el2_virt_timer_offset + sizeof(UINT32),
            sizeof(UINT32));
      }
      if (TimerTable->header.el2_virt_timer_gsiv == 0)
         pal_print_msg(ACS_PRINT_DEBUG,
                       "  GTDT don't have el2 virt timer info\n");
  }
  else
      pal_timer_platform_override(TimerTable);
}

/**
  @brief This API overrides the watch dog timer specified by WdTable
         Note: Only one watchdog information can be assigned as an override

  @param WdTable Pointer to watch dog timer info table

  @return None
**/
VOID
pal_wd_platform_override(WD_INFO_TABLE *WdTable)
{

  if (PLATFORM_OVERRIDE_WD == 1) {
      WdTable->header.num_wd              = 1;
      WdTable->wd_info[0].wd_refresh_base = PLATFORM_OVERRIDE_WD_REFRESH_BASE;
      WdTable->wd_info[0].wd_ctrl_base    = PLATFORM_OVERRIDE_WD_CTRL_BASE;
      WdTable->wd_info[0].wd_gsiv         = PLATFORM_OVERRIDE_WD_GSIV;
      WdTable->wd_info[0].wd_flags        = 0;
  }

  return;
}

/**
  @brief  This API fills in the WD_INFO_TABLE with information about Watchdogs
          in the system. This is achieved by parsing the ACPI - GTDT table.

  @param  WdTable  - Address where the Timer information needs to be filled.

  @return  None
**/

VOID
pal_wd_create_info_table(WD_INFO_TABLE *WdTable)
{

  EFI_ACPI_6_1_GTDT_SBSA_GENERIC_WATCHDOG_STRUCTURE    *Entry = NULL;
  WD_INFO_BLOCK               *WdEntry = NULL;
  UINT32                      Length= 0;
  UINT32                      num_of_entries;

  if (WdTable == NULL) {
    pal_print_msg(ACS_PRINT_ERR,
                  " Input Watchdog Table Pointer is NULL. Cannot create Watchdog INFO\n");
    return;
  }

  WdEntry = WdTable->wd_info;
  WdTable->header.num_wd = 0;
  gGtdtHdr = (EFI_ACPI_6_1_GENERIC_TIMER_DESCRIPTION_TABLE *) pal_get_gtdt_ptr();

  if (gGtdtHdr == NULL) {
    pal_print_msg(ACS_PRINT_ERR,
                  " GTDT not found\n");
    return;
  }

  Length         = gGtdtHdr->PlatformTimerOffset;
  Entry          = (EFI_ACPI_6_1_GTDT_SBSA_GENERIC_WATCHDOG_STRUCTURE *)
                    ((UINT8 *)gGtdtHdr + Length);
  Length         = sizeof (EFI_ACPI_6_1_GENERIC_TIMER_DESCRIPTION_TABLE);
  num_of_entries = gGtdtHdr->PlatformTimerCount;

  while(num_of_entries)
  {

    if (Entry->Type == EFI_ACPI_6_1_GTDT_GT_BLOCK) {
      //Skip. this info is added in the timer info function
    }

    if (Entry->Type == EFI_ACPI_6_1_GTDT_SBSA_GENERIC_WATCHDOG) {
      WdEntry->wd_refresh_base = Entry->RefreshFramePhysicalAddress;
      WdEntry->wd_ctrl_base    = Entry->WatchdogControlFramePhysicalAddress;
      WdEntry->wd_gsiv         = Entry->WatchdogTimerGSIV;
      WdEntry->wd_flags        = Entry->WatchdogTimerFlags;
      WdTable->header.num_wd++;
      pal_print_msg(ACS_PRINT_DEBUG,
                    "  Watchdog base = 0x%llx INTID = 0x%x\n",
                    WdEntry->wd_ctrl_base,
                    WdEntry->wd_gsiv);
      WdEntry++;
    }
    Entry = (EFI_ACPI_6_1_GTDT_SBSA_GENERIC_WATCHDOG_STRUCTURE *)
              ((UINT8 *)Entry + (Entry->Length));
    num_of_entries--;

  }

  pal_wd_platform_override(WdTable);

}

/**
  @brief  This API gets the counter frequency value from user

  @param  None

  @return Counter frequency value
**/

UINT64
pal_timer_get_counter_frequency(VOID)
{
  return PLATFORM_OVERRIDE_TIMER_CNTFRQ;
}
