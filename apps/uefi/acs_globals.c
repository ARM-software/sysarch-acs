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

#include <Uefi.h>
#include <stdbool.h>
#include <Library/ShellLib.h>
#include "val/include/val_interface.h"
#include "val/include/rule_based_execution.h"
#include "acs.h"

/* Global Variables */
UINT32  g_pcie_p2p;
UINT32  g_pcie_cache_present;
bool    g_pcie_skip_dp_nic_ms = 0;
UINT32  g_print_level;
UINT32  *g_skip_test_num;
UINT32  g_num_skip;
UINT64  g_stack_pointer;
UINT64  g_exception_ret_addr;
UINT64  g_ret_addr;
UINT32  g_wakeup_timeout;

/* Following g_build_* is retained to have compatibility with tests which use it, and used pass
   on -a selection hint to tests */
UINT32  g_build_sbsa = 0;
UINT32  g_build_pcbsa = 0;

UINT32  g_print_mmio;
UINT32  g_curr_module;
UINT32  g_enable_module;
UINT32  g_crypto_support = TRUE;
UINT32  *g_execute_modules;
UINT32  g_num_modules = 0;
UINT32  *g_skip_modules;
UINT32  g_num_skip_modules = 0;
UINT32  g_sys_last_lvl_cache;

/* VE systems run acs at EL1 and in some systems crash is observed during access
   of EL1 phy and virt timer, Below command line option is added only for debug
   purpose to complete BSA run on these systems */
UINT32  g_el1physkip = FALSE;

/* File handles */
SHELL_FILE_HANDLE g_acs_log_file_handle;
SHELL_FILE_HANDLE g_dtb_log_file_handle;

/* Storage for parsed rule IDs from -r */
RULE_ID_e *g_rule_list = NULL;
UINT32     g_rule_count = 0;

/* Storage for parsed skip rule IDs from -skip */
RULE_ID_e *g_skip_rule_list = NULL;
UINT32     g_skip_rule_count = 0;

/* Rule-based execution: arch and filtering selections (-a, -l/-only/-fr, -hyp/-os/-ps) */
uint32_t g_arch_selection    = ARCH_NONE;
uint32_t g_level_filter_mode = LVL_FILTER_NONE;
uint32_t g_level_value       = 0;
uint32_t g_bsa_sw_view_mask  = 0;

/* Set when invalid rule/module token encountered during CLI parsing */
BOOLEAN g_invalid_arg_seen = FALSE;

/*
 * Global counters for rule/test outcomes.
 * Updated in val/src/rule_based_execution_helpers.c::print_rule_test_status().
 */
acs_test_status_counters_t g_rule_test_stats = {0};
