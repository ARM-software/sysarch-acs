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
#include "acs_pe.h"
#include "acs_pcie_enumeration.h"
#include "acs_pcie.h"
#include "acs_memory.h"
#include "acs_cxl.h"
#include "acs_pgt.h"
#include "val_interface.h"
#include "acs_exerciser.h"

#define TEST_NUM   (ACS_EXERCISER_TEST_NUM_BASE + 43)
#define TEST_RULE  "CXL_09"
#define TEST_DESC  "CXL.mem write transaction             "

static void *branch_to_test;
static volatile uint32_t exception_observed;
#define TEST_DATA 0xA5A5A5A5A5A5A5A5ULL;

static
void
esr(uint64_t interrupt_type, void *context)
{

  /* Update the ELR to return to test specified address */
  val_pe_update_elr(context, (uint64_t)branch_to_test);

  val_print(ERROR, "\n       Received exception of type: %d", interrupt_type);
  exception_observed = 1;
}

static
void
payload()
{

  uint32_t instance;
  uint32_t pe_index;
  uint32_t e_bdf;
  uint32_t erp_bdf;
  uint32_t status;
  uint32_t fail_cnt = 0;
  uint32_t test_skip = 1;
  uint32_t comp_count;
  uint32_t host_index = CXL_COMPONENT_INVALID_INDEX;
  volatile uint64_t *test_addr;
  volatile uint64_t *mapped = NULL;
  uint32_t rp_comp_idx;
  uint32_t rp_comp_bdf;
  uint32_t rp_comp_role;
  uint64_t cfmws_base;
  uint64_t cfmws_size;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS);

  /* Install sync and async handlers to handle exceptions.*/
  status = val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
  status |= val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);
  if (status)
  {
      val_print(ERROR, "\n      Failed in installing the exception handler");
      val_set_status(pe_index, RESULT_FAIL(01));
      return;
  }

  branch_to_test = &&exception_return;

  while (instance-- != 0) {

      /* if init fail moves to next exerciser */
      if (val_exerciser_init(instance))
          continue;

      e_bdf = val_exerciser_get_bdf(instance);
      status = val_cxl_device_is_cxl(e_bdf);
      if (status == ACS_STATUS_ERR)
          continue;
      if (status == PCIE_UNKNOWN_RESPONSE) {
          val_print(ERROR,
                    "\n       Failed to probe CXL DVSEC for BDF 0x%x", e_bdf);
          fail_cnt++;
          continue;
      }
      if (status != ACS_STATUS_PASS)
          continue;

      val_print(TRACE, "\n       Exerciser BDF - 0x%x", e_bdf);

      val_pcie_enable_eru(e_bdf);
      val_pcie_enable_msa(e_bdf);

      if (val_pcie_get_rootport(e_bdf, &erp_bdf))
          continue;

      val_pcie_enable_eru(erp_bdf);
      val_pcie_enable_msa(erp_bdf);

      comp_count = val_cxl_get_component_info(CXL_COMPONENT_INFO_COUNT, 0);
      for (rp_comp_idx = 0; rp_comp_idx < comp_count; rp_comp_idx++) {
        rp_comp_bdf = val_cxl_get_component_info(CXL_COMPONENT_INFO_BDF_INDEX, rp_comp_idx);
        rp_comp_role = val_cxl_get_component_info(CXL_COMPONENT_INFO_ROLE, rp_comp_idx);
        if ((rp_comp_role != CXL_COMPONENT_ROLE_ROOT_PORT) || (rp_comp_bdf != erp_bdf))
          continue;

        host_index = val_cxl_get_component_info(CXL_COMPONENT_INFO_HOST_BRIDGE_INDEX,
                                                          rp_comp_idx);
        break;
      }

      if (host_index == CXL_COMPONENT_INVALID_INDEX)
          continue;

      val_print(TRACE, "\n       host_index: %llx", host_index);
      status = val_cxl_get_cfmws_window(host_index, &cfmws_base, &cfmws_size);
      if (status != ACS_STATUS_PASS) {
        val_set_status(pe_index, RESULT_FAIL(02));
        return;
      }

      status = val_cxl_map_hdm_address(cfmws_base, SIZE_4KB, &mapped);
      if (status != ACS_STATUS_PASS) {
        val_set_status(pe_index, RESULT_FAIL(03));
        return;
      }

      test_addr  = mapped;

      /* Enable the Poison mode in the exerciser,
       * so that it generates poison data on writes */
      val_exerciser_set_param(ENABLE_POISON_MODE, 0, 0, instance);

      /* Test will run for atleast one endpoint */
      test_skip = 0;

      *test_addr = TEST_DATA;

exception_return:
      if (exception_observed)
      {
           val_print(ERROR, "\n    Exception obtained for CXL.mem write transaction");
           fail_cnt++;
      }

      /* Disable the Poison mode in the exerciser*/
      val_exerciser_set_param(DISABLE_POISON_MODE, 0, 0, instance);

  }

  if (test_skip)
      val_set_status(pe_index, RESULT_SKIP(01));
  else if (fail_cnt)
      val_set_status(pe_index, RESULT_FAIL(04));
  else
      val_set_status(pe_index, RESULT_PASS);

  return;
}

uint32_t
e043_entry(uint32_t num_pe)
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
