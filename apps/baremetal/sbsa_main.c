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
freeAcsMeM(void)
{
    val_pe_free_info_table();
    if (acs_is_module_enabled(PE)         ||
      acs_is_module_enabled(GIC)          ||
      acs_is_module_enabled(TIMER)        ||
      acs_is_module_enabled(WATCHDOG)     ||
      acs_is_module_enabled(PERIPHERAL)   ||
      acs_is_module_enabled(POWER_WAKEUP) ||
      acs_is_module_enabled(PCIE)         ||
      acs_is_module_enabled(MPAM)         ||
      acs_is_module_enabled(RAS)          ||
      acs_is_module_enabled(ETE)          ||
      acs_is_module_enabled(PMU))
       val_gic_free_info_table();

    if (acs_is_module_enabled(TIMER)       ||
        acs_is_module_enabled(GIC)         ||
        acs_is_module_enabled(WATCHDOG)    ||
        acs_is_module_enabled(POWER_WAKEUP))
       val_timer_free_info_table();

    if (acs_is_module_enabled(WATCHDOG)    ||
        acs_is_module_enabled(POWER_WAKEUP))
       val_wd_free_info_table();

    if (acs_is_module_enabled(MPAM))
    {
        val_cache_free_info_table();
        val_mpam_free_info_table();
        val_hmat_free_info_table();
        val_srat_free_info_table();
        val_pcc_free_info_table();
    }

    if (acs_is_module_enabled(PCIE)        ||
        acs_is_module_enabled(GIC)         ||
        acs_is_module_enabled(SMMU))
       val_pcie_free_info_table();

   if (acs_is_module_enabled(SMMU)         ||
       acs_is_module_enabled(GIC)          ||
       acs_is_module_enabled(MEM_MAP)      ||
       acs_is_module_enabled(PCIE))
       val_iovirt_free_info_table();

    if (acs_is_module_enabled(PE)          ||
        acs_is_module_enabled(RAS)         ||
        acs_is_module_enabled(PCIE)        ||
        acs_is_module_enabled(PERIPHERAL)  ||
        acs_is_module_enabled(MEM_MAP)     ||
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
uint32_t apply_user_config_and_defaults(acs_run_request_t *ctx)
{
    if (ctx == NULL)
        return ACS_STATUS_FAIL;

    acs_load_run_request_defaults(ctx);

    /* Set user defined compliance level to be run for
       as defined pal/baremetal/target/../include/platform_override_fvp.h  */
    ctx->level_value = PLATFORM_OVERRIDE_SBSA_LEVEL;
    g_print_level = PLATFORM_OVERRIDE_PRINT_LEVEL;

    /* Set default values for g_print_mmio */
    g_print_mmio = 0;

    /* If selected rule count is zero, default to SBSA */
    if (ctx->rule_count == 0) {
        /* Standalone SBSA Baremetal app, default the run request to SBSA */
        ctx->arch_selection = ARCH_SBSA;
    }

    /* Check sanity of value of level if not valid default to extremes */
    if (ctx->level_value < SBSA_LEVEL_3) {
        val_print(g_print_level, "\nSBSA Level %d is not supported.\n", ctx->level_value);
        val_print(g_print_level, "\nSetting SBSA level to %d\n", SBSA_LEVEL_3);
        ctx->level_value = SBSA_LEVEL_3;
    } else if (ctx->level_value >= SBSA_LEVEL_SENTINEL) {
        val_print(g_print_level, "\nSBSA Level %d is not supported.\n", ctx->level_value);
        val_print(g_print_level, "\nSetting SBSA level FR", SBSA_LEVEL_FR);
        ctx->level_value = SBSA_LEVEL_FR;
    }

    /* Check sanity of print level, default accordingly */
    if (g_print_level < TRACE) {
        val_print(ERROR, "\nPrint Level %d is not supported.\n", g_print_level);
        val_print(ERROR, "\nSetting Print level to %d\n", TRACE);
        g_print_level = TRACE;
    } else if (g_print_level > ERROR) {
        val_print(ERROR, "\nPrint Level %d is not supported.\n", g_print_level);
        val_print(ERROR, "\nSetting Print level to %d\n", ERROR);
        g_print_level = ERROR;
    }

    /* Set g_build_sbsa hint for test */
    g_build_sbsa = 1;

    return ACS_STATUS_PASS;

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
    acs_run_request_t    *ctx;

    acs_reset_run_request();
    ctx = acs_get_run_request_mut();

    Status = apply_user_config_and_defaults(ctx);
    if (Status != ACS_STATUS_PASS) {
        val_print(ERROR, "\napply_user_config_and_defaults() failed, Exiting...\n");
        goto exit_acs;
    }


#if ACS_ENABLE_MMU
  /* Create MMU page tables before enabling the MMU at EL2 */
  if (val_setup_mmu()) {
      acs_release_run_request(ctx);
      return ACS_STATUS_FAIL;
  }

  /* Enable Stage-1 MMU */
  if (val_enable_mmu()) {
      acs_release_run_request(ctx);
      return ACS_STATUS_FAIL;
  }
#else
  val_print(INFO, "Skipping MMU setup/enable (ACS_ENABLE_MMU=0)\n");
#endif
    /* Apply any compile-time test/module overrides before we consume the run request. */
    acs_apply_compile_params(ctx);
    /* Apply any EL3-supplied selection overrides before we consume the run request. */
    acs_apply_el3_params(ctx);

    val_print(INFO, "\n\n SBSA Architecture Compliance Suite\n");
    val_print(INFO, "    Version %d.", SBSA_ACS_MAJOR_VER);
    val_print(INFO, "%d.", SBSA_ACS_MINOR_VER);
    val_print(INFO, "%d\n", SBSA_ACS_SUBMINOR_VER);


    val_print(INFO, LEVEL_PRINT_FORMAT(ctx->level_value, ctx->level_filter_mode,
                SBSA_LEVEL_FR), ctx->level_value);

    val_print(INFO, "(Print level is %2d)\n\n", g_print_level);

    val_print(INFO, " Creating Platform Information Tables\n");

    Status = createPeInfoTable();
    if (Status) {
        acs_release_run_request(ctx);
        return Status;
    }

    if (acs_is_module_enabled(PE)         ||
      acs_is_module_enabled(GIC)          ||
      acs_is_module_enabled(TIMER)        ||
      acs_is_module_enabled(WATCHDOG)     ||
      acs_is_module_enabled(PERIPHERAL)   ||
      acs_is_module_enabled(POWER_WAKEUP) ||
      acs_is_module_enabled(PCIE)         ||
      acs_is_module_enabled(MPAM)         ||
      acs_is_module_enabled(RAS)          ||
      acs_is_module_enabled(ETE)          ||
      acs_is_module_enabled(PMU))
    {
        Status = createGicInfoTable();
        if (Status) {
            acs_release_run_request(ctx);
            return Status;
        }
    }

    if (acs_is_module_enabled(TIMER)      ||
        acs_is_module_enabled(GIC)        ||
        acs_is_module_enabled(WATCHDOG)   ||
        acs_is_module_enabled(POWER_WAKEUP))
        createTimerInfoTable();

    if (acs_is_module_enabled(WATCHDOG)   ||
        acs_is_module_enabled(POWER_WAKEUP))
        createWatchdogInfoTable();

    if (acs_is_module_enabled(MPAM))
    {
        createCacheInfoTable();
        createPccInfoTable();
        createMpamInfoTable();
        createHmatInfoTable();
        createSratInfoTable();
    }

    if (acs_is_module_enabled(PCIE)       ||
        acs_is_module_enabled(GIC)        ||
        acs_is_module_enabled(SMMU))
        createPcieInfoTable();

    if (acs_is_module_enabled(GIC)        ||
        acs_is_module_enabled(PCIE)       ||
        acs_is_module_enabled(MEM_MAP)    ||
        acs_is_module_enabled(SMMU))
        createIoVirtInfoTable();

    if (acs_is_module_enabled(PE)         ||
        acs_is_module_enabled(RAS)        ||
        acs_is_module_enabled(PCIE)       ||
        acs_is_module_enabled(PERIPHERAL) ||
        acs_is_module_enabled(MEM_MAP)    ||
        acs_is_module_enabled(MPAM))
        createPeripheralInfoTable();

    if (acs_is_module_enabled(PE)         ||
        acs_is_module_enabled(SMMU)       ||
        acs_is_module_enabled(MEM_MAP))
        createMemoryInfoTable();

    if (acs_is_module_enabled(PCIE)       ||
        acs_is_module_enabled(PERIPHERAL))
        createDmaInfoTable();

    if (acs_is_module_enabled(PMU))
        createPmuInfoTable();

    if (acs_is_module_enabled(RAS))
    {
        createRasInfoTable();
        createRas2InfoTable();
    }

    if (acs_is_module_enabled(CXL))
        createCxlInfoTable();

    val_allocate_shared_mem();

    /* Initialise exception vector, so any unexpected exception gets handled
    *  by default SBSA exception handler.
    */
    branch_label = &&print_test_status;
    val_pe_context_save(AA64ReadSp(), (uint64_t)branch_label);
    val_pe_initialize_default_exception_handler(val_pe_default_esr);

    if ((ctx->rule_count > 0 && ctx->rule_list != NULL) || (ctx->arch_selection != ARCH_NONE)) {
            /* Merge arch rules if any, then apply CLI filters (-skip, -m, -skipmodule) */
            filter_rule_list_by_cli(ctx);
            if (ctx->rule_count == 0 || ctx->rule_list == NULL) {
                val_print(ERROR, "\nRule list empty, nothing to execute, Exiting...\n");
                acs_release_run_request(ctx);
                return -1;
            }

            /* Run rule based test orchestrator */
            run_tests(ctx);
    } else {
        val_print(ERROR, "\nInvalid rule list or arch selected, Exiting...\n");
        acs_release_run_request(ctx);
        return -1;
    }

print_test_status:
    val_print_acs_test_status_summary();
    val_print(INFO, "\n      *** SBSA tests complete. Reset the system. ***\n\n");
exit_acs:
    freeAcsMeM();
    /* Release any request-owned CLI/EL3 selection lists before leaving ACS. */
    acs_release_run_request(ctx);

    val_pe_context_restore(AA64WriteSp(g_stack_pointer));
    return val_exit_acs();
}
