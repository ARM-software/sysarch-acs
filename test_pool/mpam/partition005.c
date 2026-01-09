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

#include "val/include/acs_val.h"
#include "val/include/acs_pe.h"
#include "val/include/acs_mpam.h"
#include "val/include/acs_mpam_reg.h"
#include "val/include/acs_memory.h"
#include "val/include/val_interface.h"

#define TEST_NUM   ACS_MPAM_CACHE_TEST_NUM_BASE + 5
#define TEST_RULE  ""
#define TEST_DESC  "Check CMAX Partitioning with softlimit"

/* Test algorithm
1. Identify an MSC that supports cache maximum-capacity control and soft limit functionality by
   verifying that MPAMF_IDR.HAS_CCAP_PART, and MPAMF_CCAP_IDR.HAS_CMAX_SOFTLIM
   are all set.
2. Select two PARTIDs, PARTID_X and PARTID_Y.
3. Configure MPAMCFG_CMAX such that PARTID_X is assigned 75% of the cache and PARTID_Y is
   assigned 25%, both with SOFTLIM = 0.
4. Program the PE with PARTID_X using MPAM2_EL2.
5. Generate a memory workload under PARTID_X to fully utilize its 75% cache share.
6. Measure cache usage using the CSU monitor and record it as Value A.
7. Program MPAM2_EL2 with PARTID_Y and trigger the same workload for PARTID_Y to fill up
   the 25% of it's cache share.
8. Disable PARTID_Y by writing to MPAMCFG_DIS.PARTID (do not set NFU).
9. Re-program the PE with PARTID_X again and set SOFTLIM = 1 for PARTID_X.
10. Perform the same memory workload from the PE.
11. Measure cache usage again with the CSU monitor and record it as Value B.
12. Compare Value B with Value A.
13. If Value B > Value A, it confirms that PARTID_X was able to use the part of the 25% cache that
    was previously assigned to PARTID_Y, indicating correct SOFTLIM behavior.
14. If Value B is not greater than Value A, the test fails
*/

#define CMAX_SCENARIO_MAX 2
#define BUFFER_SIZE       0x6400000   /* 100 MB */

static void
payload(void)
{
    uint32_t msc_cnt   = val_mpam_get_msc_count();
    uint32_t llc_index = val_cache_get_llc_index();
    uint32_t index     = val_pe_get_index_mpid(val_pe_get_mpid());

    uint16_t partid_x     = 1;
    uint16_t partid_y     = 2;
    uint32_t max_partid   = 0;
    uint32_t status       = 0;
    uint32_t num_mon      = 0;
    bool     test_fail    = 0;
    bool     test_skip    = 1;
    void     *src_buf     = 0;
    void     *dest_buf    = 0;
    volatile uint64_t nrdy_timeout = 0;
    volatile uint64_t buf_timeout  = 0;

    uint32_t msc_index, rsrc_index, rsrc_cnt;
    uint64_t saved_el2;
    uint64_t cache_identifier;
    uint32_t counter[CMAX_SCENARIO_MAX];

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
            if (max_partid < partid_y) {
                val_print(ACS_PRINT_DEBUG,
                          "\n       MSC %u does not support required PARTIDs, skipping",
                          msc_index);
                continue;
            }

            /* Step 1: Identify an MSC that supports cache maximum-capacity control and soft
                       limit functionality */
            if (!(val_mpam_msc_supports_cmax_softlim(msc_index))) {
                val_print(ACS_PRINT_DEBUG,
                          "\n       MSC %u doesn't support CMAX with softlimit, skipping MSC",
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
                          "\n       MSC %u doesn't support PARTID disable, skipping MSC",
                          msc_index);
                continue;
            };

            /* Step 2-3: Configure MPAMCFG_CMAX such that PARTID_X is assigned 75% of the cache and
                         PARTID_Y is assigned 25%, both with SOFTLIM = 0.*/
            val_mpam_configure_ccap(msc_index, partid_x, MPAMCFG_CMAX_DISABLE_SOFTLIM, 75);
            val_mpam_configure_ccap(msc_index, partid_y, MPAMCFG_CMAX_DISABLE_SOFTLIM, 25);

            /* Disable CPOR and CASSOC settings. Set them to max */
            val_mpam_configure_cassoc(msc_index, partid_x, 100);
            val_mpam_configure_cassoc(msc_index, partid_y, 100);

            val_mpam_configure_cpor(msc_index, partid_x, 100);
            val_mpam_configure_cpor(msc_index, partid_y, 100);

            /* Allocate memory for source and destination buffers */
            src_buf  = (void *)val_aligned_alloc(MEM_ALIGN_4K, BUFFER_SIZE);
            dest_buf = (void *)val_aligned_alloc(MEM_ALIGN_4K, BUFFER_SIZE);

            if ((src_buf == NULL) || (dest_buf == NULL)) {
                val_print(ACS_PRINT_ERR, "\n       Mem allocation failed", 0);
                val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
                return;
            }

            /* Configure CSU Monitor */
            val_mpam_configure_csu_mon(msc_index, partid_x, DEFAULT_PMG, 0);

            /* Enable CSU Monitor */
            val_mpam_csumon_enable(msc_index);

            /* Wait for MAX_NRDY_USEC after monitor config change */
            nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
            while (nrdy_timeout) {
                --nrdy_timeout;
            };

            /* Save the current MPAM2_EL2 settings */
            saved_el2 = val_mpam_reg_read(MPAM2_EL2);

            val_print(ACS_PRINT_TEST, "\n       Scenario 1: PARTID_X without SOFTLIM", 0);

            /* Step 4: Program MPAM2_EL2 with partid_x and default PMG */
            status = val_mpam_program_el2(partid_x, DEFAULT_PMG);
            if (status) {
                val_print(ACS_PRINT_ERR, "\n       MPAM2_EL2 programming failed", 0);
                goto cleanup;
            }

            /* Test runs on atleast one MSC */
            test_skip = 0;

            /* Step 5 -  Trigger buffercpy workload */
            val_memcpy(src_buf, dest_buf, BUFFER_SIZE);

            /* wait for some time till memcpy settles */
            buf_timeout = TIMEOUT_MEDIUM;
            while (buf_timeout) {
                --buf_timeout;
            };

            /* Step 6 - Read the Cache line count used by PARTID X from CSU MON */
            counter[0] = val_mpam_read_csumon(msc_index);
            val_print(ACS_PRINT_TEST, "\n       Scenario 1: End Count = 0x%lx", counter[0]);

            /* Disable CSU MON */
            val_mpam_csumon_disable(msc_index);

            /* Step 7: Program MPAM2_EL2 with PARTID_Y and fill up 25% of cache with PARTID_Y.
                       No need to monitor the transactions */
            status = val_mpam_program_el2(partid_y, DEFAULT_PMG);
            if (status) {
                val_print(ACS_PRINT_ERR, "\n       MPAM2_EL2 programming failed", 0);
                goto cleanup;
            }

            val_memcpy(src_buf, dest_buf, BUFFER_SIZE);

            /* wait for some time till memcpy settles */
            buf_timeout = TIMEOUT_MEDIUM;
            while (buf_timeout) {
                --buf_timeout;
            };

            /* Step 8: Disable PARTID_Y. The disabled PARTID's cache will have higher priority of
                       eviction */
            val_mpam_msc_endis_partid(msc_index,
                                      0, // disable PARTID
                                      0, // NFU not set
                                      partid_y);

            /* Step 9: Re-program the PE with PARTID_X again and set SOFTLIM = 1 for PARTID_X.*/
            status = val_mpam_program_el2(partid_x, DEFAULT_PMG);
            if (status) {
                val_print(ACS_PRINT_ERR, "\n       MPAM2_EL2 programming failed", 0);

                /* Re-enable PARTID-Y for the next tests to behave properly */
                val_mpam_msc_endis_partid(msc_index,
                                      1, // enable PARTID
                                      0, // NFU not set
                                      partid_y);
                goto cleanup;
            }

            val_mpam_configure_ccap(msc_index, partid_x, MPAMCFG_CMAX_ENABLE_SOFTLIM, 75);

            /* Step 10: Perform bufcopy but monitor the transactions now */
            val_mpam_configure_csu_mon(msc_index, partid_x, DEFAULT_PMG, 0);

            /* Wait for MAX_NRDY_USEC after monitor config change */
            nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
            while (nrdy_timeout) {
                --nrdy_timeout;
            };

            val_mpam_csumon_enable(msc_index);

            val_memcpy(src_buf, dest_buf, BUFFER_SIZE);

            /* wait for some time till memcpy settles */
            buf_timeout = TIMEOUT_MEDIUM;
            while (buf_timeout) {
                --buf_timeout;
            };

            /* Step 11: Measure cache usage again with the CSU monitor */
            counter[1] = val_mpam_read_csumon(msc_index);
            val_print(ACS_PRINT_TEST, "\n       Scenario 2: End Count = 0x%lx", counter[1]);

            /* Compare the result. Counter[1] should be more than Counter[0]. The softlimiting
               should allow some of the disabled PARTID_Y's cache lines to be used by PARTID_X */
            if (counter[0] >= counter[1]) {
                val_print(ACS_PRINT_ERR,
                          "\n       Test Failed: Softlimit not working as expected", 0);
                test_fail = 1;
            }

            /* Disable CSU Mon */
            val_mpam_csumon_disable(msc_index);

            /* Restore MPAM2_EL2 settings */
            val_mpam_reg_write(MPAM2_EL2, saved_el2);

            /* Free the buffers to the heap manager */
            val_memory_free_aligned(src_buf);
            val_memory_free_aligned(dest_buf);

            /* Re-enable PARTID-Y for the next tests to behave properly */
            val_mpam_msc_endis_partid(msc_index,
                                      1, // enable PARTID
                                      0, // NFU not set
                                      partid_y);

        }
    }

    if (test_skip) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 1));
    } else if (test_fail) {
        val_set_status(index, RESULT_FAIL(TEST_NUM, 2));
    } else {
        val_set_status(index, RESULT_PASS(TEST_NUM, 1));
    }

    return;

cleanup:
    /* Free the buffers to the heap manager */
    val_memory_free_aligned(src_buf);
    val_memory_free_aligned(dest_buf);

    /* Disable CSU Mon */
    val_mpam_csumon_disable(msc_index);

    /* Restore MPAM2_EL2 settings */
    val_mpam_reg_write(MPAM2_EL2, saved_el2);

    val_set_status(index, RESULT_FAIL(TEST_NUM, 03));
    return;
}

uint32_t partition005_entry(void)
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