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
#include "val/include/acs_memory.h"

#define TEST_NUM   (ACS_PFDI_TEST_NUM_BASE + 7)
#define TEST_RULE  "R0076"
#define TEST_DESC  "Execute Test Parts and All Parts on PE    "

#define RUN_ALL_TEST_PARTS -1

static PFDI_RET_PARAMS *g_pfdi_range_status;
static PFDI_RET_PARAMS *g_pfdi_all_parts_status;

static void
pfdi_test_run(void)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  int64_t test_parts;
  int64_t end;
  PFDI_RET_PARAMS *pfdi_range, *pfdi_all;

  pfdi_range = g_pfdi_range_status + index;
  pfdi_all   = g_pfdi_all_parts_status + index;

  /* Run all test parts first, then retrieve test part count for range execution */
  pfdi_all->x0 = val_pfdi_pe_test_run(RUN_ALL_TEST_PARTS, RUN_ALL_TEST_PARTS,
                  &pfdi_all->x1, &pfdi_all->x2, &pfdi_all->x3, &pfdi_all->x4);
  val_pfdi_invalidate_ret_params(pfdi_all);

  /* Now get Num of Test parts supported on current PE */
  test_parts = val_pfdi_pe_test_part_count(NULL, NULL, NULL, NULL);
  if (test_parts < PFDI_ACS_SUCCESS) {
    pfdi_range->x0 = test_parts;
    val_pfdi_invalidate_ret_params(pfdi_range);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 6));
    return;
  }

  end = (test_parts == 0) ? 0 : (test_parts - 1);
  /* Run range */
  pfdi_range->x0 = val_pfdi_pe_test_run(0, end,
                    &pfdi_range->x1, &pfdi_range->x2, &pfdi_range->x3, &pfdi_range->x4);
  val_pfdi_invalidate_ret_params(pfdi_range);

  val_set_status(index, RESULT_PASS(TEST_NUM, 1));
}

static void payload_run(void *arg)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t timeout, i = 0, test_fail = 0;
  uint32_t num_pe = *(uint32_t *)arg;
  PFDI_RET_PARAMS *pfdi_range_buffer;
  PFDI_RET_PARAMS *pfdi_all_parts_buffer;

  /* Allocate memory to save all PFDI run status and fault id's for all PE's */
  g_pfdi_range_status =
      (PFDI_RET_PARAMS *) val_memory_calloc(num_pe, sizeof(PFDI_RET_PARAMS));
  if (g_pfdi_range_status == NULL) {
    val_print(ACS_PRINT_ERR, "\n       Allocation for g_pfdi_range_status Failed", 0);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
    return;
  }

  g_pfdi_all_parts_status =
      (PFDI_RET_PARAMS *) val_memory_calloc(num_pe, sizeof(PFDI_RET_PARAMS));
  if (g_pfdi_all_parts_status == NULL) {
    val_print(ACS_PRINT_ERR, "\n       Allocation for g_pfdi_all_parts_status Failed", 0);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 2));
    goto free_pfdi_details_range;
  }

  /* Invalidate both buffers in one pass */
  for (i = 0; i < num_pe; i++) {
    val_pfdi_invalidate_ret_params(g_pfdi_range_status + i);
    val_pfdi_invalidate_ret_params(g_pfdi_all_parts_status + i);
  }

  /* Run tests on primary PE, then on all others */
  pfdi_test_run();
  for (i = 0; i < num_pe; i++) {
    if (i != index) {
      timeout = TIMEOUT_LARGE;
      val_execute_on_pe(i, pfdi_test_run, 0);

      while ((--timeout) && (IS_RESULT_PENDING(val_get_status(i))));
      if (timeout == 0) {
        val_print(ACS_PRINT_ERR, "\n       **Timed out** for PE index = %d", i);
        val_set_status(i, RESULT_FAIL(TEST_NUM, 3));
        goto free_pfdi_details_both;
      }
    }
  }
  val_time_delay_ms(ONE_MILLISECOND);

  for (i = 0; i < num_pe; i++) {
    pfdi_all_parts_buffer = g_pfdi_all_parts_status + i;
    pfdi_range_buffer     = g_pfdi_range_status + i;
    val_pfdi_invalidate_ret_params(pfdi_all_parts_buffer);
    val_pfdi_invalidate_ret_params(pfdi_range_buffer);
    test_fail = 0;

    /* All-parts validation */
    if (pfdi_all_parts_buffer->x0 == PFDI_ACS_SUCCESS) {
      if (pfdi_all_parts_buffer->x1 != 0) {
        val_print(ACS_PRINT_ERR, "\n       X1 not zero for SUCCESS (all-parts): x1=0x%llx",
                  pfdi_all_parts_buffer->x1);
        val_print(ACS_PRINT_ERR, " on PE %d", i);
        test_fail++;
      }

    } else if (pfdi_all_parts_buffer->x0 == PFDI_ACS_FAULT_FOUND) {
      /* FAULT_FOUND: x1 contains the faulty test part index (zero is valid) */
      if (pfdi_all_parts_buffer->x1 == PFDI_ACS_UNKNOWN) {
        val_print(ACS_PRINT_ERR, "\n       Fault in test part (all-parts) on PE %d ", i);
        val_print(ACS_PRINT_ERR, "cannot be identified", 0);
      } else {
        val_print(ACS_PRINT_ERR, "\n       Test part %lld (all-parts) ", pfdi_all_parts_buffer->x1);
        val_print(ACS_PRINT_ERR, "triggered the fault on PE %d", i);
      }

    } else if (pfdi_all_parts_buffer->x0 == PFDI_ACS_ERROR) {
      if (pfdi_all_parts_buffer->x1 != 0) {
        val_print(ACS_PRINT_ERR, "\n       X1 not zero for ERROR (all-parts): x1=0x%llx",
                  pfdi_all_parts_buffer->x1);
        val_print(ACS_PRINT_ERR, " on PE %d", i);
        test_fail++;
      }
      val_print(ACS_PRINT_ERR, "\n       All-parts execution failed on PE index %d", i);

    } else {
      val_print(ACS_PRINT_ERR, "\n       All-parts run returned unexpected x0 on PE index %d", i);
      test_fail++;
    }

    /* Check X2-X4 are zero for all-parts execution */
    if (pfdi_all_parts_buffer->x2 || pfdi_all_parts_buffer->x3 ||
        pfdi_all_parts_buffer->x4) {
      val_print(ACS_PRINT_ERR, "\n       X2-X4 not zero (all-parts):", 0);
      val_print(ACS_PRINT_ERR, " x2=0x%llx", pfdi_all_parts_buffer->x2);
      val_print(ACS_PRINT_ERR, " x3=0x%llx", pfdi_all_parts_buffer->x3);
      val_print(ACS_PRINT_ERR, " x4=0x%llx", pfdi_all_parts_buffer->x4);
      val_print(ACS_PRINT_ERR, " on PE %d", i);
      test_fail++;
    }

    /* Check if test part count acquisition failed on this PE */
    if (IS_TEST_FAIL(val_get_status(i))) {
      val_print(ACS_PRINT_ERR, "\n       Failed to get Test Part count on PE %d ", i);
      test_fail++;
      continue;
    }

    /* Range validation */
    if (pfdi_range_buffer->x0 == PFDI_ACS_SUCCESS) {
      if (pfdi_range_buffer->x1 != 0) {
        val_print(ACS_PRINT_ERR, "\n       X1 not zero for SUCCESS (range): x1=0x%llx",
                  pfdi_range_buffer->x1);
        val_print(ACS_PRINT_ERR, " on PE %d", i);
        test_fail++;
      }

    } else if (pfdi_range_buffer->x0 == PFDI_ACS_FAULT_FOUND) {
      /* FAULT_FOUND: x1 contains the faulty test part index (zero is valid) */
      if (pfdi_range_buffer->x1 == PFDI_ACS_UNKNOWN) {
        val_print(ACS_PRINT_ERR, "\n       Fault in test part (range) on PE %d ", i);
        val_print(ACS_PRINT_ERR, "cannot be identified", 0);
      } else {
        val_print(ACS_PRINT_ERR, "\n       Test part %lld (range) ", pfdi_range_buffer->x1);
        val_print(ACS_PRINT_ERR, "triggered the fault on PE %d", i);
      }

    } else if (pfdi_range_buffer->x0 == PFDI_ACS_ERROR) {
      if (pfdi_range_buffer->x1 != 0) {
        val_print(ACS_PRINT_ERR, "\n       X1 not zero for ERROR (range): x1=0x%llx",
                  pfdi_range_buffer->x1);
        val_print(ACS_PRINT_ERR, " on PE %d", i);
        test_fail++;
      }
      val_print(ACS_PRINT_ERR, "\n       Range execution failed to complete on PE %d", i);

    } else {
      val_print(ACS_PRINT_ERR, "\n       Range run returned unexpected x0 on PE %d", i);
      test_fail++;
    }

    /* Check X2-X4 are zero for range execution */
    if (pfdi_range_buffer->x2 || pfdi_range_buffer->x3 ||
        pfdi_range_buffer->x4) {
      val_print(ACS_PRINT_ERR, "\n       X2-X4 not zero (range):", 0);
      val_print(ACS_PRINT_ERR, " x2=0x%llx", pfdi_range_buffer->x2);
      val_print(ACS_PRINT_ERR, " x3=0x%llx", pfdi_range_buffer->x3);
      val_print(ACS_PRINT_ERR, " x4=0x%llx", pfdi_range_buffer->x4);
      val_print(ACS_PRINT_ERR, " on PE %d", i);
      test_fail++;
    }

    if (test_fail)
      val_set_status(i, RESULT_FAIL(TEST_NUM, 5));
    else
      val_set_status(i, RESULT_PASS(TEST_NUM, 1));
  }

free_pfdi_details_both:
  val_memory_free((void *) g_pfdi_all_parts_status);

free_pfdi_details_range:
  val_memory_free((void *) g_pfdi_range_status);
}

uint32_t pfdi007_entry(uint32_t num_pe)
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
