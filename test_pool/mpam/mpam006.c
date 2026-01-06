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
#include "val/include/acs_memory.h"

#define TEST_NUM   (ACS_MPAM_TEST_NUM_BASE + 6)
#define TEST_RULE  "S_L7MP_03"
#define TEST_DESC  "Check PMG storage by CPOR nodes       "

#define PARTITION_PERCENTAGE 75
#define BUFFER_SIZE (100 * 1024 * 1024) //100M buffer

static uint64_t mpam2_el2_temp;
static uint32_t msc_node_cnt;
static uint64_t llc_identifier;
static uint32_t fail_cnt;

/* Program the CSU monitors within all applicable MSCs with specific PMG value */
static void
program_all_monitors_with_pmg(uint16_t partid, uint8_t pmg)
{
    uint32_t msc_index;
    uint8_t  max_pmg      = 0;
    uint32_t csumon_count = 0;
    uint32_t rsrc_node_cnt, rsrc_index;

    val_print(ACS_PRINT_TEST, "\n       Programming all MSCs to filter PMG value %d", pmg);

    for (msc_index = 0; msc_index < msc_node_cnt; msc_index++) {
        rsrc_node_cnt = val_mpam_get_info(MPAM_MSC_RSRC_COUNT, msc_index, 0);

        val_print(ACS_PRINT_DEBUG, "\n       msc index  = %d", msc_index);
        val_print(ACS_PRINT_DEBUG, "\n       Resource count = %d", rsrc_node_cnt);

        for (rsrc_index = 0; rsrc_index < rsrc_node_cnt; rsrc_index++) {

            /* Check whether resource node is a PE Cache */
            if (val_mpam_get_info(MPAM_MSC_RSRC_TYPE, msc_index, rsrc_index) ==
                                                          MPAM_RSRC_TYPE_PE_CACHE) {

                /*Check if the PE Cache ID matches LLC ID */
                if (val_mpam_get_info(MPAM_MSC_RSRC_DESC1, msc_index, rsrc_index) ==
                                                                            llc_identifier) {

                    /* Select resource instance if RIS feature implemented */
                    if (val_mpam_msc_supports_ris(msc_index))
                        val_mpam_memory_configure_ris_sel(msc_index, rsrc_index);

                    if (val_mpam_supports_cpor(msc_index)) {

                        max_pmg = val_mpam_get_max_pmg(msc_index);
                        val_mpam_configure_cpor(msc_index, partid, PARTITION_PERCENTAGE);

                        if (val_mpam_supports_csumon(msc_index))
                            csumon_count = val_mpam_get_csumon_count(msc_index);
                    } else
                        continue;

                    /* Skip if the MSC's resource does not implement CSUMON or
                       has max_pmg < programmable PMG */
                    if (csumon_count == 0 || max_pmg < pmg) {
                        val_print(ACS_PRINT_TEST, "\n       Skipping MSC resource %d", rsrc_index);
                        continue;
                    }

                    /* Program 0th CSU monitor to filter all transactions with programmed PMG */
                    val_mpam_configure_csu_mon(msc_index, partid, pmg, 0);
                }
            }
        }
    }
}

/* Enable/Disable CSU monitors for all applicable MSC nodes */
static void
set_status_for_all_csumon(uint32_t status)
{
    uint32_t msc_index;
    uint32_t rsrc_node_cnt, rsrc_index;
    uint32_t csumon_count = 0;
    uint32_t nrdy_timeout = 0;

    val_print(ACS_PRINT_DEBUG, "\n       Setting CSUMON status of all MSC to %d", status);

    for (msc_index = 0; msc_index < msc_node_cnt; msc_index++) {
        rsrc_node_cnt = val_mpam_get_info(MPAM_MSC_RSRC_COUNT, msc_index, 0);

        val_print(ACS_PRINT_DEBUG, "\n       msc index  = %d", msc_index);
        val_print(ACS_PRINT_DEBUG, "\n       Resource count = %d ", rsrc_node_cnt);

        for (rsrc_index = 0; rsrc_index < rsrc_node_cnt; rsrc_index++) {

            /* Check whether resource node is a PE Cache */
            if (val_mpam_get_info(MPAM_MSC_RSRC_TYPE, msc_index, rsrc_index) ==
                                                          MPAM_RSRC_TYPE_PE_CACHE) {

                /*Check if the PE Cache ID matches LLC ID */
                if (val_mpam_get_info(MPAM_MSC_RSRC_DESC1, msc_index, rsrc_index) ==
                                                                            llc_identifier) {

                    /* Select resource instance if RIS feature implemented */
                    if (val_mpam_msc_supports_ris(msc_index))
                        val_mpam_memory_configure_ris_sel(msc_index, rsrc_index);

                    if (val_mpam_supports_cpor(msc_index)) {

                        if (val_mpam_supports_csumon(msc_index))
                            csumon_count = val_mpam_get_csumon_count(msc_index);
                    } else
                        continue;

                    if (csumon_count == 0) {
                        val_print(ACS_PRINT_DEBUG, "\n       Skipping MSC resource %d", rsrc_index);
                        continue;
                    }

                    /* Enable/ Disable previously selected CSUMON instance */
                    if (status)
                        val_mpam_csumon_enable(msc_index);
                    else
                        val_mpam_csumon_disable(msc_index);

                    nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
                    while (nrdy_timeout) {
                        --nrdy_timeout;
                    };
                }
            }
        }
    }
}

/* Read the value filtered by programmed CSU monitors across all MSCs */
static void
read_all_msc_csu_counters(uint32_t expected_count)
{
    uint32_t msc_index;
    uint32_t rsrc_node_cnt, rsrc_index;
    uint32_t csumon_count;
    uint32_t storage_count;

    val_print(ACS_PRINT_DEBUG, "\n       Reading the CSUMON counter of all MSC", 0);

    for (msc_index = 0; msc_index < msc_node_cnt; msc_index++) {
        rsrc_node_cnt = val_mpam_get_info(MPAM_MSC_RSRC_COUNT, msc_index, 0);

        for (rsrc_index = 0; rsrc_index < rsrc_node_cnt; rsrc_index++) {

            /* Check whether resource node is a PE Cache */
            if (val_mpam_get_info(MPAM_MSC_RSRC_TYPE, msc_index, rsrc_index) ==
                                                          MPAM_RSRC_TYPE_PE_CACHE) {

                /*Check if the PE Cache ID matches LLC ID */
                if (val_mpam_get_info(MPAM_MSC_RSRC_DESC1, msc_index, rsrc_index) ==
                                                                            llc_identifier) {

                    /* Select resource instance if RIS feature implemented */
                    if (val_mpam_msc_supports_ris(msc_index))
                        val_mpam_memory_configure_ris_sel(msc_index, rsrc_index);

                    if (val_mpam_supports_cpor(msc_index)) {

                        if (val_mpam_supports_csumon(msc_index))
                            csumon_count = val_mpam_get_csumon_count(msc_index);
                    } else
                        continue;

                    if (csumon_count == 0) {
                        val_print(ACS_PRINT_DEBUG, "\n       Skipping MSC resource %d", rsrc_index);
                        continue;
                    }

                    storage_count = val_mpam_read_csumon(msc_index);
                    val_print(ACS_PRINT_TEST, "\n       msc index  = %d", msc_index);
                    val_print(ACS_PRINT_TEST, "  storage count  = %d", storage_count);

                    /* Expected count || Storage count -> Status */
                    /*       0        ||       0       ->    1   */
                    /*       0        ||       1       ->    0   */
                    /*       1        ||       0       ->    0   */
                    /*       1        ||       1       ->    1   */
                    if (!((expected_count && storage_count) || (!expected_count && !storage_count)))
                    {
                        val_print(ACS_PRINT_ERR, "\n    MSC Storage value mismatch. Failure!", 0);
                        fail_cnt++;
                    }
                }
            }
        }
    }
}

static void
payload(void)
{
    uint32_t llc_index;
    uint8_t pmg0       = 0;
    uint8_t pmg1       = 1;
    void *src_buf      = 0;
    void *dest_buf     = 0;
    uint16_t partid    = 1;
    uint64_t mpam2_el2 = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    fail_cnt = 0;

   /* Get the Index for LLC */
    llc_index = val_cache_get_llc_index();
    if (llc_index == CACHE_TABLE_EMPTY) {
        val_print(ACS_PRINT_ERR, "\n       Cache info table empty", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
        return;
    }

    /* Get the cache identifier for LLC */
    llc_identifier = val_cache_get_info(CACHE_ID, llc_index);
    if (llc_identifier == INVALID_CACHE_INFO) {
        val_print(ACS_PRINT_ERR, "\n       LLC invalid in PPTT", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 02));
        return;
    }

    /* Get total number of MSCs reported by MPAM ACPI table */
    msc_node_cnt = val_mpam_get_msc_count();
    val_print(ACS_PRINT_DEBUG, "\n       MSC count = %d", msc_node_cnt);

    if (msc_node_cnt == 0) {
        val_set_status(index, RESULT_FAIL(TEST_NUM, 03));
        return;
    }

    /*Allocate memory for source and destination buffers */
    src_buf = (void *)val_aligned_alloc(MEM_ALIGN_4K, BUFFER_SIZE);
    dest_buf = (void *)val_aligned_alloc(MEM_ALIGN_4K, BUFFER_SIZE);

    if ((src_buf == NULL) || (dest_buf == NULL)) {
        val_print(ACS_PRINT_ERR, "\n       Mem allocation failed", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 04));
        return;
    }

    mpam2_el2 = val_mpam_reg_read(MPAM2_EL2);
    mpam2_el2_temp = mpam2_el2;

    /* Scenario 1 :
    a) Program first instance of CSU monitor across all MSCs with PMG value 1.
    b) Program MPAM2_EL2 with PMG value 0
    c) As there is a PMG mismatch between the requestor and completer,
       the CSU monitors must not filter any transactions */
    program_all_monitors_with_pmg(partid, pmg1);

    /* Clear the PARTID_D & PMG_D bits in mpam2_el2 before writing to them */
    mpam2_el2 = CLEAR_BITS_M_TO_N(mpam2_el2, MPAMn_ELx_PARTID_D_SHIFT+15,
                                                                    MPAMn_ELx_PARTID_D_SHIFT);
    mpam2_el2 = CLEAR_BITS_M_TO_N(mpam2_el2, MPAMn_ELx_PMG_D_SHIFT+7,
                                                                    MPAMn_ELx_PMG_D_SHIFT);

    val_print(ACS_PRINT_TEST, "\n       Programming mpam2_el2 with PMG=0 %d", pmg0);

    mpam2_el2 |= (((uint64_t)pmg0 << MPAMn_ELx_PMG_D_SHIFT) |
                    ((uint64_t)partid << MPAMn_ELx_PARTID_D_SHIFT));

    val_mpam_reg_write(MPAM2_EL2, mpam2_el2);

    /* Enable CSU monitors for all MSCs that support CSU monitoring */
    set_status_for_all_csumon(1);

    /*Perform first memory transaction */
    val_memcpy(src_buf, dest_buf, BUFFER_SIZE);

    /* Monitor all MSCs with expected count in CSU monitors to be 0 */
    read_all_msc_csu_counters(0);

    /* Disable all monitors */
    set_status_for_all_csumon(0);

    /*Restore initial MPAM_EL2 settings */
    mpam2_el2 = mpam2_el2_temp;

    /* Clear the PARTID_D & PMG_D bits in mpam2_el2 before writing to them */
    mpam2_el2 = CLEAR_BITS_M_TO_N(mpam2_el2, MPAMn_ELx_PARTID_D_SHIFT+15,
                                                MPAMn_ELx_PARTID_D_SHIFT);
    mpam2_el2 = CLEAR_BITS_M_TO_N(mpam2_el2, MPAMn_ELx_PMG_D_SHIFT+7,
                                                MPAMn_ELx_PMG_D_SHIFT);

    /* Scenario 2 :
    a) The first instance of CSU monitor across all MSCs are already programmed with PMG=1
    b) Program MPAM2_EL2 with PMG value 1
    c) There is a PMG match between the requestor and completer,
       it should result in increased CSUMON counter */
    mpam2_el2 |= (((uint64_t)pmg1 << MPAMn_ELx_PMG_D_SHIFT) |
                    ((uint64_t)partid << MPAMn_ELx_PARTID_D_SHIFT));

    val_print(ACS_PRINT_TEST, "\n       Programming mpam2_el2 with PMG=1 %d", pmg1);

    val_mpam_reg_write(MPAM2_EL2, mpam2_el2);

    /* Enable CSU monitoring */
    set_status_for_all_csumon(1);

    /*Perform second memory transaction */
    val_memcpy(src_buf, dest_buf, BUFFER_SIZE);

    /* Monitor all MSCs with expected count in CSU monitors to be non-zero */
    read_all_msc_csu_counters(1);

    /* Disable all monitors */
    set_status_for_all_csumon(0);

    /* Test fails if storage_value1 is non zero or storage_value2 is zero */
    if (fail_cnt) {
        val_set_status(index, RESULT_FAIL(TEST_NUM, 05));

        /*Restore MPAM2_EL2 settings */
        val_mpam_reg_write(MPAM2_EL2, mpam2_el2_temp);

        /*Free the buffers */
        val_memory_free_aligned(src_buf);
        val_memory_free_aligned(dest_buf);

        return;
    }

    /*Restore MPAM2_EL2 settings */
    val_mpam_reg_write(MPAM2_EL2, mpam2_el2_temp);

    /*Free the buffers */
    val_memory_free_aligned(src_buf);
    val_memory_free_aligned(dest_buf);

    val_set_status(index, RESULT_PASS(TEST_NUM, 01));
    return;
}

uint32_t mpam006_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    num_pe = 1;
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
