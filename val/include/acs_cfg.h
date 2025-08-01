/** @file
 * Copyright (c) 2016-2018, 2022-2025, Arm Limited or its affiliates. All rights reserved.
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

#ifndef __ACS_CFG_H__
#define __ACS_CFG_H__

extern uint32_t g_print_level;
extern uint32_t g_execute_secure;
extern uint32_t *g_skip_test_num;
extern uint32_t g_num_skip;
extern uint32_t g_acs_tests_total;
extern uint32_t g_acs_tests_pass;
extern uint32_t g_acs_tests_fail;
extern uint32_t g_bsa_level;
extern uint32_t g_bsa_only_level;
extern uint32_t g_sbsa_level;
extern uint32_t g_sbsa_only_level;
extern uint32_t g_pcbsa_level;
extern uint32_t g_pcbsa_only_level;
extern uint64_t g_stack_pointer;
extern uint64_t g_exception_ret_addr;
extern uint64_t g_ret_addr;
extern uint32_t *g_execute_tests;
extern uint32_t g_num_tests;
extern uint32_t *g_execute_modules;
extern uint32_t g_num_modules;
extern uint32_t g_build_sbsa;
extern uint32_t g_build_pcbsa;
extern uint32_t g_curr_module;
extern uint32_t g_el1physkip;
extern uint32_t g_sys_last_lvl_cache;
extern uint32_t g_crypto_support;
extern uint32_t g_drtm_acs_dlme[];
extern uint64_t g_drtm_acs_dlme_size;


#endif
