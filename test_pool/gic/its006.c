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
#include "acs_iovirt.h"
#include "acs_pcie.h"

#define TEST_NUM   (ACS_GIC_ITS_TEST_NUM_BASE + 6)
#define TEST_RULE  "ITS_DEV_5"
#define TEST_DESC  "MSI-capable devices have DeviceID     "

static
void
payload(void)
{
  uint32_t bdf;
  uint32_t status;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t device_id;
  uint32_t stream_id;
  uint32_t its_id;
  uint32_t grp_id;
  uint32_t cap_base;
  uint32_t test_skip = 1;
  uint32_t test_fails = 0;
  pcie_device_bdf_table *bdf_tbl_ptr;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  if ((!bdf_tbl_ptr) || (!bdf_tbl_ptr->num_entries)) {
      val_print(DEBUG, "\n       No entries in BDF table");
      val_set_status(pe_index, RESULT_SKIP(1));
      return;
  }

  /* Check all enumerated PCIe functions that can originate MSI/MSI-X. */
  for (tbl_index = 0; tbl_index < bdf_tbl_ptr->num_entries; tbl_index++)
  {
      bdf = bdf_tbl_ptr->device[tbl_index].bdf;

      if ((val_pcie_find_capability(bdf, PCIE_CAP, CID_MSI, &cap_base) ==
           PCIE_CAP_NOT_FOUND) &&
          (val_pcie_find_capability(bdf, PCIE_CAP, CID_MSIX, &cap_base) ==
           PCIE_CAP_NOT_FOUND))
          continue;

      test_skip = 0;
      val_print(DEBUG, "\n       Checking DeviceID association for BDF : 0x%x", bdf);

      /* Every device that is expected to originate MSIs is associated with a DeviceID */
      status = val_iovirt_get_device_info(PCIE_CREATE_BDF_PACKED(bdf),
                                          PCIE_EXTRACT_BDF_SEG(bdf),
                                          &device_id, &stream_id, &its_id);
      if (status) {
          val_print(ERROR, "\n       DeviceID mapping not found for BDF : 0x%x", bdf);
          test_fails++;
          continue;
      }

      /* We get ITS ID associated with the device. Now check if ITS ID belongs to valid ITS grp */
      status = val_iovirt_get_its_info(ITS_GET_GRP_INDEX_FOR_ID, 0, its_id, &grp_id);
      if (status) {
          /* Not a failure acc to this rule but an IORT violation */
          val_print(ERROR, "\n       ITS group not found for BDF : 0x%x", bdf);
          val_print(ERROR, " ITS ID : 0x%x", its_id);
      }
  }

  /* Partially covered, because this test only checks PCIe devices that advertise MSI/MSI-X support.
     The rule, however, applies to all MSI-originating devices, not just PCIe devices. */
  if (test_skip)
      val_set_status(pe_index, RESULT_SKIP(2));
  else if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(test_fails));
  else
      val_set_status(pe_index, RESULT_PARTIAL_COVERED);

}

uint32_t
its006_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This ITS test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
