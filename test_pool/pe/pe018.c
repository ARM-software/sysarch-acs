/** @file
 * Copyright (c) 2016-2018,2020-2021,2024-2025, Arm Limited or its affiliates. All rights reserved.
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


#define TEST_NUM   (ACS_PE_TEST_NUM_BASE  +  18)
#ifdef PC_BSA
#define TEST_RULE  "P_L1PE_01"
#else
#define TEST_RULE  "B_PE_19"
#endif
#define TEST_DESC  "Check Stage 2 4KB Granule Support     "

static
void
payload()
{
  uint64_t data = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  data = val_pe_reg_read(ID_AA64MMFR0_EL1);

  /* PEs must support 4kb granule for Stage 2.
   * Check For TGran4_2[43:40] != 1 ( 1: 4KB granule size not supported at stage 2.)
   */

  if ((VAL_EXTRACT_BITS(data, 40, 43) != 0x1))
    val_set_status(index, RESULT_PASS(TEST_NUM, 1));
  else
    val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
}

uint32_t
pe018_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
