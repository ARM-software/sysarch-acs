/** @file
 * Copyright (c) 2024-2026, Arm Limited or its affiliates. All rights reserved.
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
#include "acs_gic.h"
#include "acs_pe.h"

#define TEST_NUM   (ACS_GIC_TEST_NUM_BASE + 8)
#define TEST_RULE  ""
#define TEST_DESC  "Check PPI intr ID for PPI assignment  "

static
void
payload()
{

    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t intid;

    /*Get GIC maintenance interrupt ID*/
    intid = val_pe_get_gmain_gsiv(index);
    /*Recommended GIC maintenance interrupt ID is 25 as per BSA*/
    if (intid != 25) {
       val_print(ACS_PRINT_ERR,
                 "\n       GIC Maintenance interrupt not mapped to PPI ID 25, id %d", intid);
       val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
       return;
    }

    /*Get EL2 physical timer interrupt ID*/
    intid = val_timer_get_info(TIMER_INFO_PHY_EL2_INTID, 0);
    /*Recommended EL2 physical timer interrupt ID is 26 as per BSA*/
    if (intid != 26) {
        val_print(ACS_PRINT_DEBUG,
              "\n       NS EL2 physical timer not mapped to PPI id 26, INTID: %d ", intid);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
        return;
    }

    /* Check non-secure virtual timer interrupt ID */
    intid = val_timer_get_info(TIMER_INFO_VIR_EL1_INTID, 0);
    /*Recommended non-secure virtual timer interrupt ID is 27 as per BSA*/
    if (intid != 27) {
        val_print(ACS_PRINT_ERR,
            "\n       EL0-Virtual timer not mapped to PPI ID 27, INTID: %d   ", intid);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
        return;
    }

    /*Get EL2 virtual timer interrupt ID*/
    intid = val_timer_get_info(TIMER_INFO_VIR_EL2_INTID, 0);
    /*Recommended EL2 virtual timer interrupt ID is 28 as per BSA*/
    if (intid != 28) {
       val_print(ACS_PRINT_ERR, "\n       NS EL2 virtual timer not mapped to PPI ID 28, id %d",
                                                                intid);
       val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
       return;
    }

    /* Check non-secure physical timer interrupt ID */
    intid = val_timer_get_info(TIMER_INFO_PHY_EL1_INTID, 0);
    /*Recommended non-secure physical timer interrupt ID is 30 as per BSA*/
    if (intid != 30) {
        val_print(ACS_PRINT_ERR,
            "\n       EL0-Phy timer not mapped to PPI ID 30, INTID: %d   ", intid);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
        return;
    }

    val_set_status(index, RESULT_PASS(TEST_NUM, 1));
}

uint32_t
g008_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  /* This GIC test is run on single processor */

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
