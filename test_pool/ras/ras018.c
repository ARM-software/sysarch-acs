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
#include "val/include/acs_pe.h"
#include "val/include/val_interface.h"
#include "val/include/acs_memory.h"
#include "val/include/acs_mpam.h"
#include "val/include/acs_ras.h"

#define TEST_NUM (ACS_RAS_TEST_NUM_BASE + 18)
#define TEST_RULE "KBRZG"
#define TEST_DESC "Data abort on containable device err   "

static uint32_t esr_pending = 1;
static void *branch_to_test;

static
void
esr(uint64_t interrupt_type, void *context)
{
    esr_pending = 0;

    /* Update the ELR to return to test specified address */
    val_pe_update_elr(context, (uint64_t)branch_to_test);

    val_print(ACS_PRINT_ERR, "\n       Received exception of type: 0x%llx", interrupt_type);
}

static
void
payload()
{
    uint64_t num_node;
    uint64_t intf_type;
    uint64_t dev_addr;
    uint64_t attr = 0;
    uint32_t read_data;

    uint32_t status;
    uint32_t fail_cnt = 0;
    uint32_t test_skip = 0;
    uint32_t node_index;
    uint32_t usable_node_cnt = 0;
    uint64_t aderr = 0;
    uint64_t num_err;

    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    RAS_ERR_IN_t err_in_params;
    RAS_ERR_OUT_t err_out_params;

    /* Read ID_AA64MMFR3_EL1.ADERR[59:56] == 0b0010 or 0b0011 indicate FEAT_ADERR support */
    aderr = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64MMFR3_EL1), 56, 59);

    val_print(ACS_PRINT_INFO, "\n       ID_AA64MMFR3_EL1.ADERR field = 0x%llx", aderr);

    if (aderr == FEAT_ADERR_VAL2 || aderr == FEAT_ADERR_VAL3) {
        val_print(ACS_PRINT_INFO, "\n       FEAT_ADERR implemented.", 0);
        val_set_status(index, RESULT_PASS(TEST_NUM, 01));
        return;
    }

    val_print(ACS_PRINT_INFO,
          "\n       FEAT_ADERR not implemented. "
          "Proceeding to synchronous Data Abort test.",
          0);

    /* get number of nodes with RAS functionality */
    status = val_ras_get_info(RAS_INFO_NUM_NODES, 0, &num_node);
    if (status || (num_node == 0)) {
        val_print(ACS_PRINT_ERR,
          "\n       RAS nodes not found. "
          "Firmware interface is missing. Please conduct a paper-based analysis.",
          0);
        val_set_status(index, RESULT_WARN(TEST_NUM, 01));
        return;
    }

    for (node_index = 0; node_index < num_node; node_index++) {

        esr_pending = 1;

        /* MMIO-only filter */
        status = val_ras_get_info(RAS_INFO_INTF_TYPE, node_index, &intf_type);
        status |= val_ras_get_info(RAS_INFO_NUM_ERR_REC, node_index, &num_err);

        if (status || (intf_type != RAS_INTERFACE_MMIO) || (num_err == 0)) {
            val_print(ACS_PRINT_DEBUG, "\n       intf_type: 0x%llx", intf_type);
            val_print(ACS_PRINT_DEBUG, "\n       num_err: 0x%llx", num_err);
            continue;
        }

        usable_node_cnt++;

        dev_addr = val_memory_get_addr(MEM_TYPE_DEVICE, 0, &attr);

        if (!dev_addr) {
            val_print(ACS_PRINT_ERR,
                      "\n       Failed to obtain Device memory address.\n", 0);
            val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
            return;
        }

        val_print(ACS_PRINT_INFO,
                  "\n       Using Device memory address: 0x%llx\n", dev_addr);

        /* Install sync and async handlers to handle exceptions.*/
        status = val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
        status |= val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);
        if (status) {
            val_print(ACS_PRINT_ERR, "\n      Failed in installing the exception handler", 0);
            val_set_status(index, RESULT_FAIL(TEST_NUM, 02));
            return;
        }
        branch_to_test = &&exception_return;

        /* Inject error with following parameters */
        err_in_params.rec_index = 0; /* not applicable for scenario*/
        err_in_params.node_index = node_index;
        err_in_params.ras_error_type = ERR_CONTAINABLE; /* containable error */
        err_in_params.error_pa = dev_addr;              /* address of the location where error
                                                               needs to be injected */
        err_in_params.is_pfg_check = 0;                 /* not a pseudo fault check */

        /* Setup error in an implementation defined way */
        status = val_ras_setup_error(err_in_params, &err_out_params);
        if (status) {
            val_print(ACS_PRINT_ERR, "\n       val_ras_setup_error failed, node %d", node_index);
            fail_cnt++;
            break;
        }

        /* Inject error in an implementation defined way.
           Inject error at an address, which will cause system to
           record the error on reading with address syndrome in one of
           the error records present for the current RAS node */
        status = val_ras_inject_error(err_in_params, &err_out_params);
        if (status) {
            val_print(ACS_PRINT_ERR, "\n       val_ras_inject_error failed, node %d", node_index);
            fail_cnt++;
            break;
        }

        /* wait loop to allow system to inject the error */
        val_ras_wait_timeout(10);

        /* Perform a read to error-injected address, which will cause
         * system to record the error with address syndrome in one of
         * the error records present for the current RAS node */
        read_data = val_mmio_read(dev_addr);
        val_print(ACS_PRINT_DEBUG, "\n       Error injected address: 0x%llx", dev_addr);
        val_print(ACS_PRINT_DEBUG, "\n       Data read: 0x%lx", read_data);

    exception_return:
        val_print(ACS_PRINT_INFO, "\n       value esr_pending, %d", esr_pending);
        /* Check that a synchronous Data Abort was received */
        if (esr_pending) {
            val_print(ACS_PRINT_DEBUG, "\n       Data abort Check Fail, for node %d", node_index);
            fail_cnt++;
            continue;
        }
    }

    if (usable_node_cnt == 0) {
        val_print(ACS_PRINT_INFO,
                  "\n       No usable MMIO RAS nodes -- skipping test.\n", 0);
        val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
        return;
    }

    if (fail_cnt) {
        val_set_status(index, RESULT_FAIL(TEST_NUM, 03));
        return;
    }
    else if (test_skip) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 02));
        return;
    }

    val_set_status(index, RESULT_PASS(TEST_NUM, 02));
}

uint32_t
ras018_entry(uint32_t num_pe)
{

    uint32_t status = ACS_STATUS_FAIL;

    num_pe = 1; /* This test is run on single processor */

    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}
