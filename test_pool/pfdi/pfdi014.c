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

#define TEST_NUM   (ACS_PFDI_TEST_NUM_BASE + 14)
#define TEST_RULE  "R0076"
#define TEST_DESC  "Check PFDI run with invalid parameters    "

/** Invalid parameter invalid_cases (test_cnt filled dynamically) */
#define NUM_INVALID_CASES 7

/** local struct for this test */
typedef struct {
    int64_t status[NUM_INVALID_CASES];
    int64_t fault_id[NUM_INVALID_CASES];
} pfdi_pe_test_support_info;

static pfdi_pe_test_support_info *g_pe_test_support_info;

static void
run_invalid_param_case(void)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t i;
  int64_t invalid_cases[NUM_INVALID_CASES][2];
  int64_t test_parts;
  pfdi_pe_test_support_info *pfdi_buffer;

  pfdi_buffer = g_pe_test_support_info + index;

  /* Get Num of Test parts supported on curr PE */
  test_parts = val_pfdi_pe_test_part_count(NULL, NULL, NULL, NULL);
  if (test_parts < PFDI_ACS_SUCCESS) {
    val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
    return;
  }

  if (test_parts == 0)
    test_parts = test_parts + 1;

    /** Populate invalid_cases with invalid parameter scenarios */
  invalid_cases[0][0] = 1;        invalid_cases[0][1] = 0;                /** start > end */
  invalid_cases[1][0] = test_parts; invalid_cases[1][1] = test_parts - 1; /** start > test_cnt-1 */
  invalid_cases[2][0] = 0;        invalid_cases[2][1] = test_parts;       /** end > test_cnt - 1 */
  invalid_cases[3][0] = -1;       invalid_cases[3][1] = 0;             /** start = -1, end != -1 */
  invalid_cases[4][0] = 0;        invalid_cases[4][1] = -1;            /** end = -1, start != -1 */
  invalid_cases[5][0] = -2;       invalid_cases[5][1] = test_parts - 1;/** start < -1 */
  invalid_cases[6][0] = 0;        invalid_cases[6][1] = -2;            /** end < -1 */


  for (i = 0; i < NUM_INVALID_CASES; ++i) {
    pfdi_buffer->status[i] = val_pfdi_pe_test_run(invalid_cases[i][0], invalid_cases[i][1],
                                                  &pfdi_buffer->fault_id[i], NULL, NULL, NULL);
    val_data_cache_ops_by_va((addr_t)&pfdi_buffer->status[i], CLEAN_AND_INVALIDATE);
    val_data_cache_ops_by_va((addr_t)&pfdi_buffer->fault_id[i], CLEAN_AND_INVALIDATE);
  }


  val_set_status(index, RESULT_PASS(TEST_NUM, 1));
}

/** --------------- payload that orchestrates all PEs ------------------ */
static void
payload_run_invalid_param_check(void *arg)
{
  uint32_t  num_pe = *((uint32_t *)arg);
  uint32_t  index    = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t  i = 0, j = 0, timeout, test_fail = 0;
  pfdi_pe_test_support_info *pfdi_buffer;


  /* Allocate memory to save all PFDI run status and fault id's for all PE's */
  g_pe_test_support_info = (pfdi_pe_test_support_info *)
                    val_memory_calloc(num_pe, sizeof(pfdi_pe_test_support_info));
  if (g_pe_test_support_info == NULL) {
    val_print(ACS_PRINT_ERR,
                "\n       Allocation for PFDI Run Function Failed", 0);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
    return;
  }

  for (i = 0; i < num_pe; i++) {
    pfdi_buffer = g_pe_test_support_info + i;
    for (j = 0; j < NUM_INVALID_CASES; j++) {
      val_data_cache_ops_by_va((addr_t)&pfdi_buffer->status[j], CLEAN_AND_INVALIDATE);
      val_data_cache_ops_by_va((addr_t)&pfdi_buffer->fault_id[j], CLEAN_AND_INVALIDATE);
    }
  }

  run_invalid_param_case();

  for (i = 0; i < num_pe; i++) {
    if (i != index) {
      timeout = TIMEOUT_LARGE;
      val_execute_on_pe(i, run_invalid_param_case, 0);

      /** Wait for result or timeout */
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
    pfdi_buffer = g_pe_test_support_info + i;
    for (j = 0; j < NUM_INVALID_CASES; j++) {
      val_data_cache_ops_by_va((addr_t)&pfdi_buffer->status[j], CLEAN_AND_INVALIDATE);
      val_data_cache_ops_by_va((addr_t)&pfdi_buffer->fault_id[j], CLEAN_AND_INVALIDATE);
    }
    test_fail = 0;

    if (IS_TEST_FAIL(val_get_status(i))) {
      val_print(ACS_PRINT_ERR, "\n       Failed to get Test Part count on PE %d ", i);
      val_set_status(i, RESULT_FAIL(TEST_NUM, 3));
      goto free_pfdi_details;
    }

    for (j = 0; j < NUM_INVALID_CASES; j++) {
      /* Check if status is invalid parameters and fault_id is 0 */
      if (pfdi_buffer->status[j] != PFDI_ACS_INVALID_PARAMETERS) {
        val_print(ACS_PRINT_ERR,
                "\n       Invalid param test failed on PE: %d", i);
        val_print(ACS_PRINT_ERR, " expected status -3, return status %ld", pfdi_buffer->status[j]);
        val_print(ACS_PRINT_ERR, " for case %d", j);
        test_fail++;
      }

      if (pfdi_buffer->fault_id[j] != 0) {
        val_print(ACS_PRINT_ERR,
                  "\n       Fault ID check failed on PE: %d ", i);
        val_print(ACS_PRINT_ERR, "expected fault 0, return fault %ld", pfdi_buffer->fault_id[j]);
        val_print(ACS_PRINT_ERR, " for case %d", j);
        test_fail++;
      }
    }

    if (test_fail)
      val_set_status(i, RESULT_FAIL(TEST_NUM, 4));
    else
      val_set_status(i, RESULT_PASS(TEST_NUM, 1));
  }

free_pfdi_details:
  val_memory_free((void *) g_pe_test_support_info);

  return;
}

/** ---------------- ACS entry point ---------------- */
uint32_t pfdi014_entry(uint32_t num_pe)
{
  uint32_t status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
    val_run_test_configurable_payload(&num_pe,
                          payload_run_invalid_param_check);

  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);
  return status;
}
