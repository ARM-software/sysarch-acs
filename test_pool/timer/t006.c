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
#include "acs_timer.h"
#include "acs_timer_support.h"

#define TEST_NUM   (ACS_TIMER_TEST_NUM_BASE + 6)
#define TEST_RULE  "S_L5TI_01"
#define TEST_DESC  "Check OS visible counter reports 1GHz "

#define MICROSECONDS_PER_SECOND  1000000ULL
#define MAX_DELTA_PERCENT        1ULL

static
void
payload()
{
  uint64_t counter_freq, print_freq = 0;
  uint64_t counter_start_value, counter_end_value;
  uint64_t time_start_us, time_end_us;
  uint64_t counter_delta, elapsed_time_us;
  uint64_t est_freq_hz, freq_delta_hz;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t print_mhz = 0;
  uint32_t iter;

  counter_freq = val_timer_get_info(TIMER_INFO_CNTFREQ, 0);

  if (counter_freq != 0) {
    print_freq = counter_freq/1000;
    if (print_freq > 1000) {
      print_freq = print_freq/1000;
      print_mhz = 1;
    }
  }

  if (print_mhz)
    val_print(INFO, "\n       Counter frequency is %ld MHz", print_freq);
  else
    val_print(INFO, "\n       Counter frequency is %ld KHz", print_freq);

  /* Acc to S_L5TI_01, the OS visible counter should be able to count in nanosecond units.
     That means the reported effective freq by CNTFRQ_EL0 should be exactly 1GHz,
     consistent with post Armv8.6 systems that mandate a fixed 1GHz counter freq */
  if (counter_freq == (1000 * 1000 * 1000))
      val_set_status(index, RESULT_PASS);
  else
      val_set_status(index, RESULT_FAIL(1));

  /* The test below is only a sanity check. It will not measure the compliance of the system against
     S_L5TI_01.

     Sanity Check: CNTFRQ_EL0 is programmed by the firmware and software relies on it to derive the
     timer resolution. CNTPCT_EL0 is a hardware managed physical counter which progresses at the
     rate mentioned by CNTFID and if scaling is implemented the counter increments based on the
     scale value mentioned in CNTSCR.ScaleVal.

     The purpose of this check is to verify that the observed progression of CNTPCT_EL0 is
     consistent with the frequency reported to software through CNTFRQ_EL0.
  */

  /* Algorithm: Sample CNTPCT_EL0 at two points separated by 1000 ms while tracking the same
                interval with an independent timer source.  Use the elapsed counter delta
                and the independent time delta to estimate the effective frequency of CNTPCT_EL0.
  */

  for (iter = 0; iter < 5; iter++) {
      isb();
      counter_start_value = read_cntpct_el0();
      time_start_us = val_get_platform_time_us();
      if ((time_start_us == PAL_TIME_US_INVALID) ||
          (time_start_us == ACS_STATUS_PAL_NOT_IMPLEMENTED)) {
          val_print(DEBUG, "\n       Platform time source unavailable; skipping sanity check");
          return;
      }

      val_time_delay_ms(1000 * ONE_MILLISECOND);

      isb();
      counter_end_value = read_cntpct_el0();
      time_end_us = val_get_platform_time_us();
      if (time_end_us == PAL_TIME_US_INVALID) {
          val_print(DEBUG, "\n       Platform time source unavailable; skipping sanity check");
          return;
      }

      counter_delta   = counter_end_value - counter_start_value;
      elapsed_time_us = time_end_us - time_start_us;

      if (elapsed_time_us == 0) {
          val_print(DEBUG, "\n       Platform time delta is zero; skipping sanity check");
          return;
      }

      est_freq_hz = (counter_delta * MICROSECONDS_PER_SECOND) / elapsed_time_us;
      val_print(DEBUG, "\n       Estimated CNTPCT_EL0 frequency is %ld Hz", est_freq_hz);

      if (est_freq_hz >= counter_freq)
        freq_delta_hz = est_freq_hz - counter_freq;
      else
        freq_delta_hz = counter_freq - est_freq_hz;

      if (((freq_delta_hz * 100ULL) / counter_freq) > MAX_DELTA_PERCENT)
          val_print(WARN, "\n       Sanity check: CNTPCT_EL0 progression is not consistent with"
                        " CNTFRQ_EL0");
      else
          val_print(DEBUG, "\n       Sanity check: CNTPCT_EL0 progression is consistent with"
                        " CNTFRQ_EL0");
  }
}

uint32_t
t006_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This Timer test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);
  return status;
}
