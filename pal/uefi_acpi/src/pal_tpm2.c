/** @file
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
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
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include "Include/IndustryStandard/ArmErrorSourceTable.h"
#include "Include/IndustryStandard/Tpm20.h"
#include "Include/IndustryStandard/Tpm2Acpi.h"
#include <Protocol/Tcg2Protocol.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#include "include/pal_uefi.h"

UINT64
pal_get_tpm2_ptr();

/**
  @brief  Fill TPM2 info table details by parsing TPM2 Table
  @param  TPM2InfoTable  - Address to the TPM2 information table.
  @return  None
**/

VOID
pal_tpm2_create_info_table(TPM2_INFO_TABLE * Tpm2InfoTable)
{

  EFI_STATUS Status;
  EFI_TCG2_PROTOCOL *Tcg2;
  EFI_TPM2_ACPI_TABLE *gtpm2ptr;
  EFI_TCG2_BOOT_SERVICE_CAPABILITY Capability;
  UINT64 TpmPresent, status_flag = 0;

  if (Tpm2InfoTable == NULL) {
      acs_print(ACS_PRINT_ERR, L" TPM2 Info Table pointer is NULL. Cannot create TPM2 info.\n", 0);
      return;
  }

  /* Initialize the TPM2 info table */
  Tpm2InfoTable->tpm_presence  = 0;
  Tpm2InfoTable->tpm_interface = 0;
  Tpm2InfoTable->base = 0;

  /* Locate TCG2 protocol */
  Status = gBS->LocateProtocol(&gEfiTcg2ProtocolGuid, NULL, (VOID **)&Tcg2);
  if (EFI_ERROR(Status)) {
      acs_print(ACS_PRINT_ERR, L" TCG2 Protocol Not Found\n", 0);
      status_flag = 1;
  }

  /* Get pointer to TPM2 ACPI table */
  gtpm2ptr = (EFI_TPM2_ACPI_TABLE *)pal_get_tpm2_ptr();
  if (gtpm2ptr == NULL) {
      acs_print(ACS_PRINT_ERR, L" TPM2 ACPI Table not found\n", 0);
      status_flag = 1;
  }

  if (status_flag == 1)
      return;

  /* Get TPM capability information */
  ZeroMem(&Capability, sizeof(Capability));
  Capability.Size = sizeof(EFI_TCG2_BOOT_SERVICE_CAPABILITY);
  Status = Tcg2->GetCapability(Tcg2, &Capability);
  if (EFI_ERROR(Status)) {
      acs_print(ACS_PRINT_ERR,
                L" Failed to retrieve TPM capability from TCG2 protocol, Status: 0x%x\n",
                Status);
      return;
  }

  /* Extract TPM presence flag */
  TpmPresent = (UINT64)Capability.TPMPresentFlag;
  Tpm2InfoTable->tpm_presence = TpmPresent;

  /* Populate TPM interface type and control area base address */
  Tpm2InfoTable->tpm_interface = gtpm2ptr->StartMethod;
  Tpm2InfoTable->base = gtpm2ptr->AddressOfControlArea;

  /* Log TPM presence and configuration */
  acs_print(ACS_PRINT_INFO, L" TPM2 Protocol Present\n", 0);
  acs_print(ACS_PRINT_TEST, L" TPM2 Interface Type (StartMethod): %llx\n",
                                                 Tpm2InfoTable->tpm_interface);
  acs_print(ACS_PRINT_TEST, L" TPM2 Base Address: %llx\n", Tpm2InfoTable->base);


  return;
}

/**
  @brief  Retrieve the TPM2 version by reading the TPM family indicator.
  @param  None
  @return TPM version as UINT64:
            2 - TPM 2.0
            1 - TPM 1.2
            0 - Unknown or unsupported
**/

UINT64
pal_tpm2_get_version()
{

  EFI_STATUS Status;
  EFI_TCG2_PROTOCOL *Tcg2;
  TPMS_CAPABILITY_DATA *cap_data;
  TPM2_GET_CAPABILITY_COMMAND   SendBuffer;
  TPM2_GET_CAPABILITY_RESPONSE  RecvBuffer;
  UINT32 PropertyCount;
  UINT32 SendBufferSize;
  UINT32 RecvBufferSize;
  UINT32 TempBufferSize = 4096;  /* Allocate 4KB for receiving capability data */
  VOID *TempBuffer;

  /* Locate the TCG2 protocol */
  Status = gBS->LocateProtocol(&gEfiTcg2ProtocolGuid, NULL, (VOID **)&Tcg2);
  if (EFI_ERROR(Status)) {
      acs_print(ACS_PRINT_ERR, L"       TCG2 Protocol Not Found\n", 0);
      return Status;
  }

  /* Allocate temporary buffer for capability data */
  Status = gBS->AllocatePool(EfiBootServicesData, TempBufferSize, (VOID **)&TempBuffer);
  if (EFI_ERROR(Status)) {
      acs_print(ACS_PRINT_ERR, L"       Failed to allocate memory for TPM capability data.\n", 0);
      return Status;
  }

  SetMem(TempBuffer, TempBufferSize, 0);
  cap_data = (TPMS_CAPABILITY_DATA *)TempBuffer;


  /* Construct TPM2_GetCapability command to query TPM_PT_FAMILY_INDICATOR */
  SendBuffer.Header.tag         = SwapBytes16(TPM_ST_NO_SESSIONS);
  SendBuffer.Header.commandCode = SwapBytes32(TPM_CC_GetCapability);
  SendBuffer.Capability         = SwapBytes32(TPM_CAP_TPM_PROPERTIES);
  SendBuffer.Property           = SwapBytes32(TPM_PT_FAMILY_INDICATOR);
  SendBuffer.PropertyCount      = SwapBytes32 (1);
  SendBufferSize                = (UINT32)sizeof(SendBuffer);
  SendBuffer.Header.paramSize   = SwapBytes32 (SendBufferSize);

  RecvBufferSize = sizeof(RecvBuffer);

  /* Submit the TPM command */
  Status = Tcg2->SubmitCommand(Tcg2, SendBufferSize, (UINT8 *)&SendBuffer, RecvBufferSize,
                              (UINT8 *)&RecvBuffer);

  if (EFI_ERROR(Status)) {
      acs_print(ACS_PRINT_ERR, L"       TPM2 command submission failed\n", 0);
      return Status;
  }

  /* Check response status */
  if (SwapBytes32 (RecvBuffer.Header.responseCode) != TPM_RC_SUCCESS) {
      acs_print(ACS_PRINT_ERR, L"       Tpm2GetCapability failed: Response Code error! 0x%08x\n",
             SwapBytes32 (RecvBuffer.Header.responseCode));
      return Status;
  }

  /* Copy TPM capability data from the response buffer */
  CopyMem(cap_data, &RecvBuffer.CapabilityData,
      RecvBufferSize - sizeof(TPM2_RESPONSE_HEADER) - sizeof(UINT8));


  PropertyCount = SwapBytes32(cap_data->data.tpmProperties.count);
  acs_print(ACS_PRINT_INFO, L"      TPM Property Count: %d\n", PropertyCount);

  /* Search for TPM_PT_FAMILY_INDICATOR */
  for (UINT32 i = 0; i < PropertyCount; i++) {
      UINT32 PropertyId  = SwapBytes32(cap_data->data.tpmProperties.tpmProperty[i].property);
      UINT32 PropertyVal = cap_data->data.tpmProperties.tpmProperty[i].value;

      acs_print(ACS_PRINT_INFO, L"       TPM Property ID   : 0x%08x\n", PropertyId);
      acs_print(ACS_PRINT_INFO, L"       TPM Property Value: 0x%08x\n", PropertyVal);


      if (PropertyId == TPM_PT_FAMILY_INDICATOR) {
          char family[5];

          CopyMem(family, &PropertyVal, 4);
          family[4] = '\0';
          acs_print(ACS_PRINT_TEST, L"       TPM Family: %a\n", family);

          FreePool(TempBuffer);

          if (AsciiStrCmp(family, "2.0") == 0)
              return 2;
          else if (AsciiStrCmp(family, "1.2") == 0)
              return 1;
          else
              return 0;
      }
  }

  /* Free allocated memory */
  FreePool(TempBuffer);
  return 0;
}
