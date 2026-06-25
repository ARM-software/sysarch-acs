/** @file
 * Copyright (c) 2016-2018, 2021, 2023-2026, Arm Limited or its affiliates. All rights reserved.
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
#include "acs_timer_infra.h"

#define TEST_NUM   (ACS_TIMER_TEST_NUM_BASE + 3)
#define TEST_RULE  "B_TIME_07"
#define TEST_DESC  "Memory mapped timer check             "

#define ARBIT_VALUE      0xA000

static
void
payload()
{

  uint64_t cnt_ctl_base, cnt_base_n;
  uint32_t data, status, ns_timer = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint64_t timer_num = val_timer_get_info(TIMER_INFO_NUM_PLATFORM_TIMERS, 0);
  uint64_t data1;

  if (!timer_num) {
      val_print(DEBUG, "\n       No System timers are defined  ");
      val_set_status(index, RESULT_SKIP(1));
      return;
  }

  while (timer_num) {
      --timer_num;  //array index starts from 0, so subtract 1 from count

      if (val_timer_get_info(TIMER_INFO_IS_PLATFORM_TIMER_SECURE, timer_num))
          continue;    //Skip Secure Timer

      ns_timer++;
      cnt_ctl_base = val_timer_get_info(TIMER_INFO_SYS_CNTL_BASE, timer_num);
      cnt_base_n   = val_timer_get_info(TIMER_INFO_SYS_CNT_BASE_N, timer_num);

      if (cnt_ctl_base == 0) {
          val_print(DEBUG, "\n       CNTCTL BASE is zero             ");
          val_set_status(index, RESULT_SKIP(2));
          return;
      }
      //Read CNTACR to determine whether access permission from NS state is permitted
      status = val_timer_skip_if_cntbase_access_not_allowed(timer_num);
      if (status == ACS_STATUS_SKIP) {
          val_print(DEBUG,
                    "\n       Security doesn't allow access to timer registers      ");
          val_set_status(index, RESULT_SKIP(3));
          return;
      }

      // Read write check for a read only register CNTTIDR
      data = val_mmio_read(cnt_ctl_base + CNTTIDR);
      val_mmio_write(cnt_ctl_base + CNTTIDR, 0xFFFFFFFF);
      if (data != val_mmio_read(cnt_ctl_base + CNTTIDR)) {
          val_print(DEBUG, "\n       Read-write check failed for"
              " CNTCTLBase.CNTTIDR, expected value %x ", data);
          val_set_status(index, RESULT_FAIL(1));
          return;
      }

      if (cnt_base_n == 0) {
          val_print(DEBUG, "\n       CNT_BASE_N is zero                 ");
          val_set_status(index, RESULT_SKIP(4));
          return;
      }

      // Read CNTPCT register
      data1 = val_mmio_read64(cnt_base_n + CNTPCT_LOWER);
      val_print(DEBUG, "\n       CNTPCT Read value = 0x%llx       ", data1);

      // Writes to Read-Only registers must be ignored
      val_mmio_write64(cnt_base_n + CNTPCT_LOWER, (data1 - ARBIT_VALUE));

      if (val_mmio_read64(cnt_base_n + CNTPCT_LOWER) < data1) {
          val_print(DEBUG, "\n       CNTBaseN: CNTPCT reg must be read-only ");
          val_set_status(index, RESULT_FAIL(2));
          return;
      }

      // Read CNTVCT register
      data1 = val_mmio_read64(cnt_base_n + CNTVCT_LOWER);
      val_print(DEBUG, "\n       CNTVCT Read value = 0x%llx       ", data1);

      // Writes to Read-Only registers must be ignored
      val_mmio_write64(cnt_base_n + CNTVCT_LOWER, (data1 - ARBIT_VALUE));

      if (val_mmio_read64(cnt_base_n + CNTVCT_LOWER) < data1) {
          val_print(DEBUG, "\n       CNTBaseN: CNTVCT reg must be read-only ");
          val_set_status(index, RESULT_FAIL(3));
          return;
      }

      // Read CNTFRQ register
      data = val_mmio_read(cnt_base_n + CNTBaseN_CNTFRQ);
      val_print(DEBUG, "\n       CNTFRQ Read value = 0x%x         ",
                                                                        data);

      // Writes to Read-Only registers must be ignored
      val_mmio_write(cnt_base_n + CNTBaseN_CNTFRQ, (data - ARBIT_VALUE));

      if (val_mmio_read(cnt_base_n + CNTBaseN_CNTFRQ) != data) {
          val_print(DEBUG, "\n       CNTBaseN: CNTFRQ reg must be read-only ");
          val_set_status(index, RESULT_FAIL(4));
          return;
      }

      /* Write Read check for RW register*/
      data = 0x3;
      val_mmio_write(cnt_base_n + CNTP_CTL, data);
      if (data != (val_mmio_read(cnt_base_n + CNTP_CTL) & 0x3)) {
          val_print(ERROR, "\n       Read-write check failed for "
              "CNTBaseN.CNTP_CTL, expected value %x ", data);
          val_print(DEBUG, "\n       Read value %x ",
                    val_mmio_read(cnt_base_n + CNTP_CTL));
          val_set_status(index, RESULT_FAIL(5));
          val_mmio_write(cnt_base_n + CNTP_CTL, 0x0); // Disable the timer before return
          return;
      }
      val_mmio_write(cnt_base_n + CNTP_CTL, 0x0); // Disable timer

      data = 0xFF00FF00;
      /* Write a random value to CNTP_CVAL[31:0]*/
      val_mmio_write(cnt_base_n + CNTP_CVAL_LOWER, data);
      /* Write a random value to CNTP_CVAL[63:32]*/
      val_mmio_write(cnt_base_n + CNTP_CVAL_HIGHER, data);

      if (data != val_mmio_read(cnt_base_n + CNTP_CVAL_LOWER)) {
          val_print(DEBUG, "\n       Read-write check failed for "
              "CNTBaseN.CNTP_CVAL[31:0], read value %x ",
              val_mmio_read(cnt_base_n + CNTP_CVAL_LOWER));
          val_set_status(index, RESULT_FAIL(6));
          return;
      }

      if (data != val_mmio_read(cnt_base_n + CNTP_CVAL_HIGHER)) {
          val_print(DEBUG, "\n       Read-write check failed for"
              " CNTBaseN.CNTP_CVAL[63:32], read value %x ",
              val_mmio_read(cnt_base_n + CNTP_CVAL_HIGHER));
          val_set_status(index, RESULT_FAIL(7));
          return;
      }

      val_set_status(index, RESULT_PASS);
  }

  if (!ns_timer) {
      val_print(DEBUG, "\n       No non-secure systimer implemented");
      val_set_status(index, RESULT_SKIP(5));
      return;
  }

}

uint32_t
t003_entry(uint32_t num_pe)
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
