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

#define TEST_NUM   (ACS_PFDI_TEST_NUM_BASE + 8)
#define TEST_RULE  "R0060"
#define TEST_DESC  "Check PFDI Discovery Function Support     "

typedef struct{
  int64_t status;
  int64_t status_invalid;
} pfdi_feature_check_details;

pfdi_feature_check_details *g_pfdi_feature_check_details;


void
check_feature()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  int64_t  status;
  pfdi_feature_check_details  *status_buffer;

  status_buffer = g_pfdi_feature_check_details + index;

  /* Invoke PFDI Feature function for current PE index */
  status = val_pfdi_features(PFDI_FN_PFDI_FEATURES);

  /*Save the return status*/
  status_buffer->status = status;
  val_data_cache_ops_by_va((addr_t)&status_buffer->status, CLEAN_AND_INVALIDATE);

  if (status != PFDI_ACS_SUCCESS) {
    val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
    return;
  }
  /* Invoke PFDI Feature function with invalid id for current PE index */
  status = val_pfdi_features(PFDI_FN_PFDI_INVALID);

  /*Save the return status*/
  status_buffer->status_invalid = status;
  val_data_cache_ops_by_va((addr_t)&status_buffer->status_invalid, CLEAN_AND_INVALIDATE);

  if (status != PFDI_ACS_NOT_SUPPORTED) {
    val_set_status(index, RESULT_FAIL(TEST_NUM, 2));
    return;
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 1));

  return;
}

static void payload_feature_check(void *arg)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t timeout, i = 0, run_fail = 0;
  pfdi_feature_check_details *status_buffer;
  uint32_t num_pe = *(uint32_t *)arg;

  /* Allocate memory to save all PFDI function status for all PE's */
  g_pfdi_feature_check_details = (pfdi_feature_check_details *)
                    val_memory_calloc(num_pe, sizeof(pfdi_feature_check_details));
  if (g_pfdi_feature_check_details == NULL) {
    val_print(ACS_PRINT_ERR,
                "\n       Allocation for PFDI Feature Check Function Failed", 0);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
    return;
  }

  for (i = 0; i < num_pe; i++) {
    status_buffer = g_pfdi_feature_check_details + i;
    val_data_cache_ops_by_va((addr_t)&status_buffer->status, CLEAN_AND_INVALIDATE);
    val_data_cache_ops_by_va((addr_t)&status_buffer->status_invalid, CLEAN_AND_INVALIDATE);
  }

  /* Invoke PFDI Feature function for current PE index */
  check_feature();

  /* Execute check_feature function in All PE's */
  for (i = 0; i < num_pe; i++) {
    if (i != index) {
      timeout = TIMEOUT_LARGE;
      val_execute_on_pe(i, check_feature, 0);

      while ((--timeout) && (IS_RESULT_PENDING(val_get_status(i))));

      if (timeout == 0) {
        val_print(ACS_PRINT_ERR, "\n       **Timed out** for PE index = %d", i);
        val_set_status(i, RESULT_FAIL(TEST_NUM, 2));
      }
    }
  }
  val_time_delay_ms(ONE_MILLISECOND);

  /* Check return status of function for all PE's */
  for (i = 0; i < num_pe; i++) {
    status_buffer = g_pfdi_feature_check_details + i;
    val_data_cache_ops_by_va((addr_t)&status_buffer->status, CLEAN_AND_INVALIDATE);
    val_data_cache_ops_by_va((addr_t)&status_buffer->status_invalid, CLEAN_AND_INVALIDATE);

    if (status_buffer->status < PFDI_ACS_SUCCESS) {
      val_print(ACS_PRINT_ERR, "\n       PFDI query Feature Check function failed err = %d",
                                                        status_buffer->status);
      val_print(ACS_PRINT_ERR, " on PE index = %d", i);
      val_set_status(i, RESULT_FAIL(TEST_NUM, 3));
      run_fail++;
    }

    if (status_buffer->status_invalid != PFDI_ACS_NOT_SUPPORTED) {
      val_print(ACS_PRINT_ERR, "\n       PFDI query Feature Check invalid function failed err = %d",
                                                        status_buffer->status_invalid);
      val_print(ACS_PRINT_ERR, " on PE index = %d", i);
      val_set_status(i, RESULT_FAIL(TEST_NUM, 4));
      run_fail++;
    }
  }

  if (run_fail == 0)
    val_set_status(index, RESULT_PASS(TEST_NUM, 1));

  val_memory_free((void *) g_pfdi_feature_check_details);

  return;
}

uint32_t pfdi008_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
    val_run_test_configurable_payload(&num_pe, payload_feature_check);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
