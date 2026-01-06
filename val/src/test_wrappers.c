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

/* @brief files contains test wrappers, which wraps multiple test entry functions required by
   single base rule into one single test entry function */

#include "include/acs_val.h"
#include "include/val_interface.h"
#include "include/acs_pcie.h"
#include "include/acs_gic.h"
#include "include/acs_wakeup.h"
#include "include/acs_mpam.h"
#include "include/acs_ras.h"
#include "include/acs_pe.h"
#include "include/acs_exerciser.h"
#include "include/test_wrappers.h"
#include "include/rule_based_execution.h"

extern test_entry_fn_t test_entry_func_table[TEST_ENTRY_SENTINEL];

/* Helper to execute test entries  */
static uint32_t run_test_entries(TEST_ENTRY_ID_e *tst_entry_list, uint32_t num_pe)
{
    uint32_t i;
    uint32_t entry_status = TEST_STATUS_UNKNOWN;
    uint32_t rule_status = TEST_STATUS_UNKNOWN;

    for (i = 0; tst_entry_list[i] != TEST_ENTRY_SENTINEL ; i++) {
        if (test_entry_func_table[tst_entry_list[i]] != NULL) {
            entry_status = test_entry_func_table[tst_entry_list[i]](num_pe);
        } else {
            /* If entry is NULL, then the entry is not supported in current PAL */
            entry_status = TEST_PART_COV;
        }

        /* Update overall status for the rule */
        if ((entry_status > rule_status) || (rule_status == TEST_STATUS_UNKNOWN)) {
            rule_status = entry_status;
        }
    }

    return rule_status;
}

/* PCIe-specific helpers to execute entry ID lists and merge results */
static inline uint32_t max_status(uint32_t a, uint32_t b)
{
    return (a > b) ? a : b;
}

static uint32_t run_pcie_static_and_exerciser(TEST_ENTRY_ID_e *static_list,
                                              TEST_ENTRY_ID_e *exr_list,
                                              uint32_t num_pe)
{
    uint32_t static_status = run_test_entries(static_list, num_pe);
    uint32_t exr_status    = run_test_entries(exr_list,   num_pe);

    /* Report partial coverage if PCIe static tests PASS and excerciser tests SKIP */
    if (static_status == TEST_PASS && exr_status == TEST_SKIP)
        return TEST_PART_COV;

    return max_status(static_status, exr_status);
}

/* B_PPI_00 */
uint32_t
b_ppi_00_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e tst_entry_list[] = {G006_ENTRY, G007_ENTRY, G009_ENTRY,
                                        G010_ENTRY, G011_ENTRY, TEST_ENTRY_SENTINEL};

    return run_test_entries(tst_entry_list, num_pe);
}

/* B_WAK_03, B_WAK_07 */
uint32_t
b_wak_03_07_entry(uint32_t num_pe)
{
    /* TODO: U006_ENTRY needs multi-PE interrupt handling support says comment in
       bsa_executes_tests.c */
    TEST_ENTRY_ID_e tst_entry_list[] = {U001_ENTRY, U002_ENTRY, U003_ENTRY,
                                        U004_ENTRY, U005_ENTRY,
                                        /* U006_ENTRY, */
                                        TEST_ENTRY_SENTINEL};

    return run_test_entries(tst_entry_list, num_pe);
}

/* S_L7MP_03 */
uint32_t
s_l7mp_03_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e tst_entry_list[] = {MPAM002_ENTRY, MPAM006_ENTRY, MPAM007_ENTRY,
                                        TEST_ENTRY_SENTINEL};

    return run_test_entries(tst_entry_list, num_pe);
}

/* SYS_RAS_2 */
uint32_t
sys_ras_2_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e tst_entry_list[] = {RAS011_ENTRY, RAS012_ENTRY, TEST_ENTRY_SENTINEL};

    return run_test_entries(tst_entry_list, num_pe);
}

/* Appendix I.6 */
uint32_t
appendix_i_6_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e tst_entry_list[] = {V2M001_ENTRY, V2M003_ENTRY, TEST_ENTRY_SENTINEL};

    return run_test_entries(tst_entry_list, num_pe);
}

/* P_L1PE_01 */
uint32_t
p_l1pe_01_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e tst_entry_list[] = {PE004_ENTRY, PE018_ENTRY, TEST_ENTRY_SENTINEL};

    return run_test_entries(tst_entry_list, num_pe);
}

/* IE_REG_1 */
uint32_t
ie_reg_1_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e tst_entry_list[] = {P049_ENTRY, P059_ENTRY, TEST_ENTRY_SENTINEL};

   return run_test_entries(tst_entry_list, num_pe);
}

/* PCI_IC_11 */
uint32_t
pci_ic_11_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e p_list[] = { P068_ENTRY, TEST_ENTRY_SENTINEL };
    TEST_ENTRY_ID_e e_list[] = { E007_ENTRY, TEST_ENTRY_SENTINEL };

    return run_pcie_static_and_exerciser(p_list, e_list, num_pe);
}

/* PCI_IN_04 */
uint32_t
pci_in_04_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e tst_entry_list[] = {P003_ENTRY, P072_ENTRY, TEST_ENTRY_SENTINEL};

    return run_test_entries(tst_entry_list, num_pe);
}

/* PCI_LI_02 */
uint32_t
pci_li_02_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e p_list[] = { P096_ENTRY, TEST_ENTRY_SENTINEL };
    TEST_ENTRY_ID_e e_list[] = { E006_ENTRY, TEST_ENTRY_SENTINEL };

    return run_pcie_static_and_exerciser(p_list, e_list, num_pe);
}

/* PCI_LI_03 */
uint32_t
pci_li_03_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e tst_entry_list[] = {P023_ENTRY, P078_ENTRY, TEST_ENTRY_SENTINEL};

    return run_test_entries(tst_entry_list, num_pe);
}

/* PCI_MSI_2 */
uint32_t
pci_msi_2_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e p_list[] = { P097_ENTRY, TEST_ENTRY_SENTINEL };
    TEST_ENTRY_ID_e e_list[] = { E033_ENTRY, TEST_ENTRY_SENTINEL };

    return run_pcie_static_and_exerciser(p_list, e_list, num_pe);
}

/* PCI_PP_04 */
uint32_t
pci_pp_04_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e tst_entry_list[] = {P018_ENTRY, E001_ENTRY, E002_ENTRY, TEST_ENTRY_SENTINEL};

    return run_test_entries(tst_entry_list, num_pe);
}

/* PCI_PP_05 */
uint32_t
pci_pp_05_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e tst_entry_list[] = {P017_ENTRY, TEST_ENTRY_SENTINEL};

    return run_test_entries(tst_entry_list, num_pe);
}

/* RE_REC_1 */
uint32_t
re_rec_1_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e tst_entry_list[] = {P053_ENTRY, P061_ENTRY, TEST_ENTRY_SENTINEL};

    return run_test_entries(tst_entry_list, num_pe);
}

/* RE_REG_1 */
uint32_t
re_reg_1_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e tst_entry_list[] = {P048_ENTRY, P058_ENTRY, TEST_ENTRY_SENTINEL};

    return run_test_entries(tst_entry_list, num_pe);
}

/* IE_REG_3 */
uint32_t
ie_reg_3_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e p_list[] = { P050_ENTRY, P060_ENTRY, TEST_ENTRY_SENTINEL };
    TEST_ENTRY_ID_e e_list[] = { E034_ENTRY, TEST_ENTRY_SENTINEL };

    return run_pcie_static_and_exerciser(p_list, e_list, num_pe);
}

/* PCI_IN_19 */
uint32_t
pci_in_19_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e tst_entry_list[] = {P030_ENTRY, P031_ENTRY, P032_ENTRY,
                                        TEST_ENTRY_SENTINEL};

    return run_test_entries(tst_entry_list, num_pe);
}

/* PCI_LI_01 */
uint32_t
pci_li_01_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e tst_entry_list[] = {P006_ENTRY, P027_ENTRY, TEST_ENTRY_SENTINEL};

    return run_test_entries(tst_entry_list, num_pe);
}

/* PCI_MM_01 */
uint32_t
pci_mm_01_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e p_list[] = { P045_ENTRY, P103_ENTRY, TEST_ENTRY_SENTINEL };
    TEST_ENTRY_ID_e e_list[] = { E016_ENTRY, TEST_ENTRY_SENTINEL };

    return run_pcie_static_and_exerciser(p_list, e_list, num_pe);
}

/* PCI_MM_03 */
uint32_t
pci_mm_03_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e p_list[] = { P094_ENTRY, P104_ENTRY, TEST_ENTRY_SENTINEL };
    TEST_ENTRY_ID_e e_list[] = { E039_ENTRY, TEST_ENTRY_SENTINEL };

    return run_pcie_static_and_exerciser(p_list, e_list, num_pe);
}

/* RI_SMU_1 */
uint32_t
ri_smu_1_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e p_list[] = { P080_ENTRY, TEST_ENTRY_SENTINEL };
    TEST_ENTRY_ID_e e_list[] = { E019_ENTRY, E020_ENTRY, TEST_ENTRY_SENTINEL };

    return run_pcie_static_and_exerciser(p_list, e_list, num_pe);
}

/* IE_REG_2 */
uint32_t
ie_reg_2_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e tst_entry_list[] = {P054_ENTRY, P098_ENTRY, P065_ENTRY, P067_ENTRY,
                                        TEST_ENTRY_SENTINEL};

    return run_test_entries(tst_entry_list, num_pe);
}

/* IE_REG_4 */
uint32_t
ie_reg_4_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e tst_entry_list[] = {P051_ENTRY, P099_ENTRY, P066_ENTRY, P088_ENTRY,
                                        P064_ENTRY, TEST_ENTRY_SENTINEL};

    return run_test_entries(tst_entry_list, num_pe);
}

/* PCI_IN_13 */
uint32_t
pci_in_13_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e tst_entry_list[] = {P004_ENTRY, P005_ENTRY, TEST_ENTRY_SENTINEL};

    return run_test_entries(tst_entry_list, num_pe);
}

/* PCI_IN_17 */
uint32_t
pci_in_17_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e p_list[] = { P036_ENTRY, P071_ENTRY, TEST_ENTRY_SENTINEL };
    TEST_ENTRY_ID_e e_list[] = { E015_ENTRY, TEST_ENTRY_SENTINEL };

    return run_pcie_static_and_exerciser(p_list, e_list, num_pe);
}

/* PCI_IN_05 */
uint32_t
pci_in_05_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e p_list[] = {
        P020_ENTRY, P022_ENTRY, P024_ENTRY, P025_ENTRY, P026_ENTRY, P033_ENTRY, TEST_ENTRY_SENTINEL
    };
    TEST_ENTRY_ID_e e_list[] = { E017_ENTRY, TEST_ENTRY_SENTINEL };

    return run_pcie_static_and_exerciser(p_list, e_list, num_pe);
}

/* GPU_04 */
uint32_t
gpu_04_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e tst_entry_list[] = {I024_ENTRY, P089_ENTRY,
                                        TEST_ENTRY_SENTINEL};

    return run_test_entries(tst_entry_list, num_pe);
}
/* V_L1WK_02 */
uint32_t
v_l1wk_02_05_entry(uint32_t num_pe)
{
#ifdef TARGET_LINUX
    // Test not applicable for Linux target
    return TEST_SKIP;
#endif

    if (g_el1physkip) {
        val_print(ACS_PRINT_TEST,
                    "\n       Skipping rule as EL1 physical timer access not supported", 0);
        return TEST_SKIP;
    }

    TEST_ENTRY_ID_e tst_entry_list[] = {U001_ENTRY, U002_ENTRY, TEST_ENTRY_SENTINEL};
    return run_test_entries(tst_entry_list, num_pe);
}

/* V_L1PE_02 */
uint32_t
v_l1pe_02_entry(uint32_t num_pe)
{
    TEST_ENTRY_ID_e tst_entry_list[] = {PE008_ENTRY, PE067_ENTRY, TEST_ENTRY_SENTINEL};
    return run_test_entries(tst_entry_list, num_pe);
}

/* V_L1PP_00 */
uint32_t
v_l1pp_00_entry(uint32_t num_pe)
{
#ifdef TARGET_LINUX
    // Test not applicable for Linux target
    return TEST_SKIP;
#endif

    TEST_ENTRY_ID_e skip_list[] = {G007_ENTRY, TEST_ENTRY_SENTINEL};
    TEST_ENTRY_ID_e default_list[] = {G006_ENTRY, G007_ENTRY, TEST_ENTRY_SENTINEL};

    TEST_ENTRY_ID_e *entry_list = g_el1physkip ? skip_list : default_list;

    return run_test_entries(entry_list, num_pe);
}
