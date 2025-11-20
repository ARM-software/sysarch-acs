/** @file
 * Copyright (c) 2020-2025, Arm Limited or its affiliates. All rights reserved.
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

static const
test_config_t test_entries[] = {
        { ACS_PCIE_TEST_NUM_BASE + 70, "Check Power Management rules: RCiEP   ", "RE_PWR_1"},
        { ACS_PCIE_TEST_NUM_BASE + 34, "Check Power Management rules: iEP/RP  ", "IE_PWR_1"}
    };

/* Declare and define struct - passed as argument to payload */
typedef struct {
    uint32_t test_num;
    uint32_t dev_type1;
    uint32_t dev_type2;
} test_data_t;

static
void
payload(void *arg)
{

  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t dp_type;
  uint32_t cap_base;
  uint32_t test_fails;
  bool     test_skip = 1;
  pcie_device_bdf_table *bdf_tbl_ptr;
  test_data_t *test_data = (test_data_t *)arg;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  test_fails = 0;

  /* Check for all the function present in bdf table */
  for (tbl_index = 0; tbl_index < bdf_tbl_ptr->num_entries; tbl_index++)
  {
      bdf = bdf_tbl_ptr->device[tbl_index].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      /* Check entry is onchip peripherals */
      if ((dp_type == test_data->dev_type1) || (dp_type == test_data->dev_type2))
      {
         val_print(ACS_PRINT_DEBUG, "\n       BDF - 0x%x", bdf);

         /* If test runs for atleast an endpoint */
         test_skip = 0;

         /* If Power Management capability not supported, test fails */
         if (val_pcie_find_capability(bdf, PCIE_CAP, CID_PMC, &cap_base) == PCIE_CAP_NOT_FOUND) {
          val_print(ACS_PRINT_ERR,
                    "\n       BDF - %x does not support Power Management Capability", bdf);
          test_fails++;
         }
      }
  }

  if (test_skip == 1) {
      val_print(ACS_PRINT_DEBUG,
        "\n       No target device type found. Skipping test", bdf);
      val_set_status(pe_index, RESULT_SKIP(test_data->test_num, 01));
  }
  else if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(test_data->test_num, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(test_data->test_num, 01));
}

uint32_t
p070_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  test_data_t data = {.test_num = test_entries[0].test_num, .dev_type1 = (uint32_t)RCiEP};

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
p034_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  test_data_t data = {.test_num = test_entries[1].test_num,
    .dev_type1 = (uint32_t)iEP_EP,
    .dev_type2 = (uint32_t)iEP_RP};

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