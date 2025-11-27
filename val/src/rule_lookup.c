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

/* This file holds rule lookups for various ACS specification */

#include "include/rule_based_execution.h"

/* PC-BSA rule checklist based on Arm PC-BSA 1.0 specification */
const pcbsa_rule_entry_t pcbsa_rule_list[] = {
    /* Base */
    { P_L1_01,     PCBSA_LEVEL_1 },

    /* PE */
    { P_L1PE_01,   PCBSA_LEVEL_1 },
    { P_L1PE_02,   PCBSA_LEVEL_1 },
    { P_L1PE_03,   PCBSA_LEVEL_1 },
    { P_L1PE_04,   PCBSA_LEVEL_1 },
    { P_L1PE_05,   PCBSA_LEVEL_1 },
    { P_L1PE_06,   PCBSA_LEVEL_1 },
    { P_L1PE_07,   PCBSA_LEVEL_1 },
    { P_L1PE_08,   PCBSA_LEVEL_1 },

    /* Memory map */
    { P_L1MM_01,   PCBSA_LEVEL_1 },

    /* Interrupts */
    { P_L1GI_01,   PCBSA_LEVEL_1 },
    { P_L1GI_02,   PCBSA_LEVEL_1 },
    { P_L1GI_03,   PCBSA_LEVEL_1 },
    { P_L1GI_04,   PCBSA_LEVEL_1 },
    { P_L1PP_01,   PCBSA_LEVEL_1 },

    /* SMMU */
    { P_L1SM_01,   PCBSA_LEVEL_1 },
    { P_L1SM_02,   PCBSA_LEVEL_1 },
    { P_L1SM_03,   PCBSA_LEVEL_1 },
    { P_L1SM_04,   PCBSA_LEVEL_1 },
    { P_L1SM_05,   PCBSA_LEVEL_1 },

    /* PCIe */
    { P_L1PCI_1,   PCBSA_LEVEL_1 },
    { P_L1PCI_2,   PCBSA_LEVEL_1 },

    /* NV Store */
    { P_L1NV_01,   PCBSA_LEVEL_1 },

    /* Security */
    { P_L1SE_01,   PCBSA_LEVEL_1 },
    { P_L1SE_02,   PCBSA_LEVEL_1 },
    { P_L1SE_03,   PCBSA_LEVEL_1 },
    { P_L1SE_04,   PCBSA_LEVEL_1 },
    { P_L1SE_05,   PCBSA_LEVEL_1 },

    /* TPM */
    { P_L1TP_01,   PCBSA_LEVEL_1 },
    { P_L1TP_02,   PCBSA_LEVEL_1 },
    { P_L1TP_03,   PCBSA_LEVEL_1 },
    { P_L1TP_04,   PCBSA_LEVEL_1 },

    /* Future Level */
    { P_L2WD_01,   PCBSA_LEVEL_FR },

    /* sentinel */
    { RULE_ID_SENTINEL, PCBSA_LEVEL_SENTINEL }
};

/* SBSA rule checklist based on Arm SBSA 7.2 specification */
const sbsa_rule_entry_t sbsa_rule_list[] = {
    /* Level 3 */
    { S_L3_01,     SBSA_LEVEL_3 },
    { S_L3PE_01,   SBSA_LEVEL_3 },
    { S_L3PE_02,   SBSA_LEVEL_3 },
    { S_L3PE_03,   SBSA_LEVEL_3 },
    { S_L3PE_04,   SBSA_LEVEL_3 },
    { S_L3MM_01,   SBSA_LEVEL_3 },
    { S_L3MM_02,   SBSA_LEVEL_3 },
    { S_L3GI_01,   SBSA_LEVEL_3 },
    { S_L3GI_02,   SBSA_LEVEL_3 },
    { S_L3PP_01,   SBSA_LEVEL_3 },
    { S_L3SM_01,   SBSA_LEVEL_3 },
    { S_L3WD_01,   SBSA_LEVEL_3 },
    { S_L3PR_01,   SBSA_LEVEL_3 },
    { S_PCIe_09,   SBSA_LEVEL_3 },

    /* Level 4 */
    { S_L4PE_01,   SBSA_LEVEL_4 },
    { S_L4PE_02,   SBSA_LEVEL_4 },
    { S_L4PE_03,   SBSA_LEVEL_4 },
    { S_L4PE_04,   SBSA_LEVEL_4 },
    { S_L4SM_01,   SBSA_LEVEL_4 },
    { S_L4SM_02,   SBSA_LEVEL_4 },
    { S_L4SM_03,   SBSA_LEVEL_4 },
    { S_L4PCI_1,   SBSA_LEVEL_4 },
    { S_L4PCI_2,   SBSA_LEVEL_4 },

    /* Level 5 */
    { S_L5PE_01,   SBSA_LEVEL_5 },
    { S_L5PE_02,   SBSA_LEVEL_5 },
    { S_L5PE_03,   SBSA_LEVEL_5 },
    { S_L5PE_04,   SBSA_LEVEL_5 },
    { S_L5PE_05,   SBSA_LEVEL_5 },
    { S_L5PE_06,   SBSA_LEVEL_5 },
    { S_L5PE_07,   SBSA_LEVEL_5 },
    { S_L5GI_01,   SBSA_LEVEL_5 },
    { S_L5SM_01,   SBSA_LEVEL_5 },
    { S_L5SM_02,   SBSA_LEVEL_5 },
    { S_L5SM_03,   SBSA_LEVEL_5 },
    { S_L5SM_04,   SBSA_LEVEL_5 },
    { S_L5TI_01,   SBSA_LEVEL_5 },
    { S_L5PP_01,   SBSA_LEVEL_5 },

    /* Level 6 */
    { S_L6PE_01,   SBSA_LEVEL_6 },
    { S_L6PE_02,   SBSA_LEVEL_6 },
    { S_L6PE_03,   SBSA_LEVEL_6 },
    { S_L6PE_04,   SBSA_LEVEL_6 },
    { S_L6PE_05,   SBSA_LEVEL_6 },
    { S_L6PE_06,   SBSA_LEVEL_6 },
    { S_L6PE_07,   SBSA_LEVEL_6 },
    { S_L6PE_08,   SBSA_LEVEL_6 },
    { S_L6SM_02,   SBSA_LEVEL_6 },
    { S_L6SM_04,   SBSA_LEVEL_6 },
    { S_L6WD_01,   SBSA_LEVEL_6 },
    { S_RAS_01,    SBSA_LEVEL_6 },
    { S_RAS_03,    SBSA_LEVEL_6 },
    { S_L6PCI_1,   SBSA_LEVEL_6 },

    /* Level 7 */
    { S_L7PE_01,   SBSA_LEVEL_7 },
    { S_L7PE_02,   SBSA_LEVEL_7 },
    { S_L7PE_04,   SBSA_LEVEL_7 },
    { S_L7PE_05,   SBSA_LEVEL_7 },
    { S_L7PE_06,   SBSA_LEVEL_7 },
    { S_L7PE_07,   SBSA_LEVEL_7 },
    { S_L7RAS_1,   SBSA_LEVEL_7 },
    { S_L7RAS_2,   SBSA_LEVEL_7 },
    { S_L7TME_1,   SBSA_LEVEL_7 },
    { S_L7TME_2,   SBSA_LEVEL_7 },
    { S_L7TME_3,   SBSA_LEVEL_7 },
    { S_L7TME_4,   SBSA_LEVEL_7 },
    { S_L7TME_5,   SBSA_LEVEL_7 },
    { S_L7MP_01,   SBSA_LEVEL_7 },
    { S_L7MP_02,   SBSA_LEVEL_7 },
    { S_L7MP_03,   SBSA_LEVEL_7 },
    { S_L7MP_04,   SBSA_LEVEL_7 },
    { S_L7MP_05,   SBSA_LEVEL_7 },
    { S_L7MP_08,   SBSA_LEVEL_7 },
    { S_L7ENT_1,   SBSA_LEVEL_7 },
    { S_L7SM_01,   SBSA_LEVEL_7 },
    { S_L7SM_02,   SBSA_LEVEL_7 },
    { S_L7SM_03,   SBSA_LEVEL_7 },
    { S_L7SM_04,   SBSA_LEVEL_7 },
    { S_L7PMU,     SBSA_LEVEL_7 },
    { SYS_RAS,     SBSA_LEVEL_7 },
    { SYS_RAS_1,   SBSA_LEVEL_7 },
    { SYS_RAS_2,   SBSA_LEVEL_7 },
    { SYS_RAS_3,   SBSA_LEVEL_7 },
    { S_PCIe_01,   SBSA_LEVEL_7 },
    { S_PCIe_02,   SBSA_LEVEL_7 },
    { S_PCIe_03,   SBSA_LEVEL_7 },
    { S_PCIe_04,   SBSA_LEVEL_7 },
    { S_PCIe_05,   SBSA_LEVEL_7 },
    { PCI_ER_01,   SBSA_LEVEL_7 },
    { PCI_ER_04,   SBSA_LEVEL_7 },
    { PCI_ER_05,   SBSA_LEVEL_7 },
    { PCI_ER_06,   SBSA_LEVEL_7 },

    /* Version 8.0 */
    { S_L8PE_01,   SBSA_VER_8_0 },
    { S_L8PE_02,   SBSA_VER_8_0 },
    { S_L8PE_03,   SBSA_VER_8_0 },
    { S_L8PE_04,   SBSA_VER_8_0 },
    { S_L8PE_05,   SBSA_VER_8_0 },
    { S_L8PE_06,   SBSA_VER_8_0 },
    { S_L8PE_07,   SBSA_VER_8_0 },
    { S_L8PE_08,   SBSA_VER_8_0 },
    { S_L8RME_1,   SBSA_VER_8_0 },
    { S_L8SM_01,   SBSA_VER_8_0 },
    { SYS_RAS_4,   SBSA_VER_8_0 },
    { S_L8TI_01,   SBSA_VER_8_0 },
    { S_L8GI_01,   SBSA_VER_8_0 },
    { S_L8SHD_1,   SBSA_VER_8_0 },
    { S_PCIe_06,   SBSA_VER_8_0 },
    { S_PCIe_07,   SBSA_VER_8_0 },
    { S_PCIe_08,   SBSA_VER_8_0 },
    { S_PCIe_10,   SBSA_VER_8_0 },
    { XDGKZ,       SBSA_VER_8_0 },
    { S_L8CXL_1,   SBSA_VER_8_0 },

    /* SBSA Future Requirement */
    { PCI_ER_07,   SBSA_LEVEL_FR },
    { PCI_ER_08,   SBSA_LEVEL_FR },
    { PCI_ER_09,   SBSA_LEVEL_FR },
    { PCI_ER_10,   SBSA_LEVEL_FR },
    { WNPXD,       SBSA_LEVEL_FR },
    { KBRZG,       SBSA_LEVEL_FR },
    { LVQBC,       SBSA_LEVEL_FR },

    /* sentinel */
    { RULE_ID_SENTINEL, SBSA_LEVEL_SENTINEL }
};

/* BSA rule checklist based on Arm BSA 1.2 specification */
const bsa_rule_entry_t bsa_rule_list[] = {
/* PE L1*/
    { B_PE_01,  BSA_LEVEL_1, SW_OS },
    { B_PE_02,  BSA_LEVEL_1, SW_OS },
    { B_PE_03,  BSA_LEVEL_1, SW_OS },
    { B_PE_04,  BSA_LEVEL_1, SW_OS },
    { B_PE_05,  BSA_LEVEL_1, SW_OS },
    { B_PE_06,  BSA_LEVEL_1, SW_OS },
    { B_PE_07,  BSA_LEVEL_1, SW_OS },
    { B_PE_08,  BSA_LEVEL_1, SW_OS },
    { B_PE_09,  BSA_LEVEL_1, SW_OS },
    { B_PE_10,  BSA_LEVEL_1, SW_OS },
    { B_PE_11,  BSA_LEVEL_1, SW_OS },
    { B_PE_12,  BSA_LEVEL_1, SW_OS },
    { B_PE_13,  BSA_LEVEL_1, SW_OS },
    { B_PE_14,  BSA_LEVEL_1, SW_OS },

    { B_PE_18,  BSA_LEVEL_1, SW_HYP },
    { B_PE_19,  BSA_LEVEL_1, SW_HYP },
    { B_PE_20,  BSA_LEVEL_1, SW_HYP },
    { B_PE_21,  BSA_LEVEL_1, SW_HYP },
    { B_PE_22,  BSA_LEVEL_1, SW_HYP },

    { B_PE_23,  BSA_LEVEL_1, SW_PS },
    { B_PE_24,  BSA_LEVEL_1, SW_PS },

/* Memory map L1 */
    { B_MEM_01, BSA_LEVEL_1,  SW_OS },
    { B_MEM_02, BSA_LEVEL_1,  SW_OS },
    { B_MEM_03, BSA_LEVEL_1,  SW_OS },
    { B_MEM_05, BSA_LEVEL_1,  SW_OS },
    { B_MEM_06, BSA_LEVEL_1,  SW_OS },
    { B_MEM_07, BSA_LEVEL_1,  SW_OS },

    { B_MEM_08, BSA_LEVEL_1,  SW_PS },
    { B_MEM_09, BSA_LEVEL_1,  SW_PS },

/* Interrupts (GIC, PPI) L1 */
    { B_GIC_01, BSA_LEVEL_1,  SW_OS },
    { B_GIC_02, BSA_LEVEL_1,  SW_OS },
    { B_GIC_03, BSA_LEVEL_1,  SW_OS },
    { B_GIC_04, BSA_LEVEL_1,  SW_OS },
    { B_GIC_05, BSA_LEVEL_1,  SW_OS },
    { B_PPI_00, BSA_LEVEL_1,  SW_OS },

/* SMMU L1*/
    { B_SMMU_01, BSA_LEVEL_1, SW_OS },
    { B_SMMU_02, BSA_LEVEL_1, SW_OS },
    { B_SMMU_06, BSA_LEVEL_1, SW_OS },
    { B_SMMU_07, BSA_LEVEL_1, SW_OS },
    { B_SMMU_08, BSA_LEVEL_1, SW_OS },
    { B_SMMU_12, BSA_LEVEL_1, SW_OS },

    { B_SMMU_16, BSA_LEVEL_1, SW_HYP },
    { B_SMMU_17, BSA_LEVEL_1, SW_HYP },
    { B_SMMU_18, BSA_LEVEL_1, SW_HYP },
    { B_SMMU_19, BSA_LEVEL_1, SW_HYP },
    { B_SMMU_21, BSA_LEVEL_1, SW_HYP },

/* Timer L1 */
    { B_TIME_01, BSA_LEVEL_1,  SW_OS },
    { B_TIME_02, BSA_LEVEL_1,  SW_OS },
    { B_TIME_03, BSA_LEVEL_1,  SW_OS },
    { B_TIME_04, BSA_LEVEL_1,  SW_OS },
    { B_TIME_05, BSA_LEVEL_1,  SW_OS },
    { B_TIME_06, BSA_LEVEL_1,  SW_OS },
    { B_TIME_07, BSA_LEVEL_1,  SW_OS },
    { B_TIME_08, BSA_LEVEL_1,  SW_OS },
    { B_TIME_09, BSA_LEVEL_1,  SW_OS },
    { B_TIME_10, BSA_LEVEL_1,  SW_OS },

/* Power and wakeup L1 */
    { B_WAK_01, BSA_LEVEL_1,   SW_OS },
    { B_WAK_02, BSA_LEVEL_1,   SW_OS },
    { B_WAK_03, BSA_LEVEL_1,   SW_OS },
    { B_WAK_04, BSA_LEVEL_1,   SW_OS },
    { B_WAK_05, BSA_LEVEL_1,   SW_OS },
    { B_WAK_06, BSA_LEVEL_1,   SW_OS },
    { B_WAK_07, BSA_LEVEL_1,   SW_OS },
    { B_WAK_08, BSA_LEVEL_1,   SW_OS },
    { B_WAK_10, BSA_LEVEL_1,   SW_OS },
    { B_WAK_11, BSA_LEVEL_1,   SW_OS },

/* Watchdog L1 */
    { B_WD_00,  BSA_LEVEL_1,   SW_OS },

/* Peripherals L1 */
    { B_PER_01, BSA_LEVEL_1,   SW_OS },
    { B_PER_02, BSA_LEVEL_1,   SW_OS },
    { B_PER_03, BSA_LEVEL_1,   SW_OS },
    { B_PER_04, BSA_LEVEL_1,   SW_OS },
    { B_PER_05, BSA_LEVEL_1,   SW_OS },
    { B_PER_06, BSA_LEVEL_1,   SW_OS },
    { B_PER_07, BSA_LEVEL_1,   SW_OS },
    { B_PER_08, BSA_LEVEL_1,   SW_OS },
    { B_PER_09, BSA_LEVEL_1,   SW_OS },
    { B_PER_10, BSA_LEVEL_1,   SW_OS },
    { B_PER_12, BSA_LEVEL_1,   SW_OS },

    { B_PER_11, BSA_LEVEL_1,   SW_PS },

/* PE FR */
    { B_PE_16,  BSA_LEVEL_FR, SW_OS }, // TODO mte app.
    { B_PE_17,  BSA_LEVEL_FR, SW_OS },
    { B_PE_25,  BSA_LEVEL_FR, SW_OS },
    { XRPZG,    BSA_LEVEL_FR, SW_OS },
    { B_SEC_01, BSA_LEVEL_FR, SW_OS },
    { B_SEC_02, BSA_LEVEL_FR, SW_OS },
    { B_SEC_03, BSA_LEVEL_FR, SW_OS },
    { B_SEC_04, BSA_LEVEL_FR, SW_OS },
    { B_SEC_05, BSA_LEVEL_FR, SW_OS },

/* SMMU FR */
    { B_SMMU_03, BSA_LEVEL_FR, SW_OS },
    { B_SMMU_04, BSA_LEVEL_FR, SW_OS },
    { B_SMMU_05, BSA_LEVEL_FR, SW_OS },
    { B_SMMU_09, BSA_LEVEL_FR, SW_OS },
    { B_SMMU_11, BSA_LEVEL_FR, SW_OS },
    { B_SMMU_13, BSA_LEVEL_FR, SW_OS },
    { B_SMMU_14, BSA_LEVEL_FR, SW_OS },
    { B_SMMU_20, BSA_LEVEL_FR, SW_OS },
    { B_SMMU_23, BSA_LEVEL_FR, SW_OS },
    { B_SMMU_24, BSA_LEVEL_FR, SW_OS },
    { B_SMMU_25, BSA_LEVEL_FR, SW_OS },

/* SMMU FR */
    { B_REP_1,   BSA_LEVEL_FR, SW_OS },
    { B_IEP_1,   BSA_LEVEL_FR, SW_OS },
    { BJLPB,     BSA_LEVEL_FR, SW_OS },
    { B_PCIe_10, BSA_LEVEL_FR, SW_OS },
    { B_PCIe_11, BSA_LEVEL_FR, SW_OS },

/* sentinel */
    { RULE_ID_SENTINEL, BSA_LEVEL_SENTINEL, SW_OS }
};
