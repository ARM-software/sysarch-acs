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

#include "pal_common_support.h"
#include "platform_override_struct.h"

extern PLATFORM_OVERRIDE_TPM2_INFO_TABLE platform_tpm2_cfg;

/**
  @brief  Populate the TPM2 info table from platform configuration.

  @param  TPM2InfoTable  - Address to the TPM2 information table.

  @return  None
**/
void
pal_tpm2_create_info_table(TPM2_INFO_TABLE *Tpm2InfoTable)
{
    if (Tpm2InfoTable == NULL) {
        print(ACS_PRINT_ERR, "\nTPM2: Info table pointer is NULL");
        return;
    }

    Tpm2InfoTable->tpm_presence  = 0;
    Tpm2InfoTable->tpm_interface = 0;
    Tpm2InfoTable->base          = 0;

    if (platform_tpm2_cfg.tpm_present == 0) {
        print(ACS_PRINT_INFO, "\nTPM2: Platform reports no TPM");
        return;
    }

    Tpm2InfoTable->tpm_presence  = platform_tpm2_cfg.tpm_present;
    Tpm2InfoTable->tpm_interface = platform_tpm2_cfg.tpm_interface_type;
    Tpm2InfoTable->base          = platform_tpm2_cfg.tpm_base;

    print(ACS_PRINT_INFO, "\nTPM2: Present");
    print(ACS_PRINT_INFO, "\nTPM2: Interface 0x%llx", Tpm2InfoTable->tpm_interface);
    print(ACS_PRINT_INFO, "\nTPM2: Base 0x%llx", Tpm2InfoTable->base);
}

/**
  @brief  Retrieve the TPM2 version reported by the platform.

  @param  None

  @return TPM version as UINT64:
            2 - TPM 2.0
            1 - TPM 1.2
            0 - Unknown or unsupported
**/
uint64_t
pal_tpm2_get_version(void)
{
    if (platform_tpm2_cfg.tpm_present != 0)
        return platform_tpm2_cfg.tpm_version;

    return 0;
}
