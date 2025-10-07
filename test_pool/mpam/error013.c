/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   ACS_MPAM_ERROR_TEST_NUM_BASE  +  16
#define TEST_DESC  "Check MPAMF_ESR.RIS field correctness "
#define TEST_RULE  ""

static
void payload(void)
{
    uint32_t status;
    uint64_t data;
    uint32_t msc_index;
    uint32_t pe_index;
    uint32_t total_nodes;
    uint32_t mpamf_ecr;
    uint32_t data_original;
    uint64_t esr_value;
    uint32_t programmed_ris;
    uint32_t max_ris_index;
    uint32_t test_fail = 0;
    uint32_t test_skip = 1;
    uint32_t partid;

    pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
    total_nodes = val_mpam_get_msc_count();

    for (msc_index = 0; msc_index < total_nodes; msc_index++) {

      if (!val_mpam_msc_supports_ris(msc_index)) {
        val_print(ACS_PRINT_DEBUG, "\n       MSC index %d does not support RIS", msc_index);
        continue;
      }

      if (!val_mpam_msc_supports_esr(msc_index)) {
        val_print(ACS_PRINT_DEBUG, "\n       MSC index %d does not support ESR", msc_index);
        continue;
      }

      if (!val_mpam_msc_supports_extd_esr(msc_index)) {
        val_print(ACS_PRINT_DEBUG, "\n       MSC index %d does not support EXTD_ESR", msc_index);
        continue;
      }

      /* Save and reset error state */
      mpamf_ecr = val_mpam_mmr_read(msc_index, REG_MPAMF_ECR);

      status = val_mpam_msc_reset_errcode(msc_index);
      if (!status) {
        val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 01));
        return;
      }

      test_skip = 0;

      max_ris_index = val_mpam_get_max_ris_count(msc_index);
      if (max_ris_index == 0xF) {
        /* Maximum RIS Supported, Skip This MSC */
        val_print(ACS_PRINT_DEBUG, "\n       MSC index %d supports all RIS", msc_index);
        continue;
      }

      /********* Case 1: MPAMCFG_PART_SEL *********/
      /* Save original value */
      data_original = val_mpam_mmr_read(msc_index, REG_MPAMCFG_PART_SEL);

      programmed_ris = max_ris_index + 1;
      partid = val_mpam_get_max_partid(msc_index);

      data = BITFIELD_SET(PART_SEL_PARTID_SEL, partid) |
             BITFIELD_SET(PART_SEL_RIS, programmed_ris);

      val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, data);
      val_print(ACS_PRINT_DEBUG, "\n       Value written to MPAMCFG_PART_SEL is %llx", data);

      /* Access MPAMCFG_* register to cause the error */
      val_mpam_mmr_read(msc_index, REG_MPAMCFG_CMAX);

      /* Wait some time for error to be reflected in MPAMF_ESR */
      val_time_delay_ms(100 * ONE_MILLISECOND);

      /* Read Error Status Register, check if error code recorded */
      esr_value = val_mpam_mmr_read64(msc_index, REG_MPAMF_ESR);

      if ((esr_value >> ESR_ERRCODE_SHIFT) & ESR_ERRCODE_MASK) {
        if (((esr_value >> ESR_RIS_SHIFT) & ESR_RIS_MASK) != programmed_ris) {
          val_print(ACS_PRINT_ERR, "\n       ESR.RIS mismatch MPAMCFG access: expected 0x%x",
                              programmed_ris);
          val_print(ACS_PRINT_ERR, ", Received 0x%x",
                              ((esr_value >> ESR_RIS_SHIFT) & ESR_RIS_MASK));
          test_fail++;
        }
      } else {
        /* Print Warning for Mismatch in Error Code */
        val_print(ACS_PRINT_WARN, "\n       ERRCODE mismatch MPAMCFG access: Received 0x%x",
                                    ((esr_value >> ESR_ERRCODE_SHIFT) & ESR_ERRCODE_MASK));
      }

      /* Restore and Reset */
      val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, data_original);
      val_mpam_msc_reset_errcode(msc_index);

      /********* Case 2: MSMON_CFG_MON_SEL *********/
      if (val_mpam_msc_supports_mon(msc_index)) {
        data_original = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_MON_SEL);
        programmed_ris = max_ris_index + 1;

        data = BITFIELD_SET(MON_SEL_MON_SEL, 0) |
               BITFIELD_SET(MON_SEL_RIS, programmed_ris);

        val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, data);
        val_print(ACS_PRINT_DEBUG, "\n       Value written to MSMON_CFG_MON_SEL is %llx", data);

        /* Access MSMON_CFG_* register to cause the error */
        val_mpam_mmr_read(msc_index, REG_MSMON_CFG_MBWU_FLT);

        /* Wait some time for error to be reflected in MPAMF_ESR */
        val_time_delay_ms(100 * ONE_MILLISECOND);

        /* Read Error Status Register, check if error code recorded */
        esr_value = val_mpam_mmr_read64(msc_index, REG_MPAMF_ESR);

        if ((esr_value >> ESR_ERRCODE_SHIFT) & ESR_ERRCODE_MASK) {
          if (((esr_value >> ESR_RIS_SHIFT) & ESR_RIS_MASK) != programmed_ris) {
            val_print(ACS_PRINT_ERR, "\n       ESR.RIS mismatch MSMON_CFG access: expected 0x%x",
                                programmed_ris);
            val_print(ACS_PRINT_ERR, ", Received 0x%x",
                                ((esr_value >> ESR_RIS_SHIFT) & ESR_RIS_MASK));
            test_fail++;
          }
        } else {
          /* Print Warning for Mismatch in Error Code */
          val_print(ACS_PRINT_WARN, "\n       ERRCODE mismatch MSMON_CFG access: Received 0x%x",
                                      ((esr_value >> ESR_ERRCODE_SHIFT) & ESR_ERRCODE_MASK));
        }

        /* Restore and Reset */
        val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, data_original);
        val_mpam_msc_reset_errcode(msc_index);
      }

      /* Restore */
      val_mpam_mmr_write(msc_index, REG_MPAMF_ECR, mpamf_ecr);
    }

    if (test_skip)
        val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
    else if (test_fail)
        val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 02));
    else
        val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t error013_entry(void)
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
