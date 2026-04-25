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
#include "pal/baremetal/base/include/pal_execution_policy.h"
#include "val/include/val_interface.h"
#include "val/include/acs_el3_param.h"
#include "val/include/rule_based_execution_enum.h"

extern uint64_t  g_el3_param_magic;
extern uint64_t  g_el3_param_addr;

/* === Build-time module list support (ACS_ENABLED_MODULE_LIST) === */
#if ACS_HAS_ENABLED_MODULE_LIST
uint32_t acs_build_module_array[] = { ACS_ENABLED_MODULE_LIST };
const uint32_t acs_build_module_count =
    sizeof(acs_build_module_array) / sizeof(acs_build_module_array[0]);
#endif

bool
acs_is_module_enabled(uint32_t module_base)
{
    const acs_run_request_t *ctx = acs_get_run_request();

    /* Runtime / EL3 / CLI override has highest priority */
    if (ctx->num_modules) {
        return acs_list_contains(ctx->execute_modules, ctx->num_modules, module_base);
    }
    /* No overrides: enable everything */
    (void)module_base;
    return true;
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
acs_load_execution_policy_defaults(acs_execution_policy_t *policy)
{
  const acs_execution_policy_t *defaults;
  const acs_execution_policy_t *platform_defaults;

  if (policy == NULL)
      return;

  acs_reset_execution_policy();
  defaults = acs_get_execution_policy();
  /*
   * Baremetal entry paths currently pass the shared execution-policy singleton,
   * so acs_reset_execution_policy() has already seeded `policy` with the base
   * defaults. Keep support for detached policy objects without needlessly
   * self-assigning the singleton fields here.
   */
  if (policy != defaults) {
      policy->pcie_p2p = defaults->pcie_p2p;
      policy->pcie_cache_present = defaults->pcie_cache_present;
      policy->pcie_skip_dp_nic_ms = defaults->pcie_skip_dp_nic_ms;
      policy->print_level = defaults->print_level;
      policy->print_mmio = defaults->print_mmio;
      policy->timeout_pass = defaults->timeout_pass;
      policy->timeout_fail = defaults->timeout_fail;
      policy->timer_timeout_us = defaults->timer_timeout_us;
      policy->crypto_support = defaults->crypto_support;
      policy->sys_last_lvl_cache = defaults->sys_last_lvl_cache;
      policy->el1skiptrap_mask = defaults->el1skiptrap_mask;
  }

  platform_defaults = acs_get_platform_execution_policy_defaults();
  if (platform_defaults == NULL)
      return;

  /* Overlay baremetal platform-specific defaults on top of the generic reset defaults. */
  policy->pcie_p2p = platform_defaults->pcie_p2p;
  policy->pcie_cache_present = platform_defaults->pcie_cache_present;
  policy->pcie_skip_dp_nic_ms = platform_defaults->pcie_skip_dp_nic_ms;
  policy->crypto_support = platform_defaults->crypto_support;
  policy->sys_last_lvl_cache = platform_defaults->sys_last_lvl_cache;
  policy->el1skiptrap_mask = platform_defaults->el1skiptrap_mask;

  if (platform_defaults->timeout_pass != 0u)
      policy->timeout_pass = platform_defaults->timeout_pass;
  if (platform_defaults->timeout_fail != 0u)
      policy->timeout_fail = platform_defaults->timeout_fail;
  if (platform_defaults->timer_timeout_us != 0u)
      policy->timer_timeout_us = platform_defaults->timer_timeout_us;
}

void
acs_load_run_request_defaults(acs_run_request_t *ctx)
{
  if (ctx == NULL)
      return;

  ctx->rule_list = NULL;
  ctx->rule_count = 0;
  ctx->skip_rule_list = NULL;
  ctx->skip_rule_count = 0;
  ctx->execute_modules = NULL;
  ctx->num_modules = 0;
  ctx->skip_modules = NULL;
  ctx->num_skip_modules = 0;
  ctx->arch_selection = ARCH_NONE;
  ctx->level_filter_mode = g_level_filter_mode;
  ctx->level_value = 0;
  ctx->bsa_sw_view_mask = 0;
  ctx->rule_list_owned = false;
  ctx->skip_rule_list_owned = false;
  ctx->execute_modules_owned = false;
  ctx->skip_modules_owned = false;

  if (g_rule_count) {
      ctx->rule_list = g_rule_list_arr;
      ctx->rule_count = g_rule_count;
  }
  if (g_skip_rule_count) {
      ctx->skip_rule_list = g_skip_rule_list_arr;
      ctx->skip_rule_count = g_skip_rule_count;
  }
  if (g_num_modules) {
      ctx->execute_modules = g_execute_modules_arr;
      ctx->num_modules = g_num_modules;
  }
  if (g_num_skip_modules) {
      ctx->skip_modules = g_skip_modules_arr;
      ctx->num_skip_modules = g_num_skip_modules;
  }
}

void
acs_apply_el3_params(acs_run_request_t *ctx, acs_execution_policy_t *policy)
{
  acs_el3_params *params;

  if (ctx == NULL || policy == NULL)
    return;

  /* If magic doesn't match, ignore X20 completely */
  if (g_el3_param_magic != ACS_EL3_PARAM_MAGIC)
    return;

  if (!g_el3_param_addr) {
    val_print(WARN,
              "EL3 param magic set but param address is 0, ignoring\n");
    return;
  }

  params = (acs_el3_params *)(uintptr_t)g_el3_param_addr;

  /* Optional: version check (kept minimal, versioned for future proofing) */
  if ((params->version < 0x1) || (params->version > ACS_EL3_PARAM_VERSION)) {
    val_print(WARN,
              "Unsupported EL3 param version %ld, ignoring\n", params->version);
    return;
  }

  val_print(DEBUG, "EL3 params: tests=0x%lx", params->rule_array_addr);
  val_print(DEBUG, " (%ld),", params->rule_array_count);
  val_print(DEBUG, " modules=0x%lx", params->module_array_addr);
  val_print(DEBUG, " (%ld)\n", params->module_array_count);

  /* Override tests if provided */
  if (params->rule_array_addr && params->rule_array_count) {
    ctx->rule_list = (RULE_ID_e *)(uintptr_t)params->rule_array_addr;
    ctx->rule_count = (uint32_t)params->rule_array_count;
    ctx->rule_list_owned = false;
    ctx->arch_selection = ARCH_NONE;
  }

  /* Override modules if provided */
  if (params->module_array_addr && params->module_array_count) {
    ctx->execute_modules = (uint32_t *)(uintptr_t)params->module_array_addr;
    ctx->num_modules = (uint32_t)params->module_array_count;
    ctx->execute_modules_owned = false;
  }

  /* Override skip list if provided */
  if ((params->version >= 0x2) && params->skip_rule_array_addr
     && params->skip_rule_array_count)
  {
    ctx->skip_rule_list = (RULE_ID_e *)(uintptr_t)params->skip_rule_array_addr;
    ctx->skip_rule_count = (uint32_t)params->skip_rule_array_count;
    ctx->skip_rule_list_owned = false;
  }

  if ((params->version >= 0x3))
  {
    if (params->skip_module_array_addr && params->skip_module_array_count) {
      ctx->skip_modules = (uint32_t *)(uintptr_t)params->skip_module_array_addr;
      ctx->num_skip_modules = (uint32_t)params->skip_module_array_count;
      ctx->skip_modules_owned = false;
    }

    /* Override shared runtime knobs and context filter settings from EL3 parameters. */
    policy->pcie_p2p            = params->p2p;
    policy->pcie_skip_dp_nic_ms = params->skip_dp_nic_ms;
    policy->print_mmio          = params->mmio;
    policy->crypto_support      = params->no_crypto_ext ? 0u : 1u;
    policy->el1skiptrap_mask    = params->el1skiptrap_mask;
    ctx->bsa_sw_view_mask = params->software_view_filter;
    policy->pcie_cache_present  = params->cache;
    policy->sys_last_lvl_cache  = params->sys_cache;
    ctx->level_value = params->level;

    if (params->level_selection >= LVL_FILTER_NONE &&
       params->level_selection <= LVL_FILTER_FR)
      ctx->level_filter_mode = params->level_selection;
    else
      val_print(WARN,
                "Override skipped for level filter mode  %d\n", params->level_selection);

    if (params->verbose >= TRACE && params->verbose <= FATAL)
      policy->print_level = params->verbose;
    else
      val_print(WARN,
                "Override skipped for verbose  %d\n", params->verbose);

    if (params->timeout >= TIMEOUT_THRESHOLD
       && params->timeout <= TIMEOUT_MAX_THRESHOLD)
    {
      policy->timeout_pass = params->timeout;
      policy->timer_timeout_us = params->timeout;
      policy->timeout_fail =
          policy->timeout_pass * WAKEUP_WD_FAILSAFE_TIMEOUT_MULTIPLIER;
    }
    else
      val_print(WARN,
                "Override skipped for timeout  %d\n", params->timeout);
  }
}

void
acs_apply_compile_params(acs_run_request_t *ctx, acs_execution_policy_t *policy)
{
  if (ctx == NULL || policy == NULL)
      return;

#if ACS_HAS_ENABLED_MODULE_LIST
  ctx->execute_modules = acs_build_module_array;
  ctx->num_modules = acs_build_module_count;
  ctx->execute_modules_owned = false;
#endif

#ifdef ACS_VERBOSE_LEVEL
  /*
   * Allow compile-time override of the default print verbosity.
   */
  policy->print_level = ACS_VERBOSE_LEVEL;

  if (policy->print_level < TRACE)
    policy->print_level = TRACE;
  else if (policy->print_level > FATAL)
    policy->print_level = FATAL;
#endif

  return;
}
