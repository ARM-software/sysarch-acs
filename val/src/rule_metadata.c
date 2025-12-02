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

#include "include/rule_based_execution.h"
#include "include/acs_val.h"
#include "include/acs_smmu.h"
#include "include/acs_pcie.h"
#include "include/acs_common.h"
#include "include/val_interface.h"
#include "include/acs_smmu.h"
#include "include/acs_pe.h"
#include "include/acs_memory.h"
#include "include/acs_gic.h"
#include "include/acs_wd.h"
#include "include/acs_exerciser.h"
#include "include/acs_mpam.h"
#include "include/acs_pmu.h"
#include "include/acs_ras.h"
#include "include/acs_nist.h"
#include "include/acs_ete.h"
#include "include/acs_timer.h"
#include "include/acs_peripherals.h"
#include "include/acs_tpm.h"
#include "include/acs_wakeup.h"
#include "include/test_wrappers.h"

/*
 * Per-rule test status map
 *
 * Stores the latest execution status for each rule, indexed by RULE_ID_e.
 * The array size matches RULE_ID_SENTINEL so every legal rule ID is a valid index.
 * Values use the existing status codes returned by tests (e.g., TEST_PASS, TEST_FAIL,
 * TEST_SKIP, TEST_WARN etc). Use rule_status_map_reset() to initialize all
 * entries to TEST_STATUS_UNKNOWN before a run.
 */
uint32_t rule_status_map[RULE_ID_SENTINEL] = { 0 };

/* Following structure has every test entry that was is sysarch-acs on 23/07/25 */
rule_test_map_t rule_test_map[RULE_ID_SENTINEL] = {
        [S_L3_01] = {
            .test_entry_id    = NULL_ENTRY,
            .module_id        = BASE,
            .rule_desc        = "BSA Level 1 requirements",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI | PLATFORM_LINUX,
            .flag             = ALIAS_RULE,
        },
        [P_L1_01] = {
            .test_entry_id    = NULL_ENTRY,
            .module_id        = BASE,
            .rule_desc        = "BSA Level 1 requirements",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI | PLATFORM_LINUX,
            .flag             = ALIAS_RULE,
        },
    /*PE*/
        [B_PE_01] = {
            .test_entry_id    = PE001_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check Arch symmetry across PE",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  1,
        },
        [B_PE_02] = {
            .test_entry_id    = PE002_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for number of PE",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  2,
        },
        [B_PE_03] = {
            .test_entry_id    = PE003_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for AdvSIMD and FP support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  3,
        },
        [B_PE_04] = {
            .test_entry_id    = PE004_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check PE 4KB Granule Support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  4,
        },
        [B_PE_06] = {
            .test_entry_id    = PE006_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check Cryptographic extensions",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  6,
        },
        [B_PE_07] = {
            .test_entry_id    = PE007_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check Little Endian support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  7,
        },
        [B_PE_08] = {
            .test_entry_id    = PE008_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check EL1 and EL0 implementation",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  8,
        },
        [B_PE_09] = {
            .test_entry_id    = PE009_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for PMU and PMU counters",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  9,
        },
        [B_PE_10] = {
            .test_entry_id    = PE010_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check PMU Overflow signal",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  10,
        },
        [B_PE_11] = {
            .test_entry_id    = PE011_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check num of Breakpoints and type",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  11,
        },
        [B_PE_12] = {
            .test_entry_id    = PE012_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check Synchronous Watchpoints",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  12,
        },
        [B_PE_13] = {
            .test_entry_id    = PE013_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check CRC32 instruction support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  13,
        },
        [B_PE_14] = {
            .test_entry_id    = PE016_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check SVE2 for v9 PE",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  16,
        },
        [B_PE_18] = {
            .test_entry_id    = PE017_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check EL2 implementation",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  + 17,
        },
        [B_PE_19] = {
            .test_entry_id    = PE018_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check Stage 2 4KB Granule Support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  18,
        },
        [B_PE_20] = {
            .test_entry_id    = PE019_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check Stage2 and Stage1 Granule match",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE + 19,
        },
        [B_PE_21] = {
            .test_entry_id    = PE020_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for PMU counters",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  20,
        },
        [B_PE_22] = {
            .test_entry_id    = PE021_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check VMID breakpoint number",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  21,
        },
        [B_PE_23] = {
            .test_entry_id    = PE022_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for EL3 AArch64 support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  22,
        },
        [B_PE_24] = {
            .test_entry_id    = PE063_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for Secure state implementation",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  63,
        },
        [B_PE_17] = {
            .test_entry_id    = PE037_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check SPE if implemented",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  37,
        },
        [B_PE_25] = {
            .test_entry_id    = PE015_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for FEAT_LSE support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  15,
        },
        [XRPZG] = {
            .test_entry_id    = PE066_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check num of Breakpoints and type",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  66,
        },
        [B_SEC_01] = {
            .test_entry_id    = PE043_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check Speculation Restriction",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  43,
        },
        [B_SEC_02] = {
            .test_entry_id    = PE044_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check Speculative Str Bypass Safe",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  44,
        },
        [B_SEC_03] = {
            .test_entry_id    = PE045_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check PEs Impl CSDB,SSBB,PSSBB",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  + 45,
        },
        [B_SEC_04] = {
            .test_entry_id    = PE046_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check PEs Implement SB Barrier",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  46,
        },
        [B_SEC_05] = {
            .test_entry_id    = PE047_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check PE Impl CFP,DVP,CPP RCTX",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  47,
        },
        [S_L3PE_01] = {
            .test_entry_id    = PE023_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check PE Granule Support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  23,
        },
        [S_L3PE_02] = {
            .test_entry_id    = PE024_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for 16-bit ASID support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  24,
        },
        [S_L3PE_03] = {
            .test_entry_id    = PE025_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check AARCH64 implementation",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  25,
        },
        [S_L3PE_04] = {
            .test_entry_id    = PE026_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check FEAT_LPA Requirements",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE + 26,
        },
        [S_L4PE_01] = {
            .test_entry_id    = PE027_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for RAS extension",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  + 27,
        },
        [S_L4PE_02] = {
            .test_entry_id    = PE028_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check DC CVAP support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE + 28,
        },
        [S_L4PE_03] = {
            .test_entry_id    = PE029_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for 16-Bit VMID",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  + 29,
        },
        [S_L4PE_04] = {
            .test_entry_id    = PE030_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for Virtual host extensions",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  + 30,
        },
        [S_L5PE_01] = {
            .test_entry_id    = PE031_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Support Page table map size change",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  31,
        },
        [S_L5PE_02] = {
            .test_entry_id    = PE032_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for addr and generic auth",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  32,
        },
        [S_L5PE_04] = {
            .test_entry_id    = PE033_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check Activity monitors extension",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  33,
        },
        [S_L5PE_05] = {
            .test_entry_id    = PE034_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for SHA3 and SHA512 support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  34,
        },
        [S_L5PE_06] = {
            .test_entry_id    = PE035_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Stage 2 control of mem and cache",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  35,
        },
        [S_L5PE_07] = {
            .test_entry_id    = PE036_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for nested virtualization",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  36,
        },
        [S_L6PE_01] = {
            .test_entry_id    = NULL_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check PE security features",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = ALIAS_RULE,
        },
        [S_L6PE_02] = {
            .test_entry_id    = PE038_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check Branch Target Support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  38,
        },
        [S_L6PE_03] = {
            .test_entry_id    = PE039_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check Protect Against Timing Fault",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  39,
        },
        [S_L6PE_04] = {
            .test_entry_id    = PE040_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check PMU Version v3.5 or higher",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  40,
        },
        [S_L6PE_05] = {
            .test_entry_id    = PE041_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check AccessFlag DirtyState Update",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  41,
        },
        [S_L6PE_06] = {
            .test_entry_id    = PE042_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check Enhanced Virtualization Trap",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  42,
        },
        [S_L7PE_01] = {
            .test_entry_id    = PE048_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check Fine Grain Trap Support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  48,
        },
        [S_L7PE_02] = {
            .test_entry_id    = PE049_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for ECV support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  49,
        },
        [S_L7PE_04] = {
            .test_entry_id    = PE051_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Checks ASIMD Int8 matrix multiplc",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  51,
        },
        [S_L7PE_05] = {
            .test_entry_id    = PE052_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for BFLOAT16 extension",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE + 52,
        },
        [S_L7PE_06] = {
            .test_entry_id    = PE053_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check PAuth2, FPAC & FPACCOMBINE",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE + 53,
        },
        [S_L7PE_07] = {
            .test_entry_id    = PE054_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for SVE Int8 matrix multiple",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE + 54,
        },
        // [S_L7PE_08] = {
        //     .test_entry_id    = CO35_ENTRY,
        //     .module_id        = PE,
        //     .rule_desc        = "Check for data gathering hint",
        //     .platform_bitmask = 0,
        //     .flag             = BASE_RULE,
        // },
        // [S_L7PE_09] = {
        //     .test_entry_id    = CO36_ENTRY,
        //     .module_id        = PE,
        //     .rule_desc        = "Check WFE Fine tune delay feature",
        //     .platform_bitmask = 0,
        //     .flag             = BASE_RULE,
        // },
        [S_L8PE_01] = {
            .test_entry_id    = PE058_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check XS attribute functionality",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE + 58,
        },
        [S_L8PE_02] = {
            .test_entry_id    = PE059_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check WFET and WFIT functionality",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE + 59,
        },
        [S_L8PE_03] = {
            .test_entry_id    = PE060_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check atomic 64 byte store support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE + 60,
        },
        [S_L8PE_04] = {
            .test_entry_id    = PE057_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for enhanced PAN feature",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE + 57,
        },
        [S_L8PE_05] = {
            .test_entry_id    = PE064_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check PMU Version v3.7 or higher",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  64,
        },
        [S_L8PE_06] = {
            .test_entry_id    = PE061_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for FEAT_BRBE support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE + 61,
        },
        [S_L8PE_07] = {
            .test_entry_id    = PE062_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for unsupported PBHA bits",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE + 62,
        },
        [WNPXD] = {
            .test_entry_id    = PE065_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for FEAT_PFAR support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE + 65,
        },
    /* GIC */
        [B_GIC_01] = {
            .test_entry_id    = G001_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Check GIC version",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_GIC_TEST_NUM_BASE + 1,
        },
        [B_GIC_02] = {
            .test_entry_id    = G002_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Check GICv2 Valid Configuration",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_GIC_TEST_NUM_BASE + 2,
        },
        [B_GIC_03] = {
            .test_entry_id    = G003_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "If PCIe, GICv3 then ITS, LPI",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_GIC_TEST_NUM_BASE + 3,
        },
        [B_GIC_04] = {
            .test_entry_id    = G004_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Check GICv3 Security States",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_GIC_TEST_NUM_BASE + 4,
        },
        [B_GIC_05] = {
            .test_entry_id    = G005_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Non-secure SGIs are implemented",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_GIC_TEST_NUM_BASE + 5,
        },
        [ITS_01] = {
            .test_entry_id    = ITS001_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Check number of ITS blocks in a group",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_GIC_ITS_TEST_NUM_BASE + 1,
        },
        [ITS_02] = {
            .test_entry_id    = ITS002_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Check ITS block association with group",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_GIC_ITS_TEST_NUM_BASE + 2,
        },
        [ITS_DEV_2] = {
            .test_entry_id    = ITS003_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Check uniqueness of StreamID",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_GIC_ITS_TEST_NUM_BASE + 3,
        },
        [ITS_DEV_7] = {
            .test_entry_id    = ITS004_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Check Device's SID/RID/DID behind SMMU",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_GIC_ITS_TEST_NUM_BASE + 4,
        },
        [ITS_DEV_8] = {
            .test_entry_id    = ITS005_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Check Device IDs not behind SMMU",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_GIC_ITS_TEST_NUM_BASE + 5,
        },
        [S_L3GI_01] = {
            .test_entry_id    = G012_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Check GIC version",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_GIC_TEST_NUM_BASE + 12,
        },
        [B_PPI_00] = {
            .test_entry_id    = B_PPI_00_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "PPI Assignments check",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [S_L3PP_01] = {
            .test_entry_id    = G014_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Check All PPI Interrupt IDs",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_GIC_TEST_NUM_BASE + 14,
        },
        [S_L5GI_01] = {
            .test_entry_id    = G016_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Check Non GIC Interrupts",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_GIC_TEST_NUM_BASE + 16,
        },
        [S_L5PP_01] = {
            .test_entry_id    = G013_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Check Reserved PPI Assignments",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_GIC_TEST_NUM_BASE + 13,
        },
        [S_L8GI_01] = {
            .test_entry_id    = G015_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Check GICv4.1 or higher compliant",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_GIC_TEST_NUM_BASE + 15,
        },
        [Appendix_I_5] = {
            .test_entry_id    = V2M004_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Check GICv2m SPI allocated to MSI Ctrl",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_GIC_V2M_TEST_NUM_BASE + 4,
        },
        [Appendix_I_6] = {
            .test_entry_id    = APPENDIX_I_6_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [Appendix_I_9] = {
            .test_entry_id    = V2M002_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Check GICv2m MSI Frame Register",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_GIC_V2M_TEST_NUM_BASE + 2,
        },
    /* PERIPHERAL */
        [B_PER_01] = {
            .test_entry_id    = D001_ENTRY,
            .module_id        = PERIPHERAL,
            .rule_desc        = "USB CTRL Interface EHCI check",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PER_TEST_NUM_BASE + 1,
        },
        [B_PER_02] = {
            .test_entry_id    = D008_ENTRY,
            .module_id        = PERIPHERAL,
            .rule_desc        = "USB CTRL Interface XHCI check",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PER_TEST_NUM_BASE + 8,
        },
        [B_PER_03] = {
            .test_entry_id    = D002_ENTRY,
            .module_id        = PERIPHERAL,
            .rule_desc        = "Check SATA CTRL Interface",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PER_TEST_NUM_BASE + 2,
        },
        [B_PER_05] = {
            .test_entry_id    = D003_ENTRY,
            .module_id        = PERIPHERAL,
            .rule_desc        = "Check UART type Arm Generic or 16550",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PER_TEST_NUM_BASE + 3,
        },
        [S_L3PR_01] = {
            .test_entry_id    = NULL_ENTRY,
            .module_id        = PERIPHERAL,
            .rule_desc        = "Check UART Arm Generic or 16550 presence",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = ALIAS_RULE,
        },
        [B_PER_06] = {
            .test_entry_id    = D006_ENTRY,
            .module_id        = PERIPHERAL,
            .rule_desc        = "Check ARM Generic UART Interrupt",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PER_TEST_NUM_BASE + 6,
        },
        [B_PER_08] = {
            .test_entry_id    = NULL_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check PCI Express root complex",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI | PLATFORM_LINUX,
            .flag             = ALIAS_RULE,
        },
        [B_PER_09] = {
            .test_entry_id    = D004_ENTRY,
            .module_id        = PERIPHERAL,
            .rule_desc        = "Check Memory Attributes of DMA",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_LINUX,
            .flag             = BASE_RULE,
            .test_num         = ACS_PER_TEST_NUM_BASE + 4,
        },
        [B_PER_10] = {
            .test_entry_id    = D007_ENTRY,
            .module_id        = PERIPHERAL,
            .rule_desc        = "Check DMA for I/O coherency",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_LINUX,
            .flag             = BASE_RULE,
            .test_num         = ACS_PER_TEST_NUM_BASE + 7,
        },
    /* MEM_MAP */
        [B_MEM_01] = {
            .test_entry_id    = M002_ENTRY,
            .module_id        = MEM_MAP,
            .rule_desc        = "Mem Access Response in finite time",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_MEMORY_MAP_TEST_NUM_BASE + 2,
        },
        [B_MEM_02] = {
            .test_entry_id    = M001_ENTRY,
            .module_id        = MEM_MAP,
            .rule_desc        = "Memory Access to Un-Populated addr",
            .platform_bitmask = PLATFORM_BAREMETAL,
            .flag             = BASE_RULE,
            .test_num         = ACS_MEMORY_MAP_TEST_NUM_BASE + 1,
        },
        [B_MEM_03] = {
            .test_entry_id    = M004_ENTRY,
            .module_id        = MEM_MAP,
            .rule_desc        = "Check Addressability",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_LINUX,
            .flag             = BASE_RULE,
            .test_num         = ACS_MEMORY_MAP_TEST_NUM_BASE + 4,
        },
        [B_MEM_04] = {
            .test_entry_id    = M006_ENTRY,
            .module_id        = MEM_MAP,
            .rule_desc        = "Check dev DMA behind SMMU",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_LINUX,
            .flag             = BASE_RULE,
            .test_num         = ACS_MEMORY_MAP_TEST_NUM_BASE + 6,
        },
        [B_MEM_06] = {
            .test_entry_id    = M007_ENTRY,
            .module_id        = MEM_MAP,
            .rule_desc        = "Check non-DMA dev behind SMMU",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_LINUX,
            .flag             = BASE_RULE,
            .test_num         = ACS_MEMORY_MAP_TEST_NUM_BASE + 7,
        },
        [B_MEM_05] = {
            .test_entry_id    = M003_ENTRY,
            .module_id        = MEM_MAP,
            .rule_desc        = "PE must access all NS addr space",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_MEMORY_MAP_TEST_NUM_BASE + 3,
        },
        [S_L3MM_01] = {
            .test_entry_id    = M005_ENTRY,
            .module_id        = MEM_MAP,
            .rule_desc        = "NS-EL2 Stage-2 64KB Mapping Check",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_MEMORY_MAP_TEST_NUM_BASE + 5,
        },
        [S_L3MM_02] = {
            .test_entry_id    = M008_ENTRY,
            .module_id        = MEM_MAP,
            .rule_desc        = "Check peripherals addr 64Kb apart",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_MEMORY_MAP_TEST_NUM_BASE + 8,
        },
    /* PMU */
        [PMU_PE_02] = {
            .test_entry_id    = PMU001_ENTRY,
            .module_id        = PMU,
            .rule_desc        = "Check PMU Overflow signal",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PMU_TEST_NUM_BASE  +  1,
        },
        [PMU_PE_03] = {
            .test_entry_id    = PMU002_ENTRY,
            .module_id        = PMU,
            .rule_desc        = "Check number of PMU counters",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PMU_TEST_NUM_BASE  +  2,
        },
        [PMU_SYS_1] = {
            .test_entry_id    = PMU004_ENTRY,
            .module_id        = PMU,
            .rule_desc        = "Test Simultaneous 4 traffic measures",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PMU_TEST_NUM_BASE + 4,
        },
        [PMU_SYS_2] = {
            .test_entry_id    = PMU005_ENTRY,
            .module_id        = PMU,
            .rule_desc        = "Test Simultaneous 6 traffic measures",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PMU_TEST_NUM_BASE + 5,
        },
        // [PMU_SYS_5] = {
        //     .test_entry_id    = pmu008_entry,
        //     .module_id        = PMU,
        //     .rule_desc        = "Check System PMU for NUMA systems",
        //     .platform_bitmask = 0,
        //     .flag             = BASE_RULE,
        // },
        [PMU_SYS_6] = {
            .test_entry_id    = PMU009_ENTRY,
            .module_id        = PMU,
            .rule_desc        = "Check multiple type traffic measurement",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PMU_TEST_NUM_BASE + 9,
        },
        [PMU_BM_1] = {
            .test_entry_id    = PMU007_ENTRY,
            .module_id        = PMU,
            .rule_desc        = "Check for memory bandwidth monitors",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PMU_TEST_NUM_BASE + 7,
        },
        [PMU_MEM_1] = {
            .test_entry_id    = PMU010_ENTRY,
            .module_id        = PMU,
            .rule_desc        = "Check for memory latency monitors",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PMU_TEST_NUM_BASE + 10,
        },
        [PMU_BM_2] = {
            .test_entry_id    = PMU011_ENTRY,
            .module_id        = PMU,
            .rule_desc        = "Check for PCIe bandwidth monitors",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PMU_TEST_NUM_BASE + 11,
        },
        [PMU_EV_11] = {
            .test_entry_id    = PMU003_ENTRY,
            .module_id        = PMU,
            .rule_desc        = "Check for multi-threaded PMU ext",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PMU_TEST_NUM_BASE + 3,
        },
        [PMU_SPE] = {
            .test_entry_id    = PMU006_ENTRY,
            .module_id        = PMU,
            .rule_desc        = "Check for PMU SPE Requirements",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PMU_TEST_NUM_BASE + 6,
        },
    /* RAS */
        [RAS_01] = {
            .test_entry_id    = RAS001_ENTRY,
            .module_id        = RAS,
            .rule_desc        = "Check Error Counter",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_RAS_TEST_NUM_BASE + 1,
        },
        [RAS_02] = {
            .test_entry_id    = RAS002_ENTRY,
            .module_id        = RAS,
            .rule_desc        = "Check CFI, DUI, UI Controls",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_RAS_TEST_NUM_BASE + 2,
        },
        [RAS_03] = {
            .test_entry_id    = RAS003_ENTRY,
            .module_id        = RAS,
            .rule_desc        = "Check FHI in Error Record Group",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_RAS_TEST_NUM_BASE + 3,
        },
        [RAS_04] = {
            .test_entry_id    = RAS004_ENTRY,
            .module_id        = RAS,
            .rule_desc        = "Check ERI in Error Record Group",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_RAS_TEST_NUM_BASE + 4,
        },
        [RAS_06] = {
            .test_entry_id    = RAS005_ENTRY,
            .module_id        = RAS,
            .rule_desc        = "Check ERI/FHI Connected to GIC",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_RAS_TEST_NUM_BASE + 5,
        },
        [RAS_07] = {
            .test_entry_id    = RAS006_ENTRY,
            .module_id        = RAS,
            .rule_desc        = "RAS ERR<n>ADDR.AI bit status check",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_RAS_TEST_NUM_BASE + 6,
        },
        [RAS_08] = {
            .test_entry_id    = RAS007_ENTRY,
            .module_id        = RAS,
            .rule_desc        = "Check Error Group Status",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_RAS_TEST_NUM_BASE + 7,
        },
        [RAS_11] = {
            .test_entry_id    = RAS008_ENTRY,
            .module_id        = RAS,
            .rule_desc        = "Software Fault Error Check",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_RAS_TEST_NUM_BASE + 8,
        },
        [S_L7RAS_1] = {
            .test_entry_id    = RAS009_ENTRY,
            .module_id        = RAS,
            .rule_desc        = "Data abort on Containable err",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_RAS_TEST_NUM_BASE + 9,
        },
        [S_RAS_01] = {
            .test_entry_id    = RAS014_ENTRY,
            .module_id        = RAS,
            .rule_desc        = "Check RAS SR Interface ERI/FHI are PPI",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_RAS_TEST_NUM_BASE + 14,
        },
        [SYS_RAS] = {
            .test_entry_id    = NULL_ENTRY,
            .module_id        = RAS,
            .rule_desc        = "Check Server RAS requirements",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = ALIAS_RULE,
        },
        [SYS_RAS_1] = {
            .test_entry_id    = RAS010_ENTRY,
            .module_id        = RAS,
            .rule_desc        = "Check for patrol scrubbing support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_RAS_TEST_NUM_BASE + 10,
        },
        [SYS_RAS_2] = {
            .test_entry_id    = SYS_RAS_2_ENTRY,
            .module_id        = RAS,
            .rule_desc        = "Check Pseudo Fault Injection",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [SYS_RAS_3] = {
            .test_entry_id    = RAS015_ENTRY,
            .module_id        = RAS,
            .rule_desc        = "Check Error when Poison unsupported",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_RAS_TEST_NUM_BASE + 15,
        },
        [SYS_RAS_4] = {
            .test_entry_id    = RAS013_ENTRY,
            .module_id        = RAS,
            .rule_desc        = "Check RAS memory mapped view supp",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_RAS_TEST_NUM_BASE + 13,
        },
    /* SMMU */
        [B_SMMU_01] = {
            .test_entry_id    = I001_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "All SMMUs have same Arch Revision",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 1,
        },
        [B_SMMU_02] = {
            .test_entry_id    = I002_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check SMMU Granule Support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 2,
        },
        [B_SMMU_03] = {
            .test_entry_id    = I016_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check SMMU Large VA Support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 16,
        },
        [B_SMMU_04] = {
            .test_entry_id    = I017_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check TLB Range Invalidation",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 17,
        },
        [B_SMMU_05] = {
            .test_entry_id    = I030_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check DVM capabilities",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 30,
        },
        [B_SMMU_06] = {
            .test_entry_id    = I003_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check SMMU Large Physical Addr Support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 3,
        },
        [B_SMMU_08] = {
            .test_entry_id    = I004_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check SMMU S-EL2 & stage1 support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 4,
        },
        [B_SMMU_09] = {
            .test_entry_id    = I010_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check S-EL2 & SMMU Stage1 support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 10,
        },
        [B_SMMU_11] = {
            .test_entry_id    = I012_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check SMMU for MPAM support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 12,
        },
        [B_SMMU_13] = {
            .test_entry_id    = I018_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check SMMU 16 Bit ASID Support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 18,
        },
        [B_SMMU_14] = {
            .test_entry_id    = I019_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check SMMU Endianess Support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 19,
        },
        [B_SMMU_16] = {
            .test_entry_id    = I005_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check SMMUs stage2 support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 5,
        },
        [B_SMMU_18] = {
            .test_entry_id    = I029_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check SMMU S-EL2 & stage2 support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 29,
        },
        [B_SMMU_19] = {
            .test_entry_id    = I006_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "SMMUv2 unique intr per ctxt bank",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 6,
        },
        [B_SMMU_20] = {
            .test_entry_id    = I011_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check S-EL2 & SMMU Stage2 Support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 11,
        },
        [B_SMMU_21] = {
            .test_entry_id    = NULL_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "SMMUv3 Integration compliance",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = ALIAS_RULE,
        },
        [B_SMMU_23] = {
            .test_entry_id    = I015_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check SMMU 16 Bit VMID Support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 15,
        },
        [SMMU_01] = {
            .test_entry_id    = I007_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "SMMUv3 Integration compliance",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 7,
        },
        [S_L4SM_01] = {
            .test_entry_id    = I008_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check Stage 1 SMMUv3 functionality",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 8,
        },
        [S_L4SM_02] = {
            .test_entry_id    = I025_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check Stage 2 SMMUv3 functionality",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 25,
        },
        [S_L4SM_03] = {
            .test_entry_id    = I020_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check SMMU Coherent Access Support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 20,
        },
        [S_L5SM_01] = {
            .test_entry_id    = I009_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check SMMUv3.2 or higher",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 9,
        },
        [S_L5SM_02] = {
            .test_entry_id    = I026_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check SMMU L1 and L2 table resizing",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 26,
        },
        [S_L5SM_03] = {
            .test_entry_id    = I012_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check SMMU for MPAM support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 12,
        },
        [S_L6SM_02] = {
            .test_entry_id    = I013_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check SMMU HTTU Support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 13,
        },
        [S_L6SM_03] = {
            .test_entry_id    = I014_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check SMMU MSI Support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 14,
        },
        [S_L7SM_01] = {
            .test_entry_id    = I022_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check if all DMA reqs behind SMMU",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 22,
        },
        [S_L7SM_02] = {
            .test_entry_id    = I023_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check for SMMU/CATU in ETR Path",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_LINUX,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 23,
        },
        [S_L7SM_03] = {
            .test_entry_id    = I021_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check SMMU PMU Extension presence",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 21,
        },
        [S_L7SM_04] = {
            .test_entry_id    = I027_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check SMMU PMCG has >= 4 counters",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 27,
        },
        [S_L8SM_01] = {
            .test_entry_id    = I028_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check SMMUv3.3 or higher",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 28,
        },
    /* TIMER */
        [B_TIME_01] = {
            .test_entry_id    = T001_ENTRY,
            .module_id        = TIMER,
            .rule_desc        = "Check for Generic System Counter",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_TIMER_TEST_NUM_BASE + 1,
        },
        [B_TIME_02] = {
            .test_entry_id    = T007_ENTRY,
            .module_id        = TIMER,
            .rule_desc        = "Check System Counter Frequency",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_TIMER_TEST_NUM_BASE + 7,
        },
        [B_TIME_06] = {
            .test_entry_id    = T002_ENTRY,
            .module_id        = TIMER,
            .rule_desc        = "SYS Timer if PE Timer not ON",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_TIMER_TEST_NUM_BASE + 2,
        },
        [B_TIME_07] = {
            .test_entry_id    = T003_ENTRY,
            .module_id        = TIMER,
            .rule_desc        = "Memory mapped timer check",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_TIMER_TEST_NUM_BASE + 3,
        },
        [B_TIME_08] = {
            .test_entry_id    = T004_ENTRY,
            .module_id        = TIMER,
            .rule_desc        = "Generate Mem Mapped SYS Timer Intr",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_TIMER_TEST_NUM_BASE + 4,
        },
        [B_TIME_09] = {
            .test_entry_id    = T005_ENTRY,
            .module_id        = TIMER,
            .rule_desc        = "Restore PE timer on PE wake up",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_TIMER_TEST_NUM_BASE + 5,
        },
        [S_L8TI_01] = {
            .test_entry_id    = T006_ENTRY,
            .module_id        = TIMER,
            .rule_desc        = "Check Minimum Counter Frequency 50MHz",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_TIMER_TEST_NUM_BASE + 6,
        },
    /* WATCHDOG */
        [B_WD_00] = {
            .test_entry_id    = NULL_ENTRY,
            .module_id        = WATCHDOG,
            .rule_desc        = "Generic watchdog check",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = ALIAS_RULE,
        },
        [B_WD_01] = {
            .test_entry_id    = W001_ENTRY,
            .module_id        = WATCHDOG,
            .rule_desc        = "Non Secure Watchdog Access ",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_WD_TEST_NUM_BASE + 1,
        },
        [B_WD_03] = {
            .test_entry_id    = W002_ENTRY,
            .module_id        = WATCHDOG,
            .rule_desc        = "Check Watchdog WS0 interrupt ",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_WD_TEST_NUM_BASE + 2,
        },
        [S_L3WD_01] = {
            .test_entry_id    = NULL_ENTRY,
            .module_id        = WATCHDOG,
            .rule_desc        = "Non-secure Generic watchdog check",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = ALIAS_RULE,
        },
        [S_L6WD_01] = {
            .test_entry_id    = W003_ENTRY,
            .module_id        = WATCHDOG,
            .rule_desc        = "Check NS Watchdog Revision",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_WD_TEST_NUM_BASE + 3,
        },
    /* NIST */
        // TODO nist suite compilation with rule based infra
        [S_L7ENT_1] = {
            .test_entry_id    = N001_ENTRY,
            .module_id        = NIST,
            .rule_desc        = "NIST Statistical Test Suite",
            .platform_bitmask = 0,
            .flag             = BASE_RULE,
        },
    /* POWER WAKEUP*/
        [B_WAK_03] = {
            .test_entry_id    = B_WAK_03_07_ENTRY,
            .module_id        = POWER_WAKEUP,
            .rule_desc        = "Check power wakeup interrupts",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [B_WAK_07] = {
            .test_entry_id    = B_WAK_03_07_ENTRY,
            .module_id        = POWER_WAKEUP,
            .rule_desc        = "Check power wakeup interrupts",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
    /* PCIE */
        [B_REP_1] = {
            .test_entry_id    = NULL_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check RCiEP Devices",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI | PLATFORM_LINUX,
            .flag             = ALIAS_RULE,
        },
        [B_IEP_1] = {
            .test_entry_id    = NULL_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check i-EP Devices",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI | PLATFORM_LINUX,
            .flag             = ALIAS_RULE,
        },
        [B_PER_12] = {
            .test_entry_id    = P021_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Type 0 config header rules",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 21,
        },
        [GPU_03] = {
            .test_entry_id    = P093_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Switches must support ACS if P2P",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 93,
        },
        [GPU_04] = {
            .test_entry_id    = GPU_04_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check ATS support for RC and SMMU",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [IE_ACS_1] = {
            .test_entry_id    = P082_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check ACS Cap on p2p support: iEP EP",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 82,
        },
        [IE_ACS_2] = {
            .test_entry_id    = P081_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check iEP-RootPort P2P Support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 81,
        },
        [IE_ORD_4] = {
            .test_entry_id    = E038_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Tx pending bit clear correctness: iEP",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_EXERCISER_TEST_NUM_BASE + 38,
        },
        [IE_PWR_1] = {
            .test_entry_id    = P034_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check Power Management rules: iEP/RP",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 34,
        },
        [IE_REG_1] = {
            .test_entry_id    = IE_REG_1_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check config header rule: iEP_EP",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [IE_REG_2] = {
            .test_entry_id    = IE_REG_2_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check Dev Cap & Ctrl Reg rule - iEP_EP",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [IE_REG_3] = {
            .test_entry_id    = IE_REG_3_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check config header rule: iEP_RP",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [IE_REG_4] = {
            .test_entry_id    = IE_REG_4_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check PCIe capability rules: iEP_RP",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [IE_REG_5] = {
            .test_entry_id    = P057_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check Power Mgmt Cap/Ctrl/Status - iEP",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 57,
        },
        [IE_REG_6] = {
            .test_entry_id    = P092_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Secondary PCIe ECap Check: iEP Pair",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 92,
        },
        [IE_REG_7] = {
            .test_entry_id    = P012_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Datalink feature ECap Check: iEP Pair",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 12,
        },
        [IE_REG_8] = {
            .test_entry_id    = P013_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Phy Layer 16GT/s ECap Check: iEP Pair",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 13,
        },
        [IE_REG_9] = {
            .test_entry_id    = P014_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Lane Margining at Rec ECap Check: iEP",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 14,
        },
        [IE_RST_2] = {
            .test_entry_id    = P079_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check Sec Bus Reset For iEP_RP",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 79,
        },
        [ITS_03] = {
            .test_entry_id    = E011_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "MSI-capable device linked to ITS group",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_EXERCISER_TEST_NUM_BASE + 11,
        },
        [ITS_04] = {
            .test_entry_id    = E035_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "MSI-cap device can target any ITS blk",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_EXERCISER_TEST_NUM_BASE + 35,
        },
        [ITS_05] = {
            .test_entry_id    = E012_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "MSI to ITS Blk outside assigned group",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_EXERCISER_TEST_NUM_BASE + 12,
        },
        [ITS_DEV_4] = {
            .test_entry_id    = E013_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "MSI originating from different master",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_EXERCISER_TEST_NUM_BASE + 13,
        },
        [ITS_DEV_6] = {
            .test_entry_id    = E004_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Write to ITS GITS_TRANSLATER",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_EXERCISER_TEST_NUM_BASE + 4,
        },
        [PCI_ER_01] = {
            .test_entry_id    = P010_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check RP AER feature",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 10,
        },
        [PCI_ER_04] = {
            .test_entry_id    = E023_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check AER functionality for RPs",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_EXERCISER_TEST_NUM_BASE + 23,
        },
        [PCI_ER_05] = {
            .test_entry_id    = P007_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check RP DPC feature",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 7,
        },
        [PCI_ER_06] = {
            .test_entry_id    = E024_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check DPC funcionality for RPs",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_EXERCISER_TEST_NUM_BASE + 24,
        },
        [PCI_ER_07] = {
            .test_entry_id    = E029_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "RAS ERR record for external abort",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_EXERCISER_TEST_NUM_BASE + 29,
        },
        [PCI_ER_08] = {
            .test_entry_id    = E028_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "RAS ERR record for poisoned data",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_EXERCISER_TEST_NUM_BASE + 28,
        },
        [PCI_ER_09] = {
            .test_entry_id    = P090_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check RP Extensions for DPC",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 90,
        },
        [PCI_ER_10] = {
            .test_entry_id    = E027_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "DPC trig when RP-PIO unimplemented",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_EXERCISER_TEST_NUM_BASE + 27,
        },
        [PCI_IC_11] = {
            .test_entry_id    = PCI_IC_11_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "PCIe RC,PE - Same Inr Shareable Domain",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [PCI_IC_15] = {
            .test_entry_id    = E003_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Arrival order Check",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_EXERCISER_TEST_NUM_BASE + 3,
        },
        [PCI_IN_01] = {
            .test_entry_id    = P001_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check ECAM Presence",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI | PLATFORM_LINUX,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 1,
        },
        [PCI_IN_02] = {
            .test_entry_id    = P002_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check ECAM Memory accessibility",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 2,
        },
        [PCI_IN_03] = {
            .test_entry_id    = P038_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check all RP in HB is in same ECAM",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 38,
        },
        [PCI_IN_04] = {
            .test_entry_id    = PCI_IN_04_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "All EP/Sw under RP in same ECAM Region",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [PCI_IN_05] = {
            .test_entry_id    = PCI_IN_05_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Type 0/1 common config rule",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [PCI_IN_11] = {
            .test_entry_id    = E010_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check RP Sec Bus transaction are TYPE0",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_EXERCISER_TEST_NUM_BASE + 10,
        },
        [PCI_IN_12] = {
            .test_entry_id    = P037_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check Config Txn for RP in HB",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 37,
        },
        [PCI_IN_13] = {
            .test_entry_id    = PCI_IN_13_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check RootPort NP Memory Access",
            .platform_bitmask = PLATFORM_BAREMETAL,
            .flag             = BASE_RULE,
        },
        [PCI_IN_16] = {
            .test_entry_id    = P008_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check all 1's for out of range",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 8,
        },
        [PCI_IN_17] = {
            .test_entry_id    = PCI_IN_17_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check ARI forwarding enable rule",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [PCI_IN_18] = {
            .test_entry_id    = P011_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check RP Byte Enable Rules",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 11,
        },
        [PCI_IN_19] = {
            .test_entry_id    = PCI_IN_19_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check Cmd Reg memory space enable",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [PCI_IN_20] = {
            .test_entry_id    = P009_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Vendor specific data is PCIe compliant",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 9,
        },
        [PCI_LI_01] = {
            .test_entry_id    = PCI_LI_01_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check Legacy Interrupt is SPI",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [PCI_LI_02] = {
            .test_entry_id    = PCI_LI_02_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "PCI legacy intr SPI ID unique",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI | PLATFORM_LINUX,
            .flag             = BASE_RULE,
        },
        [PCI_LI_03] = {
            .test_entry_id    = PCI_LI_03_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check Legacy Intr SPI level sensitive",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [PCI_MM_01] = {
            .test_entry_id    = PCI_MM_01_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "PCIe Device Memory mapping support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI | PLATFORM_LINUX,
            .flag             = BASE_RULE,
        },
        [PCI_MM_03] = {
            .test_entry_id    = PCI_MM_03_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "PCIe Normal Memory mapping support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI | PLATFORM_LINUX,
            .flag             = BASE_RULE,
        },
        [PCI_MM_04] = {
            .test_entry_id    = P047_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "NP type-1 pcie only support 32-bit",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 47,
        },
        [PCI_MM_05] = {
            .test_entry_id    = P095_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "PCIe & PE common physical memory view",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_LINUX,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 95,
        },
        [PCI_MM_07] = {
            .test_entry_id    = P105_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "No extra address translation",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_LINUX,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 105,
        },
        [PCI_MSI_1] = {
            .test_entry_id    = P039_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check MSI support for PCIe dev",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 39,
        },
        [PCI_MSI_2] = {
            .test_entry_id    = PCI_MSI_2_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check MSI(X) vectors uniqueness",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI | PLATFORM_LINUX,
            .flag             = BASE_RULE,
        },
        [PCI_PAS_1] = {
            .test_entry_id    = P042_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "PASID support atleast 16 bits",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 42,
        },
        [PCI_PP_02] = {
            .test_entry_id    = E014_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "P2P transactions must not deadlock",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_EXERCISER_TEST_NUM_BASE + 14,
        },
        [PCI_PP_03] = {
            .test_entry_id    = P019_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "RP must suprt ACS if P2P Txn are allow",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 19,
        },
        [PCI_PP_04] = {
            .test_entry_id    = PCI_PP_04_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check P2P ACS Functionality",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [PCI_PP_05] = {
            .test_entry_id    = PCI_PP_05_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check Direct Transl P2P Support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [PCI_SM_02] = {
            .test_entry_id    = P035_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check Function level reset",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 35,
        },
        [S_L3GI_02] = {
            .test_entry_id    = P046_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check all MSI(X) vectors are LPIs",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI | PLATFORM_LINUX,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 46,
        },
        [S_L4PCI_2] = {
            .test_entry_id    = P087_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check EA Capability",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 87,
        },
        [RE_ACS_1] = {
            .test_entry_id    = P015_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check ACS Cap on p2p support: RCiEP",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 15,
        },
        [RE_ACS_2] = {
            .test_entry_id    = P016_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check AER Cap on ACS Cap support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 16,
        },
        [RI_BAR_1] = {
            .test_entry_id    = P083_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Read and write to BAR reg",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 83,
        },
        [RI_BAR_3] = {
            .test_entry_id    = P062_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check BAR memory space & type",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 62,
        },
        [RI_INT_1] = {
            .test_entry_id    = P069_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check MSI and MSI-X support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 69,
        },
        [RI_ORD_1] = {
            .test_entry_id    = E021_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Arrival order & Gathering Check",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  21,
        },
        [RE_ORD_4] = {
            .test_entry_id    = E008_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Tx pending bit clear correctness: RCiEP",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  8,
        },
        [RE_PCI_1] = {
            .test_entry_id    = P085_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check RCiEP Hdr type & link Cap",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 85,
        },
        [RE_PCI_2] = {
            .test_entry_id    = P084_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check RCEC Class code and Ext Cap",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 84,
        },
        [RE_PWR_1] = {
            .test_entry_id    = P070_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check Power Management rules: RCiEP",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 70,
        },
        [RE_REC_1] = {
            .test_entry_id    = RE_REC_1_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check Dev Cap & Ctrl Reg rule - RCEC",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [RE_REG_1] = {
            .test_entry_id    = RE_REG_1_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check config header rule: RCEC/RCiEP",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [RE_REG_2] = {
            .test_entry_id    = P056_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check Power Mgmt Cap/Ctrl/Status - RC",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 56,
        },
        [RE_REG_3] = {
            .test_entry_id    = P052_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check Dev Cap & Ctrl Reg rule - RCiEP",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 52,
        },
        [RI_RST_1] = {
            .test_entry_id    = P063_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check Function level reset",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 63,
        },
        [RI_SMU_1] = {
            .test_entry_id    = E019_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check ATS Support Rule",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_EXERCISER_TEST_NUM_BASE + 19,
        },
        [RI_SMU_3] = {
            .test_entry_id    = E036_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Generate PASID transactions",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_EXERCISER_TEST_NUM_BASE  +  36,
        },
        [S_L6PCI_1] = {
            .test_entry_id    = NULL_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check PCIe On-chip Peripherals",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI | PLATFORM_LINUX,
            .flag             = ALIAS_RULE,
        },
        [S_PCIe_02] = {
            .test_entry_id    = P086_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check RootPort P&NP Memory Access",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 86,
        },
        [S_PCIe_03] = {
            .test_entry_id    = E022_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "PE 2/4/8B writes to PCIe as 2/4/8B",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_EXERCISER_TEST_NUM_BASE + 22,
        },
        [S_PCIe_04] = {
            .test_entry_id    = E025_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check 2/4/8 Bytes targeted writes",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_EXERCISER_TEST_NUM_BASE + 25,
        },
        [S_PCIe_07] = {
            .test_entry_id    = E026_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check Inbound writes seen in order",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE + 26,
        },
        [S_PCIe_08] = {
            .test_entry_id    = E032_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check ordered writes flush prev writes",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  32,
        },
        [B_PCIe_10] = {
            .test_entry_id    = E030_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Enable and disable STE.DCP bit",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_EXERCISER_TEST_NUM_BASE + 30,
        },
        [B_PCIe_11] = {
            .test_entry_id    = P091_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Steering Tag value properties",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_LINUX,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 91,
        },
        [BJLPB] = {
            .test_entry_id    = P100_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check MSI/MSI-X if FRS is supported",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 100,
        },
    /* MPAM */
        [S_L7MP_01] = {
            .test_entry_id    = MPAM001_ENTRY,
            .module_id        = MPAM,
            .rule_desc        = "Check for MPAM extension",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_MPAM_TEST_NUM_BASE + 1,
        },
        [S_L7MP_02] = {
            .test_entry_id    = MPAM008_ENTRY,
            .module_id        = MPAM,
            .rule_desc        = "Check for MPAM partition IDs ",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_MPAM_TEST_NUM_BASE + 8,
        },
        [S_L7MP_03] = {
            .test_entry_id    = S_L7MP_03_ENTRY,
            .module_id        = MPAM,
            .rule_desc        = "Check MPAM LLC Requirements",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [S_L7MP_04] = {
            .test_entry_id    = MPAM009_ENTRY,
            .module_id        = MPAM,
            .rule_desc        = "Check for MPAM LLC CSU Monitor count",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_MPAM_TEST_NUM_BASE + 2,
        },
        [S_L7MP_05] = {
            .test_entry_id    = MPAM003_ENTRY,
            .module_id        = MPAM,
            .rule_desc        = "Check for MPAM MBWUs Monitor func",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_MPAM_TEST_NUM_BASE + 3,
        },
        // [S_L7MP_07] = {
        //     .test_entry_id    = mpam004_entry,
        //     .module_id        = MPAM,
        //     .rule_desc        = "Check for MBWU counter size",
        //     .platform_bitmask = 0,
        //     .flag             = BASE_RULE,
        // },
        [S_L7MP_08] = {
            .test_entry_id    = MPAM005_ENTRY,
            .module_id        = MPAM,
            .rule_desc        = "Check for MPAM MSC address overlap",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_MPAM_TEST_NUM_BASE + 5,
        },
    /* ETE */
        [ETE_02] = {
            .test_entry_id    = ETE001_ENTRY,
            .module_id        = ETE,
            .rule_desc        = "Check for FEAT_ETE",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_ETE_TEST_NUM_BASE + 1,
        },
        [ETE_03] = {
            .test_entry_id    = ETE002_ENTRY,
            .module_id        = ETE,
            .rule_desc        = "Check trace unit ETE supports",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_ETE_TEST_NUM_BASE + 2,
        },
        [ETE_04] = {
            .test_entry_id    = ETE003_ENTRY,
            .module_id        = ETE,
            .rule_desc        = "Check ETE Same Trace Timestamp Source",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_ETE_TEST_NUM_BASE + 3,
        },
        [ETE_05] = {
            .test_entry_id    = ETE004_ENTRY,
            .module_id        = ETE,
            .rule_desc        = "Check Trace Same Timestamp Source",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_ETE_TEST_NUM_BASE + 4,
        },
        [ETE_06] = {
            .test_entry_id    = ETE009_ENTRY,
            .module_id        = ETE,
            .rule_desc        = "Check Concurrent Trace Generation",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_ETE_TEST_NUM_BASE + 9,
        },
        [ETE_07] = {
            .test_entry_id    = ETE005_ENTRY,
            .module_id        = ETE,
            .rule_desc        = "Check for FEAT_TRBE",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_ETE_TEST_NUM_BASE + 5,
        },
        [ETE_08] = {
            .test_entry_id    = ETE006_ENTRY,
            .module_id        = ETE,
            .rule_desc        = "Check trace buffers flag updates",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_ETE_TEST_NUM_BASE + 6,
        },
        [ETE_09] = {
            .test_entry_id    = ETE007_ENTRY,
            .module_id        = ETE,
            .rule_desc        = "Check TRBE trace buffers alignment",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_ETE_TEST_NUM_BASE + 7,
        },
        [ETE_10] = {
            .test_entry_id    = ETE008_ENTRY,
            .module_id        = ETE,
            .rule_desc        = "Check GICC TRBE Interrupt field",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_ETE_TEST_NUM_BASE + 8,
        },

    /* PC-BSA rule test mapping */
        [P_L1GI_01] = {
            .test_entry_id    = G012_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Check GIC version",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_GIC_TEST_NUM_BASE + 12,
        },
        [P_L1PP_01] = {
            .test_entry_id    = G014_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Check All PPI Interrupt IDs",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_GIC_TEST_NUM_BASE + 14,
        },
        [P_L1GI_03] = {
            .test_entry_id    = INTERFACE010_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Check GIC supports disabling LPIs",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_GIC_TEST_NUM_BASE  +  10,
        },
        [P_L1GI_04] = {
            .test_entry_id    = INTERFACE011_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Check GICR_PENDBASER when LPIs enabled",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_GIC_TEST_NUM_BASE  +  11,
        },
        [P_L1MM_01] = {
            .test_entry_id    = NULL_ENTRY,
            .module_id        = MEM_MAP,
            .rule_desc        = "NS-EL2 Stage-2 64KB Mapping Check",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = ALIAS_RULE,
        },
        [P_L1PE_01] = {
            .test_entry_id    = P_L1PE_01_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check PE 4KB Granule Support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
        },
        [P_L1PE_02] = {
            .test_entry_id    = PE024_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for 16-bit ASID support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  24,
        },
        [P_L1PE_03] = {
            .test_entry_id    = PE025_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check AARCH64 implementation",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  25,
        },
        [P_L1PE_04] = {
            .test_entry_id    = PE015_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for FEAT_LSE support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  15,
        },
        [P_L1PE_05] = {
            .test_entry_id    = PE026_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check FEAT_LPA Requirements",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE + 26,
        },
        [P_L1PE_06] = {
            .test_entry_id    = PE028_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check DC CVAP support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE + 28,
        },
        [P_L1PE_07] = {
            .test_entry_id    = PE029_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for 16-Bit VMID",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  + 29,
        },
        [P_L1PE_08] = {
            .test_entry_id    = PE030_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check for Virtual host extensions",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  + 30,
        },
        [P_L1SM_02] = {
            .test_entry_id    = I008_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check Stage 1 SMMUv3 functionality",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 8,
        },
        [P_L1SM_03] = {
            .test_entry_id    = I025_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check Stage 2 SMMUv3 functionality",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 25,
        },
        [P_L1SM_04] = {
            .test_entry_id    = I020_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check SMMU Coherent Access Support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 20,
        },
        [P_L1SM_05] = {
            .test_entry_id    = I022_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check if all DMA reqs behind SMMU",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 22,
        },
        [P_L2WD_01] = {
            .test_entry_id    = NULL_ENTRY,
            .module_id        = WATCHDOG,
            .rule_desc        = "Non-secure Generic watchdog check",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = ALIAS_RULE,
        },
        [P_L1TP_01] = {
            .test_entry_id    = TPM001_ENTRY,
            .module_id        = TPM,
            .rule_desc        = "Check TPM Version",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_TPM2_TEST_NUM_BASE  +  1,
        },
        [P_L1TP_03] = {
            .test_entry_id    = TPM002_ENTRY,
            .module_id        = TPM,
            .rule_desc        = "Check TPM interface locality support",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_TPM2_TEST_NUM_BASE  +  2,
        },
        [P_L1GI_02] = {
            .test_entry_id    = P046_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check all MSI(X) vectors are LPIs",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI | PLATFORM_LINUX,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 46,
        },
        [P_L1PCI_2] = {
            .test_entry_id    = P087_ENTRY,
            .module_id        = PCIE,
            .rule_desc        = "Check EA Capability",
            .platform_bitmask = PLATFORM_BAREMETAL | PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PCIE_TEST_NUM_BASE + 87,
        },
        /* VBSA ACS entries */
        [V_L1PE_01] = {
            .test_entry_id    = NULL_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check vPE architectural compliance",
            .platform_bitmask = PLATFORM_UEFI,
            .flag             = ALIAS_RULE
        },
        [V_L1PE_02] = {
            .test_entry_id    = V_L1PE_02_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check PMUv3 support in vPE",
            .platform_bitmask = PLATFORM_UEFI,
            .flag             = BASE_RULE
        },
        [V_L1MM_01] = {
            .test_entry_id    = NULL_ENTRY,
            .module_id        = MEM_MAP,
            .rule_desc        = "Check VE Mem architectural compliance",
            .platform_bitmask = PLATFORM_UEFI,
            .flag             = ALIAS_RULE
        },
        [V_L1MM_02] = {
            .test_entry_id    = M004_ENTRY,
            .module_id        = MEM_MAP,
            .rule_desc        = "DMA requestors access to NS addr space",
            .platform_bitmask = PLATFORM_LINUX,
            .flag             = BASE_RULE,
            .test_num         = ACS_MEMORY_MAP_TEST_NUM_BASE + 4,
        },
        [V_L1GI_01] = {
            .test_entry_id    = NULL_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Check vGIC architectural compliance",
            .platform_bitmask = PLATFORM_UEFI,
            .flag             = ALIAS_RULE
        },
        [V_L1PP_00] = {
            .test_entry_id    = V_L1PP_00_ENTRY,
            .module_id        = GIC,
            .rule_desc        = "Check VE PPI assignment mapping",
            .platform_bitmask = PLATFORM_UEFI,
            .flag             = BASE_RULE
        },
        [V_L1SM_01] = {
            .test_entry_id    = NULL_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check vSMMU architectural compliance",
            .platform_bitmask = PLATFORM_UEFI,
            .flag             = ALIAS_RULE
        },
        [V_L1SM_02] = {
            .test_entry_id    = I031_ENTRY,
            .module_id        = SMMU,
            .rule_desc        = "Check SMMU stage 1 support for VE",
            .platform_bitmask = PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_SMMU_TEST_NUM_BASE + 31,
        },
        [V_L1TM_01] = {
            .test_entry_id    = T001_ENTRY,
            .module_id        = TIMER,
            .rule_desc        = "Check Virt and Phy counter presence",
            .platform_bitmask = PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_TIMER_TEST_NUM_BASE + 1,
        },
        [V_L1TM_02] = {
            .test_entry_id    = T007_ENTRY,
            .module_id        = TIMER,
            .rule_desc        = "Check Virt and Phy counter min freq",
            .platform_bitmask = PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_TIMER_TEST_NUM_BASE + 7,
        },
        [V_L1TM_04] = {
            .test_entry_id    = T008_ENTRY,
            .module_id        = TIMER,
            .rule_desc        = "Check uniform passage of time in VE",
            .platform_bitmask = PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_TIMER_TEST_NUM_BASE + 8,
        },
        [V_L1WK_02] = {
            .test_entry_id    = V_L1WK_02_05_ENTRY,
            .module_id        = POWER_WAKEUP,
            .rule_desc        = "Check power wakeup interrupts",
            .platform_bitmask = PLATFORM_UEFI,
            .flag             = BASE_RULE
        },
        [V_L1WK_05] = {
            .test_entry_id    = V_L1WK_02_05_ENTRY,
            .module_id        = POWER_WAKEUP,
            .rule_desc        = "Check vPE power state semantics",
            .platform_bitmask = PLATFORM_UEFI,
            .flag             = BASE_RULE
        },
        [V_L1PR_01] = {
            .test_entry_id    = NULL_ENTRY,
            .module_id        = PERIPHERAL,
            .rule_desc        = "Check Peripheral arch compliance",
            .platform_bitmask = PLATFORM_UEFI,
            .flag             = ALIAS_RULE
        },
        [V_L1PR_02] = {
            .test_entry_id    = NULL_ENTRY,
            .module_id        = PERIPHERAL,
            .rule_desc        = "Check Virt PCIe arch compliance",
            .platform_bitmask = PLATFORM_UEFI,
            .flag             = ALIAS_RULE
        },
        [V_L2PE_01] = {
            .test_entry_id    = PE068_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check num PMU counters for vPEs",
            .platform_bitmask = PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  68,
        },
        [V_L2PE_02] = {
            .test_entry_id    = PE010_ENTRY,
            .module_id        = PE,
            .rule_desc        = "Check vPE PMU overflow signal",
            .platform_bitmask = PLATFORM_UEFI,
            .flag             = BASE_RULE,
            .test_num         = ACS_PE_TEST_NUM_BASE  +  10,
        },
    };

/* Following structure maps test entry enums with entry function pointers
   Entries are guarded with TARGET_* macros to compile the entry functions based on test support
   for PAL for which compilation is carried out */
test_entry_fn_t test_entry_func_table[TEST_ENTRY_SENTINEL] = {
    [NULL_ENTRY] = NULL,
/* TARGET_LINUX */
#if defined(TARGET_LINUX)
    [P001_ENTRY] = p001_entry,
    [P045_ENTRY] = p045_entry, // used in wrapper.
    [P091_ENTRY] = p091_entry, // used in wrapper.
    [P103_ENTRY] = p103_entry, // used in wrapper.
    [P094_ENTRY] = p094_entry, // used in wrapper.
    [P104_ENTRY] = p104_entry, // used in wrapper.
    [PCI_LI_02_ENTRY] = pci_li_02_entry,
    [PCI_MM_01_ENTRY] = pci_mm_01_entry,
    [PCI_MM_03_ENTRY] = pci_mm_03_entry,
    [PCI_MSI_2_ENTRY] = pci_msi_2_entry,
    [D004_ENTRY] = d004_entry,
    [D007_ENTRY] = d007_entry,
    [P105_ENTRY] = p105_entry,
    [M004_ENTRY] = m004_entry,
    [P096_ENTRY] = p096_entry, // used in wrapper.
    [P097_ENTRY] = p097_entry, // used in wrapper.
    [M006_ENTRY] = m006_entry,
    [P095_ENTRY] = p095_entry,
    [M007_ENTRY] = m007_entry,
/* Skip build of sbsa only tests for BSA */
#ifndef BSA_LINUX_BUILD
    [I023_ENTRY] = i023_entry,
    [P046_ENTRY] = p046_entry,
#endif /* BSA_LINUX_BUILD */
#endif /* TARGET_LINUX */

/* TARGET_UEFI */
#if defined(TARGET_UEFI)
    [D001_ENTRY] = d001_entry,
    [D002_ENTRY] = d002_entry,
    [D003_ENTRY] = d003_entry,
    [D006_ENTRY] = d006_entry,
    [D008_ENTRY] = d008_entry,
    [E001_ENTRY] = e001_entry, // used in wrapper.
    [E002_ENTRY] = e002_entry, // used in wrapper.
    [E003_ENTRY] = e003_entry,
    [E004_ENTRY] = e004_entry,
    [E006_ENTRY] = e006_entry, // used in wrapper.
    [E007_ENTRY] = e007_entry, // used in wrapper.
    [E008_ENTRY] = e008_entry,
    [E010_ENTRY] = e010_entry,
    [E011_ENTRY] = e011_entry,
    [E012_ENTRY] = e012_entry,
    [E013_ENTRY] = e013_entry,
    [E014_ENTRY] = e014_entry,
    [E015_ENTRY] = e015_entry, // used in wrapper.
    [E016_ENTRY] = e016_entry, // used in wrapper.
    [E017_ENTRY] = e017_entry, // used in wrapper.
    [E019_ENTRY] = e019_entry, // used in wrapper.
    [E020_ENTRY] = e020_entry, // used in wrapper.
    [E021_ENTRY] = e021_entry,
    [E022_ENTRY] = e022_entry,
    [E023_ENTRY] = e023_entry,
    [E024_ENTRY] = e024_entry,
    [E025_ENTRY] = e025_entry,
    [E026_ENTRY] = e026_entry,
    [E027_ENTRY] = e027_entry,
    [E028_ENTRY] = e028_entry,
    [E029_ENTRY] = e029_entry,
    [E030_ENTRY] = e030_entry,
    [E032_ENTRY] = e032_entry,
    [E033_ENTRY] = e033_entry, // used in wrapper.
    [E034_ENTRY] = e034_entry, // used in wrapper.
    [E035_ENTRY] = e035_entry,
    [E036_ENTRY] = e036_entry,
    [E038_ENTRY] = e038_entry,
    [E039_ENTRY] = e039_entry, // used in wrapper.
    [ETE001_ENTRY] = ete001_entry,
    [ETE002_ENTRY] = ete002_entry,
    [ETE003_ENTRY] = ete003_entry,
    [ETE004_ENTRY] = ete004_entry,
    [ETE005_ENTRY] = ete005_entry,
    [ETE006_ENTRY] = ete006_entry,
    [ETE007_ENTRY] = ete007_entry,
    [ETE008_ENTRY] = ete008_entry,
    [ETE009_ENTRY] = ete009_entry,
    [G001_ENTRY] = g001_entry,
    [G002_ENTRY] = g002_entry,
    [G003_ENTRY] = g003_entry,
    [G004_ENTRY] = g004_entry,
    [G005_ENTRY] = g005_entry,
    [G006_ENTRY] = g006_entry, // used in wrapper.
    [G007_ENTRY] = g007_entry, // used in wrapper.
    // [G008_ENTRY] = g008_entry, // unused testcase
    [G009_ENTRY] = g009_entry, // used in wrapper.
    [G010_ENTRY] = g010_entry, // used in wrapper.
    [G011_ENTRY] = g011_entry, // used in wrapper.
    [G012_ENTRY] = g012_entry,
    [P_L1PE_01_ENTRY]  = p_l1pe_01_entry,
    [B_PPI_00_ENTRY]   = b_ppi_00_entry,
    [B_WAK_03_07_ENTRY] = b_wak_03_07_entry,
    [GPU_04_ENTRY] = gpu_04_entry,
    [S_L7MP_03_ENTRY]  = s_l7mp_03_entry,
    [APPENDIX_I_6_ENTRY] = appendix_i_6_entry,
    [IE_REG_1_ENTRY]   = ie_reg_1_entry,
    [IE_REG_2_ENTRY]   = ie_reg_2_entry,
    [IE_REG_3_ENTRY]   = ie_reg_3_entry,
    [IE_REG_4_ENTRY]   = ie_reg_4_entry,
    [PCI_IC_11_ENTRY]  = pci_ic_11_entry,
    [PCI_IN_04_ENTRY]  = pci_in_04_entry,
    [PCI_IN_05_ENTRY]  = pci_in_05_entry,
    [PCI_IN_13_ENTRY]  = pci_in_13_entry,
    [PCI_IN_17_ENTRY]  = pci_in_17_entry,
    [PCI_IN_19_ENTRY]  = pci_in_19_entry,
    [PCI_LI_01_ENTRY]  = pci_li_01_entry,
    [PCI_LI_02_ENTRY]  = pci_li_02_entry,
    [PCI_LI_03_ENTRY]  = pci_li_03_entry,
    [PCI_MM_01_ENTRY]  = pci_mm_01_entry,
    [PCI_MM_03_ENTRY]  = pci_mm_03_entry,
    [PCI_MSI_2_ENTRY]  = pci_msi_2_entry,
    [PCI_PP_04_ENTRY]  = pci_pp_04_entry,
    [PCI_PP_05_ENTRY]  = pci_pp_05_entry,
    [RE_REC_1_ENTRY]   = re_rec_1_entry,
    [RE_REG_1_ENTRY]   = re_reg_1_entry,
    [V_L1PE_02_ENTRY]  = v_l1pe_02_entry,
    [RI_SMU_1_ENTRY]   = ri_smu_1_entry,
    [G013_ENTRY] = g013_entry,
    [G014_ENTRY] = g014_entry,
    [G015_ENTRY] = g015_entry,
    [G016_ENTRY] = g016_entry,
    [I001_ENTRY] = i001_entry,
    [I002_ENTRY] = i002_entry,
    [I003_ENTRY] = i003_entry,
    [I004_ENTRY] = i004_entry,
    [I005_ENTRY] = i005_entry,
    [I006_ENTRY] = i006_entry,
    [I007_ENTRY] = i007_entry,
    [I008_ENTRY] = i008_entry,
    [I009_ENTRY] = i009_entry,
    [I010_ENTRY] = i010_entry,
    [I011_ENTRY] = i011_entry,
    [I012_ENTRY] = i012_entry,
    [I013_ENTRY] = i013_entry,
    [I015_ENTRY] = i015_entry, // used in wrapper
    [I016_ENTRY] = i016_entry,
    [I017_ENTRY] = i017_entry,
    [I018_ENTRY] = i018_entry,
    [I019_ENTRY] = i019_entry,
    [I020_ENTRY] = i020_entry,
    [I021_ENTRY] = i021_entry,
    [I022_ENTRY] = i022_entry,
    [I024_ENTRY] = i024_entry, // used in wrapper
    [I025_ENTRY] = i025_entry,
    [I026_ENTRY] = i026_entry,
    [I027_ENTRY] = i027_entry,
    [I028_ENTRY] = i028_entry,
    [I029_ENTRY] = i029_entry,
    [I030_ENTRY] = i030_entry,
    [I031_ENTRY] = i031_entry,
    [INTERFACE010_ENTRY] = interface010_entry,
    [INTERFACE011_ENTRY] = interface011_entry,
    [ITS001_ENTRY] = its001_entry,
    [ITS002_ENTRY] = its002_entry,
    [ITS003_ENTRY] = its003_entry,
    [ITS004_ENTRY] = its004_entry,
    [ITS005_ENTRY] = its005_entry,
    [M002_ENTRY] = m002_entry,
    [M003_ENTRY] = m003_entry,
    [M005_ENTRY] = m005_entry,
    [M008_ENTRY] = m008_entry,
    [MPAM001_ENTRY] = mpam001_entry,
    [MPAM002_ENTRY] = mpam002_entry, // used in wrapper.
    [MPAM005_ENTRY] = mpam005_entry,
    [MPAM006_ENTRY] = mpam006_entry, // used in wrapper.
    [MPAM007_ENTRY] = mpam007_entry, // used in wrapper.
    [MPAM008_ENTRY] = mpam008_entry,
    [MPAM009_ENTRY] = mpam009_entry,
    [P001_ENTRY] = p001_entry,
    [P002_ENTRY] = p002_entry,
    [P003_ENTRY] = p003_entry, // used in wrapper.
    [P004_ENTRY] = p004_entry, // used in wrapper.
    [P005_ENTRY] = p005_entry, // used in wrapper.
    [P006_ENTRY] = p006_entry, // used in wrapper.
    [P007_ENTRY] = p007_entry,
    [P008_ENTRY] = p008_entry,
    [P009_ENTRY] = p009_entry,
    [P010_ENTRY] = p010_entry,
    [P011_ENTRY] = p011_entry,
    [P012_ENTRY] = p012_entry,
    [P013_ENTRY] = p013_entry,
    [P014_ENTRY] = p014_entry,
    [P015_ENTRY] = p015_entry,
    [P016_ENTRY] = p016_entry,
    [P017_ENTRY] = p017_entry, // used in wrapper.
    [P018_ENTRY] = p018_entry, // used in wrapper.
    [P019_ENTRY] = p019_entry,
    [P020_ENTRY] = p020_entry, // used in wrapper.
    [P021_ENTRY] = p021_entry,
    [P022_ENTRY] = p022_entry, // used in wrapper.
    [P023_ENTRY] = p023_entry, // used in wrapper.
    [P024_ENTRY] = p024_entry, // used in wrapper.
    [P025_ENTRY] = p025_entry, // used in wrapper.
    [P026_ENTRY] = p026_entry, // used in wrapper.
    [P030_ENTRY] = p030_entry, // used in wrapper.
    [P031_ENTRY] = p031_entry, // used in wrapper.
    [P032_ENTRY] = p032_entry, // used in wrapper.
    [P033_ENTRY] = p033_entry, // used in wrapper.
    [P034_ENTRY] = p034_entry,
    [P035_ENTRY] = p035_entry,
    [P036_ENTRY] = p036_entry, // used in wrapper.
    [P037_ENTRY] = p037_entry,
    [P038_ENTRY] = p038_entry,
    [P039_ENTRY] = p039_entry,
    [P042_ENTRY] = p042_entry,
    [P045_ENTRY] = p045_entry, // used in wrapper.
    [P046_ENTRY] = p046_entry,
    [P047_ENTRY] = p047_entry,
    [P048_ENTRY] = p048_entry, // used in wrapper.
    [P049_ENTRY] = p049_entry, // used in wrapper.
    [P050_ENTRY] = p050_entry, // used in wrapper.
    [P051_ENTRY] = p051_entry, // used in wrapper.
    [P052_ENTRY] = p052_entry,
    [P053_ENTRY] = p053_entry, // used in wrapper.
    [P054_ENTRY] = p054_entry, // used in wrapper.
    [P056_ENTRY] = p056_entry,
    [P057_ENTRY] = p057_entry,
    [P058_ENTRY] = p058_entry, // used in wrapper.
    [P059_ENTRY] = p059_entry, // used in wrapper.
    [P060_ENTRY] = p060_entry, // used in wrapper.
    [P061_ENTRY] = p061_entry, // used in wrapper.
    [P062_ENTRY] = p062_entry,
    [P063_ENTRY] = p063_entry,
    [P064_ENTRY] = p064_entry, // used in wrapper.
    [P065_ENTRY] = p065_entry, // used in wrapper.
    [P066_ENTRY] = p066_entry, // used in wrapper.
    [P067_ENTRY] = p067_entry, // used in wrapper.
    [P068_ENTRY] = p068_entry, // used in wrapper.
    [P069_ENTRY] = p069_entry,
    [P070_ENTRY] = p070_entry,
    [P071_ENTRY] = p071_entry, // used in wrapper.
    [P072_ENTRY] = p072_entry, // used in wrapper.
    [P078_ENTRY] = p078_entry, // used in wrapper.
    [P079_ENTRY] = p079_entry,
    [P080_ENTRY] = p080_entry,
    [P081_ENTRY] = p081_entry,
    [P082_ENTRY] = p082_entry,
    [P083_ENTRY] = p083_entry,
    [P084_ENTRY] = p084_entry,
    [P085_ENTRY] = p085_entry,
    [P086_ENTRY] = p086_entry,
    [P087_ENTRY] = p087_entry,
    [P088_ENTRY] = p088_entry, // used in wrapper.
    [P089_ENTRY] = p089_entry, // used in wrapper.
    [P090_ENTRY] = p090_entry,
    [P092_ENTRY] = p092_entry,
    [P093_ENTRY] = p093_entry,
    [P098_ENTRY] = p098_entry, // used in wrapper.
    [P099_ENTRY] = p099_entry, // used in wrapper.
    [P100_ENTRY] = p100_entry,
    [P103_ENTRY] = p103_entry, // used in wrapper.
    [PE001_ENTRY] = pe001_entry,
    [PE002_ENTRY] = pe002_entry,
    [PE003_ENTRY] = pe003_entry,
    [PE004_ENTRY] = pe004_entry, // used in wrapper.
    [PE006_ENTRY] = pe006_entry,
    [PE007_ENTRY] = pe007_entry,
    [PE008_ENTRY] = pe008_entry,
    [PE009_ENTRY] = pe009_entry,
    [PE010_ENTRY] = pe010_entry,
    [PE011_ENTRY] = pe011_entry,
    [PE012_ENTRY] = pe012_entry,
    [PE013_ENTRY] = pe013_entry,
    [PE015_ENTRY] = pe015_entry,
    [PE016_ENTRY] = pe016_entry,
    [PE017_ENTRY] = pe017_entry,
    [PE018_ENTRY] = pe018_entry, // used in wrapper.
    [PE019_ENTRY] = pe019_entry,
    [PE020_ENTRY] = pe020_entry,
    [PE021_ENTRY] = pe021_entry,
    [PE022_ENTRY] = pe022_entry,
    [PE023_ENTRY] = pe023_entry,
    [PE024_ENTRY] = pe024_entry,
    [PE025_ENTRY] = pe025_entry,
    [PE026_ENTRY] = pe026_entry,
    [PE027_ENTRY] = pe027_entry,
    [PE028_ENTRY] = pe028_entry,
    [PE029_ENTRY] = pe029_entry,
    [PE030_ENTRY] = pe030_entry,
    [PE031_ENTRY] = pe031_entry,
    [PE032_ENTRY] = pe032_entry,
    [PE033_ENTRY] = pe033_entry,
    [PE034_ENTRY] = pe034_entry,
    [PE035_ENTRY] = pe035_entry,
    [PE036_ENTRY] = pe036_entry,
    [PE037_ENTRY] = pe037_entry,
    [PE038_ENTRY] = pe038_entry,
    [PE039_ENTRY] = pe039_entry,
    [PE040_ENTRY] = pe040_entry,
    [PE041_ENTRY] = pe041_entry,
    [PE042_ENTRY] = pe042_entry,
    [PE043_ENTRY] = pe043_entry,
    [PE044_ENTRY] = pe044_entry,
    [PE045_ENTRY] = pe045_entry,
    [PE046_ENTRY] = pe046_entry,
    [PE047_ENTRY] = pe047_entry,
    [PE048_ENTRY] = pe048_entry,
    [PE049_ENTRY] = pe049_entry,
    [PE051_ENTRY] = pe051_entry,
    [PE052_ENTRY] = pe052_entry,
    [PE053_ENTRY] = pe053_entry,
    [PE054_ENTRY] = pe054_entry,
    [PE057_ENTRY] = pe057_entry,
    [PE058_ENTRY] = pe058_entry,
    [PE059_ENTRY] = pe059_entry,
    [PE060_ENTRY] = pe060_entry,
    [PE061_ENTRY] = pe061_entry,
    [PE062_ENTRY] = pe062_entry,
    [PE063_ENTRY] = pe063_entry,
    [PE064_ENTRY] = pe064_entry,
    [PE065_ENTRY] = pe065_entry,
    [PE066_ENTRY] = pe066_entry,
    [PE067_ENTRY] = pe067_entry,
    [PE068_ENTRY] = pe068_entry,
/* The following test entries are excluded from compilation for the BSA DT UEFI App, as they are
   not required for the BSA DT build. These tests invoke VAL APIs, which in turn call PAL APIs,
   and PAL_DT lacks a few necessary implementations.*/
#ifndef BSA_DT_BUILD
    [P027_ENTRY] = p027_entry, // used in wrapper.
    [MPAM003_ENTRY] = mpam003_entry,
    [PMU001_ENTRY] = pmu001_entry,
    [PMU002_ENTRY] = pmu002_entry,
    [PMU003_ENTRY] = pmu003_entry,
    [PMU004_ENTRY] = pmu004_entry,
    [PMU005_ENTRY] = pmu005_entry,
    [PMU006_ENTRY] = pmu006_entry,
    [PMU007_ENTRY] = pmu007_entry,
    [PMU009_ENTRY] = pmu009_entry,
    [PMU010_ENTRY] = pmu010_entry,
    [PMU011_ENTRY] = pmu011_entry,
    [RAS001_ENTRY] = ras001_entry,
    [RAS002_ENTRY] = ras002_entry,
    [RAS003_ENTRY] = ras003_entry,
    [RAS004_ENTRY] = ras004_entry,
    [RAS005_ENTRY] = ras005_entry,
    [RAS006_ENTRY] = ras006_entry,
    [RAS007_ENTRY] = ras007_entry,
    [RAS008_ENTRY] = ras008_entry,
    [RAS009_ENTRY] = ras009_entry,
    [RAS010_ENTRY] = ras010_entry,
    [RAS011_ENTRY] = ras011_entry, // used in wrapper.
    [RAS012_ENTRY] = ras012_entry, // used in wrapper.
    [RAS013_ENTRY] = ras013_entry,
    [RAS014_ENTRY] = ras014_entry,
    [RAS015_ENTRY] = ras015_entry,
    [SYS_RAS_2_ENTRY]  = sys_ras_2_entry,
    [TPM001_ENTRY] = tpm001_entry,
    [TPM002_ENTRY] = tpm002_entry,
#endif
    [T001_ENTRY] = t001_entry,
    [T002_ENTRY] = t002_entry,
    [T003_ENTRY] = t003_entry,
    [T004_ENTRY] = t004_entry,
    [T005_ENTRY] = t005_entry,
    [T006_ENTRY] = t006_entry,
    [T007_ENTRY] = t007_entry,
    [T008_ENTRY] = t008_entry,
    [U001_ENTRY] = u001_entry, // used in wrapper.
    [U002_ENTRY] = u002_entry, // used in wrapper.
    [U003_ENTRY] = u003_entry, // used in wrapper.
    [U004_ENTRY] = u004_entry, // used in wrapper.
    [U005_ENTRY] = u005_entry, // used in wrapper.
    [V2M001_ENTRY] = v2m001_entry, // used in wrapper.
    [V2M002_ENTRY] = v2m002_entry,
    [V2M003_ENTRY] = v2m003_entry, // used in wrapper.
    [V2M004_ENTRY] = v2m004_entry,
    [W001_ENTRY] = w001_entry,
    [W002_ENTRY] = w002_entry,
    [W003_ENTRY] = w003_entry,
    [V_L1WK_02_05_ENTRY] = v_l1wk_02_05_entry,
    [V_L1PP_00_ENTRY] = v_l1pp_00_entry,
#endif /* TARGET_UEFI */

/* TARGET_BAREMETAL */
#if defined(TARGET_BAREMETAL)
#if defined(BAREMETAL_PCBSA_BUILD)
/* PC_BSA Baremetal build to selected test entries only */
    [PE001_ENTRY] = pe001_entry,
    [PE002_ENTRY] = pe002_entry,
    [PE003_ENTRY] = pe003_entry,
    [PE004_ENTRY] = pe004_entry,
    [PE006_ENTRY] = pe006_entry,
    [PE007_ENTRY] = pe007_entry,
    [PE008_ENTRY] = pe008_entry,
    [PE009_ENTRY] = pe009_entry,
    [PE010_ENTRY] = pe010_entry,
    [PE011_ENTRY] = pe011_entry,
    [PE012_ENTRY] = pe012_entry,
    [PE013_ENTRY] = pe013_entry,
    [PE015_ENTRY] = pe015_entry,
    [PE016_ENTRY] = pe016_entry,
    [PE017_ENTRY] = pe017_entry,
    [PE018_ENTRY] = pe018_entry,
    [PE019_ENTRY] = pe019_entry,
    [PE020_ENTRY] = pe020_entry,
    [PE021_ENTRY] = pe021_entry,
    [PE022_ENTRY] = pe022_entry,
    [PE024_ENTRY] = pe024_entry,
    [PE025_ENTRY] = pe025_entry,
    [PE026_ENTRY] = pe026_entry,
    [PE028_ENTRY] = pe028_entry,
    [PE029_ENTRY] = pe029_entry,
    [PE030_ENTRY] = pe030_entry,
    [PE063_ENTRY] = pe063_entry,
    [P_L1PE_01_ENTRY] = p_l1pe_01_entry,
    [G001_ENTRY] = g001_entry,
    [G002_ENTRY] = g002_entry,
    [G003_ENTRY] = g003_entry,
    [G004_ENTRY] = g004_entry,
    [G005_ENTRY] = g005_entry,
    [G012_ENTRY] = g012_entry,
    [G014_ENTRY] = g014_entry,
    [B_PPI_00_ENTRY] = b_ppi_00_entry,
    [M001_ENTRY] = m001_entry,
    [M002_ENTRY] = m002_entry,
    [M003_ENTRY] = m003_entry,
    [M004_ENTRY] = m004_entry,
    [M005_ENTRY] = m005_entry,
    [M007_ENTRY] = m007_entry,
    [M008_ENTRY] = m008_entry,
    [I001_ENTRY] = i001_entry,
    [I002_ENTRY] = i002_entry,
    [I003_ENTRY] = i003_entry,
    [I004_ENTRY] = i004_entry,
    [I005_ENTRY] = i005_entry,
    [I006_ENTRY] = i006_entry,
    [I007_ENTRY] = i007_entry,
    [I008_ENTRY] = i008_entry,
    [I020_ENTRY] = i020_entry,
    [I022_ENTRY] = i022_entry,
    [I025_ENTRY] = i025_entry,
    [I029_ENTRY] = i029_entry,
    [T001_ENTRY] = t001_entry,
    [T002_ENTRY] = t002_entry,
    [T003_ENTRY] = t003_entry,
    [T004_ENTRY] = t004_entry,
    [T005_ENTRY] = t005_entry,
    [T007_ENTRY] = t007_entry,
    [W001_ENTRY] = w001_entry,
    [W002_ENTRY] = w002_entry,
    [D001_ENTRY] = d001_entry,
    [D002_ENTRY] = d002_entry,
    [D003_ENTRY] = d003_entry,
    [D004_ENTRY] = d004_entry,
    [D006_ENTRY] = d006_entry,
    [D007_ENTRY] = d007_entry,
    [D008_ENTRY] = d008_entry,
    [P001_ENTRY] = p001_entry,
    [P002_ENTRY] = p002_entry,
    [P003_ENTRY] = p003_entry, // used in wrapper.
    [P004_ENTRY] = p004_entry, // used in wrapper.
    [P005_ENTRY] = p005_entry, // used in wrapper.
    [P006_ENTRY] = p006_entry, // used in wrapper.
    [P008_ENTRY] = p008_entry,
    [P009_ENTRY] = p009_entry,
    [P011_ENTRY] = p011_entry,
    [P017_ENTRY] = p017_entry,
    [P018_ENTRY] = p018_entry,
    [P019_ENTRY] = p019_entry,
    [P020_ENTRY] = p020_entry, // used in wrapper.
    [P021_ENTRY] = p021_entry,
    [P022_ENTRY] = p022_entry, // used in wrapper.
    [P023_ENTRY] = p023_entry, // used in wrapper.
    [P024_ENTRY] = p024_entry, // used in wrapper.
    [P025_ENTRY] = p025_entry, // used in wrapper.
    [P026_ENTRY] = p026_entry, // used in wrapper.
    [P027_ENTRY] = p027_entry, // used in wrapper.
    [P033_ENTRY] = p033_entry, // used in wrapper.
    [P035_ENTRY] = p035_entry,
    [P036_ENTRY] = p036_entry, // used in wrapper.
    [P037_ENTRY] = p037_entry,
    [P038_ENTRY] = p038_entry,
    [P039_ENTRY] = p039_entry,
    [P042_ENTRY] = p042_entry,
    [P045_ENTRY] = p045_entry, // used in wrapper.
    [P046_ENTRY] = p046_entry,
    [P047_ENTRY] = p047_entry,
    [P064_ENTRY] = p064_entry, // used in wrapper.
    [P068_ENTRY] = p068_entry, // used in wrapper.
    [P071_ENTRY] = p071_entry, // used in wrapper.
    [P072_ENTRY] = p072_entry, // used in wrapper.
    [P078_ENTRY] = p078_entry, // used in wrapper.
    [P095_ENTRY] = p095_entry,
    [P096_ENTRY] = p096_entry, // used in wrapper.
    [P097_ENTRY] = p097_entry, // used in wrapper.
    [P087_ENTRY] = p087_entry,
    [P094_ENTRY] = p094_entry, // used in wrapper.
    [P103_ENTRY] = p103_entry, // used in wrapper.
    [P104_ENTRY] = p104_entry, // used in wrapper.
    [P105_ENTRY] = p105_entry,
    [PCI_IC_11_ENTRY] = pci_ic_11_entry,
    [PCI_IN_04_ENTRY] = pci_in_04_entry,
    [PCI_IN_05_ENTRY] = pci_in_05_entry,
    [PCI_IN_13_ENTRY] = pci_in_13_entry,
    [PCI_IN_17_ENTRY] = pci_in_17_entry,
    [PCI_IN_19_ENTRY] = pci_in_19_entry,
    [PCI_LI_01_ENTRY] = pci_li_01_entry,
    [PCI_LI_02_ENTRY] = pci_li_02_entry,
    [PCI_LI_03_ENTRY] = pci_li_03_entry,
    [PCI_MM_01_ENTRY] = pci_mm_01_entry,
    [PCI_MM_03_ENTRY] = pci_mm_03_entry,
    [PCI_MSI_2_ENTRY] = pci_msi_2_entry,
    [PCI_PP_04_ENTRY] = pci_pp_04_entry,
    [PCI_PP_05_ENTRY] = pci_pp_05_entry,
    [E001_ENTRY] = e001_entry,
    [E002_ENTRY] = e002_entry,
    [E003_ENTRY] = e003_entry,
    [E006_ENTRY] = e006_entry,
    [E007_ENTRY] = e007_entry,
    [E010_ENTRY] = e010_entry,
    [E014_ENTRY] = e014_entry,
    [E015_ENTRY] = e015_entry, // used in wrapper.
    [E016_ENTRY] = e016_entry, // used in wrapper.
    [E017_ENTRY] = e017_entry, // used in wrapper.
    [E033_ENTRY] = e033_entry, // used in wrapper.
    [E039_ENTRY] = e039_entry, // used in wrapper.
    [B_WAK_03_07_ENTRY] = b_wak_03_07_entry,
    [TPM001_ENTRY] = tpm001_entry,
    [TPM002_ENTRY] = tpm002_entry,
    [INTERFACE010_ENTRY] = interface010_entry,
    [INTERFACE011_ENTRY] = interface011_entry,
    [U001_ENTRY] = u001_entry, // used in wrapper.
    [U002_ENTRY] = u002_entry, // used in wrapper.
    [U003_ENTRY] = u003_entry, // used in wrapper.
    [U004_ENTRY] = u004_entry, // used in wrapper.
    [U005_ENTRY] = u005_entry, // used in wrapper.
    [G006_ENTRY] = g006_entry, // used in wrapper.
    [G007_ENTRY] = g007_entry, // used in wrapper.
    [G009_ENTRY] = g009_entry, // used in wrapper.
    [G010_ENTRY] = g010_entry, // used in wrapper.
    [G011_ENTRY] = g011_entry, // used in wrapper.
#else
/* Drop compiling entries not required for Bare Metal BSA compliance */
#ifndef BAREMETAL_BSA_BUILD
    [G012_ENTRY] = g012_entry,
    [G013_ENTRY] = g013_entry,
    [G016_ENTRY] = g016_entry,
    [G014_ENTRY] = g014_entry,
    [G015_ENTRY] = g015_entry,
    [PE058_ENTRY] = pe058_entry,
    [PE061_ENTRY] = pe061_entry,
    [PE032_ENTRY] = pe032_entry,
    [PE062_ENTRY] = pe062_entry,
    [PE025_ENTRY] = pe025_entry,
    [PE048_ENTRY] = pe048_entry,
    [PE045_ENTRY] = pe045_entry,
    [PE039_ENTRY] = pe039_entry,
    [PE047_ENTRY] = pe047_entry,
    [PE024_ENTRY] = pe024_entry,
    [PE030_ENTRY] = pe030_entry,
    [PE031_ENTRY] = pe031_entry,
    [PE037_ENTRY] = pe037_entry,
    [PE057_ENTRY] = pe057_entry,
    [PE038_ENTRY] = pe038_entry,
    [PE046_ENTRY] = pe046_entry,
    [PE035_ENTRY] = pe035_entry,
    [PE028_ENTRY] = pe028_entry,
    [PE060_ENTRY] = pe060_entry,
    [PE044_ENTRY] = pe044_entry,
    [PE043_ENTRY] = pe043_entry,
    [PE052_ENTRY] = pe052_entry,
    [PE033_ENTRY] = pe033_entry,
    [PE026_ENTRY] = pe026_entry,
    [PE054_ENTRY] = pe054_entry,
    [PE027_ENTRY] = pe027_entry,
    [PE041_ENTRY] = pe041_entry,
    [PE053_ENTRY] = pe053_entry,
    [PE036_ENTRY] = pe036_entry,
    [PE023_ENTRY] = pe023_entry,
    [PE049_ENTRY] = pe049_entry,
    [PE051_ENTRY] = pe051_entry,
    [PE034_ENTRY] = pe034_entry,
    [PE029_ENTRY] = pe029_entry,
    [PE042_ENTRY] = pe042_entry,
    [PE059_ENTRY] = pe059_entry,
    [PE040_ENTRY] = pe040_entry,
    [PE064_ENTRY] = pe064_entry,
    [M005_ENTRY] = m005_entry,
    [M008_ENTRY] = m008_entry,
    [T006_ENTRY] = t006_entry,
    [ETE008_ENTRY] = ete008_entry,
    [ETE007_ENTRY] = ete007_entry,
    [ETE002_ENTRY] = ete002_entry,
    [ETE001_ENTRY] = ete001_entry,
    [ETE006_ENTRY] = ete006_entry,
    [ETE003_ENTRY] = ete003_entry,
    [ETE009_ENTRY] = ete009_entry,
    [ETE004_ENTRY] = ete004_entry,
    [ETE005_ENTRY] = ete005_entry,
    [RAS001_ENTRY] = ras001_entry,
    [RAS007_ENTRY] = ras007_entry,
    [RAS010_ENTRY] = ras010_entry,
    [RAS013_ENTRY] = ras013_entry,
    [RAS003_ENTRY] = ras003_entry,
    [RAS009_ENTRY] = ras009_entry,
    [RAS002_ENTRY] = ras002_entry,
    [RAS004_ENTRY] = ras004_entry,
    [RAS011_ENTRY] = ras011_entry, // used in wrapper.
    [RAS015_ENTRY] = ras015_entry,
    [RAS006_ENTRY] = ras006_entry,
    [RAS012_ENTRY] = ras012_entry, // used in wrapper.
    [RAS008_ENTRY] = ras008_entry,
    [RAS014_ENTRY] = ras014_entry,
    [RAS005_ENTRY] = ras005_entry,
    [E027_ENTRY] = e027_entry,
    [E019_ENTRY] = e019_entry, // used in wrapper.
    [E026_ENTRY] = e026_entry,
    [E032_ENTRY] = e032_entry,
    [E008_ENTRY] = e008_entry,
    [E038_ENTRY] = e038_entry,
    [E020_ENTRY] = e020_entry, // used in wrapper.
    [E025_ENTRY] = e025_entry,
    [E021_ENTRY] = e021_entry,
    [E029_ENTRY] = e029_entry,
    [E024_ENTRY] = e024_entry,
    [E036_ENTRY] = e036_entry,
    [E028_ENTRY] = e028_entry,
    [E022_ENTRY] = e022_entry,
    [E030_ENTRY] = e030_entry,
    [E023_ENTRY] = e023_entry,
    [MPAM006_ENTRY] = mpam006_entry, // used in wrapper.
    [MPAM005_ENTRY] = mpam005_entry,
    [MPAM001_ENTRY] = mpam001_entry,
    [MPAM008_ENTRY] = mpam008_entry,
    [MPAM003_ENTRY] = mpam003_entry,
    [MPAM007_ENTRY] = mpam007_entry, // used in wrapper.
    [MPAM002_ENTRY] = mpam002_entry, // used in wrapper.
    [MPAM009_ENTRY] = mpam009_entry,
    [W003_ENTRY] = w003_entry,
    [I011_ENTRY] = i011_entry,
    [I008_ENTRY] = i008_entry,
    [I025_ENTRY] = i025_entry,
    [I009_ENTRY] = i009_entry,
    [I026_ENTRY] = i026_entry,
    [I028_ENTRY] = i028_entry,
    [I018_ENTRY] = i018_entry,
    [I010_ENTRY] = i010_entry,
    [I030_ENTRY] = i030_entry,
    [I017_ENTRY] = i017_entry,
    [I024_ENTRY] = i024_entry, // used in wrapper
    [I012_ENTRY] = i012_entry,
    [I014_ENTRY] = i014_entry,
    [I020_ENTRY] = i020_entry,
    [I021_ENTRY] = i021_entry,
    [I027_ENTRY] = i027_entry,
    [I019_ENTRY] = i019_entry,
    [I015_ENTRY] = i015_entry,
    [I016_ENTRY] = i016_entry,
    [I023_ENTRY] = i023_entry,
    [I022_ENTRY] = i022_entry,
    [I013_ENTRY] = i013_entry,
    [P085_ENTRY] = p085_entry,
    [P050_ENTRY] = p050_entry, // used in wrapper.
    [P048_ENTRY] = p048_entry, // used in wrapper.
    [P093_ENTRY] = p093_entry,
    [P054_ENTRY] = p054_entry, // used in wrapper.
    [P082_ENTRY] = p082_entry,
    [P015_ENTRY] = p015_entry,
    [P016_ENTRY] = p016_entry,
    [P071_ENTRY] = p071_entry, // used in wrapper.
    [P049_ENTRY] = p049_entry, // used in wrapper.
    [P053_ENTRY] = p053_entry, // used in wrapper.
    [P088_ENTRY] = p088_entry, // used in wrapper.
    [P052_ENTRY] = p052_entry,
    [P064_ENTRY] = p064_entry, // used in wrapper.
    [P010_ENTRY] = p010_entry,
    [P066_ENTRY] = p066_entry, // used in wrapper.
    [P090_ENTRY] = p090_entry,
    [P065_ENTRY] = p065_entry, // used in wrapper.
    [P089_ENTRY] = p089_entry, // used in wrapper.
    [P062_ENTRY] = p062_entry,
    [P027_ENTRY] = p027_entry, // used in wrapper.
    [P078_ENTRY] = p078_entry, // used in wrapper.
    [P058_ENTRY] = p058_entry, // used in wrapper.
    [P059_ENTRY] = p059_entry, // used in wrapper.
    [P060_ENTRY] = p060_entry, // used in wrapper.
    [P063_ENTRY] = p063_entry,
    [P091_ENTRY] = p091_entry,
    [P084_ENTRY] = p084_entry,
    [P061_ENTRY] = p061_entry, // used in wrapper.
    [P098_ENTRY] = p098_entry, // used in wrapper.
    [P099_ENTRY] = p099_entry, // used in wrapper.
    [P081_ENTRY] = p081_entry,
    [P068_ENTRY] = p068_entry, // used in wrapper.
    [P007_ENTRY] = p007_entry,
    [P067_ENTRY] = p067_entry, // used in wrapper.
    [P079_ENTRY] = p079_entry,
    [P092_ENTRY] = p092_entry,
    [P012_ENTRY] = p012_entry,
    [P013_ENTRY] = p013_entry,
    [P014_ENTRY] = p014_entry,
    [P070_ENTRY] = p070_entry,
    [P034_ENTRY] = p034_entry,
    [P051_ENTRY] = p051_entry, // used in wrapper.
    [P072_ENTRY] = p072_entry, // used in wrapper.
    [P056_ENTRY] = p056_entry,
    [P057_ENTRY] = p057_entry,
    [P087_ENTRY] = p087_entry,
    [P069_ENTRY] = p069_entry,
    [P080_ENTRY] = p080_entry,
    [P086_ENTRY] = p086_entry,
    [P083_ENTRY] = p083_entry,
    [P046_ENTRY] = p046_entry,
    [PMU003_ENTRY] = pmu003_entry,
    [PMU009_ENTRY] = pmu009_entry,
    [PMU006_ENTRY] = pmu006_entry,
    [PMU004_ENTRY] = pmu004_entry,
    [PMU005_ENTRY] = pmu005_entry,
    [PMU002_ENTRY] = pmu002_entry,
    [PMU007_ENTRY] = pmu007_entry,
    [PMU010_ENTRY] = pmu010_entry,
    [PMU011_ENTRY] = pmu011_entry,
    [PMU001_ENTRY] = pmu001_entry,
#endif /* BAREMETAL_BSA_BUILD */
    [P047_ENTRY] = p047_entry,
    [G001_ENTRY] = g001_entry,
    [G004_ENTRY] = g004_entry,
    [V2M002_ENTRY] = v2m002_entry,
    [P_L1PE_01_ENTRY]  = p_l1pe_01_entry,
    [B_PPI_00_ENTRY]   = b_ppi_00_entry,
    [B_WAK_03_07_ENTRY] = b_wak_03_07_entry,
    [S_L7MP_03_ENTRY]  = s_l7mp_03_entry,
    [SYS_RAS_2_ENTRY]  = sys_ras_2_entry,
    [APPENDIX_I_6_ENTRY] = appendix_i_6_entry,
    [IE_REG_1_ENTRY]   = ie_reg_1_entry,
    [IE_REG_2_ENTRY]   = ie_reg_2_entry,
    [IE_REG_3_ENTRY]   = ie_reg_3_entry,
    [IE_REG_4_ENTRY]   = ie_reg_4_entry,
    [PCI_IC_11_ENTRY]  = pci_ic_11_entry,
    [PCI_IN_04_ENTRY]  = pci_in_04_entry,
    [PCI_IN_05_ENTRY]  = pci_in_05_entry,
    [PCI_IN_13_ENTRY]  = pci_in_13_entry,
    [PCI_IN_17_ENTRY]  = pci_in_17_entry,
    [PCI_IN_19_ENTRY]  = pci_in_19_entry,
    [PCI_LI_01_ENTRY]  = pci_li_01_entry,
    [PCI_LI_02_ENTRY]  = pci_li_02_entry,
    [PCI_LI_03_ENTRY]  = pci_li_03_entry,
    [PCI_MM_01_ENTRY]  = pci_mm_01_entry,
    [PCI_MM_03_ENTRY]  = pci_mm_03_entry,
    [PCI_MSI_2_ENTRY]  = pci_msi_2_entry,
    [PCI_PP_04_ENTRY]  = pci_pp_04_entry,
    [PCI_PP_05_ENTRY]  = pci_pp_05_entry,
    [RE_REC_1_ENTRY]   = re_rec_1_entry,
    [RE_REG_1_ENTRY]   = re_reg_1_entry,
    [RI_SMU_1_ENTRY]   = ri_smu_1_entry,
    [G002_ENTRY] = g002_entry,
    [ITS002_ENTRY] = its002_entry,
    [ITS005_ENTRY] = its005_entry,
    [ITS001_ENTRY] = its001_entry,
    [G005_ENTRY] = g005_entry,
    [G006_ENTRY] = g006_entry, // used in wrapper.
    [V2M003_ENTRY] = v2m003_entry, // used in wrapper.
    [ITS004_ENTRY] = its004_entry,
    [ITS003_ENTRY] = its003_entry,
    [G010_ENTRY] = g010_entry,
    [V2M001_ENTRY] = v2m001_entry, // used in wrapper.
    // [G008_ENTRY] = g008_entry, //unused testcase
    [G007_ENTRY] = g007_entry, // used in wrapper.
    [V2M004_ENTRY] = v2m004_entry,
    [G011_ENTRY] = g011_entry, // used in wrapper.
    [G003_ENTRY] = g003_entry,
    [G009_ENTRY] = g009_entry, // used in wrapper.
    [PE017_ENTRY] = pe017_entry,
    [PE006_ENTRY] = pe006_entry,
    [PE011_ENTRY] = pe011_entry,
    [PE018_ENTRY] = pe018_entry,
    [PE019_ENTRY] = pe019_entry,
    [PE007_ENTRY] = pe007_entry,
    [PE001_ENTRY] = pe001_entry,
    [PE008_ENTRY] = pe008_entry,
    [PE004_ENTRY] = pe004_entry,
    [PE010_ENTRY] = pe010_entry,
    [PE013_ENTRY] = pe013_entry,
    [PE002_ENTRY] = pe002_entry,
    [PE015_ENTRY] = pe015_entry,
    [PE012_ENTRY] = pe012_entry,
    [PE020_ENTRY] = pe020_entry,
    [PE021_ENTRY] = pe021_entry,
    [PE003_ENTRY] = pe003_entry,
    [PE016_ENTRY] = pe016_entry,
    [PE022_ENTRY] = pe022_entry,
    [PE063_ENTRY] = pe063_entry,
    [PE009_ENTRY] = pe009_entry,
    [M001_ENTRY] = m001_entry,
    [M004_ENTRY] = m004_entry,
    [M006_ENTRY] = m006_entry,
    [M007_ENTRY] = m007_entry,
    [M002_ENTRY] = m002_entry,
    [M003_ENTRY] = m003_entry,
    [T005_ENTRY] = t005_entry,
    [T003_ENTRY] = t003_entry,
    [T004_ENTRY] = t004_entry,
    [T001_ENTRY] = t001_entry,
    [T007_ENTRY] = t007_entry,
    [T002_ENTRY] = t002_entry,
    [E039_ENTRY] = e039_entry, // used in wrapper.
    [E035_ENTRY] = e035_entry,
    [E013_ENTRY] = e013_entry,
    [E003_ENTRY] = e003_entry,
    [E002_ENTRY] = e002_entry, // used in wrapper.
    [E012_ENTRY] = e012_entry,
    [E010_ENTRY] = e010_entry,
    [E014_ENTRY] = e014_entry,
    [E017_ENTRY] = e017_entry, // used in wrapper.
    [E034_ENTRY] = e034_entry, // used in wrapper.
    [E011_ENTRY] = e011_entry,
    [E007_ENTRY] = e007_entry, // used in wrapper.
    [E016_ENTRY] = e016_entry, // used in wrapper.
    [E004_ENTRY] = e004_entry,
    [E033_ENTRY] = e033_entry, // used in wrapper.
    [E006_ENTRY] = e006_entry, // used in wrapper.
    [E015_ENTRY] = e015_entry, // used in wrapper.
    [E001_ENTRY] = e001_entry, // used in wrapper.
    [U005_ENTRY] = u005_entry, // used in wrapper.
    [U004_ENTRY] = u004_entry, // used in wrapper.
    [U003_ENTRY] = u003_entry, // used in wrapper.
    [U002_ENTRY] = u002_entry, // used in wrapper.
    [U001_ENTRY] = u001_entry, // used in wrapper.
    [D003_ENTRY] = d003_entry,
    [D006_ENTRY] = d006_entry,
    [D002_ENTRY] = d002_entry,
    [D004_ENTRY] = d004_entry,
    [D007_ENTRY] = d007_entry,
    [D001_ENTRY] = d001_entry,
    [D008_ENTRY] = d008_entry,
    [W001_ENTRY] = w001_entry,
    [W002_ENTRY] = w002_entry,
    [I001_ENTRY] = i001_entry,
    [I006_ENTRY] = i006_entry,
    [I002_ENTRY] = i002_entry,
    [I004_ENTRY] = i004_entry,
    [I003_ENTRY] = i003_entry,
    [I005_ENTRY] = i005_entry,
    [I029_ENTRY] = i029_entry,
    [I007_ENTRY] = i007_entry,
    [P042_ENTRY] = p042_entry,
    [P030_ENTRY] = p030_entry, // used in wrapper.
    [P032_ENTRY] = p032_entry, // used in wrapper.
    [P025_ENTRY] = p025_entry, // used in wrapper.
    [P011_ENTRY] = p011_entry,
    [P035_ENTRY] = p035_entry,
    [P024_ENTRY] = p024_entry, // used in wrapper.
    [P008_ENTRY] = p008_entry,
    [P006_ENTRY] = p006_entry, // used in wrapper.
    [P001_ENTRY] = p001_entry,
    [P003_ENTRY] = p003_entry, // used in wrapper.
    [P039_ENTRY] = p039_entry,
    [P033_ENTRY] = p033_entry, // used in wrapper.
    [P017_ENTRY] = p017_entry, // used in wrapper.
    [P005_ENTRY] = p005_entry, // used in wrapper.
    [P023_ENTRY] = p023_entry, // used in wrapper.
    [P096_ENTRY] = p096_entry, // used in wrapper.
    [P037_ENTRY] = p037_entry,
    [P002_ENTRY] = p002_entry,
    [P018_ENTRY] = p018_entry, // used in wrapper.
    [P094_ENTRY] = p094_entry, // used in wrapper.
    [P104_ENTRY] = p104_entry, // used in wrapper.
    [P009_ENTRY] = p009_entry,
    [P095_ENTRY] = p095_entry,
    [P020_ENTRY] = p020_entry, // used in wrapper.
    [P031_ENTRY] = p031_entry, // used in wrapper.
    [P036_ENTRY] = p036_entry, // used in wrapper.
    [P097_ENTRY] = p097_entry, // used in wrapper.
    [P038_ENTRY] = p038_entry,
    [P019_ENTRY] = p019_entry,
    [P021_ENTRY] = p021_entry,
    [P045_ENTRY] = p045_entry, // used in wrapper.
    [P103_ENTRY] = p103_entry,
    [P022_ENTRY] = p022_entry, // used in wrapper.
    [P004_ENTRY] = p004_entry, // used in wrapper.
    [P026_ENTRY] = p026_entry, // used in wrapper.
    [P100_ENTRY] = p100_entry,
    [P105_ENTRY] = p105_entry,
#endif /* TARGET_PC_BAREMETAL */
#endif /* TARGET_BAREMETAL */
};

#if defined(TARGET_BAREMETAL)
    uint8_t g_current_pal = PLATFORM_BAREMETAL;
#endif
#if defined(TARGET_LINUX)
    uint8_t g_current_pal = PLATFORM_LINUX;
#endif
#if defined(TARGET_UEFI)
    uint8_t g_current_pal = PLATFORM_UEFI;
#endif

/* BSA alias lists */
/* BSA L1 Requirements */
RULE_ID_e bsa_l1_rule_list[] = {
    /* PE L1 */
    B_PE_01, B_PE_02, B_PE_03, B_PE_04, B_PE_05, B_PE_06,
    B_PE_07, B_PE_08, B_PE_09, B_PE_10, B_PE_11, B_PE_12,
    B_PE_13, B_PE_14, B_PE_18, B_PE_19, B_PE_20, B_PE_21,
    B_PE_22, B_PE_23, B_PE_24,

    /* Memory map L1 */
    B_MEM_01, B_MEM_02, B_MEM_03, B_MEM_05, B_MEM_06, B_MEM_07,
    B_MEM_08, B_MEM_09,

    /* Interrupts (GIC, PPI) L1 */
    B_GIC_01, B_GIC_02, B_GIC_03, B_GIC_04, B_GIC_05, B_PPI_00,

    /* SMMU L1 */
    B_SMMU_01, B_SMMU_02, B_SMMU_06, B_SMMU_07, B_SMMU_08, B_SMMU_12,
    B_SMMU_16, B_SMMU_17, B_SMMU_18, B_SMMU_19,
    /*B_SMMU_21,*/
        SMMU_01, SMMU_02,

    /* Timer L1 */
    B_TIME_01, B_TIME_02, B_TIME_03, B_TIME_04, B_TIME_05,
    B_TIME_06, B_TIME_07, B_TIME_08, B_TIME_09, B_TIME_10,

    /* Power and wakeup L1 */
    B_WAK_01, B_WAK_02, B_WAK_03, B_WAK_04, B_WAK_05,
    B_WAK_06, B_WAK_07, B_WAK_08, B_WAK_10, B_WAK_11,

    /* Watchdog L1 */
    /* B_WD_00, */
        B_WD_01, B_WD_02, B_WD_03, B_WD_04, B_WD_05,

    /* Peripherals L1 */
    B_PER_01, B_PER_02, B_PER_03, B_PER_04, B_PER_05, B_PER_06,
    B_PER_07, B_PER_09, B_PER_10, B_PER_11, B_PER_12,

    /* B_PER_08, */
        /* BSA Section E */
        /* E.1 - Configuration Space */
        PCI_IN_01, PCI_IN_02, PCI_IN_03, PCI_IN_04,
        PCI_IN_05, PCI_IN_06, PCI_IN_07, PCI_IN_08,
        PCI_IN_09, PCI_IN_10, PCI_IN_11, PCI_IN_12,
        PCI_IN_13, PCI_IN_14, PCI_IN_15, PCI_IN_16,
        PCI_IN_17, PCI_IN_18, PCI_IN_19, PCI_IN_20,
        /* E.2 - PCIe Memory Space */
        PCI_MM_01, PCI_MM_02, PCI_MM_03, PCI_MM_04,
        /* E.3 - PCIe Device View of Memory */
        PCI_MM_05, PCI_MM_06, PCI_MM_07,
        /* E.4 - Message Signaled Interrupts */
        PCI_MSI_1, PCI_MSI_2,
        /* E.6 - Legacy Interrupts */
        PCI_LI_01, PCI_LI_02, PCI_LI_03, PCI_LI_04,
        /* E.7 - System MMU and Device Assignment */
        PCI_SM_01, PCI_SM_02,
        /* E.8 - I/O Coherency */
        PCI_IC_11, PCI_IC_12, PCI_IC_13, PCI_IC_14,
        PCI_IC_15, PCI_IC_16, PCI_IC_17, PCI_IC_18,
        /* E.9 - Legacy I/O */
        PCI_IO_01,
        /* E.10 - Integrated End Points */
        PCI_IEP_1,
        /* E.11 - Peer-to-Peer */
        PCI_PP_01, PCI_PP_02, PCI_PP_03,
        PCI_PP_04, PCI_PP_05, PCI_PP_06,
        /* E.12 - PASID Support */
        PCI_PAS_1,
        /* E.13 - PCIe Precision Time Measurement */
        PCI_PTM_1,

    RULE_ID_SENTINEL
};
/* B_WD_00 */
RULE_ID_e b_wd_00_rule_list[]     = {B_WD_01, B_WD_02, B_WD_03, B_WD_04, B_WD_05,
                                     RULE_ID_SENTINEL};
/* B_REP_1 */
RULE_ID_e b_rep_1_rule_list[] = {
/* BSA Section F.1 - Rules Common for RCiEP and I-EP */
    RI_CRS_1, RI_BAR_1, RI_BAR_2, RI_BAR_3,
    RI_INT_1, RI_ORD_1, RI_ORD_2, RI_ORD_3,
    RI_SMU_1, RI_SMU_2, RI_SMU_3,
    /* RI_SMU_4, refers BSA Section H */
        /* H.1 - ITS Groups */
        ITS_01, ITS_02, ITS_03, ITS_04,
        ITS_05, ITS_06, ITS_07, ITS_08,
        /* H.2 - Generation of DeviceID Values */
        ITS_DEV_1, ITS_DEV_2, ITS_DEV_3,
        ITS_DEV_4, ITS_DEV_5, ITS_DEV_6,
        ITS_DEV_7, ITS_DEV_8, ITS_DEV_9,
    RI_RST_1, RI_PWR_1,

/* BSA Section F.2 - RCiEP */
    /* JKZMT, refers BSA Section E */
        /* E.1 - Configuration Space */
        PCI_IN_01, PCI_IN_02, PCI_IN_03, PCI_IN_04,
        PCI_IN_05, PCI_IN_06, PCI_IN_07, PCI_IN_08,
        PCI_IN_09, PCI_IN_10, PCI_IN_11, PCI_IN_12,
        PCI_IN_13, PCI_IN_14, PCI_IN_15, PCI_IN_16,
        PCI_IN_17, PCI_IN_18, PCI_IN_19, PCI_IN_20,
        /* E.2 - PCIe Memory Space */
        PCI_MM_01, PCI_MM_02, PCI_MM_03, PCI_MM_04,
        /* E.3 - PCIe Device View of Memory */
        PCI_MM_05, PCI_MM_06, PCI_MM_07,
        /* E.4 - Message Signaled Interrupts */
        PCI_MSI_1, PCI_MSI_2,
        /* E.6 - Legacy Interrupts */
        PCI_LI_01, PCI_LI_02, PCI_LI_03, PCI_LI_04,
        /* E.7 - System MMU and Device Assignment */
        PCI_SM_01, PCI_SM_02,
        /* E.8 - I/O Coherency */
        PCI_IC_11, PCI_IC_12, PCI_IC_13, PCI_IC_14,
        PCI_IC_15, PCI_IC_16, PCI_IC_17, PCI_IC_18,
        /* E.9 - Legacy I/O */
        PCI_IO_01,
        /* E.10 - Integrated End Points */
        PCI_IEP_1,
        /* E.11 - Peer-to-Peer */
        PCI_PP_01, PCI_PP_02, PCI_PP_03,
        PCI_PP_04, PCI_PP_05, PCI_PP_06,
        /* E.12 - PASID Support */
        PCI_PAS_1,
        /* E.13 - PCIe Precision Time Measurement */
        PCI_PTM_1,
    RE_PCI_1, RE_PCI_2,
    RE_CFG_1, RE_CFG_2, RE_CFG_3,
    RE_ORD_4,
    RE_PWR_2, RE_PWR_3,
    RE_ACS_1, RE_ACS_2, RE_ACS_3,

/* BSA Section G.1 - RCiEP Capabilities and Registers */
    RE_REG_1, RE_REG_2, RE_REG_3,
    RE_REC_1, RE_REC_2,

    RULE_ID_SENTINEL
};
/* B_IEP_1 */
RULE_ID_e b_iep_1_rule_list[] = {
/* BSA Section F.1 - Rules Common for RCiEP and I-EP */
    RI_CRS_1, RI_BAR_1, RI_BAR_2, RI_BAR_3,
    RI_INT_1, RI_ORD_1, RI_ORD_2, RI_ORD_3,
    RI_SMU_1, RI_SMU_2, RI_SMU_3,
    /* RI_SMU_4, refers BSA Section H */
        /* H.1 - ITS Groups */
        ITS_01, ITS_02, ITS_03, ITS_04,
        ITS_05, ITS_06, ITS_07, ITS_08,
        /* H.2 - Generation of DeviceID Values */
        ITS_DEV_1, ITS_DEV_2, ITS_DEV_3,
        ITS_DEV_4, ITS_DEV_5, ITS_DEV_6,
        ITS_DEV_7, ITS_DEV_8, ITS_DEV_9,
    RI_RST_1, RI_PWR_1,

/* BSA Section  F.3 - I-EP */
    /* HVZJY, refers BSA Section E */
        /* E.1 - Configuration Space */
        PCI_IN_01, PCI_IN_02, PCI_IN_03, PCI_IN_04,
        PCI_IN_05, PCI_IN_06, PCI_IN_07, PCI_IN_08,
        PCI_IN_09, PCI_IN_10, PCI_IN_11, PCI_IN_12,
        PCI_IN_13, PCI_IN_14, PCI_IN_15, PCI_IN_16,
        PCI_IN_17, PCI_IN_18, PCI_IN_19, PCI_IN_20,
        /* E.2 - PCIe Memory Space */
        PCI_MM_01, PCI_MM_02, PCI_MM_03, PCI_MM_04,
        /* E.3 - PCIe Device View of Memory */
        PCI_MM_05, PCI_MM_06, PCI_MM_07,
        /* E.4 - Message Signaled Interrupts */
        PCI_MSI_1, PCI_MSI_2,
        /* E.6 - Legacy Interrupts */
        PCI_LI_01, PCI_LI_02, PCI_LI_03, PCI_LI_04,
        /* E.7 - System MMU and Device Assignment */
        PCI_SM_01, PCI_SM_02,
        /* E.8 - I/O Coherency */
        PCI_IC_11, PCI_IC_12, PCI_IC_13, PCI_IC_14,
        PCI_IC_15, PCI_IC_16, PCI_IC_17, PCI_IC_18,
        /* E.9 - Legacy I/O */
        PCI_IO_01,
        /* E.10 - Integrated End Points */
        PCI_IEP_1,
        /* E.11 - Peer-to-Peer */
        PCI_PP_01, PCI_PP_02, PCI_PP_03,
        PCI_PP_04, PCI_PP_05, PCI_PP_06,
        /* E.12 - PASID Support */
        PCI_PAS_1,
        /* E.13 - PCIe Precision Time Measurement */
        PCI_PTM_1,
    IE_CFG_1, IE_CFG_2,
    /* IE_CFG_3, covered by HVZJY */
    IE_CFG_4,
    IE_ORD_4,
    IE_RST_2, IE_RST_3,
    IE_PWR_2, IE_PWR_3,
    IE_ACS_1, IE_ACS_2,

/* G.2 - I-EP */
    IE_REG_1, IE_REG_2, IE_REG_3,
    IE_REG_4, IE_REG_5, IE_REG_6,
    IE_REG_7, IE_REG_8, IE_REG_9,

    RULE_ID_SENTINEL
};
/* B_PER_08 */
RULE_ID_e b_per_08_rule_list[] = {
    /* BSA Section E */
    /* E.1 - Configuration Space */
    PCI_IN_01, PCI_IN_02, PCI_IN_03, PCI_IN_04,
    PCI_IN_05, PCI_IN_06, PCI_IN_07, PCI_IN_08,
    PCI_IN_09, PCI_IN_10, PCI_IN_11, PCI_IN_12,
    PCI_IN_13, PCI_IN_14, PCI_IN_15, PCI_IN_16,
    PCI_IN_17, PCI_IN_18, PCI_IN_19, PCI_IN_20,
    /* E.2 - PCIe Memory Space */
    PCI_MM_01, PCI_MM_02, PCI_MM_03, PCI_MM_04,
    /* E.3 - PCIe Device View of Memory */
    PCI_MM_05, PCI_MM_06, PCI_MM_07,
    /* E.4 - Message Signaled Interrupts */
    PCI_MSI_1, PCI_MSI_2,
    /* E.6 - Legacy Interrupts */
    PCI_LI_01, PCI_LI_02, PCI_LI_03, PCI_LI_04,
    /* E.7 - System MMU and Device Assignment */
    PCI_SM_01, PCI_SM_02,
    /* E.8 - I/O Coherency */
    PCI_IC_11, PCI_IC_12, PCI_IC_13, PCI_IC_14,
    PCI_IC_15, PCI_IC_16, PCI_IC_17, PCI_IC_18,
    /* E.9 - Legacy I/O */
    PCI_IO_01,
    /* E.10 - Integrated End Points */
    PCI_IEP_1,
    /* E.11 - Peer-to-Peer */
    PCI_PP_01, PCI_PP_02, PCI_PP_03,
    PCI_PP_04, PCI_PP_05, PCI_PP_06,
    /* E.12 - PASID Support */
    PCI_PAS_1,
    /* E.13 - PCIe Precision Time Measurement */
    PCI_PTM_1,

    RULE_ID_SENTINEL
};

/* SBSA alias lists */
/* S_L3PR_01 */
RULE_ID_e s_l3pr_01_rule_list[]   = {B_PER_05, RULE_ID_SENTINEL};
/* S_L3WD_01 */
RULE_ID_e s_l3wd_01_rule_list[]   = {B_WD_01, B_WD_02, B_WD_03, B_WD_04, B_WD_05,
                                     RULE_ID_SENTINEL};
/* S_L6PCI_1 */
RULE_ID_e s_l6pci_1_rule_list[] = {
    /* S_L6PCI_1 refers B_REP_1 */
        /* BSA Section F.1 - Rules Common for RCiEP and I-EP */
        RI_CRS_1, RI_BAR_1, RI_BAR_2, RI_BAR_3,
        RI_INT_1, RI_ORD_1, RI_ORD_2, RI_ORD_3,
        RI_SMU_1, RI_SMU_2, RI_SMU_3,
        /* RI_SMU_4, refers BSA Section H */
            /* H.1 - ITS Groups */
            ITS_01, ITS_02, ITS_03, ITS_04,
            ITS_05, ITS_06, ITS_07, ITS_08,
            /* H.2 - Generation of DeviceID Values */
            ITS_DEV_1, ITS_DEV_2, ITS_DEV_3,
            ITS_DEV_4, ITS_DEV_5, ITS_DEV_6,
            ITS_DEV_7, ITS_DEV_8, ITS_DEV_9,
        RI_RST_1, RI_PWR_1,

        /* BSA Section F.2 - RCiEP */
        /* JKZMT, refers BSA Section E */
            /* E.1 - Configuration Space */
            PCI_IN_01, PCI_IN_02, PCI_IN_03, PCI_IN_04,
            PCI_IN_05, PCI_IN_06, PCI_IN_07, PCI_IN_08,
            PCI_IN_09, PCI_IN_10, PCI_IN_11, PCI_IN_12,
            PCI_IN_13, PCI_IN_14, PCI_IN_15, PCI_IN_16,
            PCI_IN_17, PCI_IN_18, PCI_IN_19, PCI_IN_20,
            /* E.2 - PCIe Memory Space */
            PCI_MM_01, PCI_MM_02, PCI_MM_03, PCI_MM_04,
            /* E.3 - PCIe Device View of Memory */
            PCI_MM_05, PCI_MM_06, PCI_MM_07,
            /* E.4 - Message Signaled Interrupts */
            PCI_MSI_1, PCI_MSI_2,
            /* E.6 - Legacy Interrupts */
            PCI_LI_01, PCI_LI_02, PCI_LI_03, PCI_LI_04,
            /* E.7 - System MMU and Device Assignment */
            PCI_SM_01, PCI_SM_02,
            /* E.8 - I/O Coherency */
            PCI_IC_11, PCI_IC_12, PCI_IC_13, PCI_IC_14,
            PCI_IC_15, PCI_IC_16, PCI_IC_17, PCI_IC_18,
            /* E.9 - Legacy I/O */
            PCI_IO_01,
            /* E.10 - Integrated End Points */
            PCI_IEP_1,
            /* E.11 - Peer-to-Peer */
            PCI_PP_01, PCI_PP_02, PCI_PP_03,
            PCI_PP_04, PCI_PP_05, PCI_PP_06,
            /* E.12 - PASID Support */
            PCI_PAS_1,
            /* E.13 - PCIe Precision Time Measurement */
            PCI_PTM_1,
        RE_PCI_1, RE_PCI_2,
        RE_CFG_1, RE_CFG_2, RE_CFG_3,
        RE_ORD_4,
        RE_PWR_2, RE_PWR_3,
        RE_ACS_1, RE_ACS_2, RE_ACS_3,

        /* BSA Section G.1 - RCiEP Capabilities and Registers */
        RE_REG_1, RE_REG_2, RE_REG_3,
        RE_REC_1, RE_REC_2,

    /* S_L6PCI_1 refers B_IEP_1 */
        /* BSA Section F.1 covered by B_REP_1 for S_L6PCI_1, omitting */

        /* BSA Section  F.3 - I-EP */
        /* HVZJY, refers BSA Section E, but covered by JKZMT for S_L6PCI_1, omitting */
        IE_CFG_1, IE_CFG_2,
        /* IE_CFG_3, covered by HVZJY */
        IE_CFG_4,
        IE_ORD_4,
        IE_RST_2, IE_RST_3,
        IE_PWR_2, IE_PWR_3,
        IE_ACS_1, IE_ACS_2,

        /* G.2 - I-EP */
        IE_REG_1, IE_REG_2, IE_REG_3,
        IE_REG_4, IE_REG_5, IE_REG_6,
        IE_REG_7, IE_REG_8, IE_REG_9,

        RULE_ID_SENTINEL
    };
/* S_L6PE_01 */
RULE_ID_e s_l6pe_01_rule_list[]   = {B_SEC_01, B_SEC_02, B_SEC_03, B_SEC_04, B_SEC_05,
                                     RULE_ID_SENTINEL};
/* LVQBC */
RULE_ID_e lvqbc_rule_list[]   = {ZVDJG, RKLPK, RULE_ID_SENTINEL};

/* SYS_RAS */
RULE_ID_e sys_ras_rule_list[] = {
    /* SBSA Section B Server RAS */
    RAS_01, RAS_02, RAS_03, RAS_04, RAS_05, RAS_06, RAS_07,
    RAS_08, RAS_10, RAS_11, RAS_12,

    RULE_ID_SENTINEL
};

/* PCBSA alias lists */
/* P_L2WD_01 */
RULE_ID_e p_l2wd_01_rule_list[]   = {B_WD_01, B_WD_02, B_WD_03, B_WD_04, B_WD_05,
                                     RULE_ID_SENTINEL};
/* P_L1MM_01 */
RULE_ID_e p_l1mm_01_rule_list[]   = {S_L3MM_01, S_L3MM_02, RULE_ID_SENTINEL};

/* B_SMMU_21 */
RULE_ID_e b_smmu_21_rule_list[]   = {SMMU_01, SMMU_02, RULE_ID_SENTINEL};


/* VBSA alias lists */
RULE_ID_e v_l1pe_01_rule_list[]   = {B_PE_01, B_PE_02, B_PE_03, B_PE_04, B_PE_05,
                                     B_PE_06, B_PE_07, B_PE_08, B_PE_10, B_PE_13,
                                     B_PE_14, RULE_ID_SENTINEL};
RULE_ID_e v_l1mm_01_rule_list[]   = {B_MEM_01, B_MEM_02, B_MEM_04, B_MEM_05,
                                     B_MEM_07, RULE_ID_SENTINEL};
RULE_ID_e v_l1gi_01_rule_list[]   = {B_GIC_01, B_GIC_02, B_GIC_03, B_GIC_05,
                                     RULE_ID_SENTINEL};
RULE_ID_e v_l1sm_01_rule_list[]   = {B_SMMU_01, B_SMMU_02, B_SMMU_06,
                                     B_SMMU_07, B_SMMU_12, RULE_ID_SENTINEL};
RULE_ID_e v_l1pr_01_rule_list[]   = {B_PER_01,  B_PER_02,  B_PER_03,
                                     B_PER_04,  B_PER_05,  B_PER_06,
                                     B_PER_09,  B_PER_10,  B_PER_11,
                                     B_PER_12,  RULE_ID_SENTINEL};
RULE_ID_e v_l1pr_02_rule_list[]   = {
                                     /* E.1 - Configuration Space */
                                     PCI_IN_01, PCI_IN_02, PCI_IN_03, PCI_IN_04,
                                     PCI_IN_05, PCI_IN_06, PCI_IN_07, PCI_IN_08,
                                     PCI_IN_09, PCI_IN_10, PCI_IN_11, PCI_IN_12,
                                     PCI_IN_13, PCI_IN_14, PCI_IN_15, PCI_IN_16,
                                     PCI_IN_17, PCI_IN_18, PCI_IN_19, PCI_IN_20,

                                     /* E.2 - PCI Express memory space (Except PCI_MM_02) */
                                     PCI_MM_01, PCI_MM_03, PCI_MM_04,

                                     /* E.3 - PCI Express deivce view of memory */
                                     PCI_MM_05, PCI_MM_06, PCI_MM_07,

                                     /* E.4 - Message Signaled Interrupts */
                                     PCI_MSI_1, PCI_MSI_2,

                                     /* E.6 - Legacy Interrupts */
                                     PCI_LI_01, PCI_LI_02, PCI_LI_03, PCI_LI_04,

                                     /* E.7 - System MMU and Device Assignment */
                                     PCI_SM_01, PCI_SM_02,

                                     /* E.8 - I/O Coherency */
                                     PCI_IC_11, PCI_IC_12, PCI_IC_13, PCI_IC_14,
                                     PCI_IC_15, PCI_IC_16, PCI_IC_17, PCI_IC_18,

                                     /* E.9 - Legacy I/O */
                                     PCI_IO_01,

                                     /* E.10 - Integrated End Points */
                                     PCI_IEP_1,

                                     /* E.11 - Peer-to-Peer */
                                     PCI_PP_01, PCI_PP_02, PCI_PP_03,
                                     PCI_PP_04, PCI_PP_05, PCI_PP_06,

                                     /* E.12 - PASID Support */
                                     PCI_PAS_1,

                                     /* E.13 - PCIe Precision Time Measurement */
                                     PCI_PTM_1,

                                     RULE_ID_SENTINEL};

// TODO update all alias rules in xbsa specs
alias_rule_map_t alias_rule_map[] = {
    /* BSA alias rules */
    {B_WD_00,   b_wd_00_rule_list},
    {B_PER_08,  b_per_08_rule_list},
    {B_REP_1,   b_rep_1_rule_list},
    {B_IEP_1,   b_iep_1_rule_list},
    {B_SMMU_21, b_smmu_21_rule_list},

    /* SBSA alias rules */
    {S_L3_01,   bsa_l1_rule_list},
    {S_L3PR_01, s_l3pr_01_rule_list},
    {S_L3WD_01, s_l3wd_01_rule_list},
    {S_L6PCI_1, s_l6pci_1_rule_list},
    {S_L6PE_01, s_l6pe_01_rule_list},
    {SYS_RAS,   sys_ras_rule_list},
    {LVQBC,     lvqbc_rule_list},

    /* PCBSA alias rules */
    {P_L1_01,   bsa_l1_rule_list},
    {P_L2WD_01, p_l2wd_01_rule_list},
    {P_L1MM_01, p_l1mm_01_rule_list},

    /* VBSA alias rules */
    {V_L1PE_01, v_l1pe_01_rule_list},
    {V_L1MM_01, v_l1mm_01_rule_list},
    {V_L1GI_01, v_l1gi_01_rule_list},
    {V_L1SM_01, v_l1sm_01_rule_list},
    {V_L1PR_01, v_l1pr_01_rule_list},
    {V_L1PR_02, v_l1pr_02_rule_list},

};

uint32_t alias_rule_map_count = sizeof(alias_rule_map) / sizeof(alias_rule_map[0]);
