/** @file
 * Copyright (c) 2016-2019, 2021-2026, Arm Limited or its affiliates. All rights reserved.
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
#include "val_interface.h"
#include "acs_wakeup.h"

#define TEST_NUM  (ACS_WAKEUP_TEST_NUM_BASE + 4)
#define TEST_RULE "B_WAK_03, B_WAK_07"
#define TEST_DESC "Wake from Watchdog WS0 Int            "

static uint64_t wd_num;
static uint32_t g_wd_int_received;
static uint32_t g_failsafe_int_received;
static
void
isr_failsafe()
{
  uint32_t intid;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  val_timer_set_phy_el1(0);
  val_print(ERROR, "       Received Failsafe interrupt\n");
  g_failsafe_int_received = 1;
  /* On some system the failsafe is rcvd just after test interrupt and resulting
     in incorrect fail, to avoid this ensure set test as fail only when failsafe
     is hit and test interrupt is not rcvd
  */
  if (g_wd_int_received == 0) {
      val_set_status(index, RESULT_FAIL(1));
  }

  intid = val_timer_get_info(TIMER_INFO_PHY_EL1_INTID, 0);
  val_gic_end_of_interrupt(intid);
}

static
void
isr4()
{
  uint32_t intid;

  val_wd_set_ws0(wd_num, 0);
  val_print(TRACE, "       Received WS0 interrupt\n");
  g_wd_int_received = 1;
  intid = val_wd_get_info(wd_num, WD_INFO_GSIV);
  val_gic_end_of_interrupt(intid);
  val_timer_set_phy_el1(0);
  val_print(DEBUG, "       Clear Failsafe interrupt\n");
}

static
void
wakeup_set_failsafe()
{
  uint32_t intid;
  uint64_t timer_expire_val =
      CEIL_TO_MAX_SYS_TIMEOUT(val_get_timeout_to_ticks(acs_policy_get_timeout_fail()));

  intid = val_timer_get_info(TIMER_INFO_PHY_EL1_INTID, 0);
  val_gic_install_isr(intid, isr_failsafe);
  val_timer_set_phy_el1(timer_expire_val);
}

static
void
wakeup_clear_failsafe()
{
  val_timer_set_phy_el1(0);
}

static
void
payload4()
{
  uint32_t status;
  uint32_t ns_wdg = 0;
  uint32_t intid;
  uint32_t delay_loop = MAX_SPIN_LOOPS;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint64_t timer_expire_val = val_get_timeout_to_ticks(acs_policy_get_timeout_pass());

  wd_num = val_wd_get_info(0, WD_INFO_COUNT);

  // Assume a test passes until something causes a failure.
  val_set_status(index, RESULT_PASS);

  if (!wd_num) {
      val_print(DEBUG, "\n       No watchdog implemented      ");
      val_set_status(index, RESULT_SKIP(1));
      return;
  }

  while (wd_num--) {
      if (val_wd_get_info(wd_num, WD_INFO_ISSECURE))
          continue;

      ns_wdg++;
      intid = val_wd_get_info(wd_num, WD_INFO_GSIV);
      status = val_gic_install_isr(intid, isr4);

      if (status == 0) {

          /* Set Interrupt Type Edge/Level Trigger */
          if (val_wd_get_info(wd_num, WD_INFO_IS_EDGE))
              val_gic_set_intr_trigger(intid, INTR_TRIGGER_INFO_EDGE_RISING);
          else
              val_gic_set_intr_trigger(intid, INTR_TRIGGER_INFO_LEVEL_HIGH);

          g_wd_int_received = 0;
          g_failsafe_int_received = 0;
	  wakeup_set_failsafe();
	  status = val_wd_set_ws0(wd_num, timer_expire_val);
          if (status) {
              wakeup_clear_failsafe();
    	      val_print(ERROR, "\n       Setting watchdog timeout failed");
              val_set_status(index, RESULT_FAIL(2));
              return;
          }
          val_power_enter_semantic(BSA_POWER_SEM_B);

          /* Add a delay loop after WFI called in case PE needs some time to enter WFI state
           * exit in case test or failsafe int is received
           *
           * This delay loop is a bounded spin wait used only to wait for the
           * interrupt to arrive. It is not time-based and does not represent
           * system counter ticks.
          */
	  while (delay_loop && (g_wd_int_received == 0) && (g_failsafe_int_received == 0)) {
              delay_loop--;
          }

          /* We are here means
           * 1. test interrupt has come (PASS) isr4
           * 2. failsafe int recvd before test int (FAIL) isr_failsafe
           * 3. some other system interrupt or event has wakeup the PE (SKIP)
           * 4. PE didn't enter WFI mode, treating as (SKIP), as finding 3rd,4th case not feasible
           * 5. Hang, if PE didn't exit WFI (FAIL)
          */
	  wakeup_clear_failsafe();
          if (!(g_wd_int_received || g_failsafe_int_received)) {
              intid = val_wd_get_info(wd_num, WD_INFO_GSIV);
	      val_gic_clear_interrupt(intid);
    	      val_print(DEBUG,
                        "\n       PE wakeup by some other events/int or didn't enter WFI");
              val_set_status(index, RESULT_SKIP(1));
	  }
	  val_print(DEBUG, "\n       delay loop remainig value %d", delay_loop);
      } else {
          val_print(WARN, "\n       GIC Install Handler Failed...");
          val_set_status(index, RESULT_FAIL(3));
      }

      /* Disable watchdog so it doesn't trigger after this test. */
      val_wd_set_ws0(wd_num, 0);
  }

  if (!ns_wdg) {
      val_print(DEBUG, "       No non-secure watchdog implemented\n");
      val_set_status(index, RESULT_SKIP(2));
      return;
  }

}

uint32_t
u004_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This Timer test is run on single processor

  /* Watchdog */
  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload4, 0);

  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
