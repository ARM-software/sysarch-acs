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

/* CLI parameter table for PCBSA ACS, for description refer HelpMsg */
CONST SHELL_PARAM_ITEM ParamList[] = {
    {L"-el1physkip", TypeFlag},
    {L"-f", TypeValue},
    {L"-fr", TypeValue},
    {L"-h", TypeFlag},
    {L"-help", TypeFlag},
    {L"-l", TypeValue},
    {L"-m", TypeValue},
    {L"-mmio", TypeFlag},
    {L"-only", TypeValue},
    {L"-r", TypeValue},
    {L"-skip", TypeValue},
    {L"-skipmodule", TypeValue},
    {L"-timeout", TypeValue},
    {L"-v", TypeValue},
    {NULL, TypeMax}
};

/* Limit chars to 75 chars for each new line in HelpMsg for neater print in standard consoles
   and sorted in alphabetical order */
VOID
HelpMsg (VOID)
{
    Print (L"\nUsage: PcBsa.efi [options]\n"
        "Options:\n"
        "-el1physkip \n"
        "        Skips EL1 register checks\n"
        "        VE systems run ACS at EL1 and in some systems crash is observed\n"
        "        during access of EL1 registers, this flag was introduced\n"
        "        for debugging purposes only.\n"
        "-f      Name of the log file to record the test results in\n"
        "-fr     Run rules up to the Future requirements (FR) level.\n"
        "-h, -help\n"
        "        Print this message\n"
        "-l <n>  Run compliance tests up till inputted level.\n"
        "-m      Run only the specified modules (comma-separated names).\n"
        "        Accepted: PE, GIC, PERIPHERAL, MEM_MAP, PMU, RAS, SMMU,\n"
        "                  TIMER, WATCHDOG, NIST, PCIE, MPAM, ETE, TPM, POWER_WAKEUP\n"
        "        Example: -m PE,GIC,PCIE\n"
        "-mmio   Pass this flag to enable pal_mmio_read/write prints, use with -v 1\n"
        "-r      Run tests for passed comma-separated Rule IDs or a rules file\n"
        "        Examples: -r B_PE_01,B_PE_02,B_GIC_01\n"
        "                  -r rules.txt  (file may mix commas/newlines; lines \n"
        "                     starting with # are comments)\n"
        "-only <n> \n"
        "        Only run tests for rules at level <n> \n"
        "-skip   Rule ID(s) to be skipped (comma-separated, like -r)\n"
        "        Example: -skip B_PE_01,B_GIC_02\n"
        "-skipmodule \n"
        "        Skip the specified modules (comma-separated names).\n"
        "        Example: -skipmodule PE,GIC,PCIE\n"
        "-timeout <n> \n"
        "        Set timeout multiple for wakeup tests\n"
        "        1 - min value  5 - max value, Defaults to 1 \n"
        "-v <n>  Verbosity of the prints\n"
        "        1 prints all, 5 prints only the errors\n");
}

VOID
freeAcsMem()
{
    val_pe_free_info_table();
    val_gic_free_info_table();
    val_timer_free_info_table();
    val_wd_free_info_table();
    val_pcie_free_info_table();
    val_iovirt_free_info_table();
    val_peripheral_free_info_table();
    val_tpm2_free_info_table();
    val_free_shared_mem();
    val_srat_free_info_table();
}

static UINT32
apply_cli_defaults(VOID)
{
    /* Standalone PCBSA UEFI app, set g_arch_selection to PCBSA if -r empty */
    if (g_rule_count == 0) {
        g_arch_selection = ARCH_PCBSA;
    }

    /* Set Default level for the run if level filtering CLI options (-l, -only or -fr) is
       not passed and set filter mode to LVL_FILTER_MAX for filter_rule_list_by_cli logic to work
       */
    if (g_level_filter_mode == LVL_FILTER_NONE) {
        g_level_value = PCBSA_LEVEL_1;
        g_level_filter_mode = LVL_FILTER_MAX;
    }

    /* Check sanity of value of level */
    if (g_level_value >= PCBSA_LEVEL_SENTINEL) {
        val_print(ACS_PRINT_ERR, "\nInvalid level value passed (%d), ", g_level_value);
        val_print(ACS_PRINT_ERR, "value should be less than %d.", PCBSA_LEVEL_SENTINEL);
        return ACS_STATUS_FAIL;
    }

    return ACS_STATUS_PASS;
}

UINT32
execute_tests()
{
    VOID               *branch_label;
    UINT32             Status;

    Status = apply_cli_defaults();
    if (Status != ACS_STATUS_PASS) {
        goto exit_acs;
    }

    val_print(ACS_PRINT_TEST, "\n\n PC BSA Architecture Compliance Suite", 0);
    val_print(ACS_PRINT_TEST, "\n          Version %d.", PC_BSA_ACS_MAJOR_VER);
    val_print(ACS_PRINT_TEST, "%d.", PC_BSA_ACS_MINOR_VER);
    val_print(ACS_PRINT_TEST, "%d\n", PC_BSA_ACS_SUBMINOR_VER);


    val_print(ACS_PRINT_TEST, LEVEL_PRINT_FORMAT(g_level_value, g_level_filter_mode,
              BSA_LEVEL_FR), g_level_value);

    val_print(ACS_PRINT_TEST, "(Print level is %2d)\n\n", g_print_level);
    val_print(ACS_PRINT_TEST, "\n Creating Platform Information Tables\n", 0);


    Status = createPeInfoTable();
    if (Status) {
        goto exit_acs;
    }

    Status = createGicInfoTable();
    if (Status) {
        goto exit_acs;
    }

    /* Initialise exception vector, so any unexpected exception gets handled by default
    BSA exception handler */
    branch_label = &&print_test_status;
    val_pe_context_save(AA64ReadSp(), (uint64_t)branch_label);
    val_pe_initialize_default_exception_handler(val_pe_default_esr);

    createTimerInfoTable();
    createWatchdogInfoTable();
    createPcieVirtInfoTable();
    createPeripheralInfoTable();
    createTpm2InfoTable();
    createSratInfoTable();
    val_drtm_create_info_table();
    val_allocate_shared_mem();

    FlushImage();

    if ((g_rule_count > 0 && g_rule_list != NULL) || (g_arch_selection != ARCH_NONE)) {
        /* Merge arch rules if any, then apply CLI filters (-skip, -m, -skipmodule) */
        g_rule_count = filter_rule_list_by_cli(&g_rule_list, g_rule_count);
        if (g_rule_count == 0 || g_rule_list == NULL)
            goto exit_acs;

        /* Print rule selections */
        print_selection_summary();

        /* Run rule based test orchestrator */
        run_tests(g_rule_list, g_rule_count);
    }

print_test_status:
    val_print_acs_test_status_summary();
    val_print(ACS_PRINT_ERR, "\n      *** PC BSA tests complete. Reset the system. ***\n\n", 0);

    freeAcsMem();

exit_acs:
    if (g_acs_log_file_handle) {
        ShellCloseFile(&g_acs_log_file_handle);
    }

    val_pe_context_restore(AA64WriteSp(g_stack_pointer));
    return ACS_STATUS_PASS;
}
