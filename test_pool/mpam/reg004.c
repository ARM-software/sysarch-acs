/** @file
 * Copyright (c) 2025-2026, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (ACS_MPAM_REGISTER_TEST_NUM_BASE + 4)
#define TEST_RULE  ""
#define TEST_DESC  "Check MPAM for RME Feature Test       "

static void payload(void)
{

    uint32_t pe_index;
    uint32_t mpam_minor, mpam_major;
    uint64_t pfr0, pfr1, mpamidr_val;

    pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

    /* Read PE feature registers */
    pfr0 = val_pe_reg_read(ID_AA64PFR0_EL1);
    pfr1 = val_pe_reg_read(ID_AA64PFR1_EL1);

    mpam_major = VAL_EXTRACT_BITS(pfr0, 40, 43);
    mpam_minor = VAL_EXTRACT_BITS(pfr1, 16, 19);

    /* Run the test if PE Supports RME & MPAM */
    if (val_pe_feat_check(PE_FEAT_RME))
    {
      /* Skip the test */
      val_print_primary_pe(ACS_PRINT_INFO, "\n       PE Does not Support RME", 0, pe_index);
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
      return;
    }

    if ((mpam_major == 0) && (mpam_minor == 0))
    {
      /* Skip the test */
      val_print_primary_pe(ACS_PRINT_INFO, "\n       PE Does not Support MPAM", 0, pe_index);
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 02));
      return;
    }

    /* Check Support for MPAM v1.1 */
    if ((mpam_major < 1) || ((mpam_major == 1) && (mpam_minor < 1)))
    {
      val_print_primary_pe(ACS_PRINT_ERR, "\n       PE Does not Support MPAMv1.1", 0, pe_index);
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 01));
      return;
    }

    /* Check PE Supports 4 PARTID Spaces */
    mpamidr_val = val_mpam_reg_read(MPAMIDR_EL1);
    if (VAL_EXTRACT_BITS(mpamidr_val, MPAMIDR_SP4, MPAMIDR_SP4) == 0) {
      val_print_primary_pe(ACS_PRINT_ERR,
                                    "\n       PE Does not Support 4 PARTID Spaces", 0, pe_index);
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 02));
      return;
    }

    /* Check alternative space, ALTSP feature */
    if (VAL_EXTRACT_BITS(mpamidr_val, MPAMIDR_HAS_ALTSP, MPAMIDR_HAS_ALTSP) == 0) {
      val_print_primary_pe(ACS_PRINT_ERR, "\n       PE Does not Support ALTSP", 0, pe_index);
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 03));
      return;
    }

    val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));
    return;
}


uint32_t reg004_entry(void)
{

    uint32_t status = ACS_STATUS_FAIL;
    uint32_t num_pe = val_pe_get_num();

    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

    if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

    val_report_status(0, ACS_END(TEST_NUM), NULL);

    return status;
}
