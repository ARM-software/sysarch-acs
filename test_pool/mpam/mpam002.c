/** @file
 * Copyright (c) 2024-2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/acs_pe.h"
#include "val/include/acs_mpam.h"


#define TEST_NUM   (ACS_MPAM_TEST_NUM_BASE + 2)
#define TEST_RULE  "S_L7MP_03"
#define TEST_DESC  "Check for MPAM LLC CSU Support        "

#define TEST_NUM1   (ACS_MPAM_TEST_NUM_BASE + 9)
#define TEST_RULE1  "S_L7MP_04"
#define TEST_DESC1  "Check for MPAM LLC CSU Monitor count  "

#define LLC_MSC_INDEX_UNKNOWN 0xFFFFFFFF

static uint32_t get_llc_msc_index(void)
{
    /* initialise a static variable to store MPAM MSC index of Last-level-cache (LLC)*/
    static uint32_t llc_msc_index = LLC_MSC_INDEX_UNKNOWN;

    uint32_t llc_index;
    uint64_t cache_identifier;
    uint32_t msc_node_cnt;
    uint32_t rsrc_node_cnt;
    uint32_t msc_index, rsrc_index;

    /* If parsing had passed in last call, return the cached value */
    if (llc_msc_index != LLC_MSC_INDEX_UNKNOWN) {
        val_print(ACS_PRINT_DEBUG, "\n       Returning parsed LLC MPAM MSC index = %d",
                  llc_msc_index);
        return llc_msc_index;
    }

    /* Else process the parsed info to get the llc_msc_index */
    /* Re-run even if previous parse was failed for ERROR prints */

    /* Find the LLC cache identifier */
    val_print(ACS_PRINT_DEBUG, "\n       Parsing for LLC MPAM MSC index", 0);
    llc_index = val_cache_get_llc_index();
    if (llc_index == CACHE_TABLE_EMPTY) {
        val_print(ACS_PRINT_ERR, "\n       Cache info table empty", 0);
        return LLC_MSC_INDEX_UNKNOWN;
    }

    cache_identifier = val_cache_get_info(CACHE_ID, llc_index);
    if (cache_identifier == INVALID_CACHE_INFO) {
        val_print(ACS_PRINT_ERR, "\n       LLC invalid in PPTT", 0);
        return LLC_MSC_INDEX_UNKNOWN;
    }

    /* Check in the MPAM table which MSC is attached to the LLC */
    msc_node_cnt = val_mpam_get_msc_count();
    val_print(ACS_PRINT_DEBUG, "\n       MSC count = %d", msc_node_cnt);

    if (msc_node_cnt == 0) {
        val_print(ACS_PRINT_ERR, "\n       MPAM MSC count is zero", 0);
        return LLC_MSC_INDEX_UNKNOWN;
    }

    /* visit each MSC node and check for cache resources */
    for (msc_index = 0; msc_index < msc_node_cnt; msc_index++) {
        rsrc_node_cnt = val_mpam_get_info(MPAM_MSC_RSRC_COUNT, msc_index, 0);
        val_print(ACS_PRINT_DEBUG, "\n       MSC index      = %d", msc_index);
        val_print(ACS_PRINT_DEBUG, "\n       Resource count = %d", rsrc_node_cnt);
        for (rsrc_index = 0; rsrc_index < rsrc_node_cnt; rsrc_index++) {
            /* check whether the resource location is cache */
            if (val_mpam_get_info(MPAM_MSC_RSRC_TYPE, msc_index, rsrc_index) ==
                                                                       MPAM_RSRC_TYPE_PE_CACHE) {
                if (val_mpam_get_info(MPAM_MSC_RSRC_DESC1, msc_index, rsrc_index) ==
                                                                               cache_identifier) {
                    /* We have MSC which controls/monitors the LLC cache */
                    val_print(ACS_PRINT_DEBUG, "\n       Resource index = %d", rsrc_index);
                    val_print(ACS_PRINT_DEBUG, "\n       MSC index of LLC = %d", msc_index);

                    /* Store the LLC MSC index in static variable */
                    llc_msc_index = msc_index;
                    /* Return LLC MSC index*/
                    return llc_msc_index;
                }
            }
        }
    }

    /* if we don't get MSC node on LLC, control will reach here */
    return LLC_MSC_INDEX_UNKNOWN;

}

static void payload_check_mpam_llc_csu_support(void)
{
    uint32_t llc_msc_index;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    /* Check if PE implements FEAT_MPAM */
    /* ID_AA64PFR0_EL1.MPAM bits[43:40] > 0 or ID_AA64PFR1_EL1.MPAM_frac bits[19:16] > 0
       indicates implementation of MPAM extension */
    if (!((VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 40, 43) > 0) ||
       (VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR1_EL1), 16, 19) > 0))) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
        val_print(ACS_PRINT_DEBUG, "\n       FEAT_MPAM not supported by PE", 0);
        return;
    }

    /* Get MPAM msc_index of LLC cache */
    llc_msc_index = get_llc_msc_index();

    /* Check if msc_index in valid, else mark test as FAIL */
    if (llc_msc_index == LLC_MSC_INDEX_UNKNOWN) {
        val_print(ACS_PRINT_ERR, "\n       MSC on LLC not found ", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
        return;
    }

    /* Check if LLC supports CSU monitor */
    if (!val_mpam_supports_csumon(llc_msc_index)) {
        val_print(ACS_PRINT_ERR, "\n       CSU MON unsupported by LLC", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 02));
        return;
    } else {
        val_set_status(index, RESULT_PASS(TEST_NUM, 01));
        return;
    }

}

static void payload_check_llc_csu_mon_count(void)
{
    uint32_t llc_msc_index;
    uint32_t csumon_count;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    /* Check if PE implements FEAT_MPAM */
    /* ID_AA64PFR0_EL1.MPAM bits[43:40] > 0 or ID_AA64PFR1_EL1.MPAM_frac bits[19:16] > 0
       indicates implementation of MPAM extension */
    if (!((VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 40, 43) > 0) ||
       (VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR1_EL1), 16, 19) > 0))) {
        val_set_status(index, RESULT_SKIP(TEST_NUM1, 01));
        val_print(ACS_PRINT_DEBUG, "\n       FEAT_MPAM not supported by PE", 0);
        return;
    }

    /* Get MPAM msc_index of LLC cache */
    llc_msc_index = get_llc_msc_index();

    /* Check if msc_index in valid, else mark test as FAIL */
    if (llc_msc_index == LLC_MSC_INDEX_UNKNOWN) {
        val_print(ACS_PRINT_ERR, "\n       MSC on LLC not found ", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM1, 01));
        return;
    }

    /* Check if atleast 16 CSU monitor are present */
    csumon_count = val_mpam_get_csumon_count(llc_msc_index);
    if (csumon_count < 16) {
        val_print(ACS_PRINT_ERR, "\n       CSU MON %d less than 16", csumon_count);
        val_set_status(index, RESULT_FAIL(TEST_NUM1, 02));
        return;
    } else {
        val_set_status(index, RESULT_PASS(TEST_NUM1, 01));
        return;
    }
}


uint32_t mpam002_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    num_pe = 1;
    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload_check_mpam_llc_csu_support, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}

uint32_t mpam009_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    num_pe = 1;
    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM1, TEST_DESC1, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM1, num_pe, payload_check_llc_csu_mon_count, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM1, num_pe, TEST_RULE1);
    val_report_status(0, ACS_END(TEST_NUM1), TEST_RULE1);

    return status;
}