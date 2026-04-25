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

#include "acs_execution_policy.h"
#include "val_interface.h"
#include "val_logger.h"

static acs_execution_policy_t g_execution_policy;

void acs_reset_execution_policy(void)
{
    g_execution_policy = (acs_execution_policy_t) {
        .print_level = INFO,
        .timeout_pass = WAKEUP_WD_PASS_TIMEOUT_DEFAULT,
        .timeout_fail = WAKEUP_WD_PASS_TIMEOUT_DEFAULT *
                        WAKEUP_WD_FAILSAFE_TIMEOUT_MULTIPLIER,
        .timer_timeout_us = TIMER_TIMEOUT_DEFAULT,
        .crypto_support = 1u,
    };
}

acs_execution_policy_t *acs_get_execution_policy_mut(void)
{
    return &g_execution_policy;
}

const acs_execution_policy_t *acs_get_execution_policy(void)
{
    return &g_execution_policy;
}

uint32_t acs_policy_get_print_level(void)
{
    return g_execution_policy.print_level;
}

uint32_t acs_policy_get_print_mmio(void)
{
    return g_execution_policy.print_mmio;
}

uint32_t acs_policy_get_pcie_p2p(void)
{
    return g_execution_policy.pcie_p2p;
}

uint32_t acs_policy_get_pcie_cache_present(void)
{
    return g_execution_policy.pcie_cache_present;
}

bool acs_policy_get_pcie_skip_dp_nic_ms(void)
{
    return g_execution_policy.pcie_skip_dp_nic_ms;
}

uint32_t acs_policy_get_timeout_pass(void)
{
    return g_execution_policy.timeout_pass;
}

uint32_t acs_policy_get_timeout_fail(void)
{
    return g_execution_policy.timeout_fail;
}

uint32_t acs_policy_get_timer_timeout_us(void)
{
    return g_execution_policy.timer_timeout_us;
}

uint32_t acs_policy_get_crypto_support(void)
{
    return g_execution_policy.crypto_support;
}

uint32_t acs_policy_get_sys_last_lvl_cache(void)
{
    return g_execution_policy.sys_last_lvl_cache;
}

uint32_t acs_policy_get_el1skiptrap_mask(void)
{
    return g_execution_policy.el1skiptrap_mask;
}
