/** @file
 * Copyright (c) 2021, 2023-2026, Arm Limited or its affiliates. All rights reserved.
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
#include "acs_timer.h"
#include "acs_pe.h"

#define TEST_NUM   (ACS_TIMER_TEST_NUM_BASE + 5)
#define TEST_RULE  "B_TIME_09"
#define TEST_DESC  "Restore PE timer on PE wake up        "

static uint32_t intid;
static uint64_t cnt_base_n;
static int irq_received;
static uint32_t intid_phy;

static
void
isr_sys_timer()
{
  val_timer_disable_system_timer((addr_t)cnt_base_n);
  val_gic_end_of_interrupt(intid);
  irq_received = 1;
  val_print(TRACE, "\n       System timer interrupt received");
}

static
void
isr_phy_el1()
{
  val_timer_set_phy_el1(0);
  val_print(TRACE, "\n       Received el1_phy interrupt   ");
  val_gic_end_of_interrupt(intid_phy);
}

static
void
payload()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t sys_timer_ticks;
  uint32_t pe_timer_ticks;
  uint32_t ns_timer = 0;
  uint64_t timer_num, timer_cnt;
  int32_t status;

  uint64_t ticks;
  uint32_t pe_timeout_us;
  /* System timer */
  if (acs_policy_get_timer_timeout_us() == 0) {
      val_print(ERROR, "\n       Timer timeout is zero; configure a valid timeout");
      val_set_status(index, RESULT_FAIL(3));
      return;
  }
  ticks = CEIL_TO_MAX_SYS_TIMEOUT(val_get_timeout_to_ticks(acs_policy_get_timer_timeout_us()));
  sys_timer_ticks = (uint32_t)ticks;

  /* PE timer */
  pe_timeout_us = acs_policy_get_timer_timeout_us() * 2;

  ticks = CEIL_TO_MAX_SYS_TIMEOUT(val_get_timeout_to_ticks(pe_timeout_us));
  pe_timer_ticks = (uint32_t)ticks;

  timer_num = val_timer_get_info(TIMER_INFO_NUM_PLATFORM_TIMERS, 0);

  while (timer_num--) {
      if (val_timer_get_info(TIMER_INFO_IS_PLATFORM_TIMER_SECURE, timer_num))
        continue;
      else{
        ns_timer++;
        break;
      }
  }

  if (!ns_timer) {
      val_print(DEBUG, "\n       No non-secure systimer implemented");
      val_set_status(index, RESULT_SKIP(1));
      return;
  }

  /* Start Sys timer*/
  cnt_base_n = val_timer_get_info(TIMER_INFO_SYS_CNT_BASE_N, timer_num);
  if (cnt_base_n == 0) {
      val_print(WARN, "\n       CNT_BASE_N is zero                 ");
      val_set_status(index, RESULT_SKIP(2));
      return;
  }

  val_timer_set_system_timer((addr_t)cnt_base_n, sys_timer_ticks);

  irq_received = 0;

  intid = val_timer_get_info(TIMER_INFO_SYS_INTID, timer_num);
  val_gic_install_isr(intid, isr_sys_timer);

  intid_phy = val_timer_get_info(TIMER_INFO_PHY_EL1_INTID, 0);
  val_gic_install_isr(intid_phy, isr_phy_el1);

  /* Start EL1 PHY timer */
  val_timer_set_phy_el1(pe_timer_ticks);

  /* Put current PE in to low power mode*/
  status = val_suspend_pe(0, 0);
  if (status) {
      val_print(DEBUG, "\n       Not able to suspend the PE : %d", status);
      val_timer_disable_system_timer((addr_t)cnt_base_n);
      val_gic_clear_interrupt(intid);
      val_timer_set_phy_el1(0);
      val_set_status(index, RESULT_SKIP(3));
      return;
  }

  if (irq_received == 0) {
      val_print(ERROR, "\n       System timer interrupt not generated");
      val_timer_disable_system_timer((addr_t)cnt_base_n);
      val_gic_clear_interrupt(intid);
      val_timer_set_phy_el1(0);
      val_set_status(index, RESULT_FAIL(1));
      return;
  }

  /* PE wake up from sys timer interrupt & start execution here */
  /* Read PE timer*/
  timer_cnt = val_get_phy_el1_timer_count();

  /*Disable PE timer*/
  val_timer_set_phy_el1(0);

  val_print(TRACE, "\n       Read back PE timer count :%d", timer_cnt);

  /* Check whether count is moved or not*/
  if ((timer_cnt < ((pe_timer_ticks - sys_timer_ticks) + (sys_timer_ticks/100)))
                                                      && (timer_cnt != 0))
    val_set_status(index, RESULT_PASS);
  else
    val_set_status(index, RESULT_FAIL(2));
}

uint32_t
t005_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This Timer test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);
  return status;

}
