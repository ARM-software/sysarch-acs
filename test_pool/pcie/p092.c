/** @file
 * Copyright (c) 2024-2025, Arm Limited or its affiliates. All rights reserved.
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

static const
test_config_t test_entries[] = {
        { ACS_PCIE_TEST_NUM_BASE + 92, "Secondary PCIe ECap Check: iEP Pair   ", "IE_REG_6"},
        { ACS_PCIE_TEST_NUM_BASE + 12, "Datalink feature ECap Check: iEP Pair ", "IE_REG_7"},
        { ACS_PCIE_TEST_NUM_BASE + 13, "Phy Layer 16GT/s ECap Check: iEP Pair ", "IE_REG_8"},
        { ACS_PCIE_TEST_NUM_BASE + 14, "Lane Margining at Rec ECap Check: iEP ", "IE_REG_9"}
    };

/* Declare and define struct - passed as argument to payload */
typedef struct {
    uint32_t test_num;
    char     *payload_segment;  /* Segment of the payload required to cover specific rule */
} test_data_t;

static
void
payload(void *arg)
{

  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t reg_value;
  uint32_t test_fails;
  uint32_t cap_base;
  uint32_t dp_type;
  uint32_t status;
  uint32_t supp_link_speed;
  uint32_t driver_sw;
  bool     test_skip = 1;
  pcie_device_bdf_table *bdf_tbl_ptr;
  test_data_t *test_data = (test_data_t *)arg;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  tbl_index = 0;
  test_fails = 0;

  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      dp_type = val_pcie_device_port_type(bdf);
      val_print(ACS_PRINT_INFO, "\n       BDF - 0x%x", bdf);

      if ((dp_type == iEP_EP) || (dp_type == iEP_RP))
      {
          status = val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &cap_base);
          if (status == PCIE_CAP_NOT_FOUND)
              continue;

          /* If test runs for atleast one iEP */
          test_skip = 0;

          val_pcie_read_cfg(bdf, cap_base + LCAP2R_OFFSET, &reg_value);
          supp_link_speed = (reg_value & LCAP2R_SLSV_MASK) >> LCAP2R_SLSV_SHIFT;

          /* IE_REG_6 */
          if (val_strncmp(test_data->payload_segment, "IE_REG_6", sizeof("IE_REG_6"))) {
              if (supp_link_speed >= LINK_SPEED_8GTPS)
              {
                  status = val_pcie_find_capability(bdf, PCIE_ECAP, ECID_SPCIECS, &cap_base);
                  if (status == PCIE_CAP_NOT_FOUND)
                  {
                      test_fails++;
                      val_print(ACS_PRINT_ERR, "\n       No Sec PCI ECS found for BDF: 0x%x", bdf);
                  }
              }
              continue;
          }

          if (supp_link_speed >= LINK_SPEED_16GTPS)
          {
              /* IE_REG_7 */
              if (val_strncmp(test_data->payload_segment, "IE_REG_7", sizeof("IE_REG_7"))) {
                  status = val_pcie_find_capability(bdf, PCIE_ECAP, ECID_DLFECS, &cap_base);
                  if (status == PCIE_CAP_NOT_FOUND)
                  {
                      test_fails++;
                      val_print(ACS_PRINT_ERR,
                                            "\n       No DL feature ECS found for BDF: 0x%x", bdf);
                  }
                  continue;
              }

              /* IE_REG_8 */
              if (val_strncmp(test_data->payload_segment, "IE_REG_8", sizeof("IE_REG_8"))) {
                  status = val_pcie_find_capability(bdf, PCIE_ECAP, ECID_PL16ECS,  &cap_base);
                  if (status == PCIE_CAP_NOT_FOUND)
                  {
                      test_fails++;
                      val_print(ACS_PRINT_ERR,
                                            "\n       No PL 16GT/s ECS found for BDF: 0x%x", bdf);
                  }
                  else if (status == PCIE_SUCCESS)
                  {
                      val_pcie_read_cfg(bdf, cap_base + 0x10, &reg_value);
                      if (reg_value != 0)
                      {
                          test_fails++;
                          val_print(ACS_PRINT_ERR, "\n       16GT/s LDP not 0 for BDF: 0x%x", bdf);
                      }

                      val_pcie_read_cfg(bdf, cap_base + 0x14, &reg_value);
                      if (reg_value != 0)
                      {
                          test_fails++;
                          val_print(ACS_PRINT_ERR, "\n       16GT/s FRDP not 0 for BDF: 0x%x", bdf);
                      }

                      val_pcie_read_cfg(bdf, cap_base + 0x18, &reg_value);
                      if (reg_value != 0)
                      {
                          test_fails++;
                          val_print(ACS_PRINT_ERR, "\n       16GT/s SRDP not 0 for BDF: 0x%x", bdf);
                      }
                  }
                  continue;
              }

              /* IE_REG_9 */
              if (val_strncmp(test_data->payload_segment, "IE_REG_9", sizeof("IE_REG_9"))) {
                  status = val_pcie_find_capability(bdf, PCIE_ECAP, ECID_LMREC, &cap_base);
                  if (status == PCIE_CAP_NOT_FOUND)
                  {
                      test_fails++;
                      val_print(ACS_PRINT_ERR, "\n       No LM at Rx EC found for BDF: 0x%x", bdf);
                  }
                  else if (status == PCIE_SUCCESS)
                  {
                      val_pcie_read_cfg(bdf, cap_base + 0x4, &reg_value);
                      driver_sw = (reg_value & MPCAPR_DS_MASK) >> MPCAPR_DS_SHIFT;
                      if (driver_sw != 0)
                      {
                          test_fails++;
                          val_print(ACS_PRINT_ERR,
                                            "\n       Margining drv sw not 0 for BDF: 0x%x", bdf);
                      }
                  }
                  continue;
              }
          }
      }
  }

  if (test_skip == 1) {
      val_print(ACS_PRINT_DEBUG,
            "\n       No target device type with required extended cap found. Skipping test", 0);
      val_set_status(pe_index, RESULT_SKIP(test_data->test_num, 01));
  }
  else if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(test_data->test_num, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(test_data->test_num, 01));
}

uint32_t
p092_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  test_data_t data = {.test_num = test_entries[0].test_num, .payload_segment = "IE_REG_6"};

  num_pe = 1;  //This test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(test_entries[0].test_num, test_entries[0].desc, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_configurable_payload(&data, payload);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(test_entries[0].test_num, num_pe, test_entries[0].rule);

  val_report_status(0, ACS_END(test_entries[0].test_num), test_entries[0].rule);
  return status;
}

uint32_t
p012_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  test_data_t data = {.test_num = test_entries[1].test_num, .payload_segment = "IE_REG_7"};

  num_pe = 1;  //This test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(test_entries[1].test_num, test_entries[1].desc, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_configurable_payload(&data, payload);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(test_entries[1].test_num, num_pe, test_entries[1].rule);

  val_report_status(0, ACS_END(test_entries[1].test_num), test_entries[1].rule);

  return status;
}

uint32_t
p013_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  test_data_t data = {.test_num = test_entries[2].test_num, .payload_segment = "IE_REG_8"};

  num_pe = 1;  //This test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(test_entries[2].test_num, test_entries[2].desc, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_configurable_payload(&data, payload);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(test_entries[2].test_num, num_pe, test_entries[2].rule);

  val_report_status(0, ACS_END(test_entries[2].test_num), test_entries[2].rule);

  return status;
}

uint32_t
p014_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  test_data_t data = {.test_num = test_entries[3].test_num, .payload_segment = "IE_REG_9"};

  num_pe = 1;  //This test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(test_entries[3].test_num, test_entries[3].desc, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_configurable_payload(&data, payload);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(test_entries[3].test_num, num_pe, test_entries[3].rule);

  val_report_status(0, ACS_END(test_entries[3].test_num), test_entries[3].rule);

  return status;
}