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

/*
 * Algorithm:
 * 1. Save MPAM2_EL2 state, program default PARTID/PMG, and discover MSCs with
 *    MBWU long-counter support and an overflow interrupt line.
 * 2. For each eligible MSC, iterate memory resources with valid address ranges.
 * 3. Install the overflow ISR, select the resource, and allocate source/dest buffers.
 * 4. Configure MBWU to assert OFLOW_STATUS_L and raise the long overflow interrupt.
 * 5. Drive traffic to overflow, wait for the interrupt, and validate the ISR observations.
 * 6. Reset monitors, free buffers, restore MPAM2_EL2 state, and set the final test status.
 */

#include "val/include/acs_val.h"
#include "val/include/acs_pe.h"
#include "val/include/acs_mpam.h"
#include "val/include/acs_mpam_reg.h"
#include "val/include/acs_memory.h"
#include "val/include/val_interface.h"

#define TEST_NUM   ACS_MPAM_ERROR_TEST_NUM_BASE  +  18
#define TEST_DESC  "Check MBWU_L overflow interrupt status "
#define TEST_RULE  ""

#define TEST_BUF_SIZE         SIZE_1M
#define MBWU_PREFILL_DELTA    0x1000

/* Shared ISR state for the active MSC/interrupt */
static uint32_t msc_index;
static uint32_t intr_num;
static uint64_t mpam2_el2_saved;
static volatile uint32_t status_l_asserted;
static volatile uint32_t handler_invoked;

/* Busy-wait until the monitor's NRDY window elapses */
static void
mbwu_wait_for_update(uint32_t msc_idx)
{
    uint64_t nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_idx, 0);

    while (nrdy_timeout)
        --nrdy_timeout;
}

/* Program the MBWU counter with the provided value */
static void
mbwu_write_counter(uint32_t msc_idx, uint64_t value)
{
    uint64_t data64;
    uint32_t data32;

    if (val_mpam_mbwu_supports_long(msc_idx)) {
        data64 = ((uint64_t)0 << MBWU_NRDY_SHIFT) | value;
        val_mpam_mmr_write64(msc_idx, REG_MSMON_MBWU_L, data64);
    } else {
        data32 = (0 << MBWU_NRDY_SHIFT) | (uint32_t)value;
        val_mpam_mmr_write(msc_idx, REG_MSMON_MBWU, data32);
    }
}

/* Derive the counter prefill that positions the next overflow */
static uint64_t
mbwu_get_prefill_value(uint32_t msc_idx)
{
    uint64_t max_count;

    if (val_mpam_mbwu_supports_long(msc_idx)) {
        if (val_mpam_mbwu_supports_lwd(msc_idx))
            max_count = MSMON_COUNT_63BIT;
        else
            max_count = MSMON_COUNT_44BIT;
    } else {
        max_count = MSMON_COUNT_31BIT;
    }

    return max_count - MBWU_PREFILL_DELTA;
}

/* Return true if the overflow status is currently latched */
static uint32_t
mbwu_is_overflow_set(uint32_t msc_idx)
{
    uint32_t ctl;

    ctl = val_mpam_mmr_read(msc_idx, REG_MSMON_CFG_MBWU_CTL);
    return ((ctl & ((uint32_t)1 << MBWU_CTL_OFLOW_STATUS_BIT_SHIFT)) |
            (ctl & ((uint32_t)1 << MBWU_CTL_OFLOW_STATUS_L_SHIFT)));
}

/* Clear overflow status and return Status Value */
static uint32_t
mbwu_clear_overflow_status(uint32_t msc_idx)
{
    uint32_t ctl;

    ctl = val_mpam_mmr_read(msc_idx, REG_MSMON_CFG_MBWU_CTL);
    ctl &= ~(((uint32_t)1 << MBWU_CTL_OFLOW_STATUS_BIT_SHIFT) |
             ((uint32_t)1 << MBWU_CTL_OFLOW_STATUS_L_SHIFT));
    val_mpam_mmr_write(msc_idx, REG_MSMON_CFG_MBWU_CTL, ctl);

    ctl = val_mpam_mmr_read(msc_idx, REG_MSMON_CFG_MBWU_CTL);

    return ((ctl & ((uint32_t)1 << MBWU_CTL_OFLOW_STATUS_BIT_SHIFT)) |
            (ctl & ((uint32_t)1 << MBWU_CTL_OFLOW_STATUS_L_SHIFT)));
}

/* Configure interrupt behaviour and drive MBWU into overflow */
static uint32_t
mbwu_prepare_overflow_intr(uint32_t msc_idx,
                           void *src_buf,
                           void *dest_buf,
                           uint64_t buf_size)
{
    uint32_t ctl;

    /* Program the MBWU monitor to raise the long overflow interrupt */
    val_mpam_memory_configure_mbwumon(msc_idx);

    if (mbwu_clear_overflow_status(msc_idx)) {
        val_print(ACS_PRINT_ERR,
            "\n       Failed to clear overflow status before setup for MSC %d", msc_idx);
        return ACS_STATUS_FAIL;
    }

    ctl = val_mpam_mmr_read(msc_idx, REG_MSMON_CFG_MBWU_CTL);
    ctl = BITFIELD_WRITE(ctl, MBWU_CTL_OFLOW_INTR, 0);
    ctl = BITFIELD_WRITE(ctl, MBWU_CTL_OFLOW_INTR_L, 1);
    ctl = BITFIELD_WRITE(ctl, MBWU_CTL_OFLOW_STATUS, 0);
    ctl = BITFIELD_WRITE(ctl, MBWU_CTL_OFLOW_STATUS_L, 0);
    ctl |= (1 << MBWU_CTL_ENABLE_MATCH_PARTID_SHIFT);
    ctl |= (1 << MBWU_CTL_ENABLE_MATCH_PMG_SHIFT);
    val_mpam_mmr_write(msc_idx, REG_MSMON_CFG_MBWU_CTL, ctl);

    /* Set interrupt enable bit in MPAMF_ECR */
    val_mpam_mmr_write(msc_idx, REG_MPAMF_ECR, (1 << ECR_ENABLE_INTEN_SHIFT));

    /* Prefill close to max so a small traffic burst overflows */
    mbwu_write_counter(msc_idx, mbwu_get_prefill_value(msc_idx));
    mbwu_wait_for_update(msc_idx);

    /* Enable the monitor after configuration is complete */
    ctl = val_mpam_mmr_read(msc_idx, REG_MSMON_CFG_MBWU_CTL);
    ctl |= (1 << MBWU_CTL_ENABLE_SHIFT);
    val_mpam_mmr_write(msc_idx, REG_MSMON_CFG_MBWU_CTL, ctl);

    mbwu_wait_for_update(msc_idx);

    /* Generate memory traffic to trigger the overflow */
    val_memcpy(src_buf, dest_buf, buf_size);
    mbwu_wait_for_update(msc_idx);

    /* Overflow Status is Cleared in Handler */
    if (mbwu_is_overflow_set(msc_idx)) {
        val_print(ACS_PRINT_ERR,
            "\n       Overflow status not Cleared for MSC %d in Handler", msc_idx);
        return ACS_STATUS_FAIL;
    }

    return ACS_STATUS_PASS;
}

static
void intr_handler(void)
{
    uint32_t pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t ctl;

    /* Record ISR arrival and capture the status latch */
    handler_invoked = 1;

    ctl = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_MBWU_CTL);
    status_l_asserted = ctl & ((uint32_t)1 << MBWU_CTL_OFLOW_STATUS_L_SHIFT);

    if (!status_l_asserted) {
        val_print(ACS_PRINT_ERR,
            "\n       OFLOW_STATUS_L not set on overflow interrupt for MSC %d", msc_index);
        val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 02));
    } else {
        val_print(ACS_PRINT_DEBUG,
            "\n       OFLOW_STATUS_L observed set on interrupt for MSC %d", msc_index);
        val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));

        if (mbwu_clear_overflow_status(msc_index)) {
            val_print(ACS_PRINT_ERR,
                "\n       Failed to clear overflow status via helper for MSC %d", msc_index);
            val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 03));
        }
    }

    /* Clear MPAM error status and signal end of interrupt */
    val_mpam_msc_reset_errcode(msc_index);
    val_gic_end_of_interrupt(intr_num);
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
        intr_num = val_mpam_get_info(MPAM_MSC_OF_INTR, msc_index, 0);

        /* Skip if no MBWU monitor, no overflow interrupt, or no long counter */
        mon_count = 0;
        if (val_mpam_msc_supports_mon(msc_index))
            mon_count = val_mpam_msc_supports_mbwumon(msc_index) ?
                        val_mpam_get_mbwumon_count(msc_index) : 0;

        if ((intr_num == 0) || (mon_count == 0) ||
            (!val_mpam_mbwu_supports_long(msc_index)))
            continue;

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

            /* Ensure the region can fit both source and destination buffers */
            if ((base == SRAT_INVALID_INFO) || (mem_size == SRAT_INVALID_INFO) ||
                (mem_size <= 2 * buf_size)) {
                val_print(ACS_PRINT_DEBUG,
                    "\n       MSC %d memory range invalid for MBWU_L test", msc_index);
                continue;
            }

            test_skip = 0;

            /* Install ISR once per MSC and route it to this PE */
            if (!handler_installed) {
                status = val_gic_install_isr(intr_num, intr_handler);
                if (status) {
                    val_print(ACS_PRINT_ERR,
                        "\n       Failed to install ISR for interrupt %d", intr_num);
                    test_fail++;
                    goto monitor_cleanup;
                }

                val_gic_route_interrupt_to_pe(intr_num, val_pe_get_mpid_index(pe_index));
                handler_installed = 1;
            }

            /* Allocate buffers within the target resource */
            src_buf = (void *)val_mem_alloc_at_address(base, buf_size);
            dest_buf = (void *)val_mem_alloc_at_address(base + buf_size, buf_size);
            if ((src_buf == NULL) || (dest_buf == NULL)) {
                val_print(ACS_PRINT_ERR, "\n       Mem allocation failed for MSC %d", msc_index);
                test_fail++;
                goto monitor_cleanup;
            }

            status_l_asserted = 0;
            handler_invoked = 0;

            val_set_status(pe_index, RESULT_PENDING(TEST_NUM));

            /* Configure overflow interrupt and drive the counter to overflow */
            status = mbwu_prepare_overflow_intr(msc_index, src_buf, dest_buf, buf_size);
            if (status) {
                val_print(ACS_PRINT_ERR,
                    "\n       Failed to configure overflow interrupt for MSC %d", msc_index);
                val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 01));
                test_fail++;
                goto monitor_cleanup;
            }

            timeout = TIMEOUT_LARGE;
            while ((--timeout > 0) && (IS_RESULT_PENDING(val_get_status(pe_index))));

            if (timeout == 0) {
                val_print(ACS_PRINT_ERR,
                    "\n       Overflow interrupt not received for MSC %d", msc_index);
                test_fail++;
                goto monitor_cleanup;
            }

            /* Verify the ISR ran and observed OFLOW_STATUS_L */
            if (!handler_invoked) {
                val_print(ACS_PRINT_ERR,
                    "\n       Handler not invoked for MSC %d after overflow", msc_index);
                test_fail++;
                goto monitor_cleanup;
            }

            if (!status_l_asserted) {
                val_print(ACS_PRINT_ERR,
                    "\n       OFLOW_STATUS_L not observed set for MSC %d", msc_index);
                test_fail++;
                goto monitor_cleanup;
            }

            intr_enabled = 1;

monitor_cleanup:
            /* Tear down monitor state and release buffers before next resource */
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
            mbwu_clear_overflow_status(msc_index);

            status_l_asserted = 0;
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
        val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 4));
    else
        val_set_status(pe_index, RESULT_PASS(TEST_NUM, 2));
}

uint32_t intr004_entry(void)
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
