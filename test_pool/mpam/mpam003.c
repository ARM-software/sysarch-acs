/** @file
 * Copyright (c) 2024-2026, Arm Limited or its affiliates. All rights reserved.
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

#include "acs_val.h"
#include "acs_pe.h"
#include "acs_common.h"
#include "val_interface.h"
#include "acs_memory.h"
#include "acs_pe.h"
#include "acs_mpam.h"


#define TEST_NUM   (ACS_MPAM_TEST_NUM_BASE + 3)
#define TEST_RULE  "S_L7MP_05"
#define TEST_DESC  "Check for MPAM MBWUs Monitor func     "

/* Test algorithm -
1. Check that the PE implements FEAT_MPAM and that MPAM memory resource groups are present.
   (Groups contain resources with matching locator type, descriptor1, and descriptor2)
2. For each memory resource group, identify every MSC and resource that manages the group.
3. Select the resource instance when RIS is supported, and verify that every MSC implements at
   least one Memory Bandwidth Usage (MBWU) monitor.
4. Obtain a memory range for the resource and allocate source and destination buffers from
   that range.
5. Clean and invalidate the buffers so that the test traffic reaches the monitored memory resource.
6. Configure an MBWU monitor in every MSC in the resource group for the default PARTID and PMG,
   and program MPAM2_EL2 with the same identifiers.
7. Enable all group monitors, wait for the maximum reported NRDY interval, and copy the source
   buffer to the destination buffer. Clean the destination buffer to push the writes to memory.
8. Restore MPAM2_EL2, disable the monitors, and wait for the monitors to become ready.
9. Read and add the byte counts from every MSC to obtain the aggregate bandwidth usage for the
   memory resource group.
10. Verify that the aggregate count is greater than the minimum permitted count (the expected read
    and write traffic minus the allowed implementation margin).
11. Reset all configured monitors, free the buffers, and repeat the test for every memory resource
    group. The test passes when all tested groups report a valid aggregate byte count.
*/

#define BUFFER_SIZE               65536 /* 64 Kilobytes*/
#define EXPECTED_BYTE_COUNT       (2ULL * BUFFER_SIZE)
#define BYTE_COUNT_MARGIN_PERCENT 30ULL

static void payload(void)
{
    uint32_t pe_index;
    uint32_t msc_index, rsrc_index;
    uint32_t rsrc_group_count;
    uint32_t rsrc_group_index;
    uint32_t group_member_count;
    uint32_t group_member_index;
    uint32_t group_msc_index;
    uint32_t group_rsrc_index;
    uint32_t configured_count;
    uint64_t mpam2_el2;
    uint64_t byte_count;
    uint64_t total_byte_count;
    uint64_t byte_count_min;
    uint64_t byte_count_margin;
    uint64_t addr_base, addr_len;
    uint64_t nrdy_timeout;
    uint64_t msc_nrdy_timeout;
    uint32_t test_fails = 0;
    uint32_t test_skip = 1;
    bool group_valid;
    bool mpam2_programmed = false;
    void *mem_buf  = NULL;
    void *src_buf  = NULL;
    void *dest_buf = NULL;

    pe_index = val_pe_get_index_mpid(val_pe_get_mpid());


   /* Check if PE implements FEAT_MPAM */
    if (!((VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 40, 43) > 0) ||
        (VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR1_EL1), 16, 19) > 0))) {
            val_set_status(pe_index, RESULT_FAIL(01));
            return;
    }

    /* resource groups are created when the MPAM platform table is enumerated */
    rsrc_group_count = val_mpam_get_rsrc_group_count();
    val_print(DEBUG, "\n       Resource group count = %d", rsrc_group_count);

    if (!rsrc_group_count) {
        val_set_status(pe_index, RESULT_FAIL(02));
        return;
    }

    /* read MPAM2_EL2 and store the value for restoring later */
    mpam2_el2 = val_mpam_reg_read(MPAM2_EL2);

    /* visit each resource location group and check only memory resources */
    for (rsrc_group_index = 0; rsrc_group_index < rsrc_group_count; rsrc_group_index++) {
        if (val_mpam_get_rsrc_group_type(rsrc_group_index) != MPAM_RSRC_TYPE_MEMORY)
            continue;

        test_skip = 0;
        group_member_count = val_mpam_get_rsrc_group_member_count(rsrc_group_index);

        /* Each group must have atleast one member at index 0 */
        if (!group_member_count ||
            !val_mpam_get_rsrc_group_member(rsrc_group_index, 0, &msc_index, &rsrc_index)) {
            val_print(ERROR, "\n       Failed to identify resource group members", 0);
            test_fails++;
            continue;
        }

        nrdy_timeout = 0;
        configured_count = 0;
        group_valid = true;

        /* check the MBWU monitor capabilities of every MSC in the group */
        for (group_member_index = 0; group_member_index < group_member_count; group_member_index++)
        {
            /* Get the indices of each msc and resource in the group */
            if (!val_mpam_get_rsrc_group_member(rsrc_group_index, group_member_index,
                                                &group_msc_index, &group_rsrc_index)) {
                val_print(ERROR, "\n       Failed to identify resource group member %d",
                          group_member_index);
                test_fails++;
                goto group_cleanup;
            }

            if (val_mpam_msc_supports_ris(group_msc_index))
                val_mpam_memory_configure_ris_sel(group_msc_index, group_rsrc_index);

            if (!val_mpam_msc_supports_mbwumon(group_msc_index) ||
                !val_mpam_get_mbwumon_count(group_msc_index)) {
                val_print(ERROR, "\n       MBWU MON unsupported by MSC %d", group_msc_index);
                group_valid = false;
            }

            msc_nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, group_msc_index, 0);
            if (msc_nrdy_timeout > nrdy_timeout)
                nrdy_timeout = msc_nrdy_timeout;
        }

        if (!group_valid) {
            test_fails++;
            continue;
        }

        addr_base = val_mpam_memory_get_base(msc_index, rsrc_index);
        addr_len = val_mpam_memory_get_size(msc_index, rsrc_index);
        if ((addr_base == SRAT_INVALID_INFO) || (addr_len == SRAT_INVALID_INFO) ||
            (addr_len < 2 * BUFFER_SIZE)) {
            val_print(ERROR, "\n       No usable SRAT mem range info found", 0);
            test_fails++;
            continue;
        }

        /* allocate one memory block and divide it into source and destination buffers */
        mem_buf = val_mem_alloc_at_address(addr_base, 2 * BUFFER_SIZE);
        if (mem_buf == NULL) {
            val_print(ERROR, "\n       Memory allocation of buffers failed", 0);
            test_fails++;
            continue;
        }

        src_buf = mem_buf;
        dest_buf = (void *)((uint8_t *)mem_buf + BUFFER_SIZE);

        /* Remove the buffer cache lines so the monitored copy accesses memory. */
        val_pe_cache_clean_invalidate_range((uint64_t)src_buf, BUFFER_SIZE);
        val_pe_cache_clean_invalidate_range((uint64_t)dest_buf, BUFFER_SIZE);
        val_mem_issue_dsb();

        /* configure the MBWU monitor in every MSC which manages this memory resource */
        configured_count = 0;
        for (group_member_index = 0; group_member_index < group_member_count; group_member_index++)
        {
            if (!val_mpam_get_rsrc_group_member(rsrc_group_index, group_member_index,
                                                &group_msc_index, &group_rsrc_index)) {
                val_print(ERROR, "\n       Failed to identify resource group member %d",
                          group_member_index);
                test_fails++;
                goto group_cleanup;
            }

            if (val_mpam_msc_supports_ris(group_msc_index))
                val_mpam_memory_configure_ris_sel(group_msc_index, group_rsrc_index);

            val_mpam_memory_configure_mbwumon(group_msc_index);
            configured_count++;
        }

        /* program the PE to generate traffic with the PARTID selected by the monitors */
        if (val_mpam_program_el2(DEFAULT_PARTID, DEFAULT_PMG)) {
            val_print(ERROR, "\n       MPAM2_EL2 programming failed", 0);
            test_fails++;
            goto group_cleanup;
        }
        mpam2_programmed = true;

        /* enable all group monitors before generating the memory traffic */
        for (group_member_index = 0; group_member_index < group_member_count; group_member_index++)
        {
            if (!val_mpam_get_rsrc_group_member(rsrc_group_index, group_member_index,
                                                &group_msc_index, &group_rsrc_index)) {
                val_print(ERROR, "\n       Failed to identify resource group member %d",
                          group_member_index);
                test_fails++;
                goto group_cleanup;
            }

            if (val_mpam_msc_supports_ris(group_msc_index))
                val_mpam_memory_configure_ris_sel(group_msc_index, group_rsrc_index);
            val_mpam_memory_mbwumon_enable(group_msc_index);
        }

        /* wait for the maximum NRDY time reported by the resource group */
        if (nrdy_timeout)
            val_time_delay_ms(nrdy_timeout);

        /* perform the memory operation and push destination writes to the memory resource */
        val_memcpy(dest_buf, src_buf, BUFFER_SIZE);
        val_pe_cache_clean_range((uint64_t)dest_buf, BUFFER_SIZE);
        val_mem_issue_dsb();

        /* restore MPAM2_EL2 to avoid counting monitor-management traffic */
        val_mpam_reg_write(MPAM2_EL2, mpam2_el2);
        mpam2_programmed = false;

        /* disable all group monitors before reading their count values */
        for (group_member_index = 0;
             group_member_index < group_member_count;
             group_member_index++) {
            if (!val_mpam_get_rsrc_group_member(rsrc_group_index, group_member_index,
                                                &group_msc_index, &group_rsrc_index)) {
                val_print(ERROR, "\n       Failed to identify resource group member %d",
                          group_member_index);
                test_fails++;
                goto group_cleanup;
            }

            if (val_mpam_msc_supports_ris(group_msc_index))
                val_mpam_memory_configure_ris_sel(group_msc_index, group_rsrc_index);
            val_mpam_memory_mbwumon_disable(group_msc_index);
        }

        /* Disabling a monitor can assert NRDY again. Wait for every group member to settle. */
        if (nrdy_timeout)
            val_time_delay_ms(nrdy_timeout);

        /* add each group monitor value to get the total usage of the memory resource */
        total_byte_count = 0;
        for (group_member_index = 0; group_member_index < group_member_count; group_member_index++)
        {
            if (!val_mpam_get_rsrc_group_member(rsrc_group_index, group_member_index,
                                                &group_msc_index, &group_rsrc_index)) {
                val_print(ERROR, "\n       Failed to identify resource group member %d",
                          group_member_index);
                test_fails++;
                goto group_cleanup;
            }

            if (val_mpam_msc_supports_ris(group_msc_index))
                val_mpam_memory_configure_ris_sel(group_msc_index, group_rsrc_index);

            byte_count = val_mpam_memory_mbwumon_read_count(group_msc_index);
            if (byte_count == (uint64_t)MPAM_MON_NOT_READY) {
                val_print(ERROR, "\n       MBWU MON not ready for MSC %d", group_msc_index);
                group_valid = false;
            } else {
                val_print(DEBUG, "\n       MSC %d byte_count = 0x%llx bytes",
                          group_msc_index, byte_count);
                total_byte_count += byte_count;
            }

            val_mpam_memory_mbwumon_reset(group_msc_index);
        }
        configured_count = 0;

        if (!group_valid) {
            test_fails++;
            goto group_cleanup;
        }

        val_print(DEBUG, "\n       Group byte_count = 0x%llx bytes", total_byte_count);

        /* Allow implementation variation below the expected read and write traffic. */
        byte_count_margin = (EXPECTED_BYTE_COUNT * BYTE_COUNT_MARGIN_PERCENT) / 100;
        byte_count_min = EXPECTED_BYTE_COUNT - byte_count_margin;
        if (!(total_byte_count > byte_count_min)) {
            val_print(ERROR,
                      "\n       Aggregate monitor count incorrect for SRAT domain 0x%llx",
                      val_mpam_get_info(MPAM_MSC_RSRC_DESC1, msc_index, rsrc_index));
            test_fails++;
        }

group_cleanup:
        /* restore PE settings and reset all monitors configured for this group */
        if (mpam2_programmed) {
            val_mpam_reg_write(MPAM2_EL2, mpam2_el2);
            mpam2_programmed = false;
        }

        for (group_member_index = 0;
             group_member_index < configured_count;
             group_member_index++) {
            if (!val_mpam_get_rsrc_group_member(rsrc_group_index, group_member_index,
                                                &group_msc_index, &group_rsrc_index)) {
                val_print(ERROR, "\n       Failed to clean resource group member %d",
                          group_member_index);
                break;
            }

            if (val_mpam_msc_supports_ris(group_msc_index))
                val_mpam_memory_configure_ris_sel(group_msc_index, group_rsrc_index);
            val_mpam_memory_mbwumon_disable(group_msc_index);
            val_mpam_memory_mbwumon_reset(group_msc_index);
        }

        if (mem_buf != NULL) {
            val_mem_free_at_address((uint64_t)mem_buf, 2 * BUFFER_SIZE);
            mem_buf = NULL;
            src_buf = NULL;
            dest_buf = NULL;
        }
    }

    if (test_fails)
        val_set_status(pe_index, RESULT_FAIL(05));
    else if (test_skip)
        val_set_status(pe_index, RESULT_SKIP(01));
    else
        val_set_status(pe_index, RESULT_PASS);
}

uint32_t mpam003_entry(uint32_t num_pe)
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
