/** @file
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/acs_timer_support.h"
#include "val/include/acs_pe.h"
#include "val/include/val_interface.h"

#define TEST_NUM   (ACS_TIMER_TEST_NUM_BASE + 8)
#define TEST_DESC  "Check uniform passage of time in VE"
#define TEST_RULE  "V_L1TM_04"

/* Test to be run on virtual environments */
#define NUM_ITERATIONS  1000000

static
void payload(void)
{
    uint64_t feat_ecv_impl = 0;
    uint64_t iter  = NUM_ITERATIONS;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint64_t prev_value = 0;
    uint64_t curr_value = 0;
    bool     test_fail  = 0;

    /*  ID_AA64MMFR0_EL1.ECV[63:60] != 0b00 indicate Enhanced counter vitualization */
    feat_ecv_impl = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64MMFR0_EL1), 60, 63);
    if (feat_ecv_impl) {

        val_print_primary_pe(ACS_PRINT_DEBUG, "\n       FEAT_ECV is Implemented, Reading CntPctSS",
                                                                                        0, index);

        if (g_el1physkip)
            goto read_virt_ss_timer;

        while (iter) {
            curr_value = ArmReadCntPctSS();
            if (curr_value < prev_value) {
                val_print_primary_pe(ACS_PRINT_ERR, "\n       CNTPCTSS_EL0 did not increment",
                                                                                        0, index);
                val_print_primary_pe(ACS_PRINT_DEBUG, "\n       CNTPCTSS_EL0 Value: %lx",
                                                                                curr_value, index);
                val_print_primary_pe(ACS_PRINT_DEBUG, "\n       Previous Value: %lx",
                                                                                prev_value, index);
                test_fail = 1;
            }

            prev_value = curr_value;
            iter--;
        }

read_virt_ss_timer:
        val_print_primary_pe(ACS_PRINT_DEBUG, "\n       FEAT_ECV is Implemented, Reading CntVctSS",
                                                                                        0, index);
        prev_value = 0;
        iter = NUM_ITERATIONS;

        while (iter) {
            curr_value = ArmReadCntVctSS();
            if (curr_value < prev_value) {
                val_print_primary_pe(ACS_PRINT_ERR, "\n       CNTVCTSS_EL0 did not increment",
                                                                                        0, index);
                val_print_primary_pe(ACS_PRINT_DEBUG, "\n       CNTVCTSS_EL0 Value: %lx",
                                                                                curr_value, index);
                val_print_primary_pe(ACS_PRINT_DEBUG, "\n       Previous Value: %lx",
                                                                                prev_value, index);
                test_fail = 1;
            }

            prev_value = curr_value;
            iter--;
        }

    }
    else {

        if (g_el1physkip)
            goto read_virt_timer;

        val_print_primary_pe(ACS_PRINT_DEBUG, "\n       FEAT_ECV isn't Implemented, Reading CntPct",
                                                                                        0, index);
        while (iter) {
            AA64IssueISB();
            curr_value = ArmReadCntPct();
            if (curr_value < prev_value) {
                val_print_primary_pe(ACS_PRINT_ERR, "\n       CNTPCT_EL0 did not increment",
                                                                                        0, index);
                val_print_primary_pe(ACS_PRINT_DEBUG, "\n       CNTPCT_EL0 Value: %lx",
                                                                                curr_value, index);
                val_print_primary_pe(ACS_PRINT_DEBUG, "\n       Previous Value: %lx",
                                                                                prev_value, index);
                test_fail = 1;
            }

            prev_value = curr_value;
            iter--;
        }

read_virt_timer:
        val_print_primary_pe(ACS_PRINT_DEBUG, "\n       FEAT_ECV isn't Implemented, Reading CntVct",
                                                                                        0, index);
        prev_value = 0;
        iter = NUM_ITERATIONS;

        while (iter) {
            AA64IssueISB();
            curr_value = ArmReadCntvCt();
            if (curr_value < prev_value) {
                val_print_primary_pe(ACS_PRINT_ERR, "\n       CNTVCT_EL0 did not increment",
                                                                                        0, index);
                val_print_primary_pe(ACS_PRINT_DEBUG, "\n       CNTVCT_EL0 Value: %lx",
                                                                                curr_value, index);
                val_print_primary_pe(ACS_PRINT_DEBUG, "\n       Previous Value: %lx",
                                                                                prev_value, index);
                test_fail = 1;
            }

            prev_value = curr_value;
            iter--;
        }
    }

    if (test_fail == 1) {
        val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
        return;
    }

    val_set_status(index, RESULT_PASS(TEST_NUM, 1));

}

uint32_t
t008_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);
  return status;
}
