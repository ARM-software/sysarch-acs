/** @file
 * Copyright (c) 2016-2026, Arm Limited or its affiliates. All rights reserved.
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
#include "acs_val.h"
#include "acs_pe.h"
#include "acs_pcie.h"
#include "acs_memory.h"

/* Test runs on Linux and BM Env */
static const
test_config_t test_entries[] = {
    /* p045 targets EP, RP, DP, and UP, and is executed for BSA */
    { ACS_PCIE_TEST_NUM_BASE + 45, "PCIe Device Memory mapping support    ", "PCI_MM_01"},
    /* p103 targets RCiEP, RCEC, iEP_EP, and iEP_RP, and is executed for SBSA */
    { ACS_PCIE_TEST_NUM_BASE + 103, "PCIe Device Memory mapping support    ", "PCI_MM_01"},
    /* p106 targets EP, RP, DP, and UP, and maps BAR to NORMAL memory */
    { ACS_PCIE_TEST_NUM_BASE + 106, "PCIe Device Memory mapping support    ", "PCI_MM_02"},
    /* p107 targets RCiEP, RCEC, iEP_EP, and iEP_RP, and maps BAR to NORMAL memory */
    { ACS_PCIE_TEST_NUM_BASE + 107, "PCIe Device Memory mapping support    ", "PCI_MM_02"}
};

#define DATA 0xC0DECAFE

static void *branch_to_test;
static uint32_t test_num;
static uint32_t onchip_peripherals_check;
static uint32_t memory_map_attr;

static
void
esr(uint64_t interrupt_type, void *context)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Update the ELR to point to next instrcution */
  val_pe_update_elr(context, (uint64_t)branch_to_test);

  val_print(ERROR, "\n       Received Exception %d", interrupt_type);
  val_set_status(index, RESULT_FAIL(02));
}

static
uint32_t
map_device_memory(uint64_t base, char **baseptr)
{
  return val_memory_ioremap((void *)base, 1024, memory_map_attr, (void **)baseptr);
}

static
void
payload_check_device_mem_map(void)
{
  uint32_t data;
  uint32_t old_data;
  uint32_t bdf;
  uint32_t bar_reg_value;
  uint32_t bar_reg_lower_value;
  uint64_t bar_upper_bits;
  uint32_t bar_value;
  uint32_t bar_value_1;
  uint64_t bar_size;
  char    *baseptr;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t test_skip = 1;
  bool skip_due_to_flag = false;
  bool skip_flag = acs_policy_get_pcie_skip_dp_nic_ms();
  uint32_t test_warn = 1;
  uint32_t test_fail = 0;
  uint64_t offset;
  uint64_t base;
  pcie_device_bdf_table *bdf_tbl_ptr;
  uint32_t tbl_index = 0;
  uint32_t status;
  uint32_t dev_type;
  uint32_t dp_type;
  uint32_t max_bar_offset;
  uint32_t msa_en = 0;
  uint32_t reg_value, base_cc;

  val_set_status(index, RESULT_SKIP(0));

  /* Install exception handlers */
  status = val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
  status |= val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);
  if (status)
  {
      val_print(ERROR, "\n       Failed to install exception handler");
      val_set_status(index, RESULT_FAIL(01));
      return;
  }

  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

next_bdf:
  for (; tbl_index < bdf_tbl_ptr->num_entries; tbl_index++) {
      msa_en = 0;
      bdf = bdf_tbl_ptr->device[tbl_index].bdf;
      dp_type = val_pcie_device_port_type(bdf);
      /* Based on the test scope, verify if the device type falls under DP/UP/EP/RP/
         or iEP/RCiEP/RCEC. */
      if (onchip_peripherals_check) {
          if ((dp_type != iEP_EP) && (dp_type != iEP_RP) && (dp_type != RCEC) && (dp_type != RCiEP))
              continue;
      } else {
          if ((dp_type != EP) && (dp_type != RP) && (dp_type != DP) && (dp_type != UP))
              continue;
      }

      val_print(DEBUG, "\n   BDF under check %08x", bdf);
      val_pcie_read_cfg(bdf, TYPE01_RIDR, &reg_value);
      val_print(DEBUG, "\n       Class code is 0x%x", reg_value);
      base_cc = reg_value >> TYPE01_BCC_SHIFT;

      if (skip_flag &&
          ((base_cc == UNCLAS_CC) || (base_cc == CNTRL_CC)
          || (base_cc == DP_CNTRL_CC) || (base_cc == MAS_CC))) {
          skip_due_to_flag = true;
          val_print(DEBUG, "\n       Skipping BDF 0x%x", bdf);
          tbl_index++;
          goto next_bdf;
      }

      /* Configure the max BAR offset */
      dev_type = val_pcie_get_device_type(bdf);
      if (dev_type == 1)
          max_bar_offset = BAR_TYPE_0_MAX_OFFSET;
      else
          max_bar_offset = BAR_TYPE_1_MAX_OFFSET;

      offset = BAR0_OFFSET;

      while (offset <= max_bar_offset) {
          val_pcie_read_cfg(bdf, offset, &bar_value);
          val_print(DEBUG, "\n       The BAR value at offset %x", offset);
          val_print(DEBUG, " is %x ", bar_value);
          base = 0;

          if (bar_value == 0)
          {
              /* This BAR is not implemented */
              val_print(DEBUG, "\n       BAR is not implemented for BDF 0x%x", bdf);
              goto next_bar;
          }

          /* Skip for IO address space */
          if (bar_value & BAR_VALUE_IO_MASK) {
              val_print(DEBUG, "\n       BAR is used for IO address space request");
              val_print(DEBUG, " for BDF 0x%x", bdf);
              goto next_bar;
          }

          if (BAR_REG(bar_value) == BAR_64_BIT)
          {
              val_print(TRACE,
                        "\n       The BAR supports 64-bit address decoding capability");
              val_pcie_read_cfg(bdf, offset + BAR_32B_OFFSET, &bar_value_1);
              base = bar_value_1;

              /* BAR supports 64-bit address therefore, write all 1's
               * to BARn and BARn+1 and identify the size requested
               */
              val_pcie_write_cfg(bdf, offset, 0xFFFFFFF0);
              val_pcie_write_cfg(bdf, offset + BAR_32B_OFFSET, 0xFFFFFFFF);
              val_pcie_read_cfg(bdf, offset, &bar_reg_lower_value);
              bar_size = bar_reg_lower_value & 0xFFFFFFF0;
              val_pcie_read_cfg(bdf, offset + BAR_32B_OFFSET, &bar_reg_value);
              bar_upper_bits = bar_reg_value;
              bar_size = bar_size | (bar_upper_bits << 32);
              bar_size = ~bar_size + 1;

              /* Restore the original BAR value */
              val_pcie_write_cfg(bdf, offset + BAR_32B_OFFSET, bar_value_1);
              val_pcie_write_cfg(bdf, offset, bar_value);
              base = (base << 32) | (bar_value & BAR_MASK);
          }
          else {
              val_print(TRACE,
                        "\n       The BAR supports 32-bit address decoding capability");

              /* BAR supports 32-bit address. Write all 1's
               * to BARn and identify the size requested
               */
              val_pcie_write_cfg(bdf, offset, 0xFFFFFFF0);
              val_pcie_read_cfg(bdf, offset, &bar_reg_lower_value);
              bar_reg_value = bar_reg_lower_value & 0xFFFFFFF0;
              bar_size = ~bar_reg_value + 1;

              /* Restore the original BAR value */
              val_pcie_write_cfg(bdf, offset, bar_value);
              base = bar_value & BAR_MASK;
          }

          val_print(DEBUG, "\n       BAR size is %x", bar_size);
          val_print(DEBUG, "\n       BAR base is 0x%llx", base);

          /* Check if bar supports the remap size */
          if (bar_size < 1024) {
              val_print(ERROR, "\n       Bar size less than remap requested size");
              goto next_bar;
          }

          /* Enable Memory Space Access to the BDF if not enabled */
          msa_en = val_pcie_is_msa_enabled(bdf);
          if (msa_en)
              val_pcie_enable_msa(bdf);

          branch_to_test = &&exception_return_device;

          test_skip = 0;

          /* Map the BARs to a memory attribute mentioned
           * and check transaction.
           */
          status = map_device_memory(base, &baseptr);
          /* Handle unimplemented PAL -> Report WARN */
          if (status == ACS_STATUS_PAL_NOT_IMPLEMENTED) {
            goto test_status;
          }
          else if (status) {
            val_print(ERROR,
                  "\n       pal_memory_ioremap failed, status: 0x%x", status);
            test_fail++;
            val_set_status(index, RESULT_FAIL(test_fail));
            goto next_bar;
          }

          test_warn = 0;

          /* Access check inside payload after obtaining BAR base and mapping */
          old_data = *(uint32_t *)(baseptr);
          *(uint32_t *)(baseptr) = DATA;
          data = *(uint32_t *)(baseptr);
          val_print(DEBUG, "\n       Value read: %llx", data);
          *(uint32_t *)(baseptr) = old_data;

exception_return_device:
          if (IS_TEST_FAIL(val_get_status(index))) {
              val_print(ERROR, "\n       Device memory access failed for Bdf: 0x%x", bdf);
              /* Setting the status to Pass to enable test for next BDF.
               * Failure has been recorded with test_fail.
               */
              val_set_status(index, RESULT_PASS);
              test_fail++;
          }

next_bar:
          if (BAR_REG(bar_value) == BAR_32_BIT)
              offset = offset + BAR_32B_OFFSET;
          else if (BAR_REG(bar_value) == BAR_64_BIT)
              offset = offset + BAR_64B_OFFSET;

          if (msa_en)
              val_pcie_disable_msa(bdf);
      }
  }

test_status:
  if (test_skip) {
      if (skip_flag && skip_due_to_flag) {
          val_print(WARN,
            "\n       DP/NIC/MAS/RES devices are skipped.");
          val_print(WARN,
            "\n       Please individually run the test without");
          val_print(WARN,
            "\n       --skip-dp-nic-ms to check the compliance.");
          val_set_status(index, RESULT_WARNING(1));
      } else {
          val_set_status(index, RESULT_SKIP(1));
      }
  } else if (test_warn) {
      val_set_status(index, RESULT_WARNING(1));
  } else if (test_fail) {
      val_set_status(index, RESULT_FAIL(test_fail));
  } else {
      val_set_status(index, RESULT_PASS);
  return;
  }
}

uint32_t
p045_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;
  test_num = test_entries[0].test_num;
  onchip_peripherals_check = 0;
  memory_map_attr = DEVICE_nGnRnE;

  num_pe = 1;  //This test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(test_num, test_entries[0].desc, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(test_num, num_pe, payload_check_device_mem_map, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(test_num, num_pe, test_entries[0].rule);

  val_report_status(0, ACS_END(test_num), test_entries[0].rule);

  return status;
}

uint32_t
p103_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;
  test_num = test_entries[1].test_num;
  onchip_peripherals_check = 1;
  memory_map_attr = DEVICE_nGnRnE;

  num_pe = 1;  //This test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(test_num, test_entries[1].desc, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(test_num, num_pe, payload_check_device_mem_map, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(test_num, num_pe, test_entries[1].rule);

  val_report_status(0, ACS_END(test_num), test_entries[1].rule);

  return status;
}

uint32_t
p106_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;
  test_num = test_entries[2].test_num;
  onchip_peripherals_check = 0;
  memory_map_attr = NORMAL_NC;

  num_pe = 1;  //This test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(test_num, test_entries[2].desc, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(test_num, num_pe, payload_check_device_mem_map, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(test_num, num_pe, test_entries[2].rule);

  val_report_status(0, ACS_END(test_num), test_entries[2].rule);

  return status;
}

uint32_t
p107_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;
  test_num = test_entries[3].test_num;
  onchip_peripherals_check = 1;
  memory_map_attr = NORMAL_NC;

  num_pe = 1;  //This test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(test_num, test_entries[3].desc, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(test_num, num_pe, payload_check_device_mem_map, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(test_num, num_pe, test_entries[3].rule);

  val_report_status(0, ACS_END(test_num), test_entries[3].rule);

  return status;
}
