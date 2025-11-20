/** @file
 * Copyright (c) 2020, 2022-2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/acs_pe.h"
#include "val/include/val_interface.h"


#define TEST_NUM   (ACS_PE_TEST_NUM_BASE  +  40)
#define TEST_RULE  "S_L6PE_04"
#define TEST_DESC  "Check PMU Version v3.5 or higher      "

#define TEST_NUM1   (ACS_PE_TEST_NUM_BASE  +  64)
#define TEST_RULE1  "S_L8PE_05"
#define TEST_DESC1  "Check PMU Version v3.7 or higher      "

static void payload_check_if_pmuv3_5(void)
{
    uint64_t data = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    /* Read ID_AA64DFR0_EL1.PMUVer[11:8] >= 0b0110 and != 0xF for PMU v3.5 or higher support */
    data = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64DFR0_EL1), 8, 11);
    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       ID_AA64DFR0_EL1.PMUVer = %llx",
                                                                            data, index);


    /* Read ID_AA64DFR0_EL1.PMUVer[11:8] >= 0b0110 and != 0xF for PMU v3.5 or higher support */
    if ((data < PE_PMUv3p5) || (data == 0xF)) /* 0xF is PMUv3 not supported */
        val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
    else
        val_set_status(index, RESULT_PASS(TEST_NUM, 01));
}

static void payload_check_if_pmuv3_7(void)
{
    uint64_t data = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    data = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64DFR0_EL1), 8, 11);
    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       ID_AA64DFR0_EL1.PMUVer = %llx",
                                                                            data, index);

    /* Read ID_AA64DFR0_EL1.PMUVer[11:8] >= 0b0111 and != 0xF for PMU v3.7 or higher support */
    if ((data < PE_PMUv3p7) || (data == 0xF))
        val_set_status(index, RESULT_FAIL(TEST_NUM1, 01));
    else
        val_set_status(index, RESULT_PASS(TEST_NUM1, 01));
}

uint32_t pe040_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload_check_if_pmuv3_5, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}

uint32_t pe064_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM1, TEST_DESC1, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM1, num_pe, payload_check_if_pmuv3_7, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM1, num_pe, TEST_RULE1);
    val_report_status(0, ACS_END(TEST_NUM1), TEST_RULE1);

    return status;
}
