/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
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

#include "acs.h"
#include <stdint.h>
#include <stdbool.h>
#include "val/include/val_interface.h"
#include "val/include/acs_el3_param.h"

extern uint64_t  g_el3_param_magic;
extern uint64_t  g_el3_param_addr;
extern uint32_t *g_execute_tests;
extern RULE_ID_e  *g_rule_tests;
extern uint32_t  g_num_tests;
extern uint32_t *g_execute_modules;
extern uint32_t  g_num_modules;
extern uint32_t  g_rule_tests_num;

/* === Build-time module list support (ACS_ENABLED_MODULE_LIST) === */
#if ACS_HAS_ENABLED_MODULE_LIST
uint32_t acs_build_module_array[] = { ACS_ENABLED_MODULE_LIST };
const uint32_t acs_build_module_count =
    sizeof(acs_build_module_array) / sizeof(acs_build_module_array[0]);
#endif

bool
acs_is_module_enabled(uint32_t module_base)
{
    /* Runtime / EL3 / CLI override has highest priority */
    if (g_num_modules) {
        return acs_list_contains(g_execute_modules, g_num_modules, module_base);
    }

#if ACS_HAS_ENABLED_MODULE_LIST
    /* Build-time default list */
    return acs_list_contains(acs_build_module_array,
                             acs_build_module_count,
                             module_base);
#else
    /* No overrides: enable everything */
    (void)module_base;
    return true;
#endif
}

bool
acs_list_contains(const uint32_t *list, uint32_t count, uint32_t value)
{
  uint32_t i;

  if (list == NULL || count == 0)
      return false;

  for (i = 0; i < count; i++) {
      if (list[i] == value)
          return true;
  }

  return false;
}

void
acs_apply_el3_params(void)
{
  acs_el3_params *params;

  /* If magic doesn't match, ignore X20 completely */
  if (g_el3_param_magic != ACS_EL3_PARAM_MAGIC)
    return;

  if (!g_el3_param_addr) {
    val_print(ACS_PRINT_WARN,
              "EL3 param magic set but param address is 0, ignoring\n", 0);
    return;
  }

  params = (acs_el3_params *)(uintptr_t)g_el3_param_addr;

  /* Optional: version check (kept minimal, versioned for future proofing) */
  if (params->version != ACS_EL3_PARAM_VERSION) {
    val_print(ACS_PRINT_WARN,
              "Unsupported EL3 param version %ld, ignoring\n", params->version);
    return;
  }

  val_print(ACS_PRINT_DEBUG, "EL3 params: tests=0x%lx", params->test_array_addr);
  val_print(ACS_PRINT_DEBUG, " (%ld),", params->test_array_count);
  val_print(ACS_PRINT_DEBUG, " modules=0x%lx", params->module_array_addr);
  val_print(ACS_PRINT_DEBUG, " (%ld)\n", params->module_array_count);

  /* Override tests if provided */
  if (params->test_array_addr && params->test_array_count) {
    g_rule_list  = (uint32_t *)(uintptr_t)params->test_array_addr;
    g_rule_count  = (uint32_t)params->test_array_count;
    g_arch_selection = ARCH_NONE;
  }

  /* Override modules if provided */
  if (params->module_array_addr && params->module_array_count) {
    g_execute_modules = (uint32_t *)(uintptr_t)params->module_array_addr;
    g_num_modules     = (uint32_t)params->module_array_count;
  }
}

void
acs_apply_compile_params(void)
{
#if ACS_HAS_ENABLED_MODULE_LIST
  g_execute_modules = acs_build_module_array;
  g_num_modules = acs_build_module_count;
#endif
  return;
}