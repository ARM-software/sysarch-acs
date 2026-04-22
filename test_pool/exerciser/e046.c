/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
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
#include "acs_smmu.h"
#include "acs_dma.h"
#include "acs_pcie.h"
#include "val_interface.h"
#include "acs_pe.h"
#include "acs_memory.h"
#include "acs_exerciser.h"
#include "acs_pgt.h"
#include "acs_iovirt.h"
#include "acs_pcie_enumeration.h"

#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 105)
#define TEST_RULE  "PCI_MM_07"
#define TEST_DESC  "No extra address translation          "

#define TEST_DATA_NUM_PAGES  1
#define TEST_DATA 0xDE

static void
write_test_data(void *buf, uint32_t size)
{
  uint32_t index;

  for (index = 0; index < size; index++)
    *((char8_t *)buf + index) = TEST_DATA;

  val_data_cache_ops_by_va((addr_t)buf, CLEAN_AND_INVALIDATE);
}

static void
clear_dram_buf(void *buf, uint32_t size)
{
  uint32_t index;

  for (index = 0; index < size; index++)
    *((char8_t *)buf + index) = 0;

  val_data_cache_ops_by_va((addr_t)buf, CLEAN_AND_INVALIDATE);
}

/*
  * Validate that SMMU-backed exercisers do not introduce unintended address translation.
  * - Require at least one SMMU; allocate per-exerciser stage-1 page tables and DMA buffers.
  * - For each exerciser behind an SMMU, map a cacheable buffer into the SMMU and verify the
  *   IOVA is accepted for the stream.
  * - Perform device DMA write/read (to-device then from-device) between halves of the buffer
  *   and compare data to ensure the IOVA mapping preserves attributes without extra translation.
  * - On errors, fail the test; otherwise report pass/skip and tear down mappings and page tables.
  * - The comparsion of DMA transaction makes sure that there are no extra translations occuring
  *   since we are the ones that program the exerciser with the DMA address.
  */
static
void
payload(void)
{
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t dma_len;
  uint32_t instance;
  uint32_t e_bdf;
  uint32_t bdf;
  uint32_t dev_rc_index;
  void *dram_buf_in_virt;
  void *dram_buf_out_virt;
  uint64_t dram_buf_in_phys;
  uint64_t dram_buf_in_iova;
  uint64_t dram_buf_out_iova;
  uint64_t ttbr;
  uint32_t num_exercisers, num_smmus;
  uint32_t device_id, its_id;
  uint32_t page_size = val_memory_page_size();
  memory_region_descriptor_t mem_desc_array[2], *mem_desc;
  pgt_descriptor_t pgt_desc;
  smmu_master_attributes_t master;
  uint32_t test_data_blk_size = page_size * TEST_DATA_NUM_PAGES;
  uint64_t *pgt_base_array;
  uint32_t test_skip = 1;
  uint32_t smmu_backed = 0;
  pcie_device_bdf_table *bdf_tbl_ptr;
  smmu_stream_iova_check_t check;

  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  val_memory_set(&master, sizeof(master), 0);
  val_memory_set(mem_desc_array, sizeof(mem_desc_array), 0);
  mem_desc = &mem_desc_array[0];
  dram_buf_in_phys = 0;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  num_exercisers = val_exerciser_get_info(EXERCISER_NUM_CARDS);
  num_smmus = val_iovirt_get_smmu_info(SMMU_NUM_CTRL, 0);

  if (num_smmus == 0) {
      val_print(WARN, "\n       Skipping: No SMMU present");
      val_set_status(pe_index, RESULT_SKIP(1));
      return;
  }

  pgt_base_array = val_aligned_alloc(MEM_ALIGN_4K, sizeof(uint64_t) * num_exercisers);
  if (!pgt_base_array) {
      val_print(ERROR, "\n       mem alloc failure");
      val_set_status(pe_index, RESULT_FAIL(2));
      return;
  }

  val_memory_set(pgt_base_array, sizeof(uint64_t) * num_exercisers, 0);

  dram_buf_in_virt = val_memory_alloc_pages(TEST_DATA_NUM_PAGES);
  if (!dram_buf_in_virt) {
      val_print(ERROR, "\n       Cacheable mem alloc failure");
      val_memory_free_aligned(pgt_base_array);
      val_set_status(pe_index, RESULT_FAIL(3));
      return;
  }

  dram_buf_in_phys = (uint64_t)val_memory_virt_to_phys(dram_buf_in_virt);
  dram_buf_out_virt = dram_buf_in_virt + (test_data_blk_size / 2);
  dma_len = test_data_blk_size / 2;

  if (val_pe_reg_read_tcr(0, &pgt_desc.tcr)) {
    val_print(ERROR, "\n       TCR read failure");
    goto test_fail;
  }

  if (val_pe_reg_read_ttbr(0, &ttbr)) {
    val_print(ERROR, "\n       TTBR0 read failure");
    goto test_fail;
  }

  for (instance = 0; instance < num_smmus; ++instance)
     val_smmu_enable(instance);

  for (tbl_index = 0; tbl_index < bdf_tbl_ptr->num_entries; tbl_index++)
  {
    bdf = bdf_tbl_ptr->device[tbl_index].bdf;

    dev_rc_index = val_iovirt_get_rc_index(PCIE_EXTRACT_BDF_SEG(bdf));
    if (dev_rc_index == ACS_INVALID_INDEX)
        continue;

    instance = val_exerciser_get_exerciser_instance(dev_rc_index);
    if (instance == ACS_INVALID_INDEX)
      continue;

    if (val_exerciser_init(instance))
        continue;

    e_bdf = val_exerciser_get_bdf(instance);

    master.smmu_index = val_iovirt_get_rc_smmu_index(PCIE_EXTRACT_BDF_SEG(e_bdf),
                                                     PCIE_CREATE_BDF_PACKED(e_bdf));
    if (master.smmu_index == ACS_INVALID_INDEX) {
        val_print(WARN, "\n       Exerciser %4x not behind SMMU", instance);
        continue;
    }

    smmu_backed = 1;

    if (val_iovirt_get_smmu_info(SMMU_CTRL_ARCH_MAJOR_REV, master.smmu_index) != 3) {
        val_print(WARN, "\n       Exerciser %4x not behind SMMUv3", instance);
        continue;
    }

    pgt_desc.pgt_base = (ttbr & AARCH64_TTBR_ADDR_MASK);
    pgt_desc.mair = val_pe_reg_read(MAIR_ELx);
    pgt_desc.stage = PGT_STAGE1;

    if (val_pgt_get_attributes(pgt_desc, (uint64_t)dram_buf_in_virt, &mem_desc->attributes)) {
        val_print(ERROR, "\n       Unable to get memory attributes of the test buffer");
        goto test_fail;
    }

    clear_dram_buf(dram_buf_in_virt, test_data_blk_size);

        if (val_iovirt_get_device_info(PCIE_CREATE_BDF_PACKED(e_bdf),
                                       PCIE_EXTRACT_BDF_SEG(e_bdf),
                                       &device_id, &master.streamid,
                                       &its_id))
            continue;

        mem_desc->virtual_address = (uint64_t)dram_buf_in_virt + instance * test_data_blk_size;
        mem_desc->physical_address = dram_buf_in_phys;
        mem_desc->length = test_data_blk_size;
        mem_desc->attributes |= PGT_STAGE1_AP_RW;

        pgt_desc.ias = val_smmu_get_info(SMMU_IN_ADDR_SIZE, master.smmu_index);
        if ((pgt_desc.ias) == 0) {
            val_print(ERROR, "\n       Input address size of SMMU %d is 0", master.smmu_index);
            goto test_fail;
        }

        pgt_desc.oas = val_smmu_get_info(SMMU_OUT_ADDR_SIZE, master.smmu_index);
        if ((pgt_desc.oas) == 0) {
            val_print(ERROR, "\n       Output address size of SMMU %d is 0", master.smmu_index);
            goto test_fail;
        }

        pgt_desc.pgt_base = (uint64_t) NULL;
        if (val_pgt_create(mem_desc, &pgt_desc)) {
            val_print(ERROR, "\n       Unable to create page table with given attributes");
            goto test_fail;
        }

        pgt_base_array[instance] = pgt_desc.pgt_base;

        if (val_smmu_map(master, pgt_desc))
        {
            val_print(ERROR, "\n       SMMU mapping failed (%x)     ", e_bdf);
            goto test_fail;
        }

        dram_buf_in_iova = mem_desc->virtual_address;
        dram_buf_out_iova = dram_buf_in_iova + (test_data_blk_size / 2);

        check.smmu_index = master.smmu_index;
        check.streamid = master.streamid;
        check.iova = dram_buf_in_iova;
        if (val_smmu_ops(SMMU_CHECK_STREAM_IOVA, &check, NULL)) {
            val_print(ERROR, "\n       IOVA check failed for Exerciser %4x", instance);
            goto test_fail;
        }

    test_skip = 0;

    write_test_data(dram_buf_in_virt, dma_len);

    if (val_exerciser_set_param(DMA_ATTRIBUTES, dram_buf_in_iova, dma_len, instance)) {
        val_print(ERROR, "\n       DMA attributes setting failure %4x", instance);
        goto test_fail;
    }

    val_exerciser_ops(START_DMA, EDMA_TO_DEVICE, instance);

    if (val_exerciser_set_param(DMA_ATTRIBUTES, dram_buf_out_iova, dma_len, instance)) {
        val_print(ERROR, "\n       DMA attributes setting failure %4x", instance);
        goto test_fail;
    }

    val_exerciser_ops(START_DMA, EDMA_FROM_DEVICE, instance);

    if (val_memory_compare(dram_buf_in_virt, dram_buf_out_virt, dma_len)) {
        val_print(ERROR, "\n       Data Comparasion failure for Exerciser %4x", instance);
        goto test_fail;
    }

    clear_dram_buf(dram_buf_in_virt, test_data_blk_size);

  }

  if (!smmu_backed) {
    val_print(WARN, "\n       Skipping: No exerciser behind SMMU");
    val_set_status(pe_index, RESULT_SKIP(2));
  }
  else if (test_skip)
    val_set_status(pe_index, RESULT_SKIP(1));
  else
    val_set_status(pe_index, RESULT_PASS);

  goto test_clean;

test_fail:
  val_set_status(pe_index, RESULT_FAIL(1));

test_clean:
  val_memory_free_pages(dram_buf_in_virt, TEST_DATA_NUM_PAGES);

  for (instance = 0; instance < num_exercisers; ++instance)
  {
    e_bdf = val_exerciser_get_bdf(instance);
    master.smmu_index = val_iovirt_get_rc_smmu_index(PCIE_EXTRACT_BDF_SEG(e_bdf),
                                                     PCIE_CREATE_BDF_PACKED(e_bdf));
    if (master.smmu_index == ACS_INVALID_INDEX)
        continue;
    if (val_iovirt_get_device_info(PCIE_CREATE_BDF_PACKED(e_bdf),
                                   PCIE_EXTRACT_BDF_SEG(e_bdf),
                                   &device_id, &master.streamid,
                                   &its_id))
        continue;

    val_smmu_unmap(master);

    if (pgt_base_array[instance] != 0) {
      pgt_desc.pgt_base = pgt_base_array[instance];
      val_pgt_destroy(pgt_desc);
    }
  }

  for (instance = 0; instance < num_smmus; ++instance)
     val_smmu_disable(instance);

  val_memory_free_aligned(pgt_base_array);
}

uint32_t
e046_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
  {
    if (val_exerciser_test_init() != ACS_STATUS_PASS)
          return val_exerciser_get_init_result(TEST_RULE);
    val_run_test_payload(TEST_NUM, num_pe, payload, 0);
  }
  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
