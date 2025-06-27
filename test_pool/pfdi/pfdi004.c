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

#define TEST_NUM   (ACS_PFDI_TEST_NUM_BASE + 4)
#define TEST_RULE  "R0071"
#define TEST_DESC  "Check num of Test Part supported          "

extern pfdi_pe_test_support_info *g_pfdi_pe_test_support_info;

void
pfdi_test_part_count(void)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  int64_t  test_count;
  pfdi_pe_test_support_info *test_buffer;

  test_buffer = g_pfdi_pe_test_support_info + index;

  /* Invoke PFDI Feature function for current PE index */
  test_count = val_pfdi_pe_test_part_count();
  if (test_count < PFDI_ACS_SUCCESS) {
    val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
    return;
  }

  test_buffer->test_count = test_count;
  val_data_cache_ops_by_va((addr_t)&test_buffer->test_count, CLEAN_AND_INVALIDATE);
  val_set_status(index, RESULT_PASS(TEST_NUM, 1));
  return;
}

static void payload(uint32_t num_pe)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t timeout, i = 0;
  int64_t  test_count;
  pfdi_pe_test_support_info *test_buffer;

  test_buffer = g_pfdi_pe_test_support_info + index;

  /* Invoke PFDI Test Part count function for current PE index */
  test_count = val_pfdi_pe_test_part_count();
  if (test_count < PFDI_ACS_SUCCESS) {
    val_print(ACS_PRINT_ERR,
              "\n       PFDI query PE Test Part function failed err = %d", test_count);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
    return;
  }

  test_buffer->test_count = test_count;
  val_data_cache_ops_by_va((addr_t)&test_buffer->test_count, CLEAN_AND_INVALIDATE);

  /* Execute pfdi_version_check function in All PE's */
  for (i = 0; i < num_pe; i++) {
    if (i != index) {
      timeout = TIMEOUT_LARGE;
      val_execute_on_pe(i, pfdi_test_part_count, 0);

      while ((--timeout) && (IS_RESULT_PENDING(val_get_status(i))));

      if (timeout == 0) {
        val_print(ACS_PRINT_ERR, "\n       **Timed out** for PE index = %d", i);
        val_set_status(i, RESULT_FAIL(TEST_NUM, 2));
        return;
      }
    }
  }
  val_time_delay_ms(ONE_MILLISECOND);

  /* Check return status of function for all PE's */
  for (i = 0; i < num_pe; i++) {
    test_buffer = g_pfdi_pe_test_support_info + i;

    val_data_cache_ops_by_va((addr_t)&test_buffer->test_count, CLEAN_AND_INVALIDATE);

    if (test_buffer->test_count == PFDI_ACS_NOT_SUPPORTED) {
      val_print(ACS_PRINT_ERR, "\n       PFDI query Test part count not supported", 0);
      val_print(ACS_PRINT_ERR, "\n       Failed on PE = %d", i);
      val_set_status(i, RESULT_FAIL(TEST_NUM, 3));
    } else if (test_buffer->test_count >= PFDI_ACS_SUCCESS) {
      val_print(ACS_PRINT_DEBUG, "\n       PE Index = %d", i);
      val_print(ACS_PRINT_DEBUG,
                    "\n       Test Part Count = 0x%llx", test_buffer->test_count);
    } else {
      val_print(ACS_PRINT_ERR, "\n       PFDI query Test part count failed err = %lld",
                                  test_buffer->test_count);
      val_print(ACS_PRINT_ERR, "\n       Failed on PE = %d", i);
      val_set_status(i, RESULT_FAIL(TEST_NUM, 4));
    }
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 1));
  return;
}

uint32_t pfdi004_entry(uint32_t num_pe)
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
