/** @file
 * Copyright (c) 2016-2018, 2021,2025-2026, Arm Limited or its affiliates. All rights reserved.
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
#include "acs_iovirt.h"

#define TEST_NUM   (ACS_SMMU_TEST_NUM_BASE + 6)
#define TEST_RULE  "B_SMMU_19"
#define TEST_DESC  "SMMUv2 unique intr per ctxt bank      "

static
void
payload()
{
  uint32_t num_smmu;
  uint32_t index;
  uint32_t test_skip = 1;

  index = val_pe_get_index_mpid(val_pe_get_mpid());

  num_smmu = val_smmu_get_info(SMMU_NUM_CTRL, 0);
  if (num_smmu == 0) {
      val_print(DEBUG, "\n       No SMMU Controllers are discovered ");
      val_set_status(index, RESULT_SKIP(2));
      return;
  }

  while (num_smmu--) {
      if (val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, num_smmu) != 2) {
          val_print(DEBUG, "\n       Test applicable only to SMMU v2      ");
          continue;
      }

      test_skip = 0;
      if (!val_iovirt_check_unique_ctx_intid(num_smmu)) {
          val_print(ERROR, "\n       Unique interrupt ID per context bank check "
                                   "failed for SMMU %x", num_smmu);
          val_set_status(index, RESULT_FAIL(1));
          return;
      }
  }

  if (test_skip)
      val_set_status(index, RESULT_SKIP(3));
  else
      val_set_status(index, RESULT_PASS);
}

uint32_t
i006_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
