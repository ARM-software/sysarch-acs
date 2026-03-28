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
#include "acs_gic.h"
#include "val_interface.h"

#define TEST_NUM   ACS_MPAM_ERROR_TEST_NUM_BASE  +  19
#define TEST_DESC  "Check MBWU Oflow MSI interrupt status "
#define TEST_RULE  ""

#define TEST_BUF_SIZE         SIZE_1M

static uint32_t msc_index;
static uint32_t msi_intr_num = 0x204C;
static uint64_t mpam2_el2_saved;
static volatile uint32_t status_asserted;
static volatile uint32_t handler_invoked;

static void
mbwu_disable_oflow_msi(uint32_t msc_idx)
{
    val_mpam_mmr_write(msc_idx, REG_MSMON_OFLOW_MSI_ATTR, 0);
}

/* Configure interrupt behaviour and drive MBWU into overflow */
static uint32_t
mbwu_prepare_overflow_intr(uint32_t msc_idx,
                           void *src_buf,
                           void *dest_buf,
                           uint64_t buf_size)
{
    uint32_t ctl;

    val_mpam_memory_configure_mbwumon(msc_idx);

    if (val_mpam_mbwu_clear_overflow_status(msc_idx)) {
        val_print(ACS_PRINT_ERR,
            "\n       Failed to clear overflow status before setup for MSC %d", msc_idx);
        return ACS_STATUS_FAIL;
    }

    ctl = val_mpam_mmr_read(msc_idx, REG_MSMON_CFG_MBWU_CTL);
    ctl = BITFIELD_WRITE(ctl, MBWU_CTL_OFLOW_INTR, 1);
    ctl = BITFIELD_WRITE(ctl, MBWU_CTL_OFLOW_INTR_L, 0);
    ctl = BITFIELD_WRITE(ctl, MBWU_CTL_OFLOW_STATUS, 0);
    ctl = BITFIELD_WRITE(ctl, MBWU_CTL_OFLOW_STATUS_L, 0);
    ctl |= (1 << MBWU_CTL_ENABLE_MATCH_PARTID_SHIFT);
    ctl |= (1 << MBWU_CTL_ENABLE_MATCH_PMG_SHIFT);
    val_mpam_mmr_write(msc_idx, REG_MSMON_CFG_MBWU_CTL, ctl);

    val_mpam_mbwu_write_counter(msc_idx,
        val_mpam_mbwu_get_prefill_value(msc_idx, MBWU_SHORT_CHECK),
        MBWU_SHORT_CHECK);
    val_mpam_mbwu_wait_for_update(msc_idx);

    ctl = val_mpam_mmr_read(msc_idx, REG_MSMON_CFG_MBWU_CTL);
    ctl |= (1 << MBWU_CTL_ENABLE_SHIFT);
    val_mpam_mmr_write(msc_idx, REG_MSMON_CFG_MBWU_CTL, ctl);

    val_mpam_mbwu_wait_for_update(msc_idx);

    val_memcpy(src_buf, dest_buf, buf_size);
    /* Wait for some time before the memcpy settles and counters update */
    val_time_delay_ms(TIMEOUT_MEDIUM);
    val_mpam_mbwu_wait_for_update(msc_idx);

    /* Overflow is cleared in the handler */
    if (val_mpam_mbwu_is_overflow_set(msc_idx)) {
        val_print(ACS_PRINT_ERR,
            "\n       Overflow status not cleared for MSC %d during setup", msc_idx);
        return ACS_STATUS_FAIL;
    }

    return ACS_STATUS_PASS;
}

static
void intr_handler(void)
{
    uint32_t pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t ctl;

    handler_invoked = 1;

    ctl = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_MBWU_CTL);
    status_asserted = ctl & ((uint32_t)1 << MBWU_CTL_OFLOW_STATUS_BIT_SHIFT);

    if (!status_asserted) {
        val_print(ACS_PRINT_ERR,
            "\n       OFLOW_STATUS not set on overflow MSI for MSC %d", msc_index);
        val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 02));
    } else {
        val_print(ACS_PRINT_DEBUG,
            "\n       OFLOW_STATUS observed set on MSI for MSC %d", msc_index);
        val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));

        if (val_mpam_mbwu_clear_overflow_status(msc_index)) {
            val_print(ACS_PRINT_ERR,
                "\n       Failed to clear overflow status via helper for MSC %d", msc_index);
            val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 07));
        }
    }

    /* Stop further OFLOW MSI assertions once handled */
    mbwu_disable_oflow_msi(msc_index);
    val_mpam_memory_mbwumon_disable(msc_index);

    /* Clear MPAM error status and signal end of interrupt */
    val_mpam_msc_reset_errcode(msc_index);
    val_gic_end_of_interrupt(msi_intr_num);
}

static
void payload(void)
{
    uint32_t pe_index    = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t total_nodes = val_mpam_get_msc_count();
    uint32_t rsrc_index;
    uint32_t rsrc_count;
    uint32_t mon_count;
    uint32_t status;
    uint32_t handler_installed;
    uint32_t test_fail = 0;
    uint32_t test_skip = 1;
    uint32_t intr_enabled = 0;
    uint32_t msmon_idr;
    uint32_t device_id = 0;
    uint32_t its_id = 0;
    uint64_t timeout;
    uint64_t buf_size = TEST_BUF_SIZE;
    uint64_t base = 0;
    uint64_t mem_size = 0;
    void *src_buf = NULL;
    void *dest_buf = NULL;

    mpam2_el2_saved = val_mpam_reg_read(MPAM2_EL2);

    /* Program MPAM2_EL2 with DEFAULT_PARTID and default PMG */
    status = val_mpam_program_el2(DEFAULT_PARTID, DEFAULT_PMG);
    if (status) {
        val_print(ACS_PRINT_ERR, "\n       MPAM2_EL2 programming failed", 0);
        test_fail++;
        test_skip = 0;
        goto cleanup;
    }

    for (msc_index = 0; msc_index < total_nodes; msc_index++) {

        handler_installed = 0;
        device_id = 0;
        its_id = 0;

        mon_count = 0;
        if (val_mpam_msc_supports_mon(msc_index))
            mon_count = val_mpam_msc_supports_mbwumon(msc_index) ?
                        val_mpam_get_mbwumon_count(msc_index) : 0;

        if (mon_count == 0)
            continue;

        msmon_idr = val_mpam_mmr_read(msc_index, REG_MPAMF_MSMON_IDR);
        if (!BITFIELD_READ(MSMON_IDR_HAS_OFLW_MSI, msmon_idr)) {
            /* Overflow MSI Not Supported for this MSC */
            val_print(ACS_PRINT_ERR, "\n       OFLOW_MSI is not supported for MSC %d", msc_index);
            continue;
        }

        /* Get Device ID and ITS Id for this MSC */
        status = val_mpam_get_msc_device_info(msc_index, &device_id, &its_id);
        if (status) {
            val_print(ACS_PRINT_ERR,
                "\n       Could not get device & ITS info for MSC %d", msc_index);
            test_fail++;
            break;
        }

        rsrc_count = val_mpam_get_info(MPAM_MSC_RSRC_COUNT, msc_index, 0);
        for (rsrc_index = 0; rsrc_index < rsrc_count; rsrc_index++) {

            if (val_mpam_get_info(MPAM_MSC_RSRC_TYPE, msc_index, rsrc_index)
                != MPAM_RSRC_TYPE_MEMORY)
                continue;

            /* Select resource instance if RIS feature implemented */
            if (val_mpam_msc_supports_ris(msc_index))
                val_mpam_memory_configure_ris_sel(msc_index, rsrc_index);

            base = val_mpam_memory_get_base(msc_index, rsrc_index);
            mem_size = val_mpam_memory_get_size(msc_index, rsrc_index);

            if ((base == SRAT_INVALID_INFO) || (mem_size == SRAT_INVALID_INFO) ||
                (mem_size <= 2 * buf_size)) {
                val_print(ACS_PRINT_DEBUG,
                    "\n       MSC %d memory range invalid for MBWU MSI test", msc_index);
                continue;
            }

            test_skip = 0;

            status = val_mpam_msc_request_msi(msc_index, device_id, its_id,
                                              msi_intr_num, 1 /* oflow_msi */);
            if (status) {
                val_print(ACS_PRINT_ERR,
                    "\n       MSI Assignment failed for MSC %d", msc_index);
                test_fail++;
                goto monitor_cleanup;
            }

            if (!handler_installed) {
                status = val_gic_install_isr(msi_intr_num, intr_handler);
                if (status) {
                    val_print(ACS_PRINT_ERR,
                        "\n       Failed to install ISR for interrupt %d", msi_intr_num);
                    test_fail++;
                    goto monitor_cleanup;
                }

                val_gic_route_interrupt_to_pe(msi_intr_num, val_pe_get_mpid_index(pe_index));
                handler_installed = 1;
            }

            src_buf = (void *)val_mem_alloc_at_address(base, buf_size);
            dest_buf = (void *)val_mem_alloc_at_address(base + buf_size, buf_size);
            if ((src_buf == NULL) || (dest_buf == NULL)) {
                val_print(ACS_PRINT_ERR, "\n       Mem allocation failed for MSC %d", msc_index);
                test_fail++;
                goto monitor_cleanup;
            }

            status_asserted = 0;
            handler_invoked = 0;

            val_set_status(pe_index, RESULT_PENDING(TEST_NUM));

            status = val_mpam_msc_enable_msi(msc_index, 1);
            if (status) {
                val_print(ACS_PRINT_ERR,
                    "\n       Failed to enable OFLOW_MSI for MSC %d", msc_index);
                test_fail++;
                goto monitor_cleanup;
            }

            status = mbwu_prepare_overflow_intr(msc_index, src_buf, dest_buf, buf_size);
            if (status) {
                val_print(ACS_PRINT_ERR,
                    "\n       Failed to configure overflow MSI for MSC %d", msc_index);
                val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 06));
                test_fail++;
                goto monitor_cleanup;
            }

            timeout = TIMEOUT_LARGE;
            while ((--timeout > 0) && (IS_RESULT_PENDING(val_get_status(pe_index))));

            if (timeout == 0) {
                val_print(ACS_PRINT_ERR,
                    "\n       Overflow MSI not received for MSC %d", msc_index);
                test_fail++;
                goto monitor_cleanup;
            }

            if (!handler_invoked) {
                val_print(ACS_PRINT_ERR,
                    "\n       Handler not invoked for MSC %d after overflow", msc_index);
                test_fail++;
                goto monitor_cleanup;
            }

            if (!status_asserted) {
                val_print(ACS_PRINT_ERR,
                    "\n       OFLOW_STATUS not observed set for MSC %d", msc_index);
                test_fail++;
                goto monitor_cleanup;
            }

            intr_enabled = 1;

monitor_cleanup:
            if (src_buf != NULL) {
                val_mem_free_at_address((uint64_t)src_buf, buf_size);
                src_buf = NULL;
            }
            if (dest_buf != NULL) {
                val_mem_free_at_address((uint64_t)dest_buf, buf_size);
                dest_buf = NULL;
            }

            val_mpam_memory_mbwumon_disable(msc_index);
            val_mpam_memory_mbwumon_reset(msc_index);
            val_mpam_mbwu_clear_overflow_status(msc_index);
            mbwu_disable_oflow_msi(msc_index);

            status_asserted = 0;
            handler_invoked = 0;

            if (test_fail)
                break;
        }

        if (test_fail)
            break;
    }

cleanup:
    /* Restore MPAM2_EL2 settings */
    val_mpam_reg_write(MPAM2_EL2, mpam2_el2_saved);

    if (test_skip)
        val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 1));
    else if (test_fail || !intr_enabled)
        val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 1));
    else
        val_set_status(pe_index, RESULT_PASS(TEST_NUM, 1));
}

uint32_t intr005_entry(void)
{
    uint32_t status  = ACS_STATUS_FAIL;
    uint32_t num_pe  = 1;

    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}
