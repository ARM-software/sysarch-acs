/** @file
 * Copyright (c) 2016-2018,2021,2024-2025, Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/acs_val.h"
#include "val/include/acs_timer.h"

#define TEST_NUM   (ACS_TIMER_TEST_NUM_BASE + 1)
#define TEST_RULE  "B_TIME_01"
#define TEST_DESC  "Check for Generic System Counter      "

#define TEST_NUM1   (ACS_TIMER_TEST_NUM_BASE + 7)
#define TEST_RULE1  "B_TIME_02"
#define TEST_DESC1  "Check System Counter Frequency        "

/* This test checks for presence of Generic System Counter */
static
void
payload_check_system_counter_presence()
{
  uint64_t counter_freq;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* The existence of the CNTFRQ_EL0 register and the ability to read a non-zero
     value from it indicates the system counter is present.  */
  counter_freq = val_timer_get_info(TIMER_INFO_CNTFREQ, 0);

  if (counter_freq == 0) {
      val_print(ACS_PRINT_DEBUG, "\n       Generic system counter not implemented,"
                                 " CNTFRQ_EL0 = 0", 0);
      val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
      return;
  } else {
      val_set_status(index, RESULT_PASS(TEST_NUM, 1));
      return;
  }
}

/* This test checks if Generic System Counter frequency is greater than 10Mhz */
static
void
payload_check_system_timer_freq()
{
  uint64_t counter_freq, print_freq;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t print_mhz = 0;

  /* Read CNTFRQ_EL0 register for counter frequency */
  counter_freq = val_timer_get_info(TIMER_INFO_CNTFREQ, 0);

  /* Convert frequency into MHz or KHz unit */
  if (counter_freq != 0) {
      print_freq = counter_freq/1000;
      if (print_freq > 1000) {
          print_freq = print_freq/1000;
          print_mhz = 1;
      }
  }

  /* Print counter frequency in DEBUG verbosity */
  if (print_mhz)
      val_print(ACS_PRINT_DEBUG, "\n       Counter frequency is %ld MHz", print_freq);
  else
      val_print(ACS_PRINT_DEBUG, "\n       Counter frequency is %ld KHz", print_freq);

  /* Check if Generic system counter frequency is greater than 10MHz */
  if (counter_freq > 10*1000*1000) {
      val_set_status(index, RESULT_PASS(TEST_NUM1, 1));
      return;
  }

  /* If 10Mhz check fails, print frequency in ERROR verbosity */
  if (print_mhz)
      val_print(ACS_PRINT_ERR, "\n       Counter frequency is %ld MHz", print_freq);
  else
      val_print(ACS_PRINT_ERR, "\n       Counter frequency is %ld KHz", print_freq);

  val_set_status(index, RESULT_FAIL(TEST_NUM1, 1));
}

uint32_t
t001_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  /* This Timer test is run on single processor */

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload_check_system_counter_presence, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}

uint32_t
t007_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This Timer test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM1, TEST_DESC1, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM1, num_pe, payload_check_system_timer_freq, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM1, num_pe, TEST_RULE1);

  val_report_status(0, ACS_END(TEST_NUM1), NULL);

  return status;
}
