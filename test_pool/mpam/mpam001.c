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
#include "val/include/acs_pe.h"
#include "val/include/acs_common.h"
#include "val/include/val_interface.h"
#include "val/include/acs_mpam.h"

#define TEST_NUM   (ACS_MPAM_TEST_NUM_BASE + 1)
#define TEST_RULE  "S_L7MP_01"
#define TEST_DESC  "Check for MPAM extension              "

#define TEST_NUM1   (ACS_MPAM_TEST_NUM_BASE + 8)
#define TEST_RULE1  "S_L7MP_02"
#define TEST_DESC1  "Check for MPAM partition IDs          "

/* Helper: check whether FEAT_MPAM is implemented */
static uint32_t is_feat_mpam_implemented(void)
{
    /* ID_AA64PFR0_EL1.MPAM bits[43:40] > 0 or
       ID_AA64PFR1_EL1.MPAM_frac bits[19:16] > 0 indicates implementation */
    if ((VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 40, 43) > 0) ||
        (VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR1_EL1), 16, 19) > 0))
        return 1;
    return 0;
}

static void payload_check_mpam_ext_support(void)
{
    uint32_t pe_index;

    pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

    /* PEs must implement FEAT_MPAM */
    if (!is_feat_mpam_implemented()) {
            val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 01));
            return;
    }

    val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));
}

static void payload_check_mpam_part_id_count(void)
{
    uint32_t pe_index;
    uint64_t data = 0;
    uint64_t mpamidr_val;

    pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

    /* Initial check: FEAT_MPAM must be implemented before accessing MPAMIDR_EL1 */
    if (!is_feat_mpam_implemented()) {
        val_set_status(pe_index, RESULT_FAIL(TEST_NUM1, 01));
        return;
    }

    mpamidr_val = val_mpam_reg_read(MPAMIDR_EL1);


    /* check support for minimum of 16 physical partition IDs, MPAMIDR_EL1.PARTID_MAX
    must be >= 16 */
    data = VAL_EXTRACT_BITS(mpamidr_val, 0, 15);
    if (data < 16) {
        val_set_status(pe_index, RESULT_FAIL(TEST_NUM1, 02));
        return;
    }

    /* check support for MPAM virtulization support indicated by MPAMIDR_EL1.HAS_HCR bit */
    data = VAL_EXTRACT_BITS(mpamidr_val, 17, 17);
    if (data == 0) {
        val_set_status(pe_index, RESULT_FAIL(TEST_NUM1, 03));
        return;
    }

    /* check support for minimum of 8 virtual partition IDs,
    MPAMIDR_EL1.VPMR_MAX must be > 0 */
    data = VAL_EXTRACT_BITS(mpamidr_val, 18, 20);
    if (data < 1) {
        val_set_status(pe_index, RESULT_FAIL(TEST_NUM1, 04));
        return;
    }

    val_set_status(pe_index, RESULT_PASS(TEST_NUM1, 01));
}

uint32_t mpam001_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    num_pe = 1;
    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload_check_mpam_ext_support, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}

uint32_t mpam008_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    num_pe = 1;
    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM1, TEST_DESC1, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM1, num_pe, payload_check_mpam_part_id_count, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM1, num_pe, TEST_RULE1);
    val_report_status(0, ACS_END(TEST_NUM1), TEST_RULE1);

    return status;
}
