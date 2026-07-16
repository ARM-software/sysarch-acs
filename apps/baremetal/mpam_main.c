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

/*
 * Build-time selection arrays for the MPAM bare-metal (non rule-based) path.
 */
extern uint32_t g_skip_array[];
extern const uint32_t g_skip_array_len;
extern uint32_t g_test_array[];
extern const uint32_t g_test_array_len;
extern uint32_t g_module_array[];
extern const uint32_t g_module_array_len;

/* MPAM bare-metal globals local. */
uint32_t *g_skip_test_num;
uint32_t  g_num_skip;
uint32_t *g_execute_tests;
uint32_t  g_num_tests;
uint32_t *g_execute_modules;
uint32_t  g_num_modules;
uint32_t  g_execute_secure = 0;

static void
free_mpam_mem(void)
{
    val_free_shared_mem();
    val_mpam_free_info_table();
    val_pcc_free_info_table();

    if (acs_is_module_enabled(ACS_MPAM_MEMORY_TEST_NUM_BASE)) {
        val_srat_free_info_table();
        val_hmat_free_info_table();
    }

    if (acs_is_module_enabled(ACS_MPAM_CACHE_TEST_NUM_BASE) ||
        acs_is_module_enabled(ACS_MPAM_MONITOR_TEST_NUM_BASE))
        val_cache_free_info_table();

    if (acs_is_module_enabled(ACS_MPAM_ERROR_TEST_NUM_BASE) ||
        acs_is_module_enabled(ACS_MPAM_CACHE_TEST_NUM_BASE) ||
        acs_is_module_enabled(ACS_MPAM_MONITOR_TEST_NUM_BASE) ||
        acs_is_module_enabled(ACS_MPAM_MEMORY_TEST_NUM_BASE)) {
        val_iovirt_free_info_table();
        val_gic_free_info_table();
    }

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
    ctx->arch_selection = ARCH_NONE;

    /* Use build-time selection arrays (legacy non-rulebase mechanism). */
    g_skip_test_num    = g_skip_array;
    g_num_skip         = g_skip_array_len;
    g_execute_tests    = g_test_array;
    g_num_tests        = g_test_array_len;
    g_execute_modules  = g_module_array;
    g_num_modules      = g_module_array_len;
    g_acs_tests_total = 0;
    g_acs_tests_pass = 0;
    g_acs_tests_fail = 0;
    g_execute_secure = 0;

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
    val_print(INFO, " Built for target: %s\n", ACS_TARGET);
    val_print(INFO, "(Print level is %2d)\n\n", acs_policy_get_print_level());

#if ACS_ENABLE_MMU
    val_print(INFO, " Enabling MMU\n");

    if (val_setup_mmu()) {
        status = ACS_STATUS_FAIL;
        goto exit_acs;
    }

    if (val_enable_mmu()) {
        status = ACS_STATUS_FAIL;
        goto exit_acs;
    }
#else
    val_print(INFO, "Skipping MMU setup/enable (ACS_ENABLE_MMU=0)\n");
#endif

    if (!acs_is_module_enabled(ACS_MPAM_REGISTER_TEST_NUM_BASE) &&
        !acs_is_module_enabled(ACS_MPAM_CACHE_TEST_NUM_BASE) &&
        !acs_is_module_enabled(ACS_MPAM_MONITOR_TEST_NUM_BASE) &&
        !acs_is_module_enabled(ACS_MPAM_ERROR_TEST_NUM_BASE) &&
        !acs_is_module_enabled(ACS_MPAM_MEMORY_TEST_NUM_BASE)) {
        val_print(INFO, "\n      No MPAM modules selected. Skipping table init/tests.\n");
        goto print_test_status;
    }

    val_print(INFO, " Creating Platform Information Tables\n");

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

    if (acs_is_module_enabled(ACS_MPAM_ERROR_TEST_NUM_BASE) ||
        acs_is_module_enabled(ACS_MPAM_CACHE_TEST_NUM_BASE) ||
        acs_is_module_enabled(ACS_MPAM_MONITOR_TEST_NUM_BASE) ||
        acs_is_module_enabled(ACS_MPAM_MEMORY_TEST_NUM_BASE))
        createIoVirtInfoTable();

    if (acs_is_module_enabled(ACS_MPAM_CACHE_TEST_NUM_BASE) ||
        acs_is_module_enabled(ACS_MPAM_MONITOR_TEST_NUM_BASE))
        createCacheInfoTable();

    createPccInfoTable();

    if (acs_is_module_enabled(ACS_MPAM_MEMORY_TEST_NUM_BASE)) {
        createHmatInfoTable();
        createSratInfoTable();
    }
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

    status |= val_mpam_execute_register_tests();
    status |= val_mpam_execute_cache_tests();
    status |= val_mpam_execute_error_tests();
    status |= val_mpam_execute_membw_tests();

print_test_status:
    val_print(ERROR, "\n     ------------------------------------------------------- \n");
    val_print(ERROR, "     Total Tests run  = %4d;", g_acs_tests_total);
    val_print(ERROR, "  Tests Passed  = %4d", g_acs_tests_pass);
    val_print(ERROR, "  Tests Failed = %4d\n", g_acs_tests_fail);
    val_print(ERROR, "     --------------------------------------------------------- \n");

    val_print(INFO, "\n      *** MPAM tests complete. Reset the system. *** \n\n");

exit_acs:
    free_mpam_mem();
    acs_release_run_request(ctx);
    if (context_saved)
        val_pe_context_restore(AA64WriteSp(g_stack_pointer));
    return val_exit_acs();
}
