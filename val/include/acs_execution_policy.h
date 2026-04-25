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

#ifndef __ACS_EXECUTION_POLICY_H__
#define __ACS_EXECUTION_POLICY_H__

#include "acs_stdint.h"
#include "acs_stdbool.h"

/*
 * acs_execution_policy_t captures shared runtime behavior knobs for one ACS
 * invocation. It contains only "how to run" inputs gathered from platform
 * defaults, build overrides, CLI parsing, or EL3-provided parameters:
 * - print verbosity and MMIO-print enablement
 * - PCIe/CXL behavior hints
 * - wakeup/watchdog/timer timeout controls
 * - crypto-extension and EL1 trap workarounds
 * - system last-level cache hinting
 */
typedef struct acs_execution_policy {
    uint32_t pcie_p2p;
    uint32_t pcie_cache_present;
    bool     pcie_skip_dp_nic_ms;
    uint32_t print_level;
    uint32_t print_mmio;
    uint32_t timeout_pass;
    uint32_t timeout_fail;
    uint32_t timer_timeout_us;
    uint32_t crypto_support;
    /*
     * System last-level cache hint used by MPAM and related tests:
     *   0 - Unknown
     *   1 - PPTT PE-side LLC
     *   2 - HMAT mem-side LLC
     */
    uint32_t sys_last_lvl_cache;
    /*
     * Bitmask of EL1 register accesses to skip when a platform traps or does
     * not safely expose them. Compose with EL1SKIPTRAP_* flags.
     */
    uint32_t el1skiptrap_mask;
} acs_execution_policy_t;

void acs_reset_execution_policy(void);
acs_execution_policy_t *acs_get_execution_policy_mut(void);
const acs_execution_policy_t *acs_get_execution_policy(void);
uint32_t acs_policy_get_print_level(void);
uint32_t acs_policy_get_print_mmio(void);
uint32_t acs_policy_get_pcie_p2p(void);
uint32_t acs_policy_get_pcie_cache_present(void);
bool acs_policy_get_pcie_skip_dp_nic_ms(void);
uint32_t acs_policy_get_timeout_pass(void);
uint32_t acs_policy_get_timeout_fail(void);
uint32_t acs_policy_get_timer_timeout_us(void);
uint32_t acs_policy_get_crypto_support(void);
uint32_t acs_policy_get_sys_last_lvl_cache(void);
uint32_t acs_policy_get_el1skiptrap_mask(void);

#endif /* __ACS_EXECUTION_POLICY_H__ */
