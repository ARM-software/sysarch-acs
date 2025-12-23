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

#include "val/include/acs_val.h"
#include "val/include/val_interface.h"
#include "val/include/acs_memory.h"

#define TEST_NUM   (ACS_PFDI_TEST_NUM_BASE + 27)
#define TEST_RULE  "R0193"
#define TEST_DESC  "Check FORCE ERROR overwrite behavior      "

static const char *pfdi_fn_names[PFDI_FN_MAX_IDX] = {
  [PFDI_FN_VERSION_IDX]            = "PFDI_VERSION",
  [PFDI_FN_FEATURES_IDX]           = "PFDI_FEATURES",
  [PFDI_FN_PE_TEST_ID_IDX]         = "PFDI_PE_TEST_ID",
  [PFDI_FN_PE_TEST_PART_COUNT_IDX] = "PFDI_PE_TEST_PART_COUNT",
  [PFDI_FN_PE_TEST_RUN_IDX]        = "PFDI_PE_TEST_RUN",
  [PFDI_FN_PE_TEST_RESULT_IDX]     = "PFDI_PE_TEST_RESULT",
  [PFDI_FN_FW_CHECK_IDX]           = "PFDI_FW_CHECK"
};


typedef struct {
  int64_t first_error_x0;
  int64_t second_error_x0;
  int64_t first_call_x0;
} pfdi_error_injection_results;

static pfdi_error_injection_results *g_results;

/* Test error injection overwrite on current PE */
static void
check_error_overwrite(void)
{
  uint32_t index;
  uint32_t num_pe;
  pfdi_error_injection_results *result;

  index = val_pe_get_index_mpid(val_pe_get_mpid());
  num_pe = val_pe_get_num();

  /*
   * Test sequence (per PE, per function):
   * Step 1: Inject error A using PFDI_FORCE_ERROR
   * Step 2: Inject error B using PFDI_FORCE_ERROR (must overwrite A)
   * Step 3: Call target function: must return error B
   */

  /* PFDI_VERSION */
  result = g_results + (PFDI_FN_VERSION_IDX * num_pe) + index;
  result->first_error_x0 = (int64_t)val_pfdi_force_error(PFDI_FN_PFDI_VERSION,
                                                        PFDI_ACS_NOT_SUPPORTED,
                                                        NULL, NULL, NULL, NULL);
  result->second_error_x0 = (int64_t)val_pfdi_force_error(PFDI_FN_PFDI_VERSION,
                                                         PFDI_ACS_INVALID_PARAMETERS,
                                                         NULL, NULL, NULL, NULL);
  result->first_call_x0 = (int64_t)val_pfdi_version(NULL, NULL, NULL, NULL);
  val_data_cache_ops_by_va((addr_t)result, CLEAN_AND_INVALIDATE);

  /* PFDI_FEATURES */
  result = g_results + (PFDI_FN_FEATURES_IDX * num_pe) + index;
  result->first_error_x0 = (int64_t)val_pfdi_force_error(PFDI_FN_PFDI_FEATURES,
                                                        PFDI_ACS_NOT_SUPPORTED,
                                                        NULL, NULL, NULL, NULL);
  result->second_error_x0 = (int64_t)val_pfdi_force_error(PFDI_FN_PFDI_FEATURES,
                                                         PFDI_ACS_INVALID_PARAMETERS,
                                                         NULL, NULL, NULL, NULL);
  result->first_call_x0 = (int64_t)val_pfdi_features(PFDI_FN_PFDI_VERSION,
                                                    NULL, NULL, NULL, NULL);
  val_data_cache_ops_by_va((addr_t)result, CLEAN_AND_INVALIDATE);

  /* PFDI_PE_TEST_ID */
  result = g_results + (PFDI_FN_PE_TEST_ID_IDX * num_pe) + index;
  result->first_error_x0 = (int64_t)val_pfdi_force_error(PFDI_FN_PFDI_PE_TEST_ID,
                                                        PFDI_ACS_NOT_SUPPORTED,
                                                        NULL, NULL, NULL, NULL);
  result->second_error_x0 = (int64_t)val_pfdi_force_error(PFDI_FN_PFDI_PE_TEST_ID,
                                                         PFDI_ACS_INVALID_PARAMETERS,
                                                         NULL, NULL, NULL, NULL);
  result->first_call_x0 = (int64_t)val_pfdi_pe_test_id(NULL, NULL, NULL, NULL);
  val_data_cache_ops_by_va((addr_t)result, CLEAN_AND_INVALIDATE);

  /* PFDI_PE_TEST_PART_COUNT */
  result = g_results + (PFDI_FN_PE_TEST_PART_COUNT_IDX * num_pe) + index;
  result->first_error_x0 = (int64_t)val_pfdi_force_error(
      PFDI_FN_PFDI_PE_TEST_PART_COUNT, PFDI_ACS_NOT_SUPPORTED,
      NULL, NULL, NULL, NULL);
  result->second_error_x0 = (int64_t)val_pfdi_force_error(
      PFDI_FN_PFDI_PE_TEST_PART_COUNT, PFDI_ACS_INVALID_PARAMETERS,
      NULL, NULL, NULL, NULL);
  result->first_call_x0 = (int64_t)val_pfdi_pe_test_part_count(NULL, NULL, NULL, NULL);
  val_data_cache_ops_by_va((addr_t)result, CLEAN_AND_INVALIDATE);

  /* PFDI_PE_TEST_RUN */
  result = g_results + (PFDI_FN_PE_TEST_RUN_IDX * num_pe) + index;
  result->first_error_x0 = (int64_t)val_pfdi_force_error(PFDI_FN_PFDI_PE_TEST_RUN,
                                                        PFDI_ACS_NOT_SUPPORTED,
                                                        NULL, NULL, NULL, NULL);
  result->second_error_x0 = (int64_t)val_pfdi_force_error(PFDI_FN_PFDI_PE_TEST_RUN,
                                                         PFDI_ACS_INVALID_PARAMETERS,
                                                         NULL, NULL, NULL, NULL);
  result->first_call_x0 = (int64_t)val_pfdi_pe_test_run(-1, -1, NULL, NULL, NULL, NULL);
  val_data_cache_ops_by_va((addr_t)result, CLEAN_AND_INVALIDATE);

  /* PFDI_PE_TEST_RESULT */
  result = g_results + (PFDI_FN_PE_TEST_RESULT_IDX * num_pe) + index;
  result->first_error_x0 = (int64_t)val_pfdi_force_error(PFDI_FN_PFDI_PE_TEST_RESULT,
                                                        PFDI_ACS_NOT_SUPPORTED,
                                                        NULL, NULL, NULL, NULL);
  result->second_error_x0 = (int64_t)val_pfdi_force_error(PFDI_FN_PFDI_PE_TEST_RESULT,
                                                         PFDI_ACS_INVALID_PARAMETERS,
                                                         NULL, NULL, NULL, NULL);
  result->first_call_x0 = (int64_t)val_pfdi_pe_test_result(NULL, NULL, NULL, NULL);
  val_data_cache_ops_by_va((addr_t)result, CLEAN_AND_INVALIDATE);

  /* PFDI_FW_CHECK */
  result = g_results + (PFDI_FN_FW_CHECK_IDX * num_pe) + index;
  result->first_error_x0 = (int64_t)val_pfdi_force_error(PFDI_FN_PFDI_FW_CHECK,
                                                        PFDI_ACS_NOT_SUPPORTED,
                                                        NULL, NULL, NULL, NULL);
  result->second_error_x0 = (int64_t)val_pfdi_force_error(PFDI_FN_PFDI_FW_CHECK,
                                                         PFDI_ACS_INVALID_PARAMETERS,
                                                         NULL, NULL, NULL, NULL);
  result->first_call_x0 = (int64_t)val_pfdi_fw_check(NULL, NULL, NULL, NULL);
  val_data_cache_ops_by_va((addr_t)result, CLEAN_AND_INVALIDATE);

  val_set_status(index, RESULT_PASS(TEST_NUM, 1));
}

/* Validate error injection overwrite behavior across all PEs */
static void
payload_check_error_overwrite(void *arg)
{
  uint32_t num_pe = *((uint32_t *)arg);
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t i, j, timeout, test_fail, test_skip;
  pfdi_error_injection_results *result;

  g_results = (pfdi_error_injection_results *)
      val_memory_calloc(num_pe * PFDI_FN_MAX_IDX, sizeof(pfdi_error_injection_results));
  if (g_results == NULL) {
    val_print(ACS_PRINT_ERR, "\n       Allocation for results Failed", 0);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
    return;
  }

  /* Execute test on primary PE */
  check_error_overwrite();

  /* Execute test on all other PEs */
  for (i = 0; i < num_pe; i++) {
    if (i != index) {
      timeout = TIMEOUT_LARGE;
      val_execute_on_pe(i, check_error_overwrite, 0);

      while ((--timeout) && (IS_RESULT_PENDING(val_get_status(i))));

      if (timeout == 0) {
        val_print(ACS_PRINT_ERR, "\n       **Timed out** for PE index = %d", i);
        val_set_status(i, RESULT_FAIL(TEST_NUM, 2));
        goto free_results;
      }
    }
  }

  val_time_delay_ms(ONE_MILLISECOND);

  /* Validate results from all PEs and all functions */
  for (i = 0; i < num_pe; i++) {
    test_fail = 0;
    test_skip = 0;

    for (j = 0; j < PFDI_FN_FORCE_ERROR_IDX; j++) {
      result = g_results + (j * num_pe) + i;
      val_data_cache_ops_by_va((addr_t)result, INVALIDATE);

      /* Validate first FORCE_ERROR call status */
      if (result->first_error_x0 == PFDI_ACS_ERROR) {
        val_print(ACS_PRINT_WARN, "\n       FORCE_ERROR first %a error; skipping checks",
                  (uint64_t)pfdi_fn_names[j]);
        val_print(ACS_PRINT_WARN, " on PE index %d", i);
        test_skip++;
        continue;
      }
      if (result->first_error_x0 != PFDI_ACS_SUCCESS) {
        val_print(ACS_PRINT_ERR, "\n       PFDI force_error %a ", (uint64_t)pfdi_fn_names[j]);
        val_print(ACS_PRINT_ERR, "first injection failed err %ld ",
                  (int64_t)result->first_error_x0);
        val_print(ACS_PRINT_ERR, "on PE index %d", i);
        test_fail++;
      }

      /* Validate second FORCE_ERROR call status */
      if (result->second_error_x0 == PFDI_ACS_ERROR) {
        val_print(ACS_PRINT_WARN, "\n       FORCE_ERROR second %a error; skipping checks",
                  (uint64_t)pfdi_fn_names[j]);
        val_print(ACS_PRINT_WARN, " on PE index %d", i);
        test_skip++;
        continue;
      }
      if (result->second_error_x0 != PFDI_ACS_SUCCESS) {
        val_print(ACS_PRINT_ERR, "\n       PFDI force_error %a ", (uint64_t)pfdi_fn_names[j]);
        val_print(ACS_PRINT_ERR, "second injection failed err %ld ",
                  (int64_t)result->second_error_x0);
        val_print(ACS_PRINT_ERR, "on PE index %d", i);
        test_fail++;
      }

      /* Validate first call returns second injected error (overwrite behavior) */
      if (result->first_call_x0 != PFDI_ACS_INVALID_PARAMETERS) {
        val_print(ACS_PRINT_ERR, "\n       PFDI return %a ", (uint64_t)pfdi_fn_names[j]);
        val_print(ACS_PRINT_ERR, "overwrite check failed err %ld ",
                  (int64_t)result->first_call_x0);
        val_print(ACS_PRINT_ERR, "expected %ld ", (int64_t)PFDI_ACS_INVALID_PARAMETERS);
        val_print(ACS_PRINT_ERR, "on PE index %d", i);
        if (result->first_call_x0 == PFDI_ACS_NOT_SUPPORTED)
          val_print(ACS_PRINT_ERR, " (second error did not overwrite first)", 0);
        test_fail++;
      }
    }

    if (test_fail)
      val_set_status(i, RESULT_FAIL(TEST_NUM, 3));
    else if (test_skip)
      val_set_status(i, RESULT_SKIP(TEST_NUM, 1));
    else
      val_set_status(i, RESULT_PASS(TEST_NUM, 1));
  }

free_results:
  val_memory_free((void *)g_results);
}

uint32_t
pfdi027_entry(uint32_t num_pe)
{
  uint32_t status;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
    val_run_test_configurable_payload(&num_pe, payload_check_error_overwrite);

  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);
  return status;
}
