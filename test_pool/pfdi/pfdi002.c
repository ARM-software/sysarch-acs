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

#include "acs_val.h"
#include "acs_memory.h"
#include "val_interface.h"

#define TEST_NUM   (ACS_PFDI_TEST_NUM_BASE + 2)
#define TEST_RULE  "R0104"
#define TEST_DESC  "Check PFDI Version in All PE's            "

PFDI_RET_PARAMS *g_pfdi_version_details;

void
pfdi_version_check(void)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  PFDI_RET_PARAMS *pfdi_buffer;

  pfdi_buffer = g_pfdi_version_details + index;

  /* Invoke PFDI Version function for current PE index */
  pfdi_buffer->x0 = val_pfdi_version(&pfdi_buffer->x1, &pfdi_buffer->x2,
                                     &pfdi_buffer->x3, &pfdi_buffer->x4);

  val_pfdi_invalidate_ret_params(pfdi_buffer);

  val_set_status(index, RESULT_PASS);
  return;
}

static void payload_all_pe_version_check(void *arg)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t num_pe = *((uint32_t *)arg);
  uint32_t timeout, i = 0, major = 0, minor = 0;
  uint32_t test_fail = 0;
  PFDI_RET_PARAMS *pfdi_buffer;
  int64_t version = 0;

  /* Allocate memory to save all PFDI Versions or status for all PE's */
  g_pfdi_version_details = (PFDI_RET_PARAMS *) val_memory_calloc(num_pe, sizeof(PFDI_RET_PARAMS));
  if (g_pfdi_version_details == NULL) {
    val_print(ERROR, "\n       Allocation for PFDI Version Details Failed");
    val_set_status(index, RESULT_FAIL(1));
    return;
  }

  for (i = 0; i < num_pe; i++) {
    pfdi_buffer = g_pfdi_version_details + i;
    val_pfdi_invalidate_ret_params(pfdi_buffer);
  }

  /* Invoke PFDI version function for current PE index */
  pfdi_version_check();

  /* Execute pfdi_version_check function in All PE's */
  for (i = 0; i < num_pe; i++) {
    if (i != index) {
      timeout = TIMEOUT_LARGE;
      val_execute_on_pe(i, pfdi_version_check, (uint64_t)g_pfdi_version_details);

      while ((--timeout) && (IS_RESULT_PENDING(val_get_status(i))));

      if (timeout == 0) {
        val_print(ERROR, "\n       **Timed out** for PE index = %d", i);
        val_set_status(i, RESULT_FAIL(2));
        goto free_pfdi_details;
      }
    }
  }
  val_time_delay_ms(ONE_MILLISECOND);

  for (i = 0; i < num_pe; i++) {
    pfdi_buffer = g_pfdi_version_details + i;
    test_fail = 0;

    val_pfdi_invalidate_ret_params(pfdi_buffer);

    version = pfdi_buffer->x0;
    if (version >> 31) {
      val_print(ERROR,
                "\n       PFDI get version failed %ld, ", version);
      val_print(ERROR, "on PE = %d", i);
      test_fail++;
    }

    if (val_pfdi_reserved_bits_check_is_zero(
             VAL_EXTRACT_BITS(pfdi_buffer->x0, 31, 63)) != ACS_STATUS_PASS) {
      val_print(ERROR, "\n       Failed on PE = %d", i);
      test_fail++;
    }

    major = PFDI_VERSION_GET_MAJOR(version);
    if (major != PFDI_MAJOR_VERSION) {
      val_print(ERROR,
                "\n       Major Version not as expected, Current version = %d", major);
      val_print(ERROR, "\n       Failed on PE = %d", i);
      test_fail++;
    }

    minor = PFDI_VERSION_GET_MINOR(version);
    if (minor != PFDI_MINOR_VERSION) {
      val_print(ERROR,
                "\n       Minor Version not as expected, Current version =%d", minor);
      val_print(ERROR, "\n       Failed on PE = %d", i);
      test_fail++;
    }

    if ((pfdi_buffer->x1 != 0) || (pfdi_buffer->x2 != 0) ||
        (pfdi_buffer->x3 != 0) || (pfdi_buffer->x4 != 0)) {
      val_print(ERROR, "\n       Registers X1-X4 are not zero:");
      val_print(ERROR, " x1=0x%llx", pfdi_buffer->x1);
      val_print(ERROR, " x2=0x%llx", pfdi_buffer->x2);
      val_print(ERROR, " x3=0x%llx", pfdi_buffer->x3);
      val_print(ERROR, " x4=0x%llx", pfdi_buffer->x4);
      val_print(ERROR, "\n       Failed on PE = %d", i);
      test_fail++;
    }

    if (test_fail)
      val_set_status(i, RESULT_FAIL(3));
    else
      val_set_status(i, RESULT_PASS);
  }

free_pfdi_details:
  val_memory_free((void *) g_pfdi_version_details);

  return;
}

uint32_t pfdi002_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
    val_run_test_configurable_payload(&num_pe, payload_all_pe_version_check);

  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
