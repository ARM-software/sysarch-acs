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
#include "acs_cxl.h"

#define TEST_NUM   (ACS_CXL_TEST_NUM_BASE + 3)
#define TEST_RULE  "CXL_03"
#define TEST_DESC  "Check CHBCR address map               "

static void
payload(void)
{
  uint32_t pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t num_cxl_hb;
  uint32_t index;
  uint32_t test_fails = 0;
  uint64_t cxl_hb_base, cxl_hb_len;

  num_cxl_hb = val_cxl_get_info(CXL_INFO_NUM_DEVICES, 0);
  if (num_cxl_hb == 0) {
      val_print(DEBUG, "\n       No CXL devices discovered");
      val_set_status(pe_index, RESULT_SKIP(1));
      return;
  }

  for (index = 0; index < num_cxl_hb; index++) {
      cxl_hb_base = val_cxl_get_info(CXL_INFO_COMPONENT_BASE, index);
      cxl_hb_len = val_cxl_get_info(CXL_INFO_COMPONENT_LENGTH, index);
      if ((cxl_hb_base == 0) || (cxl_hb_len == 0)) {
          val_print(ERROR, "\n   CHBCR address map failed for CXL HB %x", index);
          test_fails++;
      }
  }

  if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(1));
  else
      val_set_status(pe_index, RESULT_PASS);
}

uint32_t
cxl003_entry(uint32_t num_pe)
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
