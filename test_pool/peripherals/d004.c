/** @file
 * Copyright (c) 2016-2018, 2021,2023-2026, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/acs_smmu.h"
#include "val/include/val_interface.h"
#include "val/include/acs_dma.h"
#include "val/include/acs_peripherals.h"


#define TEST_NUM   (ACS_PER_TEST_NUM_BASE + 4)
#define TEST_RULE  "B_PER_09"
#define TEST_DESC  "Check Memory Attributes of DMA        "

#define TEST_NUM1   (ACS_PER_TEST_NUM_BASE + 7)
#define TEST_RULE1  "B_PER_10"
#define TEST_DESC1  "Memory Attribute of I/O coherent DMA  "

/* This test verifies that the memory attributes for DMA traffic must be one of the following:
   - Inner Write-Back, Outer Write-Back, Inner Shareable
   - Inner Non-Cacheable, Outer Non-Cacheable
   - Device type */

static
void
payload_check_dma_mem_attribute(void)
{
  uint32_t target_dev_index;
  void *buffer;
  uint32_t attr, sh;
  int ret;
  bool flag_fail  = 0;
  uint32_t index   = val_pe_get_index_mpid(val_pe_get_mpid());
  target_dev_index = val_dma_get_info(DMA_NUM_CTRL, 0);
  addr_t   dma_addr = 0;
  uint32_t status;

  if (!target_dev_index)
  {
      val_print(ACS_PRINT_TEST, "\n       No DMA controllers detected...    ", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM, 1));
      return;
  }

  /* Iterate all DMA controllers */
  while (target_dev_index)
  {
      target_dev_index--; /* Index is zero based */
      /* Allocate DMA memory based on coherency */
      if (val_dma_get_info(DMA_HOST_COHERENT, target_dev_index))
      {
          status = val_dma_mem_alloc(&buffer, 512, target_dev_index, DMA_COHERENT, &dma_addr);
          if (status == NOT_IMPLEMENTED) {
            val_print(ACS_PRINT_ERR,
                    "\n       pal_dma_mem_alloc is unimplemented, Skipping test.", 0);
            goto test_skip_unimplemented;
          }
      } else {
          status = val_dma_mem_alloc(&buffer, 512, target_dev_index, DMA_NOT_COHERENT, &dma_addr);
          if (status == NOT_IMPLEMENTED) {
            val_print(ACS_PRINT_ERR,
                    "\n       pal_dma_mem_alloc is unimplemented, Skipping test.", 0);
            goto test_skip_unimplemented;
          }
      }
      ret = val_dma_mem_get_attrs(buffer, &attr, &sh);
      if (ret)
      {
          if (ret == NOT_IMPLEMENTED) {
            val_print(ACS_PRINT_ERR,
                    "\n       pal_dma_mem_get_attrs is unimplemented, Skipping test.", 0);
            goto test_skip_unimplemented;
          }
          val_print(ACS_PRINT_ERR,
                    "\n       DMA controller %d: Failed to get"
                    " memory attributes\n",
                    target_dev_index);
          val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
          flag_fail = 1;
          continue;
      }

      if (!((MEM_NORMAL_WB_IN_OUT(attr) && MEM_SH_INNER(sh)) || /* Check Inner Write-Back, Outer
                                                                   Write-Back, Inner Shareable */
          MEM_NORMAL_NC_IN_OUT(attr) ||   /* Check Inner Non-Cacheable, Outer Non-Cacheable*/
          MEM_DEVICE(attr)))              /* Check Device type */
      {
          val_print(ACS_PRINT_INFO,
                    "\n       DMA controller %d: DMA memory must be inner/outer writeback inner "
                    "shareable, inner/outer non-cacheable, or device type\n",
          target_dev_index);
          val_set_status(index, RESULT_FAIL(TEST_NUM, 2));
          flag_fail = 1;
      }
  }
  /* PASS the test if no fail conditions hit */
  if (!flag_fail)
      val_set_status(index, RESULT_PASS(TEST_NUM, 0));
    return;

test_skip_unimplemented:
    val_set_status(index, RESULT_SKIP(TEST_NUM, 2));
}

/* This test verifies I/O coherent DMA traffic must have the attribute
- Inner Write-Back, Outer Write-Back, Inner Shareable */

static
void
payload_check_io_coherent_dma_mem_attribute(void)
{
    uint32_t target_dev_index;
    void *buffer;
    uint32_t attr, sh;
    int ret;
    bool flag_fail = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    addr_t   dma_addr = 0;
    uint32_t status;

    target_dev_index = val_dma_get_info(DMA_NUM_CTRL, 0);

    if (!target_dev_index)
    {
        val_print(ACS_PRINT_TEST, "\n       No DMA controllers detected...    ", 0);
        val_set_status(index, RESULT_SKIP(TEST_NUM1, 1));
        return;
    }

    while (target_dev_index)
    {
        target_dev_index--; /* Index is zero based */
        /* Check only I/O coherent DMA for attributes */
        if (val_dma_get_info(DMA_HOST_COHERENT, target_dev_index))
        {
            status = val_dma_mem_alloc(&buffer, 512, target_dev_index, DMA_COHERENT, &dma_addr);
            if (status == NOT_IMPLEMENTED) {
                val_print(ACS_PRINT_ERR,
                        "\n       pal_dma_mem_alloc is unimplemented, Skipping test.", 0);
                goto test_skip_unimplemented;
            }
            ret = val_dma_mem_get_attrs(buffer, &attr, &sh);
            if (ret)
            {
                val_print(ACS_PRINT_ERR,
                            "\n       DMA controller %d: Failed to get memory attributes\n",
                            target_dev_index);
                val_set_status(index, RESULT_FAIL(TEST_NUM1, 1));
                flag_fail = 1;
                continue;
            }
            /* Check Inner Write-Back, Outer Write-Back, Inner Shareable */
            if (!(MEM_NORMAL_WB_IN_OUT(attr) && MEM_SH_INNER(sh)))
            {
                val_print(ACS_PRINT_INFO,
                            "\n       DMA controller %d: I/O Coherent DMA memory must\n",
                            target_dev_index);
                val_print(ACS_PRINT_INFO,
                            "       be inner/outer writeback, inner shareable\n", 0);
                val_set_status(index, RESULT_FAIL(TEST_NUM1, 2));
                flag_fail = 1;
            }
        }
    }
    /* PASS the test if no fail conditions hit */
    if (!flag_fail)
        val_set_status(index, RESULT_PASS(TEST_NUM1, 0));
    return;

test_skip_unimplemented:
    val_set_status(index, RESULT_SKIP(TEST_NUM1, 2));
}

uint32_t
d004_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  /* This test is run on single processor */
  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload_check_dma_mem_attribute, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);
  return status;
}

uint32_t
d007_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  /* This test is run on single processor */
  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM1, TEST_DESC1, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM1, num_pe, payload_check_io_coherent_dma_mem_attribute, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM1, num_pe, TEST_RULE1);
  val_report_status(0, ACS_END(TEST_NUM1), NULL);
  return status;
}
