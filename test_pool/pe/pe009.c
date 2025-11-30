/** @file
 * Copyright (c) 2016-2018,2021,2023-2025, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (ACS_PE_TEST_NUM_BASE  +  9)
#define TEST_RULE  "B_PE_09"
#define TEST_DESC  "Check for PMU and PMU counters        "

#define TEST_NUM_VBSA  (ACS_PE_TEST_NUM_BASE  +  67)
#define TEST_RULE_VBSA  "V_L1PE_02"
#define TEST_DESC_VBSA  "Check PMUv3 support                   "

static
uint32_t fetch_pmu_version(void)
{
  uint64_t data;

  /* Check ID_AA64DFR0_EL1[11:8] for PMUver */
  data = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64DFR0_EL1), 8, 11);

  return (uint32_t)data;
}

static
void
payload()
{
  uint64_t data = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t primary_pe_idx = val_pe_get_primary_index();

  /* Check if PMUv3 is implemented */
  data = fetch_pmu_version();

  if ((data != 0x0) && (data != 0xF)) {
      /* PMCR_EL0 Bits 15:11 for Number of counters. */
      data = VAL_EXTRACT_BITS(val_pe_reg_read(PMCR_EL0), 11, 15);
      if (data > 3)
          val_set_status(index, RESULT_PASS(TEST_NUM, 1));
      else {
          if (index == primary_pe_idx) {
              val_print(ACS_PRINT_ERR,
              "\n       Number of PMU counters reported: %d, expected >= 4", data);
          }
          val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
      }
  } else {
      val_set_status(index, RESULT_FAIL(TEST_NUM, 2));
  }

  return;

}

static
void
payload_check_for_pmuv3()
{
  uint64_t data = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t primary_pe_idx = val_pe_get_primary_index();

  /* Check for PMUv3 */
  data = fetch_pmu_version();
  if (data == 0x0 || data == 0xF) {
      val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
      val_print_primary_pe(ACS_PRINT_ERR,
            "\n       PMUv3 not implemented, ID_AA64DFR0_EL1 PMUVer: 0x%lx", data, primary_pe_idx);
      return;
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 1));
  return;
}

uint32_t
pe009_entry(uint32_t num_pe)
{

  uint32_t status;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
      /* execute payload on present PE and then execute on other PE */
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}

uint32_t
pe067_entry(uint32_t num_pe)
{
  uint32_t status;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM_VBSA, TEST_DESC_VBSA, num_pe);

  if (status != ACS_STATUS_SKIP)
      /* execute payload on present PE and then execute on other PE */
      val_run_test_payload(TEST_NUM_VBSA, num_pe, payload_check_for_pmuv3, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM_VBSA, num_pe, TEST_RULE_VBSA);

  val_report_status(0, ACS_END(TEST_NUM_VBSA), NULL);

  return status;
}