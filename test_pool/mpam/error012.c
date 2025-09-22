/** @file
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0
 *
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
#include "val/include/acs_pe.h"
#include "val/include/acs_mpam.h"
#include "val/include/acs_mpam_reg.h"
#include "val/include/val_interface.h"

#define TEST_NUM   ACS_MPAM_ERROR_TEST_NUM_BASE  +  15
#define TEST_DESC  "Check MPAM IDR Zero for Undefined RIS "
#define TEST_RULE  ""

static
void payload(void)
{

    uint32_t msc_index;
    uint32_t status;
    uint32_t pe_index;
    uint16_t partid;
    uint32_t total_nodes;
    uint32_t mpamf_ecr;
    uint32_t esr_errcode;
    uint32_t data;
    uint32_t data_original;
    uint32_t max_ris_index;
    uint32_t test_fail = 0;
    uint32_t test_skip = 1;
    uint64_t idr_value = 0;

    pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
    total_nodes = val_mpam_get_msc_count();

    for (msc_index = 0; msc_index < total_nodes; msc_index++) {

      val_print(ACS_PRINT_DEBUG, "\n       MSC index : %d", msc_index);
      if (!val_mpam_msc_supports_ris(msc_index)) {
        val_print(ACS_PRINT_DEBUG, "\n       MSC index %d does not support RIS", msc_index);
        continue;
      }

      /* Check if Error Status Register Supported */
      if (!val_mpam_msc_supports_esr(msc_index)) {
        val_print(ACS_PRINT_DEBUG, "\n       MSC index %d does not support ESR/ECR", msc_index);
        continue;
      }

      /* Read MPAMF_ECR before generating error. This will be used to restore to default later */
      mpamf_ecr = val_mpam_mmr_read(msc_index, REG_MPAMF_ECR);

      status    = val_mpam_msc_reset_errcode(msc_index);
      if (!status) {
        val_print(ACS_PRINT_DEBUG, "\n       Error Code Reset Failed", 0);
        val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 01));
        return;
      }

      test_skip = 0;

      /* Program MPAMCFG_PART_SEL with out-of-range RIS index */
      partid = val_mpam_get_max_partid(msc_index);
      max_ris_index = val_mpam_get_max_ris_count(msc_index);

      data = BITFIELD_SET(PART_SEL_PARTID_SEL, partid) |
             BITFIELD_SET(PART_SEL_RIS, (max_ris_index + 1));

      /* Save original value */
      data_original = val_mpam_mmr_read(msc_index, REG_MPAMCFG_PART_SEL);

      val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, data);
      data = val_mpam_mmr_read(msc_index, REG_MPAMCFG_PART_SEL);
      val_print(ACS_PRINT_DEBUG, "\n       Value Read Back from MPAMCFG_PART_SEL is %llx", data);

      /* Wait for some time for the error to be reflected in MPAMF_ESR */
      val_time_delay_ms(100 * ONE_MILLISECOND);

      /* Reset Errorcode which might have updated after setting out of range RIS */
      status    = val_mpam_msc_reset_errcode(msc_index);
      if (!status) {
        val_print(ACS_PRINT_DEBUG, "\n       Error Code Reset Failed after RIS", 0);
        val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 02));
        return;
      }

      /* Access MPAMF ID register, all HAS_* fields read as 0 */
      idr_value = val_mpam_mmr_read64(msc_index, REG_MPAMF_IDR);

      /* Check All HAS_* Fields read as 0 */
      if (idr_value & MPAMF_IDR_HAS_MASK) {
        val_print(ACS_PRINT_ERR, "\n       ID Register HAS_* Field Non Zero : %llx", idr_value);
        test_fail++;
      }
      /* Restore */
      val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, data_original);

      esr_errcode = val_mpam_msc_get_errcode(msc_index);
      val_print(ACS_PRINT_DEBUG, "\n       Error code read is %llx", esr_errcode);

      if (esr_errcode != ESR_ERRCODE_NO_ERROR) {
        val_print(ACS_PRINT_ERR, "\n       Expected errcode: %d", ESR_ERRCODE_NO_ERROR);
        val_print(ACS_PRINT_ERR, "\n       Actual errcode: %d", esr_errcode);
        test_fail++;
      }
      /* Restore Error Control Register original settings */
      val_mpam_mmr_write(msc_index, REG_MPAMF_ECR, mpamf_ecr);

    }

    if (test_skip)
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
    else if (test_fail)
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 03));
    else
      val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));
    return;
}

uint32_t error012_entry(void)
{

    uint32_t status = ACS_STATUS_FAIL;
    uint32_t num_pe = 1;

    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

    if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

    val_report_status(0, ACS_END(TEST_NUM), NULL);

    return status;
}
