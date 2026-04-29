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
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include "Include/IndustryStandard/Acpi64.h"
#include "pal_uefi.h"
#include "pcie_enum.h"

/**
  @brief   Incorporate CFMWS records into the CXL info table.

  @param  Cfmws     Pointer to a CXL fixed memory window structure.
  @param  CxlTable  CXL info table to update.
**/
static VOID
pal_cxl_apply_cfmws(const EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE *Cfmws,
                    CXL_INFO_TABLE                                          *CxlTable)
{
  UINT32 window_slot;
  UINT32 max_windows;
  UINT16 record_length;
  UINT32 header_length, target_count;
  UINT32 target_index, target_uid, entry_index;

  if ((Cfmws == NULL) || (CxlTable == NULL)) {
    return;
  }

  if (CxlTable->num_entries == 0) {
    return;
  }

  record_length = Cfmws->RecordLength;
  header_length = sizeof(*Cfmws);

  if (record_length < header_length) {
    return;
  }

  target_count = (record_length - header_length) / sizeof(UINT32);

  for (target_index = 0; target_index < target_count; target_index++) {
    target_uid = Cfmws->InterleaveTargetList[target_index];

    for (entry_index = 0; entry_index < CxlTable->num_entries; entry_index++) {
      CXL_INFO_BLOCK *entry = &CxlTable->device[entry_index];

      if (entry->uid != target_uid) {
        continue;
      }

      window_slot = entry->cfmws_count;
      max_windows = (UINT32)(sizeof(entry->cfmws_base) /
                                    sizeof(entry->cfmws_base[0]));
      if (window_slot < max_windows) {
        entry->cfmws_base[window_slot]   = Cfmws->BaseHpa;
        entry->cfmws_length[window_slot] = Cfmws->WindowSize;
        entry->cfmws_window[window_slot] = Cfmws->WindowRestrictions;
        entry->cfmws_count++;
      }
    }
  }
}

/**
  @brief   Build the platform CXL info table from ACPI CEDT data.
  @param  CxlTable  Caller-provided buffer to populate.
**/
VOID
pal_cxl_create_info_table(CXL_INFO_TABLE *CxlTable)
{

  EFI_ACPI_DESCRIPTION_HEADER *Cedt;
  UINT8  *ptr;
  UINT8  *end;
  UINT32 host_count = 0;
  UINT64 cedt_address;

  if (CxlTable == NULL) {
    pal_print_msg(ACS_PRINT_ERR,
                  " Input CXL Table Pointer is NULL. Cannot create CXL INFO ");
    return;
  }
  CxlTable->num_entries = 0;

  cedt_address = pal_get_acpi_table_ptr(EFI_ACPI_6_4_CXL_EARLY_DISCOVERY_TABLE_SIGNATURE);
  if (cedt_address == 0) {
    pal_print_msg(ACS_PRINT_ERR,
                  " ACPI - CEDT Table not found.\n");
    return;
  }

  Cedt = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)cedt_address;
  ptr  = (UINT8 *)(Cedt + 1);
  end  = ((UINT8 *)Cedt) + Cedt->Length;

  while ((ptr + sizeof(UINT8) + sizeof(UINT8) + sizeof(UINT16)) <= end) {
    EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_STRUCTURE *Host =
      (EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_STRUCTURE *)ptr;

    if (Host->RecordLength == 0) {
      pal_print_msg(ACS_PRINT_WARN,
                    " CEDT record length is zero. Aborting parse. ");
      return;
    }

    if ((ptr + Host->RecordLength) > end) {
      pal_print_msg(ACS_PRINT_WARN,
                    " CEDT record overruns table length. Stopping parse. ");
      break;
    }

    if ((Host->Type == EFI_ACPI_6_4_CEDT_STRUCTURE_TYPE_CXL_HOST_BRIDGE_STRUCTURE) &&
        (Host->RecordLength >= sizeof(EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_STRUCTURE))) {
      CXL_INFO_BLOCK *entry = &CxlTable->device[host_count];
      ZeroMem(entry, sizeof(*entry));

      entry->uid                  = Host->Uid;
      entry->cxl_version          = Host->CxlVersion;
      if (entry->cxl_version == 1)
          entry->component_reg_type  = CXL_COMPONENT_CHBCR;
      else
          entry->component_reg_type  = CXL_COMPONENT_RCRB;
      entry->component_reg_base   = Host->Base;
      entry->component_reg_length = Host->Length;
      entry->cxl_struct_type      = 0;
      host_count++;
    }
    ptr += Host->RecordLength;
  }
  CxlTable->num_entries = host_count;
  ptr = (UINT8 *)(Cedt + 1);
  while ((ptr + sizeof(UINT8) + sizeof(UINT16)) <= end) {
    UINT8  type = ptr[0];
    UINT16 len  = *(UINT16 *)(ptr + 2);

    if (len == 0) {
      pal_print_msg(ACS_PRINT_WARN,
                    " CEDT record length is zero. Aborting parse. ");
      return;
    }

    if ((ptr + len) > end) {
      pal_print_msg(ACS_PRINT_WARN,
                    " CEDT record overruns table length. Stopping parse. ");
      break;
    }

    if ((type == EFI_ACPI_6_4_CEDT_STRUCTURE_TYPE_CXL_FIXED_MEMORY_WINDOW_STRUCTURE) &&
        (len >= sizeof(EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE))) {
      pal_cxl_apply_cfmws((const EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE *)ptr,
                          CxlTable);
    }

    ptr += len;
  }
}

/**
  @brief   Return the CEDT host bridge UID for the given root port BDF.

  @param  bdf  Root port Bus/Device/Function identifier.
  @param  uid  Output pointer for the host bridge UID.

  @return 0 on success, non-zero when mapping is unavailable.
**/
UINT32
pal_cxl_get_host_bridge_uid(UINT32 bdf, UINT32 *uid)
{
  UINT16 segment;
  UINT8 bus;

  /* Map root port BDF to the root bridge UID for CEDT lookup. */
  if (uid == NULL) {
    pal_print_msg(ACS_PRINT_ERR,
                  " %a UID pointer NULL ",
                  __func__);
    return 1;
  }

  segment = PCIE_EXTRACT_BDF_SEG(bdf);
  bus = PCIE_EXTRACT_BDF_BUS(bdf);

  return pal_acpi_get_root_bridge_uid(segment, bus, uid);
}
