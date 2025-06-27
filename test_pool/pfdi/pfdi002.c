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
#include "val/include/val_interface.h"

#define TEST_NUM   (ACS_PFDI_TEST_NUM_BASE + 2)
#define TEST_RULE  "R0060, R0102, R0105"
#define TEST_DESC  "Check PFDI mandatory functions            "

void
pfdi_function_check(void)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t f_id;
  int64_t  pfdi_feature;

  for (f_id = PFDI_FN_PFDI_PE_TEST_ID; f_id <= PFDI_FN_PFDI_FORCE_ERROR; f_id++) {
    /* Invoke PFDI Feature function for current PE index */
    pfdi_feature = val_pfdi_features(f_id);
    if (pfdi_feature != PFDI_ACS_SUCCESS) {
      val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
      return;
    }
  }
  val_set_status(index, RESULT_PASS(TEST_NUM, 1));

  return;
}


static void payload(uint32_t num_pe)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t f_id;
  int64_t  pfdi_feature;
  uint32_t timeout, i = 0;

  for (f_id = PFDI_FN_PFDI_PE_TEST_ID; f_id <= PFDI_FN_PFDI_FORCE_ERROR; f_id++) {
    /* Invoke PFDI Feature function for current PE index */
    pfdi_feature = val_pfdi_features(f_id);
    if (pfdi_feature != PFDI_ACS_SUCCESS) {
      val_print(ACS_PRINT_ERR,
                "\n       PFDI query mandatory function 0x%x", f_id);
      val_print(ACS_PRINT_ERR, " failed err = %d", pfdi_feature);
      val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
      return;
    }
  }

  /* Execute pfdi_version_check function in All PE's */
  for (i = 0; i < num_pe; i++) {
    if (i != index) {
      timeout = TIMEOUT_LARGE;
      val_execute_on_pe(i, pfdi_function_check, 0);

      while ((--timeout) && (IS_RESULT_PENDING(val_get_status(i))));

      if (timeout == 0) {
        val_print(ACS_PRINT_ERR, "\n       **Timed out** for PE index = %d", i);
        val_set_status(i, RESULT_FAIL(TEST_NUM, 2));
        return;
      }
    }
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 1));

  return;
}

uint32_t pfdi002_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
    payload(num_pe);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
