/** @file
 * Copyright (c) 2016-2018, 2021, 2023-2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/acs_gic.h"
#include "val/include/acs_memory.h"
#include "val/include/acs_iovirt.h"
#include "val/include/acs_smmu.h"
#include "val/include/acs_pcie_enumeration.h"
#include "val/include/acs_exerciser.h"
#include "val/driver/gic/its/acs_gic_its.h"

#define TEST_NUM   (ACS_EXERCISER_TEST_NUM_BASE + 4)
#define TEST_RULE  "PCI_MSI_2,ITS_DEV_6"
#define TEST_DESC  "MSI(-X) triggers intr with unique ID  "

static uint32_t irq_pending;
static uint32_t lpi_int_id = 0x204C;
static uint32_t instance;

static
void
intr_handler(void)
{
  /* Clear the interrupt pending state */
  irq_pending = 0;

  val_print(ACS_PRINT_INFO, "\n       Received MSI interrupt %x       ", lpi_int_id + instance);
  val_gic_end_of_interrupt(lpi_int_id + instance);
  return;
}

static
void
payload (void)
{

  uint32_t index;
  uint32_t e_bdf = 0;
  uint32_t timeout;
  uint32_t status;
  uint32_t num_cards;
  uint32_t num_smmus;
  uint32_t test_skip = 1;
  uint32_t msi_index = 0;
  uint32_t msi_cap_offset = 0;

  uint32_t device_id = 0;
  uint32_t stream_id = 0;
  uint32_t its_id = 0;
  uint64_t its_base = 0;

  index = val_pe_get_index_mpid (val_pe_get_mpid());

  if (val_gic_get_info(GIC_INFO_NUM_ITS) == 0) {
      val_print(ACS_PRINT_DEBUG, "\n       No ITS, Skipping Test.\n", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM, 1));
      return;
  }

  /* Read the number of excerciser cards */
  num_cards = val_exerciser_get_info(EXERCISER_NUM_CARDS);

  /* Disable all SMMUs */
  num_smmus = val_iovirt_get_smmu_info(SMMU_NUM_CTRL, 0);
  for (instance = 0; instance < num_smmus; ++instance)
     val_smmu_disable(instance);

  for (instance = 0; instance < num_cards; instance++)
  {

    /* if init fail moves to next exerciser */
    if (val_exerciser_init(instance))
        continue;

    /* Get the exerciser BDF */
    e_bdf = val_exerciser_get_bdf(instance);
    val_print(ACS_PRINT_DEBUG, "\n       Exerciser BDF - 0x%x", e_bdf);

    /* Search for MSI-X/MSI Capability */
    if ((val_pcie_find_capability(e_bdf, PCIE_CAP, CID_MSIX, &msi_cap_offset)) &&
        (val_pcie_find_capability(e_bdf, PCIE_CAP, CID_MSI, &msi_cap_offset))) {
      val_print(ACS_PRINT_DEBUG, "\n       No MSI/MSI-X Capability, Skipping for 0x%x", e_bdf);
      continue;
    }

    test_skip = 0;

    /* Get DeviceID & ITS_ID for this device */
    status = val_iovirt_get_device_info(PCIE_CREATE_BDF_PACKED(e_bdf),
                                        PCIE_EXTRACT_BDF_SEG(e_bdf), &device_id,
                                        &stream_id, &its_id);
    if (status) {
        val_print(ACS_PRINT_ERR,
            "\n       Could not get device info for BDF : 0x%x", e_bdf);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
        return;
    }

    status = val_gic_request_msi(e_bdf, device_id, its_id, lpi_int_id + instance, msi_index);
    if (status) {
        val_print(ACS_PRINT_ERR,
            "\n       MSI Assignment failed for bdf : 0x%x", e_bdf);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 2));
        return;
    }

    status = val_gic_install_isr(lpi_int_id + instance, intr_handler);

    if (status) {
        val_print(ACS_PRINT_ERR,
            "\n       Intr handler registration failed Interrupt : 0x%x", lpi_int_id + instance);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 3));
        return;
    }

    /* Set the interrupt trigger status to pending */
    irq_pending = 1;

    /* Get ITS Base for current ITS */
    if (val_gic_its_get_base(its_id, &its_base)) {
        val_print(ACS_PRINT_ERR,
            "\n       Could not find ITS Base for its_id : 0x%x", its_id);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 4));
        return;
    }

    /* Part 1 : ITS_DEV_6 */
    /* Trigger the interrupt by writing to GITS_TRANSLATER from PE */
    val_mmio_write(its_base + GITS_TRANSLATER, (lpi_int_id - ARM_LPI_MINID) + instance);

    /* PE busy polls to check the completion of interrupt service routine */
    timeout = TIMEOUT_MEDIUM;
    while ((--timeout > 0) && irq_pending)
        {};

    /* Interrupt must not be generated */
    if (irq_pending == 0) {
        val_print(ACS_PRINT_ERR,
            "\n       Interrupt triggered from PE for bdf : 0x%x, ", e_bdf);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 5));
        val_gic_free_msi(e_bdf, device_id, its_id, lpi_int_id + instance, msi_index);
        return;
    }

    /* Part 2: PCI_MSI_2 */
    /* Trigger the interrupt for this Exerciser instance */
    val_exerciser_ops(GENERATE_MSI, msi_index, instance);

    /* PE busy polls to check the completion of interrupt service routine */
    timeout = TIMEOUT_LARGE;
    while ((--timeout > 0) && irq_pending)
        {};

    if (timeout == 0) {
        val_print(ACS_PRINT_ERR,
            "\n       Interrupt trigger failed for : 0x%x, ", lpi_int_id + instance);
        val_print(ACS_PRINT_ERR,
            "BDF : 0x%x   ", e_bdf);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 6));
        val_gic_free_msi(e_bdf, device_id, its_id, lpi_int_id + instance, msi_index);
        return;
    }

    /* Clear Interrupt and Mappings */
    val_gic_free_msi(e_bdf, device_id, its_id, lpi_int_id + instance, msi_index);

  }

  if (test_skip) {
    val_set_status(index, RESULT_SKIP(TEST_NUM, 2));
    return;
  }

  /* Pass Test */
  val_set_status(index, RESULT_PASS(TEST_NUM, 1));

}

uint32_t
e004_entry(void)
{

  uint32_t status = ACS_STATUS_FAIL;

  uint32_t num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
