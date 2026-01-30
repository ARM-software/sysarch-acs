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
 * 1. Save MPAM2_EL2 state, program default PARTID/PMG, and determine MBWU monitor support.
 * 2. For every MSC that exposes MBWU monitors, iterate memory resources with valid ranges.
 * 3. Allocate source/destination buffers, drive the MBWU counter near overflow, then
      trigger traffic.
 * 4. After overflow, clear status via control register, verify counter readiness, and
      confirm resume.
 * 5. Repeat overflow, clear by writing the counter, and ensure the counter advances after
      resumed traffic.
 * 6. Reset monitors, free buffers, restore MPAM2_EL2 state, and set the overall test status.
 */

#include "val/include/acs_val.h"
#include "val/include/acs_pe.h"
#include "val/include/acs_mpam.h"
#include "val/include/acs_mpam_reg.h"
#include "val/include/acs_memory.h"
#include "val/include/val_interface.h"

#define TEST_NUM   ACS_MPAM_MONITOR_TEST_NUM_BASE + 8
#define TEST_RULE  ""
#define TEST_DESC  "Check MBWU MSMON OFLOW Reset behavior "

#define TEST_BUF_SIZE         SIZE_1M
#define MBWU_PREFILL_DELTA    0x1000

/* Busy-wait until the monitor's NRDY window elapses */
static void
mbwu_wait_for_update(uint32_t msc_index)
{
  uint64_t nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);

  while (nrdy_timeout)
    --nrdy_timeout;
}

/* Program the MBWU counter with the provided value */
static void
mbwu_write_counter(uint32_t msc_index, uint64_t value)
{
  uint64_t data64;
  uint32_t data32;

  if (val_mpam_mbwu_supports_long(msc_index)) {
    data64 = ((uint64_t)0 << MBWU_NRDY_SHIFT) | value;
    val_mpam_mmr_write64(msc_index, REG_MSMON_MBWU_L, data64);
  } else {
    data32 = (0 << MBWU_NRDY_SHIFT) | (uint32_t)value;
    val_mpam_mmr_write(msc_index, REG_MSMON_MBWU, data32);
  }
}

/* Derive the counter prefill that positions the next overflow */
static uint64_t
mbwu_get_prefill_value(uint32_t msc_index)
{
  uint64_t max_count;

  if (val_mpam_mbwu_supports_long(msc_index)) {
    if (val_mpam_mbwu_supports_lwd(msc_index))
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
mbwu_is_overflow_set(uint32_t msc_index)
{
  uint32_t ctl;

  ctl = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_MBWU_CTL);
  return ((ctl & ((uint32_t)1 << MBWU_CTL_OFLOW_STATUS_BIT_SHIFT)) |
          (ctl & ((uint32_t)1 << MBWU_CTL_OFLOW_STATUS_L_SHIFT)));
}

/* Clear overflow status and return Status Value */
static uint32_t
mbwu_clear_overflow_status(uint32_t msc_index)
{
  uint32_t ctl;

  ctl = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_MBWU_CTL);
  ctl &= ~(((uint32_t)1 << MBWU_CTL_OFLOW_STATUS_BIT_SHIFT) |
           ((uint32_t)1 << MBWU_CTL_OFLOW_STATUS_L_SHIFT));
  val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MBWU_CTL, ctl);

  ctl = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_MBWU_CTL);

  return ((ctl & ((uint32_t)1 << MBWU_CTL_OFLOW_STATUS_BIT_SHIFT)) |
          (ctl & ((uint32_t)1 << MBWU_CTL_OFLOW_STATUS_L_SHIFT)));
}

static uint32_t
mbwu_prepare_overflow(uint32_t msc_index, void *src_buf, void *dest_buf, uint64_t buf_size)
{
  uint32_t ctl;

  val_mpam_memory_configure_mbwumon(msc_index);

  /* Set overflow behaviour bits before enabling the monitor */
  ctl = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_MBWU_CTL);
  ctl = BITFIELD_WRITE(ctl, MBWU_CTL_OFLOW_INTR, 0);
  ctl = BITFIELD_WRITE(ctl, MBWU_CTL_OFLOW_INTR_L, 0);
  ctl = BITFIELD_WRITE(ctl, MBWU_CTL_OFLOW_STATUS, 0);
  ctl = BITFIELD_WRITE(ctl, MBWU_CTL_OFLOW_STATUS_L, 0);
  val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MBWU_CTL, ctl);

  /* Update monitor to near max value */
  mbwu_write_counter(msc_index, mbwu_get_prefill_value(msc_index));
  mbwu_wait_for_update(msc_index);

  /* Enable Monitor */
  ctl = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_MBWU_CTL);
  ctl |= (1 << MBWU_CTL_ENABLE_SHIFT);
  val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MBWU_CTL, ctl);

  val_memcpy(src_buf, dest_buf, buf_size);
  mbwu_wait_for_update(msc_index);

  if (!mbwu_is_overflow_set(msc_index)) {
    val_print(ACS_PRINT_ERR,
        "\n       Overflow status not set during setup for MSC %d", msc_index);
    goto exit_fail;
  }

  return ACS_STATUS_PASS;

exit_fail:
  val_mpam_memory_mbwumon_disable(msc_index);
  val_mpam_memory_mbwumon_reset(msc_index);
  return ACS_STATUS_FAIL;
}

static void
payload(void)
{
  uint32_t pe_index    = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t total_nodes = val_mpam_get_msc_count();
  uint32_t msc_index;
  uint32_t rsrc_index;
  uint32_t rsrc_count;
  uint32_t test_fail = 0;
  bool test_skip = 1;
  uint64_t mpam2_el2_saved;
  uint64_t status;
  uint64_t buf_size = TEST_BUF_SIZE;
  uint64_t mem_base = 0;
  uint64_t mem_size = 0;
  void *src_buf = NULL;
  void *dest_buf = NULL;
  uint32_t mon_count;
  uint64_t post_clear_count;
  uint64_t resumed_count;

  mpam2_el2_saved = val_mpam_reg_read(MPAM2_EL2);

  /* Program MPAM2_EL2 with DEFAULT_PARTID and default PMG */
  status = val_mpam_program_el2(DEFAULT_PARTID, DEFAULT_PMG);
  if (status) {
    val_print(ACS_PRINT_ERR, "\n       MPAM2_EL2 programming failed", 0);
    test_skip = 0;
    test_fail++;
    goto cleanup;
  }

  for (msc_index = 0; msc_index < total_nodes; msc_index++) {

    mon_count = 0;
    if (val_mpam_msc_supports_mbwumon(msc_index))
      mon_count = val_mpam_get_mbwumon_count(msc_index);

    if (mon_count == 0)
      continue;

    rsrc_count = val_mpam_get_info(MPAM_MSC_RSRC_COUNT, msc_index, 0);
    for (rsrc_index = 0; rsrc_index < rsrc_count; rsrc_index++) {

      if (val_mpam_get_info(MPAM_MSC_RSRC_TYPE, msc_index, rsrc_index)
          != MPAM_RSRC_TYPE_MEMORY)
        continue;

      /* Select resource instance if RIS feature implemented */
      if (val_mpam_msc_supports_ris(msc_index))
        val_mpam_memory_configure_ris_sel(msc_index, rsrc_index);

      mem_base = val_mpam_memory_get_base(msc_index, rsrc_index);
      mem_size = val_mpam_memory_get_size(msc_index, rsrc_index);

      if ((mem_base == SRAT_INVALID_INFO) || (mem_size == SRAT_INVALID_INFO) ||
          (mem_size <= 2 * buf_size)) {
        val_print(ACS_PRINT_DEBUG,
            "\n       MSC %d memory range invalid for MBWU test", msc_index);
        continue;
      }

      test_skip = 0;

      src_buf = (void *)val_mem_alloc_at_address(mem_base, buf_size);
      dest_buf = (void *)val_mem_alloc_at_address(mem_base + buf_size, buf_size);
      if ((src_buf == NULL) || (dest_buf == NULL)) {
        val_print(ACS_PRINT_ERR, "\n       Mem allocation failed for MSC %d", msc_index);
        test_fail++;
        goto cleanup;
      }

      /* Drive the monitor into overflow so the disable paths can be validated */
      if (mbwu_prepare_overflow(msc_index, src_buf, dest_buf, buf_size)) {
        test_fail++;
        goto monitor_cleanup;
      }

      /* Clear overflow via control register and ensure counter resumes */
      if (mbwu_clear_overflow_status(msc_index)) {
        val_print(ACS_PRINT_ERR,
            "\n       Overflow status not cleared via control write for MSC %d", msc_index);
        test_fail++;
        goto monitor_cleanup;
      }

      mbwu_wait_for_update(msc_index);
      post_clear_count = val_mpam_memory_mbwumon_read_count(msc_index);
      /* Check if Monitor Read Failed */
      if (post_clear_count == (uint64_t)MPAM_MON_NOT_READY) {
        val_print(ACS_PRINT_ERR,
            "\n       Monitor not ready after clearing via control write for MSC %d",
            msc_index);
        test_fail++;
        goto monitor_cleanup;
      }

      /* Issue traffic after clearing via control to exercise the resume path */
      val_memcpy(src_buf, dest_buf, buf_size);
      mbwu_wait_for_update(msc_index);

      resumed_count = val_mpam_memory_mbwumon_read_count(msc_index);
      /* Check if Monitor Read Failed */
      if (resumed_count == (uint64_t)MPAM_MON_NOT_READY) {
        val_print(ACS_PRINT_ERR,
            "\n       Monitor not ready after resume traffic (control clear) for MSC %d",
            msc_index);
        test_fail++;
        goto monitor_cleanup;
      } else if (resumed_count <= post_clear_count) {
        val_print(ACS_PRINT_ERR,
            "\n       Counter did not advance after control clear for MSC %d", msc_index);
        test_fail++;
        goto monitor_cleanup;
      }

      /* Disable and reset MBWU monitor */
      val_mpam_memory_mbwumon_disable(msc_index);
      val_mpam_memory_mbwumon_reset(msc_index);

      /* Recreate overflow to validate resume path via counter write */
      if (mbwu_prepare_overflow(msc_index, src_buf, dest_buf, buf_size)) {
        test_fail++;
        goto monitor_cleanup;
      }

      mbwu_write_counter(msc_index, 0);
      mbwu_wait_for_update(msc_index);

      if (mbwu_is_overflow_set(msc_index)) {
        val_print(ACS_PRINT_ERR,
            "\n       Overflow status not cleared by counter write for MSC %d", msc_index);
        test_fail++;
        goto monitor_cleanup;
      }

      post_clear_count = val_mpam_memory_mbwumon_read_count(msc_index);
      /* Check if Monitor Read Failed */
      if (post_clear_count == (uint64_t)MPAM_MON_NOT_READY) {
        val_print(ACS_PRINT_ERR,
            "\n       Monitor not ready after counter write for MSC %d", msc_index);
        test_fail++;
        goto monitor_cleanup;
      }

      /* Repeat the traffic sequence after clearing via counter write */
      val_memcpy(src_buf, dest_buf, buf_size);
      mbwu_wait_for_update(msc_index);

      resumed_count = val_mpam_memory_mbwumon_read_count(msc_index);
      /* Check if Monitor Read Failed */
      if (resumed_count == (uint64_t)MPAM_MON_NOT_READY) {
        val_print(ACS_PRINT_ERR,
            "\n       Monitor not ready after resume traffic (counter write) for MSC %d",
            msc_index);
        test_fail++;
      } else if (resumed_count <= post_clear_count) {
        val_print(ACS_PRINT_ERR,
            "\n       Counter did not advance after counter write clear for MSC %d",
            msc_index);
        test_fail++;
      }

      val_mpam_memory_mbwumon_disable(msc_index);
      val_mpam_memory_mbwumon_reset(msc_index);

monitor_cleanup:
      val_mem_free_at_address((uint64_t)src_buf, buf_size);
      val_mem_free_at_address((uint64_t)dest_buf, buf_size);
      src_buf = NULL;
      dest_buf = NULL;
    }
  }

cleanup:
  /* Restore MPAM2_EL2 settings */
  val_mpam_reg_write(MPAM2_EL2, mpam2_el2_saved);

  if (src_buf != NULL)
    val_mem_free_at_address((uint64_t)src_buf, buf_size);
  if (dest_buf != NULL)
    val_mem_free_at_address((uint64_t)dest_buf, buf_size);

  if (test_skip)
    val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 1));
  else if (test_fail)
    val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 1));
  else
    val_set_status(pe_index, RESULT_PASS(TEST_NUM, 1));

  return;
}

uint32_t monitor008_entry(void)
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
