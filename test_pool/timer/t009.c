/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
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
#include "val_interface.h"
#include "acs_pe.h"
#include "acs_timer.h"

#define TEST_NUM   (ACS_TIMER_TEST_NUM_BASE + 10)
#define TEST_RULE  "B_TIME_10"
#define TEST_DESC  "Check Non-secure timer frame access   "

#define ACCESS_CNTTIDR  1
#define ACCESS_CNTPCT   2

static void *branch_to_test;
static uint64_t fault_timer_index;
static uint32_t fault_access_type;

static
void
esr(uint64_t interrupt_type, void *context)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  val_pe_update_elr(context, (uint64_t)branch_to_test);

  val_print(ERROR, "\n       Received exception type %d", interrupt_type);
  if (fault_access_type == ACCESS_CNTTIDR)
      val_print(ERROR, " during CNTCTLBase.CNTTIDR read for timer %llx", fault_timer_index);
  else if (fault_access_type == ACCESS_CNTPCT)
      val_print(ERROR, " during CNTBaseN.CNTPCT read for timer %llx", fault_timer_index);
  val_set_status(index, RESULT_FAIL(1));
}

static
void
payload()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t status;
  uint32_t ns_timer = 0;
  uint32_t readable_timer = 0;
  uint32_t cnttidr;
  uint64_t cnt_ctl_base;
  uint64_t cnt_base_n;
  uint64_t cntpct;
  uint64_t timer_num = val_timer_get_info(TIMER_INFO_NUM_PLATFORM_TIMERS, 0);

  branch_to_test = &&exception_taken;
  status = val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
  status |= val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);
  if (status) {
      val_print(ERROR, "\n       Failed to install exception handlers");
      val_set_status(index, RESULT_FAIL(2));
      return;
  }

  if (!timer_num) {
      val_print(DEBUG, "\n       No System timers are defined");
      val_set_status(index, RESULT_SKIP(1));
      return;
  }

  while (timer_num) {
      --timer_num;

      /* Skip secure timers */
      if (val_timer_get_info(TIMER_INFO_IS_PLATFORM_TIMER_SECURE, timer_num))
          continue;

      ns_timer++;
      cnt_ctl_base = val_timer_get_info(TIMER_INFO_SYS_CNTL_BASE, timer_num);
      cnt_base_n   = val_timer_get_info(TIMER_INFO_SYS_CNT_BASE_N, timer_num);

      /*
       * Firmware should expose non-zero base addresses for usable Non-secure
       * timer frames. Keep scanning other frames so one bad descriptor does not
       * hide a later usable frame.
       */
      if (cnt_ctl_base == 0)
          val_print(WARN, "\n       CNTCTL BASE is zero for timer %llx", timer_num);

      if (cnt_base_n == 0)
          val_print(WARN, "\n       CNT_BASE_N is zero for timer %llx", timer_num);

      if ((cnt_ctl_base == 0) || (cnt_base_n == 0))
          continue;

      /*
       * The payload checks Non-secure memory access of CNTCTLBase and CNTBaseN,
       * not register behavior. Do read probes only: no writes, no read-only/RW
       * validation, and no CNTACR permission interpretation.
       */
      fault_timer_index = timer_num;
      fault_access_type = ACCESS_CNTTIDR;
      cnttidr = val_mmio_read(cnt_ctl_base + CNTTIDR);
      val_print(DEBUG, "\n       CNTTIDR Read value = 0x%x", cnttidr);

      /*
       * CNTPCT may legally read as RAZ/WI if CNTACR<n>.RPCT is clear. That is
       * acceptable here because CNTACR/CNTNSAR policy is Secure-state owned;
       * this test only requires that the Non-secure read access itself does
       * not fault.
       */
      fault_access_type = ACCESS_CNTPCT;
      cntpct = val_mmio_read64(cnt_base_n + CNTPCT_LOWER);
      val_print(DEBUG, "\n       CNTPCT Read value = 0x%llx", cntpct);

      readable_timer++;
  }

  /* Report fail if OS visible platform timers exists, but non of them are non-secure access */
  if (!ns_timer) {
      val_print(ERROR, "\n       No non-secure systimer implemented");
      val_set_status(index, RESULT_FAIL(3));
      return;
  }

  if (!readable_timer) {
      val_print(ERROR, "\n       No usable non-secure systimer frame found");
      val_set_status(index, RESULT_FAIL(4));
      return;
  }

  val_set_status(index, RESULT_PASS);

exception_taken:
  return;
}

uint32_t
t009_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
