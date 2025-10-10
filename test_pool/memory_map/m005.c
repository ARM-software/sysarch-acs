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
#include "val/include/acs_common.h"
#include "val/include/acs_pe.h"
#include "val/include/acs_peripherals.h"
#include "val/include/acs_memory.h"
#include "val/include/acs_mpam.h"

#define TEST_NUM (ACS_MEMORY_MAP_TEST_NUM_BASE + 5)
#define TEST_NUM1 (ACS_MEMORY_MAP_TEST_NUM_BASE + 8)
#ifdef PC_BSA
#define TEST_RULE  "P_L1MM_01"
#define TEST_RULE1  "P_L1MM_01"
#else
#define TEST_RULE "S_L3MM_01"
#define TEST_RULE1 "S_L3MM_02"
#endif
#define TEST_DESC "NS-EL2 Stage-2 64KB Mapping Check     "
#define TEST_DESC1 "Check peripherals addr 64Kb apart     "


static void payload_check_s2_64kb_granule_support(void)
{
    uint64_t data = 0;
    uint32_t pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

    /* Check if EL2 is supported, if not skip the test. non-zero value in ID_AA64PFR0_EL1[11:8]
       indicate EL2 support */
    data = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 8, 11);

    if (data == 0) {
        val_print(ACS_PRINT_DEBUG, "\n       EL2 not implemented, Skipping the test.", 0);
        val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
        return;
    }

    /* Check PE support for 64KB memory granule size at stage 2. ID_AA64MMFR0_EL1[39:36] == b0010
       indicates 64KB granule supported at stage 2, and value b0000 is deprecated when EL2 is
       implemented hence not checking ID_AA64MMFR0_EL1.TGran64 field.*/
    data = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64MMFR0_EL1), 36, 39);
    if (data != 0x2) {
        val_print(ACS_PRINT_ERR, "\n       64KB granule not supported at stage 2.", 0);
        val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 01));
        return;
    }

    val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));
    return;
}


static void payload_check_peripheral_addr_64kb_apart(void)
{
    uint32_t pe_index;
    uint32_t peri_index, peri_index1;
    uint64_t peri_count, addr_diff;
    uint64_t peri_addr1, peri_addr2;
    uint32_t fail_cnt = 0;

    pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
    peri_count = val_peripheral_get_info(NUM_ALL, 0);

    if (peri_count < 2) {
        val_print(ACS_PRINT_DEBUG, "\n       No or one peripherals reported by the system.", 0);
        val_set_status(pe_index, RESULT_SKIP(TEST_NUM1, 01));
        return;
    }

    /* check whether all peripheral base addresses are 64KB apart from each other */
    for (peri_index = 0 ; peri_index < peri_count; peri_index++) {
        for (peri_index1 = peri_index + 1; peri_index1 < peri_count; peri_index1++) {

            peri_addr1 = val_peripheral_get_info(ANY_BASE0, peri_index);
            peri_addr2 = val_peripheral_get_info(ANY_BASE0, peri_index1);
            val_print(ACS_PRINT_INFO, "\n   addr of Peripheral 1 is  %llx", peri_addr1);
            val_print(ACS_PRINT_INFO, "\n   addr of Peripheral 2 is  %llx", peri_addr2);

           if ((peri_addr1 == 0) || (peri_addr2 == 0)) {
                continue;
           }

            addr_diff = (peri_addr1 > peri_addr2) ?
                         peri_addr1 - peri_addr2 : peri_addr2 - peri_addr1;

            if (addr_diff < MEM_SIZE_64KB) {
                val_print(ACS_PRINT_ERR,
                         "\n  Peripheral base addresses isn't atleast 64Kb apart %llx", addr_diff);
                fail_cnt++;
            }
        }
    }

    if (fail_cnt) {
        val_set_status(pe_index, RESULT_FAIL(TEST_NUM1, 01));
    } else {
        val_set_status(pe_index, RESULT_PASS(TEST_NUM1, 01));
    }
}

uint32_t m005_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;
    /* run on single PE */
    num_pe = 1;
    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload_check_s2_64kb_granule_support, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);
    return status;
}

uint32_t m008_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;
    /* run on single PE */
    num_pe = 1;
    status = val_initialize_test(TEST_NUM1, TEST_DESC1, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM1, num_pe, payload_check_peripheral_addr_64kb_apart, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM1, num_pe, TEST_RULE1);
    val_report_status(0, ACS_END(TEST_NUM1), TEST_RULE1);
    return status;
}
