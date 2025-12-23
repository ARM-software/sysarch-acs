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

#define TEST_NUM   (ACS_PFDI_TEST_NUM_BASE + 28)
#define TEST_RULE  "R0194"
#define TEST_DESC  "Check FORCE ERROR PE locality behavior    "

static const char *pfdi_fn_names[PFDI_FN_MAX_IDX] = {
    [PFDI_FN_VERSION_IDX] = "PFDI_VERSION",
    [PFDI_FN_FEATURES_IDX] = "PFDI_FEATURES",
    [PFDI_FN_PE_TEST_ID_IDX] = "PFDI_PE_TEST_ID",
    [PFDI_FN_PE_TEST_PART_COUNT_IDX] = "PFDI_PE_TEST_PART_COUNT",
    [PFDI_FN_PE_TEST_RUN_IDX] = "PFDI_PE_TEST_RUN",
    [PFDI_FN_PE_TEST_RESULT_IDX] = "PFDI_PE_TEST_RESULT",
    [PFDI_FN_FW_CHECK_IDX] = "PFDI_FW_CHECK"
};
#define PFDI_FN_TARGET_COUNT (PFDI_FN_FW_CHECK_IDX + 1U)

/* Per-PE storage for scheduled and observed PFDI results */
typedef struct {
  int64_t x0[PFDI_FN_MAX_IDX];
} pfdi_fun_status;

typedef struct {
  pfdi_fun_status force_err;
  pfdi_fun_status cross_pe;
  pfdi_fun_status same_pe_first;
} pfdi_pe_locality_check;

/* Single buffer for PE-locality test results */
static pfdi_pe_locality_check *g_pfdi_pe_locality_check;

/* Clean+invalidate cache lines for all x0 entries in a status array */
static void
cache_clean_pfdi_status(pfdi_fun_status *status)
{
  uint32_t i;

  for (i = 0; i < PFDI_FN_TARGET_COUNT; i++)
    val_data_cache_ops_by_va((addr_t)&status->x0[i], CLEAN_AND_INVALIDATE);
}

/* Call all PFDI functions and store x0 results */
static void
call_all_pfdi_functions(pfdi_fun_status *out)
{
  out->x0[PFDI_FN_VERSION_IDX] =
      val_pfdi_version(NULL, NULL, NULL, NULL);

  out->x0[PFDI_FN_FEATURES_IDX] =
      val_pfdi_features(PFDI_FN_PFDI_VERSION, NULL, NULL, NULL, NULL);

  out->x0[PFDI_FN_PE_TEST_ID_IDX] =
      val_pfdi_pe_test_id(NULL, NULL, NULL, NULL);

  out->x0[PFDI_FN_PE_TEST_PART_COUNT_IDX] =
      val_pfdi_pe_test_part_count(NULL, NULL, NULL, NULL);

  out->x0[PFDI_FN_PE_TEST_RUN_IDX] =
      val_pfdi_pe_test_run(-1, -1, NULL, NULL, NULL, NULL);

  out->x0[PFDI_FN_PE_TEST_RESULT_IDX] =
      val_pfdi_pe_test_result(NULL, NULL, NULL, NULL);

  out->x0[PFDI_FN_FW_CHECK_IDX] =
      val_pfdi_fw_check(NULL, NULL, NULL, NULL);

  cache_clean_pfdi_status(out);
}

/* schedule injected errors on the calling PE */
static void
inject_errors_on_calling_pe(void)
{
  pfdi_fun_status *err = &g_pfdi_pe_locality_check->force_err;
  uint32_t fn_id;
  uint32_t idx;

  /* Step 1: schedule an injected error per target function on the calling PE */
  for (fn_id = PFDI_FN_PFDI_VERSION; fn_id <= PFDI_FN_PFDI_FW_CHECK; fn_id++) {
    idx = fn_id - PFDI_FN_PFDI_VERSION;
    err->x0[idx] = val_pfdi_force_error(
        fn_id, PFDI_ACS_UNKNOWN,
        NULL, NULL, NULL, NULL);

  }

  /* Make injection results visible to other PEs */
  cache_clean_pfdi_status(err);
}

/* Step 3 helper: call all target functions on the calling PE */
static void
consume_errors_on_calling_pe(void)
{
  pfdi_fun_status *same_first = &g_pfdi_pe_locality_check->same_pe_first;

  /* Step 3: consume injected errors on the calling PE */
  call_all_pfdi_functions(same_first);

}

/* Step 2: Other PE calls all PFDI functions.
 * Injected errors must NOT be visible on a different PE.
 */
static void
call_pfdi_functions_on_other_pe(void)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  pfdi_fun_status *cross_pe = &g_pfdi_pe_locality_check->cross_pe;

  call_all_pfdi_functions(cross_pe);

  val_set_status(index, RESULT_PASS(TEST_NUM, 2));
  return;
}

static void
payload_check_pe_locality(void *arg)
{
  uint32_t num_pe = *((uint32_t *)arg);
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t other_pe_index;
  uint32_t timeout;
  uint32_t i;
  uint32_t idx;
  uint32_t test_fail = 0;
  uint32_t test_skip = 0;
  pfdi_fun_status *err;
  pfdi_fun_status *cross_pe;
  pfdi_fun_status *same_first;

  if (num_pe < 2) {
    val_print(ACS_PRINT_WARN, "\n       Test requires minimum 2 PEs, skipping", 0);
    val_set_status(index, RESULT_SKIP(TEST_NUM, 1));
    return;
  }

  other_pe_index = (index == 0) ? 1 : 0;

  g_pfdi_pe_locality_check = (pfdi_pe_locality_check *)
      val_memory_calloc(1, sizeof(pfdi_pe_locality_check));
  if (g_pfdi_pe_locality_check == NULL) {
    val_print(ACS_PRINT_ERR, "\n       Memory allocation failed", 0);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
    return;
  }

  cache_clean_pfdi_status(&g_pfdi_pe_locality_check->force_err);
  cache_clean_pfdi_status(&g_pfdi_pe_locality_check->cross_pe);
  cache_clean_pfdi_status(&g_pfdi_pe_locality_check->same_pe_first);

  /*
  * Test sequence:
  * Step 1: Calling PE injects errors for all PFDI functions
  * Step 2: Other PE calls all PFDI functions and observes normal behavior
  * Step 3: Calling PE calls all PFDI functions and observes injected results
  */

  /* Step 1: inject errors on the calling PE */
  inject_errors_on_calling_pe();

  /* Step 2: run calls on another PE and wait for completion */
  val_execute_on_pe(other_pe_index, call_pfdi_functions_on_other_pe, 0);

  /* Wait for the other PE to finish its calls */
  timeout = TIMEOUT_LARGE;
  while ((--timeout) && (IS_RESULT_PENDING(val_get_status(other_pe_index))));

  if (timeout == 0) {
    val_print(ACS_PRINT_ERR, "\n       **Timed out** waiting for other PE index = %d",
              other_pe_index);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 2));
    goto free_locality_buffer;
  }

  consume_errors_on_calling_pe();

  /* Collect per-PE result buffers for validation */
  err = &g_pfdi_pe_locality_check->force_err;
  same_first = &g_pfdi_pe_locality_check->same_pe_first;
  cross_pe = &g_pfdi_pe_locality_check->cross_pe;

  cache_clean_pfdi_status(err);
  cache_clean_pfdi_status(same_first);
  cache_clean_pfdi_status(cross_pe);

  /* Validate per-target results for cross-PE isolation and same-PE delivery */
  for (i = 0; i < PFDI_FN_TARGET_COUNT; i++) {
    idx = i;

    if (err->x0[idx] == PFDI_ACS_ERROR) {
      val_print(ACS_PRINT_WARN, "\n       FORCE_ERROR %a error; skipping checks",
                (uint64_t)pfdi_fn_names[idx]);
      val_print(ACS_PRINT_WARN, " on PE index %d", index);
      test_skip++;
      continue;
    }

    if (err->x0[idx] != PFDI_ACS_SUCCESS) {
      val_print(ACS_PRINT_ERR, "\n       PFDI Force Error failed for %a",
                (uint64_t)pfdi_fn_names[idx]);
      val_print(ACS_PRINT_ERR, " x0=%ld", err->x0[idx]);
      val_print(ACS_PRINT_ERR, " on PE index %d", index);
      test_fail++;
      continue;
    }

    if (cross_pe->x0[idx] == PFDI_ACS_UNKNOWN) {
      val_print(ACS_PRINT_ERR, "\n       Other PE saw injected error for %a",
                (uint64_t)pfdi_fn_names[idx]);
      val_print(ACS_PRINT_ERR, " x0=%ld", cross_pe->x0[idx]);
      val_print(ACS_PRINT_ERR, " on PE index %d", other_pe_index);
      test_fail++;
    }

    if (same_first->x0[idx] != PFDI_ACS_UNKNOWN) {
      val_print(ACS_PRINT_ERR, "\n       Calling PE did not see injected error for %a",
                (uint64_t)pfdi_fn_names[idx]);
      val_print(ACS_PRINT_ERR, " x0=%ld", same_first->x0[idx]);
      val_print(ACS_PRINT_ERR, " on PE index %d", index);
      test_fail++;
    }

  }

  /* Report aggregate pass/fail for the calling PE */
  if (test_fail) {
    val_set_status(index, RESULT_FAIL(TEST_NUM, 3));
  } else if (test_skip) {
    val_set_status(index, RESULT_SKIP(TEST_NUM, 2));
  } else {
    val_set_status(index, RESULT_PASS(TEST_NUM, 1));
  }

free_locality_buffer:
  val_memory_free((void *)g_pfdi_pe_locality_check);
  g_pfdi_pe_locality_check = NULL;
}

uint32_t
pfdi028_entry(uint32_t num_pe)
{
  uint32_t status;
  if (num_pe > 2)
    num_pe = 2;
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
    val_run_test_configurable_payload(&num_pe, payload_check_pe_locality);

  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
