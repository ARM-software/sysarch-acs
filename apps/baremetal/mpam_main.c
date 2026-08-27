/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/val_interface.h"
#include "val/include/pal_interface.h"
#include "val/include/acs_val.h"
#include "val/include/acs_pe.h"
#include "val/include/acs_mpam.h"
#include "val/include/acs_memory.h"
#include "acs.h"

static void
free_mpam_mem(void)
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

static uint32_t
apply_mpam_defaults(acs_run_request_t *ctx, acs_execution_policy_t *policy)
{
    if (ctx == NULL || policy == NULL)
        return ACS_STATUS_FAIL;

    acs_load_run_request_defaults(ctx);
    acs_load_execution_policy_defaults(policy);

    policy->print_level = PLATFORM_OVERRIDE_PRINT_LEVEL;
    policy->print_mmio = 0;

    if (ctx->rule_count == 0)
        ctx->arch_selection = ARCH_SYS_MPAM;

    ctx->level_value = MPAM_VERSION_B_c;
    if (ctx->level_filter_mode == LVL_FILTER_NONE)
        ctx->level_filter_mode = LVL_FILTER_MAX;

    if (ctx->level_value >= MPAM_VERSION_SENTINEL) {
        val_print(ERROR, "\nInvalid MPAM version value (%d)", ctx->level_value);
        return ACS_STATUS_FAIL;
    }

    g_acs_tests_total = 0;
    g_acs_tests_pass = 0;
    g_acs_tests_fail = 0;

    return ACS_STATUS_PASS;
}

int32_t
ShellAppMainmpam(void)
{
    uint32_t status = ACS_STATUS_PASS;
    uint32_t msc_node_cnt;
    void *branch_label;
    uint32_t context_saved = 0;
    acs_run_request_t *ctx;
    acs_execution_policy_t *policy;

    acs_reset_run_request();
    ctx = acs_get_run_request_mut();
    policy = acs_get_execution_policy_mut();

    status = apply_mpam_defaults(ctx, policy);
    if (status != ACS_STATUS_PASS) {
        val_print(ERROR, "\napply_mpam_defaults() failed, Exiting...\n");
        goto exit_acs;
    }

    acs_apply_compile_params(ctx, policy);
    acs_apply_el3_params(ctx, policy);

    val_print(INFO, "\n\n MPAM Architecture Compliance Suite\n");
    val_print(INFO, "    Version %d.", MPAM_ACS_MAJOR_VER);
    val_print(INFO, "%d.", MPAM_ACS_MINOR_VER);
    val_print(INFO, "%d\n", MPAM_ACS_SUBMINOR_VER);
    val_print(INFO, "\nBuilt for target: %s", ACS_TARGET);
    val_print(INFO, "(Print level is %2d)\n\n", acs_policy_get_print_level());

#if ACS_ENABLE_MMU
    val_print(INFO, "\nEnabling MMU");

    if (val_setup_mmu()) {
        status = ACS_STATUS_FAIL;
        goto exit_acs;
    }

    if (val_enable_mmu()) {
        status = ACS_STATUS_FAIL;
        goto exit_acs;
    }
#else
    val_print(INFO, "\nSkipping MMU setup/enable (ACS_ENABLE_MMU=0)");
#endif

    val_print(INFO, "\nCreating Platform Information Tables");

    status = createPeInfoTable();
    if (status)
        goto exit_acs;

    if (val_pe_feat_check(PE_FEAT_MPAM)) {
        val_print(INFO,
                  "\n       PE MPAM extension unimplemented. Skipping all MPAM tests\n");
        goto print_test_status;
    }

    status = createGicInfoTable();
    if (status)
        goto exit_acs;

    createIoVirtInfoTable();

    createCacheInfoTable();

    createPccInfoTable();

    createHmatInfoTable();

    createSratInfoTable();
    createMpamInfoTable();
    val_mpam_update_msc_device_names();

    msc_node_cnt = val_mpam_get_msc_count();
    if (msc_node_cnt == 0) {
        val_print(INFO, "\n      *** Exiting suite - No MPAM nodes *** \n");
        goto print_test_status;
    }

    val_allocate_shared_mem();

    branch_label = &&print_test_status;
    val_pe_context_save(AA64ReadSp(), (uint64_t)branch_label);
    context_saved = 1;
    val_pe_initialize_default_exception_handler(val_pe_default_esr);

    if ((ctx->rule_count > 0 && ctx->rule_list != NULL) ||
        (ctx->arch_selection != ARCH_NONE)) {
        filter_rule_list_by_cli(ctx);
        if (ctx->rule_count == 0 || ctx->rule_list == NULL) {
            val_print(ERROR, "\nRule list empty, nothing to execute.\n");
            status = ACS_STATUS_FAIL;
            goto print_test_status;
        }

        run_tests(ctx);
    } else {
        val_print(ERROR, "\nInvalid rule list or architecture selected.\n");
        status = ACS_STATUS_FAIL;
    }

print_test_status:
    val_print_acs_test_status_summary();

    val_print(INFO, "\n      *** MPAM tests complete. Reset the system. *** \n\n");

exit_acs:
    free_mpam_mem();
    acs_release_run_request(ctx);
    if (context_saved)
        val_pe_context_restore(AA64WriteSp(g_stack_pointer));
    return val_exit_acs();
}
