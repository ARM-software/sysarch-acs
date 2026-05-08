/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0
 *
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
#include "acs_cxl.h"
#include "acs_exerciser.h"
#include "acs_memory.h"
#include "acs_pgt.h"
#include "acs_pe.h"
#include "acs_iovirt.h"
#include "acs_pcie.h"
#include "acs_smmu.h"

#define TEST_NUM   (ACS_EXERCISER_TEST_NUM_BASE + 40)
#define TEST_RULE  "CXL_02"
#define TEST_DESC  "CXL path ATS translation + DMA check  "

#define TEST_DATA_NUM_PAGES  1
#define TEST_DATA 0xDE

static
void
write_test_data(void *buf, uint32_t size)
{
  uint32_t idx;

  for (idx = 0; idx < size; idx++)
    *((char8_t *)buf + idx) = TEST_DATA;

  val_data_cache_ops_by_va((addr_t)buf, CLEAN_AND_INVALIDATE);
}

static
void
clear_dram_buf(void *buf, uint32_t size)
{
  uint32_t idx;

  for (idx = 0; idx < size; idx++)
    *((char8_t *)buf + idx) = 0;

  val_data_cache_ops_by_va((addr_t)buf, CLEAN_AND_INVALIDATE);
}

static
uint32_t
run_cxl_path_ats_dma(uint32_t e_bdf)
{
  uint32_t rc_index;
  uint32_t instance;
  uint32_t cap_base = 0;
  uint32_t reg_value = 0;
  uint32_t dma_len;
  uint32_t page_size = val_memory_page_size();
  uint64_t translated_addr;
  uint64_t m_vir_addr;
  uint64_t ttbr;
  uint32_t device_id;
  uint32_t its_id;
  void *dram_buf_virt = NULL;
  void *dram_buf_out_virt;
  uint64_t dram_buf_phys;
  uint64_t dram_buf_iova;
  uint64_t dram_buf_out_iova;
  pgt_descriptor_t pgt_desc;
  memory_region_descriptor_t mem_desc;
  smmu_master_attributes_t master;
  uint8_t ats_enabled = 0;
  uint8_t smmu_mapped = 0;
  uint8_t smmu_enabled = 0;

  val_memory_set(&master, sizeof(master), 0);
  val_memory_set(&mem_desc, sizeof(mem_desc), 0);
  val_memory_set(&pgt_desc, sizeof(pgt_desc), 0);

  rc_index = val_iovirt_get_rc_index(PCIE_EXTRACT_BDF_SEG(e_bdf));
  if (rc_index == ACS_INVALID_INDEX)
    return ACS_STATUS_FAIL;

  instance = val_exerciser_get_exerciser_instance(rc_index);
  if (instance == ACS_INVALID_INDEX)
    return ACS_STATUS_FAIL;

  if (val_exerciser_init(instance))
    return ACS_STATUS_FAIL;

  if (val_pcie_find_capability(e_bdf, PCIE_ECAP, ECID_ATS, &cap_base) != PCIE_SUCCESS)
    return ACS_STATUS_FAIL;

  val_pcie_read_cfg(e_bdf, cap_base + ATS_CTRL, &reg_value);
  reg_value |= ATS_CACHING_EN;
  val_pcie_write_cfg(e_bdf, cap_base + ATS_CTRL, reg_value);
  ats_enabled = 1;

  dram_buf_virt = val_memory_alloc_pages(TEST_DATA_NUM_PAGES);
  if (!dram_buf_virt) {
      val_print(ERROR, "\n       Cacheable mem alloc failure");
      goto test_fail;
  }

  dram_buf_phys = (uint64_t)val_memory_virt_to_phys(dram_buf_virt);
  dma_len = (page_size * TEST_DATA_NUM_PAGES) / 2;
  dram_buf_out_virt = dram_buf_virt + dma_len;

  if (val_pe_reg_read_tcr(0, &pgt_desc.tcr) || val_pe_reg_read_ttbr(0, &ttbr)) {
    val_print(ERROR, "\n       TCR read failure");
    goto test_fail;
  }

  pgt_desc.stage = PGT_STAGE1;
  pgt_desc.mair = val_pe_reg_read(MAIR_ELx);
  pgt_desc.pgt_base = (ttbr & AARCH64_TTBR_ADDR_MASK);

  if (val_pgt_get_attributes(pgt_desc, (uint64_t)dram_buf_virt, &mem_desc.attributes))
    goto test_fail;

  master.smmu_index = val_iovirt_get_rc_smmu_index(PCIE_EXTRACT_BDF_SEG(e_bdf),
                                                   PCIE_CREATE_BDF_PACKED(e_bdf));
  if (master.smmu_index == ACS_INVALID_INDEX)
    goto test_fail;

  if (val_iovirt_get_device_info(PCIE_CREATE_BDF_PACKED(e_bdf),
                                 PCIE_EXTRACT_BDF_SEG(e_bdf),
                                 &device_id, &master.streamid, &its_id))
    goto test_fail;

  pgt_desc.ias = val_smmu_get_info(SMMU_IN_ADDR_SIZE, master.smmu_index);
  pgt_desc.oas = val_smmu_get_info(SMMU_OUT_ADDR_SIZE, master.smmu_index);
  if ((pgt_desc.ias == 0) || (pgt_desc.oas == 0))
    goto test_fail;


  if (val_smmu_enable(master.smmu_index) != ACS_STATUS_PASS)
    goto test_fail;

  smmu_enabled = 1;

  mem_desc.virtual_address = (uint64_t)dram_buf_virt + (2 * page_size);
  mem_desc.physical_address = dram_buf_phys;
  mem_desc.length = page_size;
  mem_desc.attributes |= PGT_STAGE1_AP_RW;

  /* set pgt_desc.pgt_base to NULL to create new translation table, val_pgt_create
     will update pgt_desc.pgt_base to point to created translation table */
  pgt_desc.pgt_base = 0;
  if (val_pgt_create(&mem_desc, &pgt_desc)) {
    val_print(ERROR, "\n       Unable to create page table with given attributes");
    goto test_fail;
  }

  if (val_smmu_map(master, pgt_desc))
  {
    val_print(ERROR, "\n       SMMU mapping failed (%x)     ", e_bdf);
    goto test_fail;
  }
  smmu_mapped = 1;

  clear_dram_buf(dram_buf_virt, page_size);

  dram_buf_iova = mem_desc.virtual_address;
  dram_buf_out_iova = dram_buf_iova + dma_len;

  val_exerciser_set_param(DMA_ATTRIBUTES, dram_buf_iova, dma_len, instance);

  /* Send an ATS Translation Request for the VA */
  if (val_exerciser_ops(ATS_TXN_REQ, dram_buf_iova, instance)) {
    val_print(ERROR, "\n       ATS Translation Req Failed exerciser %4x", instance);
    goto test_fail;
  }

  /* Get ATS Translation Response */
  m_vir_addr = dram_buf_iova;
  if (val_exerciser_get_param(ATS_RES_ATTRIBUTES, &translated_addr, &m_vir_addr, instance)) {
    val_print(ERROR, "\n       ATS Response failure %4x", instance);
    goto test_fail;
  }

  /* Compare Translated Addr with Physical Address from the Mappings */
  if (translated_addr != dram_buf_phys) {
    val_print(ERROR, "\n       ATS Translation failure %4x", instance);
    goto test_fail;
  }

  /* Initialize the sender buffer with test specific data */
  write_test_data(dram_buf_virt, dma_len);

  /* Configure Exerciser to issue subsequent DMA transactions with Address Translated bit Set */
  val_exerciser_set_param(CFG_TXN_ATTRIBUTES, TXN_ADDR_TYPE, AT_TRANSLATED, instance);

  if (val_exerciser_set_param(DMA_ATTRIBUTES, dram_buf_phys, dma_len, instance)) {
    val_print(ERROR, "\n       DMA attributes setting failure %4x", instance);
    goto test_fail;
  }

  /* Trigger DMA from input buffer to exerciser memory */
  val_exerciser_ops(START_DMA, EDMA_TO_DEVICE, instance);

  if (val_exerciser_set_param(DMA_ATTRIBUTES, dram_buf_out_iova, dma_len, instance)) {
    val_print(ERROR, "\n       DMA attributes setting failure %4x", instance);
    goto test_fail;
  }

  /* Trigger DMA from exerciser memory to output buffer*/
  val_exerciser_ops(START_DMA, EDMA_FROM_DEVICE, instance);

  if (val_memory_compare(dram_buf_virt, dram_buf_out_virt, dma_len)) {
    val_print(ERROR, "\n       Data Comparasion failure for Exerciser %4x", instance);
    goto test_fail;
  }

  if (smmu_mapped)
    val_smmu_unmap(master);

  if (smmu_enabled)
    val_smmu_disable(master.smmu_index);

  if (pgt_desc.pgt_base)
    val_pgt_destroy(pgt_desc);

  if (ats_enabled) {
    val_pcie_read_cfg(e_bdf, cap_base + ATS_CTRL, &reg_value);
    reg_value &= ATS_CACHING_DIS;
    val_pcie_write_cfg(e_bdf, cap_base + ATS_CTRL, reg_value);
  }

  if (dram_buf_virt)
    val_memory_free_pages(dram_buf_virt, TEST_DATA_NUM_PAGES);
  return ACS_STATUS_PASS;

test_fail:
  if (smmu_mapped)
    val_smmu_unmap(master);

  if (smmu_enabled)
    val_smmu_disable(master.smmu_index);

  if (pgt_desc.pgt_base)
    val_pgt_destroy(pgt_desc);

  if (ats_enabled) {
    val_pcie_read_cfg(e_bdf, cap_base + ATS_CTRL, &reg_value);
    reg_value &= ATS_CACHING_DIS;
    val_pcie_write_cfg(e_bdf, cap_base + ATS_CTRL, reg_value);
  }

  if (dram_buf_virt)
    val_memory_free_pages(dram_buf_virt, TEST_DATA_NUM_PAGES);
  return ACS_STATUS_FAIL;
}

static
void
payload(void)
{
  uint32_t pe_index;
  uint32_t comp_count;
  uint32_t comp_idx;
  uint32_t test_skip = 0;
  uint32_t fail_cnt = 0;
  uint32_t cxl_type;
  uint32_t e_bdf;
  uint32_t status;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  if (val_exerciser_test_init() != ACS_STATUS_PASS) {
    val_set_status(pe_index, val_exerciser_get_init_result(TEST_RULE));
    return;
  }

  comp_count = val_cxl_get_component_info(CXL_COMPONENT_INFO_COUNT, 0);
  if (comp_count == 0) {
    val_set_status(pe_index, RESULT_SKIP(02));
    return;
  }

  for (comp_idx = 0; comp_idx < comp_count; comp_idx++) {

    cxl_type = val_cxl_get_component_info(CXL_COMPONENT_INFO_DEVICE_TYPE, comp_idx);
    if ((cxl_type != CXL_DEVICE_TYPE_TYPE1) && (cxl_type != CXL_DEVICE_TYPE_TYPE2))
      continue;

    e_bdf = val_cxl_get_component_info(CXL_COMPONENT_INFO_BDF_INDEX, comp_idx);
    status = val_cxl_device_is_cxl(e_bdf);
    if (status != ACS_STATUS_PASS) {
      val_print(DEBUG, "\n       No CXL Exerciser device BDF");
      continue;
    }

    test_skip = 1;

    if (run_cxl_path_ats_dma(e_bdf) != ACS_STATUS_PASS) {
      val_print(ERROR, "\n       CXL ATS+DMA functional test failed, BDF: 0x%x", e_bdf);
      fail_cnt++;
    }
  }

  if (test_skip == 0)
    val_set_status(pe_index, RESULT_SKIP(03));
  else if (fail_cnt)
    val_set_status(pe_index, RESULT_FAIL(01));
  else
    val_set_status(pe_index, RESULT_PASS);
}

uint32_t
e040_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP) {
      if (val_exerciser_test_init() != ACS_STATUS_PASS)
          return val_exerciser_get_init_result(TEST_RULE);
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);
  }

  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
