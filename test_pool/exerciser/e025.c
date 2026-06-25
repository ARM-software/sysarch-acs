/** @file
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
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
#include "acs_pcie_enumeration.h"
#include "acs_pcie.h"
#include "acs_pe.h"
#include "acs_smmu.h"
#include "acs_memory.h"
#include "acs_exerciser.h"
#include "val_interface.h"

#define TEST_NUM   (ACS_EXERCISER_TEST_NUM_BASE + 25)
#define TEST_DESC  "Check 2/4/8 Bytes targeted writes     "
#define TEST_RULE  "S_PCIe_04"

#define TEST_DMA_BUF_SIZE      SIZE_4KB
#define TEST_DMA_SRC_SIZE      (SIZE_4KB/2)
#define TEST_DMA_DST_SIZE      (SIZE_4KB/2)
#define TEST_DMA_PATTERN 0xABCDC0DEABCDC0DE

uint64_t test_dma_src_buffer[TEST_DMA_SRC_SIZE/8];
uint64_t test_dma_dst_buffer[TEST_DMA_DST_SIZE/8];

static
uint32_t
get_target_exer_bdf(uint32_t req_rp_bdf, uint32_t *tgt_e_bdf,
                    uint32_t *tgt_rp_bdf, uint64_t *bar_base, uint32_t *tgt_instance)
{

  uint32_t erp_bdf;
  uint32_t e_bdf;
  uint32_t instance;
  uint32_t req_rp_ecam_index;
  uint32_t erp_ecam_index;
  uint32_t status;

  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS);

  while (instance-- != 0)
  {
      /* if init fail moves to next exerciser */
      if (val_exerciser_init(instance))
          continue;

      e_bdf = val_exerciser_get_bdf(instance);

      /* Read e_bdf BAR Register to get the Address to perform P2P */
      /* If No BAR Space, continue */
      val_pcie_get_mmio_bar(e_bdf, bar_base);
      if (*bar_base == 0)
          continue;

      /* Get RP of the exerciser */
      if (val_pcie_get_rootport(e_bdf, &erp_bdf))
          continue;

      if (req_rp_bdf != erp_bdf)
      {
          status = val_pcie_get_ecam_index(req_rp_bdf, &req_rp_ecam_index);
          if (status)
          {
             val_print(ERROR,
                       "\n       Error Ecam index for req RP BDF: 0x%x", req_rp_bdf);
             goto clean_fail;
          }

          status = val_pcie_get_ecam_index(erp_bdf, &erp_ecam_index);
          if (status)
          {
             val_print(ERROR, "\n       Error Ecam index for tgt RP BDF: 0x%x", erp_bdf);
             goto clean_fail;
          }

          if (req_rp_ecam_index != erp_ecam_index)
              continue;

          *tgt_e_bdf = e_bdf;
          *tgt_rp_bdf = erp_bdf;

          /* Enable Bus Master Enable */
          val_pcie_enable_bme(e_bdf);
          /* Enable Memory Space Access */
          val_pcie_enable_msa(e_bdf);

          *tgt_instance = instance;

          return ACS_STATUS_PASS;
      }
  }

clean_fail:
  /* Return failure if No Such Exerciser Found */
  *tgt_e_bdf = 0;
  *tgt_rp_bdf = 0;
  *bar_base = 0;
  return ACS_STATUS_FAIL;
}

static
uint32_t
check_p2p_sequence(uint64_t dma_src, uint32_t tgt_instance, uint32_t req_instance,
               uint64_t bar_base, uint32_t size)
{
  uint32_t status;
  uint64_t transaction_data;
  uint64_t idx;

  /* Copy the contents of the memory to requestor exercise's memory */
  if (val_exerciser_set_param(DMA_ATTRIBUTES, (uint64_t)dma_src, size, req_instance))
      return ACS_STATUS_FAIL;

  if (val_exerciser_ops(START_DMA, EDMA_TO_DEVICE, req_instance))
      return ACS_STATUS_FAIL;

  /* Set the destination buffer as BAR base address of target exerciser */
  if (val_exerciser_set_param(DMA_ATTRIBUTES, (uint64_t)bar_base, size, req_instance))
      return ACS_STATUS_FAIL;

  /* Start the transaction monitoring in the target exerciser */
  status = val_exerciser_ops(START_TXN_MONITOR, CFG_READ, tgt_instance);
  if (status == PCIE_CAP_NOT_FOUND)
  {
      val_print(ERROR, "\n       Transaction Monitoring capability not found");
      return ACS_STATUS_FAIL;
  }

  /* Copy the contents from requestor exerciser to target exerciser's BAR address */
  if (val_exerciser_ops(START_DMA, EDMA_FROM_DEVICE, req_instance))
      return ACS_STATUS_FAIL;

  /* Stop the transaction monitoring in the target exerciser */
  status = val_exerciser_ops(STOP_TXN_MONITOR, CFG_READ, tgt_instance);
  if (status == PCIE_CAP_NOT_FOUND)
  {
      val_print(ERROR, "\n       Transaction Monitoring capability not found");
      return ACS_STATUS_FAIL;
  }

  /* Compare the transaction data */
  status = val_exerciser_get_param(DATA_ATTRIBUTES, (uint64_t *)&transaction_data,
                                    &idx, tgt_instance);
  if (status)
  {
      val_print(ERROR, "\n       Read Transaction data failed");
      return ACS_STATUS_FAIL;
  }
  if (val_memory_compare((void *)&transaction_data, (void *)dma_src, size))
  {
      val_print(ERROR,
                "\n       Data mismatch for target exerciser instance: %x", tgt_instance);
      val_print(ERROR, " with value: %x\n", transaction_data);
      return ACS_STATUS_FAIL;
  }

  return ACS_STATUS_PASS;
}

static
uint32_t
check_host_sequence(uint64_t dma_src, uint64_t dma_dst, uint32_t req_instance, uint32_t size)
{
  uint64_t tmp_zerobuf = 0;

  /* Copy the contents of host memory to requestor exerciser's memory. */
  if (val_exerciser_set_param(DMA_ATTRIBUTES, dma_src, size, req_instance))
      return ACS_STATUS_FAIL;

  if (val_exerciser_ops(START_DMA, EDMA_TO_DEVICE, req_instance))
      return ACS_STATUS_FAIL;

  // Clear the destination buffer; cleaning the first 8 bytes is sufficient
  *((uint64_t *)dma_dst) = 0;
  val_pe_cache_clean_invalidate_range(dma_dst, size);

  /* Copy the contents from requestor exerciser to host memory. */
  if (val_exerciser_set_param(DMA_ATTRIBUTES, dma_dst, size, req_instance))
      return ACS_STATUS_FAIL;

  if (val_exerciser_ops(START_DMA, EDMA_FROM_DEVICE, req_instance))
      return ACS_STATUS_FAIL;

  val_pe_cache_invalidate_range(dma_dst, size);

  if ((val_memory_compare((void *) dma_src, (void *)dma_dst, size)) ||
      (val_memory_compare((void *)(((uint8_t *) dma_dst) + size),
                          (void *)&tmp_zerobuf, 8-size)))
  {
      val_print(ERROR,
                "\n       Host memory data mismatch for exerciser instance: %x", req_instance);
      val_print(ERROR, " with value: %x", ((uint32_t *)(dma_dst))[0]);
      return ACS_STATUS_FAIL;
  }

  return ACS_STATUS_PASS;
}

static const char *printStatus(uint32_t ret_status)
{
  switch (ret_status) {
  case ACS_STATUS_FAIL:
      return "FAILED";
  case ACS_STATUS_SKIP:
      return "SKIPPED";
  case ACS_STATUS_PASS:
      return "PASSED";
  default:
      return "UNKNOWN";
  }
}

static
uint32_t
host_dma_test(uint32_t req_instance, uint64_t dma_src, uint64_t dma_dst)
{
  uint32_t req_e_bdf;
  uint32_t ret_status = ACS_STATUS_SKIP;
  // Data sizes = 2Bytes, 4Bytes, 8Bytes
  uint32_t datasizes[3] = {2, 4, 8};
  uint32_t i;
  uint32_t test_fail = 0;
  uint32_t count = (sizeof(datasizes)/sizeof(datasizes[0]));

  val_print(DEBUG, "\n   Testing endpoint to Host DMA\n");
  while (req_instance-- != 0)
  {
      /* if init fail moves to next exerciser */
      if (val_exerciser_init(req_instance))
          continue;

      req_e_bdf = val_exerciser_get_bdf(req_instance);
      val_print(DEBUG, "       Requester exerciser BDF - 0x%x\n", req_e_bdf);

      for (i = 0; i < count; i++)
      {
          ret_status = check_host_sequence(dma_src, dma_dst, req_instance, datasizes[i]);
          if (ret_status != ACS_STATUS_PASS)
          {
              val_print(ERROR,
                    "       Failed for %dByte host memory transaction from exerciser: %x\n",
                     datasizes[i], req_instance);
              // Continue running the tests even if this fails
              test_fail++;
          }
      }
  }
  // Check whether test failed in any instance
  if (test_fail)
  {
      ret_status = ACS_STATUS_FAIL;
  }
  // else test is either skip or pass
  val_print(INFO, "Endpoint to Host DMA test  %s\n", printStatus(ret_status));
  return ret_status;
}

static
uint32_t
p2p_dma_test(uint32_t req_instance, uint64_t dma_src)
{
  uint32_t ret_status = ACS_STATUS_SKIP;
  uint32_t req_e_bdf, req_rp_bdf, tgt_e_bdf, tgt_rp_bdf, tgt_instance;
  uint64_t bar_base;
  // Data sizes = 2Bytes, 4Bytes, 8Bytes
  uint32_t datasizes[3] = {2, 4, 8};
  uint32_t i;
  uint32_t test_fail = 0;
  uint32_t count = (sizeof(datasizes)/sizeof(datasizes[0]));


  val_print(DEBUG, "\n   Testing endpoint to endpoint DMA\n");

  while (req_instance-- != 0)
  {
      /* if init fail moves to next exerciser */
      if (val_exerciser_init(req_instance))
          continue;

      req_e_bdf = val_exerciser_get_bdf(req_instance);
      val_print(DEBUG, "       Requester exerciser BDF - 0x%x\n", req_e_bdf);

      /* Get RP of the exerciser */
      if (val_pcie_get_rootport(req_e_bdf, &req_rp_bdf))
          continue;

      /* Find another exerciser on other rootport,
         Break from the test if no such exerciser if found */
      if (get_target_exer_bdf(req_rp_bdf, &tgt_e_bdf, &tgt_rp_bdf, &bar_base, &tgt_instance))
          continue;

      for (i = 0; i < count; i++)
      {
          /* Passing the buffer address as a value, to be configured as the source address for
          * performing DMA */
          ret_status = check_p2p_sequence(dma_src, tgt_instance, req_instance,
                                                   bar_base, datasizes[i]);
          if (ret_status != ACS_STATUS_PASS)
          {
              val_print(ERROR,
                      "       Failed for %dBytes transaction from exerciser: %x\n",
                           datasizes[i], req_instance);
              test_fail++;
          }
      }

  }
  // Check whether test failed in any instance
  if (test_fail)
  {
      ret_status = ACS_STATUS_FAIL;
  }
  // else test is either skip or pass
  val_print(INFO, "Endpoint to Endpoint DMA test  %s\n", printStatus(ret_status));
  return ret_status;
}

static
void
set_status(uint32_t pe_index, uint32_t ret_status, int32_t instance)
{
    switch (ret_status) {
    case ACS_STATUS_PASS:
        val_set_status(pe_index, RESULT_PASS);
        break;
    case ACS_STATUS_FAIL:
        val_set_status(pe_index, RESULT_FAIL(instance));
        break;
    case ACS_STATUS_SKIP:
        val_set_status(pe_index, RESULT_SKIP(instance));
        break;
    }
    return;
}

static
void
payload(void)
{
  uint32_t pe_index;
  uint32_t req_instance;
  uint32_t p2p_status;
  uint32_t test_status = 0;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  if (val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0) == 0) {
      val_print(DEBUG, "\n       No ECAM region found. Skipping test");
      val_set_status(pe_index, RESULT_SKIP(1));
      return;
  }

  do
  {
      req_instance = val_exerciser_get_info(EXERCISER_NUM_CARDS);

      // Initialize the DMA source buffer with 8 bytes, as this is the maximum transfer size
      test_dma_src_buffer[0] = TEST_DMA_PATTERN;
      val_pe_cache_clean_invalidate_range((uint64_t)test_dma_src_buffer, 8);

      val_print(DEBUG, "\nPerforming Endpoint to Host DMA transfer\n");
      test_status = host_dma_test(req_instance,
                                   (uint64_t) test_dma_src_buffer,
                                   (uint64_t) test_dma_dst_buffer);
      if (test_status != ACS_STATUS_PASS) {
          set_status(pe_index, test_status, 1);
          break;
      }
      else {
          val_print(DEBUG, "Endpoint to Host DMA transfer Success\n");
      }

      /* Check If PCIe Hierarchy supports P2P. */
      p2p_status = val_pcie_p2p_support();
      if (p2p_status == ACS_STATUS_PASS)
      {
          val_print(DEBUG, "Platform supports PCIe P2P dma transfer ");
          val_print(DEBUG, "performing Endpoint to Endpoint transfer\n");
          test_status = p2p_dma_test(req_instance, (uint64_t)test_dma_src_buffer);
          if (test_status != ACS_STATUS_PASS) {
              set_status(pe_index, test_status, 2);
              break;
          }
          else {
              val_print(DEBUG, "Endpoint to Endpoint DMA transfer Success\n");
          }
      }
      else
      {
          val_print(DEBUG, "Platform do not support PCIe P2P dma transfer\n");
      }
  } while (0);

  if (test_status == ACS_STATUS_PASS)
      set_status(pe_index, test_status, 0);
  return;
}


uint32_t
e025_entry(uint32_t num_pe)
{
  /* Run test on single PE */
  num_pe = 1;
  uint32_t status = ACS_STATUS_FAIL;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP) {
      if (val_exerciser_test_init() != ACS_STATUS_PASS)
          return val_exerciser_get_init_result(TEST_RULE);
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);
  }

  /* Get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
