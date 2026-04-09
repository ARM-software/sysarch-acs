/** @file
 * Copyright (c) 2025-2026, Arm Limited or its affiliates. All rights reserved.
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
#include "val_interface.h"
#include "acs_gic.h"

#define TEST_NUM   (ACS_GIC_TEST_NUM_BASE + 16)
#define TEST_RULE  "S_L5GI_01"
#define TEST_DESC  "Check Non standard GICv3 implmentation"

static
void
payload(void)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t num_non_gic = 0;
  uint32_t num_gicd = 0;

  num_non_gic = val_get_num_nongic_ctrl();

  val_print(DEBUG, "\n       Non GIC Interrupt count: %d", num_non_gic);
  if (num_non_gic > 0) {
      val_print(ERROR, "\n       Non GIC Interrupt found");
      val_set_status(index, RESULT_FAIL(01));
      return;
  }

  /* MADT permits only a single GICD structure. Exposing multiple GICDs via MADT would be treated
     as a non-standard GICv3 implementation */
  num_gicd = val_gic_get_info(GIC_INFO_NUM_GICD);
  if (num_gicd > 1) {
      val_print(ERROR, "\n       GIC Distributor count is greater than 1");
      val_set_status(index, RESULT_FAIL(02));
      return;
  }

  val_set_status(index, RESULT_PASS);
  return;
}

uint32_t
g016_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This GIC test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
