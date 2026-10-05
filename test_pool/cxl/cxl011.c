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
#include "acs_common.h"
#include "acs_cxl.h"
#include "acs_pcie.h"
#include "acs_pcie_spec.h"
#include "acs_memory.h"
#include "acs_pe.h"
#include "acs_pgt.h"
#include "acs_mmu.h"
#include "val_interface.h"

#define TEST_NUM   (ACS_CXL_TEST_NUM_BASE + 11)
#define TEST_RULE  "CXL_11"
#define TEST_DESC  "Validate PCMO reaches PoP for CXL.mem            "

#define CXL_TEST_PATTERN 0xA5A55A5AA55A5A5A
#define PGT_WB 0x448

static void *branch_to_test;
static uint64_t branch_stack_pointer;

typedef struct {
  uint32_t uncor_status;
  uint32_t corr_status;
} aer_status_t;

typedef struct {
  uint32_t cxl_bdf;
  uint32_t rp_bdf;
  uint32_t host_index;
} cxl_mem_target_t;

static
void
esr(uint64_t interrupt_type, void *context)
{
  uint32_t pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  if (interrupt_type == EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS) {
    val_pe_update_elr_and_sp(context, (uint64_t)branch_to_test, branch_stack_pointer);
  }
  val_print(ERROR, "\n       Received exception type: %d", interrupt_type);
  val_set_status(pe_index, RESULT_FAIL(1));
}

static
uint32_t
get_aer_status(uint32_t bdf, aer_status_t *status)
{
  uint32_t aer_cap_base;

  if (val_pcie_find_capability(bdf, PCIE_ECAP, ECID_AER, &aer_cap_base) != PCIE_SUCCESS)
    return 1;

  val_pcie_read_cfg(bdf, aer_cap_base + AER_UNCORR_STATUS_OFFSET, &status->uncor_status);
  val_pcie_read_cfg(bdf, aer_cap_base + AER_CORR_STATUS_OFFSET, &status->corr_status);
  return 0;
}

static
uint32_t
compare_aer_status(const aer_status_t *before, const aer_status_t *after)
{
  if ((after->uncor_status & ~(before->uncor_status)) != 0)
    return 1;

  if ((after->corr_status & ~(before->corr_status)) != 0)
    return 1;

  return 0;
}

static
uint32_t
find_cxl_mem_target(cxl_mem_target_t *target)
{
  uint32_t comp_count;
  uint32_t comp_idx;
  uint32_t cxl_type;
  uint32_t bdf;
  uint32_t rp_bdf;
  uint32_t dvsec_off;
  uint32_t reg_value;
  uint32_t cxl_caps;
  uint32_t rp_comp_idx;
  uint32_t rp_comp_bdf;
  uint32_t rp_comp_role;
  uint32_t host_index;

  comp_count = val_cxl_get_component_info(CXL_COMPONENT_INFO_COUNT, 0);
  if (comp_count == 0)
    return 1;

  for (comp_idx = 0; comp_idx < comp_count; comp_idx++) {
    cxl_type = val_cxl_get_component_info(CXL_COMPONENT_INFO_DEVICE_TYPE, comp_idx);
    if ((cxl_type != CXL_DEVICE_TYPE_TYPE2) && (cxl_type != CXL_DEVICE_TYPE_TYPE3))
      continue;

    bdf = val_cxl_get_component_info(CXL_COMPONENT_INFO_BDF_INDEX, comp_idx);
    if (bdf == CXL_COMPONENT_INVALID_INDEX)
      continue;

    if (val_cxl_find_capability(bdf, CXL_DVSEC_ID_DEVICE, &dvsec_off))
      continue;

    val_pcie_read_cfg(bdf, dvsec_off + CXL_DVSEC_HDR2_OFFSET, &reg_value);
    cxl_caps = (reg_value >> CXL_DVSEC_CXL_CAPABILITY_SHIFT) & CXL_DVSEC_CXL_CAPABILITY_MASK;
    if ((cxl_caps & CXL_DVSEC_CXL_CAP_MEM_CAPABLE) == 0)
      continue;

    if (val_pcie_get_rootport(bdf, &rp_bdf))
      continue;

    host_index = CXL_COMPONENT_INVALID_INDEX;
    for (rp_comp_idx = 0; rp_comp_idx < comp_count; rp_comp_idx++) {
      rp_comp_bdf = val_cxl_get_component_info(CXL_COMPONENT_INFO_BDF_INDEX, rp_comp_idx);
      rp_comp_role = val_cxl_get_component_info(CXL_COMPONENT_INFO_ROLE, rp_comp_idx);
      if ((rp_comp_role != CXL_COMPONENT_ROLE_ROOT_PORT) || (rp_comp_bdf != rp_bdf))
        continue;

      host_index = val_cxl_get_component_info(CXL_COMPONENT_INFO_HOST_BRIDGE_INDEX,
                                                        rp_comp_idx);
      break;
    }

    if (host_index == CXL_COMPONENT_INVALID_INDEX)
      continue;

    target->cxl_bdf = bdf;
    target->rp_bdf = rp_bdf;
    target->host_index = host_index;
    return 0;
  }

  return 1;
}

static
void
payload(void)
{
  uint32_t pe_index;
  uint32_t status;
  uint32_t count;
  uint32_t dpb_field;
  uint64_t read_back;
  uint64_t cfmws_base;
  uint64_t cfmws_size;
  volatile uint64_t *test_addr;
  volatile uint64_t *mapped = NULL;
  cxl_mem_target_t target;
  aer_status_t aer_ori;
  aer_status_t aer_updated;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  status = val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
  status |= val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);
  if (status) {
    val_print(ERROR, "\n       Failed to install exception handler");
    val_set_status(pe_index, RESULT_FAIL(1));
    return;
  }

  branch_to_test = &&exception_return;
  branch_stack_pointer = val_pe_save_exception_return_context();

  dpb_field = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64ISAR1_EL1), 0, 3);
  val_print(TRACE, "\n       DPB %x", dpb_field);
  if ((dpb_field != 0x1) && (dpb_field != 0x2)) {
    val_print(TRACE, "\n       DC CVAP/CVADP not supported by this PE");
    val_set_status(pe_index, RESULT_SKIP(1));
    return;
  }

  if (find_cxl_mem_target(&target)) {
    val_print(TRACE, "\n       No CXL Type 3 mem-capable target found");
    val_set_status(pe_index, RESULT_SKIP(2));
    return;
  }

  count = val_cxl_get_cfmws_count(target.host_index);
  if (count == 0)
    return;

  status = val_cxl_get_cfmws_window(target.host_index, &cfmws_base, &cfmws_size);
  if (status != ACS_STATUS_PASS)
    return;

  if ((cfmws_base == 0) || (cfmws_size == 0))
    return;

  val_pcie_enable_bme(target.cxl_bdf);
  val_pcie_enable_msa(target.cxl_bdf);
  val_pcie_enable_bme(target.rp_bdf);
  val_pcie_enable_msa(target.rp_bdf);

  if (get_aer_status(target.rp_bdf, &aer_ori)) {
    val_print(ERROR, "\n       AER capability not found on root port 0x%x", target.rp_bdf);
    val_set_status(pe_index, RESULT_FAIL(2));
    return;
  }

  status = val_cxl_map_hdm_address(cfmws_base, SIZE_4KB, &mapped);
  if (status != ACS_STATUS_PASS) {
    val_set_status(pe_index, RESULT_FAIL(3));
    return;
  }

  test_addr  = mapped;
  *test_addr = CXL_TEST_PATTERN;

  val_data_cache_ops_by_va((addr_t)test_addr, CLEAN_POC);

  if (get_aer_status(target.rp_bdf, &aer_updated)) {
    val_print(ERROR, "\n       Failed to read AER status after PCMO sequence");
    val_set_status(pe_index, RESULT_FAIL(4));
    return;
  }

  if (compare_aer_status(&aer_ori, &aer_updated)) {
    val_print(ERROR, "\n       AER errors detected after PCMO sequence");
    val_set_status(pe_index, RESULT_FAIL(5));
    return;
  }

  read_back = *test_addr;
exception_return:
  if (IS_TEST_FAIL(val_get_status(pe_index))) {
    return;
  }

  if (read_back != CXL_TEST_PATTERN) {
    val_print(ERROR, "\n       Readback mismatch: expected 0x%llx", CXL_TEST_PATTERN);
    val_print(ERROR, " observed 0x%llx", read_back);
    val_set_status(pe_index, RESULT_FAIL(6));
    return;
  }

  val_set_status(pe_index, RESULT_PASS);
}

uint32_t
cxl011_entry(uint32_t num_pe)
{
  uint32_t status;

  num_pe = 1;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
    val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
