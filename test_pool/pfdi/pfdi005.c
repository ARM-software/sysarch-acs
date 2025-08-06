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

#define TEST_NUM   (ACS_PFDI_TEST_NUM_BASE + 5)
#define TEST_RULE  "R0066"
#define TEST_DESC  "Check PE HW test mechanism info           "

typedef struct{
  int64_t pfdi_st_version;
  int64_t pfdi_st_status;
} st_version_details;

st_version_details *g_pfdi_st_version_details;

void
pfdi_st_version_check(void)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  st_version_details *pfdi_st_status_buffer;
  int64_t feat;

  pfdi_st_status_buffer = g_pfdi_st_version_details + index;

  /* Invoke PFDI Test ID  function for current PE index */
  pfdi_st_status_buffer->pfdi_st_status = val_pfdi_pe_test_id(&feat);
  pfdi_st_status_buffer->pfdi_st_version = feat;

  val_data_cache_ops_by_va((addr_t)&pfdi_st_status_buffer->pfdi_st_version, CLEAN_AND_INVALIDATE);
  val_data_cache_ops_by_va((addr_t)&pfdi_st_status_buffer->pfdi_st_status, CLEAN_AND_INVALIDATE);

  val_set_status(index, RESULT_PASS(TEST_NUM, 1));
  return;
}

static void payload_pe_test_id_check(void *arg)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t num_pe = *((uint32_t *)arg);
  int64_t test_fail = 0;
  int64_t version, temp_status;
  uint32_t timeout, i = 0, major, minor, vendor_id;
  st_version_details *st_pfdi_buffer;

  /* Allocate memory to save all PFDI Self Test Versions for all PE's */
  g_pfdi_st_version_details =
            (st_version_details *) val_memory_calloc(num_pe, sizeof(st_version_details));
  if (g_pfdi_st_version_details == NULL) {
    val_print(ACS_PRINT_ERR,
                "\n       Allocation for PFDI Self Test Version Details Failed", 0);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 4));
    return;
  }

  for (i = 0; i < num_pe; i++) {
    st_pfdi_buffer = g_pfdi_st_version_details + i;
    val_data_cache_ops_by_va((addr_t)&st_pfdi_buffer->pfdi_st_version, CLEAN_AND_INVALIDATE);
    val_data_cache_ops_by_va((addr_t)&st_pfdi_buffer->pfdi_st_status, CLEAN_AND_INVALIDATE);
  }

  /* Invoke PFDI PE Test ID function for current PE index */
  pfdi_st_version_check();

  /* Execute pfdi_st_version_check function in All PE's */
  for (i = 0; i < num_pe; i++) {
    if (i != index) {
      timeout = TIMEOUT_LARGE;
      val_execute_on_pe(i, pfdi_st_version_check, (uint64_t)g_pfdi_st_version_details);

      while ((--timeout) && (IS_RESULT_PENDING(val_get_status(i))));

      if (timeout == 0) {
        val_print(ACS_PRINT_ERR, "\n       **Timed out** for PE index = %d", i);
        val_set_status(i, RESULT_FAIL(TEST_NUM, 5));
        goto free_pfdi_details;
      }
    }
  }
  val_time_delay_ms(ONE_MILLISECOND);

  for (i = 0; i < num_pe; i++) {
    val_print(ACS_PRINT_DEBUG, "\n       PFDI Self Test version details for PE index = %d", i);
    st_pfdi_buffer = g_pfdi_st_version_details + i;
    test_fail = 0;

    val_data_cache_ops_by_va((addr_t)&st_pfdi_buffer->pfdi_st_status, CLEAN_AND_INVALIDATE);
    val_data_cache_ops_by_va((addr_t)&st_pfdi_buffer->pfdi_st_version, CLEAN_AND_INVALIDATE);

    version = st_pfdi_buffer->pfdi_st_version;
    temp_status  = st_pfdi_buffer->pfdi_st_status;

    if (temp_status != PFDI_ACS_SUCCESS) {
      val_print(ACS_PRINT_ERR,
                "\n       PFDI PE Test ID failed err = %d", temp_status);
      test_fail++;
    }

    /* Return status Bits[63:32] must be zero */
    if (val_pfdi_reserved_bits_check_is_zero(
             VAL_EXTRACT_BITS(version, 32, 63)) != ACS_STATUS_PASS) {
      val_print(ACS_PRINT_ERR, "\n       Failed on PE = %d", i);
      test_fail++;
    }

    /* Return status Bits[23:20] must be zero */
    if (val_pfdi_reserved_bits_check_is_zero(
             VAL_EXTRACT_BITS(version, 20, 23)) != ACS_STATUS_PASS) {
      val_print(ACS_PRINT_ERR, "\n       Failed on PE = %d", i);
      test_fail++;
    }

    major = VAL_EXTRACT_BITS(version, 8, 15);
    val_print(ACS_PRINT_DEBUG, "\n       PFDI Self Test Major Version = %d", major);

    minor = VAL_EXTRACT_BITS(version, 0, 7);
    val_print(ACS_PRINT_DEBUG, "\n       PFDI Self Test Minor Version = %d", minor);

    vendor_id = VAL_EXTRACT_BITS(version, 24, 31);
    val_print(ACS_PRINT_DEBUG, "\n       PFDI Self Test Vendor ID     = %d", vendor_id);

    if (test_fail)
      val_set_status(i, RESULT_FAIL(TEST_NUM, 3));
    else
      val_set_status(i, RESULT_PASS(TEST_NUM, 1));
  }

free_pfdi_details:
  val_memory_free((void *) g_pfdi_st_version_details);

  return;
}

uint32_t pfdi005_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
    val_run_test_configurable_payload(&num_pe, payload_pe_test_id_check);

  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
