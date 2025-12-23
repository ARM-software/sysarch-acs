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
#include  <Protocol/ShellParameters.h>

#include "val/include/val_interface.h"
#include "val/include/acs_pe.h"
#include "val/include/acs_val.h"
#include "val/include/acs_memory.h"
#include "val/include/rule_based_execution.h"
#include "acs.h"

/* CLI parameter table for xBSA UEFI application, for description refer HelpMsg */
CONST SHELL_PARAM_ITEM ParamList[] = {
    {L"-a", TypeValue},
    {L"-cache", TypeFlag},
    {L"-dtb", TypeValue},
    {L"-el1physkip", TypeFlag},
    {L"-f", TypeValue},
    {L"-fr", TypeFlag},
    {L"-h", TypeFlag},
    {L"-help", TypeFlag},
    {L"-hyp", TypeFlag},
    {L"-l", TypeValue},
    {L"-m", TypeValue},
    {L"-mmio", TypeFlag},
    {L"-no_crypto_ext", TypeFlag},
    {L"-only", TypeValue},
    {L"-os", TypeFlag},
    {L"-p2p", TypeFlag},
    {L"-ps", TypeFlag},
    {L"-r", TypeValue},
    {L"-skip", TypeValue},
    {L"-skip-dp-nic-ms", TypeFlag},
    {L"-skipmodule", TypeValue},
    {L"-slc", TypeValue},
    {L"-timeout", TypeValue},
    {L"-v", TypeValue},
    {NULL, TypeMax}
    };

/* Limit chars to 75 chars for each new line in HelpMsg for neater print in standard consoles
   and sorted in alphabetical order */
VOID
HelpMsg (VOID)
{
    Print (L"\nUsage: xbsa_acpi.efi [options]\n"
        "Options:\n"
        "-a      Architecture selection: 'bsa', 'sbsa', or 'pcbsa'\n"
        "        -a bsa    Use full BSA rule checklist \n"
        "        -a sbsa   Use full SBSA rule checklist \n"
        "        -a pcbsa  Use full PC BSA rule checklist \n"
        "-cache  Pass this flag to indicate that if the test system supports\n"
        "        PCIe address translation cache\n"
        "-dtb    Pass this flag to dump DTB file (Device Tree Blob) \n"
        "-el1physkip \n"
        "        Skips EL1 register checks\n"
        "        VE systems run ACS at EL1 and in some systems crash is observed\n"
        "        during access of EL1 registers, this flag was introduced\n"
        "        for debugging purposes only.\n"
        "-f      Name of the log file to record the test results in\n"
        "-fr     Run rules up to the Future requirements (FR) level.\n"
        "        The flag will be validated against -a selected,\n"
        "        e.g -a sbsa -fr will run tests required for SBSA compliance\n"
        "        future requirements level (highest level). \n"
        "-h, -help\n"
        "        Print this message\n"
        "-l <n>  Run compliance tests up till inputted level.\n"
        "        Example: -l 4\n"
        "        The level passed will be validated against -a selected,\n"
        "        e.g -a sbsa -l 7 will run tests required for\n"
        "            SBSA level 7 compliance.\n"
        "-m      Run only the specified modules (comma-separated names).\n"
        "        Accepted: PE, GIC, PERIPHERAL, MEM_MAP, PMU, RAS, SMMU,\n"
        "                  TIMER, WATCHDOG, NIST, PCIE, MPAM, ETE, TPM, POWER_WAKEUP\n"
        "        Example: -m PE,GIC,PCIE\n"
        "-mmio   Pass this flag to enable pal_mmio_read/write prints, use with -v 1\n"
        "-no_crypto_ext \n"
        "        Pass this flag if cryptography extension not supported\n"
        "        due to export restrictions\n"
        "-only <n> \n"
        "        Only run tests for rules at level <n> \n"
        "        The level passed will be validated against -a selected, e.g -a \n"
        "        sbsa -only 4 will run tests for rule ids specified for \n"
        "        SBSA at level 4 of specification \n"
        "-os, -hyp, -ps \n"
        "        Software view filter (works with -a bsa only; can be combined).\n"
        "        Pass -os  to run BSA Operating system software view tests.\n"
        "        Pass -hyp to run BSA Hypervisior software view tests.\n"
        "        Pass -ps  to run BSA Platform security software view tests.\n"
        "-p2p    Pass this flag to indicate that PCIe Hierarchy Supports Peer-to-Peer\n"
        "-r      Run tests for passed comma-separated Rule IDs or a rules file\n"
        "        Examples: -r B_PE_01,B_PE_02,B_GIC_01\n"
        "                  -r rules.txt  (file may mix commas/newlines; lines \n"
        "                     starting with # are comments)\n"
        "-slc    Provide system last level cache type\n"
        "        1 - PPTT PE-side cache,  2 - HMAT mem-side cache\n"
        "-skip   Rule ID(s) to be skipped (comma-separated, like -r)\n"
        "        Example: -skip B_PE_01,B_GIC_02\n"
        "-skip-dp-nic-ms \n"
        "        Skip PCIe tests for DisplayPort, Network, and Mass Storage devices\n"
        "-skipmodule \n"
        "        Skip the specified modules (comma-separated names).\n"
        "        Example: -skipmodule PE,GIC,PCIE\n"
        "-timeout <n> \n"
        "        Set timeout multiple for wakeup tests\n"
        "        1 - min value  5 - max value, Defaults to 1 \n"
        "-v <n>  Verbosity of the prints\n"
        "        1 prints all, 5 prints only the errors\n"
    );
}

static VOID
freeAcsMem(void)
{
    val_pe_free_info_table();
    val_gic_free_info_table();
    val_timer_free_info_table();
    val_wd_free_info_table();
    val_pcie_free_info_table();
    val_iovirt_free_info_table();
    val_peripheral_free_info_table();
    val_smbios_free_info_table();
    val_pmu_free_info_table();
    val_cache_free_info_table();
    val_mpam_free_info_table();
    val_hmat_free_info_table();
    val_srat_free_info_table();
    val_ras2_free_info_table();
    val_pcc_free_info_table();
    val_tpm2_free_info_table();
    val_free_shared_mem();
}

/* Apply default values to -a -l options if no value passed to cli */
static UINT32
apply_cli_defaults(VOID)
{
    if (g_arch_selection == ARCH_NONE && g_rule_count == 0) {
        Print(L"No -r or -a specified; defaulting to -a bsa\n");
        g_arch_selection = ARCH_BSA;
    }

    if (g_bsa_sw_view_mask != 0 && g_arch_selection != ARCH_BSA) {
        Print(L"-hyp/-os/-ps ignored: requires -a bsa.\n");
        g_bsa_sw_view_mask = 0;
    }

    if (g_level_filter_mode == LVL_FILTER_NONE && g_arch_selection != ARCH_NONE) {
        if (g_arch_selection == ARCH_BSA) {
            g_level_filter_mode = LVL_FILTER_MAX;
            g_level_value = BSA_LEVEL_1;
        } else if (g_arch_selection == ARCH_SBSA) {
            g_level_filter_mode = LVL_FILTER_MAX;
            g_level_value = SBSA_LEVEL_4;
        } else if (g_arch_selection == ARCH_PCBSA) {
            g_level_filter_mode = LVL_FILTER_MAX;
            g_level_value = PCBSA_LEVEL_1;
        }
    }

    return ACS_STATUS_PASS;
}

UINT32
execute_tests()
{
    VOID               *branch_label;
    UINT32             Status;

    /* Apply any ACS specific default values */
    Status = apply_cli_defaults();
    if (Status != ACS_STATUS_PASS) {
        goto exit_acs;
    }

    /* Print ACS header */
    val_print(ACS_PRINT_TEST, "\n\nxBSA Architecture Compliance Suite", 0);
    val_print(ACS_PRINT_TEST, "\n          Version %d.", XBSA_ACS_MAJOR_VER);
    val_print(ACS_PRINT_TEST, "%d.", XBSA_ACS_MINOR_VER);
    val_print(ACS_PRINT_TEST, "%d\n", XBSA_ACS_SUBMINOR_VER);
    val_print(ACS_PRINT_TEST, "(Print level is %2d)\n\n", g_print_level);
    val_print(ACS_PRINT_TEST, "\n       Creating Platform Information Tables\n", 0);


    /* Create info tables */
    Status = createPeInfoTable();
    if (Status) {
            if (g_acs_log_file_handle)
                ShellCloseFile(&g_acs_log_file_handle);
            return Status;
    }
    Status = createGicInfoTable();
    if (Status) {
            if (g_acs_log_file_handle)
                ShellCloseFile(&g_acs_log_file_handle);
            return Status;
    }

    /* Initialise exception vector, so any unexpected exception gets handled by default
    BSA exception handler */
    branch_label = &&exit_acs;
    val_pe_context_save(AA64ReadSp(), (uint64_t)branch_label);
    val_pe_initialize_default_exception_handler(val_pe_default_esr);

    createTimerInfoTable();
    createWatchdogInfoTable();
    createPcieVirtInfoTable();
    createPeripheralInfoTable();
    createSmbiosInfoTable();
    createCacheInfoTable();
    createPccInfoTable();
    createMpamInfoTable();
    createHmatInfoTable();
    createSratInfoTable();
    createRas2InfoTable();
    createPmuInfoTable();
    createRasInfoTable();
    createTpm2InfoTable();
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

    val_print_acs_test_status_summary();
    val_print(ACS_PRINT_ERR, "\n      *** ACS tests complete. Reset the system. ***\n\n", 0);

    freeAcsMem();

exit_acs:
    /* Close any file handles open */
    if (g_dtb_log_file_handle) {
        ShellCloseFile(&g_dtb_log_file_handle);
    }
    if (g_acs_log_file_handle) {
        ShellCloseFile(&g_acs_log_file_handle);
    }

    val_pe_context_restore(AA64WriteSp(g_stack_pointer));
    return ACS_STATUS_PASS;;
}
