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

#include "val/include/acs_val.h"
#include "val/include/acs_pe.h"
#include "val/include/val_interface.h"
#include "val/include/acs_tpm.h"

#define TEST_NUM   (ACS_TPM2_TEST_NUM_BASE  +  1)
#define TEST_RULE  "P_L1TP_01"
#define TEST_DESC  "Check TPM Version                     "

static
void
payload()
{

  uint64_t tpm_family_version;
  uint64_t tpm_is_present;
  uint32_t pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Check if TPM is present */
  tpm_is_present = val_tpm2_get_info(TPM2_INFO_IS_PRESENT);
  if (tpm_is_present == 0) {
      val_print(ACS_PRINT_ERR, "\n       TPM not present", 0);
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 01));
      return;
  }

  /* Retrieve TPM version via family identifier (e.g., "2.0", "1.2") */
  tpm_family_version = val_tpm2_get_version();
  if (tpm_family_version != 2) {
      val_print(ACS_PRINT_ERR, "\n       TPM version is not 2.0", 0);
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 02));
      return;
  }

  /* TPM is present and version is 2.0 */
  val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));
  return;
}


uint32_t
tpm001_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;
  num_pe = 1;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
     val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}

