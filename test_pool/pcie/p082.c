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
        { ACS_PCIE_TEST_NUM_BASE + 82, "Check ACS Cap on p2p support: iEP EP  ", "IE_ACS_1"},
        { ACS_PCIE_TEST_NUM_BASE + 15, "Check ACS Cap on p2p support: RCiEP   ", "RE_ACS_1"},
        { ACS_PCIE_TEST_NUM_BASE + 16, "Check AER Cap on ACS Cap support      ", "RE_ACS_2"}
    };

/* Declare and define struct - passed as argument to payload */
typedef struct {
    uint32_t test_num;
    uint32_t dev_type;
    uint32_t aer_check_flag;
} test_data_t;

/* Global variable declaration - to be used for RCiEP rules */
static uint32_t g_aer_cap_status;

static
void
parse_test_status(uint32_t test_fail, uint32_t test_skip)
{
  /* Report failure for RE_ACS_2 only if pre-requisite is met and AER is not supported. */
  if (test_skip == 1)
      g_aer_cap_status = ACS_STATUS_SKIP;
  else if (test_fail)
      g_aer_cap_status = ACS_STATUS_FAIL;
  else
      g_aer_cap_status = ACS_STATUS_PASS;

  return;
}

/* IE_ACS_1 rule covers for iEP_EP -  RE_ACS_1 and RE_ACS_2 together cover for RCiEP.
   So run the payload only once and mark the status of AER Cap.
   Report RE_ACS_2 based on AER Cap status. */
static
void
payload(void *arg)
{

  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t dp_type;
  uint32_t cap_base = 0;
  uint32_t test_fails;
  bool     test_skip = 1;
  uint32_t aer_cap_fail = 0;
  uint32_t acs_data;
  uint32_t data;
  uint8_t p2p_support_flag = 0;
  pcie_device_bdf_table *bdf_tbl_ptr;
  test_data_t *test_data = (test_data_t *)arg;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Check If PCIe Hierarchy supports P2P */
  if (val_pcie_p2p_support() == NOT_IMPLEMENTED) {
    val_print(ACS_PRINT_DEBUG, "\n       The test is applicable only if the system supports", 0);
    val_print(ACS_PRINT_DEBUG, "\n       P2P traffic. If the system supports P2P, pass the", 0);
    val_print(ACS_PRINT_DEBUG, "\n       command line option '-p2p' while running the binary", 0);
    val_set_status(pe_index, RESULT_SKIP(test_data->test_num, 01));
    return;
  }

  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  test_fails = 0;

  /* Check for all the function present in bdf table */
  for (tbl_index = 0; tbl_index < bdf_tbl_ptr->num_entries; tbl_index++)
  {
      bdf = bdf_tbl_ptr->device[tbl_index].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      /* Check entry is RCiEP or iEP end point */
      if ((dp_type == RCiEP) || (dp_type == iEP_EP))
      {
          val_print(ACS_PRINT_DEBUG, "\n       BDF - 0x%x", bdf);

          /* Check if the EP Supports Multifunction */
          if (val_pcie_multifunction_support(bdf))
              continue;

          /* Check If Endpoint supports P2P with other Functions. */
          if (val_pcie_dev_p2p_support(bdf))
              continue;

          /* If test runs for atleast an endpoint */
          test_skip = 0;

          /* Read the ACS Capability */
          if (val_pcie_find_capability(bdf, PCIE_ECAP, ECID_ACS, &cap_base) != PCIE_SUCCESS)
          {
              val_print(ACS_PRINT_ERR, "\n       ACS Capability not supported, Bdf : 0x%x", bdf);
              test_fails++;
              continue;
          }

          val_pcie_read_cfg(bdf, cap_base + ACSCR_OFFSET, &acs_data);

          /* Extract ACS p2p Request Redirect bit */
          data = VAL_EXTRACT_BITS(acs_data, 2, 2);
          if (data == 0) {
              val_print(ACS_PRINT_DEBUG, "\n       Request Redirect P2P not supported", 0);
              p2p_support_flag++;
          }

          /* Extract ACS p2p Completion Redirect bit */
          data = VAL_EXTRACT_BITS(acs_data, 3, 3);
          if (data == 0) {
              val_print(ACS_PRINT_DEBUG, "\n       Completion Redirect P2P not supported", 0);
              p2p_support_flag++;
          }

          /* Extract ACS p2p Direct Translated bit */
          data = VAL_EXTRACT_BITS(acs_data, 6, 6);
          if (data == 0) {
              val_print(ACS_PRINT_DEBUG, "\n       Direct Translated P2P not supported", 0);
              p2p_support_flag++;
          }

          if (p2p_support_flag > 0) {
              val_print(ACS_PRINT_ERR, "\n       P2P not supported for bdf: %d", bdf);
              p2p_support_flag = 0;
              test_fails++;
          }
          /* If device supports ACS then it must have AER Capability */
          if (val_pcie_find_capability(bdf, PCIE_ECAP, ECID_AER, &cap_base) != PCIE_SUCCESS)
          {
              val_print(ACS_PRINT_ERR, "\n       AER Capability not supported, Bdf : 0x%x", bdf);
              aer_cap_fail++;
          }
      }
  }

  /* ACS and AER capability reporting for RCiEPs are performed separately */
  if (test_data->aer_check_flag) {
    parse_test_status(aer_cap_fail, test_skip);
    /* Don't let AER failure effect RE_ACS_1 status */
    aer_cap_fail = 0;
  }

  if (test_skip == 1) {
      val_print(ACS_PRINT_DEBUG,
      "\n       No target device type with Multifunction and P2P support.Skipping test", 0);
      val_set_status(pe_index, RESULT_SKIP(test_data->test_num, 02));
  }
  else if (test_fails + aer_cap_fail)
      val_set_status(pe_index, RESULT_FAIL(test_data->test_num, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(test_data->test_num, 01));
}

uint32_t
p082_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  test_data_t data = {.test_num = test_entries[0].test_num,
                      .dev_type = (uint32_t)iEP_EP,
                      .aer_check_flag = 0};

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
p015_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  test_data_t data = {.test_num = test_entries[1].test_num,
                      .dev_type = (uint32_t)RCiEP,
                      .aer_check_flag = 1};

  num_pe = 1;  //This test is run on single processor
  g_aer_cap_status = ACS_STATUS_SKIP;

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
p016_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  uint32_t pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  num_pe = 1;  //This test is run on single processor
  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(test_entries[2].test_num, test_entries[2].desc, num_pe);

  if (status != ACS_STATUS_SKIP) {
      if (g_aer_cap_status == ACS_STATUS_SKIP)
          val_set_status(pe_index, RESULT_SKIP(test_entries[2].test_num, 01));
      else if (g_aer_cap_status == ACS_STATUS_FAIL)
          val_set_status(pe_index, RESULT_FAIL(test_entries[2].test_num, 01));
      else
          val_set_status(pe_index, RESULT_PASS(test_entries[2].test_num, 01));
  }

  /* get the result from all PE and check for failure */
  status = val_check_for_error(test_entries[2].test_num, num_pe, test_entries[2].rule);

  val_report_status(0, ACS_END(test_entries[2].test_num), test_entries[2].rule);
  return status;
}
