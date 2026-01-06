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
#include "val/include/acs_pe.h"
#include "val/include/val_interface.h"
#include "val/include/acs_pcie.h"

static const
test_config_t test_entries[] = {
    { ACS_PCIE_TEST_NUM_BASE + 58, "Check MSE, CapPtr & BIST: RCiEP, RCEC ", "RE_REG_1"},
    { ACS_PCIE_TEST_NUM_BASE + 59, "Check MSE, CapPtr & BIST: iEP EP      ", "IE_REG_1"},
    { ACS_PCIE_TEST_NUM_BASE + 60, "Check MSE, CapPtr & BIST: iEP RP      ", "IE_REG_3"}
};

/* Declare and define struct - passed as argument to payload */
typedef struct {
    uint32_t dev_type1;
    uint32_t dev_type2;
} test_data_t;

static void *branch_to_test;
static uint32_t test_num;

static
void
esr(uint64_t interrupt_type, void *context)
{
  uint32_t pe_index;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Update the ELR to return to test specified address */
  val_pe_update_elr(context, (uint64_t)branch_to_test);

  val_print(ACS_PRINT_DEBUG, "\n       Received exception of type: %d", interrupt_type);
  val_set_status(pe_index, RESULT_PASS(test_num, 01));
}

static
void
payload(void *arg)
{

  uint32_t bdf;
  uint32_t dsf_bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t bar_data;
  uint32_t test_fails;
  uint32_t test_skip = 1;
  uint64_t bar_base;
  uint32_t dp_type;
  uint32_t status;
  uint32_t timeout;
  uint32_t reg_value;
  test_data_t *test_data = (test_data_t *)arg;

  pcie_device_bdf_table *bdf_tbl_ptr;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  /* Install sync and async handlers to handle exceptions.*/
  status = val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
  status |= val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);
  if (status)
  {
      val_print(ACS_PRINT_ERR, "\n       Failed in installing the exception handler", 0);
      val_set_status(pe_index, RESULT_FAIL(test_num, 01));
      return;
  }

  branch_to_test = &&exception_return;

  bar_data = 0;
  tbl_index = 0;
  test_fails = 0;

  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      val_print(ACS_PRINT_DEBUG, "\n       tbl_index %x", tbl_index - 1);

      dp_type = val_pcie_device_port_type(bdf);

      /* Check entry is RCiEP/ RCEC/ iEP. Else move to next BDF. */
      if ((dp_type != test_data->dev_type1) && (dp_type != test_data->dev_type2))
          continue;

      val_print(ACS_PRINT_DEBUG, "\n       BDF - 0x%x", bdf);

      /* Get BIST register value */
      reg_value = val_pcie_get_bist(bdf);

      /* If BIST Capable bit[7] is clear Completion Code[0:3] and Start Bist[6]
      * must be hardwired to 0b */
      if (((reg_value & BIST_BC_MASK) == 0x00) &&
         (((reg_value & BIST_CC_MASK) != 0x00) || ((reg_value & BIST_SB_MASK) != 0x00)))
      {
          val_print(ACS_PRINT_ERR, "\n       BDF - 0x%x", bdf);
          val_print(ACS_PRINT_ERR, " BIST Reg Value : %d", reg_value);
          test_fails++;
      }

      /* Get Capabilities Pointer register value */
      reg_value = val_pcie_get_cap_ptr(bdf);

      /* Check Capabilities Pointer is not NULL and is between 40h and FCh */
      if (!((reg_value != 0x00) && ((reg_value >= 0x40) && (reg_value <= 0xFC))))
      {
          val_print(ACS_PRINT_ERR, "\n       BDF 0x%x", bdf);
          val_print(ACS_PRINT_ERR, " Cap Ptr Value: 0x%x", reg_value);
          test_fails++;
      }

      /*
       * For a Function with type 0 config space header, obtain
       * base address of its Memory mapped BAR. For Function with
       * Type 1 config space header, obtain base address of the
       * downstream function memory mapped BAR. If there is no
       * downstream Function exist, obtain its own BAR address.
       */
      if ((val_pcie_function_header_type(bdf) == TYPE1_HEADER) &&
           (!val_pcie_get_downstream_function(bdf, &dsf_bdf)))
          val_pcie_get_mmio_bar(dsf_bdf, &bar_base);
      else
          val_pcie_get_mmio_bar(bdf, &bar_base);

      /* Skip this function if it doesn't have mmio BAR */
      val_print(ACS_PRINT_DEBUG, "       Bar Base %x", bar_base);
      if (!bar_base)
         continue;

      /* Disable error reporting of this function to the Upstream */
      val_pcie_disable_eru(bdf);

      /*
       * Clear unsupported request detected bit in Device
       * Status Register to clear any pending urd status.
       */
      val_pcie_clear_urd(bdf);

      /*
       * Disable BAR memory space access to cause address
       * decode failures. With memory space aceess disable,
       * all received memory space accesses are handled as
       * Unsupported Requests by the pcie function.
       */
      val_pcie_disable_msa(bdf);

      /* Set test status as FAIL, update to PASS in exception handler */
      val_set_status(pe_index, RESULT_FAIL(test_num, 02));

      /* If test runs for atleast an endpoint */
      test_skip = 0;

      /*
       * Read memory mapped BAR to cause unsupported request
       * response. Based on platform configuration, this may
       * even cause an sync/async exception.
       */
      bar_data = (*(volatile addr_t *)bar_base);
      timeout = TIMEOUT_SMALL;
      while (--timeout > 0);

exception_return:
      /*
       * If none of below condition are met, device MSE check is considered fail
       *   - UR bit detected set
       *   - All 1's response received
       *   - Abort is not received.
       */
      val_print(ACS_PRINT_DEBUG, "       bar_data %x ", bar_data);
      if (!(IS_TEST_PASS(val_get_status(pe_index)) || (bar_data == PCIE_UNKNOWN_RESPONSE)
            || (val_pcie_is_urd(bdf))))
      {
          val_print(ACS_PRINT_ERR, "\n       BDF %x MSE functionality failure", bdf);
          test_fails++;
      }

      /* Enable memory space access to decode BAR addresses */
      val_pcie_enable_msa(bdf);

      /* Reset the loop variables */
      bar_data = 0;
  }

  if (test_skip == 1) {
      val_print(ACS_PRINT_DEBUG,
        "\n       Found no target device type with MMIO BAR. Skipping test.", 0);
      val_set_status(pe_index, RESULT_SKIP(test_num, 01));
  }
  else if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(test_num, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(test_num, 01));
}

uint32_t
p058_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  test_data_t data = {.dev_type1 = (uint32_t)RCEC, .dev_type2 = (uint32_t)RCiEP};

  num_pe = 1;  //This test is run on single processor
  test_num = test_entries[0].test_num;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(test_num, test_entries[0].desc, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_configurable_payload(&data, payload);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(test_num, num_pe, test_entries[0].rule);

  val_report_status(0, ACS_END(test_num), test_entries[0].rule);

  return status;
}

uint32_t
p059_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  test_data_t data = {.dev_type1 = (uint32_t)iEP_EP};

  num_pe = 1;  //This test is run on single processor
  test_num = test_entries[1].test_num;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(test_num, test_entries[1].desc, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_configurable_payload(&data, payload);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(test_num, num_pe, test_entries[1].rule);

  val_report_status(0, ACS_END(test_num), test_entries[1].rule);

  return status;
}

uint32_t
p060_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  test_data_t data = {.dev_type1 = (uint32_t)iEP_RP};

  num_pe = 1;  //This test is run on single processor
  test_num = test_entries[2].test_num;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(test_num, test_entries[2].desc, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_configurable_payload(&data, payload);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(test_num, num_pe, test_entries[2].rule);

  val_report_status(0, ACS_END(test_num), test_entries[2].rule);

  return status;
}