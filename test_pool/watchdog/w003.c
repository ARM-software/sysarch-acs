/** @file
 * Copyright (c) 2020-2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/val_interface.h"
#include "val/include/acs_wd.h"

#define TEST_NUM   (ACS_WD_TEST_NUM_BASE + 3)
#define TEST_RULE  "S_L6WD_01"
#define TEST_DESC  "Check NS Watchdog Revision            "

static
void
payload(void)
{

  uint64_t ctrl_base;
  uint64_t wd_num = val_wd_get_info(0, WD_INFO_COUNT);
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t data, ns_wdg = 0;

  if (g_sbsa_level < 5) {
      val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
      return;
  }

  val_print(ACS_PRINT_DEBUG, "\n       Found %d watchdogs in ACPI table ", wd_num);

  if (wd_num == 0) {
      val_print(ACS_PRINT_WARN, "\n       No Watchdogs reported          %d  ", wd_num);
      val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
      return;
  }

  do {
      wd_num--; //array index starts from 0, so subtract 1 from count

      if (val_wd_get_info(wd_num, WD_INFO_ISSECURE))
          continue;    //Skip Secure watchdog

      ns_wdg++;
      ctrl_base    = val_wd_get_info(wd_num, WD_INFO_CTRL_BASE);
      val_print(ACS_PRINT_INFO, "\n      Watchdog CTRL base is  %llx      ", ctrl_base);

      /* W_IIDR.Architecture Revision [19:16] = 0x1 for Watchdog Rev 1 */
      data = VAL_EXTRACT_BITS(val_mmio_read(ctrl_base + WD_IIDR_OFFSET), 16, 19);

      if (g_sbsa_level == 5) {
          if (data != 1) {
              val_print(ACS_PRINT_WARN, "\n       Watchdog Architecture version is %x", data);
              val_set_status(index, RESULT_SKIP(TEST_NUM, 02));
              return;
          }
      }

      if (g_sbsa_level > 5) {
          if (data != 1) {
              val_print(ACS_PRINT_ERR, "\n       Watchdog Architecture version is %x", data);
              val_set_status(index, RESULT_FAIL(TEST_NUM, 02));
              return;
          }
      }

  } while(wd_num);

  if(!ns_wdg) {
      val_print(ACS_PRINT_WARN, "\n       No non-secure Watchdogs reported", 0);
      val_set_status(index, RESULT_FAIL(TEST_NUM, 03));
      return;
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 01));

}

uint32_t
w003_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    num_pe = 1;  //This test is run on single processor

    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;

}
