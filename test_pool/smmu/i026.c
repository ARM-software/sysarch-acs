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
#include "acs_pe.h"
#include "val_interface.h"
#include "acs_smmu.h"

#define TEST_NUM   (ACS_SMMU_TEST_NUM_BASE + 32)
#define TEST_RULE  "SMMU_02"
#define TEST_DESC  "Check SMMU stall flow support             "

#define SMMU_STALL_MODEL_TERMINATE  0x1

static
void
payload()
{
  uint32_t index;
  uint32_t num_smmu;
  uint32_t smmu_index;
  uint32_t stall_model;
  uint32_t relevant_smmu = 0;
  uint32_t stall_supported = 0;

  index = val_pe_get_index_mpid(val_pe_get_mpid());
  num_smmu = val_smmu_get_info(SMMU_NUM_CTRL, 0);

  if (num_smmu == 0) {
    val_print(DEBUG, "\n       No SMMU Controllers are discovered");
    val_set_status(index, RESULT_SKIP(1));
    return;
  }

  for (smmu_index = 0; smmu_index < num_smmu; smmu_index++) {
    /* IDR0.STALL_MODEL is defined for SMMUv3 controllers. */
    if (val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, smmu_index) < 3) {
      val_print(DEBUG, "\n       SMMU %x is not an SMMUv3 controller", smmu_index);
      continue;
    }

    relevant_smmu++;
    stall_model = VAL_EXTRACT_BITS(
                      val_smmu_read_cfg(SMMUv3_IDR0, smmu_index), 24, 25);

    val_print(DEBUG, "\n       SMMU %x IDR0.STALL_MODEL = 0x%x",
              smmu_index, stall_model);

    if (stall_model != SMMU_STALL_MODEL_TERMINATE)
      stall_supported = 1;
  }

  if (relevant_smmu == 0) {
    val_print(DEBUG, "\n       SMMU_02 not applicable: No SMMUv3 controllers");
    val_set_status(index, RESULT_SKIP(2));
    return;
  }

  if (!stall_supported) {
    val_print(INFO, "\n       SMMU_02 not applicable: Stall model is not supported");
    val_set_status(index, RESULT_SKIP(3));
    return;
  }

  val_print(WARN, "\n       Stall supported - integration/DV evidence required");
  val_set_status(index, RESULT_WARNING(1));
}

uint32_t
i032_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  // This test is run on a single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
    val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
