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

#include "acs_val.h"
#include "acs_pcie.h"
#include "acs_pe.h"

#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 101)
#define TEST_RULE  "PCI_IN_06"
#define TEST_DESC  "Check config read for device          "

static
void
payload(void)
{
  uint32_t bdf;
  uint32_t dp_type;
  uint32_t reg_value;
  uint32_t status;
  uint32_t pe_index;
  uint32_t index;
  uint32_t test_fail;
  uint32_t test_skip;
  pcie_device_bdf_table *bdf_tbl_ptr;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();
  test_fail = 0;
  test_skip = 1;

  for (index = 0; index < bdf_tbl_ptr->num_entries; index++)
  {
      bdf = bdf_tbl_ptr->device[index].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      if ((dp_type == RCiEP) || (dp_type == RCEC))
          continue;

      val_print(DEBUG, "\n       BDF 0x%x", bdf);
      test_skip = 0;

      status = val_pcie_read_cfg(bdf, TYPE01_VIDR, &reg_value);

      if ((status != PCIE_SUCCESS) || (reg_value == PCIE_UNKNOWN_RESPONSE)) {
          val_print(ERROR, "\n       Config read failed for BDF 0x%x", bdf);
          test_fail++;
      }
  }

  if (test_skip == 1)
      val_set_status(pe_index, RESULT_SKIP(1));
  else if (test_fail)
      val_set_status(pe_index, RESULT_FAIL(test_fail));
  else
      val_set_status(pe_index, RESULT_PASS);
}

uint32_t
p101_entry(uint32_t num_pe)
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
