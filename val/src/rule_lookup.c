/** @file
 * Copyright (c) 2025-2026, Arm Limited or its affiliates. All rights reserved.
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

#include "rule_based_execution.h"

/* PC-BSA rule checklist based on Arm PC-BSA 1.0 specification */
const pcbsa_rule_entry_t pcbsa_rule_list[] = {
    /* Base */
    { P_L1_01,     PCBSA_LEVEL_1 },

    /* PE */
    { P_L1PE_02,   PCBSA_LEVEL_1 },
    { P_L1PE_03,   PCBSA_LEVEL_1 },
    { P_L1PE_04,   PCBSA_LEVEL_1 },
    { P_L1PE_05,   PCBSA_LEVEL_1 },
    { P_L1PE_06,   PCBSA_LEVEL_1 },
    { P_L1PE_07,   PCBSA_LEVEL_1 },
    { P_L1PE_08,   PCBSA_LEVEL_1 },
    { GFRQR,       PCBSA_LEVEL_1 },
    { MHCBW,       PCBSA_LEVEL_1 },
    { YKRHG,       PCBSA_LEVEL_2 },
    { CNBRV,       PCBSA_LEVEL_2 },
    { PBCRQ,       PCBSA_LEVEL_2 },
    { NWCYZ,       PCBSA_LEVEL_2 },

    /* Memory map */
    { P_L1MM_01,   PCBSA_LEVEL_1 },

    /* Interrupts */
    { P_L1GI_01,   PCBSA_LEVEL_1 },
    { P_L1GI_02,   PCBSA_LEVEL_1 },
    { P_L1GI_03,   PCBSA_LEVEL_1 },
    { P_L1GI_04,   PCBSA_LEVEL_1 },
    { P_L1PP_01,   PCBSA_LEVEL_1 },
    { TTLFJ,       PCBSA_LEVEL_2 },
    { PGKDP,       PCBSA_LEVEL_2 },

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
    { KQQWG,       PCBSA_LEVEL_FR },

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
    { B_PER_12, BSA_LEVEL_1,   SW_OS },

    { B_PER_11, BSA_LEVEL_1,   SW_PS },

/* PE FR */
    { B_PE_16,  BSA_LEVEL_FR, SW_OS }, // TODO mte app.
    { B_PE_17,  BSA_LEVEL_FR, SW_OS },
    { B_PE_25,  BSA_LEVEL_FR, SW_OS },
    { XRPZG,    BSA_LEVEL_FR, SW_OS },
    { B_SEC_01, BSA_LEVEL_FR, SW_OS },
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

/* PCIe FR */
    { B_REP_1,   BSA_LEVEL_FR, SW_OS },
    { B_IEP_1,   BSA_LEVEL_FR, SW_OS },
    { BJLPB,     BSA_LEVEL_FR, SW_OS },
    { B_PCIe_10, BSA_LEVEL_FR, SW_OS },
    { B_PCIe_11, BSA_LEVEL_FR, SW_OS },

/* sentinel */
    { RULE_ID_SENTINEL, BSA_LEVEL_SENTINEL, SW_OS }
};

const vbsa_rule_entry_t vbsa_rule_list[] = {

    /* L1 PE Rules */
    { V_L1PE_01, VBSA_LEVEL_1 },
    { V_L1PE_02, VBSA_LEVEL_1 },

    /* L1 Memory map rules */
    { V_L1MM_01, VBSA_LEVEL_1 },
    { V_L1MM_02, VBSA_LEVEL_1 },

    /* L1 GIC and PPI assignment rules */
    { V_L1GI_01, VBSA_LEVEL_1 },
    { V_L1PP_00, VBSA_LEVEL_1 },

    /* L1 SMMU rules */
    { V_L1SM_01, VBSA_LEVEL_1 },
    { V_L1SM_02, VBSA_LEVEL_1 },
    { V_L1SM_03, VBSA_LEVEL_1 },

    /* L1 Timer rules */
    { V_L1TM_01, VBSA_LEVEL_1 },
    { V_L1TM_02, VBSA_LEVEL_1 },
    { V_L1TM_03, VBSA_LEVEL_1 },
    { V_L1TM_04, VBSA_LEVEL_1 },

    /* L1 Wakeup rules */
    { V_L1WK_01, VBSA_LEVEL_1 },
    { V_L1WK_02, VBSA_LEVEL_1 },
    { V_L1WK_03, VBSA_LEVEL_1 },
    { V_L1WK_04, VBSA_LEVEL_1 },
    { V_L1WK_05, VBSA_LEVEL_1 },
    { V_L1WK_06, VBSA_LEVEL_1 },
    { V_L1WK_07, VBSA_LEVEL_1 },
    { V_L1WK_08, VBSA_LEVEL_1 },
    { V_L1WK_09, VBSA_LEVEL_1 },

    /* L1 Peripheral rules */
    { V_L1PR_01, VBSA_LEVEL_1 },
    { V_L1PR_02, VBSA_LEVEL_1 },

    /* FR (L2) PE rules */
    { V_L2PE_01, VBSA_LEVEL_FR },
    { V_L2PE_02, VBSA_LEVEL_FR },

    /* FR (L2) Watchdog rules */
    { V_L2WD_01, VBSA_LEVEL_FR },

    /* Sentinel to indicate end-of-list */
    { RULE_ID_SENTINEL, VBSA_LEVEL_SENTINEL }
};

/* SYS-MPAM rule checklist based on the MPAM system architecture */
const mpam_rule_entry_t mpam_rule_list[] = {
    { BDJXQ, MPAM_VERSION_B_c },
    { DDGSM, MPAM_VERSION_B_c },
    { RBCMK, MPAM_VERSION_B_c },
    { CFXGZ, MPAM_VERSION_B_c },
    { FXVBB, MPAM_VERSION_B_c },
    { LRZGD, MPAM_VERSION_B_c },
    { PBSTK, MPAM_VERSION_B_c },
    { KXMHB, MPAM_VERSION_B_c },
    { CZHSY, MPAM_VERSION_B_c },
    { RBWRS, MPAM_VERSION_B_c },
    { QKQCF, MPAM_VERSION_B_c },
    { CZSZX, MPAM_VERSION_B_c },
    { NVNHD, MPAM_VERSION_B_c },
    { SLJDR, MPAM_VERSION_B_c },
    { VNVMD, MPAM_VERSION_B_c },
    { SJFGX, MPAM_VERSION_B_c },
    { YPHLH, MPAM_VERSION_B_c },
    { RZCWR, MPAM_VERSION_B_c },
    { MRWWK, MPAM_VERSION_B_c },
    { XPPBS, MPAM_VERSION_B_c },
    { CWTSC, MPAM_VERSION_B_c },
    { JWKJD, MPAM_VERSION_B_c },
    { SGBFN, MPAM_VERSION_B_c },
    { ZTFBQ, MPAM_VERSION_B_c },
    { JGWSM, MPAM_VERSION_B_c },
    { LRNPR, MPAM_VERSION_B_c },
    { VBHPB, MPAM_VERSION_B_c },
    { WZXPM, MPAM_VERSION_B_c },
    { CBBRW, MPAM_VERSION_B_c },
    { CNQCS, MPAM_VERSION_B_c },
    { BHHCR, MPAM_VERSION_B_c },
    { BSLZQ, MPAM_VERSION_B_c },
    { WBZST, MPAM_VERSION_B_c },
    { YWPPT, MPAM_VERSION_B_c },
    { BNKHP, MPAM_VERSION_B_c },
    { KFDFD, MPAM_VERSION_B_c },
    { XRQHH, MPAM_VERSION_B_c },
    { VKQKB, MPAM_VERSION_B_c },
    { TTSLQ, MPAM_VERSION_B_c },
    { RGSJV, MPAM_VERSION_B_c },
    { MFRGP, MPAM_VERSION_B_c },
    { CYKPC, MPAM_VERSION_B_c },
    { KPNNZ, MPAM_VERSION_B_c },
    { JXRZM, MPAM_VERSION_B_c },
    { RHYKB, MPAM_VERSION_B_c },
    { LQQJN, MPAM_VERSION_B_c },
    { KBJPW, MPAM_VERSION_B_c },
    { YBYPZ, MPAM_VERSION_B_c },
    { CNGYT, MPAM_VERSION_B_c },
    { HHQWY, MPAM_VERSION_B_c },
    { WSXSL, MPAM_VERSION_B_c },
    { YQBMW, MPAM_VERSION_B_c },
    { YXZFG, MPAM_VERSION_B_c },
    { DQNFB, MPAM_VERSION_B_c },
    { YGZYL, MPAM_VERSION_B_c },
    { PLZWM, MPAM_VERSION_B_c },
    { DNNTB, MPAM_VERSION_B_c },
    { LXXDB, MPAM_VERSION_B_c },
    { MRZLG, MPAM_VERSION_B_c },
    { NPVVN, MPAM_VERSION_B_c },
    { HSFKH, MPAM_VERSION_B_c },
    { XRHKL, MPAM_VERSION_B_c },
    { GDMQZ, MPAM_VERSION_B_c },
    { MMXWZ, MPAM_VERSION_B_c },
    { QFCTM, MPAM_VERSION_B_c },
    { CZMFT, MPAM_VERSION_B_c },
    { XDZGT, MPAM_VERSION_B_c },
    { HLHYS, MPAM_VERSION_B_c },
    { GVCBB, MPAM_VERSION_B_c },
    { KWGHQ, MPAM_VERSION_B_c },
    { TZWJG, MPAM_VERSION_B_c },
    { VFJMZ, MPAM_VERSION_B_c },
    { WFQFS, MPAM_VERSION_B_c },
    { GMCWW, MPAM_VERSION_B_c },
    { CSLHH, MPAM_VERSION_B_c },
    { QFRFQ, MPAM_VERSION_B_c },
    { RNSLZ, MPAM_VERSION_B_c },
    { NJXHP, MPAM_VERSION_B_c },
    { XPPGG, MPAM_VERSION_B_c },
    { ZVTKX, MPAM_VERSION_B_c },
    { RLHRV, MPAM_VERSION_B_c },
    { ZRSRZ, MPAM_VERSION_B_c },
    { WMPLL, MPAM_VERSION_B_c },
    { LPLMM, MPAM_VERSION_B_c },
    { BDRCM, MPAM_VERSION_B_c },
    { ZHBXH, MPAM_VERSION_B_c },
    { SWNLN, MPAM_VERSION_B_c },
    { VZBFH, MPAM_VERSION_B_c },
    { CXZDR, MPAM_VERSION_B_c },
    { FRTVY, MPAM_VERSION_B_c },
    { DPZKC, MPAM_VERSION_B_c },
    { DKYDB, MPAM_VERSION_B_c },
    { GYMDB, MPAM_VERSION_B_c },
    { CFHQL, MPAM_VERSION_B_c },
    { HVRZS, MPAM_VERSION_B_c },
    { WKZBJ, MPAM_VERSION_B_c },
    { BPRDL, MPAM_VERSION_B_c },
    { WRXBV, MPAM_VERSION_B_c },
    { NWVFK, MPAM_VERSION_B_c },
    { PKRGZ, MPAM_VERSION_B_c },
    { VWHYW, MPAM_VERSION_B_c },
    { PWSWQ, MPAM_VERSION_B_c },
    { XSYFH, MPAM_VERSION_B_c },
    { ZQXQL, MPAM_VERSION_B_c },
    { CMZNL, MPAM_VERSION_B_c },
    { PXSBS, MPAM_VERSION_B_c },
    { ZDTLN, MPAM_VERSION_B_c },
    { ZFCRG, MPAM_VERSION_B_c },
    { TKBDC, MPAM_VERSION_B_c },
    { MSPZM, MPAM_VERSION_B_c },
    { JJKQP, MPAM_VERSION_B_c },
    { NGDBL, MPAM_VERSION_B_c },
    { MQRPH, MPAM_VERSION_B_c },
    { RZDSR, MPAM_VERSION_B_c },
    { MBNHC, MPAM_VERSION_B_c },
    { JPLMZ, MPAM_VERSION_B_c },
    { DQTTD, MPAM_VERSION_B_c },
    { RNWBS, MPAM_VERSION_B_c },
    { GFYDF, MPAM_VERSION_B_c },
    { WCQKV, MPAM_VERSION_B_c },
    { ZGGMX, MPAM_VERSION_B_c },
    { TMCXJ, MPAM_VERSION_B_c },
    { YFMQP, MPAM_VERSION_B_c },
    { DLSGF, MPAM_VERSION_B_c },
    { WHJQG, MPAM_VERSION_B_c },
    { VLSJS, MPAM_VERSION_B_c },
    { RYRQJ, MPAM_VERSION_B_c },
    { QGXSZ, MPAM_VERSION_B_c },
    { VJDKF, MPAM_VERSION_B_c },
    { DSCQB, MPAM_VERSION_B_c },
    { FWCFZ, MPAM_VERSION_B_c },
    { PVNHR, MPAM_VERSION_B_c },
    { KCWDZ, MPAM_VERSION_B_c },
    { XLWYW, MPAM_VERSION_B_c },
    { GKWNB, MPAM_VERSION_B_c },
    { LRTGP, MPAM_VERSION_B_c },
    { NXKSF, MPAM_VERSION_B_c },
    { FVFYC, MPAM_VERSION_B_c },
    { ZTXDS, MPAM_VERSION_B_c },
    { VBZTW, MPAM_VERSION_B_c },
    { FZHDC, MPAM_VERSION_B_c },
    { YFVPY, MPAM_VERSION_B_c },
    { NKTBN, MPAM_VERSION_B_c },
    { NWKJR, MPAM_VERSION_B_c },
    { SQSMV, MPAM_VERSION_B_c },
    { WQJKV, MPAM_VERSION_B_c },
    { XZXVK, MPAM_VERSION_B_c },
    { NDMMK, MPAM_VERSION_B_c },
    { VBBTL, MPAM_VERSION_B_c },
    { XSTRM, MPAM_VERSION_B_c },
    { JCNST, MPAM_VERSION_B_c },
    { DFWFP, MPAM_VERSION_B_c },
    { GNQFY, MPAM_VERSION_B_c },
    { MGGJW, MPAM_VERSION_B_c },
    { HXNGD, MPAM_VERSION_B_c },
    { RPGBQ, MPAM_VERSION_B_c },
    { MTGGZ, MPAM_VERSION_B_c },
    { DYNFP, MPAM_VERSION_B_c },
    { WHSLQ, MPAM_VERSION_B_c },
    { GVBGS, MPAM_VERSION_B_c },
    { QBKDG, MPAM_VERSION_B_c },
    { DLSCZ, MPAM_VERSION_B_c },
    { ZWRQW, MPAM_VERSION_B_c },
    { VKKDP, MPAM_VERSION_B_c },
    { XTQZR, MPAM_VERSION_B_c },
    { XDCDL, MPAM_VERSION_B_c },
    { JMLNM, MPAM_VERSION_B_c },
    { XMLDN, MPAM_VERSION_B_c },
    { VMTBP, MPAM_VERSION_B_c },
    { WXKWK, MPAM_VERSION_B_c },
    { MGXDK, MPAM_VERSION_B_c },
    { HJRZC, MPAM_VERSION_B_c },
    { SQTSJ, MPAM_VERSION_B_c },
    { VZWYW, MPAM_VERSION_B_c },
    { PNPQQ, MPAM_VERSION_B_c },
    { SJFMB, MPAM_VERSION_B_c },
    { VYLZC, MPAM_VERSION_B_c },
    { QBQJG, MPAM_VERSION_B_c },
    { PFNBV, MPAM_VERSION_B_c },
    { CZZZK, MPAM_VERSION_B_c },
    { HVNKJ, MPAM_VERSION_B_c },
    { TMMMD, MPAM_VERSION_B_c },
    { QDSPY, MPAM_VERSION_B_c },
    { PPJSM, MPAM_VERSION_B_c },
    { GLPPK, MPAM_VERSION_B_c },
    { NRGQD, MPAM_VERSION_B_c },
    { KVGWS, MPAM_VERSION_B_c },
    { KVHTZ, MPAM_VERSION_B_c },
    { ZRDMM, MPAM_VERSION_B_c },
    { YLCVK, MPAM_VERSION_B_c },
    { YJCDR, MPAM_VERSION_B_c },
    { YVSNK, MPAM_VERSION_B_c },
    { JLYJJ, MPAM_VERSION_B_c },
    { MBDJF, MPAM_VERSION_B_c },
    { LVCGQ, MPAM_VERSION_B_c },
    { DMKKT, MPAM_VERSION_B_c },
    { VTFQK, MPAM_VERSION_B_c },
    { CTGKD, MPAM_VERSION_B_c },
    { RDMVJ, MPAM_VERSION_B_c },
    { KNNBY, MPAM_VERSION_B_c },
    { ZPWLC, MPAM_VERSION_B_c },
    { DYDFT, MPAM_VERSION_B_c },
    { HXLMF, MPAM_VERSION_B_c },
    { QCYPH, MPAM_VERSION_B_c },
    { FLWNK, MPAM_VERSION_B_c },
    { GTKCX, MPAM_VERSION_B_c },
    { DFBBW, MPAM_VERSION_B_c },
    { FYTQL, MPAM_VERSION_B_c },
    { KZBWG, MPAM_VERSION_B_c },
    { YCBKB, MPAM_VERSION_B_c },
    { TVYHS, MPAM_VERSION_B_c },
    { RJFKG, MPAM_VERSION_B_c },
    { FKNKB, MPAM_VERSION_B_c },
    { RULE_ID_SENTINEL, MPAM_VERSION_SENTINEL }
};

/* PFDI rule checklist (single level at present) */
const pfdi_rule_entry_t pfdi_rule_list[] = {
    { R0040, PFDI_LEVEL_1 },
    { R0053, PFDI_LEVEL_1 },
    { R0060, PFDI_LEVEL_1 },
    { R0066, PFDI_LEVEL_1 },
    { R0071, PFDI_LEVEL_1 },
    { R0076, PFDI_LEVEL_1 },
    { R0082, PFDI_LEVEL_1 },
    { R0089, PFDI_LEVEL_1 },
    { R0099, PFDI_LEVEL_1 },
    { R0100, PFDI_LEVEL_1 },
    { R0102, PFDI_LEVEL_1 },
    { R0104, PFDI_LEVEL_1 },
    { R0154, PFDI_LEVEL_1 },
    { R0155, PFDI_LEVEL_1 },
    { R0156, PFDI_LEVEL_1 },
    { R0157, PFDI_LEVEL_1 },
    { R0158, PFDI_LEVEL_1 },
    { R0160, PFDI_LEVEL_1 },
    { R0163, PFDI_LEVEL_1 },
    { R0164, PFDI_LEVEL_1 },
    { R0165, PFDI_LEVEL_1 },
    { R0166, PFDI_LEVEL_1 },
    { R0167, PFDI_LEVEL_1 },
    { R0168, PFDI_LEVEL_1 },
    { R0172, PFDI_LEVEL_1 },
    { R0173, PFDI_LEVEL_1 },
    { R0176, PFDI_LEVEL_1 },
    { R0179, PFDI_LEVEL_1 },
    { R0180, PFDI_LEVEL_1 },
    { R0193, PFDI_LEVEL_1 },
    { R0194, PFDI_LEVEL_1 },
    { RULE_ID_SENTINEL, PFDI_LEVEL_SENTINEL }
};

/* Length helpers (exclude sentinel) */
const uint32_t pcbsa_rule_list_len = (sizeof(pcbsa_rule_list) / sizeof(pcbsa_rule_list[0])) - 1U;
const uint32_t sbsa_rule_list_len  = (sizeof(sbsa_rule_list)  / sizeof(sbsa_rule_list[0]))  - 1U;
const uint32_t bsa_rule_list_len   = (sizeof(bsa_rule_list)   / sizeof(bsa_rule_list[0]))   - 1U;
const uint32_t vbsa_rule_list_len  = (sizeof(vbsa_rule_list)  / sizeof(vbsa_rule_list[0]))  - 1U;
const uint32_t pfdi_rule_list_len  = (sizeof(pfdi_rule_list)  / sizeof(pfdi_rule_list[0]))  - 1U;
const uint32_t mpam_rule_list_len  = (sizeof(mpam_rule_list)  / sizeof(mpam_rule_list[0]))  - 1U;
