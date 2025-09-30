/** @file
 * Copyright (c) 2025-2026, Arm Limited or its affiliates. All rights reserved.
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

static const char *pfdi_test_names[PFDI_FN_MAX_IDX] = {
  [PFDI_FN_VERSION_IDX]  = "Version Unsupported",
  [PFDI_FN_FEATURES_IDX]  = "Feature Unsupported",
  [PFDI_FN_PE_TEST_ID_IDX]  = "TEST ID Unknown",
  [PFDI_FN_PE_TEST_PART_COUNT_IDX] = "TEST PART Unsupported",
  [PFDI_FN_PE_TEST_RUN_IDX]   = "TEST RUN Fault",
  [PFDI_FN_PE_TEST_RESULT_IDX] = "Result Error",
  [PFDI_FN_FW_CHECK_IDX]  = "FW CHECK Unsupported",
  [PFDI_FN_FORCE_ERROR_IDX] = "Force Error Unsupported",
};

static const int64_t fail_status[PFDI_FN_MAX_IDX] = {
  [PFDI_FN_VERSION_IDX]  = PFDI_ACS_NOT_SUPPORTED,
  [PFDI_FN_FEATURES_IDX]  = PFDI_ACS_NOT_SUPPORTED,
  [PFDI_FN_PE_TEST_ID_IDX]  = PFDI_ACS_UNKNOWN,
  [PFDI_FN_PE_TEST_PART_COUNT_IDX] = PFDI_ACS_NOT_SUPPORTED,
  [PFDI_FN_PE_TEST_RUN_IDX]   = PFDI_ACS_FAULT_FOUND,
  [PFDI_FN_PE_TEST_RESULT_IDX]    = PFDI_ACS_ERROR,
  [PFDI_FN_FW_CHECK_IDX]  = PFDI_ACS_NOT_SUPPORTED,
  [PFDI_FN_FORCE_ERROR_IDX] = PFDI_ACS_NOT_SUPPORTED,
};

typedef struct {
  PFDI_RET_PARAMS force_err[PFDI_FN_MAX_IDX];
} pfdi_force_error_check;

typedef struct {
  int64_t force_err_status[PFDI_FN_MAX_IDX];
  int64_t alt_status[PFDI_FN_MAX_IDX];
  PFDI_RET_PARAMS rec_status[PFDI_FN_MAX_IDX];
  int64_t norm_mode_status[PFDI_FN_MAX_IDX];
} pfdi_err_recovery_check;

static uint32_t test_num;
pfdi_force_error_check *g_pfdi_force_error_check;
pfdi_err_recovery_check *g_pfdi_err_recovery_check;

static void pfdi_error_injection(void)
{
  uint32_t i, index = val_pe_get_index_mpid(val_pe_get_mpid());
  pfdi_force_error_check *pfdi_buffer;
  PFDI_RET_PARAMS *err;

  pfdi_buffer = g_pfdi_force_error_check + index;
  err = pfdi_buffer->force_err;

  /* PFDI_VERSION force NOT_SUPPORTED */
  err[PFDI_FN_VERSION_IDX].x0 = val_pfdi_force_error(PFDI_FN_PFDI_VERSION, PFDI_ACS_NOT_SUPPORTED,
            &err[PFDI_FN_VERSION_IDX].x1, &err[PFDI_FN_VERSION_IDX].x2,
            &err[PFDI_FN_VERSION_IDX].x3, &err[PFDI_FN_VERSION_IDX].x4);
  val_pfdi_version(NULL, NULL, NULL, NULL);

  /* PFDI_FEATURES force NOT_SUPPORTED */
  err[PFDI_FN_FEATURES_IDX].x0 = val_pfdi_force_error(PFDI_FN_PFDI_FEATURES,
            PFDI_ACS_NOT_SUPPORTED, &err[PFDI_FN_FEATURES_IDX].x1, &err[PFDI_FN_FEATURES_IDX].x2,
            &err[PFDI_FN_FEATURES_IDX].x3, &err[PFDI_FN_FEATURES_IDX].x4);
  val_pfdi_features(PFDI_FN_PFDI_VERSION, NULL, NULL, NULL, NULL);

  /* PFDI_PE_TEST_ID force UNKNOWN */
  err[PFDI_FN_PE_TEST_ID_IDX].x0 = val_pfdi_force_error(PFDI_FN_PFDI_PE_TEST_ID, PFDI_ACS_UNKNOWN,
            &err[PFDI_FN_PE_TEST_ID_IDX].x1, &err[PFDI_FN_PE_TEST_ID_IDX].x2,
            &err[PFDI_FN_PE_TEST_ID_IDX].x3, &err[PFDI_FN_PE_TEST_ID_IDX].x4);
  val_pfdi_pe_test_id(NULL, NULL, NULL, NULL);

  /* PFDI_PE_TEST_PART_COUNT force NOT_SUPPORTED */
  err[PFDI_FN_PE_TEST_PART_COUNT_IDX].x0 =
            val_pfdi_force_error(PFDI_FN_PFDI_PE_TEST_PART_COUNT, PFDI_ACS_NOT_SUPPORTED,
            &err[PFDI_FN_PE_TEST_PART_COUNT_IDX].x1, &err[PFDI_FN_PE_TEST_PART_COUNT_IDX].x2,
            &err[PFDI_FN_PE_TEST_PART_COUNT_IDX].x3, &err[PFDI_FN_PE_TEST_PART_COUNT_IDX].x4);
  val_pfdi_pe_test_part_count(NULL, NULL, NULL, NULL);

  /* PFDI_PE_TEST_RUN force FAULT_FOUND */
  err[PFDI_FN_PE_TEST_RUN_IDX].x0 =
            val_pfdi_force_error(PFDI_FN_PFDI_PE_TEST_RUN, PFDI_ACS_FAULT_FOUND,
            &err[PFDI_FN_PE_TEST_RUN_IDX].x1, &err[PFDI_FN_PE_TEST_RUN_IDX].x2,
            &err[PFDI_FN_PE_TEST_RUN_IDX].x3, &err[PFDI_FN_PE_TEST_RUN_IDX].x4);
  val_pfdi_pe_test_run(PFDI_RUN_ALL_TEST_PARTS, PFDI_RUN_ALL_TEST_PARTS, NULL, NULL, NULL, NULL);

  /* PFDI_PE_TEST_RESULT force ERROR */
  err[PFDI_FN_PE_TEST_RESULT_IDX].x0 =
            val_pfdi_force_error(PFDI_FN_PFDI_PE_TEST_RESULT, PFDI_ACS_ERROR,
            &err[PFDI_FN_PE_TEST_RESULT_IDX].x1, &err[PFDI_FN_PE_TEST_RESULT_IDX].x2,
            &err[PFDI_FN_PE_TEST_RESULT_IDX].x3, &err[PFDI_FN_PE_TEST_RESULT_IDX].x4);
  val_pfdi_pe_test_result(NULL, NULL, NULL, NULL);

  /* PFDI_FW_CHECK force NOT_SUPPORTED */
  err[PFDI_FN_FW_CHECK_IDX].x0 =
            val_pfdi_force_error(PFDI_FN_PFDI_FW_CHECK,  PFDI_ACS_NOT_SUPPORTED,
            &err[PFDI_FN_FW_CHECK_IDX].x1, &err[PFDI_FN_FW_CHECK_IDX].x2,
            &err[PFDI_FN_FW_CHECK_IDX].x3, &err[PFDI_FN_FW_CHECK_IDX].x4);
  val_pfdi_fw_check(NULL, NULL, NULL, NULL);

  /* PFDI_FORCE_ERROR force NOT_SUPPORTED */
  err[PFDI_FN_FORCE_ERROR_IDX].x0 =
            val_pfdi_force_error(PFDI_FN_PFDI_FORCE_ERROR, PFDI_ACS_NOT_SUPPORTED,
            &err[PFDI_FN_FORCE_ERROR_IDX].x1, &err[PFDI_FN_FORCE_ERROR_IDX].x2,
            &err[PFDI_FN_FORCE_ERROR_IDX].x3, &err[PFDI_FN_FORCE_ERROR_IDX].x4);
  val_pfdi_force_error(PFDI_FN_PFDI_VERSION, PFDI_ACS_NOT_SUPPORTED,
            NULL, NULL, NULL, NULL);

  for (i = 0; i < PFDI_FN_MAX_IDX; i++) {
    val_pfdi_invalidate_ret_params(&err[i]);
  }

  val_set_status(index, RESULT_PASS(test_num, 1));
  return;
}

static void pfdi_error_recovery(void)
{
  uint32_t i, index = val_pe_get_index_mpid(val_pe_get_mpid());
  pfdi_err_recovery_check *rec_buffer;
  PFDI_RET_PARAMS *rec;

  rec_buffer = g_pfdi_err_recovery_check + index;
  rec = rec_buffer->rec_status;

  /* PFDI_VERSION should behave normally after forced error */
  rec_buffer->force_err_status[PFDI_FN_VERSION_IDX] =
            val_pfdi_force_error(PFDI_FN_PFDI_VERSION, PFDI_ACS_NOT_SUPPORTED,
            NULL, NULL, NULL, NULL);
  if (rec_buffer->force_err_status[PFDI_FN_VERSION_IDX] == PFDI_ACS_SUCCESS) {
    /* Error should remain pending until PFDI_VERSION is called */
    rec_buffer->alt_status[PFDI_FN_VERSION_IDX] =
            val_pfdi_features(PFDI_FN_PFDI_VERSION, NULL, NULL, NULL, NULL);
    rec[PFDI_FN_VERSION_IDX].x0 = val_pfdi_version(&rec[PFDI_FN_VERSION_IDX].x1,
        &rec[PFDI_FN_VERSION_IDX].x2, &rec[PFDI_FN_VERSION_IDX].x3, &rec[PFDI_FN_VERSION_IDX].x4);
    /* Normal behaviour of PFDI_VERSION function after injected error */
    rec_buffer->norm_mode_status[PFDI_FN_VERSION_IDX] = val_pfdi_version(NULL, NULL, NULL, NULL);
  }

  /* PFDI_FEATURES should behave normally after forced error */
  rec_buffer->force_err_status[PFDI_FN_FEATURES_IDX] =
            val_pfdi_force_error(PFDI_FN_PFDI_FEATURES, PFDI_ACS_NOT_SUPPORTED,
            NULL, NULL, NULL, NULL);
  if (rec_buffer->force_err_status[PFDI_FN_FEATURES_IDX] == PFDI_ACS_SUCCESS) {
    /* Error should remain pending until PFDI_FEATURES is called */
    rec_buffer->alt_status[PFDI_FN_FEATURES_IDX] = val_pfdi_version(NULL, NULL, NULL, NULL);
    rec[PFDI_FN_FEATURES_IDX].x0 = val_pfdi_features(PFDI_FN_PFDI_VERSION,
                &rec[PFDI_FN_FEATURES_IDX].x1, &rec[PFDI_FN_FEATURES_IDX].x2,
                &rec[PFDI_FN_FEATURES_IDX].x3, &rec[PFDI_FN_FEATURES_IDX].x4);
    /* Normal behaviour of PFDI_FEATURES function after injected error */
    rec_buffer->norm_mode_status[PFDI_FN_FEATURES_IDX] =
                val_pfdi_features(PFDI_FN_PFDI_VERSION, NULL, NULL, NULL, NULL);
  }

  /* PE_TEST_ID should behave normally after forced error */
    rec_buffer->force_err_status[PFDI_FN_PE_TEST_ID_IDX] =
                val_pfdi_force_error(PFDI_FN_PFDI_PE_TEST_ID, PFDI_ACS_UNKNOWN,
                NULL, NULL, NULL, NULL);
  if (rec_buffer->force_err_status[PFDI_FN_PE_TEST_ID_IDX] == PFDI_ACS_SUCCESS) {
    /* Error should remain pending until PFDI_PE_TEST_ID is called */
    rec_buffer->alt_status[PFDI_FN_PE_TEST_ID_IDX] = val_pfdi_version(NULL, NULL, NULL, NULL);
    rec[PFDI_FN_PE_TEST_ID_IDX].x0 =
            val_pfdi_pe_test_id(&rec[PFDI_FN_PE_TEST_ID_IDX].x1, &rec[PFDI_FN_PE_TEST_ID_IDX].x2,
            &rec[PFDI_FN_PE_TEST_ID_IDX].x3, &rec[PFDI_FN_PE_TEST_ID_IDX].x4);
    /* Normal behaviour of PFDI_PE_TEST_ID function after injected error */
    rec_buffer->norm_mode_status[PFDI_FN_PE_TEST_ID_IDX] =
            val_pfdi_pe_test_id(NULL, NULL, NULL, NULL);
  }

  /* PE_TEST_PART_COUNT should behave normally after forced error */
  rec_buffer->force_err_status[PFDI_FN_PE_TEST_PART_COUNT_IDX] =
                val_pfdi_force_error(PFDI_FN_PFDI_PE_TEST_PART_COUNT, PFDI_ACS_NOT_SUPPORTED,
                NULL, NULL, NULL, NULL);
  if (rec_buffer->force_err_status[PFDI_FN_PE_TEST_PART_COUNT_IDX] == PFDI_ACS_SUCCESS) {
    /* Error should remain pending until PFDI_PE_TEST_PART_COUNT is called */
    rec_buffer->alt_status[PFDI_FN_PE_TEST_PART_COUNT_IDX] =
                val_pfdi_version(NULL, NULL, NULL, NULL);
    rec[PFDI_FN_PE_TEST_PART_COUNT_IDX].x0 =
            val_pfdi_pe_test_part_count(&rec[PFDI_FN_PE_TEST_PART_COUNT_IDX].x1,
            &rec[PFDI_FN_PE_TEST_PART_COUNT_IDX].x2, &rec[PFDI_FN_PE_TEST_PART_COUNT_IDX].x3,
            &rec[PFDI_FN_PE_TEST_PART_COUNT_IDX].x4);
    /* Normal behaviour of PFDI_PE_TEST_PART_COUNT function after injected error */
    rec_buffer->norm_mode_status[PFDI_FN_PE_TEST_PART_COUNT_IDX] =
            val_pfdi_pe_test_part_count(NULL, NULL, NULL, NULL);
  }

  /* PE_TEST_RUN should behave normally after forced error */
    rec_buffer->force_err_status[PFDI_FN_PE_TEST_RUN_IDX] =
                    val_pfdi_force_error(PFDI_FN_PFDI_PE_TEST_RUN, PFDI_ACS_FAULT_FOUND,
                    NULL, NULL, NULL, NULL);
  if (rec_buffer->force_err_status[PFDI_FN_PE_TEST_RUN_IDX] == PFDI_ACS_SUCCESS) {
    /* Error should remain pending until PFDI_PE_TEST_RUN is called */
    rec_buffer->alt_status[PFDI_FN_PE_TEST_RUN_IDX] = val_pfdi_version(NULL, NULL, NULL, NULL);
    rec[PFDI_FN_PE_TEST_RUN_IDX].x0 =
            val_pfdi_pe_test_run(PFDI_RUN_ALL_TEST_PARTS, PFDI_RUN_ALL_TEST_PARTS,
            &rec[PFDI_FN_PE_TEST_RUN_IDX].x1, &rec[PFDI_FN_PE_TEST_RUN_IDX].x2,
            &rec[PFDI_FN_PE_TEST_RUN_IDX].x3, &rec[PFDI_FN_PE_TEST_RUN_IDX].x4);
    /* Normal behaviour of PFDI_PE_TEST_RUN function after injected error */
    rec_buffer->norm_mode_status[PFDI_FN_PE_TEST_RUN_IDX] =
            val_pfdi_pe_test_run(PFDI_RUN_ALL_TEST_PARTS, PFDI_RUN_ALL_TEST_PARTS,
            NULL, NULL, NULL, NULL);
  }

  /* PE_TEST_RESULT should behave normally after forced error */
  rec_buffer->force_err_status[PFDI_FN_PE_TEST_RESULT_IDX] =
                    val_pfdi_force_error(PFDI_FN_PFDI_PE_TEST_RESULT, PFDI_ACS_ERROR,
                    NULL, NULL, NULL, NULL);
  if (rec_buffer->force_err_status[PFDI_FN_PE_TEST_RESULT_IDX] == PFDI_ACS_SUCCESS) {
    /* Error should remain pending until PFDI_PE_TEST_RESULT is called */
    rec_buffer->alt_status[PFDI_FN_PE_TEST_RESULT_IDX] = val_pfdi_version(NULL, NULL, NULL, NULL);
    rec[PFDI_FN_PE_TEST_RESULT_IDX].x0 =
            val_pfdi_pe_test_result(&rec[PFDI_FN_PE_TEST_RESULT_IDX].x1,
            &rec[PFDI_FN_PE_TEST_RESULT_IDX].x2, &rec[PFDI_FN_PE_TEST_RESULT_IDX].x3,
            &rec[PFDI_FN_PE_TEST_RESULT_IDX].x4);
    /* Normal behaviour of PFDI_PE_TEST_RESULT function after injected error */
    rec_buffer->norm_mode_status[PFDI_FN_PE_TEST_RESULT_IDX] =
            val_pfdi_pe_test_result(NULL, NULL, NULL, NULL);
  }

  /* FW_CHECK should behave normally after forced error */
  rec_buffer->force_err_status[PFDI_FN_FW_CHECK_IDX] =
                val_pfdi_force_error(PFDI_FN_PFDI_FW_CHECK, PFDI_ACS_NOT_SUPPORTED,
                NULL, NULL, NULL, NULL);
  if (rec_buffer->force_err_status[PFDI_FN_FW_CHECK_IDX] == PFDI_ACS_SUCCESS) {
    /* Error should remain pending until PFDI_FW_CHECK is called */
    rec_buffer->alt_status[PFDI_FN_FW_CHECK_IDX] = val_pfdi_version(NULL, NULL, NULL, NULL);
    rec[PFDI_FN_FW_CHECK_IDX].x0 =
            val_pfdi_fw_check(&rec[PFDI_FN_FW_CHECK_IDX].x1, &rec[PFDI_FN_FW_CHECK_IDX].x2,
            &rec[PFDI_FN_FW_CHECK_IDX].x3, &rec[PFDI_FN_FW_CHECK_IDX].x4);
    /* Normal behaviour of PFDI_FW_CHECK function after injected error */
    rec_buffer->norm_mode_status[PFDI_FN_FW_CHECK_IDX] =
            val_pfdi_fw_check(NULL, NULL, NULL, NULL);
  }

  /* FORCE_ERROR should behave normally after forced error */
  rec_buffer->force_err_status[PFDI_FN_FORCE_ERROR_IDX] =
                    val_pfdi_force_error(PFDI_FN_PFDI_FORCE_ERROR, PFDI_ACS_NOT_SUPPORTED,
                    NULL, NULL, NULL, NULL);
  if (rec_buffer->force_err_status[PFDI_FN_FORCE_ERROR_IDX] == PFDI_ACS_SUCCESS) {
    /* Error should remain pending until PFDI_FORCE_ERROR is called */
    rec_buffer->alt_status[PFDI_FN_FORCE_ERROR_IDX] = val_pfdi_version(NULL, NULL, NULL, NULL);
    rec[PFDI_FN_FORCE_ERROR_IDX].x0 = val_pfdi_force_error(PFDI_FN_PFDI_FORCE_ERROR, 0,
        &rec[PFDI_FN_FORCE_ERROR_IDX].x1, &rec[PFDI_FN_FORCE_ERROR_IDX].x2,
        &rec[PFDI_FN_FORCE_ERROR_IDX].x3, &rec[PFDI_FN_FORCE_ERROR_IDX].x4);
    /* Normal behaviour of PFDI_FORCE_ERROR function after injected error */
    rec_buffer->norm_mode_status[PFDI_FN_FORCE_ERROR_IDX] =
            val_pfdi_force_error(PFDI_FN_PFDI_FORCE_ERROR, 0, NULL, NULL, NULL, NULL);
  }

  for (i = 0; i < PFDI_FN_MAX_IDX; i++) {
    val_data_cache_ops_by_va((addr_t)&rec_buffer->force_err_status[i], CLEAN_AND_INVALIDATE);
    val_data_cache_ops_by_va((addr_t)&rec_buffer->alt_status[i], CLEAN_AND_INVALIDATE);
    val_pfdi_invalidate_ret_params(&rec[i]);
    val_data_cache_ops_by_va((addr_t)&rec_buffer->norm_mode_status[i], CLEAN_AND_INVALIDATE);
  }

  val_set_status(index, RESULT_PASS(test_num, 1));
  return;
}

static void payload_pfdi_error_injection(void *arg)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t timeout = 0, i = 0, j = 0, run_fail = 0;
  pfdi_force_error_check *pfdi_buffer;
  uint32_t num_pe = *(uint32_t *)arg;

  /* Allocate memory to save all PFDI function status for all PE's */
  g_pfdi_force_error_check = (pfdi_force_error_check *)
                    val_memory_calloc(num_pe, sizeof(pfdi_force_error_check));
  if (g_pfdi_force_error_check == NULL) {
    val_print(ACS_PRINT_ERR,
                "\n       Allocation for PFDI Force Error Check Failed", 0);
    val_set_status(index, RESULT_FAIL(test_num, 1));
    return;
  }

  for (i = 0; i < num_pe; i++) {
    pfdi_buffer = g_pfdi_force_error_check + i;
    for (j = 0; j < PFDI_FN_MAX_IDX; j++) {
      val_pfdi_invalidate_ret_params(&pfdi_buffer->force_err[j]);
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
    pfdi_buffer = g_pfdi_force_error_check + i;
    run_fail = 0;

    for (j = 0; j < PFDI_FN_MAX_IDX; j++) {
      val_pfdi_invalidate_ret_params(&pfdi_buffer->force_err[j]);

      if (pfdi_buffer->force_err[j].x0 == PFDI_ACS_ERROR) {
        val_print(ACS_PRINT_ERR, "\n       PFDI Force Error can not be scheduled for %a",
                                                    (uint64_t)pfdi_test_names[j]);
        val_print(ACS_PRINT_ERR, " status %ld", pfdi_buffer->force_err[j].x0);
        val_print(ACS_PRINT_ERR, " on PE index %d", i);
      } else if (pfdi_buffer->force_err[j].x0 != PFDI_ACS_SUCCESS) {
        val_print(ACS_PRINT_ERR, "\n       PFDI Force Error failed for %a",
                                                    (uint64_t)pfdi_test_names[j]);
        val_print(ACS_PRINT_ERR, " err %ld", pfdi_buffer->force_err[j].x0);
        val_print(ACS_PRINT_ERR, " on PE index %d", i);
        run_fail++;
      }

      if ((pfdi_buffer->force_err[j].x1 != 0) || (pfdi_buffer->force_err[j].x2 != 0) ||
           (pfdi_buffer->force_err[j].x3 != 0) || (pfdi_buffer->force_err[j].x4 != 0)) {
        val_print(ACS_PRINT_ERR, "\n       Registers X1-X4 are not zero:", 0);
        val_print(ACS_PRINT_ERR, " x1=0x%llx", pfdi_buffer->force_err[j].x1);
        val_print(ACS_PRINT_ERR, " x2=0x%llx", pfdi_buffer->force_err[j].x2);
        val_print(ACS_PRINT_ERR, " x3=0x%llx", pfdi_buffer->force_err[j].x3);
        val_print(ACS_PRINT_ERR, " x4=0x%llx", pfdi_buffer->force_err[j].x4);
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
  uint32_t timeout = 0, i = 0, j = 0, run_fail = 0, run_skip = 0;
  pfdi_err_recovery_check *rec_buffer;
  uint32_t num_pe = *(uint32_t *)arg;

  /* Allocate memory to save Error Recovery status for all PE's */
  g_pfdi_err_recovery_check = (pfdi_err_recovery_check *)
                    val_memory_calloc(num_pe, sizeof(pfdi_err_recovery_check));
  if (g_pfdi_err_recovery_check == NULL) {
    val_print(ACS_PRINT_ERR,
                "\n       Allocation for PFDI Error Recovery Check Failed", 0);
    val_set_status(index, RESULT_FAIL(test_num, 2));
    return;
  }

  for (i = 0; i < num_pe; i++) {
    rec_buffer = g_pfdi_err_recovery_check + i;
    for (j = 0; j < PFDI_FN_MAX_IDX; j++) {
      val_data_cache_ops_by_va((addr_t)&rec_buffer->force_err_status[j], CLEAN_AND_INVALIDATE);
      val_data_cache_ops_by_va((addr_t)&rec_buffer->alt_status[j], CLEAN_AND_INVALIDATE);
      val_pfdi_invalidate_ret_params(&rec_buffer->rec_status[j]);
      val_data_cache_ops_by_va((addr_t)&rec_buffer->norm_mode_status[j], CLEAN_AND_INVALIDATE);
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
    rec_buffer = g_pfdi_err_recovery_check + i;
    run_fail = 0;
    run_skip = 0;

    for (j = 0; j < PFDI_FN_MAX_IDX; j++) {
      val_data_cache_ops_by_va((addr_t)&rec_buffer->force_err_status[j], CLEAN_AND_INVALIDATE);
      val_data_cache_ops_by_va((addr_t)&rec_buffer->alt_status[j], CLEAN_AND_INVALIDATE);
      val_pfdi_invalidate_ret_params(&rec_buffer->rec_status[j]);
      val_data_cache_ops_by_va((addr_t)&rec_buffer->norm_mode_status[j], CLEAN_AND_INVALIDATE);

      if (rec_buffer->force_err_status[j] != PFDI_ACS_SUCCESS) {
        val_print(ACS_PRINT_ERR, "\n       PFDI Force Error failed for %a",
                                                    (uint64_t)pfdi_test_names[j]);
        val_print(ACS_PRINT_ERR, " err %ld", rec_buffer->force_err_status[j]);
        val_print(ACS_PRINT_ERR, " on PE index %d, Skipping the test", i);
        run_skip++;
      }

      if (rec_buffer->alt_status[j] == fail_status[j]) {
        val_print(ACS_PRINT_ERR,
            "\n       PFDI Scheduled error affected unrelated function, return status %ld",
            rec_buffer->alt_status[j]);
        run_fail++;
      }

      if (rec_buffer->rec_status[j].x0 != fail_status[j]) {
        val_print(ACS_PRINT_ERR, "\n       PFDI return %a", (uint64_t)pfdi_test_names[j]);
        val_print(ACS_PRINT_ERR, " check failed err  %ld", rec_buffer->rec_status[j].x0);
        val_print(ACS_PRINT_ERR, " on PE index %d", i);
        run_fail++;
      }

      if (rec_buffer->norm_mode_status[j] == fail_status[j]) {
        val_print(ACS_PRINT_ERR, "\n       PFDI Error recovery failed for  %a",
                                                    (uint64_t)pfdi_test_names[j]);
        val_print(ACS_PRINT_ERR, " failed err  %ld", rec_buffer->norm_mode_status[j]);
        val_print(ACS_PRINT_ERR, " on PE index %d", i);
        run_fail++;
      }

      if ((rec_buffer->rec_status[j].x1 != 0) || (rec_buffer->rec_status[j].x2 != 0) ||
           (rec_buffer->rec_status[j].x3 != 0) || (rec_buffer->rec_status[j].x4 != 0)) {
        val_print(ACS_PRINT_ERR, "\n       Registers X1-X4 are not zero:", 0);
        val_print(ACS_PRINT_ERR, " x1=0x%llx", rec_buffer->rec_status[j].x1);
        val_print(ACS_PRINT_ERR, " x2=0x%llx", rec_buffer->rec_status[j].x2);
        val_print(ACS_PRINT_ERR, " x3=0x%llx", rec_buffer->rec_status[j].x3);
        val_print(ACS_PRINT_ERR, " x4=0x%llx", rec_buffer->rec_status[j].x4);
        val_print(ACS_PRINT_ERR, "\n       Failed on PE = %d", i);
        val_print(ACS_PRINT_ERR, " for %a", (uint64_t)pfdi_test_names[j]);
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

