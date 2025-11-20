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
#include "val/include/acs_pe.h"
#include "val/include/acs_pcie.h"
#include "val/include/acs_memory.h"
#include "val/include/acs_pcie_enumeration.h"
#include "val/include/val_interface.h"
#include "val/include/acs_exerciser.h"

static const
test_config_t test_entries[] = {
    { ACS_EXERCISER_TEST_NUM_BASE + 26, "Check Inbound writes seen in order    ", "S_PCIe_07"},
    { ACS_EXERCISER_TEST_NUM_BASE + 32, "Check ordered writes flush prev writes", "S_PCIe_08"}
};

/* Declare and define struct - passed as argument to payload */
typedef struct {
    uint32_t check2_only;
    uint32_t test_num;
} payload_data_t;

#define DMA_BUFF_LEN 0x8
#define BUFF_LEN 0x12
#define MAX_LEN  24

static
void
write_test_data(uint64_t pgt_base_addr)
{

  /* The final data after a series of transactions should be
   * in the order of test_data1, test_data2 and test_data3
   * This is the data against which the final memory location
   * will be verified against
  */
  uint64_t test_data1 = 0x5678567856780000;
  uint64_t test_data2 = 0x5678567856785678;
  uint64_t test_data3 = 0xABCDC0DE12345678;

  val_mmio_write64((uint64_t)pgt_base_addr, test_data1);
  val_mmio_write64((uint64_t)pgt_base_addr+0x8, test_data2);
  val_mmio_write64((uint64_t)pgt_base_addr+0x10, test_data3);

}

static
void
payload(void *arg)
{
  uint32_t pe_index;
  uint32_t instance;
  uint32_t e_bdf;
  uint32_t num_exercisers;
  uint64_t *pgt_base_array;
  uint64_t *pgt_base;
  uint64_t dma_buffer = 0xABCDC0DE12345678;
  uint64_t test_data  = 0x5678567856785678;
  uint8_t  j = 0x2, i = 0;
  bool     test_skip = 1;
  payload_data_t *payload_data = (payload_data_t *)arg;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  num_exercisers = val_exerciser_get_info(EXERCISER_NUM_CARDS);

  /* Allocate an array to store base addresses of page tables allocated for
   * all exercisers
   * pgt_base_array = array of data on which memeory operations will be performed
   * pgt_base = array of data against which we will verify the final values
   */
  pgt_base_array = val_aligned_alloc(MEM_ALIGN_4K, sizeof(uint64_t) * num_exercisers);
  if (!pgt_base_array) {
      val_print(ACS_PRINT_ERR, "\n       mem alloc failure for pgt_base_array", 0);
      val_set_status(pe_index, RESULT_FAIL(payload_data->test_num, 01));
      return;
  }

  pgt_base = val_aligned_alloc(MEM_ALIGN_4K, sizeof(uint64_t) * num_exercisers);
  if (!pgt_base) {
      val_print(ACS_PRINT_ERR, "\n       mem alloc failure for pgt_base", 0);
      val_set_status(pe_index, RESULT_FAIL(payload_data->test_num, 02));
      val_memory_free_aligned(pgt_base_array);
      return;
  }

  val_memory_set(pgt_base_array, sizeof(uint64_t) * num_exercisers, 0);
  val_memory_set(pgt_base, sizeof(uint64_t) * num_exercisers, 0);

  for (instance = 0; instance < num_exercisers; ++instance) {

      /* if init fail moves to next exerciser */
      if (val_exerciser_init(instance))
          continue;

      /* Test runs for atleast one exerciser */
      test_skip = 0;

      /* Get exerciser bdf */
      e_bdf = val_exerciser_get_bdf(instance);
      val_print(ACS_PRINT_DEBUG, "\n       Exercise BDF - 0x%x", e_bdf);

      /* S_PCIe_07 requires only the second part of the check,
         while S_PCIe_08 requires both the first and second parts.
      */
      if (payload_data->check2_only)
        goto disable_ro;

      /* Check 1: Enable Relaxed ordering by setting RO = 1 and
       * Send a set of additional writes to address. Before sending
       * the next transaction with RO = 0, all the previous writes
       * should have been completed
       */

      /* Enable Relaxed Ordering */
      val_pcie_enable_ordering(e_bdf);
      val_mmio_write64((uint64_t)pgt_base_array, 0);
      if (val_exerciser_set_param(DMA_ATTRIBUTES, (uint64_t)&dma_buffer, 8, instance)) {
          val_print(ACS_PRINT_ERR, "\n       DMA attributes setting failure %4x", instance);
          goto test_fail;
      }

      /* Trigger DMA from input buffer to exerciser memory */
      if (val_exerciser_ops(START_DMA, EDMA_TO_DEVICE, instance)) {
          val_print(ACS_PRINT_ERR, "\n       DMA write failure to exerciser %4x", instance);
          goto test_fail;
      }

      while (i < DMA_BUFF_LEN) {

          if (val_exerciser_set_param(DMA_ATTRIBUTES, (uint64_t)(pgt_base_array) + (uint8_t)i,
                                      2, instance))
          {
              val_print(ACS_PRINT_ERR, "\n       DMA attributes setting failure %4x", instance);
              goto test_fail;
          }

          /* Trigger DMA from exerciser memory to output buffer*/
          if (val_exerciser_ops(START_DMA, EDMA_FROM_DEVICE, instance)) {
              val_print(ACS_PRINT_ERR, "\n       DMA read failure from exerciser %4x", instance);
              goto test_fail;
          }
          i = i + 2;

      }
      i = 0;

disable_ro:
      /* Check 2: Disable Relaxed ordering by setting RO = 0 and
       * Send a set of staggered writes to address. The transactions
       * must be recieved in the same order in which it was initiated
       */

      /*Disable Relaxed Ordering*/
      val_pcie_disable_ordering(e_bdf);

      if (!payload_data->check2_only) {
          /* All the previous writes sent must be completed, before next transaction */
          if (val_memory_compare(&test_data, pgt_base_array, 8)) {
              val_print(ACS_PRINT_ERR,
                                  "\n      Data Comparasion failure for Exerciser %4x", instance);
              goto test_fail;
          }

          goto test_pass;
      }

      /* Initialize the sender buffer with test specific data */
      write_test_data((uint64_t)pgt_base);

      val_mmio_write64((uint64_t)pgt_base_array, 0);
      if (val_exerciser_set_param(DMA_ATTRIBUTES, (uint64_t)&dma_buffer, 8, instance)) {
          val_print(ACS_PRINT_ERR, "\n       DMA attributes setting failure %4x", instance);
          goto test_fail;
      }

      /* Trigger DMA from input buffer to exerciser memory */
      if (val_exerciser_ops(START_DMA, EDMA_TO_DEVICE, instance)) {
          val_print(ACS_PRINT_ERR, "\n       DMA write failure to exerciser %4x", instance);
          goto test_fail;
      }

      /* Dump a set of staggered writes from the exerciser memory to the destination address
       * If the writes are in order the output the transactions have been viewed in same order
       * as it was being sent by the exerciser*/
      while (j < BUFF_LEN) {

          if (val_exerciser_set_param(DMA_ATTRIBUTES, (uint64_t)(pgt_base_array) + (uint8_t)j,
                                      8, instance))
          {
              val_print(ACS_PRINT_ERR, "\n       DMA attributes setting failure %4x", instance);
              goto test_fail;
          }

          /* Trigger DMA from exerciser memory to output buffer*/
          if (val_exerciser_ops(START_DMA, EDMA_FROM_DEVICE, instance)) {
              val_print(ACS_PRINT_ERR, "\n       DMA read failure from exerciser %4x", instance);
              goto test_fail;
          }
          j = j + 2;
      }
      j = 0x2;

      if (val_memory_compare(pgt_base, pgt_base_array, MAX_LEN)) {
          val_print(ACS_PRINT_ERR, "\n      Data Comparasion failure for Exerciser %4x", instance);
          goto test_fail;
      }
  }

test_pass:
  val_set_status(pe_index, RESULT_PASS(payload_data->test_num, 01));
  goto test_clean;

  if (test_skip)
      val_set_status(pe_index, RESULT_SKIP(payload_data->test_num, 01));

test_fail:
  val_set_status(pe_index, RESULT_FAIL(payload_data->test_num, 03));
test_clean:
  val_memory_free_aligned(pgt_base_array);
  val_memory_free_aligned(pgt_base);
}

uint32_t
e026_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  /* Run test on single PE */
  num_pe = 1;

  payload_data_t data = {.test_num = test_entries[0].test_num, .check2_only = 1};


  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(test_entries[0].test_num, test_entries[0].desc, num_pe);
  if (status != ACS_STATUS_SKIP) {
      if (val_exerciser_test_init() != ACS_STATUS_PASS)
          return TEST_SKIP_VAL;
      val_run_test_configurable_payload(&data, payload);
  }

  /* get the result from all PE and check for failure */
  status = val_check_for_error(test_entries[0].test_num, num_pe, test_entries[0].rule);

  val_report_status(0, ACS_END(test_entries[0].test_num), test_entries[0].rule);
  return status;
}

uint32_t
e032_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  /* Run test on single PE */
  num_pe = 1;

  payload_data_t data = {.test_num = test_entries[1].test_num, .check2_only = 0};


  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(test_entries[1].test_num, test_entries[1].desc, num_pe);
  if (status != ACS_STATUS_SKIP) {
      if (val_exerciser_test_init() != ACS_STATUS_PASS)
          return TEST_SKIP_VAL;
      val_run_test_configurable_payload(&data, payload);
  }

  /* get the result from all PE and check for failure */
  status = val_check_for_error(test_entries[1].test_num, num_pe, test_entries[1].rule);

  val_report_status(0, ACS_END(test_entries[1].test_num), test_entries[1].rule);
  return status;
}
