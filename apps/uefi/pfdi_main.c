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

#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/ShellCEntryLib.h>
#include  <Library/ShellLib.h>
#include  <Library/UefiBootServicesTableLib.h>

#include "val/include/val_interface.h"
#include "val/include/acs_pe.h"
#include "val/include/acs_val.h"
#include "val/include/acs_memory.h"

#include "acs.h"

VOID
HelpMsg (
  VOID
  )
{
  Print (L"\nUsage: pfdi.efi [options]\n"
        "-f <file>    Log file to record test results\n"
        "-h, -help    Print this message\n"
        "-m <list>    Run only the specified modules (comma-separated names)\n"
        "-r <rules>   Run specified rule IDs (comma-separated) or a rules file\n"
        "-skip <rules>\n"
        "             Skip the specified rule IDs\n"
        "             Accepted value: PFDI\n"
        "-skipmodule <list>\n"
        "             Skip the specified modules\n"
        "-v <n>       Verbosity of the prints (1-5)\n");
}

CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-f", TypeValue},
  {L"-h", TypeFlag},
  {L"-help", TypeFlag},
  {L"-m", TypeValue},
  {L"-r", TypeValue},
  {L"-skip", TypeValue},
  {L"-skipmodule", TypeValue},
  {L"-v", TypeValue},
  {NULL, TypeMax}
  };

static VOID
freePfdiAcsMem(void)
{
  val_pe_free_info_table();
  val_gic_free_info_table();
  val_free_shared_mem();
}

static UINT32
apply_cli_defaults(acs_run_request_t *ctx)
{
  if (ctx == NULL)
      return ACS_STATUS_FAIL;

  if (ctx->rule_count == 0) {
      ctx->arch_selection = ARCH_PFDI;
  }

  if (ctx->level_filter_mode == LVL_FILTER_NONE) {
      ctx->level_filter_mode = LVL_FILTER_MAX;
      ctx->level_value = PFDI_LEVEL_1;
  }

  if (ctx->level_value >= PFDI_LEVEL_SENTINEL) {
      val_print(ERROR, "\nInvalid level value passed (%d), ", ctx->level_value);
      val_print(ERROR, "value should be less than %d.", PFDI_LEVEL_SENTINEL);
      return ACS_STATUS_FAIL;
  }

  return ACS_STATUS_PASS;
}

UINT32
execute_tests()
{
  UINT32 Status;
  acs_run_request_t *ctx;

  ctx = acs_get_run_request_mut();

  Status = apply_cli_defaults(ctx);
  if (Status != ACS_STATUS_PASS) {
      goto exit_close;
  }

  val_print(INFO, "\n\n PFDI Architecture Compliance Suite");
  val_print(INFO, "\n          Version %d.", PFDI_ACS_MAJOR_VER);
  val_print(INFO, "%d.", PFDI_ACS_MINOR_VER);
  val_print(INFO, "%d\n", PFDI_ACS_SUBMINOR_VER);

  val_print(INFO, "\n Starting tests with print level : %2d\n\n", g_print_level);
  val_print(INFO, "\n Creating Platform Information Tables\n");

  Status = createPeInfoTable();
  if (Status)
      goto exit_close;

  Status = createGicInfoTable();
  if (Status)
      goto exit_close;

  val_allocate_shared_mem();

  Status = val_pfdi_check_implementation();
  if (Status == PFDI_ACS_NOT_IMPLEMENTED) {
      val_print(ERROR, "\n      PFDI not implemented - Skipping all PFDI tests\n");
      goto exit_summary;
  } else if (Status != ACS_STATUS_PASS) {
      goto exit_summary;
  }

  if ((ctx->rule_count > 0 && ctx->rule_list != NULL) || (ctx->arch_selection != ARCH_NONE)) {
      filter_rule_list_by_cli(ctx);
      if (ctx->rule_count == 0 || ctx->rule_list == NULL)
          goto exit_summary;

      print_selection_summary();
      run_tests(ctx);
  } else {
    val_print(INFO, "\nNo rules selected for execution.\n");
  }

exit_summary:
  val_print_acs_test_status_summary();
  val_print(ERROR, "\n      *** PFDI tests complete. *** \n\n");

exit_close:
  acs_release_run_request(ctx);
  freePfdiAcsMem();

  if (g_acs_log_file_handle) {
    ShellCloseFile(&g_acs_log_file_handle);
  }

  return ACS_STATUS_PASS;
}
