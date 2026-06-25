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
#include "acs_pe.h"
#include "acs_pcie.h"
#include "acs_exerciser.h"
#include "acs_cxl.h"
#include "val_interface.h"

#define TEST_NUM   (ACS_EXERCISER_TEST_NUM_BASE + 45)
#define TEST_RULE  "CXL_06"
#define TEST_DESC  "CXL MEFN VDM sink handling on host    "

static void *branch_to_test;
static volatile uint32_t exception_observed;

static
void
esr(uint64_t interrupt_type, void *context)
{
  /* Return to the test flow after handling the exception. */
  val_pe_update_elr(context, (uint64_t)branch_to_test);
  val_print(ERROR, "\n       Received exception of type: %d", interrupt_type);
  exception_observed = 1;
}

static
uint32_t
is_cxl_root_port(uint32_t rp_bdf)
{
  uint32_t comp_count;
  uint32_t comp_idx;
  uint32_t comp_bdf;
  uint32_t comp_role;

  comp_count = val_cxl_get_component_info(CXL_COMPONENT_INFO_COUNT, 0);
  for (comp_idx = 0; comp_idx < comp_count; comp_idx++) {
    comp_bdf = val_cxl_get_component_info(CXL_COMPONENT_INFO_BDF_INDEX, comp_idx);
    comp_role = val_cxl_get_component_info(CXL_COMPONENT_INFO_ROLE, comp_idx);
    if ((comp_role == CXL_COMPONENT_ROLE_ROOT_PORT) && (comp_bdf == rp_bdf))
      return ACS_STATUS_PASS;
  }

  return ACS_STATUS_ERR;
}

static
void
payload(void)
{
  uint32_t pe_index;
  uint32_t num_instances;
  uint32_t instance;
  uint32_t e_bdf;
  uint32_t rp_bdf;
  uint32_t status;
  uint32_t fail_count = 0;
  uint32_t test_skip = 1;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  num_instances = val_exerciser_get_info(EXERCISER_NUM_CARDS);

  status = val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
  status |= val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);
  if (status) {
    val_print(ERROR, "\n       Failed in installing the exception handler");
    val_set_status(pe_index, RESULT_FAIL(01));
    return;
  }

  status = val_exerciser_check_firmware_handle_support();
  if (status == ACS_STATUS_PAL_NOT_IMPLEMENTED) {
    val_set_status(pe_index, RESULT_WARNING(2));
    return;
  }

  if (status != ACS_STATUS_PASS) {
    val_print(ERROR, "\n       Firmware first handling not supported");
    val_set_status(pe_index, RESULT_SKIP(01));
    return;
  }

  branch_to_test = &&exception_return;

  while (num_instances-- != 0) {
    instance = num_instances;

    if (val_exerciser_init(instance))
      continue;

    e_bdf = val_exerciser_get_bdf(instance);
    if (val_cxl_device_is_cxl(e_bdf) != ACS_STATUS_PASS)
      continue;

    if (val_pcie_get_rootport(e_bdf, &rp_bdf))
      continue;

    if (is_cxl_root_port(rp_bdf) != ACS_STATUS_PASS)
      continue;

    exception_observed = 0;

    status = val_exerciser_set_param(GENERATE_MEFN_VDM, 0, e_bdf, instance);
    if (status != ACS_STATUS_PASS) {
      val_print(ERROR, "\n       MEFN trigger not succesful BDF 0x%x", e_bdf);
      continue;
    }

    test_skip = 0;
    val_time_delay_ms(1 * ONE_MILLISECOND);

exception_return:
    if (!exception_observed) {
      val_print(ERROR, "\n      MEFN trigger did not raise exception for BDF 0x%x", e_bdf);
      val_print(ERROR, " RP BDF is 0x%x", rp_bdf);
      fail_count++;
    }
  }

  if (test_skip) {
    val_set_status(pe_index, RESULT_SKIP(01));
  } else if (fail_count) {
    val_set_status(pe_index, RESULT_FAIL(fail_count));
  } else {
    val_set_status(pe_index, RESULT_PASS);
  }
}

uint32_t
e045_entry(uint32_t num_pe)
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
