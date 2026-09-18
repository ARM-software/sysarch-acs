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
#include "acs_timer.h"
#include "acs_pe.h"
#include "acs_gic.h"

#define TEST_NUM  (ACS_GIC_TEST_NUM_BASE + 21)
#define TEST_DESC "Verify Secure Physical timer (CNTPS) mapping"
#define TEST_RULE "B_PPI_03"

#define BSA_RECOMMENDED_SECURE_TIMER_PPI     29U

static void payload(void)
{
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t intid = val_timer_get_info(TIMER_INFO_SEC_PHY_EL1_INTID, 0);

    val_print(DEBUG, "\n       CNTPS PPI INTID (reported) = %u", intid);

    /* Check PPI/EPPI range */
    if ((intid < 16U || intid > 31U) && !val_gic_is_valid_eppi(intid)) {
        val_print(ERROR, "\n       CNTPS INTID not a valid PPI/EPPI: %u", intid);
        val_set_status(index, RESULT_FAIL(1));
        return;
    }

    if (intid != BSA_RECOMMENDED_SECURE_TIMER_PPI) {
        val_print(WARN, "\n       PPI ID %d does not match the BSA recommended PPI ID %d",
            intid, BSA_RECOMMENDED_SECURE_TIMER_PPI);
        val_set_status(index, RESULT_WARNING(1));
        return;
    }

    /* Verifies CNTPS PPI/EPPI mapping only. CNTPS is a Secure Physical Timer
     * and interrupt delivery cannot be validated from Non-secure execution state.
     */
    val_set_status(index, RESULT_PARTIAL_COVERED);
}

uint32_t g017_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This GIC test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
