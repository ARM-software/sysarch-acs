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

#define TEST_NUM   (ACS_RAS_TEST_NUM_BASE + 17)
#define TEST_RULE  "RKLPK"
#define TEST_DESC  "Check FEAT_RASSA_DFI and CED support  "

static
void
payload(void)
{
    uint32_t status;
    uint32_t fail_cnt = 0;
    uint64_t num_nodes;
    uint64_t regval, value, rec_index;
    uint64_t fi_field;
    uint32_t node_index;
    uint32_t ras_field;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    /* Check that the PE implements RAS System Architecture v2 */
    ras_field = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 28, 31);
    val_print(ACS_PRINT_DEBUG, "\n       ID_AA64PFR0_EL1.RAS : %x", ras_field);

    if (ras_field < RAS_VERSION_2) {
        val_print(ACS_PRINT_ERR,
                  "\n       RASv2 not implemented (ID_AA64PFR0_EL1.RAS = 0x%x).",
                  ras_field);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
        return;
    }

    /* Get number of RAS nodes */
    status = val_ras_get_info(RAS_INFO_NUM_NODES, 0, &num_nodes);
    if (status || (num_nodes == 0)) {
        val_print(ACS_PRINT_ERR, "\n       RAS Nodes not found. ", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 2));
        return;
    }

    for (node_index = 0; node_index < num_nodes; node_index++) {

        /* Get Node Type */
        status = val_ras_get_info(RAS_INFO_NODE_TYPE, node_index, &value);
        if (status) {
            val_print(ACS_PRINT_DEBUG, "\n       Node Type not found index %d", node_index);
            fail_cnt++;
            break;
        }

        /* Only Memory Controller or Processor (cache) nodes */
        if (!((value == NODE_TYPE_MC) || (value == NODE_TYPE_PE)))
            continue;

        if (value == NODE_TYPE_PE) {
            status = val_ras_get_info(RAS_INFO_PE_RES_TYPE, node_index, &value);
            if (status) {
                val_print(ACS_PRINT_DEBUG, "\n       PE Resource type not found index %d",
                            node_index);
                fail_cnt++;
                break;
            }
            if (value != 0)
                continue;
        }

        /* Get first record index for this node */
        status = val_ras_get_info(RAS_INFO_START_INDEX, node_index, &rec_index);
        if (status) {
            val_print(ACS_PRINT_DEBUG, "\n       Could not get Start Index for index %d",
                        node_index);
            fail_cnt++;
            continue;
        }

        /* Read ERR<start_rec_index>FR */
        regval = val_ras_reg_read(node_index, RAS_ERR_FR, rec_index);
        if (regval == INVALID_RAS_REG_VAL) {
            val_print(ACS_PRINT_ERR,
                      "\n       Couldn't read ERR<%d>FR register.", rec_index);
            fail_cnt++;
            continue;
        }

        /* CEC bits [14:12] determine whether CED is valid */
        if ((regval & ERR_FR_CEC_MASK) != 0) {
            /* CED field [30] must be nonzero */
            if ((regval & ERR_FR_CED_MASK) == 0) {
                val_print(ACS_PRINT_ERR,
                        "\n       Node %d: FEAT_RASSA_CED not implemented.",
                        node_index);
                fail_cnt++;
            }
        }

        /* FI bits [7:6] determine whether DFI is valid */
        fi_field = (regval & ERR_FR_FI_MASK) >> 6;

        if (fi_field == 2 || fi_field == 3) {
            /* DFI field [27:26] must be nonzero */
            if ((regval & ERR_FR_DFI_MASK) == 0) {
                val_print(ACS_PRINT_ERR,
                          "\n       Node %d: FEAT_RASSA_DFI not implemented (FR bits[27:26] == 0).",
                          node_index);
                fail_cnt++;
            }
        }
    }

    if (fail_cnt)
        val_set_status(index, RESULT_FAIL(TEST_NUM, 3));
    else
        val_set_status(index, RESULT_PASS(TEST_NUM, 1));
}

uint32_t
ras017_entry(uint32_t num_pe)
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

