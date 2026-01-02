/** @file
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/acs_memory.h"
#include "val/include/acs_peripherals.h"
#include "val/include/acs_pe.h"
#include "val/include/acs_pcie_enumeration.h"
#include "val/include/acs_exerciser.h"

#define TEST_NUM   (ACS_EXERCISER_TEST_NUM_BASE + 39)
#define TEST_DESC  "PCIe Normal Memory access check       "
#define TEST_RULE  "PCI_MM_03"

#define TEST_DATA 0xDEADDAED
static const ARM_NORMAL_MEM ARM_NORMAL_MEM_ARRAY[] = {NORMAL_NC, NORMAL_WT};

static void *branch_to_test;


static
void
esr(uint64_t interrupt_type, void *context)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Update the ELR to point to next instrcution */
  val_pe_update_elr(context, (uint64_t)branch_to_test);

  val_print(ACS_PRINT_ERR, "\n       Received Exception of type %d", interrupt_type);
  val_set_status(index, RESULT_FAIL(TEST_NUM, 02));
}

static
void
payload(void)
{
  char *baseptr;
  uint32_t idx;
  uint32_t pe_index;
  uint32_t bdf;
  uint32_t status;
  uint32_t instance;
  bool     test_skip = 1;
  exerciser_data_t e_data;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  status = val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
  status |= val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);
  branch_to_test = &&test_fail;
  if (status)
  {
      val_print(ACS_PRINT_ERR, "\n       Failed in installing the exception handler", 0);
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 01));
      return;
  }

  /* Read the number of excerciser cards */
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS);

  while (instance-- != 0) {

      /* if init fail moves to next exerciser */
      if (val_exerciser_init(instance))
          continue;

    bdf = val_exerciser_get_bdf(instance);
    val_print(ACS_PRINT_DEBUG, "\n       Exerciser BDF - 0x%x", bdf);

    /* Get BAR 0 details for this instance */
    status = val_exerciser_get_data(EXERCISER_DATA_MMIO_SPACE, &e_data, instance);
    if (status == NOT_IMPLEMENTED) {
        val_print(ACS_PRINT_ERR, "\n       pal_exerciser_get_data() for MMIO not implemented", 0);
        goto test_fail;
    } else if (status) {
        val_print(ACS_PRINT_ERR, "\n       Exerciser %d data read error     ", instance);
        goto test_fail;
    }

    /* Do additional checks if the BAR is pcie prefetchable mmio space */
    if (e_data.bar_space.type == MMIO_PREFETCHABLE) {

        /* test runs on atleast one device */
        test_skip = 0;

        /* Map the mmio space to ARM normal memory in MMU page tables */
        for (idx = 0; idx < sizeof(ARM_NORMAL_MEM_ARRAY)/sizeof(ARM_NORMAL_MEM_ARRAY[0]); idx++) {
            status = val_memory_ioremap((void *)e_data.bar_space.base_addr,
                                                    512,
                                                    ARM_NORMAL_MEM_ARRAY[idx], (void **)&baseptr);
            /* Handle unimplemented PAL -> SKIP gracefully */
            if (status == NOT_IMPLEMENTED) {
                val_print(ACS_PRINT_ERR,
                        "\n       pal_memory_ioremap not implemented, skipping test.", 0);
                goto test_skip_unimplemented;
            }

            if (!baseptr) {
                val_print(ACS_PRINT_ERR,
                            "\n       Failed in BAR ioremap for instance %x", instance);
                goto test_fail;
            }

            val_pcie_enable_msa(bdf);
            val_pcie_clear_urd(bdf);

            /* Write predefined data to an unaligned address in mmio space and read it back */
            val_mmio_write((addr_t)(baseptr+3), TEST_DATA);
            if (TEST_DATA != val_mmio_read((addr_t)(baseptr+3))) {
                val_print(ACS_PRINT_ERR,
                        "\n       Exerciser BAR space access error %x", instance);
                goto test_fail;
            }

            /* Remove BAR mapping from MMU page tables */
            val_memory_unmap(baseptr);
        }
    }
  }

  if (test_skip) {
      val_print(ACS_PRINT_DEBUG,
                "\n       No exerciser with prefetchable mmio space, Skipping test", 0);
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
      return;
  }

  val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));
  return;

 test_skip_unimplemented:
  val_memory_unmap(baseptr);
  val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 02));
  return;

test_fail:
  val_memory_unmap(baseptr);
  val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 03));
  return;

}

uint32_t
e039_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  /* Run test on single PE */
  num_pe = 1;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP) {
     if (val_exerciser_test_init() != ACS_STATUS_PASS)
         return TEST_SKIP_VAL;
     val_run_test_payload(TEST_NUM, num_pe, payload, 0);
  }

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
