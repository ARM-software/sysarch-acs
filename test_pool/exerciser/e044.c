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
#include "acs_pcie_spec.h"
#include "acs_exerciser.h"
#include "acs_cxl.h"
#include "val_interface.h"

#define TEST_NUM   (ACS_EXERCISER_TEST_NUM_BASE + 44)
#define TEST_RULE  "CXL_05"
#define TEST_DESC  "CXL PMReq/PMRes VDM support on root port"

static
uint32_t
check_pm_state(uint32_t bdf, uint32_t *state)
{
  uint32_t cap_base;
  uint32_t reg_value;

  if (val_pcie_find_capability(bdf, PCIE_CAP, CID_PMC, &cap_base) != PCIE_SUCCESS)
    return ACS_STATUS_ERR;

  val_pcie_read_cfg(bdf, cap_base + PMCSR_OFFSET, &reg_value);
  *state = reg_value & PMCSR_POWER_STATE_MASK;
  return ACS_STATUS_PASS;
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
  uint32_t pm_state_before;
  uint32_t pm_state_after;
  uint32_t status;
  uint32_t fail_cnt = 0;
  uint32_t test_skip = 1;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  num_instances = val_exerciser_get_info(EXERCISER_NUM_CARDS);

  if (num_instances == 0) {
    val_set_status(pe_index, RESULT_SKIP(01));
    return;
  }

  for (instance = 0; instance < num_instances; instance++) {
    if (val_exerciser_init(instance))
      continue;

    e_bdf = val_exerciser_get_bdf(instance);
    status = val_cxl_device_is_cxl(e_bdf);
    if (status != ACS_STATUS_PASS)
      continue;

    if (val_pcie_get_rootport(e_bdf, &rp_bdf))
      continue;

    if (is_cxl_root_port(rp_bdf) != ACS_STATUS_PASS)
      continue;

    if (check_pm_state(e_bdf, &pm_state_before) != ACS_STATUS_PASS) {
      val_print(ERROR, "\n       PM capability missing for exerciser BDF 0x%x", e_bdf);
      fail_cnt++;
      continue;
    }

    /* Only validate a real transition to lower power state. */
    if (pm_state_before == PM_STATE_D3HOT)
      continue;

    status = val_exerciser_set_param(GENERATE_PMREQ_VDM, PM_STATE_D3HOT, rp_bdf, instance);
    if (status != ACS_STATUS_PASS) {
      continue;
    }

    test_skip = 0;
    val_time_delay_ms(1 * ONE_MILLISECOND);

    if (check_pm_state(e_bdf, &pm_state_after) != ACS_STATUS_PASS) {
      val_print(ERROR, "\n       Failed to read PMCSR for exerciser BDF 0x%x", e_bdf);
      fail_cnt++;
      continue;
    }

    if ((pm_state_after != PM_STATE_D3HOT) || (pm_state_after == pm_state_before)) {
      val_print(ERROR, "\n       PM state transition failed for BDF 0x%x", e_bdf);
      val_print(ERROR, " (before 0x%x)", pm_state_before);
      val_print(ERROR, " (after 0x%x)", pm_state_after);
      fail_cnt++;
    }
  }

  if (fail_cnt)
    val_set_status(pe_index, RESULT_FAIL(fail_cnt));
  else if (test_skip)
    val_set_status(pe_index, RESULT_SKIP(02));
  else
    val_set_status(pe_index, RESULT_PASS);
}

uint32_t
e044_entry(uint32_t num_pe)
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
