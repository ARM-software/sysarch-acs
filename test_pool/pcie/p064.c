/** @file
 * Copyright (c) 2019-2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/acs_memory.h"

#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 64)
#define TEST_DESC  "Check ARI forwarding support rule     "
#define TEST_RULE  "PCI_IN_17"

static
void
payload(void)
{

  uint32_t bdf;
  uint32_t rp_bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t reg_value;
  uint32_t dp_type;
  uint32_t cap_base;
  uint32_t ari_frwd_support;
  uint32_t test_fails;
  uint32_t test_skip = 1;
  pcie_device_bdf_table *bdf_tbl_ptr;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  test_fails = 0;

  /* Check for all the function present in bdf table */
  for (tbl_index = 0; tbl_index < bdf_tbl_ptr->num_entries; tbl_index++)
  {
      bdf = bdf_tbl_ptr->device[tbl_index].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      /* Check entry is iEP */
      if (dp_type == iEP_EP)
      {
          val_print(ACS_PRINT_DEBUG, "\n       BDF - 0x%x", bdf);
          /* Check ARI capability support */
          if (val_pcie_find_capability(bdf, PCIE_ECAP, ECID_ARICS, &cap_base) ==
              PCIE_CAP_NOT_FOUND)
              continue;

          /* Get the rootport of ARI device */
          rp_bdf = bdf_tbl_ptr->device[tbl_index].rp_bdf;

          /* Read the ARI forwarding bit */
          val_pcie_find_capability(rp_bdf, PCIE_CAP, CID_PCIECS, &cap_base);
          val_pcie_read_cfg(rp_bdf, cap_base + DCAP2R_OFFSET, &reg_value);
          ari_frwd_support = (reg_value >> DCAP2R_AFS_SHIFT) & DCAP2R_AFS_MASK;

          /* If test runs for atleast an endpoint */
          test_skip = 0;

          /* If root port not support ARI forwarding, fail the test */
          if (!ari_frwd_support) {
            val_print(ACS_PRINT_ERR, "\n       BDF - 0x%x does not support ARI Forwarding. ", bdf);
            test_fails++;
          }

      }
  }

  if (test_skip == 1) {
      val_print(ACS_PRINT_DEBUG,
                "\n       No iEP_EP found with ARI Capability Support. Skipping test", 0);
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
  }
  else if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t
p064_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
