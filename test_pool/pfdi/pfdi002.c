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
#include "val/include/acs_memory.h"
#include "val/include/val_interface.h"

#define TEST_NUM   (ACS_PFDI_TEST_NUM_BASE + 2)
#define TEST_RULE  "R0055"
#define TEST_DESC  "Check PFDI Version in All PE's            "

typedef struct{
  int64_t pfdi_version;
} version_details;

version_details *g_pfdi_version_details;

void
pfdi_version_check(void)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  version_details *pfdi_status_buffer;

  pfdi_status_buffer = g_pfdi_version_details + index;

  /* Invoke PFDI Version function for current PE index */
  pfdi_status_buffer->pfdi_version = val_pfdi_version();

  val_data_cache_ops_by_va((addr_t)&pfdi_status_buffer->pfdi_version, CLEAN_AND_INVALIDATE);
  val_set_status(index, RESULT_PASS(TEST_NUM, 1));

  return;
}

static void payload_all_pe_version_check(void *arg)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t num_pe = *((uint32_t *)arg);
  uint32_t timeout, i = 0, major = 0, minor = 0;
  uint32_t test_fail = 0;
  version_details *pfdi_buffer;
  int64_t version = 0;

  /* Allocate memory to save all PFDI Versions or status for all PE's */
  g_pfdi_version_details = (version_details *) val_memory_calloc(num_pe, sizeof(version_details));
  if (g_pfdi_version_details == NULL) {
    val_print(ACS_PRINT_ERR, "\n       Allocation for PFDI Version Details Failed \n", 0);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
    return;
  }

  for (i = 0; i < num_pe; i++) {
    pfdi_buffer = g_pfdi_version_details + i;
    val_data_cache_ops_by_va((addr_t)&pfdi_buffer->pfdi_version, CLEAN_AND_INVALIDATE);
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
        val_print(ACS_PRINT_ERR, "\n       **Timed out** for PE index = %d", i);
        val_set_status(i, RESULT_FAIL(TEST_NUM, 2));
        goto free_pfdi_details;
      }
    }
  }
  val_time_delay_ms(ONE_MILLISECOND);

  for (i = 0; i < num_pe; i++) {
    pfdi_buffer = g_pfdi_version_details + i;
    test_fail = 0;

    val_data_cache_ops_by_va((addr_t)&pfdi_buffer->pfdi_version, CLEAN_AND_INVALIDATE);
    version = pfdi_buffer->pfdi_version;
    if (version >> 31) {
      val_print(ACS_PRINT_ERR,
                "\n       PFDI get version failed %ld, ", version);
      val_print(ACS_PRINT_ERR, "on PE = %d", i);
      test_fail++;
    }

    if (val_pfdi_reserved_bits_check_is_zero(
             VAL_EXTRACT_BITS(pfdi_buffer->pfdi_version, 31, 63)) != ACS_STATUS_PASS) {
      val_print(ACS_PRINT_ERR, "\n       Failed on PE = %d", i);
      test_fail++;
    }

    major = PFDI_VERSION_GET_MAJOR(version);
    if (major != PFDI_MAJOR_VERSION) {
      val_print(ACS_PRINT_ERR,
                "\n       Major Version not as expected, Current version = 0x%llx", major);
      val_print(ACS_PRINT_ERR, "\n       Failed on PE = %d", i);
      test_fail++;
    }

    minor = PFDI_VERSION_GET_MINOR(version);
    if (minor != PFDI_MINOR_VERSION) {
      val_print(ACS_PRINT_ERR,
                "\n       Minor Version not as expected, Current version =%d", minor);
      val_print(ACS_PRINT_ERR, "\n       Failed on PE = %d", i);
      test_fail++;
    }

    if (test_fail)
      val_set_status(i, RESULT_FAIL(TEST_NUM, 3));
    else
      val_set_status(i, RESULT_PASS(TEST_NUM, 1));
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
