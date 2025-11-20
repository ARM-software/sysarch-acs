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
#include "val/include/acs_gic.h"
#include "val/include/acs_pcie.h"
#include "val/include/acs_pe.h"
#include "val/include/acs_memory.h"

static const
test_config_t test_entries[] = {
        { ACS_PCIE_TEST_NUM_BASE + 27, "Check Legacy Intrrupt is SPI          ", "PCI_LI_01"},
        { ACS_PCIE_TEST_NUM_BASE + 78, "Check Legacy Intr SPI level sensitive ", "PCI_LI_03"}
    };

/* Declare static global variables - used across multiple test entries */
static uint32_t prereq_status;
static uint32_t intr_line;
static uint32_t test_num;

/* Payload for the rule PCI_LI_01 */
static
void
payload_primary(void)
{
  uint32_t status;
  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t dp_type;
  uint32_t reg_value;
  bool     test_skip = 1;
  uint32_t intr_pin;
  PERIPHERAL_IRQ_MAP *intr_map;
  pcie_device_bdf_table *bdf_tbl_ptr;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Allocate memory for interrupt mappings */
  intr_map = val_aligned_alloc(MEM_ALIGN_4K, sizeof(PERIPHERAL_IRQ_MAP));
  if (!intr_map) {
    val_print (ACS_PRINT_ERR, "\n       Memory allocation error", 0);
    val_set_status(pe_index, RESULT_FAIL (test_num, 01));
    return;
  }

  tbl_index = 0;
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      /* Check entry is RCiEP/ RCEC,  else move to next BDF. */
      if ((dp_type != RCEC) && (dp_type != RCiEP))
          continue;

      val_print(ACS_PRINT_DEBUG, "\n       BDF - 0x%x", bdf);

      /* Read Interrupt Line Register */
      val_pcie_read_cfg(bdf, TYPE01_ILR, &reg_value);

      intr_pin = (reg_value >> TYPE01_IPR_SHIFT) & TYPE01_IPR_MASK;
      if ((intr_pin == 0) || (intr_pin > 0x4))
        continue;

      status = val_pci_get_legacy_irq_map(bdf, intr_map);
      if (status) {
        /* Skip the test if the Legacy IRQ map does not exist */
        if (status == NOT_IMPLEMENTED) {
            val_print (ACS_PRINT_WARN,
                        "\n       pal_pcie_get_legacy_irq_map unimplemented. Skipping test", 0);
            val_print(ACS_PRINT_WARN,
                        "\n       The API is platform specific and to be populated", 0);
            val_print(ACS_PRINT_WARN,
                        "\n       by partners with system legacy irq map", 0);
            val_set_status(pe_index, RESULT_SKIP(test_num, 02));
        }
        else {
            val_print (ACS_PRINT_DEBUG,
                        "\n       PCIe Legacy IRQs unmapped. Skipping BDF %llx", bdf);
            val_set_status(pe_index, RESULT_SKIP(test_num, 3));
            continue;
        }

        val_memory_free_aligned(intr_map);
        return;
      }

      /* If test runs for atleast an endpoint */
      test_skip = 0;
      intr_line = intr_map->legacy_irq_map[intr_pin-1].irq_list[0];

      /* Check if the int falls in SPI range */
      if ((intr_line >= 32 && intr_line <= 1019) ||
       (val_gic_espi_supported() && (intr_line >= 4096 &&
                         intr_line <= val_gic_max_espi_val())))  {
          val_print(ACS_PRINT_INFO, "\n Int is SPI", 0);
      }
      else {
        val_print(ACS_PRINT_ERR, "\n Int id %d is not SPI", intr_line);
        val_memory_free_aligned(intr_map);
        val_set_status(pe_index, RESULT_FAIL(test_num, 02));
        return;
      }
  }

  val_memory_free_aligned(intr_map);

  if (test_skip == 1)
      val_set_status(pe_index, RESULT_SKIP(test_num, 01));
  else
      val_set_status(pe_index, RESULT_PASS(test_num, 01));
}

/* Payload for the rule PCI_LI_03 */
static void
payload_secondary(void)
{
  uint32_t status;
  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t dp_type;
  bool     test_skip = 1;
  pcie_device_bdf_table *bdf_tbl_ptr;
  INTR_TRIGGER_INFO_TYPE_e trigger_type;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  tbl_index = 0;
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      /* Check entry is RCiEP/ RCEC,  else move to next BDF. */
      if ((dp_type != RCEC) && (dp_type != RCiEP))
          continue;

      val_print(ACS_PRINT_DEBUG, "\n       BDF - 0x%x", bdf);

      /* If test runs for atleast an endpoint */
      test_skip = 0;

      /* Read GICD_ICFGR register to Check for Level/Edge Sensitive. */
      if (intr_line >= 32 && intr_line <= 1019)
          status = val_gic_get_intr_trigger_type(intr_line, &trigger_type);
      else
          status = val_gic_get_espi_intr_trigger_type(intr_line, &trigger_type);

      if (status) {
          val_set_status(pe_index, RESULT_FAIL(test_num, 01));
          return;
      }

      if (trigger_type != INTR_TRIGGER_INFO_LEVEL_HIGH) {
          val_print(ACS_PRINT_ERR,
                   "\n       Legacy interrupt programmed with incorrect trigger type", 0);
          val_set_status(pe_index, RESULT_FAIL(test_num, 02));
          return;
      }
  }

  if (test_skip == 1)
      val_set_status(pe_index, RESULT_SKIP(test_num, 01));
  else
      val_set_status(pe_index, RESULT_PASS(test_num, 01));
}

uint32_t
p027_entry(uint32_t num_pe)
{
  prereq_status = ACS_STATUS_FAIL;

  num_pe = 1;  /* This test is run on single processor */
  test_num = test_entries[0].test_num;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  prereq_status = val_initialize_test(test_num, test_entries[0].desc, num_pe);
  if (prereq_status != ACS_STATUS_SKIP)
      val_run_test_payload(test_num, num_pe, payload_primary, 0);

  /* get the result from all PE and check for failure */
  prereq_status = val_check_for_error(test_num, num_pe, test_entries[0].rule);

  val_report_status(0, ACS_END(test_num), test_entries[0].rule);
  return prereq_status;
}

uint32_t
p078_entry(uint32_t num_pe)
{  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor
  test_num = test_entries[1].test_num;

  /* The rule covered by this test (PCI_LI_03) has a pre-requisite (PCI_LI_01) */
  /* Check for the status of the pre-requisite before proceeding with the test */
  status = val_check_for_prerequisite(num_pe, prereq_status, &test_entries[0], &test_entries[1]);

  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(test_num, num_pe, payload_secondary, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(test_num, num_pe, test_entries[1].rule);

  val_report_status(0, ACS_END(test_num), test_entries[1].rule);
  return status;
}