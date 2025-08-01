/** @file
 * Copyright (c) 2016-2025, Arm Limited or its affiliates. All rights reserved.
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

#ifndef __ACS_LEVEL_H__
#define __ACS_LEVEL_H__

/* BSA Release versions */
#define BSA_ACS_MAJOR_VER       1
#define BSA_ACS_MINOR_VER       1
#define BSA_ACS_SUBMINOR_VER    1

/* SBSA Release versions */
#define SBSA_ACS_MAJOR_VER       7
#define SBSA_ACS_MINOR_VER       2
#define SBSA_ACS_SUBMINOR_VER    3

/* PC BSA Release versions */
#define PC_BSA_ACS_MAJOR_VER     0
#define PC_BSA_ACS_MINOR_VER     8
#define PC_BSA_ACS_SUBMINOR_VER  0

/* DRTM Release versions */
#define DRTM_ACS_MAJOR_VER      0
#define DRTM_ACS_MINOR_VER      7

/* MPAM Release versions */
#define MPAM_ACS_MAJOR_VER      0
#define MPAM_ACS_MINOR_VER      5
#define MPAM_ACS_SUBMINOR_VER   1

#define G_PRINT_LEVEL ACS_PRINT_TEST

#define G_SW_OS                 0
#define G_SW_HYP                1
#define G_SW_PS                 2

#define G_BSA_LEVEL             1
#define BSA_MIN_LEVEL_SUPPORTED 1
#define BSA_MAX_LEVEL_SUPPORTED 1

#define G_SBSA_LEVEL             4
#define SBSA_MIN_LEVEL_SUPPORTED 3
#define SBSA_MAX_LEVEL_SUPPORTED 7

#define G_PCBSA_LEVEL             1
#define PCBSA_MIN_LEVEL_SUPPORTED 1
#define PCBSA_MAX_LEVEL_SUPPORTED 1

#define SIZE_4K                 0x1000

/* Note : Total Size Required for Info tables ~ 550 KB
  * Table size is required to be updated whenever new members
  * are added in the info table structures
  */

/* Please MAKE SURE all the table sizes are 16 Bytes aligned */
#define PE_INFO_TBL_SZ          16384  /*Supports max 400 PEs     */
                                       /*[24 B Each + 4 B Header] */
#define GIC_INFO_TBL_SZ         240000 /*Supports max 832 GIC info (GICH,CPUIF,RD,ITS,MSI,D)*/
                                       /*[48 B Each + 32 B Header]*/
#define TIMER_INFO_TBL_SZ       2048   /*Supports max 4 system timers*/
                                       /*[248 B Each + 56 B Header]  */
#define WD_INFO_TBL_SZ          512    /*Supports max 20 Watchdogs*/
                                       /*[24 B Each + 4 B Header] */
#define MEM_INFO_TBL_SZ         32768  /*Supports max 800 memory regions*/
                                       /*[40 B Each + 16 B Header]      */
#define IOVIRT_INFO_TBL_SZ      1048576/*Supports max 2400 nodes of a typical iort table*/
                                       /*[(268+32*5) B Each + 24 B Header]*/
#define PERIPHERAL_INFO_TBL_SZ  2048   /*Supports max 20 PCIe EPs (USB and SATA controllers)*/
                                       /*[72 B Each + 16 B Header]*/
#define PCIE_INFO_TBL_SZ        1024   /*Supports max 40 RC's    */
                                       /*[24 B Each + 4 B Header]*/
#define SMBIOS_INFO_TBL_SZ      1024   /*Supports max 16 Processor Slots/Sockets*/
                                       /*[64 B Each]*/
#define PMU_INFO_TBL_SZ         20496  /*Supports maximum 512 PMUs*/
                                       /*[40 B Each + 4 B Header]*/
#define RAS_INFO_TBL_SZ         40960  /*Supports maximum 256 RAS Nodes*/
                                       /*[144 B Each + 12 B Header]*/
#define RAS2_FEAT_INFO_TBL_SZ   20480  /*Supports maximum of 1024 RAS2 memory feature entries*/
                                       /*[16 B Each + 8 B Header]*/
#define CACHE_INFO_TBL_SZ       262144 /*Support maximum of 7280 cache entries*/
                                       /*[36 B Each + 4 B Header]*/
#define SRAT_INFO_TBL_SZ        16384  /*Support maximum of 500 mem proximity domain entries*/
                                       /*[32 B Each + 8 B Header]*/
#define MPAM_INFO_TBL_SZ        262144 /*Supports maximum of 1800 MSC entries*/
                                       /*[24+(24*5) B Each + 4 B Header]*/
#define HMAT_INFO_TBL_SZ        12288  /*Supports maximum of 400 Proximity domains*/
                                       /*[24 B Each + 8 B Header]*/
#define PCC_INFO_TBL_SZ         262144 /*Supports maximum of 234 PCC info entries*/
                                       /*[112 B Each + 4B Header]*/
#define TPM2_INFO_TBL_SZ        256   /* Supports maximum of 10 TPM2 info entries */
                                      /* [24 B each: 3 x uint64_t] + 16 B Header */

#define BSA_LEVEL_PRINT_FORMAT(level, only) ((level > BSA_MAX_LEVEL_SUPPORTED) ? \
    ((only) != 0 ? "\n Starting tests for only level FR " : "\n Starting tests for level FR ") : \
    ((only) != 0 ? "\n Starting tests for only level %2d " : "\n Starting tests for level %2d "))

#define SBSA_LEVEL_PRINT_FORMAT(level, only) ((level > SBSA_MAX_LEVEL_SUPPORTED) ? \
    ((only) != 0 ? "\n Starting tests for only level FR " : "\n Starting tests for level FR ") : \
    ((only) != 0 ? "\n Starting tests for only level %2d " : "\n Starting tests for level %2d "))

#ifdef _AARCH64_BUILD_
unsigned long __stack_chk_guard = 0xBAAAAAAD;
unsigned long __stack_chk_fail =  0xBAAFAAAD;
#endif

uint32_t command_init(void);

#endif
