/** @file
 * Copyright (c) 2016-2018, 2020-2025, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0
 *
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
#include "val/include/acs_smmu.h"

#define TEST_NUM   (ACS_SMMU_TEST_NUM_BASE + 8)
#define TEST_NUM1   (ACS_SMMU_TEST_NUM_BASE + 25)

#ifdef PC_BSA
#define TEST_RULE  "P_L1SM_02"
#define TEST_RULE1 "P_L1SM_03"
#else
#define TEST_RULE  "S_L4SM_01"
#define TEST_RULE1 "S_L4SM_02"
#endif

#define TEST_DESC  "Check Stage 1 SMMUv3 functionality    "
#define TEST_DESC1 "Check Stage 2 SMMUv3 functionality    "

static
void
payload_check_smmu_stg1_support(void)
{
    uint64_t data;
    uint32_t num_smmu;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    num_smmu = val_iovirt_get_smmu_info(SMMU_NUM_CTRL, 0);

    if (num_smmu == 0) {
        val_print(ACS_PRINT_ERR, "\n       No SMMU Controllers are discovered.", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
        return;
    }

    while (num_smmu--)
    {
        if (val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, num_smmu) < 3) {
            val_print(ACS_PRINT_ERR,
                     "\n       SMMUv3, or higher must be supported.", 0);
            val_set_status(index, RESULT_FAIL(TEST_NUM, 02));
            return;
        } else {
            val_print(ACS_PRINT_INFO, "\n       Detected SMMUv3, or higher implementation ", 0);
            data = val_smmu_read_cfg(SMMUv3_IDR0, num_smmu);
            /* Check Stage 1 translation support */
            if ((data & BIT1) == 0) {
                val_print(ACS_PRINT_ERR, "\n       Stage 1 translation not supported ", 0);
                val_set_status(index, RESULT_FAIL(TEST_NUM, 03));
                return;
            }

        }
    }
    val_set_status(index, RESULT_PASS(TEST_NUM, 01));
}

static
void
payload_check_smmu_stg2_support(void)
{
    uint64_t data;
    uint32_t num_smmu;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    num_smmu = val_iovirt_get_smmu_info(SMMU_NUM_CTRL, 0);

    if (num_smmu == 0) {
        val_print(ACS_PRINT_ERR, "\n       No SMMU Controllers are discovered.", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM1, 01));
        return;
    }

    while (num_smmu--)
    {
        if (val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, num_smmu) < 3) {
            val_print(ACS_PRINT_ERR,
                    "\n       SMMUv3, or higher must be supported.", 0);
            val_set_status(index, RESULT_FAIL(TEST_NUM1, 02));
            return;
        } else {
            val_print(ACS_PRINT_INFO, "\n       Detected SMMUv3, or higher implementation ", 0);
            data = val_smmu_read_cfg(SMMUv3_IDR0, num_smmu);
            /* Check Stage 2 translation support */
            if ((data & BIT0) == 0) {
                val_print(ACS_PRINT_ERR, "\n       Stage 2 translation not supported ", 0);
                val_set_status(index, RESULT_FAIL(TEST_NUM1, 03));
                return;
            }

        }
    }
    val_set_status(index, RESULT_PASS(TEST_NUM1, 01));
}

uint32_t
i008_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  num_pe = 1;  /* This test is run on single processor */
  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload_check_smmu_stg1_support, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);
  return status;
}

uint32_t
i025_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  num_pe = 1;  /* This test is run on single processor */
  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM1, TEST_DESC1, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM1, num_pe, payload_check_smmu_stg2_support, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM1, num_pe, TEST_RULE1);
  val_report_status(0, ACS_END(TEST_NUM1), TEST_RULE1);
  return status;
}