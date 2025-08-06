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

#define TEST_NUM   (ACS_PFDI_TEST_NUM_BASE + 3)
#define TEST_RULE  "R0102"
#define TEST_DESC  "Check PFDI mandatory functions            "

typedef struct{
  int64_t status[PFDI_FN_PFDI_MAX_NUM];
} feature_details;

feature_details *g_pfdi_feature_details;


void
pfdi_function_check(void)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t fn_status = 0, f_id;
  feature_details *pfdi_buffer = g_pfdi_feature_details + index;

  for (f_id = PFDI_FN_PFDI_VERSION; f_id <= PFDI_FN_PFDI_FORCE_ERROR; f_id++) {
    /* Invoke PFDI Feature function for current PE index */
    pfdi_buffer->status[fn_status] = val_pfdi_features(f_id);
    val_data_cache_ops_by_va((addr_t)&pfdi_buffer->status[fn_status], CLEAN_AND_INVALIDATE);
    fn_status++;
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 1));
  return;
}

static void payload_functions_check(void *arg)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t num_pe = *((uint32_t *)arg);
  uint32_t f_id, fn_status = 0;
  uint32_t timeout, i = 0, test_fail = 0;
  feature_details *pfdi_buffer;

  /* Allocate memory to save all PFDI features status for all PE's */
  g_pfdi_feature_details = (feature_details *) val_memory_calloc(num_pe, sizeof(feature_details));
  if (g_pfdi_feature_details == NULL) {
    val_print(ACS_PRINT_ERR, "\n       Allocation for PFDI Feature Details Failed \n", 0);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
    return;
  }

  for (i = 0; i < num_pe; i++) {
    pfdi_buffer = g_pfdi_feature_details + i;
    for (f_id = PFDI_FN_PFDI_VERSION; f_id <= PFDI_FN_PFDI_FORCE_ERROR; f_id++)
      val_data_cache_ops_by_va((addr_t)&pfdi_buffer->status[fn_status++], CLEAN_AND_INVALIDATE);
  }


  /* Invoke PFDI Feature function for current PE index */
  pfdi_function_check();

  /* Execute pfdi_function_check function in All PE's */
  for (i = 0; i < num_pe; i++) {
    if (i != index) {
      timeout = TIMEOUT_LARGE;
      val_execute_on_pe(i, pfdi_function_check, 0);

      while ((--timeout) && (IS_RESULT_PENDING(val_get_status(i))));

      if (timeout == 0) {
        val_print(ACS_PRINT_ERR, "\n       **Timed out** for PE index = %d", i);
        val_set_status(i, RESULT_FAIL(TEST_NUM, 2));
        goto free_pfdi_details;
      }
    }
  }
  val_time_delay_ms(ONE_MILLISECOND);

  for (i = 0; i < num_pe; i++) {
    pfdi_buffer = g_pfdi_feature_details + i;
    test_fail = 0;
    fn_status = 0;

    for (f_id = PFDI_FN_PFDI_VERSION; f_id <= PFDI_FN_PFDI_FORCE_ERROR; f_id++)
      val_data_cache_ops_by_va((addr_t)&pfdi_buffer->status[fn_status++], CLEAN_AND_INVALIDATE);

    fn_status = 0;
    for (f_id = PFDI_FN_PFDI_VERSION; f_id <= PFDI_FN_PFDI_FORCE_ERROR; f_id++) {
      if (pfdi_buffer->status[fn_status] != PFDI_ACS_SUCCESS) {
        val_print(ACS_PRINT_ERR,
                "\n       PFDI mandatory function 0x%x ", f_id);
        val_print(ACS_PRINT_ERR, "failed on PE %d, ", i);
        val_print(ACS_PRINT_ERR, "status = %d", pfdi_buffer->status[fn_status]);
        test_fail++;
      }
      fn_status++;
    }

    if (test_fail)
      val_set_status(i, RESULT_FAIL(TEST_NUM, 3));
    else
      val_set_status(i, RESULT_PASS(TEST_NUM, 1));
  }

free_pfdi_details:
  val_memory_free((void *) g_pfdi_feature_details);

  return;
}

uint32_t pfdi003_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
    val_run_test_configurable_payload(&num_pe, payload_functions_check);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
