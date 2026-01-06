/** @file
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/pal_interface.h"
#include "val/include/val_interface.h"
#include "val/include/acs_pe.h"
#include "val/include/acs_val.h"
#include "val/include/acs_memory.h"
#include "val/include/acs_dma.h"
#include "acs.h"

void
freeAcsMeM()
{
    val_pe_free_info_table();
    val_gic_free_info_table();
    val_timer_free_info_table();
    val_wd_free_info_table();
    val_pcie_free_info_table();
    val_iovirt_free_info_table();
    val_peripheral_free_info_table();
    val_smbios_free_info_table();
    val_dma_free_info_table();
    val_free_shared_mem();
}

/* This routine will furnish global variables with user defined config and set any
   default values for the ACS */
uint32_t apply_user_config_and_defaults(void)
{
    /* Set user defined compliance level to be run for
       as defined pal/baremetal/target/../include/platform_override_fvp.h  */
    g_level_value  = PLATFORM_OVERRIDE_BSA_LEVEL;
    g_print_level = PLATFORM_OVERRIDE_PRINT_LEVEL;

    /* Set user defined configuration from pal/baremetal/target/../src/platform_cfg_fvp.c*/
    if (g_rule_count) {
        g_rule_list = g_rule_list_arr;
    }
    if (g_skip_rule_count) {
        g_skip_rule_list = g_skip_rule_list_arr;
    }
    if (g_num_modules) {
        g_execute_modules = g_execute_modules_arr;
    }
    if (g_num_skip_modules) {
        g_skip_modules = g_skip_modules_arr;
    }

    /* Set default values for g_print_mmio and g_wakeup_timeout */
    g_print_mmio = 0;
    g_wakeup_timeout = 1;

    /* If selected rule count is zero, default to BSA */
    if (g_rule_count == 0) {
        /* Standalone BSA Baremetal app, set g_arch_selection to BSA */
        g_arch_selection = ARCH_BSA;
    }

    /* Check sanity of value of level if not valid default to extremes */
    if (g_level_value < BSA_LEVEL_1) {
        val_print(g_print_level, "\nBSA Level %d is not supported.\n", g_level_value);
        val_print(g_print_level, "\nSetting BSA level to %d\n", BSA_LEVEL_1);
        g_level_value = BSA_LEVEL_1;
    } else if (g_level_value >= BSA_LEVEL_SENTINEL) {
        val_print(g_print_level, "\nBSA Level %d is not supported.\n", g_level_value);
        val_print(g_print_level, "\nSetting BSA level FR", BSA_LEVEL_FR);
        g_level_value = BSA_LEVEL_FR;
    }

    /* Check sanity of print level, default accordingly */
    if (g_print_level < ACS_PRINT_INFO) {
        val_print(ACS_PRINT_ERR, "\nPrint Level %d is not supported.\n", g_print_level);
        val_print(ACS_PRINT_ERR, "\nSetting Print level to %d\n", ACS_PRINT_INFO);
        g_print_level = ACS_PRINT_INFO;
    } else if (g_print_level > ACS_PRINT_ERR) {
        val_print(ACS_PRINT_ERR, "\nPrint Level %d is not supported.\n", g_print_level);
        val_print(ACS_PRINT_ERR, "\nSetting Print level to %d\n", ACS_PRINT_ERR);
        g_print_level = ACS_PRINT_ERR;
    }

    return ACS_STATUS_PASS;

}

/***
  BSA Compliance Suite Entry Point.

  Call the Entry points of individual modules.

  @retval  0         The application exited normally.
  @retval  Other     An error occurred.
***/
int32_t
ShellAppMainbsa()
{

    uint32_t             Status;
    void                 *branch_label;

    Status = apply_user_config_and_defaults();
    if (Status != ACS_STATUS_PASS) {
        val_print(ACS_PRINT_ERR, "\napply_user_config_and_defaults() failed, Exiting...\n", 0);
        goto exit_acs;
    }

    /* Create MMU page tables before enabling the MMU at EL2 */
    if (val_setup_mmu())
        return ACS_STATUS_FAIL;

    /* Enable Stage-1 MMU */
    if (val_enable_mmu())
        return ACS_STATUS_FAIL;

    val_print(ACS_PRINT_TEST, "\n\n BSA Architecture Compliance Suite\n", 0);
    val_print(ACS_PRINT_TEST, "\n          Version %d.", BSA_ACS_MAJOR_VER);
    val_print(ACS_PRINT_TEST, "%d.", BSA_ACS_MINOR_VER);
    val_print(ACS_PRINT_TEST, "%d\n", BSA_ACS_SUBMINOR_VER);

    val_print(ACS_PRINT_TEST, LEVEL_PRINT_FORMAT(g_level_value, g_level_filter_mode,
                BSA_LEVEL_FR), g_level_value);

    val_print(ACS_PRINT_TEST, "(Print level is %2d)\n\n", g_print_level);

    val_print(ACS_PRINT_TEST, " Creating Platform Information Tables\n", 0);

    Status = createPeInfoTable();
    if (Status)
        return Status;

    Status = createGicInfoTable();
    if (Status)
        return Status;

    /* Initialise exception vector, so any unexpected exception gets handled
    *  by default exception handler.
    */
    branch_label = &&print_test_status;
    val_pe_context_save(AA64ReadSp(), (uint64_t)branch_label);
    val_pe_initialize_default_exception_handler(val_pe_default_esr);

    createTimerInfoTable();
    createWatchdogInfoTable();
    createPcieVirtInfoTable();
    createPeripheralInfoTable();
    createDmaInfoTable();
    createSmbiosInfoTable();
    val_allocate_shared_mem();

    if ((g_rule_count > 0 && g_rule_list != NULL) || (g_arch_selection != ARCH_NONE)) {
            /* Merge arch rules if any, then apply CLI filters (-skip, -m, -skipmodule) */
            g_rule_count = filter_rule_list_by_cli(&g_rule_list, g_rule_count);
            if (g_rule_count == 0 || g_rule_list == NULL) {
                val_print(ACS_PRINT_ERR, "\nRule list empty, nothing to execute, Exiting...\n", 0);
                return -1;
            }

            /* Run rule based test orchestrator */
            run_tests(g_rule_list, g_rule_count);
    } else {
        val_print(ACS_PRINT_ERR, "\nInvalid rule list or arch selected, Exiting...\n", 0);
        return -1;
    }

print_test_status:
    val_print_acs_test_status_summary();
    val_print(ACS_PRINT_ERR, "\n      *** BSA tests complete. Reset the system. ***\n\n", 0);
exit_acs:
    freeAcsMeM();

    val_pe_context_restore(AA64WriteSp(g_stack_pointer));
    while (1);
    return 0;
}
