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

#include "acs_val.h"
#include "acs_pe.h"
#include "acs_mpam.h"
#include "acs_mpam_reg.h"
#include "val_interface.h"

#define TEST_NUM   (ACS_MPAM_REGISTER_TEST_NUM_BASE + 6)
#define TEST_RULE  "RNSLZ"
#define TEST_DESC  "Check NFU bit when PARTID NRW is impl "

static void payload(void)
{
    uint32_t msc_index;
    uint32_t nfu_bit     = 0;
    uint64_t mpamf_idr   = 0;
    uint32_t pe_index    = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t total_nodes = val_mpam_get_msc_count();

    for (msc_index = 0; msc_index < total_nodes; msc_index++) {

        /* Check if MSC supports PARTID narrowing feature */
        if (!val_mpam_msc_supports_partid_nrw(msc_index))
            continue;

        val_print(DEBUG, "\n       MSC %d supports PARTID NRW", msc_index);

        /* Read MPAMF_IDR register */
        mpamf_idr = val_mpam_mmr_read64(msc_index, REG_MPAMF_IDR);

        if (!(BITFIELD_READ(IDR_EXT, mpamf_idr)))
            /* Skip if 32b MPAMF_IDR is implemented */
            continue;

        /* If PARTID narrowing is supported, MPAMCFG_DIS.NFU must not be implemented.
           Check for MPAMF_IDR.HAS_NFU = 0 */
        if (BITFIELD_READ(IDR_HAS_NFU, mpamf_idr)) {
            val_print(ERROR, "\n       MSC %d has NFU bit implemented", msc_index);
            nfu_bit++;
        }
    }

    if (nfu_bit != 0) {
        /* Fail The Test */
        val_set_status(pe_index, RESULT_FAIL(01));
        return;
    }

    val_set_status(pe_index, RESULT_PASS);
    return;
}

uint32_t reg006_entry(uint32_t num_pe)
{

    uint32_t status = ACS_STATUS_FAIL;

    num_pe = 1;

    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

    if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}
