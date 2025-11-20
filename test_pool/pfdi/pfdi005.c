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

PFDI_RET_PARAMS *g_pfdi_st_version_details;

void
pfdi_st_version_check(void)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  PFDI_RET_PARAMS *pfdi_buffer;

  pfdi_buffer = g_pfdi_st_version_details + index;

  /* Invoke PFDI Test ID  function for current PE index */
  pfdi_buffer->x0 = val_pfdi_pe_test_id(&pfdi_buffer->x1, &pfdi_buffer->x2,
                                        &pfdi_buffer->x3, &pfdi_buffer->x4);

  val_pfdi_invalidate_ret_params(pfdi_buffer);

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
  PFDI_RET_PARAMS *pfdi_buffer;

  /* Allocate memory to save all PFDI Self Test Versions for all PE's */
  g_pfdi_st_version_details =
            (PFDI_RET_PARAMS *) val_memory_calloc(num_pe, sizeof(PFDI_RET_PARAMS));
  if (g_pfdi_st_version_details == NULL) {
    val_print(ACS_PRINT_ERR,
                "\n       Allocation for PFDI Self Test Version Details Failed", 0);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 4));
    return;
  }

  for (i = 0; i < num_pe; i++) {
    pfdi_buffer = g_pfdi_st_version_details + i;
    val_pfdi_invalidate_ret_params(pfdi_buffer);
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
    pfdi_buffer = g_pfdi_st_version_details + i;
    test_fail = 0;

    val_pfdi_invalidate_ret_params(pfdi_buffer);

    temp_status  = pfdi_buffer->x0;
    version = pfdi_buffer->x1;

    if (temp_status == PFDI_ACS_SUCCESS) {
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
    } else if (temp_status == PFDI_ACS_UNKNOWN) {
      val_print(ACS_PRINT_ERR, "\n       PFDI Self test metadata not available on PE %d ", i);
      if (pfdi_buffer->x1 != 0) {
        val_print(ACS_PRINT_ERR, "\n       Registers X1 is not zero:", 0);
        val_print(ACS_PRINT_ERR, " x1=0x%llx", pfdi_buffer->x1);
        test_fail++;
      }
    } else {
      val_print(ACS_PRINT_ERR,
                "\n       PFDI PE Test ID failed err = %lld", temp_status);
      test_fail++;
    }

    if ((pfdi_buffer->x2 != 0) || (pfdi_buffer->x3 != 0) || (pfdi_buffer->x4 != 0)) {
      val_print(ACS_PRINT_ERR, "\n       Registers X2-X4 are not zero:", 0);
      val_print(ACS_PRINT_ERR, " x2=0x%llx", pfdi_buffer->x2);
      val_print(ACS_PRINT_ERR, " x3=0x%llx", pfdi_buffer->x3);
      val_print(ACS_PRINT_ERR, " x4=0x%llx", pfdi_buffer->x4);
      val_print(ACS_PRINT_ERR, "\n       Failed on PE = %d", i);
      test_fail++;
    }

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
