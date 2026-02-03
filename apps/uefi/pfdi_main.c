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
apply_cli_defaults(void)
{
  if (g_rule_count == 0) {
      g_arch_selection = ARCH_PFDI;
  }

  if (g_level_filter_mode == LVL_FILTER_NONE) {
      g_level_filter_mode = LVL_FILTER_MAX;
      g_level_value = PFDI_LEVEL_1;
  }

  if (g_level_value >= PFDI_LEVEL_SENTINEL) {
      val_print(ACS_PRINT_ERR, "\nInvalid level value passed (%d), ", g_level_value);
      val_print(ACS_PRINT_ERR, "value should be less than %d.", PFDI_LEVEL_SENTINEL);
      return ACS_STATUS_FAIL;
  }

  return ACS_STATUS_PASS;
}

UINT32
execute_tests()
{
  UINT32 Status;

  Status = apply_cli_defaults();
  if (Status != ACS_STATUS_PASS) {
      goto exit_close;
  }

  val_print(ACS_PRINT_TEST, "\n\n PFDI Architecture Compliance Suite", 0);
  val_print(ACS_PRINT_TEST, "\n          Version %d.", PFDI_ACS_MAJOR_VER);
  val_print(ACS_PRINT_TEST, "%d.", PFDI_ACS_MINOR_VER);
  val_print(ACS_PRINT_TEST, "%d\n", PFDI_ACS_SUBMINOR_VER);

  val_print(ACS_PRINT_TEST, "\n Starting tests with print level : %2d\n\n", g_print_level);
  val_print(ACS_PRINT_TEST, "\n Creating Platform Information Tables\n", 0);

  Status = createPeInfoTable();
  if (Status)
      goto exit_close;

  Status = createGicInfoTable();
  if (Status)
      goto exit_close;

  val_allocate_shared_mem();

  Status = val_pfdi_check_implementation();
  if (Status == PFDI_ACS_NOT_IMPLEMENTED) {
      val_print(ACS_PRINT_ERR, "\n      PFDI not implemented - Skipping all PFDI tests\n", 0);
      goto exit_summary;
  } else if (Status != ACS_STATUS_PASS) {
      goto exit_summary;
  }

  if ((g_rule_count > 0 && g_rule_list != NULL) || (g_arch_selection != ARCH_NONE)) {
      g_rule_count = filter_rule_list_by_cli(&g_rule_list, g_rule_count);
      if (g_rule_count == 0 || g_rule_list == NULL)
          goto exit_summary;

      print_selection_summary();
      run_tests(g_rule_list, g_rule_count);
  } else {
    val_print(ACS_PRINT_TEST, "\nNo rules selected for execution.\n", 0);
  }

exit_summary:
  val_print_acs_test_status_summary();
  val_print(ACS_PRINT_ERR, "\n      *** PFDI tests complete. *** \n\n", 0);

exit_close:
  freePfdiAcsMem();

  if (g_acs_log_file_handle) {
    ShellCloseFile(&g_acs_log_file_handle);
  }

  return ACS_STATUS_PASS;
}
