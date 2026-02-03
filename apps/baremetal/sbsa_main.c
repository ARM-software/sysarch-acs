/** @file
 * Copyright (c) 2022-2026, Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/val_interface.h"
#include "val/include/pal_interface.h"
#include "val/include/acs_val.h"
#include "val/include/acs_memory.h"
#include "val/include/acs_pe.h"
#include "val/include/acs_dma.h"
#include "acs_el3_param.h"
#include "acs.h"

void
freeAcsMeM()
{
    val_pe_free_info_table();
    if (acs_is_module_enabled(PE)          ||
        acs_is_module_enabled(GIC)         ||
        acs_is_module_enabled(PCIE)        ||
        acs_is_module_enabled(MPAM)        ||
        acs_is_module_enabled(RAS)         ||
       acs_is_module_enabled(ETE)         ||
       acs_is_module_enabled(PMU))
       val_gic_free_info_table();

    if (acs_is_module_enabled(GIC)          ||
       acs_is_module_enabled(TIMER))
       val_timer_free_info_table();

    if (acs_is_module_enabled(WATCHDOG))
       val_wd_free_info_table();

    if (acs_is_module_enabled(MPAM))
       val_cache_free_info_table();

    if (acs_is_module_enabled(MPAM))
    {
        val_mpam_free_info_table();
        val_hmat_free_info_table();
        val_srat_free_info_table();
        val_pcc_free_info_table();
    }

   if (acs_is_module_enabled(PCIE))
       val_pcie_free_info_table();

   if (acs_is_module_enabled(SMMU)   ||
       acs_is_module_enabled(MEM_MAP) ||
       acs_is_module_enabled(PCIE))
       val_iovirt_free_info_table();

   if (acs_is_module_enabled(PE)     ||
       acs_is_module_enabled(PCIE)   ||
       acs_is_module_enabled(MEM_MAP) ||
       acs_is_module_enabled(MPAM))
       val_peripheral_free_info_table();

   if (acs_is_module_enabled(PMU))
       val_pmu_free_info_table();

   if (acs_is_module_enabled(RAS))
   {
       val_ras2_free_info_table();
       val_ras_free_info_table();
   }

   val_free_shared_mem();
}

/* This routine will furnish global variables with user defined config and set any
   default values for the ACS */
uint32_t apply_user_config_and_defaults(void)
{
    /* Set user defined compliance level to be run for
       as defined pal/baremetal/target/../include/platform_override_fvp.h  */
    g_level_value  = PLATFORM_OVERRIDE_SBSA_LEVEL;
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

    /* Set default values for g_print_mmio, g_wakeup_timeout */
    g_print_mmio = 0;
    g_wakeup_timeout = 1;

    /* If selected rule count is zero, default to SBSA */
    if (g_rule_count == 0) {
        /* Standalone SBSA Baremetal app, set g_arch_selection to SBSA */
        g_arch_selection = ARCH_SBSA;
    }

    /* Check sanity of value of level if not valid default to extremes */
    if (g_level_value < SBSA_LEVEL_3) {
        val_print(g_print_level, "\nSBSA Level %d is not supported.\n", g_level_value);
        val_print(g_print_level, "\nSetting SBSA level to %d\n", SBSA_LEVEL_3);
        g_level_value = SBSA_LEVEL_3;
    } else if (g_level_value >= SBSA_LEVEL_SENTINEL) {
        val_print(g_print_level, "\nSBSA Level %d is not supported.\n", g_level_value);
        val_print(g_print_level, "\nSetting SBSA level FR", SBSA_LEVEL_FR);
        g_level_value = SBSA_LEVEL_FR;
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

    /* Set g_build_sbsa hint for test */
    g_build_sbsa = 1;

    return ACS_STATUS_PASS;

}

void
freeSbsaAvsMem()
{
  val_pe_free_info_table();
  if (acs_is_module_enabled(PE)          ||
      acs_is_module_enabled(GIC)         ||
      acs_is_module_enabled(PCIE)        ||
      acs_is_module_enabled(MPAM)        ||
      acs_is_module_enabled(RAS)         ||
      acs_is_module_enabled(ETE)         ||
      acs_is_module_enabled(PMU))
      val_gic_free_info_table();

  if (acs_is_module_enabled(GIC)          ||
      acs_is_module_enabled(TIMER))
      val_timer_free_info_table();

  if (acs_is_module_enabled(WATCHDOG))
      val_wd_free_info_table();

  if (acs_is_module_enabled(MPAM))
      val_cache_free_info_table();

  if (acs_is_module_enabled(MPAM))
  {
      val_mpam_free_info_table();
      val_hmat_free_info_table();
      val_srat_free_info_table();
      val_pcc_free_info_table();
  }

  if (acs_is_module_enabled(PCIE))
      val_pcie_free_info_table();

  if (acs_is_module_enabled(SMMU)   ||
      acs_is_module_enabled(MEM_MAP) ||
      acs_is_module_enabled(PCIE))
      val_iovirt_free_info_table();

  if (acs_is_module_enabled(PE)     ||
      acs_is_module_enabled(PCIE)   ||
      acs_is_module_enabled(MEM_MAP) ||
      acs_is_module_enabled(MPAM))
      val_peripheral_free_info_table();

  if (acs_is_module_enabled(PMU))
      val_pmu_free_info_table();

  if (acs_is_module_enabled(RAS))
  {
      val_ras2_free_info_table();
      val_ras_free_info_table();
  }

  val_free_shared_mem();

}

/***
  SBSA Compliance Suite Entry Point.

  Call the Entry points of individual modules.

  @retval  0         The application exited normally.
  @retval  Other     An error occurred.
***/
int32_t
ShellAppMainsbsa()
{
    uint32_t             Status = ACS_STATUS_SKIP;
    void                 *branch_label;

    Status = apply_user_config_and_defaults();
    if (Status != ACS_STATUS_PASS) {
        val_print(ACS_PRINT_ERR, "\napply_user_config_and_defaults() failed, Exiting...\n", 0);
        goto exit_acs;
    }


#if ACS_ENABLE_MMU
  /* Create MMU page tables before enabling the MMU at EL2 */
  if (val_setup_mmu())
      return ACS_STATUS_FAIL;

  /* Enable Stage-1 MMU */
  if (val_enable_mmu())
      return ACS_STATUS_FAIL;
#else
  val_print(ACS_PRINT_TEST, "Skipping MMU setup/enable (ACS_ENABLE_MMU=0)\n", 0);
#endif
    /* NEW: apply any compile-time test/module overrides before
    *      we look at g_num_tests/g_num_modules and build masks.
    */
    acs_apply_compile_params();
    /* NEW: apply any EL3-supplied test/module overrides before
    *      we look at g_num_tests/g_num_modules and build masks.
    */
    acs_apply_el3_params();

    val_print(ACS_PRINT_TEST, "\n\n SBSA Architecture Compliance Suite\n", 0);
    val_print(ACS_PRINT_TEST, "    Version %d.", SBSA_ACS_MAJOR_VER);
    val_print(ACS_PRINT_TEST, "%d.", SBSA_ACS_MINOR_VER);
    val_print(ACS_PRINT_TEST, "%d\n", SBSA_ACS_SUBMINOR_VER);


    val_print(ACS_PRINT_TEST, LEVEL_PRINT_FORMAT(g_level_value, g_level_filter_mode,
                SBSA_LEVEL_FR), g_level_value);

    val_print(ACS_PRINT_TEST, "(Print level is %2d)\n\n", g_print_level);

    val_print(ACS_PRINT_TEST, " Creating Platform Information Tables\n", 0);

    Status = createPeInfoTable();
    if (Status)
        return Status;

    if (acs_is_module_enabled(PE)          ||
      acs_is_module_enabled(GIC)         ||
      acs_is_module_enabled(PCIE)        ||
      acs_is_module_enabled(MPAM)        ||
      acs_is_module_enabled(RAS)         ||
      acs_is_module_enabled(ETE)         ||
      acs_is_module_enabled(PMU))
    {
        Status = createGicInfoTable();
        if (Status)
            return Status;
    }

    if (acs_is_module_enabled(GIC)          ||
        acs_is_module_enabled(TIMER))
        createTimerInfoTable();

    if (acs_is_module_enabled(WATCHDOG))
        createWatchdogInfoTable();

    if (acs_is_module_enabled(MPAM))
        createCacheInfoTable();

    if (acs_is_module_enabled(MPAM))
    {
        createPccInfoTable();
        createMpamInfoTable();
        createHmatInfoTable();
        createSratInfoTable();
    }

    if (acs_is_module_enabled(PCIE))
        createPcieInfoTable();

    if (acs_is_module_enabled(SMMU)        ||
        acs_is_module_enabled(MEM_MAP)      ||
        acs_is_module_enabled(PCIE))
        createIoVirtInfoTable();

    if (acs_is_module_enabled(PE)          ||
        acs_is_module_enabled(PCIE)        ||
        acs_is_module_enabled(MEM_MAP)      ||
        acs_is_module_enabled(MPAM))
        createPeripheralInfoTable();

    if (acs_is_module_enabled(PE)          ||
        acs_is_module_enabled(SMMU)        ||
        acs_is_module_enabled(MEM_MAP))
        createMemoryInfoTable();

    if (acs_is_module_enabled(PMU))
        createPmuInfoTable();

    if (acs_is_module_enabled(RAS))
    {
        createRasInfoTable();
        createRas2InfoTable();
    }

    val_allocate_shared_mem();

    /* Initialise exception vector, so any unexpected exception gets handled
    *  by default SBSA exception handler.
    */
    branch_label = &&print_test_status;
    val_pe_context_save(AA64ReadSp(), (uint64_t)branch_label);
    val_pe_initialize_default_exception_handler(val_pe_default_esr);

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
    val_print(ACS_PRINT_ERR, "\n      *** SBSA tests complete. Reset the system. ***\n\n", 0);
exit_acs:
    freeAcsMeM();

    val_pe_context_restore(AA64WriteSp(g_stack_pointer));
    while (1);
    return 0;
}
