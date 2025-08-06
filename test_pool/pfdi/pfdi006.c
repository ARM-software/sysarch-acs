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
#include "val/include/acs_memory.h"

#define TEST_NUM   (ACS_PFDI_TEST_NUM_BASE + 6)
#define TEST_RULE  "R0071"
#define TEST_DESC  "Check num of Test Part supported          "

typedef struct {
  int64_t test_count;
} pfdi_pe_test_support_info;

pfdi_pe_test_support_info *g_pfdi_pe_test_support_info;

void
pfdi_test_part_count(void)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  pfdi_pe_test_support_info *test_buffer = g_pfdi_pe_test_support_info + index;

  /* Invoke PFDI Feature function for current PE index */
  test_buffer->test_count = val_pfdi_pe_test_part_count();
  val_data_cache_ops_by_va((addr_t)&test_buffer->test_count, CLEAN_AND_INVALIDATE);

  val_set_status(index, RESULT_PASS(TEST_NUM, 1));
  return;
}

static void payload_pe_test_info_check(void *arg)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t num_pe = *((uint32_t *)arg);
  uint32_t timeout, i = 0;
  pfdi_pe_test_support_info *test_buffer;

  /* Allocate memory to save all PFDI Versions or status for all PE's */
  g_pfdi_pe_test_support_info =
        (pfdi_pe_test_support_info *)val_memory_calloc(num_pe, sizeof(pfdi_pe_test_support_info));
  if (g_pfdi_pe_test_support_info == NULL) {
      val_print(ACS_PRINT_ERR, "\n       Allocation for PFDI PE Test Support Info Failed \n", 0);
      return;
  }

  for (i = 0; i < num_pe; i++) {
    test_buffer = g_pfdi_pe_test_support_info + i;
    val_data_cache_ops_by_va((addr_t)&test_buffer->test_count, CLEAN_AND_INVALIDATE);
  }

  /* Invoke PFDI Test Part count function for current PE index */
  pfdi_test_part_count();

  /* Execute pfdi_version_check function in All PE's */
  for (i = 0; i < num_pe; i++) {
    if (i != index) {
      timeout = TIMEOUT_LARGE;
      val_execute_on_pe(i, pfdi_test_part_count, 0);

      while ((--timeout) && (IS_RESULT_PENDING(val_get_status(i))));

      if (timeout == 0) {
        val_print(ACS_PRINT_ERR, "\n       **Timed out** for PE index = %d", i);
        val_set_status(i, RESULT_FAIL(TEST_NUM, 2));
        goto free_pfdi_details;
      }
    }
  }
  val_time_delay_ms(ONE_MILLISECOND);

  /* Check return status of function for all PE's */
  for (i = 0; i < num_pe; i++) {
    test_buffer = g_pfdi_pe_test_support_info + i;

    val_data_cache_ops_by_va((addr_t)&test_buffer->test_count, CLEAN_AND_INVALIDATE);

    if (test_buffer->test_count < PFDI_ACS_SUCCESS) {
      val_print(ACS_PRINT_ERR, "\n       PFDI query Test part count failed %lld",
                                  test_buffer->test_count);
      val_print(ACS_PRINT_ERR, "on PE = %d", i);
      val_set_status(i, RESULT_FAIL(TEST_NUM, 3));
    } else {
      val_print(ACS_PRINT_DEBUG,
                    "\n       Test Part Count = 0x%llx", test_buffer->test_count);
      val_print(ACS_PRINT_DEBUG, "on PE  %d", i);
      val_set_status(i, RESULT_PASS(TEST_NUM, 1));
    }
  }

free_pfdi_details:
  val_memory_free((void *) g_pfdi_pe_test_support_info);

  return;
}

uint32_t pfdi006_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
    val_run_test_configurable_payload(&num_pe, payload_pe_test_info_check);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
