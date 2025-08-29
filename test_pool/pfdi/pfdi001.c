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

#define TEST_NUM   (ACS_PFDI_TEST_NUM_BASE + 1)
#define TEST_RULE  "R0053"
#define TEST_DESC  "Check PFDI Version is returned            "

static void payload_version_check(void *arg)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t major = 0, minor = 0;
  PFDI_RET_PARAMS pfdi_version;

  /* Invoke PFDI Version function for current PE index */
  pfdi_version.x0 = val_pfdi_version(&pfdi_version.x1, &pfdi_version.x2,
                                     &pfdi_version.x3, &pfdi_version.x4);
  if (pfdi_version.x0 >> 31) {
    val_print(ACS_PRINT_ERR,
                "\n       PFDI get version failed err = %ld", pfdi_version.x0);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
    return;
  }

  /* Return status Bits[63:31] must be zero */
  if (val_pfdi_reserved_bits_check_is_zero(
             VAL_EXTRACT_BITS(pfdi_version.x0, 31, 63)) != ACS_STATUS_PASS) {
    val_print(ACS_PRINT_ERR, "\n       Failed on PE = %d", index);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 2));
    return;
  }

  major = PFDI_VERSION_GET_MAJOR(pfdi_version.x0);
  if (major != PFDI_MAJOR_VERSION) {
    val_print(ACS_PRINT_ERR, "\n       Major Version not as expected, Current version = %d", major);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 3));
    return;
  }

  minor = PFDI_VERSION_GET_MINOR(pfdi_version.x0);
  if (minor != PFDI_MINOR_VERSION) {
    val_print(ACS_PRINT_ERR, "\n       Minor Version not as expected, Current version = %d", minor);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 4));
    return;
  }

  if ((pfdi_version.x1 != 0) || (pfdi_version.x2 != 0) ||
      (pfdi_version.x3 != 0) || (pfdi_version.x4 != 0)) {
    val_print(ACS_PRINT_ERR, "\n       Registers X1-X4 are not zero:", 0);
    val_print(ACS_PRINT_ERR, " x1=0x%llx", pfdi_version.x1);
    val_print(ACS_PRINT_ERR, " x2=0x%llx", pfdi_version.x2);
    val_print(ACS_PRINT_ERR, " x3=0x%llx", pfdi_version.x3);
    val_print(ACS_PRINT_ERR, " x4=0x%llx", pfdi_version.x4);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 5));
    return;
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 1));

  return;
}

uint32_t pfdi001_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1; /* This test will run on single PE */
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
    val_run_test_configurable_payload(&num_pe, payload_version_check);

  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
