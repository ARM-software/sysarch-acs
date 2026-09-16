/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
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

#include "acs_val.h"
#include "acs_pe.h"
#include "val_interface.h"

#define TEST_NUM   (ACS_PE_TEST_NUM_BASE + 70)
#define TEST_RULE  "KQQWG"
#define TEST_DESC  "Check for FEAT_NV2 support            "

#define FEAT_NV2_SUPPORTED  0x2U

static void payload(void)
{
    uint64_t data = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    /*
     * ID_AA64MMFR2_EL1.NV[27:24] == 0b0010 indicates FEAT_NV2 is implemented.
     */
    data = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64MMFR2_EL1), 24, 27);
    val_print_primary_pe(DEBUG, "\n       ID_AA64MMFR2_EL1.NV = %llx", data, index);

    if (data == FEAT_NV2_SUPPORTED)
        val_set_status(index, RESULT_PASS);
    else
        val_set_status(index, RESULT_FAIL(01));
}

uint32_t pe070_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);

    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}
