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
#include "val/include/acs_pe.h"
#include "val/include/val_interface.h"
#include "val/include/acs_tpm.h"
#include "val/include/acs_mmu.h"

#define TEST_NUM   (ACS_TPM2_TEST_NUM_BASE  +  2)
#define TEST_RULE  "P_L1TP_03"
#define TEST_DESC  "Check TPM interface locality support  "

#define TPM_MMIO_MAP_SIZE   0x1000

static
void
payload()
{
    uint64_t tpm_present;
    uint64_t tpm_start_method;
    uint64_t tpm_base_addr;
    uint64_t interface_id_val;
    uint64_t interface_id_addr;
    uint32_t cap_locality;
    uint32_t pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

    /* Step 1: Check TPM presence */
    tpm_present = val_tpm2_get_info(TPM2_INFO_IS_PRESENT);
    if (tpm_present == 0) {
        val_print(ACS_PRINT_ERR, "\n       TPM not present", 0);
        val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 01));
        return;
    }

    /* Step 2: Get TPM interface type from ACPI TPM2 table */
    tpm_start_method = val_tpm2_get_info(TPM2_INFO_INTERFACE_TYPE);
    val_print(ACS_PRINT_INFO, "\n       TPM interface type: 0x%llx", tpm_start_method);


    /* Step 3: Reject interfaces that are known to support only locality 0 */
    if (tpm_start_method == TPM_IF_START_METHOD_ACPI ||
        tpm_start_method == TPM_IF_START_METHOD_CRB_SMC ||
        tpm_start_method == TPM_IF_START_METHOD_CRB_FFA) {
        val_print(ACS_PRINT_INFO,
          "\n       Skipping test: TPM locality not accessible at current privilege level", 0);
        val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
        return;
    }

    /* Step 4: Get TPM base address from ACPI TPM2 table */
    tpm_base_addr = val_tpm2_get_info(TPM2_INFO_BASE_ADDR);
    val_print(ACS_PRINT_INFO, "\n       TPM Base Address: 0x%llx", tpm_base_addr);

    /* Step 5: Handle FIFO (TIS) interface */
    if (tpm_start_method == TPM_IF_START_METHOD_TIS) {

        /* Use default FIFO address if ACPI base is zero */
        if (tpm_base_addr == 0) {
            /*
             * TPM base address used here is 0xFED40000, which is the default
             * base address for TPM (Locality 0) as defined in the TCG PC Client
             * Platform TPM Profile Specification for TPM 2.0, Section 6.2.1.
             */
            tpm_base_addr = TPM_FIFO_BASE_ADDRESS;
            val_mmu_update_entry(tpm_base_addr, TPM_MMIO_MAP_SIZE);
            val_print(ACS_PRINT_WARN, "\n       ACPI base address is 0, "
                                      "Using default FIFO base: 0x%llx", tpm_base_addr);
        }

        interface_id_addr = tpm_base_addr + TPM_FIFO_INTERFACE_ID_OFFSET;
        val_print(ACS_PRINT_INFO, "\n       Tpm_fifo_interface_id_0 address: 0x%llx",
                                                                         interface_id_addr);

        interface_id_val = val_mmio_read64(interface_id_addr);
        val_print(ACS_PRINT_INFO, "\n       Tpm_fifo_interface_id_0 Value : 0x%llx",
                                                                         interface_id_val);

        cap_locality = VAL_EXTRACT_BITS(interface_id_val, 8, 8); /* CapLocality is bit[8] */
        if (cap_locality == 0) {
            val_print(ACS_PRINT_ERR, "\n       TPM FIFO interface supports only locality 0", 0);
            val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 03));
            return;
        }
    }

    /* Step 6: Handle CRB interface (with or without ACPI) */
    else if (tpm_start_method == TPM_IF_START_METHOD_CRB ||
             tpm_start_method == TPM_IF_START_METHOD_CRB_ACPI) {

        /* Base address in ACPI points to TPM_CRB_CTRL_REQ_0 so adjust to get CRB register base */
        val_mmu_update_entry((tpm_base_addr - TPM_CRB_CONTROL_AREA_OFFSET), TPM_MMIO_MAP_SIZE);
        interface_id_addr = (tpm_base_addr - TPM_CRB_CONTROL_AREA_OFFSET) +
                                             TPM_CRB_INTERFACE_ID_OFFSET;
         val_print(ACS_PRINT_INFO, "\n       Tpm_fifo_interface_id_0 address: 0x%llx",
                                                                         interface_id_addr);

        interface_id_val = val_mmio_read64(interface_id_addr);
        val_print(ACS_PRINT_INFO, "\n       Tpm_fifo_interface_id_0 Value : 0x%llx",
                                                                         interface_id_val);

        cap_locality = VAL_EXTRACT_BITS(interface_id_val, 8, 8); /* CapLocality is bit[8] */
        if (cap_locality == 0) {
            val_print(ACS_PRINT_ERR, "\n       TPM CRB interface supports only locality 0", 0);
            val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 04));
            return;
        }
    }

    /* Step 7: Catch any unknown/unhandled interface types */
    else {
        val_print(ACS_PRINT_ERR, "\n       Invalid TPM interface type per TPM2 spec", 0);
        val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 05));
        return;
    }

    /* Interface supports localities 0 - 4 */
    val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));
    return;
}


uint32_t
tpm002_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;
  num_pe = 1;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}

