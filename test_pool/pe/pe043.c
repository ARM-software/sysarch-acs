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

#define TEST_NUM   (ACS_PE_TEST_NUM_BASE  +  43)
#define TEST_RULE  "B_SEC_01"
#define TEST_DESC  "Check Speculation Restriction         "

static void payload(void)
{
    uint64_t data_csv2 = 0;
    uint64_t data_csv3 = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    if (g_sbsa_level < 6) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
        return;
    }

    /* ID_AA64PFR0_EL1.CSV2[59:56] = 0b0010 or 0b0011 indicates Speculative use of Out of
       Ctxt Branch Targets and use of SCXTNUM_ELx registers */
    /* ID_AA64PFR0_EL1.CSV3[63:60] = 0b0001 Speculative use of Faulting data */
    data_csv2 = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 56, 59);
    data_csv3 = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 60, 63);

    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       ID_AA64PFR0_EL1.CSV2 = %llx", data_csv2, index);
    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       ID_AA64PFR0_EL1.CSV3 = %llx", data_csv3, index);

    if (((data_csv2 == 2) || (data_csv2 == 3)) && (data_csv3 == 1))
        val_set_status(index, RESULT_PASS(TEST_NUM, 01));
    else
        val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
}

uint32_t pe043_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}
