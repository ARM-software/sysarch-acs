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

#define TEST_NUM   (ACS_PFDI_TEST_NUM_BASE + 17)
#define TEST_RULE  "R0154"
#define TEST_DESC  "Check PFDI unsupported function           "

PFDI_RET_PARAMS *g_pfdi_unsupp_function_check;

void
check_unsupp_fn()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  PFDI_RET_PARAMS  *pfdi_buffer;

  pfdi_buffer = g_pfdi_unsupp_function_check + index;

  /* Invoke PFDI with unsupported function for current PE index */
  pfdi_buffer->x0 = val_invoke_pfdi_fn(PFDI_FN_PFDI_RESERVED, 0, 0, 0, 0, 0,
                    (unsigned long *)&pfdi_buffer->x1, (unsigned long *)&pfdi_buffer->x2,
                    (unsigned long *)&pfdi_buffer->x3, (unsigned long *)&pfdi_buffer->x4);

  val_pfdi_invalidate_ret_params(pfdi_buffer);

  val_set_status(index, RESULT_PASS(TEST_NUM, 1));
  return;
}

static void payload_unsupp_fn_check(void *arg)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t timeout, i = 0, run_fail = 0;
  PFDI_RET_PARAMS *pfdi_buffer;
  uint32_t num_pe = *(uint32_t *)arg;

  /* Allocate memory to save all PFDI function status for all PE's */
  g_pfdi_unsupp_function_check = (PFDI_RET_PARAMS *)
                    val_memory_calloc(num_pe, sizeof(PFDI_RET_PARAMS));
  if (g_pfdi_unsupp_function_check == NULL) {
    val_print(ACS_PRINT_ERR,
                "\n       Allocation for PFDI Invalid Feature Support Check Failed", 0);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
    return;
  }

  for (i = 0; i < num_pe; i++) {
    pfdi_buffer = g_pfdi_unsupp_function_check + i;
    val_pfdi_invalidate_ret_params(pfdi_buffer);
  }

  /* Invoke PFDI Unsupported function for current PE index */
  check_unsupp_fn();

  /* Execute check_unsupp_fn function in All PE's */
  for (i = 0; i < num_pe; i++) {
    if (i != index) {
      timeout = TIMEOUT_LARGE;
      val_execute_on_pe(i, check_unsupp_fn, 0);

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
    pfdi_buffer = g_pfdi_unsupp_function_check + i;
    val_pfdi_invalidate_ret_params(pfdi_buffer);
    run_fail = 0;

    if (pfdi_buffer->x0 != PFDI_ACS_NOT_SUPPORTED) {
      val_print(ACS_PRINT_ERR, "\n       PFDI Unsupported function check failed err = %ld",
                                                        pfdi_buffer->x0);
      val_print(ACS_PRINT_ERR, " on PE index = %d", i);
      run_fail++;
    }

    if ((pfdi_buffer->x1 != 0) || (pfdi_buffer->x2 != 0) ||
        (pfdi_buffer->x3 != 0) || (pfdi_buffer->x4 != 0)) {
      val_print(ACS_PRINT_ERR, "\n       Registers X1-X4 are not zero:", 0);
      val_print(ACS_PRINT_ERR, " x1=0x%llx", pfdi_buffer->x1);
      val_print(ACS_PRINT_ERR, " x2=0x%llx", pfdi_buffer->x2);
      val_print(ACS_PRINT_ERR, " x3=0x%llx", pfdi_buffer->x3);
      val_print(ACS_PRINT_ERR, " x4=0x%llx", pfdi_buffer->x4);
      val_print(ACS_PRINT_ERR, "\n       Failed on PE = %d", i);
      run_fail++;
    }

    if (run_fail)
      val_set_status(i, RESULT_FAIL(TEST_NUM, 3));
    else
      val_set_status(i, RESULT_PASS(TEST_NUM, 1));
  }

free_pfdi_details:
  val_memory_free((void *) g_pfdi_unsupp_function_check);

  return;
}

uint32_t pfdi017_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
    val_run_test_configurable_payload(&num_pe, payload_unsupp_fn_check);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
