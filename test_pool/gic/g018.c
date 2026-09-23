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

#define BSA_RECOMMENDED_CNTHPS_PPI     20U
#define BSA_RECOMMENDED_CNTHVS_PPI     19U

#define TEST_NUM   (ACS_GIC_TEST_NUM_BASE + 22)
#define TEST_DESC  "Verify S-EL2 CNTHPS PPI mapping       "
#define TEST_RULE  "B_PPI_03"

#define TEST_NUM1   (ACS_GIC_TEST_NUM_BASE + 23)
#define TEST_DESC1  "Verify S-EL2 CNTHVS PPI mapping       "
#define TEST_RULE1  "B_PPI_03"

static
void
payload_get_cnthps_intid(void)
{
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint64_t intid = 0;
    uint64_t ret;
    uint64_t gicr_base;
    uint32_t s_el2;

    /* Check if Secure EL2 is implemented. */
    s_el2 = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 36, 39);
    if (!s_el2) {
        val_print(ERROR, "\n       Secure EL2 not implemented", 0);
        val_set_status(index, RESULT_SKIP(1));
        return;
    }
    /* Get GIC Redistributor base for the current PE. */
    gicr_base = val_gic_get_pe_rdbase(val_pe_get_mpid());
    if (!gicr_base) {
        val_print(ERROR, "\n       Failed to obtain GICR base for current PE", 0);
        val_set_status(index, RESULT_SKIP(2));
        return;
    }
    /* Get CNTHPS PPI INTID through EL3. */
    ret = val_el3_get_cnthps_intid(TIMEOUT_MEDIUM, gicr_base, &intid);
    if (ret == ACS_SMC_UNK_RET) {
        val_set_status(index, RESULT_SKIP(1));
        return;
    }

    if (ret) {
        val_print(ERROR, "\n       Failed to get CNTHPS INTID via EL3", 0);
        val_set_status(index, RESULT_WARNING(1));
        return;
    }

    val_print(DEBUG, "\n       CNTHPS PPI INTID (reported) = %llu", (unsigned long long)intid);

    /* Check PPI/EPPI range */
    if ((intid < 16U || intid > 31U) && !val_gic_is_valid_eppi(intid)) {
        val_print(ERROR, "\n       CNTHPS INTID not a valid PPI/EPPI: %llu",
            (unsigned long long)intid);
        val_set_status(index, RESULT_FAIL(1));
        return;
    }

    if (intid != BSA_RECOMMENDED_CNTHPS_PPI) {
        val_print(WARN, "\n       PPI ID %d does not match the BSA recommended PPI ID %llu",
            (unsigned long long)intid, BSA_RECOMMENDED_CNTHPS_PPI);
        val_set_status(index, RESULT_WARNING(2));
        return;
    }

    val_set_status(index, RESULT_PASS);
}

static
void
payload_get_cnthvs_intid(void)
{
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint64_t intid = 0;
    uint64_t ret;
    uint64_t gicr_base;
    uint32_t s_el2;

    /* Check if Secure EL2 is implemented. */
    s_el2 = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 36, 39);
    if (!s_el2) {
        val_print(ERROR, "\n       Secure EL2 not implemented", 0);
        val_set_status(index, RESULT_SKIP(1));
        return;
    }
    /* Get GIC Redistributor base for the current PE. */
    gicr_base = val_gic_get_pe_rdbase(val_pe_get_mpid());
    if (!gicr_base) {
        val_print(ERROR, "\n       Failed to obtain GICR base for current PE", 0);
        val_set_status(index, RESULT_SKIP(2));
        return;
    }

    /* Get CNTHVS PPI INTID through EL3. */
    ret = val_el3_get_cnthvs_intid(TIMEOUT_MEDIUM, gicr_base, &intid);
    if (ret == ACS_SMC_UNK_RET) {
        val_set_status(index, RESULT_SKIP(1));
        return;
    }
    if (ret) {
        val_print(ERROR, "\n       Failed to get CNTHVS INTID via EL3", 0);
        val_set_status(index, RESULT_WARNING(1));
        return;
    }

    val_print(DEBUG, "\n       CNTHVS PPI INTID (reported) = %llu", (unsigned long long)intid);

    /* Check PPI/EPPI range */
    if ((intid < 16U || intid > 31U) && !val_gic_is_valid_eppi(intid)) {
        val_print(ERROR, "\n       CNTHVS INTID not a valid PPI/EPPI: %llu",
            (unsigned long long)intid);
        val_set_status(index, RESULT_FAIL(1));
        return;
    }

    if (intid != BSA_RECOMMENDED_CNTHVS_PPI) {
        val_print(WARN, "\n       PPI ID %d does not match the BSA recommended PPI ID %llu",
            (unsigned long long)intid, BSA_RECOMMENDED_CNTHVS_PPI);
        val_set_status(index, RESULT_WARNING(2));
        return;
    }

    val_set_status(index, RESULT_PASS);
}

uint32_t
g018_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    /* This PPI test runs on a single processor */
    num_pe = 1;
    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload_get_cnthps_intid, 0);

    /* Collect results */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);
    return status;
}

uint32_t
g019_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    /* This PPI test runs on a single processor */
    num_pe = 1;
    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM1, TEST_DESC1, num_pe);
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM1, num_pe, payload_get_cnthvs_intid, 0);

    /* Collect results */
    status = val_check_for_error(TEST_NUM1, num_pe, TEST_RULE1);

    val_report_status(0, ACS_END(TEST_NUM1), TEST_RULE1);
    return status;
}

