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

#define TEST_NUM   (ACS_PFDI_TEST_NUM_BASE + 5)
#define TEST_RULE  "R0076"
#define TEST_DESC  "Execute Test Parts on PE                  "

extern pfdi_pe_test_support_info *g_pfdi_pe_test_support_info;

typedef struct{
  int64_t run_status;
  int64_t fault_id;
} pfdi_run_status_details;

pfdi_run_status_details *g_pfdi_run_status_details;

void
pfdi_test_run(void)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  int64_t  status, fault_test_id, start, end;
  pfdi_pe_test_support_info *test_buffer;
  pfdi_run_status_details   *status_buffer;

  test_buffer = g_pfdi_pe_test_support_info + index;
  val_data_cache_ops_by_va((addr_t)&test_buffer->test_count, CLEAN_AND_INVALIDATE);
  start = 0;
  end   = test_buffer->test_count;

  status_buffer = g_pfdi_run_status_details + index;

  /* Invoke PFDI Run function for current PE index */
  status = val_pfdi_pe_test_run(start, end, &fault_test_id);

  status_buffer->run_status = status;
  val_data_cache_ops_by_va((addr_t)&status_buffer->run_status, CLEAN_AND_INVALIDATE);
  if (status < PFDI_ACS_SUCCESS) {
    status_buffer->fault_id = fault_test_id;
    val_data_cache_ops_by_va((addr_t)&status_buffer->fault_id, CLEAN_AND_INVALIDATE);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
    return;
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 1));
  return;
}

static void payload_run(void *arg)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t timeout, i = 0;
  int64_t  status, fault_test_id, start, end;
  uint32_t num_pe = *(uint32_t *)arg;
  pfdi_pe_test_support_info *test_buffer;
  pfdi_run_status_details   *status_buffer;

  test_buffer = g_pfdi_pe_test_support_info + index;
  val_data_cache_ops_by_va((addr_t)&test_buffer->test_count, CLEAN_AND_INVALIDATE);
  start = 0;
  end   = test_buffer->test_count;

  /* Invoke PFDI Run count function for current PE index */
  status = val_pfdi_pe_test_run(start, end, &fault_test_id);
  if (status < PFDI_ACS_SUCCESS) {
    if (status == PFDI_ACS_FAULT_FOUND) {
      if (fault_test_id == PFDI_ACS_UNKNOWN) {
        val_print(ACS_PRINT_ERR,
                "\n       PFDI test part that triggered the fault cannot be identified", 0);
      } else {
        val_print(ACS_PRINT_ERR,
                "\n       PFDI test part that triggered the fault = %d", fault_test_id);
      }
    } else if (status == PFDI_ACS_ERROR) {
      val_print(ACS_PRINT_ERR,
                "\n       PFDI Test part(s) have executed but failed to complete", 0);
    } else {
      val_print(ACS_PRINT_ERR,
               "\n       PFDI query PE Run function failed err = %d", status);
    }
    val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
    return;
  }

  /* Allocate memory to save all PFDI run status and fault id's for all PE's */
  g_pfdi_run_status_details =
            (pfdi_run_status_details *) val_memory_calloc(num_pe, sizeof(pfdi_run_status_details));
  if (g_pfdi_run_status_details == NULL) {
    val_print(ACS_PRINT_ERR,
                "\n       Allocation for PFDI Run Function Failed", 0);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 2));
    return;
  }

  for (i = 0; i < num_pe; i++) {
    status_buffer = g_pfdi_run_status_details + i;
    val_data_cache_ops_by_va((addr_t)&status_buffer->run_status, CLEAN_AND_INVALIDATE);
    val_data_cache_ops_by_va((addr_t)&status_buffer->fault_id, CLEAN_AND_INVALIDATE);
  }

  /* Execute pfdi_test_run function in All PE's */
  for (i = 0; i < num_pe; i++) {
    if (i != index) {
      timeout = TIMEOUT_LARGE;
      val_execute_on_pe(i, pfdi_test_run, 0);

      while ((--timeout) && (IS_RESULT_PENDING(val_get_status(i))));

      if (timeout == 0) {
        val_print(ACS_PRINT_ERR, "\n       **Timed out** for PE index = %d", i);
        val_set_status(i, RESULT_FAIL(TEST_NUM, 3));
        goto free_pfdi_details;
      }
    }
  }
  val_time_delay_ms(ONE_MILLISECOND);

  /* Check return status of function for all PE's */
  for (i = 0; i < num_pe; i++) {
    if (i != index) {
      status_buffer = g_pfdi_run_status_details + i;
      val_data_cache_ops_by_va((addr_t)&status_buffer->run_status, CLEAN_AND_INVALIDATE);
      val_data_cache_ops_by_va((addr_t)&status_buffer->fault_id, CLEAN_AND_INVALIDATE);

      if (status_buffer->run_status < PFDI_ACS_SUCCESS) {
        if (status_buffer->run_status == PFDI_ACS_FAULT_FOUND) {
          if (status_buffer->fault_id == PFDI_ACS_UNKNOWN) {
            val_print(ACS_PRINT_ERR,
                "\n       PFDI test part that triggered the fault cannot be identified", 0);
          } else {
            val_print(ACS_PRINT_ERR,
                "\n       PFDI test part that triggered the fault = %d", status_buffer->fault_id);
          }
        } else if (status_buffer->run_status == PFDI_ACS_ERROR) {
          val_print(ACS_PRINT_ERR,
                "\n       PFDI Test part(s) have executed but failed to complete", 0);
        } else {
          val_print(ACS_PRINT_ERR,
                "\n       PFDI query PE Run function failed err = %d", status_buffer->run_status);
        }
        val_set_status(i, RESULT_FAIL(TEST_NUM, 4));
        goto free_pfdi_details;
      }

    }
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 1));

free_pfdi_details:
  val_memory_free((void *) g_pfdi_run_status_details);
  return;
}

uint32_t pfdi005_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
    val_run_test_configurable_payload(&num_pe, payload_run);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
