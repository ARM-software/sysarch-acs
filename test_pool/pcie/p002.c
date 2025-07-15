/** @file
 * Copyright (c) 2016-2018, 2021-2025, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 2)
#define TEST_RULE  "PCI_IN_02"
#define TEST_DESC  "PE - ECAM Region accessibility check  "

/* Giving max VF as 256*/
#define MAX_VFS 256

static void *branch_to_test;

static uint32_t num_vf;
static uint32_t skip_rid_list[MAX_VFS];

static
void
esr(uint64_t interrupt_type, void *context)
{
  uint32_t pe_index;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Update the ELR to return to test specified address */
  val_pe_update_elr(context, (uint64_t)branch_to_test);

  val_print(ACS_PRINT_INFO, "\n       Received exception of type: %d", interrupt_type);
  val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 1));
}

/*
 * Calculate the list of Virtual Function (VF) BDFs for a given Physical Function (PF) using
 * the SR-IOV capability structure. Extract First VF Offset, VF Stride, and Number of VFs
 * to compute each VF's Routing ID, convert to BDF, and store them in the skip list.
 */
static void
calculate_vf(uint32_t seg, uint32_t bdf, uint32_t sriov_base)
{
  uint32_t vf_bdf;
  uint32_t first_vf_offset, vf_rid;
  uint32_t vf_stride;
  uint32_t index;
  uint32_t reg_value;
  uint32_t pf_rid = PCIE_CREATE_BDF_PACKED(bdf);

  val_pcie_read_cfg(bdf, sriov_base + SRIOV_VF_OFF_STR, &reg_value);
  first_vf_offset = reg_value & SRIOV_FIRST_VF_SHIFT;
  vf_stride = (reg_value >> SRIOV_STRIDE_SHIFT) & SRIOV_STRIDE_MASK;
  val_print(ACS_PRINT_INFO, "\n    First vf offset is 0x%x", first_vf_offset);
  val_print(ACS_PRINT_INFO, "\n    vf stride is 0x%x", vf_stride);

  val_pcie_read_cfg(bdf, sriov_base + SRIOV_VF_COUNT, &reg_value);
  num_vf = (reg_value >> SRIOV_NUM_VF_SHIFT) & SRIOV_NUM_VF_MASK;
  val_print(ACS_PRINT_INFO, "\n    Number of VF's is 0x%x", num_vf);

  vf_rid = pf_rid + first_vf_offset;
  if (num_vf > MAX_VFS)
     val_print(ACS_PRINT_WARN, "\n    Number of VF's present is more than 256", 0);

  for (index = 0; index < num_vf; index++)
  {
     vf_bdf = PCIE_CREATE_BDF_FROM_PACKED(seg, vf_rid);
     val_print(ACS_PRINT_INFO, "\n    Seg is 0x%x", seg);
     val_print(ACS_PRINT_INFO, "\n    vf rid is 0x%x", vf_rid);
     val_print(ACS_PRINT_INFO, "\n    vf bdf is 0x%x", vf_bdf);
     skip_rid_list[index] = vf_bdf;
     vf_rid += vf_stride;
     if (index > MAX_VFS)
         val_print(ACS_PRINT_WARN, "\n    Index value is more than 255", 0);
  }
}

static
void
payload(void)
{

  uint32_t data;
  uint32_t num_ecam;
  uint64_t ecam_base;
  uint32_t index;
  uint32_t bdf = 0;
  uint32_t bus, segment;
  uint32_t end_bus;
  uint32_t bus_index;
  uint32_t dev_index;
  uint32_t func_index;
  uint32_t ret, vf_index;
  uint32_t next_offset = 0;
  uint32_t curr_offset = 0;
  uint32_t status;
  uint32_t sriov_base;

  index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Install sync and async handlers to handle exceptions.*/
  status = val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
  status |= val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);
  if (status)
  {
      val_print(ACS_PRINT_ERR, "\n      Failed in installing the exception handler", 0);
      val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
      return;
  }

  branch_to_test = &&exception_return;

  num_ecam = val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0);
  if (num_ecam == 0) {
      val_print(ACS_PRINT_DEBUG, "\n       No ECAM in MCFG                   ", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM, 1));
      return;
  }

  while (num_ecam) {
      num_ecam--;
      ecam_base = val_pcie_get_info(PCIE_INFO_ECAM, num_ecam);
      if (ecam_base == 0) {
          val_print(ACS_PRINT_ERR, "\n       ECAM Base in MCFG is 0            ", 0);
          val_set_status(index, RESULT_SKIP(TEST_NUM, 1));
          return;
      }

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
                  val_print(ACS_PRINT_ERR,
                        "\n       Incorrect data at ECAM Base %4x    ", data);
                  val_set_status(index, RESULT_FAIL(TEST_NUM, (bus_index << 8)|dev_index));
                  return;
               }

               /* Access the config space, if device ID and vendor ID are valid */
               if (data != PCIE_UNKNOWN_RESPONSE)
               {
                  if (val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS,  &data) != PCIE_SUCCESS)
                      continue;

                  val_print(ACS_PRINT_INFO, "\n     Valid BDF is %x", bdf);
                  if (val_pcie_function_header_type(bdf) == TYPE0_HEADER)
                  {
                      if (val_pcie_find_capability(bdf, PCIE_ECAP, ECID_SRIOV,
                                                   &sriov_base) == PCIE_SUCCESS) {
                          val_print(ACS_PRINT_INFO, "\n     SR-IOV capability present", 0);
                          val_print(ACS_PRINT_INFO, "\n     Check for VF's to skip", 0);
                          calculate_vf(segment, bdf, sriov_base);
                      }
                  }

                  /* Read till the last capability in Extended Capability Structure */
                  next_offset = PCIE_ECAP_START;
                  while (next_offset)
                  {
                     val_pcie_read_cfg(bdf, next_offset, &data);
                     curr_offset = next_offset;
                     next_offset = ((data >> PCIE_ECAP_NCPR_SHIFT) & PCIE_ECAP_NCPR_MASK);
                  }

                  /* Read the start and end from the end of last valid capability register */
                  val_pcie_read_cfg(bdf, curr_offset, &data);
                  val_pcie_read_cfg(bdf, PCIE_ECAP_END, &data);
               }

               /* Access the start and end of the config space for PCIe devices whose
                  device ID and vendor ID are all FF's */
               else{
                  /* Skip the check for VF's as in certain platforms read to config space can
                     give valid response. Hence skipping the VF's */
                  for (vf_index = 0; vf_index < num_vf; vf_index++) {
                      if (bdf == skip_rid_list[vf_index]) {
                          val_print(ACS_PRINT_INFO, "\n   BDF 0x%x is a VF. Hence skipping", bdf);
                          continue;
                      }
                  }

                  val_pcie_read_cfg(bdf, PCIE_ECAP_START, &data);

                  /* Returned data must be FF's, otherwise the test must fail */
                  if (data != PCIE_UNKNOWN_RESPONSE) {
                     val_print(ACS_PRINT_ERR, "\n      Incorrect data for Bdf 0x%x    ", bdf);
                     val_set_status(index, RESULT_FAIL(TEST_NUM,
                                     (bus_index << PCIE_BUS_SHIFT)|dev_index));
                     return;
                  }

                  val_pcie_read_cfg(bdf, PCIE_ECAP_END, &data);

                  /* Returned data must be FF's, otherwise the test must fail */
                  if (data != PCIE_UNKNOWN_RESPONSE) {
                     val_print(ACS_PRINT_ERR, "\n      Incorrect data for Bdf 0x%x    ", bdf);
                     val_set_status(index, RESULT_FAIL(TEST_NUM,
                                     (bus_index << PCIE_BUS_SHIFT)|dev_index));
                     return;
                  }

               }
            }
        }
      }
      val_memory_set(skip_rid_list, sizeof(uint32_t) * MAX_VFS, 0);
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 1));

exception_return:
  return;
}

uint32_t
p002_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}

