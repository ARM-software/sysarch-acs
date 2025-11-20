/** @file
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/acs_val.h"
#include "val/include/val_interface.h"

#define TEST_NUM   (ACS_PFDI_TEST_NUM_BASE + 11)
#define TEST_RULE  "R0040"
#define TEST_DESC  "Check if X5 to X17 are preserved          "

extern int32_t gPsciConduit;

static void payload_regs_preserve_check(void *arg)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  int32_t i, reg_verify_fail = 0;
  ARM_SMC_ARGS args;
  uint64_t pre_smc_regs[REG_COUNT_X5_X17] = {0};
  uint64_t post_smc_regs[REG_COUNT_X5_X17] = {0};

  /* Verify X5-X17 by sending version function */
  args.Arg0 = PFDI_FN_PFDI_VERSION;
  args.Arg1 = 0;
  args.Arg2 = 0;
  args.Arg3 = 0;
  args.Arg4 = 0;
  args.Arg5 = 0;

  val_pfdi_verify_regs(&args, gPsciConduit, pre_smc_regs, post_smc_regs);

  for (i = 0; i < REG_COUNT_X5_X17; i++) {
    if (pre_smc_regs[i] != post_smc_regs[i]) {
      reg_verify_fail++;
      val_print(ACS_PRINT_ERR, "\n       Reg Verify fail for version X%d ", i + 5);
      val_print(ACS_PRINT_ERR, "before 0x%llx ", pre_smc_regs[i]);
      val_print(ACS_PRINT_ERR, "after  0x%llx ", post_smc_regs[i]);
    }
  }

  /* Verify X5-X17 by sending pe test id function */
  args.Arg0 = PFDI_FN_PFDI_PE_TEST_ID;
  args.Arg1 = 0;
  args.Arg2 = 0;
  args.Arg3 = 0;
  args.Arg4 = 0;
  args.Arg5 = 0;

  val_pfdi_verify_regs(&args, gPsciConduit, pre_smc_regs, post_smc_regs);

  for (i = 0; i < REG_COUNT_X5_X17; i++) {
    if (pre_smc_regs[i] != post_smc_regs[i]) {
      reg_verify_fail++;
      val_print(ACS_PRINT_ERR, "\n       Reg Verify fail for PE test ID X%d ", i + 5);
      val_print(ACS_PRINT_ERR, "before 0x%llx ", pre_smc_regs[i]);
      val_print(ACS_PRINT_ERR, "after  0x%llx ", post_smc_regs[i]);
    }
  }

  /* Verify X5-X17 by sending test result function */
  args.Arg0 = PFDI_FN_PFDI_PE_TEST_RESULT;
  args.Arg1 = 0;
  args.Arg2 = 0;
  args.Arg3 = 0;
  args.Arg4 = 0;
  args.Arg5 = 0;

  val_pfdi_verify_regs(&args, gPsciConduit, pre_smc_regs, post_smc_regs);

  for (i = 0; i < REG_COUNT_X5_X17; i++) {
    if (pre_smc_regs[i] != post_smc_regs[i]) {
      reg_verify_fail++;
      val_print(ACS_PRINT_ERR, "\n       Reg Verify fail for Test result X%d ", i + 5);
      val_print(ACS_PRINT_ERR, "before 0x%llx ", pre_smc_regs[i]);
      val_print(ACS_PRINT_ERR, "after  0x%llx ", post_smc_regs[i]);
    }
  }

  if (reg_verify_fail)
    val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
  else
    val_set_status(index, RESULT_PASS(TEST_NUM, 1));

  return;
}

uint32_t pfdi011_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1; /* This test will run on single PE */
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
    val_run_test_configurable_payload(&num_pe, payload_regs_preserve_check);

  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
