/** @file
 * Copyright (c) 2016-2018,2021,2024-2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/acs_pe.h"

#define TEST_NUM   (ACS_PE_TEST_NUM_BASE  +  22)
#define TEST_RULE  "B_PE_23"
#define TEST_DESC  "Check for EL3 AArch64 support         "

#define TEST_NUM1   (ACS_PE_TEST_NUM_BASE  +  63)
#define TEST_RULE1  "B_PE_24"
#define TEST_DESC1  "Check for Secure state implementation "

/* This test checks for EL3 Aarch64 implementation */
static
void
payload_check_el3_support()
{

  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  /* EL3   : bits [15:12] */
  uint32_t feat_el3 = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 12, 15);

  /* Check if PE implements EL3 AArch64 execution state, ID_AA64PFR0_EL1[15:12] should be non-zero
     value */
  if (feat_el3)
	val_set_status(index, RESULT_PASS(TEST_NUM, 1));
  else
	val_set_status(index, RESULT_FAIL(TEST_NUM, 1));

  return;
}

/* This test checks for  Secure state implementation */
static
void
payload_check_secure_state_support()
{
  /* Get index of current PE for log reporting */
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Raw 64-bit value from ID_AA64PFR0_EL1 */
  uint64_t data  = val_pe_reg_read(ID_AA64PFR0_EL1);

  /* Extract bitfields of ID_AA64PFR0_EL1 */
  /* SEL2  : bits [39:36] */
  uint32_t feat_sel2     = VAL_EXTRACT_BITS(data, 36, 39);
  /* EL3   : bits [15:12] */
  uint32_t feat_el3      = VAL_EXTRACT_BITS(data, 12, 15);

  /* Pass the test if Secure state is implemented via EL3 or SEL2 */
  if (feat_sel2 || feat_el3) {
      val_set_status(index, RESULT_PASS(TEST_NUM1, 1));
      return;
  } else {
      val_set_status(index, RESULT_FAIL(TEST_NUM1, 1));
      return;
  }

}

uint32_t
pe022_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload_check_el3_support, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}

uint32_t
pe063_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM1, TEST_DESC1, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM1, num_pe, payload_check_secure_state_support, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM1, num_pe, TEST_RULE1);

  val_report_status(0, ACS_END(TEST_NUM1), NULL);

  return status;
}
