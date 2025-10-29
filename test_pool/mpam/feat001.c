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
#include "val/include/acs_memory.h"
#include "val/include/val_interface.h"

#define TEST_NUM   ACS_MPAM_CACHE_TEST_NUM_BASE + 25
#define TEST_RULE  ""
#define TEST_DESC  "Check PARTID Disable functionality    "

/* Test algorithm -
1. Check if the MSC supports PARTID enable/disable control by confirming MPAMF_IDR.HAS_ENDIS == 1.
2. Select a valid PARTID (e.g., PARTID_X) and identify a cache resource that supports
   portion-based partitioning.
3. Program PARTID_X with a defined partition share (e.g., 100%) using the cache's CPOR config.
4. Set MPAM2_EL2 to assign PARTID_X to the active PE or thread.
5. Perform a memory workload (e.g., memcpy or streaming load/store) large enough to utilize a
   measurable portion of the cache.
6. Use the CSU monitor to measure the cache usage by PARTID_X. Record the CSU counter as Value A.
7. Disable PARTID_X by writing its ID to MPAMCFG_DIS.PARTID. Do not set the NFU bit.
8. Generate Traffic for PARTID_Y with CPOR (100%)
9. Use the CSU monitor to measure the cache usage by PARTID_X. Record the CSU counter as Value B.
10. Value B should be less than Value A and gradually decreasing.
11. Now, re-enable PARTID_X by writing it to MPAMCFG_EN.PARTID and Disable PARTID_Y
12. Re-run the same memory workload once more from the same PE.
13. Measure cache usage again using CSU. Record this as Value C.
Validate the results:
Value A ~= Value C: confirms that the original partition settings were preserved after re-enabling.
Value B < Value A: confirms that the disabled PARTID was deprioritized or blocked from allocation.
If both conditions hold, the test is passed.
*/

#define BUFFER_SIZE   0x6400000  /* 100 MB */
#define NUM_SCENARIOS 3

static void
payload(void)
{

    uint32_t msc_cnt  = val_mpam_get_msc_count();
    uint32_t llc_idx  = val_cache_get_llc_index();
    uint32_t index    = val_pe_get_index_mpid(val_pe_get_mpid());

    void     *src_buf;
    void     *dest_buf;
    uint32_t start_count  = 0;
    uint32_t end_count    = 0;
    uint32_t test_fail    = 0;
    uint16_t partid_x     = 1;
    uint16_t partid_y     = 2;
    uint32_t msc_index, rsrc_index, rsrc_cnt;
    uint32_t nrdy_timeout;
    uint64_t cache_identifier;
    uint64_t counter[NUM_SCENARIOS];
    uint64_t saved_el2;
    bool     cpart_flag  = 0;
    bool     test_skip   = 1;
    uint32_t status      = 0;
    uint32_t num_mon     = 0;

    /* Check if LLC is valid */
    if (llc_idx == CACHE_TABLE_EMPTY) {
        val_print(ACS_PRINT_DEBUG, "\n       LLC not present, Skipping test", 0);
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

    /* Iterate through all available LLC MSCs and their resources */
    for (msc_index = 0; msc_index < msc_cnt; msc_index++)
    {

        /* Check if the MSC supports PARTID Disable feature (MPAMCFG.HAS_ENDIS bit) */
        if (!val_mpam_msc_supports_partid_endis(msc_index)) {
            val_print(ACS_PRINT_DEBUG,
                      "\n       MSC index %d does not support PARTID Enable/Disable, skipping...",
                      msc_index);
            continue;
        }

        /* Iterate through resources of MSC to find a resource which implements CPOR or CCAP */
        rsrc_cnt = val_mpam_get_info(MPAM_MSC_RSRC_COUNT, msc_index, 0);
        for (rsrc_index = 0; rsrc_index < rsrc_cnt; rsrc_index++)
        {

            if (val_mpam_get_info(MPAM_MSC_RSRC_TYPE, msc_index, rsrc_index)
                != MPAM_RSRC_TYPE_PE_CACHE)
                continue;

            if (val_mpam_get_info(MPAM_MSC_RSRC_DESC1, msc_index, rsrc_index)
                != cache_identifier)
                continue;

            /* Select resource instance if RIS feature implemented */
            if (val_mpam_msc_supports_ris(msc_index))
                val_mpam_memory_configure_ris_sel(msc_index, rsrc_index);

            if (val_mpam_get_max_partid(msc_index) < partid_y) {
                val_print(ACS_PRINT_DEBUG,
                    "\n       MSC %d does not support required PARTIDs, skipping MSC", msc_index);
                continue;
            }

            num_mon     = 0;
            cpart_flag  = 0;
            start_count = 0;
            end_count   = 0;

            /* Check if MSC implements CSU monitors */
            if (val_mpam_supports_csumon(msc_index))
                num_mon = val_mpam_get_csumon_count(msc_index);

            if (num_mon == 0) {
                val_print(ACS_PRINT_DEBUG,
                    "\n       MSC %d does not have CSU monitors, skipping MSC", msc_index);
                continue;
            }

            /* Check if MSC supports CPOR or CCAP.
               Select Cache partitioning scheme based on the support */
            if (val_mpam_supports_cpor(msc_index)) {
                cpart_flag = 1;
                val_print(ACS_PRINT_DEBUG, "\n       MSC %d supports CPOR partitioning", msc_index);

                /* The test uses CPOR as cache partitioning scheme. Disable CCAP settings */
                val_mpam_configure_ccap(msc_index, partid_x, 0, 100);
                val_mpam_configure_ccap(msc_index, partid_y, 0, 100);

            } else
            if (val_mpam_supports_ccap(msc_index)) {
                cpart_flag = 2;
                val_print(ACS_PRINT_DEBUG, "\n       MSC %d supports CCAP partitioning", msc_index);

                /* The test uses CCAP as cache partitioning scheme. Disable CPOR settings */
                val_mpam_configure_cpor(msc_index, partid_x, 100);
                val_mpam_configure_cpor(msc_index, partid_y, 100);

            } else {
                val_print(ACS_PRINT_DEBUG,
                    "\n       MSC %d does not support CPOR/CCAP partitioning, skipping...",
                    msc_index);
                continue;
            }

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

            /* Step 3: Program PARTID_X with 100% Cache partition control */
            if (cpart_flag == 1)
                val_mpam_configure_cpor(msc_index, partid_x, 100);
            else
                val_mpam_configure_ccap(msc_index, partid_x, 0, 100);

            /* Configure CSU Monitor */
            val_mpam_configure_csu_mon(msc_index, partid_x, DEFAULT_PMG, 0);

            /* Enable CSU Monitor */
            val_mpam_csumon_enable(msc_index);

            /* Wait for MAX_NRDY_USEC after monitor config change */
            nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
            while (nrdy_timeout) {
                --nrdy_timeout;
            };

            /* Step 4: Set MPAM2_EL2 to assign PARTID_X to the active PE or thread */
            saved_el2 = val_mpam_reg_read(MPAM2_EL2);

            status = val_mpam_program_el2(partid_x, DEFAULT_PMG);
            if (status) {
                val_print(ACS_PRINT_ERR, "\n       MPAM2_EL2 programming failed", 0);
                /* Free the buffers to the heap manager */
                val_memory_free_aligned(src_buf);
                val_memory_free_aligned(dest_buf);
                val_set_status(index, RESULT_FAIL(TEST_NUM, 02));
                return;
            }

            /* Step 5: Start mem copy */
            val_memcpy(src_buf, dest_buf, BUFFER_SIZE);

            /* Read CSU monitor cnt - number of cache lines filled by PARTID_X at this point */
            end_count = val_mpam_read_csumon(msc_index);

            /* Disable CSU MON */
            val_mpam_csumon_disable(msc_index);

            /* Step 6 */
            counter[0] = end_count;
            val_print(ACS_PRINT_TEST, "\n       PARTID_X Counter (Scenario 1)= 0x%lx", counter[0]);

            /* Counter should be non-zero */
            if (counter[0] == 0) {
                val_print(ACS_PRINT_ERR,
                "\n       Scenario 1: PARTID_X counter is zero, test failed for MSC %d", msc_index);
                test_fail++;
                goto cleanup;
            }

            /* Step 7: Disable PARTID_X. MPAMCFG_DIS.PARTID = PARTID_X.
               Do not set NFU. We need the PARTID later */
            status = val_mpam_msc_endis_partid(msc_index, 0, 0, partid_x);
            if (status) {
                test_fail++;
                goto cleanup;
            }

            /* Step 8: Program PARTID_Y with 100% Cache partitioning control */
            if (cpart_flag == 1)
                val_mpam_configure_cpor(msc_index, partid_y, 100);
            else
                val_mpam_configure_ccap(msc_index, partid_y, 0, 100);

            /* Reset CSU monitor counting PARTID_X, only a fail safe.
               It should not impact the next read anyway */
            val_mpam_reset_csumon(msc_index, 0);

            status = val_mpam_program_el2(partid_y, DEFAULT_PMG);
            if (status) {
                val_print(ACS_PRINT_ERR, "\n       MPAM2_EL2 programming failed", 0);
                /* Free the buffers to the heap manager */
                val_memory_free_aligned(src_buf);
                val_memory_free_aligned(dest_buf);
                val_set_status(index, RESULT_FAIL(TEST_NUM, 03));
                return;
            }

            /* Enable CSU Monitor */
            val_mpam_csumon_enable(msc_index);

            /* Wait for MAX_NRDY_USEC after monitor config change */
            nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
            while (nrdy_timeout) {
                --nrdy_timeout;
            };

            val_memcpy(src_buf, dest_buf, BUFFER_SIZE);

            /* Step 9 */
            end_count = val_mpam_read_csumon(msc_index);

            /* Disable CSU MON */
            val_mpam_csumon_disable(msc_index);

            counter[1] = end_count;
            val_print(ACS_PRINT_TEST, "\n       PARTID_X Counter (Scenario 2)= 0x%lx", counter[1]);

            /* Step 10: Validate Value B < Value A */
            /* Txn tagged with PARTID_Y but monitor counts PARTID X. PARTID_X is disabled, so
            cache lines occupied previously by PARTID_X should have highest eviction priority.
            Buffer tagged with PARTID_Y should evict PARTID_X cache lines */
            if (counter[1] >= counter[0]) {
                val_print(ACS_PRINT_ERR,
                "\n       Scenario 2: Counter more than S1, test failed for MSC %d", msc_index);
                test_fail++;
                goto cleanup;
            }

            /* Step 11: Enable PARTID_X and Disable PARTID_Y. H/w must remember PARTID_X config */
            status = val_mpam_msc_endis_partid(msc_index, 1, 0, partid_x);
            status |= val_mpam_msc_endis_partid(msc_index, 0, 0, partid_y);

            if (status) {
                test_fail++;
                goto cleanup;
            }

            /* Step 12: Program MPAM2_EL2 with PARTID_X and perform bufcopy */
            status = val_mpam_program_el2(partid_x, DEFAULT_PMG);
            if (status) {
                val_print(ACS_PRINT_ERR, "\n       MPAM2_EL2 programming failed", 0);
                /* Free the buffers to the heap manager */
                val_memory_free_aligned(src_buf);
                val_memory_free_aligned(dest_buf);
                val_set_status(index, RESULT_FAIL(TEST_NUM, 04));
                return;
            }

            /* Reset CSU monitor counting PARTID_X */
            val_mpam_reset_csumon(msc_index, 0);

            /* Enable CSU Monitor */
            val_mpam_csumon_enable(msc_index);

            /* Wait for MAX_NRDY_USEC after monitor config change */
            nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
            while (nrdy_timeout) {
                --nrdy_timeout;
            };

            val_memcpy(src_buf, dest_buf, BUFFER_SIZE);

            end_count = val_mpam_read_csumon(msc_index);

            /* Disable CSU MON */
            val_mpam_csumon_disable(msc_index);

            counter[2] = end_count;
            val_print(ACS_PRINT_TEST, "\n       PARTID_X Counter (Scenario 3)= 0x%lx", counter[2]);

            /* With PARTID_Y disabled and PARTID_X enabled, CPOR settings of PARTID_X must be
               restored by the H/W. PARTID_Y cache lines should be evicted by txn tagged with
               PARTID_X */
            if (counter[1] >= counter[2]) {
                val_print(ACS_PRINT_ERR,
                    "\n       Scenario 3: Counter less than S2, test failed for MSC %d", msc_index);
                test_fail++;
                goto cleanup;
            }

cleanup:
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

uint32_t feat001_entry(void)
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