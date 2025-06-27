/** @file
 * Pfdi API
 *
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
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

#include "include/acs_val.h"
#include "include/val_interface.h"
#include "include/acs_common.h"
#include "include/pal_interface.h"
#include "include/acs_std_smc.h"

/**
  @brief   This API will execute all PFDI tests
           1. Caller       -  Application layer.
           2. Prerequisite
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_pfdi_execute_pfdi_tests(uint32_t num_pe)
{
  uint32_t status, i;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == ACS_PFDI_TEST_NUM_BASE) {
        val_print(ACS_PRINT_INFO, "\n       USER Override - Skipping all PFDI tests\n", 0);
        return ACS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  status = val_check_skip_module(ACS_PFDI_TEST_NUM_BASE);
  if (status) {
      val_print(ACS_PRINT_INFO, "\n       USER Override - Skipping all PFDI tests\n", 0);
      return ACS_STATUS_SKIP;
  }

  status = ACS_STATUS_PASS;

  val_print_test_start("PFDI");
  g_curr_module = 1 << PFDI_MODULE;

  status  = pfdi001_entry(num_pe);
  status  = pfdi002_entry(num_pe);
  status  = pfdi003_entry(num_pe);
  status  = pfdi004_entry(num_pe);
  status  = pfdi005_entry(num_pe);
  status  = pfdi006_entry(num_pe);

  val_print_test_end(status, "PFDI");

  return status;

}
