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
#include "val/include/acs_pfdi.h"
#include "val/include/acs_memory.h"
#include <stdint.h>

/* Test runs on UEFI Env */
static const
test_config_t test_entries[] = {
    /* pfdi012 targets force error injection behaviour */
    { ACS_PFDI_TEST_NUM_BASE + 12, "PFDI forced error injection               ", "R0099"},
    /* pfdi015 targets recovery after forced error */
    { ACS_PFDI_TEST_NUM_BASE + 15, "PFDI recovery after forced error          ", "R0100"}
};

enum {
  VERSION_UNSUPP = 0,
  FEATURE_UNSUPP,
  TESTID_UNKNOWN,
  TESTPART_UNSUPP,
  TESTRUN_FAULT,
  RESULT_ERROR,
  FWCHECK_UNSUPP,
  STATUS_COUNT
};

static const char *pfdi_test_names[STATUS_COUNT] = {
 [VERSION_UNSUPP]  = "Version Unsupported",
 [FEATURE_UNSUPP]  = "Feature Unsupported",
 [TESTID_UNKNOWN]  = "TEST_ID Unknown",
 [TESTPART_UNSUPP] = "TEST_PART Unsupported",
 [TESTRUN_FAULT]   = "TEST_RUN Fault",
 [RESULT_ERROR]    = "Result Error",
 [FWCHECK_UNSUPP]  = "FW_CHECK Unsupported",
};

static const int64_t fail_status[STATUS_COUNT] = {
 [VERSION_UNSUPP]  = PFDI_ACS_NOT_SUPPORTED,
 [FEATURE_UNSUPP]  = PFDI_ACS_NOT_SUPPORTED,
 [TESTID_UNKNOWN]  = PFDI_ACS_UNKNOWN,
 [TESTPART_UNSUPP] = PFDI_ACS_NOT_SUPPORTED,
 [TESTRUN_FAULT]   = PFDI_ACS_FAULT_FOUND,
 [RESULT_ERROR]    = PFDI_ACS_ERROR,
 [FWCHECK_UNSUPP]  = PFDI_ACS_NOT_SUPPORTED,
};

typedef struct {
  int64_t status[STATUS_COUNT];
  int64_t x1[STATUS_COUNT];
  int64_t x2[STATUS_COUNT];
  int64_t x3[STATUS_COUNT];
  int64_t x4[STATUS_COUNT];
} pfdi_fun_status;

typedef struct {
  pfdi_fun_status force_err;
  pfdi_fun_status pfdi_func;
} pfdi_force_error_check;

typedef struct {
  int64_t status[STATUS_COUNT];
} pfdi_err_recovery_check;

static uint32_t test_num;
pfdi_force_error_check *g_pfdi_force_error_check;
pfdi_err_recovery_check *g_pfdi_err_recovery_check;

static volatile uint32_t g_pfdi_set_status = 1;

static void pfdi_error_injection(void)
{
  uint32_t i, index = val_pe_get_index_mpid(val_pe_get_mpid());
  pfdi_force_error_check *status_buffer;
  pfdi_fun_status *err;
  pfdi_fun_status *fun;

  status_buffer = g_pfdi_force_error_check + index;
  err = &status_buffer->force_err;
  fun = &status_buffer->pfdi_func;

  /* ---- PFDI_VERSION force NOT_SUPPORTED---- */
  err->status[VERSION_UNSUPP] = val_pfdi_force_error(PFDI_FN_PFDI_VERSION, PFDI_ACS_NOT_SUPPORTED,
                                               &err->x1[VERSION_UNSUPP], &err->x2[VERSION_UNSUPP],
                                               &err->x3[VERSION_UNSUPP], &err->x4[VERSION_UNSUPP]);
  if (err->status[VERSION_UNSUPP] == PFDI_ACS_SUCCESS) {
    fun->status[VERSION_UNSUPP] = val_pfdi_version(NULL, NULL, NULL, NULL);
  }

  /* ---- PFDI_FEATURES force NOT_SUPPORTED ---- */
  err->status[FEATURE_UNSUPP] = val_pfdi_force_error(PFDI_FN_PFDI_FEATURES,
                        PFDI_ACS_NOT_SUPPORTED, &err->x1[FEATURE_UNSUPP], &err->x2[FEATURE_UNSUPP],
                          &err->x3[FEATURE_UNSUPP], &err->x4[FEATURE_UNSUPP]);
  if (err->status[FEATURE_UNSUPP] == PFDI_ACS_SUCCESS) {
    fun->status[FEATURE_UNSUPP] = val_pfdi_features(PFDI_FN_PFDI_VERSION, NULL, NULL, NULL, NULL);
  }

  /* ---- PE_TEST_ID force UNKNOWN ---- */
  err->status[TESTID_UNKNOWN] = val_pfdi_force_error(PFDI_FN_PFDI_PE_TEST_ID, PFDI_ACS_UNKNOWN,
                                               &err->x1[TESTID_UNKNOWN], &err->x2[TESTID_UNKNOWN],
                                               &err->x3[TESTID_UNKNOWN], &err->x4[TESTID_UNKNOWN]);
  if (err->status[TESTID_UNKNOWN] == PFDI_ACS_SUCCESS) {
    fun->status[TESTID_UNKNOWN] = val_pfdi_pe_test_id(NULL, NULL, NULL, NULL);
  }

  /* ---- TEST_PART_COUNT force NOT_SUPPORTED ---- */
  err->status[TESTPART_UNSUPP] = val_pfdi_force_error(PFDI_FN_PFDI_PE_TEST_PART_COUNT,
                     PFDI_ACS_NOT_SUPPORTED, &err->x1[TESTPART_UNSUPP], &err->x2[TESTPART_UNSUPP],
                       &err->x3[TESTPART_UNSUPP], &err->x4[TESTPART_UNSUPP]);
  if (err->status[TESTPART_UNSUPP] == PFDI_ACS_SUCCESS) {
    fun->status[TESTPART_UNSUPP] = val_pfdi_pe_test_part_count(NULL, NULL, NULL, NULL);
  }

  /* ---- TEST_RUN force FAULT_FOUND ---- */
  err->status[TESTRUN_FAULT] = val_pfdi_force_error(PFDI_FN_PFDI_PE_TEST_RUN, PFDI_ACS_FAULT_FOUND,
                                               &err->x1[TESTRUN_FAULT], &err->x2[TESTRUN_FAULT],
                                               &err->x3[TESTRUN_FAULT], &err->x4[TESTRUN_FAULT]);
  if (err->status[TESTRUN_FAULT] == PFDI_ACS_SUCCESS) {
    fun->status[TESTRUN_FAULT] = val_pfdi_pe_test_run(-1, -1, NULL, NULL, NULL, NULL);
  }

  /* ---- TEST_RESULT force ERROR ---- */
  err->status[RESULT_ERROR] = val_pfdi_force_error(PFDI_FN_PFDI_PE_TEST_RESULT, PFDI_ACS_ERROR,
                                               &err->x1[RESULT_ERROR], &err->x2[RESULT_ERROR],
                                               &err->x3[RESULT_ERROR], &err->x4[RESULT_ERROR]);
  if (err->status[RESULT_ERROR] == PFDI_ACS_SUCCESS) {
    fun->status[RESULT_ERROR] = val_pfdi_pe_test_result(NULL, NULL, NULL, NULL);
  }

  /* ---- FW_CHECK force NOT_SUPPORTED ---- */
  err->status[FWCHECK_UNSUPP] = val_pfdi_force_error(PFDI_FN_PFDI_FW_CHECK,
                        PFDI_ACS_NOT_SUPPORTED, &err->x1[FWCHECK_UNSUPP], &err->x2[FWCHECK_UNSUPP],
                        &err->x3[FWCHECK_UNSUPP], &err->x4[FWCHECK_UNSUPP]);
  if (err->status[FWCHECK_UNSUPP] == PFDI_ACS_SUCCESS) {
    fun->status[FWCHECK_UNSUPP] = val_pfdi_fw_check(NULL, NULL, NULL, NULL);
  }

  for (i = 0; i < STATUS_COUNT; i++) {
    val_data_cache_ops_by_va((addr_t)&err->status[i], CLEAN_AND_INVALIDATE);
    val_data_cache_ops_by_va((addr_t)&err->x1[i], CLEAN_AND_INVALIDATE);
    val_data_cache_ops_by_va((addr_t)&err->x2[i], CLEAN_AND_INVALIDATE);
    val_data_cache_ops_by_va((addr_t)&err->x3[i], CLEAN_AND_INVALIDATE);
    val_data_cache_ops_by_va((addr_t)&err->x4[i], CLEAN_AND_INVALIDATE);
    val_data_cache_ops_by_va((addr_t)&fun->status[i], CLEAN_AND_INVALIDATE);
  }

  val_data_cache_ops_by_va((addr_t)&g_pfdi_set_status, CLEAN_AND_INVALIDATE);
  if (g_pfdi_set_status)
    val_set_status(index, RESULT_PASS(test_num, 1));

  return;
}

static void pfdi_error_recovery(void)
{
  uint32_t i, index = val_pe_get_index_mpid(val_pe_get_mpid());
  pfdi_force_error_check *error_buffer;
  pfdi_err_recovery_check *recovery_buffer;

  error_buffer = g_pfdi_force_error_check + index;
  recovery_buffer = g_pfdi_err_recovery_check + index;

  /* Invoke PFDI Force Error Injection for current PE index */
  pfdi_error_injection();

  for (i = 0; i < STATUS_COUNT; i++) {
    val_data_cache_ops_by_va((addr_t)&error_buffer->force_err.status[i], CLEAN_AND_INVALIDATE);
    val_data_cache_ops_by_va((addr_t)&error_buffer->pfdi_func.status[i], CLEAN_AND_INVALIDATE);
  }

  /* ---- PFDI_VERSION should behave normally after forced error ---- */
  if (error_buffer->pfdi_func.status[VERSION_UNSUPP] == fail_status[VERSION_UNSUPP])
    recovery_buffer->status[VERSION_UNSUPP] = val_pfdi_version(NULL, NULL, NULL, NULL);

  /* ---- PFDI_FEATURES should behave normally after forced error ---- */
  if (error_buffer->pfdi_func.status[FEATURE_UNSUPP] == fail_status[FEATURE_UNSUPP])
    recovery_buffer->status[FEATURE_UNSUPP] = val_pfdi_features(PFDI_FN_PFDI_VERSION,
                                                        NULL, NULL, NULL, NULL);

  /* ---- PE_TEST_ID should behave normally after forced error ---- */
  if (error_buffer->pfdi_func.status[TESTID_UNKNOWN] == fail_status[TESTID_UNKNOWN])
    recovery_buffer->status[TESTID_UNKNOWN] = val_pfdi_pe_test_id(NULL, NULL, NULL, NULL);

  /* ---- PE_TEST_PART_COUNT should behave normally after forced error ---- */
  if (error_buffer->pfdi_func.status[TESTPART_UNSUPP] == fail_status[TESTPART_UNSUPP])
  recovery_buffer->status[TESTPART_UNSUPP] = val_pfdi_pe_test_part_count(NULL, NULL, NULL, NULL);

  /* ---- PE_TEST_RUN should behave normally after forced error ---- */
  if (error_buffer->pfdi_func.status[TESTRUN_FAULT] == fail_status[TESTRUN_FAULT])
  recovery_buffer->status[TESTRUN_FAULT] = val_pfdi_pe_test_run(-1, -1, NULL, NULL, NULL, NULL);

  /* ---- PE_TEST_RESULT should behave normally after forced error ---- */
  if (error_buffer->pfdi_func.status[RESULT_ERROR] == fail_status[RESULT_ERROR])
    recovery_buffer->status[RESULT_ERROR] = val_pfdi_pe_test_result(NULL, NULL, NULL, NULL);

  /* ---- FW_CHECK should behave normally after forced error ---- */
  if (error_buffer->pfdi_func.status[FWCHECK_UNSUPP] == fail_status[FWCHECK_UNSUPP])
    recovery_buffer->status[FWCHECK_UNSUPP] = val_pfdi_fw_check(NULL, NULL, NULL, NULL);

  for (i = 0; i < STATUS_COUNT; i++) {
    val_data_cache_ops_by_va((addr_t)&recovery_buffer->status[i], CLEAN_AND_INVALIDATE);
  }

  val_set_status(index, RESULT_PASS(test_num, 1));
  return;
}

static void payload_pfdi_error_injection(void *arg)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t timeout, i = 0, j = 0, run_fail = 0;
  pfdi_force_error_check *status_buffer;
  uint32_t num_pe = *(uint32_t *)arg;

  g_pfdi_set_status = 1;
  val_data_cache_ops_by_va((addr_t)&g_pfdi_set_status, CLEAN_AND_INVALIDATE);

  /* Allocate memory to save all PFDI function status for all PE's */
  g_pfdi_force_error_check = (pfdi_force_error_check *)
                    val_memory_calloc(num_pe, sizeof(pfdi_force_error_check));
  if (g_pfdi_force_error_check == NULL) {
    val_print(ACS_PRINT_ERR,
                "\n       Allocation for PFDI Invalid Function Check Failed", 0);
    val_set_status(index, RESULT_FAIL(test_num, 1));
    return;
  }

  for (i = 0; i < num_pe; i++) {
    status_buffer = g_pfdi_force_error_check + i;
    for (j = 0; j < STATUS_COUNT; j++) {
      val_data_cache_ops_by_va((addr_t)&status_buffer->force_err.status[j], CLEAN_AND_INVALIDATE);
      val_data_cache_ops_by_va((addr_t)&status_buffer->force_err.x1[j], CLEAN_AND_INVALIDATE);
      val_data_cache_ops_by_va((addr_t)&status_buffer->force_err.x2[j], CLEAN_AND_INVALIDATE);
      val_data_cache_ops_by_va((addr_t)&status_buffer->force_err.x3[j], CLEAN_AND_INVALIDATE);
      val_data_cache_ops_by_va((addr_t)&status_buffer->force_err.x4[j], CLEAN_AND_INVALIDATE);
      val_data_cache_ops_by_va((addr_t)&status_buffer->pfdi_func.status[j], CLEAN_AND_INVALIDATE);
    }
  }

  /* Invoke PFDI Force Error Injection for current PE index */
  pfdi_error_injection();

  /* Execute check_invalid_fn function in All PE's */
  for (i = 0; i < num_pe; i++) {
    if (i != index) {
      timeout = TIMEOUT_LARGE;
      val_execute_on_pe(i, pfdi_error_injection, 0);

      while ((--timeout) && (IS_RESULT_PENDING(val_get_status(i))));

      if (timeout == 0) {
        val_print(ACS_PRINT_ERR, "\n       **Timed out** for PE index = %d", i);
        val_set_status(i, RESULT_FAIL(test_num, 2));
        goto free_pfdi_details;
      }
    }
  }
  val_time_delay_ms(ONE_MILLISECOND);

  /* Check return status of function for all PE's */
  for (i = 0; i < num_pe; i++) {
    status_buffer = g_pfdi_force_error_check + i;
    run_fail = 0;

    for (j = 0; j < STATUS_COUNT; j++) {
      val_data_cache_ops_by_va((addr_t)&status_buffer->force_err.status[j], CLEAN_AND_INVALIDATE);
      val_data_cache_ops_by_va((addr_t)&status_buffer->force_err.x1[j], CLEAN_AND_INVALIDATE);
      val_data_cache_ops_by_va((addr_t)&status_buffer->force_err.x2[j], CLEAN_AND_INVALIDATE);
      val_data_cache_ops_by_va((addr_t)&status_buffer->force_err.x3[j], CLEAN_AND_INVALIDATE);
      val_data_cache_ops_by_va((addr_t)&status_buffer->force_err.x4[j], CLEAN_AND_INVALIDATE);
      val_data_cache_ops_by_va((addr_t)&status_buffer->pfdi_func.status[j], CLEAN_AND_INVALIDATE);

      if (status_buffer->force_err.status[j] == PFDI_ACS_ERROR) {
        val_print(ACS_PRINT_ERR, "\n       PFDI Force Error can not be scheduled for %a",
                                                    (uint64_t)pfdi_test_names[j]);
        val_print(ACS_PRINT_ERR, " status %ld", status_buffer->force_err.status[j]);
        val_print(ACS_PRINT_ERR, " on PE index %d", i);
      } else if (status_buffer->force_err.status[j] != PFDI_ACS_SUCCESS) {
        val_print(ACS_PRINT_ERR, "\n       PFDI Force Error failed for %a",
                                                    (uint64_t)pfdi_test_names[j]);
        val_print(ACS_PRINT_ERR, " err %ld", status_buffer->force_err.status[j]);
        val_print(ACS_PRINT_ERR, " on PE index %d", i);
        run_fail++;
      } else if (status_buffer->pfdi_func.status[j] != fail_status[j]) {
        val_print(ACS_PRINT_ERR, "\n       PFDI return %a", (uint64_t)pfdi_test_names[j]);
        val_print(ACS_PRINT_ERR, " check failed err  %ld", status_buffer->pfdi_func.status[j]);
        val_print(ACS_PRINT_ERR, " on PE index %d", i);
        run_fail++;
      }

      if ((status_buffer->force_err.x1[j] != 0) || (status_buffer->force_err.x2[j] != 0) ||
           (status_buffer->force_err.x3[j] != 0) || (status_buffer->force_err.x4[j] != 0)) {
        val_print(ACS_PRINT_ERR, "\n       Registers X1-X4 are not zero:", 0);
        val_print(ACS_PRINT_ERR, " x1=0x%llx", status_buffer->force_err.x1[j]);
        val_print(ACS_PRINT_ERR, " x2=0x%llx", status_buffer->force_err.x2[j]);
        val_print(ACS_PRINT_ERR, " x3=0x%llx", status_buffer->force_err.x3[j]);
        val_print(ACS_PRINT_ERR, " x4=0x%llx", status_buffer->force_err.x4[j]);
        val_print(ACS_PRINT_ERR, "\n       Failed on PE = %d", i);
        val_print(ACS_PRINT_ERR, " for %a", (uint64_t)pfdi_test_names[j]);
        run_fail++;
      }
    }

    if (run_fail)
      val_set_status(i, RESULT_FAIL(test_num, 3));
    else
      val_set_status(i, RESULT_PASS(test_num, 1));
  }

free_pfdi_details:
  val_memory_free((void *) g_pfdi_force_error_check);
  g_pfdi_force_error_check = NULL;
  return;
}

static void payload_pfdi_error_recovery_check(void *arg)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t timeout, i = 0, j = 0, run_fail = 0, run_skip = 0;
  pfdi_force_error_check *error_buffer;
  pfdi_err_recovery_check *recovery_buffer;
  uint32_t num_pe = *(uint32_t *)arg;

  g_pfdi_set_status = 0;
  val_data_cache_ops_by_va((addr_t)&g_pfdi_set_status, CLEAN_AND_INVALIDATE);

  /* Allocate memory to save Force Error status for all PE's */
  g_pfdi_force_error_check = (pfdi_force_error_check *)
                    val_memory_calloc(num_pe, sizeof(pfdi_force_error_check));
  if (g_pfdi_force_error_check == NULL) {
    val_print(ACS_PRINT_ERR,
                "\n       Allocation for PFDI Invalid Function Check Failed", 0);
    val_set_status(index, RESULT_FAIL(test_num, 1));
    return;
  }

  /* Allocate memory to save Error Recovery status for all PE's */
  g_pfdi_err_recovery_check = (pfdi_err_recovery_check *)
                    val_memory_calloc(num_pe, sizeof(pfdi_err_recovery_check));
  if (g_pfdi_err_recovery_check == NULL) {
    val_print(ACS_PRINT_ERR,
                "\n       Allocation for PFDI Invalid Function Check Failed", 0);
    val_set_status(index, RESULT_FAIL(test_num, 2));
    goto free_pfdi_force_error;
  }

  for (i = 0; i < num_pe; i++) {
    error_buffer = g_pfdi_force_error_check + i;
    recovery_buffer = g_pfdi_err_recovery_check + i;
    for (j = 0; j < STATUS_COUNT; j++) {
      val_data_cache_ops_by_va((addr_t)&error_buffer->force_err.status[j], CLEAN_AND_INVALIDATE);
      val_data_cache_ops_by_va((addr_t)&error_buffer->pfdi_func.status[j], CLEAN_AND_INVALIDATE);
      val_data_cache_ops_by_va((addr_t)&recovery_buffer->status[j], CLEAN_AND_INVALIDATE);
    }
  }

  /* Invoke PFDI Error Recovery for current PE index */
  pfdi_error_recovery();

  /* Execute check_invalid_fn function in All PE's */
  for (i = 0; i < num_pe; i++) {
    if (i != index) {
      timeout = TIMEOUT_LARGE;
      val_execute_on_pe(i, pfdi_error_recovery, 0);

      while ((--timeout) && (IS_RESULT_PENDING(val_get_status(i))));

      if (timeout == 0) {
        val_print(ACS_PRINT_ERR, "\n       **Timed out** for PE index = %d", i);
        val_set_status(i, RESULT_FAIL(test_num, 2));
        goto free_pfdi_error_recovery;
      }
    }
  }
  val_time_delay_ms(ONE_MILLISECOND);

  /* Check return status of function for all PE's */
  for (i = 0; i < num_pe; i++) {
    error_buffer = g_pfdi_force_error_check + i;
    recovery_buffer = g_pfdi_err_recovery_check + i;
    run_fail = 0;
    run_skip = 0;

    for (j = 0; j < STATUS_COUNT; j++) {
      val_data_cache_ops_by_va((addr_t)&error_buffer->force_err.status[j], CLEAN_AND_INVALIDATE);
      val_data_cache_ops_by_va((addr_t)&error_buffer->pfdi_func.status[j], CLEAN_AND_INVALIDATE);
      val_data_cache_ops_by_va((addr_t)&recovery_buffer->status[j], CLEAN_AND_INVALIDATE);

      if (error_buffer->force_err.status[j] != PFDI_ACS_SUCCESS) {
        val_print(ACS_PRINT_ERR, "\n       PFDI Force Error failed for %a",
                                                    (uint64_t)pfdi_test_names[j]);
        val_print(ACS_PRINT_ERR, " err %ld", error_buffer->force_err.status[j]);
        val_print(ACS_PRINT_ERR, " on PE index %d, Skipping the test", i);
        run_skip++;
      } else if (error_buffer->pfdi_func.status[j] != fail_status[j]) {
        val_print(ACS_PRINT_ERR, "\n       PFDI return %a", (uint64_t)pfdi_test_names[j]);
        val_print(ACS_PRINT_ERR, " check failed err  %ld", error_buffer->pfdi_func.status[j]);
        val_print(ACS_PRINT_ERR, " on PE index %d, Skipping the test", i);
        run_skip++;
      } else if (recovery_buffer->status[j] < PFDI_ACS_SUCCESS) {
        val_print(ACS_PRINT_ERR, "\n       PFDI Error Recovery for  %a",
                                                    (uint64_t)pfdi_test_names[j]);
        val_print(ACS_PRINT_ERR, " failed err  %ld", recovery_buffer->status[j]);
        val_print(ACS_PRINT_ERR, " on PE index %d", i);
        run_fail++;
      }
    }

    if (run_fail)
      val_set_status(i, RESULT_FAIL(test_num, 3));
    else if (run_skip)
      val_set_status(i, RESULT_SKIP(test_num, 1));
    else
      val_set_status(i, RESULT_PASS(test_num, 1));
  }

free_pfdi_error_recovery:
  val_memory_free((void *) g_pfdi_err_recovery_check);
  g_pfdi_err_recovery_check = NULL;
free_pfdi_force_error:
  val_memory_free((void *) g_pfdi_force_error_check);
  g_pfdi_force_error_check = NULL;

  return;
}

uint32_t pfdi012_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  test_num = test_entries[0].test_num;

  status = val_initialize_test(test_num, test_entries[0].desc, num_pe);

  if (status != ACS_STATUS_SKIP)
    val_run_test_configurable_payload(&num_pe, payload_pfdi_error_injection);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(test_num, num_pe, test_entries[0].rule);

  val_report_status(0, ACS_END(test_num), test_entries[0].rule);

  return status;
}

uint32_t pfdi015_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  test_num = test_entries[1].test_num;

  status = val_initialize_test(test_num, test_entries[1].desc, num_pe);

  if (status != ACS_STATUS_SKIP)
    val_run_test_configurable_payload(&num_pe, payload_pfdi_error_recovery_check);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(test_num, num_pe, test_entries[1].rule);

  val_report_status(0, ACS_END(test_num), test_entries[1].rule);

  return status;
}

