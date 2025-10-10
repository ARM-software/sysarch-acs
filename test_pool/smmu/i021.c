/** @file
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/acs_iovirt.h"
#include "val/include/acs_smmu.h"
#include "val/include/acs_mmu.h"
#include "val/include/acs_pcie.h"

#define TEST_NUM     (ACS_SMMU_TEST_NUM_BASE + 21)
#define TEST_RULE    "S_L7SM_03"
#define TEST_DESC    "Check SMMU PMU Extension presence     "

#define TEST_NUM1    (ACS_SMMU_TEST_NUM_BASE + 27)
#define TEST_RULE1   "S_L7SM_04"
#define TEST_DESC1   "Check SMMU PMCG has >= 4 counters     "

/**
 * @brief Helper function to validate PMCG presence and counter count for each SMMUv3
 *
 * @param check_counter If 1, checks for minimum 4 counters in each PMCG.
 *                      If 0, only checks for at least one PMCG per SMMU.
 * @return ACS_STATUS_PASS if checks pass; otherwise ACS_STATUS_FAIL
 */
static uint32_t
check_smmu_pmcg(uint32_t check_counter)
{
    uint32_t num_smmu, num_pmcg, i;
    uint32_t test_skip = 1;
    uint32_t smmu_version;
    uint64_t smmu_base, pmcg_base, pmcg_smmu_base, num_pmcg_count;
    uint32_t test_fail = 0, num_pmcg_found;
    uint32_t status;

    num_smmu = val_smmu_get_info(SMMU_NUM_CTRL, 0);
    num_pmcg = val_iovirt_get_pmcg_info(PMCG_NUM_CTRL, 0);

    if (num_smmu == 0) {
        val_print(ACS_PRINT_DEBUG, "\n       No SMMU Controllers are discovered ", 0);
        return ACS_STATUS_SKIP;
    } else if (num_pmcg == 0) {
        /* If SMMUs are present in system and no PMCGs detected, report test as FAIL*/
        val_print(ACS_PRINT_ERR, "\n       SMMUs in the system don't implement SMMUv3"
                                   " Performance Monitors Extension ", 0);
        return ACS_STATUS_FAIL;
    }

    while (num_smmu--) {
        smmu_version = val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, num_smmu);
        if (smmu_version != 3) {
            val_print(ACS_PRINT_DEBUG, "\n       Valid for only SMMU v3, smmu version %d",
                      smmu_version);
            continue;
        }

        test_skip = 0; // At least one SMMU v3 found
        smmu_base = val_smmu_get_info(SMMU_CTRL_BASE, num_smmu);
        num_pmcg_found = 0;

        /* Scan all PMCGs to find matches for this SMMU base */
        for (i = 0; i < num_pmcg; i++) {
            pmcg_smmu_base = val_iovirt_get_pmcg_info(PMCG_NODE_SMMU_BASE, i);
            if (smmu_base == pmcg_smmu_base) {
                pmcg_base = val_iovirt_get_pmcg_info(PMCG_CTRL_BASE, i);

                /* Check if PMCG node memory is mapped. If not, map it */
                status = val_mmu_update_entry(pmcg_base, SIZE_1K);
                if (status) {
                    val_print(ACS_PRINT_ERR, "\n       Could not map PMCG node memory", 0);
                    return ACS_STATUS_FAIL;
                }

                num_pmcg_count = VAL_EXTRACT_BITS(val_mmio_read(pmcg_base + SMMU_PMCG_CFGR), 0, 5)
                                 + 1;

                /* If checking counter count, ensure there are at least 4 */
                if (check_counter && num_pmcg_count < 4) {
                    val_print(ACS_PRINT_ERR, "\n       PMCG has less than 4 counters for SMMU"
                              "index : %d", num_smmu);
                    val_print(ACS_PRINT_ERR, "\n       Number of PMCG counters : %d",
                              num_pmcg_count);
                    test_fail++;
                }

                num_pmcg_found++;
            }
        }

        if (num_pmcg_found == 0) {
            val_print(ACS_PRINT_ERR, "\n       PMU Extension not implemented for SMMU index : %d",
                      num_smmu);
            test_fail++;
        }
    }

    if (test_skip)
        return ACS_STATUS_SKIP;

    return test_fail ? ACS_STATUS_FAIL : ACS_STATUS_PASS;
}

/**
 * @brief Payload to verify PMCG presence for each SMMUv3
 */
static void payload_pmcg_present(void)
{
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t result = check_smmu_pmcg(0);

    if (result == ACS_STATUS_SKIP)
        val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
    else
        val_set_status(index, result == ACS_STATUS_FAIL ?
                    RESULT_FAIL(TEST_NUM, 01) :
                    RESULT_PASS(TEST_NUM, 01));
}

/**
 * @brief Payload to verify each PMCG for SMMUv3 has >= 4 counters
 */
static void payload_counter_check(void)
{
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t result = check_smmu_pmcg(1);

    if (result == ACS_STATUS_SKIP)
        val_set_status(index, RESULT_SKIP(TEST_NUM1, 01));
    else
        val_set_status(index, result == ACS_STATUS_FAIL ?
                    RESULT_FAIL(TEST_NUM1, 01) :
                    RESULT_PASS(TEST_NUM1, 01));
}

/**
 * @brief Entry point for PMCG presence test
 */
uint32_t
i021_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;
    num_pe = 1; /* This test is run on single PE */

    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload_pmcg_present, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);
    return status;
}

/**
 * @brief Entry point for PMCG counter count test
 */
uint32_t
i027_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;
    num_pe = 1; /* This test is run on single PE */

    status = val_initialize_test(TEST_NUM1, TEST_DESC1, num_pe);
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM1, num_pe, payload_counter_check, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM1, num_pe, TEST_RULE1);
    val_report_status(0, ACS_END(TEST_NUM1), TEST_RULE1);
    return status;
}
