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
#include "val/include/acs_pe.h"
#include "val/include/acs_memory.h"

#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 8)
#define TEST_RULE  "PCI_IN_16"
#define TEST_DESC  "Check all 1's for out of range        "

static void *branch_to_test;

static
void
esr(uint64_t interrupt_type, void *context)
{
  uint32_t pe_index;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Update the ELR to return to test specified address */
  val_pe_update_elr(context, (uint64_t)branch_to_test);

  val_print(ACS_PRINT_INFO, "\n       Received exception of type: %d", interrupt_type);
  val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 01));
}

static
void
payload(void)
{

  uint32_t data;
  uint32_t num_ecam;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t bdf = 0;
  uint32_t bus, segment;
  uint32_t end_bus;
  uint32_t bus_index;
  uint32_t dev_index;
  uint32_t func_index;
  uint32_t ret;
  uint32_t status;

  /* Install sync and async handlers to handle exceptions.*/
  status = val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
  status |= val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);
  if (status)
  {
      val_print(ACS_PRINT_ERR, "\n       Failed in installing the exception handler", 0);
      val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
      return;
  }

  branch_to_test = &&exception_return;

  num_ecam = val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0);

  if (num_ecam == 0) {
      val_print(ACS_PRINT_DEBUG, "\n       No ECAM in MCFG. Skipping test               ", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
      return;
  }

  while (num_ecam) {
      num_ecam--;
      segment = val_pcie_get_info(PCIE_INFO_SEGMENT, num_ecam);
      bus = val_pcie_get_info(PCIE_INFO_START_BUS, num_ecam);
      end_bus = val_pcie_get_info(PCIE_INFO_END_BUS, num_ecam);

      /* Accessing the BDF PCIe config range */
      for (bus_index = bus; bus_index <= end_bus; bus_index++) {
        for (dev_index = 0; dev_index < PCIE_MAX_DEV; dev_index++) {
          for (func_index = 0; func_index < PCIE_MAX_FUNC; func_index++) {

               bdf = PCIE_CREATE_BDF(segment, bus_index, dev_index, func_index);
               ret = val_pcie_read_cfg(bdf, TYPE01_VIDR, &data);

               //If this is really PCIe CFG space, Device ID and Vendor ID cannot be 0
               if (ret == PCIE_NO_MAPPING || (data == 0)) {
                  val_print(ACS_PRINT_ERR, "\n       Incorrect data at ECAM Base %4x    ", data);
                  val_print(ACS_PRINT_ERR, "\n       BDF is  %x    ", bdf);
                  val_set_status(index, RESULT_FAIL(TEST_NUM,
                                  (bus_index << PCIE_BUS_SHIFT)|dev_index));
                  return;
               }

               /* Access the start and end of the config space for PCIe devices whose
                  device ID and vendor ID are all FF's */
               if (data == PCIE_UNKNOWN_RESPONSE)
               {
                  val_pcie_read_cfg(bdf, PCIE_ECAP_START, &data);

                  /* Returned data must be FF's, otherwise the test must fail */
                  if (data != PCIE_UNKNOWN_RESPONSE) {
                     val_print(ACS_PRINT_ERR, "\n       Incorrect data for Bdf 0x%x    ", bdf);
                     val_set_status(index, RESULT_FAIL(TEST_NUM,
                                     (bus_index << PCIE_BUS_SHIFT)|dev_index));
                     return;
                  }
               }
          }
        }
      }
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 01));

exception_return:
  return;
}

uint32_t
p008_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from single PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
