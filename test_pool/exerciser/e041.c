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
#include "acs_iovirt.h"
#include "acs_memory.h"
#include "acs_pcie.h"
#include "acs_pe.h"
#include "val_interface.h"

#define TEST_NUM   (ACS_EXERCISER_TEST_NUM_BASE + 41)
#define TEST_RULE  "CXL_12"
#define TEST_DESC  "CXL.cache coherency with exerciser read/write"

#define TEST_SIZE  256

static
uint32_t
compare_test_data(const uint8_t *buf, uint32_t len, uint8_t pattern)
{
  uint32_t index;

  for (index = 0; index < len; index++) {
    if (buf[index] != pattern)
      return 0;
  }

  return 1;
}

static
uint32_t
cache_sequence(uint32_t instance, uint32_t e_bdf)
{
  void *buf_pa;
  uint8_t *buf_va;
  uint8_t *target_va;
  uint8_t *dram_buf1_virt;
  uint8_t *dram_buf2_virt;
  uint32_t test_data;
  uint64_t dest_addr;
  uint64_t dram_buf1_phys;
  uint64_t dram_buf2_phys;
  uint64_t base_pa;

  buf_va = val_memory_alloc_cacheable(e_bdf, SIZE_4KB, &buf_pa);
  if (buf_va == NULL)
    return ACS_STATUS_FAIL;

  base_pa = (uint64_t)buf_pa;
  target_va = buf_va;
  dram_buf1_virt = buf_va + TEST_SIZE;
  dram_buf2_virt = buf_va + (2 * TEST_SIZE);

  dest_addr = base_pa;
  dram_buf1_phys = base_pa + TEST_SIZE;
  dram_buf2_phys = base_pa + (2 * TEST_SIZE);

  if (val_exerciser_ops(TXN_NO_SNOOP_DISABLE, 0, instance)) {
    val_memory_free_cacheable(e_bdf, SIZE_4KB, buf_va, buf_pa);
    return ACS_STATUS_FAIL;
  }

  test_data = 0xab;
  /* CPU writes Pattern A. Do not clean/invalidate this cache line. */
  val_print(TRACE, "\n       STEP3: CPU writes Pattern A (0x%x) without cache clean",
            test_data);
  val_memory_set(target_va, TEST_SIZE, test_data);

  if (val_exerciser_set_param(DMA_ATTRIBUTES, dest_addr, TEST_SIZE, instance)) {
    val_memory_free_cacheable(e_bdf, SIZE_4KB, buf_va, buf_pa);
    return ACS_STATUS_FAIL;
  }
  if (val_exerciser_ops(START_DMA, EDMA_TO_DEVICE, instance)) {
    val_memory_free_cacheable(e_bdf, SIZE_4KB, buf_va, buf_pa);
    return ACS_STATUS_FAIL;
  }

  val_memory_set(dram_buf1_virt, TEST_SIZE, 0);
  val_pe_cache_clean_invalidate_range((uint64_t)dram_buf1_virt, TEST_SIZE);

  if (val_exerciser_set_param(DMA_ATTRIBUTES, dram_buf1_phys, TEST_SIZE, instance)) {
    val_memory_free_cacheable(e_bdf, SIZE_4KB, buf_va, buf_pa);
    return ACS_STATUS_FAIL;
  }
  if (val_exerciser_ops(START_DMA, EDMA_FROM_DEVICE, instance)) {
    val_memory_free_cacheable(e_bdf, SIZE_4KB, buf_va, buf_pa);
    return ACS_STATUS_FAIL;
  }

  val_pe_cache_invalidate_range((uint64_t)dram_buf1_virt, TEST_SIZE);
  if (!compare_test_data(dram_buf1_virt, TEST_SIZE, test_data)) {
    val_print(ERROR, "\n       Exerciser did not observe Pattern A over coherent path");
    val_print(ERROR, "\n       Expected 0x%x", test_data);
    val_print(ERROR, "\n       Observed first byte 0x%x", dram_buf1_virt[0]);
    val_memory_free_cacheable(e_bdf, SIZE_4KB, buf_va, buf_pa);
    return ACS_STATUS_FAIL;
  }

  test_data = 0xde;
  val_print(TRACE, "\n Exerciser writes 0xDE CPU reads without invalidate");
  val_memory_set(dram_buf2_virt, TEST_SIZE, test_data);
  val_pe_cache_clean_invalidate_range((uint64_t)dram_buf2_virt, TEST_SIZE);

  if (val_exerciser_set_param(DMA_ATTRIBUTES, dram_buf2_phys, TEST_SIZE, instance)) {
    val_memory_free_cacheable(e_bdf, SIZE_4KB, buf_va, buf_pa);
    return ACS_STATUS_FAIL;
  }
  if (val_exerciser_ops(START_DMA, EDMA_TO_DEVICE, instance)) {
    val_memory_free_cacheable(e_bdf, SIZE_4KB, buf_va, buf_pa);
    return ACS_STATUS_FAIL;
  }

  if (val_exerciser_set_param(DMA_ATTRIBUTES, dest_addr, TEST_SIZE, instance)) {
    val_memory_free_cacheable(e_bdf, SIZE_4KB, buf_va, buf_pa);
    return ACS_STATUS_FAIL;
  }
  if (val_exerciser_ops(START_DMA, EDMA_FROM_DEVICE, instance)) {
    val_memory_free_cacheable(e_bdf, SIZE_4KB, buf_va, buf_pa);
    return ACS_STATUS_FAIL;
  }

  /* CPU reads target without explicit invalidate. */
  if (!compare_test_data(target_va, TEST_SIZE, test_data)) {
    val_print(ERROR, "\n     CPU did not observe pattern without explicit invalidate");
    val_print(ERROR, "\n     Expected 0x%x", test_data);
    val_print(ERROR, "\n     Observed first byte 0x%x", target_va[0]);
    val_memory_free_cacheable(e_bdf, SIZE_4KB, buf_va, buf_pa);
    return ACS_STATUS_FAIL;
  }

  val_memory_free_cacheable(e_bdf, SIZE_4KB, buf_va, buf_pa);
  return ACS_STATUS_PASS;
}

static
void
payload(void)
{
  uint32_t pe_index;
  uint32_t status;
  uint32_t comp_count;
  uint32_t comp_idx;
  uint32_t cxl_type;
  uint32_t rc_index;
  uint32_t instance;
  uint32_t e_bdf;
  uint32_t test_skip = 1;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  comp_count = val_cxl_get_component_info(CXL_COMPONENT_INFO_COUNT, 0);
  if (comp_count == 0) {
    val_set_status(pe_index, RESULT_SKIP(01));
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
    if (!val_cxl_device_cache_capable(e_bdf)) {
      val_print(TRACE, "CXL.cache not supported for component BDF 0x%x", e_bdf);
      continue;
    }

    rc_index = val_iovirt_get_rc_index(PCIE_EXTRACT_BDF_SEG(e_bdf));
    if (rc_index == ACS_INVALID_INDEX)
      continue;

    instance = val_exerciser_get_exerciser_instance(rc_index);
    if (instance == ACS_INVALID_INDEX)
      continue;

    if (val_exerciser_init(instance))
        continue;

    val_print(DEBUG, "\n       Exerciser BDF: 0x%x", e_bdf);

    if (val_exerciser_set_param(ENABLE_CACHE_TXN, 1, 1, instance)) {
      val_print(DEBUG, "\n       Enable CXL.cache Transaction from the Exerciser");
      continue;
    }

    /* Test is running for atleast one CXL exerciser device */
    test_skip = 0;
    status = cache_sequence(instance, e_bdf);
    if (status != ACS_STATUS_PASS) {
      val_print(ERROR, "\n       CXL.cache exerciser sequence failed");
      val_set_status(pe_index, RESULT_FAIL(1));
      return;
    }

  }

  if (test_skip)
      val_set_status(pe_index, RESULT_SKIP(02));
  else
      val_set_status(pe_index, RESULT_PASS);
}

uint32_t
e041_entry(uint32_t num_pe)
{
  uint32_t status;

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
