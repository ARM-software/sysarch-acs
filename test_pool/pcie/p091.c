/** @file
 * Copyright (c) 2024-2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/acs_pcie.h"
#include "val/include/val_interface.h"

#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 91)
#define TEST_RULE  "B_PCIe_11"
#define TEST_DESC  "Steering Tag value properties         "

static
void
payload(void)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t status;

  /* Obtain the STE values from PAL using _DSM Method */
  status = val_pcie_dsm_ste_tags();
  val_print(ACS_PRINT_DEBUG, "\n  STE tag value is %x", status);


  if (status == NOT_IMPLEMENTED) {
      val_print(ACS_PRINT_DEBUG, "\n DSM method for STE not implemented\n", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM, 0));
  }
  else if (status == 0) {
      val_print(ACS_PRINT_ERR, "\n STE tag value should not be 0\n", 0);
      val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
  }
  else
      val_set_status(index, RESULT_PASS(TEST_NUM, 0));

}

uint32_t
p091_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}

