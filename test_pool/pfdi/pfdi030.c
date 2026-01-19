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
#include "val/include/acs_memory.h"
#include "val/include/val_interface.h"

#define TEST_NUM   (ACS_PFDI_TEST_NUM_BASE + 30)
#define TEST_RULE  "R0180"
#define TEST_DESC  "PFDI Force Error invalid params check     "

#define NUM_REGS_TO_CHECK 2

typedef struct{
  PFDI_RET_PARAMS inval[NUM_REGS_TO_CHECK];
} PFDI_INVAL_RETURNS;

PFDI_INVAL_RETURNS *g_pfdi_invalid_force_error;

static void
pfdi_invalid_force_error_check(void)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid()), i;
  PFDI_INVAL_RETURNS *pfdi_buffer;

  pfdi_buffer = g_pfdi_invalid_force_error + index;

  /* Invoke PFDI Force Error function for current PE index with invalid x3 parameter*/
  pfdi_buffer->inval[0].x0 = val_invoke_pfdi_fn(
            PFDI_FN_PFDI_FORCE_ERROR, PFDI_FN_PFDI_FORCE_ERROR,
            PFDI_ACS_UNKNOWN, PFDI_INVALID_X_VALUE, 0, 0,
            (unsigned long *)&pfdi_buffer->inval[0].x1, (unsigned long *)&pfdi_buffer->inval[0].x2,
            (unsigned long *)&pfdi_buffer->inval[0].x3, (unsigned long *)&pfdi_buffer->inval[0].x4);

  /* Invoke PFDI Force Error function for current PE index with invalid x4 parameter*/
  pfdi_buffer->inval[1].x0 = val_invoke_pfdi_fn(
            PFDI_FN_PFDI_FORCE_ERROR, PFDI_FN_PFDI_FORCE_ERROR,
            PFDI_ACS_UNKNOWN, 0, PFDI_INVALID_X_VALUE, 0,
            (unsigned long *)&pfdi_buffer->inval[1].x1, (unsigned long *)&pfdi_buffer->inval[1].x2,
            (unsigned long *)&pfdi_buffer->inval[1].x3, (unsigned long *)&pfdi_buffer->inval[1].x4);

  for (i = 0; i < NUM_REGS_TO_CHECK; i++) {
    val_pfdi_invalidate_ret_params(&pfdi_buffer->inval[i]);
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 1));
  return;
}

static void payload_invalid_force_error_check(void *arg)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t num_pe = *((uint32_t *)arg);
  uint32_t timeout = 0, i = 0, num_regs = 0;
  uint32_t test_fail = 0;
  uint32_t inval_case = 0;
  PFDI_INVAL_RETURNS *pfdi_buffer;

  /* Allocate memory to save all PFDI Force Error or status for all PE's */
  g_pfdi_invalid_force_error = (PFDI_INVAL_RETURNS *)
                            val_memory_calloc(num_pe, sizeof(PFDI_INVAL_RETURNS));
  if (g_pfdi_invalid_force_error == NULL) {
    val_print(ACS_PRINT_ERR, "\n       Allocation for PFDI Invalid Force Error Failed \n", 0);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
    return;
  }

  for (i = 0; i < num_pe; i++) {
    pfdi_buffer = g_pfdi_invalid_force_error + i;
    for (num_regs = 0; num_regs < NUM_REGS_TO_CHECK; num_regs++) {
      val_pfdi_invalidate_ret_params(&pfdi_buffer->inval[num_regs]);
    }
  }

  /* Invoke pfdi_invalid_force_error_check for current PE index */
  pfdi_invalid_force_error_check();

  /* Execute pfdi_invalid_force_error_check function in All PE's */
  for (i = 0; i < num_pe; i++) {
    if (i != index) {
      timeout = TIMEOUT_LARGE;
      val_execute_on_pe(i, pfdi_invalid_force_error_check, 0);

      while ((--timeout) && (IS_RESULT_PENDING(val_get_status(i))));

      if (timeout == 0) {
        val_print(ACS_PRINT_ERR, "\n       **Timed out** for PE index = %d", i);
        val_set_status(i, RESULT_FAIL(TEST_NUM, 2));
        goto free_pfdi_details;
      }
    }
  }

  for (i = 0; i < num_pe; i++) {
    pfdi_buffer = g_pfdi_invalid_force_error + i;
    test_fail = 0;
    inval_case = 0;

    /* If X3-X4 are non-zero, return params X0=INVALID_PARAM and X1-X4=0 */
    for (inval_case = 0; inval_case < NUM_REGS_TO_CHECK; inval_case++) {

      val_pfdi_invalidate_ret_params(&pfdi_buffer->inval[inval_case]);

      if (pfdi_buffer->inval[inval_case].x0 != PFDI_ACS_INVALID_PARAMETERS) {
        val_print(ACS_PRINT_ERR, "\n       PFDI force_error with invalid x%d", inval_case + 2);
        val_print(ACS_PRINT_ERR, " param failed %ld ", pfdi_buffer->inval[inval_case].x0);
        val_print(ACS_PRINT_ERR, "on PE = %d", i);
        test_fail++;
      }

      if ((pfdi_buffer->inval[inval_case].x1 != 0) || (pfdi_buffer->inval[inval_case].x2 != 0) ||
          (pfdi_buffer->inval[inval_case].x3 != 0) || (pfdi_buffer->inval[inval_case].x4 != 0)) {
        val_print(ACS_PRINT_ERR,
                "\n       Registers X1-X4 are not zero for x%d invalid case:", inval_case + 2);
        val_print(ACS_PRINT_ERR, " x1=0x%llx", pfdi_buffer->inval[inval_case].x1);
        val_print(ACS_PRINT_ERR, " x2=0x%llx", pfdi_buffer->inval[inval_case].x2);
        val_print(ACS_PRINT_ERR, " x3=0x%llx", pfdi_buffer->inval[inval_case].x3);
        val_print(ACS_PRINT_ERR, " x4=0x%llx", pfdi_buffer->inval[inval_case].x4);
        val_print(ACS_PRINT_ERR, "\n       Failed on PE = %d", i);
        test_fail++;
      }
    }

    if (test_fail)
      val_set_status(i, RESULT_FAIL(TEST_NUM, 3));
    else
      val_set_status(i, RESULT_PASS(TEST_NUM, 1));
  }

free_pfdi_details:
  val_memory_free((void *) g_pfdi_invalid_force_error);

  return;
}

uint32_t pfdi030_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
    val_run_test_configurable_payload(&num_pe, payload_invalid_force_error_check);

  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
