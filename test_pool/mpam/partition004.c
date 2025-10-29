/** @file
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/acs_mpam.h"
#include "val/include/acs_mpam_reg.h"
#include "val/include/acs_memory.h"
#include "val/include/val_interface.h"

#define TEST_NUM   ACS_MPAM_CACHE_TEST_NUM_BASE + 4
#define TEST_RULE  ""
#define TEST_DESC  "Check CASSOC Partitioning             "

/* Test algorithm -
1.  Identify an MSC with cache resources that support associativity partitioning by checking
    MPAMF_IDR.HAS_CCAP_PART is set and MPAMF_CCAP_IDR.HAS_CASSOC is also set.
2.  Select a valid PARTID, for example PARTID_X.
3.  Configure MPAMCFG_CASSOC for PARTID_X to 100% cache associativity
4.  Assign PARTID_X to the running PE using MPAM2_EL2.
5.  Generate a memory workload from the PE to populate the cache and observe full usage of
    available ways.
6.  Measure the cache usage using the CSU monitor and record the value.
7.  Reconfigure MPAMCFG_CASSOC for PARTID_Y to restrict associativity to 50%
    (and later to 25% using PARTID_Z).
8.  Repeat the same memory workload under the same conditions.
9.  Observe whether cache lines allocated to new PARTID are now limited to fewer ways
10. Use the CSU monitor to confirm the reduced cache allocation compared to the
    full associativity setting.
11. If the observed cache usage aligns with the restricted CASSOC setting, confirming enforcement
    of associativity limits, mark the test as passed.
12. If the allocation behavior remains unchanged despite the restriction, indicating
    that the control is not being applied, mark the test as failed.
*/

#define CASSOC_SCENARIO_MAX   3
#define CASSOC_PARTITION_100  100
#define CASSOC_PARTITION_50   50
#define CASSOC_PARTITION_25   25
#define BUFFER_SIZE           0x6400000  /* 100MB */

static void
payload(void)
{
    uint32_t msc_cnt  = val_mpam_get_msc_count();
    uint32_t llc_idx  = val_cache_get_llc_index();
    uint32_t index    = val_pe_get_index_mpid(val_pe_get_mpid());

    uint32_t start_count = 0;
    uint32_t end_count   = 0;
    uint32_t test_fail   = 0;
    uint16_t test_partid = 0;
    uint32_t max_partid  = 0;
    void     *src_buf    = 0;
    void     *dest_buf   = 0;
    uint32_t msc_index, rsrc_index, rsrc_cnt;
    uint64_t nrdy_timeout;
    uint64_t cache_identifier;
    uint64_t counter[CASSOC_SCENARIO_MAX];
    uint64_t saved_el2;
    uint32_t test_skip   = 1;
    uint32_t status      = 0;
    uint32_t num_mon     = 0;

    /* Check if LLC is valid */
    if (llc_idx == CACHE_TABLE_EMPTY) {
        val_print(ACS_PRINT_DEBUG, "\n       No LLC found, skipping test", 0);
        val_set_status(index, RESULT_SKIP(TEST_NUM, 1));
        return;
    }

    /* Get the LLC Cache ID */
    cache_identifier = val_cache_get_info(CACHE_ID, llc_idx);
    if (cache_identifier == INVALID_CACHE_INFO) {
        val_print(ACS_PRINT_DEBUG, "\n       Invalid LLC ID, skipping test", 0);
        val_set_status(index, RESULT_SKIP(TEST_NUM, 2));
        return;
    }

    /* Iterate through all MSCs and get the minmax PARTID -> the test PARTID */
    for (msc_index = 0; msc_index < msc_cnt; msc_index++) {
        max_partid = val_mpam_get_max_partid(msc_index);
        if (max_partid == 0) {
            val_print(ACS_PRINT_DEBUG, "\n       MSC %d has no PARTID support", msc_index);
            continue;
        }

        test_partid = (test_partid == 0) ?
                      (max_partid - 1) :
                      GET_MIN_VALUE(test_partid, max_partid - 1);
    }

    val_print(ACS_PRINT_TEST, "\n       Selected PARTID = %d", test_partid);

    /* Iterate through all available LLC MSCs and their resources */
    for (msc_index = 0; msc_index < msc_cnt; msc_index++) {

        rsrc_cnt = val_mpam_get_info(MPAM_MSC_RSRC_COUNT, msc_index, 0);
        for (rsrc_index = 0; rsrc_index < rsrc_cnt; rsrc_index++) {

            if (val_mpam_get_info(MPAM_MSC_RSRC_TYPE, msc_index, rsrc_index)
                != MPAM_RSRC_TYPE_PE_CACHE)
                continue;

            if (val_mpam_get_info(MPAM_MSC_RSRC_DESC1, msc_index, rsrc_index)
                != cache_identifier)
                continue;

            /* Check if the LLC is atleast 4-way set associative.
               Else restricting the cache to 1/4 (25 percent) might result
               in unexpected behavior in the test
            */
            if (val_cache_get_associativity(cache_identifier) < 4) {
                val_print(ACS_PRINT_TEST,
                          "\n       LLC is less than 4-way, skipping MSC", 0);
                continue;
            }

            num_mon     = 0;
            start_count = 0;
            end_count   = 0;

            /* Select resource instance if RIS feature implemented */
            if (val_mpam_msc_supports_ris(msc_index))
                val_mpam_memory_configure_ris_sel(msc_index, rsrc_index);

            /* Check if MSC supports CASSOC */
            if (!val_mpam_supports_cassoc(msc_index)) {
                val_print(ACS_PRINT_DEBUG,
                    "\n       MSC %d does not support CASSOC partitioning, skipping", msc_index);
                continue;
            }

            /* Check if MSC implements CSU monitors */
            if (val_mpam_supports_csumon(msc_index))
                num_mon = val_mpam_get_csumon_count(msc_index);

            if (num_mon == 0) {
                val_print(ACS_PRINT_DEBUG,
                    "\n       MSC %d does not have CSU monitors, skipping MSC", msc_index);
                continue;
            }

            /* Disable CCAP and CPOR settings -> Set them to max */
            if (val_mpam_supports_cpor(msc_index))
                val_mpam_configure_cpor(msc_index, test_partid, 100);

            if (val_mpam_supports_ccap(msc_index))
                val_mpam_configure_ccap(msc_index, test_partid, 0, 100);

            /* Allocate memory for source and destination buffers */
            src_buf  = (void *)val_aligned_alloc(MEM_ALIGN_4K, BUFFER_SIZE);
            dest_buf = (void *)val_aligned_alloc(MEM_ALIGN_4K, BUFFER_SIZE);

            if ((src_buf == NULL) || (dest_buf == NULL)) {
                val_print(ACS_PRINT_ERR, "\n       Mem allocation failed", 0);
                val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
                return;
            }

            /* Test runs on atleast one MSC */
            test_skip = 0;

            /* Configure PARTID with 100% cassoc settings */
            val_mpam_configure_cassoc(msc_index, test_partid, CASSOC_PARTITION_100);

            /* Configure CSU Monitor */
            val_mpam_configure_csu_mon(msc_index, test_partid, DEFAULT_PMG, 0);

            /* Enable CSU Monitor */
            val_mpam_csumon_enable(msc_index);

            /* Wait for MAX_NRDY_USEC after monitor config change */
            nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
            while (nrdy_timeout) {
                --nrdy_timeout;
            };

            /* Save the current MPAM2_EL2 settings */
            saved_el2 = val_mpam_reg_read(MPAM2_EL2);

            /***  Scenario 1: Buffer copy with 100% CASSOC settings  ***/
            val_print(ACS_PRINT_DEBUG, "\n       Scenario 1: 100 percent CASSOC setting", 0);

            /* Program MPAM2_EL2 with test_partid and default PMG */
            status = val_mpam_program_el2(test_partid, DEFAULT_PMG);
            if (status) {
                val_print(ACS_PRINT_ERR, "\n       MPAM2_EL2 programming failed", 0);
                /* Free the buffers to the heap manager */
                val_memory_free_aligned(src_buf);
                val_memory_free_aligned(dest_buf);
                val_set_status(index, RESULT_FAIL(TEST_NUM, 02));
                return;
            }

            start_count = val_mpam_read_csumon(msc_index);
            val_print(ACS_PRINT_DEBUG, "\n       Start Count = 0x%lx", start_count);

            /* Start mem copy */
            val_memcpy(src_buf, dest_buf, BUFFER_SIZE);

            end_count = val_mpam_read_csumon(msc_index);
            val_print(ACS_PRINT_DEBUG, "\n       End Count = 0x%lx", end_count);

            /* Disable CSU MON */
            val_mpam_csumon_disable(msc_index);

            /* Read CSU MON */
            counter[0] = end_count - start_count;
            val_print(ACS_PRINT_DEBUG, "\n       Count Difference = 0x%lx",
                                      end_count - start_count);

            /***  Scenario 2: Buffer copy with 50% CASSOC settings  ***/
            val_print(ACS_PRINT_DEBUG, "\n       Scenario 2: 50 percent CASSOC setting", 0);

            /* Disable CCAP and CPOR settings -> Set them to max */
            if (val_mpam_supports_cpor(msc_index))
                val_mpam_configure_cpor(msc_index, test_partid - 1, 100);

            if (val_mpam_supports_ccap(msc_index))
                val_mpam_configure_ccap(msc_index, test_partid - 1, 0, 100);

            /* Configure different PARTID with 50% cassoc settings */
            val_mpam_configure_cassoc(msc_index, test_partid - 1, CASSOC_PARTITION_50);

            /* Configure CSU Monitor */
            val_mpam_configure_csu_mon(msc_index, test_partid - 1, DEFAULT_PMG, 0);

            /* Enable CSU Monitor */
            val_mpam_csumon_enable(msc_index);

            /* Wait for MAX_NRDY_USEC after monitor config change */
            nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
            while (nrdy_timeout) {
                --nrdy_timeout;
            };

            /* Program MPAM2_EL2 with test_partid-1 and default PMG */
            status = val_mpam_program_el2(test_partid - 1, DEFAULT_PMG);
            if (status) {
                val_print(ACS_PRINT_ERR, "\n       MPAM2_EL2 programming failed", 0);
                /* Free the buffers to the heap manager */
                val_memory_free_aligned(src_buf);
                val_memory_free_aligned(dest_buf);
                val_set_status(index, RESULT_FAIL(TEST_NUM, 03));
                return;
            }

            start_count = val_mpam_read_csumon(msc_index);
            val_print(ACS_PRINT_DEBUG, "\n       Start Count = 0x%lx", start_count);

            /* Start mem copy */
            val_memcpy(src_buf, dest_buf, BUFFER_SIZE);

            end_count = val_mpam_read_csumon(msc_index);
            val_print(ACS_PRINT_DEBUG, "\n       End Count = 0x%lx", end_count);

            /* Disable CSU MON */
            val_mpam_csumon_disable(msc_index);

            /* Read CSU MON */
            counter[1] = end_count - start_count;
            val_print(ACS_PRINT_DEBUG, "\n       Count Difference = 0x%lx",
                                      end_count - start_count);

            /***  Scenario 3: Buffer copy with 25% CASSOC settings  ***/
            val_print(ACS_PRINT_DEBUG, "\n       Scenario 3: 25 percent CASSOC setting", 0);

            /* Disable CCAP and CPOR settings -> Set them to max */
            if (val_mpam_supports_cpor(msc_index))
                val_mpam_configure_cpor(msc_index, test_partid - 2, 100);

            if (val_mpam_supports_ccap(msc_index))
                val_mpam_configure_ccap(msc_index, test_partid - 2, 0, 100);

            /* Configure different PARTID with 25% cassoc settings */
            val_mpam_configure_cassoc(msc_index, test_partid - 2, CASSOC_PARTITION_25);

            /* Configure CSU Monitor */
            val_mpam_configure_csu_mon(msc_index, test_partid - 2, DEFAULT_PMG, 0);

            /* Enable CSU Monitor */
            val_mpam_csumon_enable(msc_index);

            /* Wait for MAX_NRDY_USEC after monitor config change */
            nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
            while (nrdy_timeout) {
                --nrdy_timeout;
            };

            /* Program MPAM2_EL2 with test_partid-2 and default PMG */
            status = val_mpam_program_el2(test_partid - 2, DEFAULT_PMG);
            if (status) {
                val_print(ACS_PRINT_ERR, "\n       MPAM2_EL2 programming failed", 0);
                /* Free the buffers to the heap manager */
                val_memory_free_aligned(src_buf);
                val_memory_free_aligned(dest_buf);
                val_set_status(index, RESULT_FAIL(TEST_NUM, 04));
                return;
            }

            start_count = val_mpam_read_csumon(msc_index);
            val_print(ACS_PRINT_DEBUG, "\n       Start Count = 0x%lx", start_count);

            /* Start mem copy */
            val_memcpy(src_buf, dest_buf, BUFFER_SIZE);

            end_count = val_mpam_read_csumon(msc_index);
            val_print(ACS_PRINT_DEBUG, "\n       End Count = 0x%lx", end_count);

            /* Disable CSU MON */
            val_mpam_csumon_disable(msc_index);

            /* Read CSU MON */
            counter[2] = end_count - start_count;
            val_print(ACS_PRINT_DEBUG, "\n       Count Difference = 0x%lx",
                                      end_count - start_count);

            /* Compare CSU mon counter across all 3 scenarios.
               As the cache associativity allocated to an MSC is restricted further,
               the number of cache lines in a set that the MSC can occupy is
               expected to decrease in each scenario, resulting in reduced CSU MON counter */

            if (!((counter[0] > counter[1]) && (counter[1] > counter[2]))) {
                val_print(ACS_PRINT_ERR, "\n       Unexpected cache usage behavior observed", 0);
                val_print(ACS_PRINT_ERR, "\n       CASSOC 100 percent count = 0x%lx", counter[0]);
                val_print(ACS_PRINT_ERR, "\n       CASSOC 50 percent count  = 0x%lx", counter[1]);
                val_print(ACS_PRINT_ERR, "\n       CASSOC 25 percent count  = 0x%lx", counter[2]);
                test_fail++;
            }

            /* Restore MPAM2_EL2 settings */
            val_mpam_reg_write(MPAM2_EL2, saved_el2);

            /* Free the buffers to the heap manager */
            val_memory_free_aligned(src_buf);
            val_memory_free_aligned(dest_buf);
        }
    }

    if (test_skip)
        val_set_status(index, RESULT_SKIP(TEST_NUM, 3));
    else if (test_fail)
        val_set_status(index, RESULT_FAIL(TEST_NUM, 5));
    else
        val_set_status(index, RESULT_PASS(TEST_NUM, 1));

    return;
}

uint32_t partition004_entry(void)
{
    uint32_t status  = ACS_STATUS_FAIL;
    uint32_t num_pe  = 1;

    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

    /* Check if test needs to be skipped - based on user configuration */
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* Get the result from PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}