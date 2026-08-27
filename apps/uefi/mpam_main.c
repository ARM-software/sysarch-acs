/** @file
 * Copyright (c) 2025-2026, Arm Limited or its affiliates. All rights reserved.
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

#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/ShellCEntryLib.h>
#include  <Library/ShellLib.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Library/CacheMaintenanceLib.h>
#include  <Protocol/LoadedImage.h>

#include "val/include/val_interface.h"
#include "val/include/acs_pe.h"
#include "val/include/acs_val.h"
#include "val/include/acs_mpam.h"
#include "val/include/acs_memory.h"

#include "acs.h"

UINT32  g_execute_secure;
UINT32  g_acs_tests_total;
UINT32  g_acs_tests_pass;
UINT32  g_acs_tests_fail;
UINT32  *g_execute_tests;
UINT32  g_num_tests = 0;
UINT32  *g_execute_modules;
UINT32  g_num_modules = 0;

static UINT32
apply_cli_defaults(acs_run_request_t *ctx)
{
  if (ctx == NULL)
    return ACS_STATUS_FAIL;

  if (ctx->rule_count == 0) {
    ctx->arch_selection = ARCH_SYS_MPAM;
  }

  if (ctx->level_filter_mode == LVL_FILTER_NONE) {
    ctx->level_filter_mode = LVL_FILTER_MAX;
    ctx->level_value = MPAM_VERSION_B_c;
  }

  if (ctx->level_value >= MPAM_VERSION_SENTINEL) {
    val_print(ERROR, "\nInvalid MPAM version value passed (%d), ", ctx->level_value);
    val_print(ERROR, "value should be less than %d.", MPAM_VERSION_SENTINEL);
    return ACS_STATUS_FAIL;
  }

  return ACS_STATUS_PASS;
}

VOID
createIovirtInfoTable(
)
{
  UINT64 *IoVirtInfoTable;

  IoVirtInfoTable = val_aligned_alloc(SIZE_4K, IOVIRT_INFO_TBL_SZ);

  val_iovirt_create_info_table(IoVirtInfoTable);
}

VOID
FreeMpamAcsMem (
)
{
    val_free_shared_mem();
    val_mpam_free_info_table();
    val_srat_free_info_table();
    val_hmat_free_info_table();
    val_pcc_free_info_table();
    val_cache_free_info_table();
    val_iovirt_free_info_table();
    val_gic_free_info_table();
    val_pe_free_info_table();
}

VOID
HelpMsg (
    VOID
    )
{

    Print (L"\nUsage: Mpam.efi [options]\n"
             "Options:\n"
             "-f      Name of the log file to record the test results in\n"
             "-h, -help\n"
             "        Print this message\n"
             "-l <n>  Run SYS-MPAM rules for the specified MPAM architecture version\n"
             "        Supported MPAM_VERSION_e values:\n"
             "        1 - MPAM_VERSION_B_c\n"
             "-m      Run only the specified modules (comma-separated names).\n"
             "        Accepted values: CACHE, REG, ERROR, MBWU\n"
             "-r      Run tests for passed comma-separated SYS-MPAM rule IDs or a rules file\n"
             "        Example: -r RBDJXQ\n"
             "-skip   SYS-MPAM rule ID(s) to be skipped (comma-separated, like -r)\n"
             "-skipmodule\n"
             "        Skip the specified modules (comma-separated names).\n"
             "-v <n>  Verbosity of the prints\n"
             "        1 prints all, 5 prints only the errors\n"
             );
}

CONST SHELL_PARAM_ITEM ParamList[] = {
    {L"-f", TypeValue},
    {L"-h", TypeFlag},
    {L"-help", TypeFlag},
    {L"-l", TypeValue},
    {L"-m", TypeValue},
    {L"-r", TypeValue},
    {L"-skip", TypeValue},
    {L"-skipmodule", TypeValue},
    {L"-v", TypeValue},
    {NULL     , TypeMax}
};

/**
 * @brief   MPAM Compliance Suite Entry Point.
 *
 * Call the Entry points of individual modules.
 *
 * @retval  0       The application exited normally.
 * @retval  Other   An error occurred.
 */
UINT32
execute_tests()
{
    VOID               *branch_label;
    UINT32             Status;
    UINT32             msc_node_cnt;
    acs_run_request_t  *ctx;

    ctx = acs_get_run_request_mut();
    Status = apply_cli_defaults(ctx);
    if (Status != ACS_STATUS_PASS) {
        acs_release_run_request(ctx);
        return Status;
    }

    branch_label = &&print_test_status;
    val_pe_context_save(AA64ReadSp(), (uint64_t)branch_label);

    Print(L"\n\nMPAM System Architecture Compliance Suite \n");
    Print(L"    Version %d.%d.", MPAM_ACS_MAJOR_VER, MPAM_ACS_MINOR_VER);
    Print(L"%d  \n", MPAM_ACS_SUBMINOR_VER);

    Print(L"\nStarting tests for Print level %2d\n", acs_policy_get_print_level());

    Print(L"\nCreating Platform Information Tables");
    Status = createPeInfoTable();
    if (Status)
        goto cleanup;

    /* check if PE supports MPAM extension, else skip all MPAM tests */
    if (val_pe_feat_check(PE_FEAT_MPAM)) {
        val_print(INFO,
                  "\n       PE MPAM extension unimplemented. Skipping all MPAM tests\n");
        goto print_test_status;
    }

    Status = createGicInfoTable();
    if (Status)
        goto cleanup;

    createIovirtInfoTable();

    createCacheInfoTable();

    /* required before calling createMpamInfoTable() */
    createPccInfoTable();

    createHmatInfoTable();

    createSratInfoTable();

    createMpamInfoTable();
    val_mpam_update_msc_device_names();

    /* Get total number of MSCs reported by MPAM ACPI table */
    msc_node_cnt = val_mpam_get_msc_count();
    if (msc_node_cnt == 0) {
        val_print(INFO, "\n      *** Exiting suite - No MPAM nodes *** \n");
        goto print_test_status;
    }

    val_allocate_shared_mem();

    /*
     * Initialise exception vector, so any unexpected exception gets handled
     * by default MPAM exception handler
     */
    val_pe_initialize_default_exception_handler(val_pe_default_esr);
    FlushImage();

    if ((ctx->rule_count > 0 && ctx->rule_list != NULL) || (ctx->arch_selection != ARCH_NONE)) {
        filter_rule_list_by_cli(ctx);
        if (ctx->rule_count == 0 || ctx->rule_list == NULL)
            goto print_test_status;

        print_selection_summary();
        run_tests(ctx);
    } else {
        val_print(INFO, "\nNo rules selected for execution.\n");
    }

print_test_status:
    val_print_acs_test_status_summary();

    if (g_acs_log_file_handle) {
        ShellCloseFile(&g_acs_log_file_handle);
    }

    Print(L"\n      *** MPAM tests complete. Reset the system. *** \n\n");

cleanup:
    FreeMpamAcsMem();

    acs_release_run_request(ctx);

    val_pe_context_restore(AA64WriteSp(g_stack_pointer));

    return Status;
}
