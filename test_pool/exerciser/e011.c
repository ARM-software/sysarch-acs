/** @file
 * Copyright (c) 2021, 2023-2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/acs_pcie.h"
#include "val/include/acs_memory.h"
#include "val/include/acs_iovirt.h"
#include "val/include/acs_smmu.h"
#include "val/include/acs_pcie_enumeration.h"
#include "val/include/acs_exerciser.h"

#define TEST_NUM   (ACS_EXERCISER_TEST_NUM_BASE + 11)
#define TEST_RULE  "ITS_03"
#define TEST_DESC  "MSI-capable device linked to ITS group"

static
void
payload (void)
{

  uint32_t index;
  uint32_t instance;
  uint32_t e_bdf = 0;
  uint32_t status;
  uint32_t num_instance, grp_id = 0;
  bool     test_skip = 1;
  uint32_t msi_cap_offset = 0;
  uint32_t device_id = 0;
  uint32_t stream_id = 0;
  uint32_t its_id = 0;

  index = val_pe_get_index_mpid (val_pe_get_mpid());

  if (val_gic_get_info(GIC_INFO_NUM_ITS) == 0) {
      val_print(ACS_PRINT_DEBUG, "\n       Skipping Test as GIC ITS not available", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM, 1));
      return;
  }

  /* Disable all SMMUs */
  num_instance = val_iovirt_get_smmu_info(SMMU_NUM_CTRL, 0);
  for (instance = 0; instance < num_instance; ++instance)
      val_smmu_disable(instance);

  /* Read the number of excerciser cards */
  num_instance = val_exerciser_get_info(EXERCISER_NUM_CARDS);

  for (instance = 0; instance < num_instance; instance++)
  {
      /* if init fail moves to next exerciser */
      if (val_exerciser_init(instance))
          continue;

      /* Get the exerciser BDF */
      e_bdf = val_exerciser_get_bdf(instance);
      val_print(ACS_PRINT_DEBUG, "\n       Exerciser BDF - 0x%x", e_bdf);

      /* Search for MSI-X Capability */
      if ((val_pcie_find_capability(e_bdf, PCIE_CAP, CID_MSIX, &msi_cap_offset)) &&
          (val_pcie_find_capability(e_bdf, PCIE_CAP, CID_MSI, &msi_cap_offset))) {
        val_print(ACS_PRINT_DEBUG, "\n       No MSI-X Capability, Skipping for 0x%x", e_bdf);
        continue;
      }

      test_skip = 0;

      /* Get DeviceID & ITS_ID for this device */
      status = val_iovirt_get_device_info(PCIE_CREATE_BDF_PACKED(e_bdf),
                                          PCIE_EXTRACT_BDF_SEG(e_bdf), &device_id,
                                          &stream_id, &its_id);
      if (status) {
          val_print(ACS_PRINT_ERR,
              "\n       Could not get device info for BDF : 0x%x", e_bdf);
          val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
          return;
      }

      /* Get ITS Group Index for current device */
      status = val_iovirt_get_its_info(ITS_GET_GRP_INDEX_FOR_ID, 0, its_id, &grp_id);
      if (status) {
          val_print(ACS_PRINT_ERR, "\n       Invalid ITS ID, Failed on BDF 0x%x", e_bdf);
          val_set_status(index, RESULT_FAIL(TEST_NUM, 2));
          return;
      }
  }

  if (test_skip)
      val_set_status(index, RESULT_SKIP(TEST_NUM, 2));
  else
      val_set_status(index, RESULT_PASS(TEST_NUM, 1));

}

uint32_t
e011_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  /* Run test on single PE */
  num_pe = 1;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP) {
      if (val_exerciser_test_init() != ACS_STATUS_PASS)
          return TEST_SKIP_VAL;
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);
  }

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
