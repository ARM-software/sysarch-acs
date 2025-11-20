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

static const
test_config_t test_entries[] = {
    { ACS_PCIE_TEST_NUM_BASE + 61, "Check Max payload size support: RCEC  ", "RE_REC_1"},
    { ACS_PCIE_TEST_NUM_BASE + 98, "Check Max payload size support: iEP_EP", "IE_REG_2"},
    { ACS_PCIE_TEST_NUM_BASE + 99, "Check Max payload size support: iEP_RP", "IE_REG_4"}
};

/* Declare and define struct - passed as argument to payload */
typedef struct {
    uint32_t test_num;
    uint32_t dev_type;
} test_data_t;

static
void
payload(void *arg)
{

  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t reg_value;
  int32_t max_payload_value;
  uint32_t dp_type;
  uint32_t test_fails;
  uint32_t test_skip = 1;
  uint32_t cap_base;
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

      /* Check entry matches the payload argument */
      /* Arg can be RCEC/iEP_EP/iEP_RP based on the rule */
      if (dp_type != test_data->dev_type)
          continue;

      val_print(ACS_PRINT_DEBUG, "\n       BDF - 0x%x", bdf);
      val_print(ACS_PRINT_DEBUG, " dp_type is %llx", dp_type);

      /* Retrieve the addr of PCI express capability (10h) */
      val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &cap_base);

      /* Read Device Capabilities register(04h) present in PCIE capability struct(10h) */
      val_pcie_read_cfg(bdf, cap_base + DCAPR_OFFSET, &reg_value);

      /* Extract Max payload Size Supported value */
      max_payload_value = (reg_value >> DCAPR_MPSS_SHIFT) & DCAPR_MPSS_MASK;

      /* If test runs for atleast an endpoint */
      test_skip = 0;

      /* Valid payload size between 000b (129-bytes) to 101b (4096 bytes) */
      if (!((max_payload_value >= 0x00) && (max_payload_value <= 0x05)))
      {
          val_print(ACS_PRINT_ERR, "\n        BDF 0x%x", bdf);
          val_print(ACS_PRINT_ERR, " Cap Ptr Value: 0x%x", max_payload_value);
          test_fails++;
      }
  }

  if (test_skip == 1)
      val_set_status(pe_index, RESULT_SKIP(test_data->test_num, 01));
  else if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(test_data->test_num, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(test_data->test_num, 01));
}

uint32_t
p061_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  test_data_t data = {.test_num = test_entries[0].test_num, .dev_type = (uint32_t)RCEC};

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
p098_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  test_data_t data = {.test_num = test_entries[1].test_num, .dev_type = (uint32_t)iEP_EP};

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
p099_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  test_data_t data = {.test_num = test_entries[2].test_num, .dev_type = (uint32_t)iEP_RP};

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