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

#define TEST_NUM  (ACS_PE_TEST_NUM_BASE + 66)
#define TEST_RULE "XRPZG"
#define TEST_DESC "Check num of Breakpoints and type      "

static
void
payload()
{
    uint64_t dfr0 = 0, dfr1 = 0;
    uint64_t dfr0_brps = 0, dfr1_brps = 0;
    uint32_t dfr0_ctx = 0, dfr1_ctx = 0;
    uint32_t ctx_start = 0, ctx_end = 0;
    int32_t breakpointcount;
    uint32_t context_aware_breakpoints = 0;
    uint32_t pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

    dfr0 = val_pe_reg_read(ID_AA64DFR0_EL1);
    dfr1 = val_pe_reg_read(ID_AA64DFR1_EL1);

    /* bits 15:12 for Number of breakpoints - 1 */
    dfr0_brps = VAL_EXTRACT_BITS(dfr0, 12, 15);
    dfr1_brps = VAL_EXTRACT_BITS(dfr1, 12, 15);

    /* If DFR0 reports maximum (0xF) and DFR1 is non-zero,
     *   Total breakpoints = (DFR1 + 1)
     *   Example: DFR0=0xF, DFR1=0x2 -> 16 + 3 = 19
     * Else
     *   Total breakpoints = DFR0 + 1
     */
    breakpointcount = (dfr0_brps == 0xF && dfr1_brps != 0) ?
                  (dfr1_brps + 1) : (dfr0_brps + 1);

    if (breakpointcount < 6)
    {
        val_print_primary_pe(ACS_PRINT_ERR,
                             "\n       Number of PE breakpoints reported: %d, expected >= 6",
                             breakpointcount, pe_index);
        val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 1));
        return;
    }

    /*bits [31:28] Number of breakpoints that are context-aware, minus 1*/
    dfr0_ctx = VAL_EXTRACT_BITS(dfr0, 28, 31);
    dfr1_ctx = VAL_EXTRACT_BITS(dfr1, 28, 31);

    /* If CTX_CMP field in DFR0 is not 0xF,
     *   Context-aware breakpoints = CTX_CMP + 1
     * Else (when DFR0 reports 0xF),
     *   If DFR1.CTX_CMP == 0 -> 16 context-aware breakpoints
     *   Else -> (DFR1.CTX_CMP + 1)
     */
    if (dfr0_ctx != 0xF)
        context_aware_breakpoints = dfr0_ctx + 1;
    else
        context_aware_breakpoints = (dfr1_ctx == 0) ? 16 : (dfr1_ctx + 1);

    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       Total Breakpoints          : %u",
                         breakpointcount, pe_index);
    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       Context-Aware Breakpoints  : %u",
                         context_aware_breakpoints, pe_index);

    if (context_aware_breakpoints < 2)
    {
        val_print_primary_pe(ACS_PRINT_ERR,
                             "\n       Context-aware breakpoints reported: %u, expected >= 2",
                             context_aware_breakpoints, pe_index);
        val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 2));
        return;
    }

    if (breakpointcount <= 16) {
        /* Highest-numbered breakpoints are context-aware */
        ctx_start = breakpointcount - context_aware_breakpoints;
        ctx_end = breakpointcount - 1;
    } else if (context_aware_breakpoints <= 16) {
        /* Top 16 slots used when >16 BRPs */
        ctx_start = 16 - context_aware_breakpoints;
        ctx_end = 15;
    } else {
        /* More than 16 context-aware BPs -> start from BP[0] */
        ctx_start = 0;
        ctx_end = context_aware_breakpoints - 1;
    }

    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       Context-Aware BP range st     : BP[%u]",
                ctx_start, pe_index);
    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       Context-Aware BP range end     : BP[%u]",
                ctx_end, pe_index);

    /* Breakpoints 4 and 5 must be context-aware */
    if (ctx_start <= 4 && ctx_end >= 5)
        val_set_status(pe_index, RESULT_PASS(TEST_NUM, 1));
    else
    {
        val_print_primary_pe(ACS_PRINT_ERR,
                "\n       Breakpoints 4 and 5 are not context-aware as required", 0, pe_index);
        val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 3));
    }
}

/**
  @brief   Check for the number of breakpoints available
**/
uint32_t
pe066_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);

    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
    if (status != ACS_STATUS_SKIP)
        /* execute payload on present PE and then execute on other PE */
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

    val_report_status(0, ACS_END(TEST_NUM), NULL);

    return status;
}
