/** @file
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
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

#ifndef __BSA_AVS_LEVEL_H__
#define __BSA_AVS_LEVEL_H__

#include "platform_image_def.h"
#include "platform_override_fvp.h"

#define SIZE_4K 0x1000

#define BSA_ACS_MAJOR_VER      1
#define BSA_ACS_MINOR_VER      2
#define BSA_ACS_SUBMINOR_VER   0

#define SBSA_ACS_MAJOR_VER       8
#define SBSA_ACS_MINOR_VER       0
#define SBSA_ACS_SUBMINOR_VER    0
#define SBSA_FR_LEVEL            0x8

#define PC_BSA_ACS_MAJOR_VER     1
#define PC_BSA_ACS_MINOR_VER     0
#define PC_BSA_ACS_SUBMINOR_VER  0

#define INVALID_MPIDR     0xffffffff

#define STACK_SIZE          0x1000

#define PAGE_SIZE_4K        0x1000
#define PAGE_SIZE_16K       (4 * 0x1000)
#define PAGE_SIZE_64K       (16 * 0x1000)

#define PLATFORM_CPU_COUNT PLATFORM_OVERRIDE_PE_CNT

#if (PLATFORM_PAGE_SIZE == PAGE_SIZE_4K)
 #define PAGE_SIZE           PAGE_SIZE_4K
#elif (PLATFORM_PAGE_SIZE == PAGE_SIZE_16K)
 #define PAGE_SIZE           PAGE_SIZE_16K
#elif (PLATFORM_PAGE_SIZE == PAGE_SIZE_64K)
 #define PAGE_SIZE           PAGE_SIZE_64K
#endif

#define BSA_MIN_LEVEL_SUPPORTED 1
#define BSA_MAX_LEVEL_SUPPORTED 1

#define G_SBSA_LEVEL             4
#define SBSA_MIN_LEVEL_SUPPORTED 3
#define SBSA_MAX_LEVEL_SUPPORTED 8

/*******************************************************************************
 * Used to align variables on the biggest cache line size in the platform.
 * This is known only to the platform as it might have a combination of
 * integrated and external caches.
 ******************************************************************************/
#define CACHE_WRITEBACK_SHIFT     6
#define CACHE_WRITEBACK_GRANULE   (1 << CACHE_WRITEBACK_SHIFT)

#ifdef _AARCH64_BUILD_
  unsigned long __stack_chk_guard = 0xBAAAAAAD;
  unsigned long __stack_chk_fail =  0xBAAFAAAD;
#endif

#define SCTLR_I_BIT         (1 << 12)
#define SCTLR_M_BIT         (1 << 0)
#define DISABLE_MMU_BIT     (0xFFFFFFFFFFFFFFFE)

/* This .h file is included by .S assembly files, and assembler will choke on the C code, hence
   gating C code when included by .S files */
#ifndef __ASSEMBLER__
#include "val/include/rule_based_execution.h"

#define LEVEL_PRINT_FORMAT(level, filter_mode, fr_level) ((filter_mode == LVL_FILTER_FR) ? \
    ((filter_mode == LVL_FILTER_ONLY && level == fr_level) ? \
    "\n Starting tests for only level FR " : "\n Starting tests for level FR ") : \
    ((filter_mode == LVL_FILTER_ONLY) ? \
    "\n Starting tests for only level %2d " : "\n Starting tests for level %2d "))

/* Extern declarations for globals from platform_cfg_fvp.c */
extern RULE_ID_e g_rule_list_arr[];
extern uint32_t  g_rule_count;
extern RULE_ID_e g_skip_rule_list_arr[];
extern uint32_t  g_skip_rule_count;
extern uint32_t g_execute_modules_arr[];
extern uint32_t g_num_modules;
extern uint32_t g_skip_modules_arr[];
extern uint32_t g_num_skip_modules;
extern uint32_t g_level_filter_mode;
extern uint32_t g_sys_last_lvl_cache;

/* Globals from apps/baremetal/acs_globals.c */
extern RULE_ID_e *g_rule_tests;
extern uint32_t  g_rule_tests_num;
extern RULE_ID_e *g_rule_list;
extern RULE_ID_e *g_skip_rule_list;
extern uint32_t  *g_execute_modules;
extern uint32_t  *g_skip_modules;
extern uint32_t  g_print_level;
extern uint32_t  g_print_mmio;
extern uint32_t  g_curr_module;
extern uint32_t  g_enable_module;
extern uint32_t  g_acs_tests_total;
extern uint32_t  g_acs_tests_pass;
extern uint32_t  g_acs_tests_fail;
extern uint64_t  g_stack_pointer;
extern uint64_t  g_exception_ret_addr;
extern uint64_t  g_ret_addr;
extern uint32_t  g_wakeup_timeout;
extern bool      g_pcie_skip_dp_nic_ms;
extern uint32_t  g_build_sbsa;
extern uint32_t  g_build_pcbsa;
extern uint32_t  g_its_init;
extern uint32_t  g_arch_selection;
extern uint32_t  g_level_value;
extern uint32_t  g_bsa_sw_view_mask;

/* Function declarations */
uint32_t createPeInfoTable(void);
uint32_t createGicInfoTable(void);

void     createMemoryInfoTable(void);
void     createPcieInfoTable(void);
void     createIoVirtInfoTable(void);
void     createTimerInfoTable(void);
void     createWatchdogInfoTable(void);
void     createPeripheralInfoTable(void);
void     createDmaInfoTable(void);
void     createSmbiosInfoTable(void);
void     createPmuInfoTable(void);
void     createRasInfoTable(void);
void     createCacheInfoTable(void);
void     createMpamInfoTable(void);
void     createHmatInfoTable(void);
void     createSratInfoTable(void);
void     createPccInfoTable(void);
void     createRas2InfoTable(void);
void     createTpm2InfoTable(void);

#endif /* __ASSEMBLER__ */
/*
 * MMU configuration
 *
 * 1 = do val_setup_mmu/val_enable_mmu
 * 0 = skip MMU setup/enable
 *
 * Can be overridden from the build system, e.g.:
 *   -DACS_ENABLE_MMU=0
 */
#ifndef ACS_ENABLE_MMU
#define ACS_ENABLE_MMU   1
#endif

/*
 * Optional compile-time default enabled module list.
 *
 * If the build system defines, for example:
 *
 *   -DACS_ENABLED_MODULE_LIST=TIMER,PCIE
 *
 * then ACS will treat that as a static uint32_t[] containing the module
 * base IDs that are **enabled to run by default**.
 *
 * Semantics:
 *   - All modules are still compiled into the binary.
 *   - This list only controls which modules are enabled for execution.
 *   - If a runtime override is provided (g_module_array / EL3 params),
 *     it takes priority over this list.
 *
 * If ACS_ENABLED_MODULE_LIST is not defined, ACS falls back to:
 *   - runtime overrides (if present), otherwise
 *   - "all modules enabled" behaviour.
 */
#ifdef ACS_ENABLED_MODULE_LIST
#define ACS_HAS_ENABLED_MODULE_LIST  1
#else
#define ACS_HAS_ENABLED_MODULE_LIST  0
#endif

#endif /* __BSA_AVS_LEVEL_H__ */

