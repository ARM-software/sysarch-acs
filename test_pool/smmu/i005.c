/** @file
 * Copyright (c) 2016-2018, 2021-2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/acs_smmu.h"
#include "val/include/acs_pcie.h"

#define TEST_NUM   (ACS_SMMU_TEST_NUM_BASE + 5)
#define TEST_RULE  "B_SMMU_16"
#define TEST_DESC  "Check SMMUs stage2 support            "

#define TEST_NUM1   (ACS_SMMU_TEST_NUM_BASE + 29)
#define TEST_RULE1  "B_SMMU_18"
#define TEST_DESC1  "Check SMMU S-EL2 & stage2 support     "

/* This function iterates over all SMMU present in system and checks if it supports stage 2
   translation support */
test_status_t check_smmu_stg2_support (void)
{
    uint32_t num_smmu;
    uint32_t i;
    uint32_t s2ts, s2p;
    uint32_t smmu_rev;
    uint32_t fail_cnt = 0;

    /* Get total number of SMMUs present in the system */
    num_smmu = val_smmu_get_info(SMMU_NUM_CTRL, 0);
    if (num_smmu == 0) {
        val_print(ACS_PRINT_ERR, "\n       No SMMU Controllers are discovered ", 0);
        return TEST_FAIL;
    }

    for (i = 0; i < num_smmu; i++) {
        smmu_rev = val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, i);

        if (smmu_rev == 2) {
            /* Check for SMMUv2 stage 2 support by reading SMMUv2_IDR0 29th bit*/
            s2ts = VAL_EXTRACT_BITS(val_smmu_read_cfg(SMMUv2_IDR0, i), 29, 29);
            if (!s2ts) {
                val_print(ACS_PRINT_ERR,
                        "\n       SMMUv2 index: 0x%x not providing Stage2 functionality", i);
                fail_cnt++;
            }
        } else if (smmu_rev == 3) {
            /* Read SMMUv3_IDR0 bit 0 for stage 2 translation support */
            s2p = VAL_EXTRACT_BITS(val_smmu_read_cfg(SMMUv3_IDR0, i), 0, 0);
            if (!s2p) {
                val_print(ACS_PRINT_ERR,
                    "\n       SMMUv3 index: 0x%x not providing Stage2 functionality", i);
                fail_cnt++;
            }
        } else {
            val_print(ACS_PRINT_ERR,
                "\n       SMMUv%d register read is not supported", smmu_rev);
            fail_cnt++;
        }

    }

    if (fail_cnt)
        return TEST_FAIL;
    else
        return TEST_PASS;
}

static
void
payload_check_smmu_stg2_support()
{
    /* No known way to discover which devices would be assigned to OS under hypervisor and see
       if its behind SMMU with stage 2 support, hence checking super set that all SMMUs support
       stage 2 translation, report as warning if failure */

    test_status_t status;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    status = check_smmu_stg2_support();
    if (status == TEST_FAIL) {
        val_set_status(index, RESULT_WARN(TEST_NUM, 1));
    } else {
        val_set_status(index, RESULT_PASS(TEST_NUM, 1));
    }
}

static
void
payload_check_sel2_and_smmu_stg2_support()
{
    test_status_t status;
    uint32_t pe_s_el2;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    /* Check if PE supports Secure EL2 */
    pe_s_el2 = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 36, 39);

    /* Skip the test if Secure EL2 is implemented */
    if (pe_s_el2 == 0x1) {
        val_print(ACS_PRINT_DEBUG, "\n       Secure EL2 not supported, skipping the test.", 0);
        val_set_status(index, RESULT_SKIP(TEST_NUM1, 1));
        return;
    }

    /* If Secure EL2 not implemented then all SMMUs in system must support stage 2 translation */
    status = check_smmu_stg2_support();
    val_set_status(index, TEST_STATUS(TEST_NUM1, status, 2));
    return;
}

uint32_t
i005_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload_check_smmu_stg2_support, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}

uint32_t
i029_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM1, TEST_DESC1, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM1, num_pe, payload_check_sel2_and_smmu_stg2_support, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM1, num_pe, TEST_RULE1);

  val_report_status(0, ACS_END(TEST_NUM1), NULL);

  return status;
}
