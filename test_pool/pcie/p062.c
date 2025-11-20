/** @file
 * Copyright (c) 2019, 2021-2025, Arm Limited or its affiliates. All rights reserved.
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
        { ACS_PCIE_TEST_NUM_BASE + 62, "Check BAR memory space & type: RCiEP  ", "RE_BAR_3"},
        { ACS_PCIE_TEST_NUM_BASE + 41, "Check BAR memory space & type: iEP/RP ", "IE_BAR_3"}
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
  uint32_t reg_value;
  uint32_t hdr_type;
  uint32_t max_bar;
  uint32_t addr_type;
  uint32_t bar_index;
  uint32_t  dp_type;
  uint32_t test_fails;
  uint32_t test_skip = 1;
  pcie_device_bdf_table *bdf_tbl_ptr;
  test_data_t *test_data = (test_data_t *)arg;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  tbl_index = 0;
  test_fails = 0;

  /* Check for all the function present in bdf table */
  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      /* Check for RCiEP, iEP_EP and iEP_RP type devices */
      if (dp_type == test_data->dev_type1 || dp_type == test_data->dev_type2)
      {
          val_print(ACS_PRINT_DEBUG, "\n       BDF - 0x%x ", bdf);
          /* Extract Hdr Type */
          hdr_type = val_pcie_function_header_type(bdf);
          val_print(ACS_PRINT_INFO, "\n       HDR TYPE 0x%x ", hdr_type);

          max_bar = 0;
          /* For Type0 header max bars 6, type1 header max bars 2 */
          if (hdr_type == TYPE0_HEADER)
             max_bar = TYPE0_MAX_BARS;
          else if (hdr_type == TYPE1_HEADER)
             max_bar = TYPE1_MAX_BARS;
          val_print(ACS_PRINT_INFO, "\n       MAX BARS 0x%x ", max_bar);

          for (bar_index = 0; bar_index < max_bar; bar_index++)
          {
              /* Read BAR0 register */
              val_pcie_read_cfg(bdf, TYPE01_BAR + (bar_index * BAR_BASE_SHIFT), &reg_value);

              /* If BAR not in use skip the BAR */
              if (reg_value == 0)
                  continue;

              /* If test runs for atleast an endpoint */
              test_skip = 0;

              /* Check type[1:2] must be 32-bit or 64-bit */
              addr_type = (reg_value >> BAR_MDT_SHIFT) & BAR_MDT_MASK;
              if ((addr_type != BITS_32) && (addr_type != BITS_64))
              {
                  val_print(ACS_PRINT_ERR, "\n       BDF 0x%x ", bdf);
                  val_print(ACS_PRINT_ERR, " Addr Type: 0x%x", addr_type);
                  test_fails++;
                  continue;
              }

              /* if BAR is 64 bit move index to next BAR */
              if (addr_type == BITS_64)
                  bar_index++;

              /* Check BAR must be MMIO */
              if (reg_value & BAR_MIT_MASK)
              {
                 val_print(ACS_PRINT_ERR, "\n       BDF 0x%x Not MMIO", 0);
                 test_fails++;
              }
           }
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
p062_entry(uint32_t num_pe)
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
p041_entry(uint32_t num_pe)
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