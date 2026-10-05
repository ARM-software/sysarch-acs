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
#include "acs_pe.h"
#include "acs_pgt.h"
#include "acs_memory.h"
#include "val_interface.h"

#define TEST_NUM   (ACS_CXL_TEST_NUM_BASE + 13)
#define TEST_RULE  "CXL_13"
#define TEST_DESC  "Validate CXL Type3 atomic memory features      "

static void *branch_to_test;
static uint64_t branch_stack_pointer;
static volatile uint32_t exception;

typedef struct {
  uint32_t cxl_bdf;
  uint32_t rp_bdf;
  uint32_t host_index;
} cxl_type3_target_t;

static
void
esr(uint64_t interrupt_type, void *context)
{
  uint32_t pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  if (interrupt_type == EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS) {
    val_pe_update_elr_and_sp(context, (uint64_t)branch_to_test, branch_stack_pointer);
  }
  val_print(ERROR, "\n       Received exception type: %d", interrupt_type);
  exception = 1;
  val_set_status(pe_index, RESULT_FAIL(1));
}

static
uint32_t
find_cxl_type3_target(cxl_type3_target_t *target)
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
    return ACS_STATUS_ERR;

  for (comp_idx = 0; comp_idx < comp_count; comp_idx++) {
    cxl_type = val_cxl_get_component_info(CXL_COMPONENT_INFO_DEVICE_TYPE, comp_idx);
    if (cxl_type != CXL_DEVICE_TYPE_TYPE3)
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

      host_index = val_cxl_get_component_info(CXL_COMPONENT_INFO_HOST_BRIDGE_INDEX, rp_comp_idx);
      break;
    }

    if (host_index == CXL_COMPONENT_INVALID_INDEX)
      continue;

    target->cxl_bdf = bdf;
    target->rp_bdf = rp_bdf;
    target->host_index = host_index;
    return ACS_STATUS_PASS;
  }

  return ACS_STATUS_ERR;
}

static
uint32_t
run_atomic_sequence(volatile uint64_t *addr)
{
  uint64_t val;
  uint64_t old;
  uint32_t st;

  *addr = 0x10;

  /* LDAXR/STLXR sequence */
  asm volatile(
      "1: ldaxr %x0, [%2]\n"
      "add %x0, %x0, #1\n"
      "stlxr %w1, %x0, [%2]\n"
      "cbnz %w1, 1b\n"
      : "=&r"(val), "=&r"(st)
      : "r"(addr)
      : "memory");

  if (exception)
    return ACS_STATUS_ERR;

  if (*addr != 0x11)
    return ACS_STATUS_ERR;

  /* SWP sequence */
  val = 0x22;
  asm volatile("swp %x[newv], %x[oldv], [%[addr]]"
               : [oldv]"=r"(old)
               : [newv]"r"(val), [addr]"r"(addr)
               : "memory");

  if (exception)
    return ACS_STATUS_ERR;

  if ((old != 0x11) || (*addr != 0x22))
    return ACS_STATUS_ERR;

  /* LDADD sequence */
  val = 0x3;
  asm volatile("ldadd %x[addv], %x[oldv], [%[addr]]"
               : [oldv]"=r"(old)
               : [addv]"r"(val), [addr]"r"(addr)
               : "memory");

  if (exception)
    return ACS_STATUS_ERR;

  if ((old != 0x22) || (*addr != 0x25))
    return ACS_STATUS_ERR;

  /* CAS sequence */
  old = 0x25;
  val = 0x31;
  asm volatile("cas %x[exp], %x[newv], [%[addr]]"
               : [exp]"+r"(old)
               : [newv]"r"(val), [addr]"r"(addr)
               : "memory");
  if (exception)
    return ACS_STATUS_ERR;
  if ((old != 0x25) || (*addr != 0x31))
    return ACS_STATUS_ERR;

  return ACS_STATUS_PASS;
}

static
void
payload(void)
{
  uint32_t pe_index;
  uint32_t status;
  uint64_t data;
  uint32_t atomic_feat;
  uint32_t lse2_feat;
  uint32_t rme_feat;
  uint32_t mec_feat;
  uint32_t ras_feat;
  uint32_t mte_feat;
  uint32_t mte_frac_feat;
  uint32_t ls64_feat;
  uint64_t cfmws_base;
  uint64_t cfmws_size;
  volatile uint64_t *mapped = NULL;
  cxl_type3_target_t target;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  data = val_pe_reg_read(ID_AA64ISAR0_EL1);
  atomic_feat = VAL_EXTRACT_BITS(data, 20, 23);

  if (atomic_feat < 0x2) {
    val_print(ERROR,
              "\n       FEAT_LSE not supported (ID_AA64ISAR0_EL1.Atomic = 0x%x)", atomic_feat);
    val_set_status(pe_index, RESULT_FAIL(1));
    return;
  }

  data = val_pe_reg_read(ID_AA64MMFR2_EL1);
  lse2_feat = VAL_EXTRACT_BITS(data, 32, 35);
  if (lse2_feat == 0) {
    val_print(ERROR,
              "\n       FEAT_LSE2 not supported (ID_AA64MMFR2_EL1.AT = 0x%x)", lse2_feat);
    val_set_status(pe_index, RESULT_FAIL(2));
    return;
  }

  status = val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
  status |= val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);
  if (status) {
    val_print(ERROR, "\n       Failed to install exception handler");
    val_set_status(pe_index, RESULT_FAIL(3));
    return;
  }
  branch_to_test = &&exception_return;
  branch_stack_pointer = val_pe_save_exception_return_context();
  exception = 0;

  if (find_cxl_type3_target(&target) != ACS_STATUS_PASS) {
    val_print(TRACE, "\n       No CXL Type-3 memory target found");
    val_set_status(pe_index, RESULT_SKIP(1));
    return;
  }

  status = val_cxl_get_cfmws_window(target.host_index, &cfmws_base, &cfmws_size);
  if ((status != ACS_STATUS_PASS) || (cfmws_base == 0) || (cfmws_size < SIZE_4KB)) {
    val_print(ERROR, "\n       Failed to locate usable CFMWS window");
    val_set_status(pe_index, RESULT_FAIL(4));
    return;
  }

  status = val_cxl_map_hdm_address(cfmws_base, SIZE_4KB, &mapped);
  if (status != ACS_STATUS_PASS) {
    val_print(ERROR, "\n       Failed to map CXL Type-3 memory region");
    val_set_status(pe_index, RESULT_FAIL(4));
    return;
  }

  status = run_atomic_sequence(mapped);

exception_return:
  if (IS_TEST_FAIL(val_get_status(pe_index)))
    return;

  if ((status != ACS_STATUS_PASS) || exception) {
    val_print(ERROR, "\n       Atomic sequence failed on CXL memory");
    val_set_status(pe_index, RESULT_FAIL(5));
    return;
  }

  /* Recommended symmetric memory features: warning only if unavailable. */
  data = val_pe_reg_read(ID_AA64ISAR1_EL1);
  ls64_feat = VAL_EXTRACT_BITS(data, 60, 63);
  if (ls64_feat == 0)
    val_print(WARN, "\n       FEAT_LS64/LS4_V/LS64_ACCDATA is not supported");

  data = val_pe_reg_read(ID_AA64PFR0_EL1);
  ras_feat = VAL_EXTRACT_BITS(data, 28, 31);
  if (ras_feat == 0)
    val_print(WARN, "\n       FEAT_RAS is not supported");

  rme_feat = VAL_EXTRACT_BITS(data, 52, 55);
  if (rme_feat == 0)
    val_print(WARN, "\n       FEAT_RME is not supported");


  data = val_pe_reg_read(ID_AA64PFR1_EL1);
  mte_feat = VAL_EXTRACT_BITS(data, 8, 11);
  if (mte_feat == 0)
    val_print(WARN, "\n       FEAT_MTE/MTE2 is not supported");

  mte_frac_feat = VAL_EXTRACT_BITS(data, 40, 43);
  if (!((mte_feat >= 3) || ((mte_feat == 2) && (mte_frac_feat == 0))))
    val_print(WARN, "\n       FEAT_MTE_ASYNC is not supported");


  data = val_pe_reg_read(ID_AA64MMFR3_EL1);
  mec_feat = VAL_EXTRACT_BITS(data, 28, 31);
  if (mec_feat == 0)
    val_print(WARN, "\n       Optional feature FEAT_MEC is not supported");

  val_set_status(pe_index, RESULT_PASS);
}

uint32_t
cxl013_entry(uint32_t num_pe)
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
