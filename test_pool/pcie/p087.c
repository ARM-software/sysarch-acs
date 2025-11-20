/** @file
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/val_interface.h"
#include "val/include/acs_pcie.h"
#include "val/include/acs_pe.h"


#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 87)
#ifdef PC_BSA
#define TEST_RULE  "P_L1PCI_2"
#else
#define TEST_RULE  "S_L4PCI_2"
#endif
#define TEST_DESC  "Check EA Capability                   "

static
void
payload(void)
{

  uint64_t num_ecam;
  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t reg_value;
  uint32_t enable_value;
  uint32_t test_fails;
  uint32_t test_skip = 1;
  uint32_t cap_base;
  uint32_t status;
  uint32_t entry, num_entries;
  uint32_t entry_size;
  uint32_t entry_type_offset;
  pcie_device_bdf_table *bdf_tbl_ptr;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  tbl_index = 0;
  test_fails = 0;

  num_ecam = val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0);

  if (num_ecam == 0) {
      val_print(ACS_PRINT_ERR, "\n       No ECAMs discovered              ", 0);
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
      return;
  }

  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      val_print(ACS_PRINT_DEBUG, "\n       BDF - 0x%x", bdf);

      /* If test runs for atleast an endpoint */
      test_skip = 0;

      /* Retrieve the addr of Enhanced Allocation capability (14h) and check if the
       * capability structure is not supported. */
      status = val_pcie_find_capability(bdf, PCIE_CAP, CID_EA, &cap_base);
      if (status == PCIE_CAP_NOT_FOUND)
          continue;

      /* Read number of entries in the EA Cap structure */
      val_pcie_read_cfg(bdf, cap_base, &reg_value);
      num_entries = (reg_value >> EA_NUM_ENTRY_SHIFT) & EA_NUM_ENTRY_MASK;

      /* Move to next bdf if the EA Cap structure has no entries */
      if (!num_entries)
          continue;

      /* First DW of the structure is common across all entires */
      entry_type_offset = PCIE_DWORD_SIZE;

      /* Type 1 functions implement additional DW after the fist DW - Bus Numbers register */
      if (val_pcie_function_header_type(bdf) == TYPE1_HEADER)
          entry_type_offset += PCIE_DWORD_SIZE;

      for (entry = 0; entry < num_entries; entry++) {
          val_print(ACS_PRINT_DEBUG, "\n       Reading entry at offset %llx", entry_type_offset);

          /* Read Entry type register present in Enhanced Allocation capability struct(14h) */
          val_pcie_read_cfg(bdf, cap_base + entry_type_offset, &reg_value);

          /* Extract enable value */
          enable_value = (reg_value >> EA_ENTRY_TYPE_ENABLE_SHIFT) & EA_ENTRY_TYPE_ENABLE_MASK;
          if (enable_value)
          {
              val_print(ACS_PRINT_ERR, "\n       Enhanced Allocation enabled for BDF 0x%x", bdf);
              test_fails++;
          }

          entry_size = (reg_value & EA_ENTRY_TYPE_SIZE_MASK);

          /* Skip Base and Max registers and move to next entry */
          entry_type_offset += ((entry_size + 1) * PCIE_DWORD_SIZE);
      }
  }

  if (test_skip == 1) {
      val_print(ACS_PRINT_DEBUG,
               "\n       Found no Endpoint with PCIe Capability. Skipping test", 0);
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
  }
  else if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t
p087_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
