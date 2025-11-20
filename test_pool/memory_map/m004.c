/** @file
 * Copyright (c) 2016-2018, 2021, 2023-2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/val_interface.h"

#define TEST_NUM   (ACS_MEMORY_MAP_TEST_NUM_BASE + 4)
#define TEST_RULE  "B_MEM_03"
#define TEST_DESC  "Check Addressability                  "

#define TEST_NUM1   (ACS_MEMORY_MAP_TEST_NUM_BASE + 6)
#define TEST_RULE1  "B_MEM_04"
#define TEST_DESC1  "Check Addressability when SMMU's off  "

#define TEST_NUM2   (ACS_MEMORY_MAP_TEST_NUM_BASE + 7)
#define TEST_RULE2  "B_MEM_06"
#define TEST_DESC2  "Check Addressability for non-DMA dev  "

/* This payload targets peripherals (PCIe) subset of Non-secure on-chip DMA requesters in a
base system for the capability of addressing all of the Non-secure address space (i.e, 64 bit
while writing the test). And know ways were by PCIe 64 bit DMA or placing device behind an SMMU. */
static
void
check_peripheral_dma_capability (void)
{
  uint32_t i, fail_cnt = 0;
  uint32_t index = val_pe_get_index_mpid (val_pe_get_mpid());
  pcie_bdf_list_t *pcie_peripherals_bdf_list = val_pcie_get_pcie_peripheral_bdf_list();

  if (pcie_peripherals_bdf_list == NULL || pcie_peripherals_bdf_list->count == 0) {
      val_print(ACS_PRINT_DEBUG, "\n       Skip as no peripherals detected   ", 0);
      val_set_status(index, RESULT_SKIP (TEST_NUM, 1));
      return;
  }

  /* Check if a device is capable of accessing non secure address */
  for (i = 0; i < pcie_peripherals_bdf_list->count; i++) {
      /* Fail the test if device isn't capable of DMA access */
      if (!(val_pcie_is_devicedma_64bit(pcie_peripherals_bdf_list->dev_bdfs[i]) ||
            val_pcie_is_device_behind_smmu(pcie_peripherals_bdf_list->dev_bdfs[i]))) {
          val_print(ACS_PRINT_DEBUG, "\n       Failed for BDF = 0x%x",
                    pcie_peripherals_bdf_list->dev_bdfs[i]);
          fail_cnt++;
      }
      /* test can't be skipped as it's required by all devices irrespective
         how they are configured to have DMA */
  }

  if (fail_cnt) {
      val_set_status (index, RESULT_FAIL (TEST_NUM, 01));
  } else {
      val_set_status (index, RESULT_PASS (TEST_NUM, 01));
  }

}

/* This payload verifies that when a device's DMA transactions pass through an SMMU,
it can still access the entire Non-secure address space even if the SMMU is disabled.
We're testing PCIe peripherals behind the SMMU to ensure they can perform 64-bit DMA
as if the SMMU were not present. */
static
void
payload_check_dev_dma_if_behind_smmu (void)
{
  uint32_t i, fail_cnt = 0;
  bool test_run = 0;
  uint32_t index = val_pe_get_index_mpid (val_pe_get_mpid());
  pcie_bdf_list_t *pcie_peripherals_bdf_list = val_pcie_get_pcie_peripheral_bdf_list();

  if (pcie_peripherals_bdf_list == NULL || pcie_peripherals_bdf_list->count == 0) {
      val_print(ACS_PRINT_DEBUG, "\n       Skip as no peripherals detected   ", 0);
      val_set_status(index, RESULT_SKIP (TEST_NUM1, 1));
      return;
  }

  /* Check if a device is capable of accessing non secure address */
  for (i = 0; i < pcie_peripherals_bdf_list->count; i++) {
      /* Fail the test if device isn't capable of 64 bit DMA access without SMMU's help */
      if (val_pcie_is_device_behind_smmu(pcie_peripherals_bdf_list->dev_bdfs[i])) {
          /* Flag if test is run, since check is conditional on availability of
          device behind a SMMU */
          test_run = 1;
          if (!(val_pcie_is_devicedma_64bit(pcie_peripherals_bdf_list->dev_bdfs[i]))) {
            val_print(ACS_PRINT_DEBUG, "\n       Failed for BDF = 0x%x",
                      pcie_peripherals_bdf_list->dev_bdfs[i]);
          fail_cnt++;
          }
      }
  }

  if (!test_run) {
      val_set_status (index, RESULT_SKIP (TEST_NUM1, 02));
  } else if (fail_cnt) {
      val_set_status (index, RESULT_FAIL (TEST_NUM1, 01));
  } else {
      val_set_status (index, RESULT_PASS (TEST_NUM1, 01));
  }
}

/* This payload verifies that PCIe peripherals without DMA capability are located behind an SMMU,
as required. This test scenario is for all Non-secure off-chip devices but in here we are
targeting PCIe ones with intent of covering a subset, since discovery of off-chip can't be done
in generic way */
static
void
payload_check_if_non_dma_dev_behind_smmu (void)
{
  uint32_t i, fail_cnt = 0;
  bool test_run = 0;
  uint32_t index = val_pe_get_index_mpid (val_pe_get_mpid());
  pcie_bdf_list_t *pcie_peripherals_bdf_list = val_pcie_get_pcie_peripheral_bdf_list();

  if (pcie_peripherals_bdf_list == NULL || pcie_peripherals_bdf_list->count == 0) {
      val_print(ACS_PRINT_DEBUG, "\n       Skip as no peripherals detected   ", 0);
      val_set_status(index, RESULT_SKIP (TEST_NUM2, 1));
      return;
  }

  /* Check if a device is capable of accessing non secure address */
  for (i = 0; i < pcie_peripherals_bdf_list->count; i++) {
      /* Test is valid for devices which can't DMA and fail if such
        device not behind a SMMU */
      if (!(val_pcie_is_devicedma_64bit(pcie_peripherals_bdf_list->dev_bdfs[i]))) {
          /* Flag if test is run, since check is conditional on availability of
            non DMA devices */
          test_run = 1;
          /* Mark has fail if non DMA device not behind SMMU */
          if (!(val_pcie_is_device_behind_smmu(pcie_peripherals_bdf_list->dev_bdfs[i]))) {
              val_print(ACS_PRINT_DEBUG, "\n       Failed for BDF = 0x%x",
                        pcie_peripherals_bdf_list->dev_bdfs[i]);
              fail_cnt++;
          }
      }
  }

  if (!test_run) {
      val_set_status (index, RESULT_SKIP (TEST_NUM2, 02));
  } else if (fail_cnt) {
      val_set_status (index, RESULT_FAIL (TEST_NUM2, 01));
  } else {
      val_set_status (index, RESULT_PASS (TEST_NUM2, 01));
  }
}

uint32_t
m004_entry (uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  /* This test is run on single processor */
  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test (TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP) {
      val_run_test_payload (TEST_NUM, num_pe, check_peripheral_dma_capability, 0);
  }
  /* Get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);
  return status;
}

uint32_t
m006_entry (uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  /* This test is run on single processor */
  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test (TEST_NUM1, TEST_DESC1, num_pe);
  if (status != ACS_STATUS_SKIP) {
      val_run_test_payload (TEST_NUM1, num_pe, payload_check_dev_dma_if_behind_smmu, 0);
  }
  /* Get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM1, num_pe, TEST_RULE1);
  val_report_status(0, ACS_END(TEST_NUM1), NULL);
  return status;
}

uint32_t
m007_entry (uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  /* This test is run on single processor */
  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test (TEST_NUM2, TEST_DESC2, num_pe);
  if (status != ACS_STATUS_SKIP) {
      val_run_test_payload (TEST_NUM2, num_pe, payload_check_if_non_dma_dev_behind_smmu, 0);
  }
  /* Get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM2, num_pe, TEST_RULE2);
  val_report_status(0, ACS_END(TEST_NUM2), NULL);
  return status;
}
