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

#define TEST_NUM    (ACS_TIMER_TEST_NUM_BASE + 11)
#define TEST_DESC   "System counter freq at least 10MHz   "
#define TEST_RULE   "B_TIME_02"

#define TEST_NUM1    (ACS_TIMER_TEST_NUM_BASE + 12)
#define TEST_DESC1   "System counter freq at least 50MHz   "
#define TEST_RULE1   "S_L8TI_01"

#define MIN_SYS_CNT_FREQ_10MHZ   10000000ULL
#define MIN_SYS_CNT_FREQ_50MHZ   50000000ULL

static uint64_t min_freq_hz;

static
void
payload(void)
{
    uint64_t freq = 0;
    uint64_t ret;
    uint64_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    ret = val_el3_read_base_freq(&freq);
    if (ret == ACS_SMC_UNK_RET) {
        val_set_status(index, RESULT_SKIP(1));
        return;
    }

    if (ret) {
        val_print(ERROR, "\n       Failed to read base system frequency via EL3");
        val_set_status(index, RESULT_WARNING(1));
        return;
    }

    val_print(DEBUG, "\n       Base system frequency = %ld MHz", freq / 1000000);

    if (freq < min_freq_hz) {
        val_print(ERROR, "\n       Base system frequency %ld MHz is below required freq %ld MHz",
                freq / 1000000, min_freq_hz / 1000000);
        val_set_status(index, RESULT_FAIL(1));
        return;
    }

    val_set_status(index, RESULT_PASS);
    return;
}

uint32_t
t010_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;
  min_freq_hz = MIN_SYS_CNT_FREQ_10MHZ;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}

uint32_t
t011_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;
  min_freq_hz = MIN_SYS_CNT_FREQ_50MHZ;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM1, TEST_DESC1, num_pe);

  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM1, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM1, num_pe, TEST_RULE1);

  val_report_status(0, ACS_END(TEST_NUM1), NULL);

  return status;
}