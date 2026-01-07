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

#include "val/include/acs_val.h"
#include "val/include/acs_common.h"
#include "val/include/val_interface.h"
#include "val/include/acs_memory.h"
#include "val/include/acs_pe.h"
#include "val/include/acs_ras.h"

#define TEST_NUM (ACS_RAS_TEST_NUM_BASE + 16)
#define TEST_RULE "ZVDJG"
#define TEST_DESC "Check FEAT_RASSA_RV implementation     "

static
void
payload()
{
    uint32_t status;
    uint32_t fail_cnt = 0;
    uint64_t num_nodes;
    uint64_t regval;
    uint32_t node_index;
    uint32_t ras_field;
    uint64_t rec_index;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    /* Check that the PE implements RAS System Architecture v2 */
    ras_field = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 28, 31);
    val_print(ACS_PRINT_DEBUG, "\n       ID_AA64PFR0_EL1.RAS : %x", ras_field);

    if (ras_field < RAS_VERSION_2)
    {
        val_print(ACS_PRINT_ERR, "\n       RASv2 not implemented (ID_AA64PFR0_EL1.RAS = 0x%x).",
                  ras_field);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
        return;
    }

    /* Get Number of nodes with RAS Functionality */
    status = val_ras_get_info(RAS_INFO_NUM_NODES, 0, &num_nodes);
    if (status || (num_nodes == 0))
    {
        val_print(ACS_PRINT_ERR, "\n       RAS Nodes not found. ", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 2));
        return;
    }

    for (node_index = 0; node_index < num_nodes; node_index++)
    {

        /* Get Error Record number for this Node */
        status = val_ras_get_info(RAS_INFO_START_INDEX, node_index, &rec_index);
        if (status) {
            val_print(ACS_PRINT_DEBUG, "\n       Could not get Start Index : %d", node_index);
            fail_cnt++;
            continue;
        }

        /* For each RAS node, read ERR<rec_index>FR and check RV bit */
        regval = val_ras_reg_read(node_index, RAS_ERR_FR, rec_index);
        if (regval == INVALID_RAS_REG_VAL)
        {
            val_print(ACS_PRINT_ERR,
                      "\n       Node %d: couldn't read ERR<0>FR register.",
                      node_index);
            fail_cnt++;
            continue;
        }

        /* RV field [28] must be nonzero */
        if (!(regval & ERR_FR_RV_MASK))
        {
            val_print(ACS_PRINT_ERR,
                      "\n       Node %d: FEAT_RASSA_RV not implemented.",
                      node_index);
            fail_cnt++;
            continue;
        }

        val_print(ACS_PRINT_INFO, "\n       Node %d: ", node_index);
        val_print(ACS_PRINT_INFO, "\n       FEAT_RASSA_RV implemented (ERR_FR = 0x%llx).", regval);
    }

    if (fail_cnt)
        val_set_status(index, RESULT_FAIL(TEST_NUM, 3));
    else
        val_set_status(index, RESULT_PASS(TEST_NUM, 1));
}

uint32_t
ras016_entry(uint32_t num_pe)
{

    uint32_t status = ACS_STATUS_FAIL;

    num_pe = 1; // This test is run on single processor

    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}
