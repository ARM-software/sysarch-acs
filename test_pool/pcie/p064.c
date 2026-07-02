/** @file
 * Copyright (c) 2019-2026, Arm Limited or its affiliates. All rights reserved.
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
#include "acs_pcie.h"
#include "acs_pe.h"
#include "acs_memory.h"

#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 64)
#define TEST_DESC  "Check ARI forwarding support rule     "
#define TEST_RULE  "IE_REG_4"
#define ARI_FW_STATUS(val) ((val) ? "supports ARI Forwarding" : "does not support ARI Forwarding")

static
void
payload(void)
{

  uint32_t ep_bdf;
  uint32_t rp_bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t reg_value;
  uint32_t dp_type;
  uint32_t rp_cap_base;
  uint32_t ep_cap_base;
  uint32_t rp_ari_support;
  uint32_t test_fails;
  uint32_t test_status = TEST_SKIP;
  uint32_t ep_ari_support = 1;
  pcie_device_bdf_table *bdf_tbl_ptr;
  uint32_t reg_overwrite_value;
  uint32_t afs_mask;
  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  test_fails = 0;

  /* Check for all the function present in bdf table */
  for (tbl_index = 0; tbl_index < bdf_tbl_ptr->num_entries; tbl_index++)
  {
      ep_bdf = bdf_tbl_ptr->device[tbl_index].bdf;
      dp_type = val_pcie_device_port_type(ep_bdf);
      ep_ari_support = 1;
      /* Check entry is iEP */
      if (dp_type == iEP_EP)
      {
          val_print(DEBUG, "\n       BDF - 0x%x", ep_bdf);
          /* Get the rootport of ARI device */
          rp_bdf = bdf_tbl_ptr->device[tbl_index].rp_bdf;

          val_pcie_disable_eru(rp_bdf);

          /* Check ARI capability support */
          if (val_pcie_find_capability(ep_bdf, PCIE_ECAP, ECID_ARICS, &ep_cap_base) ==
              PCIE_CAP_NOT_FOUND)
              ep_ari_support = 0;

          /* Derive bit-field of interest from the register value */
          val_pcie_find_capability(rp_bdf, PCIE_CAP, CID_PCIECS, &rp_cap_base);
          val_pcie_read_cfg(rp_bdf, rp_cap_base + DCAP2R_OFFSET, &reg_value);

          rp_ari_support = (reg_value >> DCAP2R_AFS_SHIFT) & DCAP2R_AFS_MASK;
          /* If test runs for atleast an endpoint */
          test_status = TEST_START;

          afs_mask = DCAP2R_AFS_MASK << DCAP2R_AFS_SHIFT;
          reg_overwrite_value = reg_value ^ afs_mask;

          /* Bit 5 (AFS) is RO: attempt to toggle and ensure it remains unchanged */
          val_pcie_write_cfg(rp_bdf, rp_cap_base + DCAP2R_OFFSET, reg_overwrite_value);
          val_pcie_read_cfg(rp_bdf, rp_cap_base + DCAP2R_OFFSET, &reg_overwrite_value);

          if ((reg_overwrite_value & afs_mask) != (reg_value & afs_mask)) {
            val_print(ERROR,
                      "\n       RP_BDF - 0x%x Bit 5 (AFS) should be read-only ", rp_bdf);
            test_fails++;
            /* Restore original value in case the write was accepted */
            val_pcie_write_cfg(rp_bdf, rp_cap_base + DCAP2R_OFFSET, reg_value);
          }

          /* If endpoint supports ARI, AFS must be 1 */
          if ((ep_ari_support == 1) && (rp_ari_support == 0)) {
            val_print(ERROR,
                        "\n     RP_BDF 0x%x Bit 5 (AFS) should be 1 when EP supports ARI",
                        rp_bdf);
            test_fails++;
          }
          else if (rp_ari_support !=  ep_ari_support) {
            val_print(WARN, "\n      Endpoint 0x%0x %s Root Port 0x%x %s ",
                        ep_bdf, ARI_FW_STATUS(ep_ari_support),
                        rp_bdf, ARI_FW_STATUS(rp_ari_support));
          }

      }
  }

  if (test_status == TEST_SKIP) {
      val_print(DEBUG,
                "\n       No iEP_EP Found. Skipping test");
      val_set_status(pe_index, RESULT_SKIP(01));
  }
  else if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(test_fails));
  else
      val_set_status(pe_index, RESULT_PASS);
}

uint32_t
p064_entry(uint32_t num_pe)
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
