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
#include "acs_mpam.h"
#include "acs_mpam_reg.h"
#include "acs_memory.h"
#include "val_interface.h"

#define TEST_NUM   ACS_MPAM_CACHE_TEST_NUM_BASE + 6
#define TEST_RULE  ""
#define TEST_DESC  "Check CMIN resource controls          "

/* Test Algorithm
1. Identify an MSC that supports Cache Minimum-Capacity resource control by checking
   both MPAMF_IDR.HAS_CCAP_PART and MPAMF_CCAP_IDR.HAS_CMIN are set.
2. Select a valid PARTID, for example PARTID_X, and configure MPAMCFG_CMAX to allocate max of 100%
   of the cache capacity to this PARTID.
3. Perform a memory workload that utilizes the cache to ensure that PARTID_X is actively
   using its allocated cache capacity.
4. Select another PARTID, PARTID_Y, and similarly configure MPAMCFG_CMAX to allocate max of
   50% of the cache capacity to it.
5. Perform a memory workload that utilizes the cache to ensure that PARTID_Y is actively
   using its allocated cache capacity.
6. Disable PARTID_Y by writing its ID to MPAMCFG_DIS.PARTID without setting the NFU bit,
   effectively marking it's cache lines as highest priority for eviction.
7. Select a third PARTID, PARTID_Z, and configure MPAMCFG_CMIN to request 75% of the cache capacity
8. Perform a memory workload that utilizes the cache to ensure that PARTID_Z is actively trying
   to use the cache.
Info - Since cache usage by PARTID_Z is below its CMIN threshold,
       it should be given elevated allocation priority.
Info - During cache pressure, the system is expected to evict cache lines from the disabled
       PARTID_Y first, followed by evictions from PARTID_X
9. Use the CSU monitor to observe and confirm that cache usage increases for PARTID_Z while eviction
   or reduction in cache usage occurs for PARTID_X and disabled PARTID_Y.
10. If this behavior is observed, the test passes as it confirms that minimum-capacity rules and
    priority adjustments are correctly enforced.
11. If PARTID_Z does not receive allocation priority or if evictions do not follow the
    expected order, the test fails.
*/

#define CMIN_SCENARIOS 3
#define BUFFER_SIZE           0x100000  /* 1MB */

static void
payload(void)
{
    uint32_t msc_cnt   = val_mpam_get_msc_count();
    uint32_t llc_index = val_cache_get_llc_index();
    uint32_t index     = val_pe_get_index_mpid(val_pe_get_mpid());

    uint16_t partid_x     = 1;
    uint16_t partid_y     = 2;
    uint16_t partid_z     = 3;
    uint32_t max_partid   = 0;
    uint32_t status       = 0;
    uint32_t num_mon      = 0;
    bool     test_fail    = 0;
    bool     test_skip    = 1;
    bool     is_partid_en = 1;
    void     *src_buf     = NULL;
    void     *dest_buf    = NULL;
    uint32_t page_size;
    uint32_t num_pages;
    volatile uint64_t nrdy_timeout = 0;

    uint32_t msc_index, rsrc_index, rsrc_cnt;
    uint64_t saved_el2;
    uint64_t cache_identifier;
    uint32_t counter[CMIN_SCENARIOS];

    page_size = val_memory_page_size();
    num_pages = (uint32_t)((BUFFER_SIZE + page_size - 1) / page_size);

    /* Check if LLC is valid */
    if (llc_index == CACHE_TABLE_EMPTY) {
        val_print(ACS_PRINT_DEBUG, "\n       No LLC found, skipping test", 0);
        val_set_status(index, RESULT_SKIP(TEST_NUM, 1));
        return;
    }

    /* Get the LLC Cache ID */
    cache_identifier = val_cache_get_info(CACHE_ID, llc_index);
    if (cache_identifier == INVALID_CACHE_INFO) {
        val_print(ACS_PRINT_DEBUG, "\n       Invalid LLC ID, skipping test", 0);
        val_set_status(index, RESULT_SKIP(TEST_NUM, 2));
        return;
    }

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

            /* Check if the MSC support selected PARTIDs (max_partid >= 2) */
            max_partid = val_mpam_get_max_partid(msc_index);
            if (max_partid < partid_z) {
                val_print(ACS_PRINT_DEBUG,
                          "\n       MSC %d does not support required PARTIDs, skipping",
                          msc_index);
                continue;
            }

            /* Skip the MSC if no CSU MON are present */
            num_mon = val_mpam_get_csumon_count(msc_index);
            if (num_mon == 0) {
                val_print(ACS_PRINT_DEBUG, "\n       MSC %u has no CSU MON, skipping MSC",
                                                                                        msc_index);
                continue;
            }

            /* Check if PARTID Disabling is supported. The test relies on this feature. Skip the
               MSC if endis not present */
            if (!val_mpam_msc_supports_partid_endis(msc_index)) {
                val_print(ACS_PRINT_DEBUG,
                          "\n       MSC %d doesn't support PARTID disable, skipping MSC",
                          msc_index);
                continue;
            }

            /* Step 1: Check if the MSC supports Cache Minimum-Capacity resource control */
            if (!val_mpam_msc_supports_cmin(msc_index)) {
                val_print(ACS_PRINT_DEBUG,
                          "\n       MSC %d does not support CMIN-Capacity control, skipping",
                          msc_index);
                continue;
            }

            /* Step 2: Program PARTID_X with 100% CMAX settings */
            val_mpam_configure_ccap(msc_index, partid_x, SOFTLIMIT_DIS, 100);

            /* Step 3: Perform a memory workload for PARTID_X */
            /* Allocate memory for source and destination buffers */
            src_buf  = (void *)val_memory_alloc_pages(num_pages);
            dest_buf = (void *)val_memory_alloc_pages(num_pages);

            if ((src_buf == NULL) || (dest_buf == NULL)) {
                val_print(ACS_PRINT_ERR, "\n       Mem allocation failed", 0);
                val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
                if (dest_buf != NULL)
                    val_memory_free_pages(dest_buf, num_pages);
                if (src_buf != NULL)
                    val_memory_free_pages(src_buf, num_pages);
                return;
            }

            /* Save the current MPAM2_EL2 settings */
            saved_el2 = val_mpam_reg_read(MPAM2_EL2);

            status = val_mpam_program_el2(partid_x, DEFAULT_PMG);
            if (status) {
                val_print(ACS_PRINT_ERR, "\n       MPAM2_EL2 programming failed", 0);
                goto cleanup;
            }

            /* Test runs on atleast one MSC */
            test_skip = 0;

            /* Configure CSU Monitor */
            val_mpam_configure_csu_mon(msc_index, partid_x, DEFAULT_PMG, 0);

            /* Enable CSU Monitor */
            val_mpam_csumon_enable(msc_index);

            /* Wait for MAX_NRDY_USEC after monitor config change */
            nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
            while (nrdy_timeout) {
                --nrdy_timeout;
            };

            val_memcpy(dest_buf, src_buf, BUFFER_SIZE);

            /* Wait for some time before the memcpy settles and counters update */
            val_time_delay_ms(TIMEOUT_MEDIUM);

            /* Read the monitor counter for PARTID_X */
            counter[0] = val_mpam_read_csumon(msc_index);
            val_print(ACS_PRINT_TEST, "\n       PARTID_X Counter: 0x%x", counter[0]);

            /* Disable CSU MON */
            val_mpam_csumon_disable(msc_index);

            val_pe_cache_invalidate_range((uint64_t)dest_buf, BUFFER_SIZE);
            val_pe_cache_invalidate_range((uint64_t)src_buf, BUFFER_SIZE);
            val_mem_issue_dsb();

            /* Step 4: Program PARTID_Y with 50% CMAX settings */
            val_mpam_configure_ccap(msc_index, partid_y, SOFTLIMIT_DIS, 50);

            /* Step 5: Perform a memory workload for PARTID_Y */
            status = val_mpam_program_el2(partid_y, DEFAULT_PMG);
            if (status) {
                val_print(ACS_PRINT_ERR, "\n       MPAM2_EL2 programming failed", 0);
                goto cleanup;
            }

            /* Configure CSU Monitor for PARTID_Y */
            val_mpam_configure_csu_mon(msc_index, partid_y, DEFAULT_PMG, 0);

            /* Enable CSU Monitor */
            val_mpam_csumon_enable(msc_index);

            /* Wait for MAX_NRDY_USEC after monitor config change */
            nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
            while (nrdy_timeout) {
                --nrdy_timeout;
            }

            val_memcpy(dest_buf, src_buf, BUFFER_SIZE);

            /* Wait for some time before the memcpy settles and counters update */
            val_time_delay_ms(TIMEOUT_MEDIUM);

            /* Read the monitor counter for PARTID_Y */
            counter[1] = val_mpam_read_csumon(msc_index);
            val_print(ACS_PRINT_TEST, "\n       PARTID_Y Counter: 0x%x", counter[1]);

            /* Read PARTID_X counter after the second buffer copy */
            val_mpam_configure_csu_mon(msc_index, partid_x, DEFAULT_PMG, 0);
            if (val_mpam_read_csumon(msc_index) > counter[0]) {
                val_print(ACS_PRINT_ERR,
                    "\n       PARTID_X usage increased after PARTID_Y workload, failing test", 0);
                test_fail = 1;
            }

            counter[0] = val_mpam_read_csumon(msc_index);
            val_print(ACS_PRINT_TEST,
                            "\n       PARTID_X Counter after PARTID_Y workload: 0x%x", counter[0]);

            val_pe_cache_invalidate_range((uint64_t)dest_buf, BUFFER_SIZE);
            val_pe_cache_invalidate_range((uint64_t)src_buf, BUFFER_SIZE);
            val_mem_issue_dsb();

            /* Step 6: Disable PARTID_Y */
            val_mpam_msc_endis_partid(msc_index,
                                      0, // disable PARTID
                                      0, // NFU not set
                                      partid_y);

            /* Keep track of disabled PARTID */
            is_partid_en = 0;

            /* Step 7: configure MPAMCFG_CMIN to request 75% of the cache capacity */
            val_mpam_configure_cmin(msc_index, partid_z, 75);

            /* Check PARTID_Z cache usage before performing the buffer copy */
            val_mpam_configure_csu_mon(msc_index, partid_z, DEFAULT_PMG, 0);

            counter[2] = val_mpam_read_csumon(msc_index);
            val_print(ACS_PRINT_TEST,
                                    "\n       PARTID_Z Counter before workload: 0x%x", counter[2]);

            /* Step 8: Perform a memory workload for PARTID_Z */
            status = val_mpam_program_el2(partid_z, DEFAULT_PMG);
            if (status) {
                val_print(ACS_PRINT_ERR, "\n       MPAM2_EL2 programming failed", 0);
                goto cleanup;
            }

            /* Perform buffer copy */
            val_memcpy(dest_buf, src_buf, BUFFER_SIZE);

            /* Wait for some time before the memcpy settles and counters update */
            val_time_delay_ms(TIMEOUT_MEDIUM);

            /* Read the monitor counter for PARTID_Z */
            if (val_mpam_read_csumon(msc_index) <= counter[2]) {
                val_print(ACS_PRINT_ERR,
                    "\n       PARTID_Z usage did not increase after workload, failing test", 0);
                test_fail = 1;
            }

            counter[2] = val_mpam_read_csumon(msc_index);
            val_print(ACS_PRINT_TEST, "\n       PARTID_Z Counter after workload: %x", counter[2]);

            /* Read PARTID_X and PARTID_Y counters after PARTID_Z workload */
            val_mpam_configure_csu_mon(msc_index, partid_x, DEFAULT_PMG, 0);
            if (val_mpam_read_csumon(msc_index) > counter[0]) {
                val_print(ACS_PRINT_ERR,
                    "\n       PARTID_X usage increased after PARTID_Z workload, failing test", 0);
                test_fail = 1;
            }

            counter[0] = val_mpam_read_csumon(msc_index);
            val_print(ACS_PRINT_TEST,
                            "\n       PARTID_X Counter after PARTID_Z workload: 0x%x", counter[0]);

            val_mpam_configure_csu_mon(msc_index, partid_y, DEFAULT_PMG, 0);
            if (val_mpam_read_csumon(msc_index) > counter[1]) {
                val_print(ACS_PRINT_ERR,
                    "\n       PARTID_Y usage increased after PARTID_Z workload, failing test", 0);
                test_fail = 1;
            }

            counter[1] = val_mpam_read_csumon(msc_index);
            val_print(ACS_PRINT_TEST,
                            "\n       PARTID_Y Counter after PARTID_Z workload: 0x%x", counter[1]);

            if (counter[1] > counter[0]) {
                val_print(ACS_PRINT_ERR,
                    "\n       PARTID_Y usage higher than PARTID_X, failing test", 0);
                test_fail = 1;
            }

            /* Disable CSU monitor */
            val_mpam_csumon_disable(msc_index);

            /* Restore MPAM2_EL2 settings */
            val_mpam_reg_write(MPAM2_EL2, saved_el2);

            /* Free the buffers to the heap manager */
            if (dest_buf != NULL) {
                val_pe_cache_invalidate_range((uint64_t)dest_buf, BUFFER_SIZE);
                val_mem_issue_dsb();
                val_memory_free_pages(dest_buf, num_pages);
            }
            if (src_buf != NULL) {
                val_pe_cache_invalidate_range((uint64_t)src_buf, BUFFER_SIZE);
                val_mem_issue_dsb();
                val_memory_free_pages(src_buf, num_pages);
            }

            /* Re-enable PARTID-Y for the next tests to behave properly */
            val_mpam_msc_endis_partid(msc_index,
                                      1, // enable PARTID
                                      0, // NFU not set
                                      partid_y);
            is_partid_en = 1;
        }
    }

    if (test_skip) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 3));
    } else if (test_fail) {
        val_set_status(index, RESULT_FAIL(TEST_NUM, 2));
    } else {
        val_set_status(index, RESULT_PASS(TEST_NUM, 1));
    }
    return;

cleanup:
    if (!is_partid_en) {
        /* Re-enable PARTID-Y for the next tests to behave properly */
        val_mpam_msc_endis_partid(msc_index,
                                  1, // enable PARTID
                                  0, // NFU not set
                                  partid_y);
    }

    /* Free the buffers to the heap manager */
    if (dest_buf != NULL) {
        val_pe_cache_invalidate_range((uint64_t)dest_buf, BUFFER_SIZE);
        val_mem_issue_dsb();
        val_memory_free_pages(dest_buf, num_pages);
    }
    if (src_buf != NULL) {
        val_pe_cache_invalidate_range((uint64_t)src_buf, BUFFER_SIZE);
        val_mem_issue_dsb();
        val_memory_free_pages(src_buf, num_pages);
    }

    /* Disable CSU Mon */
    val_mpam_csumon_disable(msc_index);

    /* Restore MPAM2_EL2 settings */
    val_mpam_reg_write(MPAM2_EL2, saved_el2);

    val_set_status(index, RESULT_FAIL(TEST_NUM, 03));
    return;
}

uint32_t partition006_entry(void)
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
