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

#include "val/include/pal_interface.h"
#include "val/include/val_interface.h"
#include "val/include/rule_based_execution.h"
#include "acs.h"

/* Global Variables */
bool      g_pcie_skip_dp_nic_ms = 0;
uint32_t  g_print_level;
uint32_t  g_print_mmio;
uint32_t  g_curr_module;
uint32_t  g_enable_module;
uint32_t  g_acs_tests_total;
uint32_t  g_acs_tests_pass;
uint32_t  g_acs_tests_fail;
uint32_t  g_wakeup_timeout;
uint32_t  g_build_sbsa = 0;
uint32_t  g_build_pcbsa = 0;
uint32_t  g_its_init = 0;
uint64_t  g_stack_pointer;
uint64_t  g_exception_ret_addr;
uint64_t  g_ret_addr;

/* Rule-based execution: arch and filtering selections */
RULE_ID_e *g_rule_list = NULL;
RULE_ID_e *g_skip_rule_list = NULL;
uint32_t  *g_execute_modules = NULL;
uint32_t  *g_skip_modules = NULL;
uint32_t g_arch_selection    = ARCH_NONE;
uint32_t g_level_value  = 0;
uint32_t g_bsa_sw_view_mask  = 0;

/*
 * Global counters for rule/test outcomes.
 * Updated in val/src/rule_based_execution_helpers.c::print_rule_test_status().
 */
acs_test_status_counters_t g_rule_test_stats = {0};

/* ***Note***: few globals are defined in pal/baremetal/target/../src/platform_cfg_fvp.c
   for partners to furnish */