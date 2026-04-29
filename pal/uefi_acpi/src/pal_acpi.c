/** @file
 * Copyright (c) 2016-2018, 2020-2021,2023-2026, Arm Limited or its affiliates. All rights reserved.
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
 * limitations under the  License.
**/
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ShellLib.h>
#include "Include/IndustryStandard/Acpi61.h"
#include "Include/IndustryStandard/ArmErrorSourceTable.h"

#include "Include/Guid/Acpi.h"
#include <Protocol/AcpiTable.h>
#include "Include/IndustryStandard/Acpi61.h"

#include "pal_uefi.h"
#include "pal_pmu.h"
#include "pal_mpam.h"

UINT64
pal_get_madt_ptr();

STATIC ACPI_ROOT_BRIDGE_INFO g_root_bridges[ACPI_MAX_ROOT_BRIDGES];
STATIC UINT32 g_root_bridge_count;
STATIC UINT32 g_root_bridge_parsed;

UINT32
pal_target_is_bm()
{
  return 0;
}

/**
  @brief Dump DTB to file

  @param None

  @return None
**/
VOID
pal_dump_dtb()
{
  pal_print_msg(ACS_PRINT_ERR,
                " DTB dump not available for platform initialized with ACPI table\n");
}

/**
  @brief   Checks if System information is passed using Device Tree (DT)
           This api is also used to check if GIC/Interrupt Init ACS Code
           is used or not. In case of DT, ACS Code is used for INIT

  @param  None

  @return True/False
*/
UINT32
pal_target_is_dt()
{
  return 0;
}

/**
  @brief   Use UEFI System Table to look up Acpi20TableGuid and returns the Xsdt Address

  @param  None

  @return Returns 64-bit XSDT address
 */
UINT64
pal_get_xsdt_ptr()
{
  EFI_ACPI_6_1_ROOT_SYSTEM_DESCRIPTION_POINTER *Rsdp;
  UINT32                        Index;

  for (Index = 0, Rsdp = NULL; Index < gST->NumberOfTableEntries; Index++) {
    if (CompareGuid (&(gST->ConfigurationTable[Index].VendorGuid), &gEfiAcpiTableGuid) ||
      CompareGuid (&(gST->ConfigurationTable[Index].VendorGuid), &gEfiAcpi20TableGuid)
      ) {
      // A match was found.
      Rsdp = (EFI_ACPI_6_1_ROOT_SYSTEM_DESCRIPTION_POINTER *) gST->ConfigurationTable[Index].VendorTable;
      break;
    }
  }
  if (Rsdp == NULL) {
      return 0;
  } else {
      return((UINT64) (EFI_ACPI_6_1_ROOT_SYSTEM_DESCRIPTION_POINTER *) Rsdp->XsdtAddress);
  }

}

/**
  @brief  Iterate through the tables pointed by XSDT and return MADT address

  @param  None

  @return 64-bit MADT address
**/
UINT64
pal_get_madt_ptr()
{

  EFI_ACPI_DESCRIPTION_HEADER   *Xsdt;
  UINT64                        *Entry64;
  UINT32                        Entry64Num;
  UINT32                        Idx;

  Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *) pal_get_xsdt_ptr();
  if (Xsdt == NULL) {
      pal_print_msg(ACS_PRINT_ERR,
                    " XSDT not found\n");
      return 0;
  }

  Entry64  = (UINT64 *)(Xsdt + 1);
  Entry64Num = (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) >> 3;
  for (Idx = 0; Idx < Entry64Num; Idx++) {
    if (*(UINT32 *)(UINTN)(Entry64[Idx]) == EFI_ACPI_6_1_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE) {
        return(UINT64)(Entry64[Idx]);
    }
  }

  return 0;

}

/**
  @brief  Iterate through the tables pointed by XSDT and return GTDT address

  @param  None

  @return 64-bit GTDT address
**/
UINT64
pal_get_gtdt_ptr()
{

  EFI_ACPI_DESCRIPTION_HEADER   *Xsdt;
  UINT64                        *Entry64;
  UINT32                        Entry64Num;
  UINT32                        Idx;

  Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *) pal_get_xsdt_ptr();
  if (Xsdt == NULL) {
      pal_print_msg(ACS_PRINT_ERR,
                    " XSDT not found\n");
      return 0;
  }

  Entry64  = (UINT64 *)(Xsdt + 1);
  Entry64Num = (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) >> 3;
  for (Idx = 0; Idx < Entry64Num; Idx++) {
    if (*(UINT32 *)(UINTN)(Entry64[Idx]) == EFI_ACPI_6_1_GENERIC_TIMER_DESCRIPTION_TABLE_SIGNATURE) {
        return(UINT64)(Entry64[Idx]);
    }
  }

  return 0;

}

/**
  @brief  Iterate through the tables pointed by XSDT and return MCFG Table address

  @param  None

  @return 64-bit MCFG address
**/
UINT64
pal_get_mcfg_ptr()
{
  EFI_ACPI_DESCRIPTION_HEADER   *Xsdt;
  UINT64                        *Entry64;
  UINT32                        Entry64Num;
  UINT32                        Idx;

  Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *) pal_get_xsdt_ptr();
  if (Xsdt == NULL) {
      pal_print_msg(ACS_PRINT_ERR,
                    " XSDT not found\n");
      return 0;
  }

  Entry64  = (UINT64 *)(Xsdt + 1);
  Entry64Num = (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) >> 3;
  for (Idx = 0; Idx < Entry64Num; Idx++) {
    if (*(UINT32 *)(UINTN)(Entry64[Idx]) == EFI_ACPI_6_1_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE) {
        return(UINT64)(Entry64[Idx]);
    }
  }

  return 0;

}

/**
  @brief  Iterate through the tables pointed by XSDT and return SPCR Table address

  @param  None

  @return 64-bit SPCR address
**/
UINT64
pal_get_spcr_ptr()
{
  EFI_ACPI_DESCRIPTION_HEADER   *Xsdt;
  UINT64                        *Entry64;
  UINT32                        Entry64Num;
  UINT32                        Idx;

  Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *) pal_get_xsdt_ptr();
  if (Xsdt == NULL) {
      pal_print_msg(ACS_PRINT_ERR,
                    " XSDT not found\n");
      return 0;
  }

  Entry64  = (UINT64 *)(Xsdt + 1);
  Entry64Num = (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) >> 3;
  for (Idx = 0; Idx < Entry64Num; Idx++) {
    if (*(UINT32 *)(UINTN)(Entry64[Idx]) == EFI_ACPI_2_0_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE) {
        return(UINT64)(Entry64[Idx]);
    }
  }

  return 0;

}

/**
  @brief  Iterate through the tables pointed by XSDT and return IORT Table address

  @param  None

  @return 64-bit IORT address
**/
UINT64
pal_get_iort_ptr()
{
  EFI_ACPI_DESCRIPTION_HEADER   *Xsdt;
  UINT64                        *Entry64;
  UINT32                        Entry64Num;
  UINT32                        Idx;

  Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *) pal_get_xsdt_ptr();
  if (Xsdt == NULL) {
      pal_print_msg(ACS_PRINT_ERR,
                    " XSDT not found\n");
      return 0;
  }

  Entry64  = (UINT64 *)(Xsdt + 1);
  Entry64Num = (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) >> 3;
  for (Idx = 0; Idx < Entry64Num; Idx++) {
#ifdef EFI_ACPI_6_1_IO_REMAPPING_TABLE_SIGNATURE
    if (*(UINT32 *)(UINTN)(Entry64[Idx]) == EFI_ACPI_6_1_IO_REMAPPING_TABLE_SIGNATURE) {
#else
    if (*(UINT32 *)(UINTN)(Entry64[Idx]) == EFI_ACPI_6_1_INTERRUPT_SOURCE_OVERRIDE_SIGNATURE) {
#endif
        return(UINT64)(Entry64[Idx]);
    }
  }

  return 0;

}

/**
  @brief   Iterate through the tables pointed by XSDT and return FADT Table address
  @param   None
  @return  64-bit address of FADT table
  @retval  0:  FADT table could not be found
**/
UINT64
pal_get_fadt_ptr (
  VOID
  )
{
  EFI_ACPI_DESCRIPTION_HEADER   *Xsdt;
  UINT64                        *Entry64;
  UINT32                        Entry64Num;
  UINT32                        Idx;

  Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *) pal_get_xsdt_ptr();
  if (Xsdt == NULL) {
      pal_print_msg(ACS_PRINT_ERR,
                    " XSDT not found\n");
      return 0;
  }

  Entry64 = (UINT64 *)(Xsdt + 1);
  Entry64Num = (Xsdt->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) >> 3;
  for (Idx = 0; Idx < Entry64Num; Idx++) {
    if (*(UINT32 *)(UINTN)(Entry64[Idx]) == EFI_ACPI_6_1_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) {
      return (UINT64)(Entry64[Idx]);
    }
  }

  return 0;
}

/**
  @brief  Iterate through the ACPI tables pointed by XSDT and return table address.

  @param  table_signature Signature of the requested ACPI table.

  @return 64-bit ACPI table address if found, else zero is returned.
**/
UINT64
pal_get_acpi_table_ptr(UINT32 table_signature)
{

  EFI_ACPI_DESCRIPTION_HEADER   *Xsdt;
  UINT64                        *Entry64;
  UINT32                        Entry64Num;
  UINT32                        Idx;

  Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *) pal_get_xsdt_ptr();
  if (Xsdt == NULL) {
      pal_print_msg(ACS_PRINT_ERR,
                    " XSDT not found\n");
      return 0;
  }

  Entry64  = (UINT64 *)(Xsdt + 1);
  Entry64Num = (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) >> 3;
  for (Idx = 0; Idx < Entry64Num; Idx++) {
    if (*(UINT32 *)(UINTN)(Entry64[Idx]) == table_signature) {
        return(UINT64)(Entry64[Idx]);
    }
  }

  return 0;
}

/**
    @brief  Iterate through the tables pointed by XSDT and return AEST Table address

    @param  None

    @return 64-bit AEST address
  **/
UINT64
pal_get_aest_ptr()
{
  EFI_ACPI_DESCRIPTION_HEADER   *Xsdt;
  UINT64                        *Entry64;
  UINT32                        Entry64Num;
  UINT32                        Idx;

  Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *) pal_get_xsdt_ptr();
  if (Xsdt == NULL) {
      pal_print_msg(ACS_PRINT_ERR,
                    " XSDT not found\n");
      return 0;
  }

  Entry64  = (UINT64 *)(Xsdt + 1);
  Entry64Num = (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) >> 3;
  for (Idx = 0; Idx < Entry64Num; Idx++) {
    if (*(UINT32 *)(UINTN)(Entry64[Idx]) == EFI_ACPI_6_3_ARM_ERROR_SOURCE_TABLE_SIGNATURE) {
      return(UINT64)(Entry64[Idx]);
    }
  }

  return 0;
}

  /**
    @brief  Iterate through the tables pointed by XSDT and return APMT Table address

    @param  None

    @return 64-bit APMT address
  **/
UINT64
pal_get_apmt_ptr()
{
  EFI_ACPI_DESCRIPTION_HEADER   *Xsdt;
  UINT64                        *Entry64;
  UINT32                        Entry64Num;
  UINT32                        Idx;

  Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *) pal_get_xsdt_ptr();
  if (Xsdt == NULL) {
      pal_print_msg(ACS_PRINT_ERR,
                    " XSDT not found\n");
      return 0;
  }

  Entry64  = (UINT64 *)(Xsdt + 1);
  Entry64Num = (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) >> 3;
  for (Idx = 0; Idx < Entry64Num; Idx++) {
    if (*(UINT32 *)(UINTN)(Entry64[Idx]) == ARM_PERFORMANCE_MONITORING_TABLE_SIGNATURE) {
      return(UINT64)(Entry64[Idx]);
    }
  }

  return 0;
}

/**
  @brief  Iterate through the tables pointed by XSDT and return HMAT address

  @param  None

  @return 64-bit HMAT address
**/
UINT64
pal_get_hmat_ptr(void)
{

  EFI_ACPI_DESCRIPTION_HEADER   *Xsdt;
  UINT64                        *Entry64;
  UINT32                        Entry64Num;
  UINT32                        Idx;

  Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *) pal_get_xsdt_ptr();
  if (Xsdt == NULL) {
      pal_print_msg(ACS_PRINT_ERR,
                    " XSDT not found\n");
      return 0;
  }

  Entry64  = (UINT64 *)(Xsdt + 1);
  Entry64Num = (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) >> 3;
  for (Idx = 0; Idx < Entry64Num; Idx++) {
    if (*(UINT32 *)(UINTN)(Entry64[Idx]) ==
        EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_SIGNATURE) {
        return(UINT64)(Entry64[Idx]);
    }
  }

  return 0;
}

  /**
    @brief  Iterate through the tables pointed by XSDT and return MPAM Table address

    @param  None

    @return 64-bit MPAM address
  **/
UINT64
pal_get_mpam_ptr()
{
  EFI_ACPI_DESCRIPTION_HEADER   *Xsdt;
  UINT64                        *Entry64;
  UINT32                        Entry64Num;
  UINT32                        Idx;

  Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *) pal_get_xsdt_ptr();
  if (Xsdt == NULL) {
      pal_print_msg(ACS_PRINT_ERR,
                    " XSDT not found\n");
      return 0;
  }

  Entry64  = (UINT64 *)(Xsdt + 1);
  Entry64Num = (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) >> 3;
  for (Idx = 0; Idx < Entry64Num; Idx++) {
    if (*(UINT32 *)(UINTN)(Entry64[Idx]) ==
                MEMORY_RESOURCE_PARTITIONING_AND_MONITORING_TABLE_SIGNATURE) {
      return(UINT64)(Entry64[Idx]);
    }
  }

  return 0;
}

/**
  @brief  Iterate through the tables pointed by XSDT and return PPTT address

  @param  None

  @return 64-bit PPTT address
**/
UINT64
pal_get_pptt_ptr(void)
{

  EFI_ACPI_DESCRIPTION_HEADER   *Xsdt;
  UINT64                        *Entry64;
  UINT32                        Entry64Num;
  UINT32                        Idx;

  Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *) pal_get_xsdt_ptr();
  if (Xsdt == NULL) {
      pal_print_msg(ACS_PRINT_ERR,
                    " XSDT not found\n");
      return 0;
  }

  Entry64  = (UINT64 *)(Xsdt + 1);
  Entry64Num = (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) >> 3;
  for (Idx = 0; Idx < Entry64Num; Idx++) {
    if (*(UINT32 *)(UINTN)(Entry64[Idx]) ==
        EFI_ACPI_6_4_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_STRUCTURE_SIGNATURE) {
        return (UINT64)(Entry64[Idx]);
    }
  }

  return 0;
}

/**
  @brief  Iterate through the tables pointed by XSDT and return SRAT address

  @param  None

  @return 64-bit SRAT address
**/
UINT64
pal_get_srat_ptr(void)
{

  EFI_ACPI_DESCRIPTION_HEADER   *Xsdt;
  UINT64                        *Entry64;
  UINT32                        Entry64Num;
  UINT32                        Idx;

  Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *) pal_get_xsdt_ptr();
  if (Xsdt == NULL) {
      pal_print_msg(ACS_PRINT_ERR,
                    " XSDT not found\n");
      return 0;
  }

  Entry64  = (UINT64 *)(Xsdt + 1);
  Entry64Num = (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) >> 3;
  for (Idx = 0; Idx < Entry64Num; Idx++) {
    if (*(UINT32 *)(UINTN)(Entry64[Idx]) ==
        EFI_ACPI_3_0_SYSTEM_RESOURCE_AFFINITY_TABLE_SIGNATURE) {
        return(UINT64)(Entry64[Idx]);
    }
  }

  return 0;
}

/**
  @brief  Iterate through the tables pointed by XSDT and return TPM2 table address

  @param  None

  @return 64-bit TPM2 table address
**/
UINT64
pal_get_tpm2_ptr(void)
{
  EFI_ACPI_DESCRIPTION_HEADER   *Xsdt;
  UINT64                        *Entry64;
  UINT32                        Entry64Num;
  UINT32                        Idx;

  Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *) pal_get_xsdt_ptr();
  if (Xsdt == NULL) {
      pal_print_msg(ACS_PRINT_ERR,
                    " XSDT not found\n");
      return 0;
  }

  Entry64  = (UINT64 *)(Xsdt + 1);
  Entry64Num = (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) >> 3;

  for (Idx = 0; Idx < Entry64Num; Idx++) {
    if (*(UINT32 *)(UINTN)(Entry64[Idx]) ==
        EFI_ACPI_6_1_TRUSTED_COMPUTING_PLATFORM_2_TABLE_SIGNATURE) {
        return (UINT64)(Entry64[Idx]);
    }
  }

  return 0;
}

STATIC UINT32
pal_acpi_string_equal(CONST CHAR8 * left, CONST CHAR8 * right)
{
  UINT32 idx = 0u;

  if ((left == NULL) || (right == NULL))
    return 0u;

  while (left[idx] != '\0' || right[idx] != '\0') {
    if (left[idx] != right[idx])
      return 0u;
    idx++;
  }

  return 1u;
}

STATIC UINT32
pal_acpi_parse_numeric_string(CONST CHAR8 * text, UINT32 *value_out)
{
  UINT32 value = 0u;
  UINT32 base = 10u;
  UINT32 idx = 0u;

  if ((text == NULL) || (value_out == NULL) || (text[0] == '\0'))
    return 0u;

  if ((text[0] == '0') && ((text[1] == 'x') || (text[1] == 'X'))) {
    base = 16u;
    idx = 2u;
  }

  for (; text[idx] != '\0'; idx++) {
    UINT32 digit;

    if ((text[idx] >= '0') && (text[idx] <= '9'))
      digit = (UINT32)(text[idx] - '0');
    else if ((base == 16u) && (text[idx] >= 'a') && (text[idx] <= 'f'))
      digit = (UINT32)(text[idx] - 'a') + 10u;
    else if ((base == 16u) && (text[idx] >= 'A') && (text[idx] <= 'F'))
      digit = (UINT32)(text[idx] - 'A') + 10u;
    else
      return 0u;

    value = (value * base) + digit;
  }

  *value_out = value;
  return 1u;
}

STATIC VOID
pal_acpi_decode_eisa_id(UINT32 encoded, CHAR8 * out, UINT32 out_len)
{
  /* Decode EISA IDs (PNPxxxx) from numeric _HID values. */
  if ((out == NULL) || (out_len < 8u))
    return;

  out[0] = (CHAR8)(((encoded >> 26) & 0x1Fu) + 'A' - 1);
  out[1] = (CHAR8)(((encoded >> 21) & 0x1Fu) + 'A' - 1);
  out[2] = (CHAR8)(((encoded >> 16) & 0x1Fu) + 'A' - 1);

  UINT16 product = (UINT16)(encoded & 0xFFFFu);
  for (UINT32 idx = 0; idx < 4u; idx++) {
    UINT8 nibble = (UINT8)((product >> ((3u - idx) * 4u)) & 0xFu);
    out[3u + idx] = (CHAR8)(nibble < 10u ? ('0' + nibble) : ('A' + nibble - 10u));
  }

  out[7] = '\0';
}

STATIC UINT32
pal_acpi_is_pci_hid(PAL_AML_DATA_TYPE data_type, UINT64 value, CONST CHAR8 * text)
{
  /* Accept PCI root bridge HIDs expressed as strings or numeric EISA IDs. */
  if (data_type == AML_DATA_STRING) {
    if (pal_acpi_string_equal(text, "PNP0A08"))
      return 1u;
    if (pal_acpi_string_equal(text, "PNP0A03"))
      return 1u;
    return 0u;
  }

  if (data_type == AML_DATA_INTEGER) {
    CHAR8 decoded[8];

    SetMem(decoded, sizeof(decoded), 0);
    pal_acpi_decode_eisa_id((UINT32)value, decoded, sizeof(decoded));
    if (pal_acpi_string_equal(decoded, "PNP0A08"))
      return 1u;
    if (pal_acpi_string_equal(decoded, "PNP0A03"))
      return 1u;
  }

  return 0u;
}

STATIC UINT32
pal_acpi_parse_pkg_length(CONST UINT8 *data,
                          UINT32 length,
                          UINT32 *pkg_length,
                          UINT32 *consumed)
{
  /* Decode AML PkgLength encoding for scoped objects. */
  UINT8 lead;
  UINT32 byte_count;
  UINT32 value;

  if ((data == NULL) || (pkg_length == NULL) || (consumed == NULL) || (length == 0u))
    return 0u;

  lead = data[0];
  byte_count = (UINT32)(lead >> 6);
  if (byte_count > 3u)
    return 0u;

  if ((1u + byte_count) > length)
    return 0u;

  if (byte_count == 0u)
    value = lead & 0x3Fu;
  else
    value = lead & 0x0Fu;
  for (UINT32 idx = 0; idx < byte_count; idx++)
    value |= (UINT32)data[1u + idx] << (4u + (idx * 8u));

  *pkg_length = value;
  *consumed = 1u + byte_count;
  return 1u;
}

STATIC UINT32
pal_acpi_parse_name_string(CONST UINT8 *data, UINT32 length, CHAR8 name[5])
{
  /* Parse AML NameString and return the final 4-character segment. */
  UINT32 offset = 0u;

  if ((data == NULL) || (length == 0u))
    return 0u;

  while ((offset < length) &&
         ((data[offset] == AML_NAME_ROOT) || (data[offset] == AML_NAME_PARENT)))
    offset++;

  if (offset >= length)
    return 0u;

  if (data[offset] == AML_NAME_NULL) {
    if (name != NULL)
      name[0] = '\0';
    return offset + 1u;
  }

  if (data[offset] == AML_NAME_DUAL) {
    if ((offset + 1u + 8u) > length)
      return 0u;
    if (name != NULL) {
      CopyMem(name, &data[offset + 1u + 4u], 4u);
      name[4] = '\0';
    }
    return offset + 1u + 8u;
  }

  if (data[offset] == AML_NAME_MULTI) {
    UINT32 count;

    if ((offset + 1u) >= length)
      return 0u;

    count = data[offset + 1u];
    if ((offset + 2u + (count * 4u)) > length)
      return 0u;

    if ((name != NULL) && (count > 0u)) {
      UINT32 last = offset + 2u + ((count - 1u) * 4u);
      CopyMem(name, &data[last], 4u);
      name[4] = '\0';
    }

    return offset + 2u + (count * 4u);
  }

  if ((offset + 4u) > length)
    return 0u;

  if (name != NULL) {
    CopyMem(name, &data[offset], 4u);
    name[4] = '\0';
  }

  return offset + 4u;
}

STATIC UINT32
pal_acpi_parse_data_object(CONST UINT8 *data,
                           UINT32 length,
                           PAL_AML_DATA_TYPE *type,
                           UINT64 *value,
                           CHAR8 * text,
                           UINT32 text_len,
                           UINT32 *consumed)
{
  /* Parse a limited AML data object (integer or string). */
  UINT8 op;

  if ((data == NULL) || (length == 0u) || (type == NULL) || (consumed == NULL))
    return 0u;

  op = data[0];
  *type = AML_DATA_NONE;
  *consumed = 0u;

  switch (op) {
  case AML_OP_ZERO:
    *type = AML_DATA_INTEGER;
    if (value != NULL)
      *value = 0u;
    *consumed = 1u;
    return 1u;
  case AML_OP_ONE:
    *type = AML_DATA_INTEGER;
    if (value != NULL)
      *value = 1u;
    *consumed = 1u;
    return 1u;
  case AML_OP_BYTE:
    if (length < 2u)
      return 0u;
    *type = AML_DATA_INTEGER;
    if (value != NULL)
      *value = data[1];
    *consumed = 2u;
    return 1u;
  case AML_OP_WORD:
    if (length < 3u)
      return 0u;
    *type = AML_DATA_INTEGER;
    if (value != NULL)
      *value = (UINT16)(data[1] | (data[2] << 8));
    *consumed = 3u;
    return 1u;
  case AML_OP_DWORD:
    if (length < 5u)
      return 0u;
    *type = AML_DATA_INTEGER;
    if (value != NULL)
      *value = (UINT32)(data[1] | (data[2] << 8) |
                        (data[3] << 16) | (data[4] << 24));
    *consumed = 5u;
    return 1u;
  case AML_OP_QWORD:
    if (length < 9u)
      return 0u;
    *type = AML_DATA_INTEGER;
    if (value != NULL) {
      *value = ((UINT64)data[1]) |
               ((UINT64)data[2] << 8) |
               ((UINT64)data[3] << 16) |
               ((UINT64)data[4] << 24) |
               ((UINT64)data[5] << 32) |
               ((UINT64)data[6] << 40) |
               ((UINT64)data[7] << 48) |
               ((UINT64)data[8] << 56);
    }
    *consumed = 9u;
    return 1u;
  case AML_OP_STRING: {
    UINT32 text_index = 0u;

    *type = AML_DATA_STRING;
    if (text != NULL)
      text[0] = '\0';
    for (UINT32 idx = 1u; idx < length; idx++) {
      if (data[idx] == '\0') {
        if ((text != NULL) && (text_len > 0u))
          text[text_index] = '\0';
        *consumed = idx + 1u;
        return 1u;
      }
      if ((text != NULL) && (text_len > 1u) && (text_index < text_len - 1u)) {
        text[text_index] = (CHAR8)data[idx];
        text_index++;
      }
    }
    return 0u;
  }
  default:
    return 0u;
  }
}

STATIC VOID
pal_acpi_parse_aml(CONST UINT8 *aml, UINT32 length)
{
  /* Walk AML to collect PCI root bridge _HID/_UID/_SEG/_BBN data. */
  AML_DEVICE_SCOPE stack[PAL_AML_MAX_DEVICE_DEPTH];
  INT32 depth = -1;
  UINT32 offset = 0u;

  while (offset < length) {
    while ((depth >= 0) && (offset >= stack[depth].end_offset)) {
      if ((stack[depth].info.hid_is_pci != 0u) &&
          (stack[depth].info.uid_valid != 0u) &&
          (g_root_bridge_count < ACPI_MAX_ROOT_BRIDGES)) {
        g_root_bridges[g_root_bridge_count++] = stack[depth].info;
      }
      depth--;
    }

    if ((offset + 1u) < length &&
        (aml[offset] == AML_OP_DEVICE_PREFIX) &&
        (aml[offset + 1u] == AML_OP_DEVICE)) {
      UINT32 pkg_length;
      UINT32 pkg_consumed;
      UINT32 name_consumed;
      UINT32 body_start;
      UINT32 end_offset;
      CHAR8 device_name[5];

      offset += 2u;
      if (!pal_acpi_parse_pkg_length(&aml[offset], length - offset,
                                     &pkg_length, &pkg_consumed))
        break;

      body_start = offset + pkg_consumed;
      end_offset = body_start + pkg_length;
      if (end_offset > length)
        break;

      if (depth + 1 >= (INT32)PAL_AML_MAX_DEVICE_DEPTH) {
        offset = end_offset;
        continue;
      }

      depth++;
      SetMem(&stack[depth], sizeof(stack[depth]), 0);
      stack[depth].end_offset = end_offset;
      stack[depth].info.segment = 0u;
      stack[depth].info.bbn = 0u;
      stack[depth].info.uid_valid = 0u;

      SetMem(device_name, sizeof(device_name), 0);
      name_consumed =
        pal_acpi_parse_name_string(&aml[body_start], end_offset - body_start,
                                   device_name);
      if (name_consumed == 0u) {
        offset = end_offset;
        continue;
      }

      offset = body_start + name_consumed;
      continue;
    }

    if (aml[offset] == AML_OP_NAME) {
      CHAR8 name[5];
      UINT32 name_consumed;
      UINT32 data_consumed;
      PAL_AML_DATA_TYPE data_type;
      UINT64 data_value = 0u;
      CHAR8 data_text[16];

      offset += 1u;
      name_consumed = pal_acpi_parse_name_string(&aml[offset], length - offset, name);
      if (name_consumed == 0u)
        break;

      offset += name_consumed;
      SetMem(data_text, sizeof(data_text), 0);
      if (!pal_acpi_parse_data_object(&aml[offset], length - offset, &data_type,
                                      &data_value, data_text, sizeof(data_text),
                                      &data_consumed)) {
        offset++;
        continue;
      }

      if (depth >= 0) {
        /* Only record attributes within the current Device scope. */
        if (pal_acpi_string_equal(name, "_HID")) {
          stack[depth].info.hid_is_pci =
            pal_acpi_is_pci_hid(data_type, data_value, data_text);
        } else if (pal_acpi_string_equal(name, "_UID")) {
          UINT32 uid_value;

          if (data_type == AML_DATA_INTEGER) {
            stack[depth].info.uid = (UINT32)data_value;
            stack[depth].info.uid_valid = 1u;
          } else if (data_type == AML_DATA_STRING) {
            if (pal_acpi_parse_numeric_string(data_text, &uid_value)) {
              stack[depth].info.uid = uid_value;
              stack[depth].info.uid_valid = 1u;
            }
          }
        } else if (pal_acpi_string_equal(name, "_SEG")) {
          UINT32 seg_value;

          if (data_type == AML_DATA_INTEGER) {
            stack[depth].info.segment = (UINT16)data_value;
          } else if ((data_type == AML_DATA_STRING) &&
                     pal_acpi_parse_numeric_string(data_text, &seg_value)) {
            stack[depth].info.segment = (UINT16)seg_value;
          }
        } else if (pal_acpi_string_equal(name, "_BBN")) {
          UINT32 bbn_value;

          if (data_type == AML_DATA_INTEGER) {
            stack[depth].info.bbn = (UINT8)data_value;
          } else if ((data_type == AML_DATA_STRING) &&
                     pal_acpi_parse_numeric_string(data_text, &bbn_value)) {
            stack[depth].info.bbn = (UINT8)bbn_value;
          }
        }
      }

      offset += data_consumed;
      continue;
    }

    offset++;
  }

  while (depth >= 0) {
    if ((stack[depth].info.hid_is_pci != 0u) &&
        (stack[depth].info.uid_valid != 0u) &&
        (g_root_bridge_count < ACPI_MAX_ROOT_BRIDGES)) {
      g_root_bridges[g_root_bridge_count++] = stack[depth].info;
    }
    depth--;
  }
}

VOID
pal_acpi_parse_root_bridges(VOID)
{
  /* Parse DSDT/SSDT AML once and cache PCI root bridge properties. */
  EFI_ACPI_DESCRIPTION_HEADER *xsdt;
  EFI_ACPI_DESCRIPTION_HEADER *fadt;
  EFI_ACPI_DESCRIPTION_HEADER *dsdt;
  EFI_ACPI_6_4_FIXED_ACPI_DESCRIPTION_TABLE *fadt_table;
  UINT64 *entries;
  UINT32 entry_count;

  if (g_root_bridge_parsed != 0u)
    return;

  g_root_bridge_parsed = 1u;
  g_root_bridge_count = 0u;
  SetMem(g_root_bridges, sizeof(g_root_bridges), 0);

  xsdt = (EFI_ACPI_DESCRIPTION_HEADER *)pal_get_xsdt_ptr();
  if ((xsdt == NULL) ||
      (xsdt->Signature != EFI_ACPI_6_1_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE)) {
    return;
  }

  fadt = (EFI_ACPI_DESCRIPTION_HEADER *)pal_get_fadt_ptr();
  if (fadt == NULL) {
    return;
  }

  fadt_table = (EFI_ACPI_6_4_FIXED_ACPI_DESCRIPTION_TABLE *)(VOID *)fadt;
  if (fadt_table->XDsdt != 0u)
    dsdt = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)fadt_table->XDsdt;
  else if (fadt_table->Dsdt != 0u)
    dsdt = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)fadt_table->Dsdt;
  else
  {
    pal_print_msg(ACS_PRINT_WARN,
                  " DSDT not found; root bridge AML unavailable ");
    dsdt = NULL;
  }

  if ((dsdt != NULL) && (dsdt->Length > sizeof(*dsdt))) {
    CONST UINT8 *aml = (CONST UINT8 *)(dsdt + 1);
    UINT32 aml_length = dsdt->Length - sizeof(*dsdt);

    pal_acpi_parse_aml(aml, aml_length);
  }

  if (xsdt->Length < sizeof(*xsdt)) {
    return;
  }

  entry_count = (xsdt->Length - sizeof(*xsdt)) / sizeof(UINT64);
  entries = (UINT64 *)(xsdt + 1);

  for (UINT32 idx = 0; idx < entry_count; idx++) {
    EFI_ACPI_DESCRIPTION_HEADER *table =
      (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)entries[idx];

    if ((table == NULL) ||
        (table->Signature != EFI_ACPI_6_1_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE))
      continue;
    if (table->Length <= sizeof(*table))
      continue;

    CONST UINT8 *aml = (CONST UINT8 *)(table + 1);
    UINT32 aml_length = table->Length - sizeof(*table);

    pal_acpi_parse_aml(aml, aml_length);
  }
}

/**
  @brief   Return the ACPI root bridge UID for the given segment and base bus.

  @param  segment  PCI segment number for the root bridge.
  @param  bbn      Base bus number from the root bridge.
  @param  uid      Output pointer for the root bridge UID.

  @return 0 on success, non-zero when mapping is unavailable.
**/
UINT32
pal_acpi_get_root_bridge_uid(UINT16 segment, UINT8 bbn, UINT32 *uid)
{
  if (uid == NULL) {
    pal_print_msg(ACS_PRINT_ERR,
                  " pal_acpi_get_root_bridge_uid UID pointer NULL ");
    return 1;
  }

  pal_acpi_parse_root_bridges();
  if (g_root_bridge_count == 0) {
    if ((segment == 0) && (bbn == 0)) {
      *uid = 0;
      return 0;
    }
    return 1u;
  }

  for (UINT32 idx = 0; idx < g_root_bridge_count; idx++) {
    if ((g_root_bridges[idx].segment != segment) ||
        (g_root_bridges[idx].bbn != bbn))
      continue;

    *uid = g_root_bridges[idx].uid;
    return 0;
  }

  return 1;
}
