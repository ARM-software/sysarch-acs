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

/* ------------------------------  ACS settings  ------------------------------ */
#define PLATFORM_OVERRIDE_PRINT_LEVEL        0x3     /* Console log level (1-5)                  */
#define PLATFORM_OVERRIDE_BSA_LEVEL          0x1     /* Target BSA compliance level (only 1)     */
#define PLATFORM_OVERRIDE_SBSA_LEVEL         0x7     /* Target SBSA compliance level (3-7)       */
#define PLATFORM_OVERRIDE_PCBSA_LEVEL        0x1     /* Target PCBSA compliance level (only 1)   */

/* ------------------------------  MMU page table ------------------------------ */
#define PLATFORM_PAGE_SIZE              0x1000       /* MMU Memory Page Size                     */
#define PLATFORM_OVERRIDE_MMU_PGT_IAS   48           /* MMU Input Address Size                   */
#define PLATFORM_OVERRIDE_MMU_PGT_OAS   48           /* MMU Output Address Size                  */

/* --------------------------------  PE info  -------------------------------- */
#define PLATFORM_OVERRIDE_PE_CNT             16      /* Total processing elements present        */

/*  Per-CPU descriptors (index, MPIDR, interrupt IDs) */
#define PLATFORM_OVERRIDE_PE0_INDEX        0x0       /* Index of PE in the system's PE list       */
#define PLATFORM_OVERRIDE_PE0_MPIDR        0x0       /* Multiprocessor Affinity Reg for PE[Index] */
#define PLATFORM_OVERRIDE_PE0_PMU_GSIV     0x17      /* Performance Interrupt GSIV                */
#define PLATFORM_OVERRIDE_PE0_GMAIN_GSIV   0x19      /* VGIC Maintaninance Interrupt GSIV         */
#define PLATFORM_OVERRIDE_PE0_TRBE_INTR    0x0       /* Trace Buffer Extension interrupt GSIV     */

#define PLATFORM_OVERRIDE_PE1_INDEX        0x1
#define PLATFORM_OVERRIDE_PE1_MPIDR        0x10000
#define PLATFORM_OVERRIDE_PE1_PMU_GSIV     0x17
#define PLATFORM_OVERRIDE_PE1_GMAIN_GSIV   0x19
#define PLATFORM_OVERRIDE_PE1_TRBE_INTR    0x0

#define PLATFORM_OVERRIDE_PE2_INDEX        0x2
#define PLATFORM_OVERRIDE_PE2_MPIDR        0x20000
#define PLATFORM_OVERRIDE_PE2_PMU_GSIV     0x17
#define PLATFORM_OVERRIDE_PE2_GMAIN_GSIV   0x19
#define PLATFORM_OVERRIDE_PE2_TRBE_INTR    0x0

#define PLATFORM_OVERRIDE_PE3_INDEX        0x3
#define PLATFORM_OVERRIDE_PE3_MPIDR        0x30000
#define PLATFORM_OVERRIDE_PE3_PMU_GSIV     0x17
#define PLATFORM_OVERRIDE_PE3_GMAIN_GSIV   0x19
#define PLATFORM_OVERRIDE_PE3_TRBE_INTR    0x0

#define PLATFORM_OVERRIDE_PE4_INDEX        0x4
#define PLATFORM_OVERRIDE_PE4_MPIDR        0x40000
#define PLATFORM_OVERRIDE_PE4_PMU_GSIV     0x17
#define PLATFORM_OVERRIDE_PE4_GMAIN_GSIV   0x19
#define PLATFORM_OVERRIDE_PE4_TRBE_INTR    0x0

#define PLATFORM_OVERRIDE_PE5_INDEX        0x5
#define PLATFORM_OVERRIDE_PE5_MPIDR        0x50000
#define PLATFORM_OVERRIDE_PE5_PMU_GSIV     0x17
#define PLATFORM_OVERRIDE_PE5_GMAIN_GSIV   0x19
#define PLATFORM_OVERRIDE_PE5_TRBE_INTR    0x0

#define PLATFORM_OVERRIDE_PE6_INDEX        0x6
#define PLATFORM_OVERRIDE_PE6_MPIDR        0x60000
#define PLATFORM_OVERRIDE_PE6_PMU_GSIV     0x17
#define PLATFORM_OVERRIDE_PE6_GMAIN_GSIV   0x19
#define PLATFORM_OVERRIDE_PE6_TRBE_INTR    0x0

#define PLATFORM_OVERRIDE_PE7_INDEX        0x7
#define PLATFORM_OVERRIDE_PE7_MPIDR        0x70000
#define PLATFORM_OVERRIDE_PE7_PMU_GSIV     0x17
#define PLATFORM_OVERRIDE_PE7_GMAIN_GSIV   0x19
#define PLATFORM_OVERRIDE_PE7_TRBE_INTR    0x0

#define PLATFORM_OVERRIDE_PE8_INDEX        0x8
#define PLATFORM_OVERRIDE_PE8_MPIDR        0x80000
#define PLATFORM_OVERRIDE_PE8_PMU_GSIV     0x17
#define PLATFORM_OVERRIDE_PE8_GMAIN_GSIV   0x19
#define PLATFORM_OVERRIDE_PE8_TRBE_INTR    0x0

#define PLATFORM_OVERRIDE_PE9_INDEX        0x9
#define PLATFORM_OVERRIDE_PE9_MPIDR        0x90000
#define PLATFORM_OVERRIDE_PE9_PMU_GSIV     0x17
#define PLATFORM_OVERRIDE_PE9_GMAIN_GSIV   0x19
#define PLATFORM_OVERRIDE_PE9_TRBE_INTR    0x0

#define PLATFORM_OVERRIDE_PE10_INDEX       0xA
#define PLATFORM_OVERRIDE_PE10_MPIDR       0xA0000
#define PLATFORM_OVERRIDE_PE10_PMU_GSIV    0x17
#define PLATFORM_OVERRIDE_PE10_GMAIN_GSIV  0x19
#define PLATFORM_OVERRIDE_PE10_TRBE_INTR   0x0

#define PLATFORM_OVERRIDE_PE11_INDEX       0xB
#define PLATFORM_OVERRIDE_PE11_MPIDR       0xB0000
#define PLATFORM_OVERRIDE_PE11_PMU_GSIV    0x17
#define PLATFORM_OVERRIDE_PE11_GMAIN_GSIV  0x19
#define PLATFORM_OVERRIDE_PE11_TRBE_INTR   0x0

#define PLATFORM_OVERRIDE_PE12_INDEX       0xC
#define PLATFORM_OVERRIDE_PE12_MPIDR       0xC0000
#define PLATFORM_OVERRIDE_PE12_PMU_GSIV    0x17
#define PLATFORM_OVERRIDE_PE12_GMAIN_GSIV  0x19
#define PLATFORM_OVERRIDE_PE12_TRBE_INTR   0x0

#define PLATFORM_OVERRIDE_PE13_INDEX       0xD
#define PLATFORM_OVERRIDE_PE13_MPIDR       0xD0000
#define PLATFORM_OVERRIDE_PE13_PMU_GSIV    0x17
#define PLATFORM_OVERRIDE_PE13_GMAIN_GSIV  0x19
#define PLATFORM_OVERRIDE_PE13_TRBE_INTR   0x0

#define PLATFORM_OVERRIDE_PE14_INDEX       0xE
#define PLATFORM_OVERRIDE_PE14_MPIDR       0xE0000
#define PLATFORM_OVERRIDE_PE14_PMU_GSIV    0x17
#define PLATFORM_OVERRIDE_PE14_GMAIN_GSIV  0x19
#define PLATFORM_OVERRIDE_PE14_TRBE_INTR   0x0

#define PLATFORM_OVERRIDE_PE15_INDEX       0xF
#define PLATFORM_OVERRIDE_PE15_MPIDR       0xF0000
#define PLATFORM_OVERRIDE_PE15_PMU_GSIV    0x17
#define PLATFORM_OVERRIDE_PE15_GMAIN_GSIV  0x19
#define PLATFORM_OVERRIDE_PE15_TRBE_INTR   0x0

/* -----------------------------  SMBIOS processor info  --------------------------- */
#define PLATFORM_OVERRIDE_SMBIOS_SLOT_COUNT        0x1    /* Number of SMBIOS CPU slot entries    */
#define PLATFROM_OVERRIDE_SMBIOS_SLOT0_FAMILY      0x118  /* Slot0: CPU family code               */
#define PLATFROM_OVERRIDE_SMBIOS_SLOT0_CORE_COUNT  16     /* Slot0: cores reported in SMBIOS      */


/* -----------------------------------  GIC info  ----------------------------------- */
#define PLATFORM_OVERRIDE_GIC_VERSION        0x3     /* GIC arch version                         */
#define PLATFORM_OVERRIDE_GICC_COUNT         16      /* Number of CPU interface structures       */
#define PLATFORM_OVERRIDE_GICD_COUNT         0x1     /* Number of Distributor blocks             */
#define PLATFORM_OVERRIDE_GICR_GICRD_COUNT   0x1     /* Number of RD ranges present              */
#define PLATFORM_OVERRIDE_GICC_GICRD_COUNT   0x0     /* Number of RD - if GICR str not available */
#define PLATFORM_OVERRIDE_GICITS_COUNT       0x6     /* Number of ITS blocks                     */
#define PLATFORM_OVERRIDE_GICH_COUNT         0x1     /* Hypervisor interface count               */
#define PLATFORM_OVERRIDE_GICMSIFRAME_COUNT  0x0     /* MSI frames implemented                   */
#define PLATFORM_OVERRIDE_NONGIC_COUNT       0x0     /* Non-GIC interrupt controllers            */

#define PLATFORM_OVERRIDE_GICC_BASE          0x30000000 /* Base of CPU interface (GICC)          */
#define PLATFORM_OVERRIDE_GICD_BASE          0x30000000 /* Base of distributor (GICD)            */
#define PLATFORM_OVERRIDE_GICR_GICRD_BASE    0x301C0000 /* Base of redistributors (GICR)         */
#define PLATFORM_OVERRIDE_GICC_GICRD_BASE    0x0        /* Base of redistributors                */
#define PLATFORM_OVERRIDE_GICH_BASE          0x2C010000 /* Base of GICH interface                */

#define PLATFORM_OVERRIDE_GICITS0_ID         0          /* ITS0: identifier                      */
#define PLATFORM_OVERRIDE_GICITS0_BASE       0x30040000 /* ITS0: base address                    */
#define PLATFORM_OVERRIDE_GICITS1_ID         0x1
#define PLATFORM_OVERRIDE_GICITS1_BASE       0x30080000
#define PLATFORM_OVERRIDE_GICITS2_ID         0x2
#define PLATFORM_OVERRIDE_GICITS2_BASE       0x300C0000
#define PLATFORM_OVERRIDE_GICITS3_ID         0x3
#define PLATFORM_OVERRIDE_GICITS3_BASE       0x30100000
#define PLATFORM_OVERRIDE_GICITS4_ID         0x4
#define PLATFORM_OVERRIDE_GICITS4_BASE       0x30140000
#define PLATFORM_OVERRIDE_GICITS5_ID         0x5
#define PLATFORM_OVERRIDE_GICITS5_BASE       0x30180000

#define PLATFORM_OVERRIDE_GICCIRD_LENGTH     0x0        /* Length of GICC RD discovery           */
#define PLATFORM_OVERRIDE_GICRIRD_LENGTH     0x1000000  /* Length of GICR RD discovery range     */
#define PLATFORM_OVERRIDE_GICRD_COUNT        PLATFORM_OVERRIDE_GICR_GICRD_COUNT + \
                                             PLATFORM_OVERRIDE_GICC_GICRD_COUNT

/*
 *Secure EL1 timer Flags, Non-Secure EL1 timer Flags, EL2 timer Flags,
 *and Virtual timer Flags all can have the same definition as follows.
 */
#define INTERRUPT_IS_LEVEL_TRIGGERED 0x0
#define INTERRUPT_IS_EDGE_TRIGGERED  0x1
#define INTERRUPT_IS_ACTIVE_HIGH     0x0
#define INTERRUPT_IS_ACTIVE_LOW      0x1

/* ----------------------------------  Timer info  ----------------------------------- */
#define TIMER_MODE                   INTERRUPT_IS_LEVEL_TRIGGERED     /*      Mode encoding      */
#define TIMER_POLARITY               INTERRUPT_IS_ACTIVE_LOW          /*      Polarity encoding  */
#define TIMER_IS_SECURE              0x1             /* Secure timer implemented                 */
#define TIMER_IS_ALWAYS_ON_CAPABLE   0x1             /* Always On Capability                     */

/* Timer platform config parameters */
#define PLATFORM_OVERRIDE_S_EL1_TIMER_FLAGS     ((TIMER_POLARITY << 1) | (TIMER_MODE << 0))
#define PLATFORM_OVERRIDE_NS_EL1_TIMER_FLAGS    ((TIMER_POLARITY << 1) | (TIMER_MODE << 0))
#define PLATFORM_OVERRIDE_NS_EL2_TIMER_FLAGS    ((TIMER_POLARITY << 1) | (TIMER_MODE << 0))
#define PLATFORM_OVERRIDE_VIRTUAL_TIMER_FLAGS   ((TIMER_POLARITY << 1) | (TIMER_MODE << 0))
#define PLATFORM_OVERRIDE_S_EL1_TIMER_GSIV      0x1D    /* S-EL1 timer interrupt                 */
#define PLATFORM_OVERRIDE_NS_EL1_TIMER_GSIV     0x1E    /* NS-EL1 timer interrupt                */
#define PLATFORM_OVERRIDE_NS_EL2_TIMER_GSIV     0x1A    /* EL2 physical timer interrupt          */
#define PLATFORM_OVERRIDE_VIRTUAL_TIMER_GSIV    0x1B    /* Virtual timer interrupt               */
#define PLATFORM_OVERRIDE_EL2_VIR_TIMER_GSIV    28      /* EL2 virtual timer interrupt (GSIV)    */
#define PLATFORM_OVERRIDE_PLATFORM_TIMER_COUNT  0x2     /* Number of system physical timers      */

#define PLATFORM_OVERRIDE_SYS_TIMER_TYPE     0x2001                           /* timer-type field*/
#define PLATFORM_OVERRIDE_TIMER_TYPE         PLATFORM_OVERRIDE_SYS_TIMER_TYPE /* Alias           */
#define PLATFORM_OVERRIDE_TIMER_COUNT        0x2        /* Frames in system counter block        */
#define PLATFORM_OVERRIDE_TIMER_CNTCTL_BASE  0x2a810000 /* CNTCTL base address                   */

/*  Counter frame info */
#define PLATFORM_OVERRIDE_TIMER_FRAME_NUM_0    0                   /* Frame index                */
#define PLATFORM_OVERRIDE_TIMER_CNTBASE_0      0x2A830000          /* CNTBase_N of frame0        */
#define PLATFORM_OVERRIDE_TIMER_CNTEL0BASE_0   0xFFFFFFFFFFFFFFFF  /* CNTBase_EL0_N              */
#define PLATFORM_OVERRIDE_TIMER_GSIV_0         0x6D                /* Physical timer GSIV        */
#define PLATFORM_OVERRIDE_TIMER_VIRT_GSIV_0    0x0                 /* Virtual timer GSIV         */
#define PLATFORM_OVERRIDE_TIMER_PHY_FLAGS_0    0x0                 /* Physical timer flag bundle */
#define PLATFORM_OVERRIDE_TIMER_VIRT_FLAGS_0   0x0                 /* Virtual timer flag bundle  */
#define PLATFORM_OVERRIDE_TIMER_CMN_FLAGS_0     ((TIMER_IS_ALWAYS_ON_CAPABLE << 1) | \
                                                 (!TIMER_IS_SECURE << 0))
#define PLATFORM_OVERRIDE_TIMER_FLAGS_0         ((PLATFORM_OVERRIDE_TIMER_CMN_FLAGS_0 << 16) | \
                                                 (PLATFORM_OVERRIDE_TIMER_VIRT_FLAGS_0 << 8) | \
                                                 (PLATFORM_OVERRIDE_TIMER_PHY_FLAGS_0))

#define PLATFORM_OVERRIDE_TIMER_FRAME_NUM_1     1
#define PLATFORM_OVERRIDE_TIMER_CNTBASE_1       0x2a820000
#define PLATFORM_OVERRIDE_TIMER_CNTEL0BASE_1    0xFFFFFFFFFFFFFFFF
#define PLATFORM_OVERRIDE_TIMER_GSIV_1          0x6c
#define PLATFORM_OVERRIDE_TIMER_VIRT_GSIV_1     0x0
#define PLATFORM_OVERRIDE_TIMER_PHY_FLAGS_1     0x0
#define PLATFORM_OVERRIDE_TIMER_VIRT_FLAGS_1    0x0
#define PLATFORM_OVERRIDE_TIMER_CMN_FLAGS_1     ((TIMER_IS_ALWAYS_ON_CAPABLE << 1) | \
                                                 (TIMER_IS_SECURE << 0))
#define PLATFORM_OVERRIDE_TIMER_FLAGS_1         ((PLATFORM_OVERRIDE_TIMER_CMN_FLAGS_1 << 16) | \
                                                 (PLATFORM_OVERRIDE_TIMER_VIRT_FLAGS_1 << 8) | \
                                                 (PLATFORM_OVERRIDE_TIMER_PHY_FLAGS_1))

#define PLATFORM_BM_TIMER_CNTFRQ         0x5F5E100   /* System counter frequency (100 MHz)       */

/* -----------------------------  Watchdog info  -------------------------------- */
#define WD_MODE          INTERRUPT_IS_LEVEL_TRIGGERED     /* Trigger mode                */
#define WD_POLARITY      INTERRUPT_IS_ACTIVE_HIGH         /* Signal polarity             */
#define WD_IS_SECURE     0x1                /* Implemented as secure watchdog            */

#define PLATFORM_OVERRIDE_WD_TIMER_COUNT    0x2          /* Number of watchdogs                  */
#define PLATFORM_OVERRIDE_WD_REFRESH_BASE   0x2A450000   /* Refresh register base                */
#define PLATFORM_OVERRIDE_WD_CTRL_BASE      0x2A440000   /* Control register base                */

#define PLATFORM_OVERRIDE_WD_GSIV_0         0x6E         /* Watchdog0 interrupt line             */
#define PLATFORM_OVERRIDE_WD_FLAGS_0        ((!WD_IS_SECURE << 2) | (WD_POLARITY << 1) | \
                                             (WD_MODE << 0))

#define PLATFORM_OVERRIDE_WD_GSIV_1         0x6F         /* Watchdog1 interrupt line             */
#define PLATFORM_OVERRIDE_WD_FLAGS_1        ((WD_IS_SECURE << 2) | (WD_POLARITY << 1) | \
                                             (WD_MODE << 0))

/* -----------------------------  PCIe root complex  ------------------------------ */
#define PLATFORM_OVERRIDE_NUM_ECAM                1  /* Number of ECAM regions defined           */
#define PLATFORM_MAX_HB_COUNT                     1  /* Host-brigdes max number                  */

/* Offset from the memory range to be accesed
 * Modify this macro w.r.t to the requirement */
#define MEM_OFFSET_SMALL   0x10
#define MEM_OFFSET_MEDIUM  0x1000

/* Platform config parameters for ECAM_0 */
#define PLATFORM_OVERRIDE_PCIE_ECAM_BASE_ADDR_0   0x4000000000  /* ECAM0: base address           */
#define PLATFORM_OVERRIDE_PCIE_SEGMENT_GRP_NUM_0  0x0           /* ECAM0: PCI segment number     */
#define PLATFORM_OVERRIDE_PCIE_START_BUS_NUM_0    0x0           /* ECAM0: first bus in range     */
#define PLATFORM_OVERRIDE_PCIE_END_BUS_NUM_0      0x8           /* ECAM0: last bus in range      */

#define PLATFORM_OVERRIDE_PCIE_ECAM0_HB_COUNT       1           /* ECAM0: Host bridge count      */
#define PLATFORM_OVERRIDE_PCIE_ECAM0_SEG_NUM        0x0         /* ECAM0: segment number for HB  */
#define PLATFORM_OVERRIDE_PCIE_ECAM0_START_BUS_NUM  0x0         /* ECAM0 HB bus start            */
#define PLATFORM_OVERRIDE_PCIE_ECAM0_END_BUS_NUM    0x8         /* ECAM0 HB bus end              */

#define PLATFORM_OVERRIDE_PCIE_ECAM0_EP_BAR64     0x4040000000 /* ECAM0: MMIO base 64bit EP BAR */
#define PLATFORM_OVERRIDE_PCIE_ECAM0_RP_BAR64     0x4040100000 /* ECAM0: MMIO base 64bit RP BAR */
#define PLATFORM_OVERRIDE_PCIE_ECAM0_EP_NPBAR32   0x60200000 /* ECAM0: MMIO base 32bit EP N-P BAR */
#define PLATFORM_OVERRIDE_PCIE_ECAM0_EP_PBAR32    0x60000000 /* ECAM0: MMIO base 32bit EP P BAR   */
#define PLATFORM_OVERRIDE_PCIE_ECAM0_RP_BAR32     0x60800000 /* ECAM0: MMIO base 32bit RP BAR     */

/* Placeholder in case of multiple ECAM regions
#define PLATFORM_OVERRIDE_PCIE_ECAM_BASE_ADDR_1   0x4000900000
#define PLATFORM_OVERRIDE_PCIE_SEGMENT_GRP_NUM_1  0x0
#define PLATFORM_OVERRIDE_PCIE_START_BUS_NUM_1    0x9
#define PLATFORM_OVERRIDE_PCIE_END_BUS_NUM_1      0xB

#define PLATFORM_OVERRIDE_PCIE_ECAM1_HB_COUNT       1
#define PLATFORM_OVERRIDE_PCIE_ECAM1_SEG_NUM        0x0
#define PLATFORM_OVERRIDE_PCIE_ECAM1_START_BUS_NUM  0x9
#define PLATFORM_OVERRIDE_PCIE_ECAM1_END_BUS_NUM    0xB
#define PLATFORM_OVERRIDE_PCIE_ECAM1_EP_BAR64       0x4080000000
#define PLATFORM_OVERRIDE_PCIE_ECAM1_RP_BAR64       0x4080100000
#define PLATFORM_OVERRIDE_PCIE_ECAM1_EP_NPBAR32     0x70300000
#define PLATFORM_OVERRIDE_PCIE_ECAM1_EP_PBAR32      0x70000000
#define PLATFORM_OVERRIDE_PCIE_ECAM1_RP_BAR32       0x70940000
*/

#define PLATFORM_BM_OVERRIDE_PCIE_MAX_BUS      0x9    /* Max bus walked by bare-metal tests      */
#define PLATFORM_BM_OVERRIDE_PCIE_MAX_DEV      32     /* Max device per bus checked              */
#define PLATFORM_BM_OVERRIDE_PCIE_MAX_FUNC     8      /* Max function per device checked         */
#define PLATFORM_BM_OVERRIDE_MAX_IRQ_CNT       0xFFFF /* Max IRQs any device may raise           */

#define PLATFORM_OVERRIDE_TIMEOUT              0      /* Override default wakeup timeout         */

/* Generic timeout helpers for bare-metal tests */
#define PLATFORM_BM_OVERRIDE_TIMEOUT_LARGE         0x10000
#define PLATFORM_BM_OVERRIDE_TIMEOUT_MEDIUM        0x1000
#define PLATFORM_BM_OVERRIDE_TIMEOUT_SMALL         0x10

/* ---------------------  PCIe device description table  --------------------- */

#define PLATFORM_PCIE_NUM_ENTRIES        21   /* Number of devices in PCIe hierarchy             */
#define PLATFORM_PCIE_P2P_NOT_SUPPORTED  1    /* PCIe hierarchy forbids peer-to-peer transfers   */

#define PLATFORM_PCIE_DEV0_CLASSCODE     0x6040000   /* DEV0: Device class code                  */
#define PLATFORM_PCIE_DEV0_VENDOR_ID     0x13B5      /* DEV0: Device vendor ID                   */
#define PLATFORM_PCIE_DEV0_DEV_ID        0xDEF       /* DEV0: Device Dev ID                      */
#define PLATFORM_PCIE_DEV0_BUS_NUM          0        /* DEV0: Bus number                         */
#define PLATFORM_PCIE_DEV0_DEV_NUM          1        /* DEV0: Device number                      */
#define PLATFORM_PCIE_DEV0_FUNC_NUM         0        /* DEV0: Function number                    */
#define PLATFORM_PCIE_DEV0_SEG_NUM          0        /* DEV0: Segment                            */
#define PLATFORM_PCIE_DEV0_DMA_SUPPORT      1        /* DEV0: Device supports DMA                */
#define PLATFORM_PCIE_DEV0_DMA_COHERENT     0        /* DEV0: cache-coherent DMA                 */
#define PLATFORM_PCIE_DEV0_P2P_SUPPORT      0        /* DEV0: Supports peer-to-peer              */
#define PLATFORM_PCIE_DEV0_DMA_64BIT        0        /* DEV0: 64-bit DMA addressing bbbb         */
#define PLATFORM_PCIE_DEV0_BEHIND_SMMU      1        /* DEV0: Path includes SMMU                 */
#define PLATFORM_PCIE_DEV0_ATC_SUPPORT      0        /* DEV0: Address-translation cache support  */

#define PLATFORM_PCIE_DEV1_CLASSCODE     0x6040000
#define PLATFORM_PCIE_DEV1_VENDOR_ID     0x13B5
#define PLATFORM_PCIE_DEV1_DEV_ID        0xDEF
#define PLATFORM_PCIE_DEV1_BUS_NUM       0
#define PLATFORM_PCIE_DEV1_DEV_NUM       2
#define PLATFORM_PCIE_DEV1_FUNC_NUM      0
#define PLATFORM_PCIE_DEV1_SEG_NUM       0
#define PLATFORM_PCIE_DEV1_DMA_SUPPORT   1
#define PLATFORM_PCIE_DEV1_DMA_COHERENT  0
#define PLATFORM_PCIE_DEV1_P2P_SUPPORT   0
#define PLATFORM_PCIE_DEV1_DMA_64BIT     0
#define PLATFORM_PCIE_DEV1_BEHIND_SMMU   1
#define PLATFORM_PCIE_DEV1_ATC_SUPPORT   0

#define PLATFORM_PCIE_DEV2_CLASSCODE     0x6040000
#define PLATFORM_PCIE_DEV2_VENDOR_ID     0x13B5
#define PLATFORM_PCIE_DEV2_DEV_ID        0xDEF
#define PLATFORM_PCIE_DEV2_BUS_NUM       0
#define PLATFORM_PCIE_DEV2_DEV_NUM       3
#define PLATFORM_PCIE_DEV2_FUNC_NUM      0
#define PLATFORM_PCIE_DEV2_SEG_NUM       0
#define PLATFORM_PCIE_DEV2_DMA_SUPPORT   1
#define PLATFORM_PCIE_DEV2_DMA_COHERENT  0
#define PLATFORM_PCIE_DEV2_P2P_SUPPORT   0
#define PLATFORM_PCIE_DEV2_DMA_64BIT     0
#define PLATFORM_PCIE_DEV2_BEHIND_SMMU   1
#define PLATFORM_PCIE_DEV2_ATC_SUPPORT   0

#define PLATFORM_PCIE_DEV3_CLASSCODE     0x6040000
#define PLATFORM_PCIE_DEV3_VENDOR_ID     0x13B5
#define PLATFORM_PCIE_DEV3_DEV_ID        0x0DEF
#define PLATFORM_PCIE_DEV3_BUS_NUM       0
#define PLATFORM_PCIE_DEV3_DEV_NUM       4
#define PLATFORM_PCIE_DEV3_FUNC_NUM      0
#define PLATFORM_PCIE_DEV3_SEG_NUM       0
#define PLATFORM_PCIE_DEV3_DMA_SUPPORT   1
#define PLATFORM_PCIE_DEV3_DMA_COHERENT  0
#define PLATFORM_PCIE_DEV3_P2P_SUPPORT   0
#define PLATFORM_PCIE_DEV3_DMA_64BIT     0
#define PLATFORM_PCIE_DEV3_BEHIND_SMMU   1
#define PLATFORM_PCIE_DEV3_ATC_SUPPORT   0

#define PLATFORM_PCIE_DEV4_CLASSCODE     0x1060101
#define PLATFORM_PCIE_DEV4_VENDOR_ID     0x0ABC
#define PLATFORM_PCIE_DEV4_DEV_ID        0xACED
#define PLATFORM_PCIE_DEV4_BUS_NUM       1
#define PLATFORM_PCIE_DEV4_DEV_NUM       0
#define PLATFORM_PCIE_DEV4_FUNC_NUM      0
#define PLATFORM_PCIE_DEV4_SEG_NUM       0
#define PLATFORM_PCIE_DEV4_DMA_SUPPORT   1
#define PLATFORM_PCIE_DEV4_DMA_COHERENT  0
#define PLATFORM_PCIE_DEV4_P2P_SUPPORT   0
#define PLATFORM_PCIE_DEV4_DMA_64BIT     0
#define PLATFORM_PCIE_DEV4_BEHIND_SMMU   1
#define PLATFORM_PCIE_DEV4_ATC_SUPPORT   0

#define PLATFORM_PCIE_DEV5_CLASSCODE     0xED000000
#define PLATFORM_PCIE_DEV5_VENDOR_ID     0x13B5
#define PLATFORM_PCIE_DEV5_DEV_ID        0xED01
#define PLATFORM_PCIE_DEV5_BUS_NUM       2
#define PLATFORM_PCIE_DEV5_DEV_NUM       0
#define PLATFORM_PCIE_DEV5_FUNC_NUM      0
#define PLATFORM_PCIE_DEV5_SEG_NUM       0
#define PLATFORM_PCIE_DEV5_DMA_SUPPORT   1
#define PLATFORM_PCIE_DEV5_DMA_COHERENT  0
#define PLATFORM_PCIE_DEV5_P2P_SUPPORT   0
#define PLATFORM_PCIE_DEV5_DMA_64BIT     0
#define PLATFORM_PCIE_DEV5_BEHIND_SMMU   1
#define PLATFORM_PCIE_DEV5_ATC_SUPPORT   0

#define PLATFORM_PCIE_DEV6_CLASSCODE     0xED000000
#define PLATFORM_PCIE_DEV6_VENDOR_ID     0x13B5
#define PLATFORM_PCIE_DEV6_DEV_ID        0xED01
#define PLATFORM_PCIE_DEV6_BUS_NUM       2
#define PLATFORM_PCIE_DEV6_DEV_NUM       0
#define PLATFORM_PCIE_DEV6_FUNC_NUM      1
#define PLATFORM_PCIE_DEV6_SEG_NUM       0
#define PLATFORM_PCIE_DEV6_DMA_SUPPORT   1
#define PLATFORM_PCIE_DEV6_DMA_COHERENT  0
#define PLATFORM_PCIE_DEV6_P2P_SUPPORT   0
#define PLATFORM_PCIE_DEV6_DMA_64BIT     0
#define PLATFORM_PCIE_DEV6_BEHIND_SMMU   0
#define PLATFORM_PCIE_DEV6_ATC_SUPPORT   0

#define PLATFORM_PCIE_DEV7_CLASSCODE     0x6040000
#define PLATFORM_PCIE_DEV7_VENDOR_ID     0x13B5
#define PLATFORM_PCIE_DEV7_DEV_ID        0xDEF
#define PLATFORM_PCIE_DEV7_BUS_NUM       3
#define PLATFORM_PCIE_DEV7_DEV_NUM       0
#define PLATFORM_PCIE_DEV7_FUNC_NUM      0
#define PLATFORM_PCIE_DEV7_SEG_NUM       0
#define PLATFORM_PCIE_DEV7_DMA_SUPPORT   1
#define PLATFORM_PCIE_DEV7_DMA_COHERENT  0
#define PLATFORM_PCIE_DEV7_P2P_SUPPORT   0
#define PLATFORM_PCIE_DEV7_DMA_64BIT     0
#define PLATFORM_PCIE_DEV7_BEHIND_SMMU   0
#define PLATFORM_PCIE_DEV7_ATC_SUPPORT   0

#define PLATFORM_PCIE_DEV8_CLASSCODE    0x6040000
#define PLATFORM_PCIE_DEV8_VENDOR_ID    0x13B5
#define PLATFORM_PCIE_DEV8_DEV_ID       0xDEF
#define PLATFORM_PCIE_DEV8_BUS_NUM      4
#define PLATFORM_PCIE_DEV8_DEV_NUM      0
#define PLATFORM_PCIE_DEV8_FUNC_NUM     0
#define PLATFORM_PCIE_DEV8_SEG_NUM      0
#define PLATFORM_PCIE_DEV8_DMA_SUPPORT  1
#define PLATFORM_PCIE_DEV8_DMA_COHERENT 0
#define PLATFORM_PCIE_DEV8_P2P_SUPPORT  0
#define PLATFORM_PCIE_DEV8_DMA_64BIT    0
#define PLATFORM_PCIE_DEV8_BEHIND_SMMU  0
#define PLATFORM_PCIE_DEV8_ATC_SUPPORT  0

#define PLATFORM_PCIE_DEV9_CLASSCODE    0x6040000
#define PLATFORM_PCIE_DEV9_VENDOR_ID    0x13B5
#define PLATFORM_PCIE_DEV9_DEV_ID       0xDEF
#define PLATFORM_PCIE_DEV9_BUS_NUM      4
#define PLATFORM_PCIE_DEV9_DEV_NUM      1
#define PLATFORM_PCIE_DEV9_FUNC_NUM     0
#define PLATFORM_PCIE_DEV9_SEG_NUM      0
#define PLATFORM_PCIE_DEV9_DMA_SUPPORT  1
#define PLATFORM_PCIE_DEV9_DMA_COHERENT 0
#define PLATFORM_PCIE_DEV9_P2P_SUPPORT  0
#define PLATFORM_PCIE_DEV9_DMA_64BIT    0
#define PLATFORM_PCIE_DEV9_BEHIND_SMMU  0
#define PLATFORM_PCIE_DEV9_ATC_SUPPORT  0

#define PLATFORM_PCIE_DEV10_CLASSCODE    0x6040000
#define PLATFORM_PCIE_DEV10_VENDOR_ID    0x13B5
#define PLATFORM_PCIE_DEV10_DEV_ID       0xDEF
#define PLATFORM_PCIE_DEV10_BUS_NUM      4
#define PLATFORM_PCIE_DEV10_DEV_NUM      2
#define PLATFORM_PCIE_DEV10_FUNC_NUM     0
#define PLATFORM_PCIE_DEV10_SEG_NUM      0
#define PLATFORM_PCIE_DEV10_DMA_SUPPORT  1
#define PLATFORM_PCIE_DEV10_DMA_COHERENT 0
#define PLATFORM_PCIE_DEV10_P2P_SUPPORT  0
#define PLATFORM_PCIE_DEV10_DMA_64BIT    0
#define PLATFORM_PCIE_DEV10_BEHIND_SMMU  0
#define PLATFORM_PCIE_DEV10_ATC_SUPPORT  0

#define PLATFORM_PCIE_DEV11_CLASSCODE    0x1060101
#define PLATFORM_PCIE_DEV11_VENDOR_ID    0x0ABC
#define PLATFORM_PCIE_DEV11_DEV_ID       0xACED
#define PLATFORM_PCIE_DEV11_BUS_NUM      5
#define PLATFORM_PCIE_DEV11_DEV_NUM      0
#define PLATFORM_PCIE_DEV11_FUNC_NUM     0
#define PLATFORM_PCIE_DEV11_SEG_NUM      0
#define PLATFORM_PCIE_DEV11_DMA_SUPPORT  1
#define PLATFORM_PCIE_DEV11_DMA_COHERENT 0
#define PLATFORM_PCIE_DEV11_P2P_SUPPORT  0
#define PLATFORM_PCIE_DEV11_DMA_64BIT    0
#define PLATFORM_PCIE_DEV11_BEHIND_SMMU  0
#define PLATFORM_PCIE_DEV11_ATC_SUPPORT  0

#define PLATFORM_PCIE_DEV12_CLASSCODE    0xED000000
#define PLATFORM_PCIE_DEV12_VENDOR_ID    0x13B5
#define PLATFORM_PCIE_DEV12_DEV_ID       0xED01
#define PLATFORM_PCIE_DEV12_BUS_NUM      6
#define PLATFORM_PCIE_DEV12_DEV_NUM      0
#define PLATFORM_PCIE_DEV12_FUNC_NUM     0
#define PLATFORM_PCIE_DEV12_SEG_NUM      0
#define PLATFORM_PCIE_DEV12_DMA_SUPPORT  1
#define PLATFORM_PCIE_DEV12_DMA_COHERENT 0
#define PLATFORM_PCIE_DEV12_P2P_SUPPORT  0
#define PLATFORM_PCIE_DEV12_DMA_64BIT    0
#define PLATFORM_PCIE_DEV12_BEHIND_SMMU  0
#define PLATFORM_PCIE_DEV12_ATC_SUPPORT  0

#define PLATFORM_PCIE_DEV13_CLASSCODE    0xED000000
#define PLATFORM_PCIE_DEV13_VENDOR_ID    0x13B5
#define PLATFORM_PCIE_DEV13_DEV_ID       0xED01
#define PLATFORM_PCIE_DEV13_BUS_NUM      6
#define PLATFORM_PCIE_DEV13_DEV_NUM      0
#define PLATFORM_PCIE_DEV13_FUNC_NUM     7
#define PLATFORM_PCIE_DEV13_SEG_NUM      0
#define PLATFORM_PCIE_DEV13_DMA_SUPPORT  1
#define PLATFORM_PCIE_DEV13_DMA_COHERENT 0
#define PLATFORM_PCIE_DEV13_P2P_SUPPORT  0
#define PLATFORM_PCIE_DEV13_DMA_64BIT    0
#define PLATFORM_PCIE_DEV13_BEHIND_SMMU  0
#define PLATFORM_PCIE_DEV13_ATC_SUPPORT  0

#define PLATFORM_PCIE_DEV14_CLASSCODE    0xFF000000
#define PLATFORM_PCIE_DEV14_VENDOR_ID    0x13B5
#define PLATFORM_PCIE_DEV14_DEV_ID       0xFF80
#define PLATFORM_PCIE_DEV14_BUS_NUM      7
#define PLATFORM_PCIE_DEV14_DEV_NUM      0
#define PLATFORM_PCIE_DEV14_FUNC_NUM     0
#define PLATFORM_PCIE_DEV14_SEG_NUM      0
#define PLATFORM_PCIE_DEV14_DMA_SUPPORT  1
#define PLATFORM_PCIE_DEV14_DMA_COHERENT 0
#define PLATFORM_PCIE_DEV14_P2P_SUPPORT  0
#define PLATFORM_PCIE_DEV14_DMA_64BIT    0
#define PLATFORM_PCIE_DEV14_BEHIND_SMMU  0
#define PLATFORM_PCIE_DEV14_ATC_SUPPORT  0

#define PLATFORM_PCIE_DEV15_CLASSCODE    0xFF000000
#define PLATFORM_PCIE_DEV15_VENDOR_ID    0x13B5
#define PLATFORM_PCIE_DEV15_DEV_ID       0xFF80
#define PLATFORM_PCIE_DEV15_BUS_NUM      7
#define PLATFORM_PCIE_DEV15_DEV_NUM      0
#define PLATFORM_PCIE_DEV15_FUNC_NUM     7
#define PLATFORM_PCIE_DEV15_SEG_NUM      0
#define PLATFORM_PCIE_DEV15_DMA_SUPPORT  1
#define PLATFORM_PCIE_DEV15_DMA_COHERENT 0
#define PLATFORM_PCIE_DEV15_P2P_SUPPORT  0
#define PLATFORM_PCIE_DEV15_DMA_64BIT    0
#define PLATFORM_PCIE_DEV15_BEHIND_SMMU  0
#define PLATFORM_PCIE_DEV15_ATC_SUPPORT  0

#define PLATFORM_PCIE_DEV16_CLASSCODE    0xFF000000
#define PLATFORM_PCIE_DEV16_VENDOR_ID    0x13B5
#define PLATFORM_PCIE_DEV16_DEV_ID       0xFF80
#define PLATFORM_PCIE_DEV16_BUS_NUM      8
#define PLATFORM_PCIE_DEV16_DEV_NUM      0
#define PLATFORM_PCIE_DEV16_FUNC_NUM     0
#define PLATFORM_PCIE_DEV16_SEG_NUM      0
#define PLATFORM_PCIE_DEV16_DMA_SUPPORT  1
#define PLATFORM_PCIE_DEV16_DMA_COHERENT 0
#define PLATFORM_PCIE_DEV16_P2P_SUPPORT  0
#define PLATFORM_PCIE_DEV16_DMA_64BIT    0
#define PLATFORM_PCIE_DEV16_BEHIND_SMMU  0
#define PLATFORM_PCIE_DEV16_ATC_SUPPORT  0

#define PLATFORM_PCIE_DEV17_CLASSCODE    0xFF000000
#define PLATFORM_PCIE_DEV17_VENDOR_ID    0x13B5
#define PLATFORM_PCIE_DEV17_DEV_ID       0xFF80
#define PLATFORM_PCIE_DEV17_BUS_NUM      8
#define PLATFORM_PCIE_DEV17_DEV_NUM      0
#define PLATFORM_PCIE_DEV17_FUNC_NUM     1
#define PLATFORM_PCIE_DEV17_SEG_NUM      0
#define PLATFORM_PCIE_DEV17_DMA_SUPPORT  1
#define PLATFORM_PCIE_DEV17_DMA_COHERENT 0
#define PLATFORM_PCIE_DEV17_P2P_SUPPORT  0
#define PLATFORM_PCIE_DEV17_DMA_64BIT    0
#define PLATFORM_PCIE_DEV17_BEHIND_SMMU  0
#define PLATFORM_PCIE_DEV17_ATC_SUPPORT  0

#define PLATFORM_PCIE_DEV18_CLASSCODE    0xED000001
#define PLATFORM_PCIE_DEV18_VENDOR_ID    0x13B5
#define PLATFORM_PCIE_DEV18_DEV_ID       0xED01
#define PLATFORM_PCIE_DEV18_BUS_NUM      0
#define PLATFORM_PCIE_DEV18_DEV_NUM      0x0F
#define PLATFORM_PCIE_DEV18_FUNC_NUM     0
#define PLATFORM_PCIE_DEV18_SEG_NUM      0
#define PLATFORM_PCIE_DEV18_DMA_SUPPORT  0
#define PLATFORM_PCIE_DEV18_DMA_COHERENT 0
#define PLATFORM_PCIE_DEV18_P2P_SUPPORT  1
#define PLATFORM_PCIE_DEV18_DMA_64BIT    0
#define PLATFORM_PCIE_DEV18_BEHIND_SMMU  1
#define PLATFORM_PCIE_DEV18_ATC_SUPPORT  0

#define PLATFORM_PCIE_DEV19_CLASSCODE    0x1060101
#define PLATFORM_PCIE_DEV19_VENDOR_ID    0x0ABC
#define PLATFORM_PCIE_DEV19_DEV_ID       0xACED
#define PLATFORM_PCIE_DEV19_BUS_NUM      0
#define PLATFORM_PCIE_DEV19_DEV_NUM      0x1E
#define PLATFORM_PCIE_DEV19_FUNC_NUM     0
#define PLATFORM_PCIE_DEV19_SEG_NUM      0
#define PLATFORM_PCIE_DEV19_DMA_SUPPORT  0
#define PLATFORM_PCIE_DEV19_DMA_COHERENT 0
#define PLATFORM_PCIE_DEV19_P2P_SUPPORT  1
#define PLATFORM_PCIE_DEV19_DMA_64BIT    0
#define PLATFORM_PCIE_DEV19_BEHIND_SMMU  1
#define PLATFORM_PCIE_DEV19_ATC_SUPPORT  0

#define PLATFORM_PCIE_DEV20_CLASSCODE    0xED000000
#define PLATFORM_PCIE_DEV20_VENDOR_ID    0x13B5
#define PLATFORM_PCIE_DEV20_DEV_ID       0xED01
#define PLATFORM_PCIE_DEV20_BUS_NUM      0
#define PLATFORM_PCIE_DEV20_DEV_NUM      0x1F
#define PLATFORM_PCIE_DEV20_FUNC_NUM     0
#define PLATFORM_PCIE_DEV20_SEG_NUM      0
#define PLATFORM_PCIE_DEV20_DMA_SUPPORT  0
#define PLATFORM_PCIE_DEV20_DMA_COHERENT 0
#define PLATFORM_PCIE_DEV20_P2P_SUPPORT  1
#define PLATFORM_PCIE_DEV20_DMA_64BIT    0
#define PLATFORM_PCIE_DEV20_BEHIND_SMMU  1
#define PLATFORM_PCIE_DEV20_ATC_SUPPORT  0

/* PCIe RAS Related Offset */

#define RAS_OFFSET     0x10000                    /* Used in PAL APIs. Modify acc to the API impl */
#define CTRL_OFFSET    0x08                       /* Used in PAL APIs. Modify acc to the API impl */
#define STATUS_OFFSET  0x10                       /* Used in PAL APIs. Modify acc to the API impl */

/* -----------------------------  Peripheral info  ----------------------------- */
#define PLATFORM_OVERRIDE_PERIPHERAL_COUNT 2  //UART + USB + SATA

#define UART_ADDRESS                     0xF98DFE18  /* Non-zero if UART is present              */
#define BASE_ADDRESS_ADDRESS             0x2A400000  /* UART MMIO base                           */
#define INTERFACE_TYPE                   8           /* UART interface type                      */
#define UART_BAUD_RATE                   0x7        /* Baud-rate enum (as per SPCR table)        */
#define UART_BAUD_RATE_BPS               115200     /* Actual bits-per-second                    */
#define UART_CLK_IN_HZ                   24000000   /* Reference clock input                     */
#define UART_GLOBAL_SYSTEM_INTERRUPT     0x70       /* UART GSIV                                 */
#define UART_PCI_DEVICE_ID               0xFFFF     /* Device ID - If UART is on PCIe            */
#define UART_PCI_VENDOR_ID               0xFFFF     /* Vendor ID - If UART is on PCIe            */
#define UART_PCI_BUS_NUMBER              0x0
#define UART_PCI_DEV_NUMBER              0x0
#define UART_PCI_FUNC_NUMBER             0x0
#define UART_PCI_FLAGS                   0x0
#define UART_PCI_SEGMENT                 0x0

/* IOVIRT platform config parameters */
#define IOVIRT_ADDRESS                0xF98DEB18  /* Non-zero if IORT is present                 */
#define IORT_NODE_COUNT               9           /* Total nodes in IORT                         */
#define NUM_ITS_COUNT                 4
#define IOVIRT_ITS_COUNT              1           /* Number of ITS nodes in IORT                 */
#define IOVIRT_SMMUV3_COUNT           4           /* Number of SMMUv3 nodes                      */
#define IOVIRT_RC_COUNT               1           /* Root-complex nodes present                  */
#define IOVIRT_SMMUV2_COUNT           0           /* Number of SMMUv2 nodes                      */
#define IOVIRT_NAMED_COMPONENT_COUNT  0           /* Number of named-component nodes             */
#define IOVIRT_PMCG_COUNT             0           /* Number of PMCG nodes                        */

#define IOVIRT_SMMUV3_0_BASE_ADDRESS  0x280000000  /* 0:SMMUv3 base address                      */
#define IOVIRT_SMMUV3_1_BASE_ADDRESS  0x288000000  /* 1:SMMUv3 base address                      */
#define IOVIRT_SMMUV3_2_BASE_ADDRESS  0x290000000  /* 2:SMMUv3 base address                      */
#define IOVIRT_SMMUV3_3_BASE_ADDRESS  0x298000000  /* 3:SMMUv3 base address                      */
#define IOVIRT_SMMU_CTX_INT_OFFSET    0x0          /* Offset to context interrupts               */
#define IOVIRT_SMMU_CTX_INT_CNT       0x0          /* Number of context interrupts               */

#define IOVIRT_RC_PCI_SEG_NUM         0x0          /* Root-complex PCI segment                   */
#define IOVIRT_RC_MEMORY_PROPERTIES   0x1          /* Root-complex Memory Access properties      */
#define IOVIRT_RC_ATS_ATTRIBUTE       0x1          /* Root-complex ATS attribute                 */

#define IOVIRT_MAX_NUM_MAP            4            /* Used to define size of mapping array       */
#define IOVIRT_RC_NUM_MAP             4            /* Root complex mapping entries               */
#define IOVIRT_SMMUV3_0_NUM_MAP       2            /* SMMU0 mapping entries                      */
#define IOVIRT_SMMUV3_1_NUM_MAP       2            /* SMMU1 mapping entries                      */
#define IOVIRT_SMMUV3_2_NUM_MAP       2            /* SMMU2 mapping entries                      */
#define IOVIRT_SMMUV3_3_NUM_MAP       2            /* SMMU3 mapping entries                      */
#define IOVIRT_NAMED_COMP0_NUM_MAP    0            /* Named component mapping entries            */

#define RC_MAP0_INPUT_BASE            0x0          /* Input base - Refer table 4 in IORT table   */
#define RC_MAP0_ID_COUNT              0x8FF        /* The number of IDs in the range minus one   */
#define RC_MAP0_OUTPUT_BASE           0x40000      /* The lowest value in the output range       */
#define RC_MAP0_OUTPUT_REF            0x328        /* Reference to the output IORT Node          */
#define RC_MAP1_INPUT_BASE            0x900
#define RC_MAP1_ID_COUNT              0xFF
#define RC_MAP1_OUTPUT_BASE           0x40900
#define RC_MAP1_OUTPUT_REF            0x3FC
#define RC_MAP2_INPUT_BASE            0xA00
#define RC_MAP2_ID_COUNT              0xFF
#define RC_MAP2_OUTPUT_BASE           0x40A00
#define RC_MAP2_OUTPUT_REF            0x4D0
#define RC_MAP3_INPUT_BASE            0xB00
#define RC_MAP3_ID_COUNT              0xFF
#define RC_MAP3_OUTPUT_BASE           0x40B00
#define RC_MAP3_OUTPUT_REF            0x5A4

#define SMMUV3_0_ID_MAP0_INPUT_BASE   0x0       /* Map0: Input base - Refer table 4 in IORT table */
#define SMMUV3_0_ID_MAP0_ID_COUNT     0x0       /* Map0: The number of IDs in the range minus one */
#define SMMUV3_0_ID_MAP0_OUTPUT_BASE  0x0       /* Map0: The lowest value in the output range     */
#define SMMUV3_0_ID_MAP0_OUTPUT_REF   0x18      /* Map0: Reference to the output IORT Node        */
#define SMMUV3_0_ID_MAP1_INPUT_BASE   0x40000   /* Map1: Input base - Refer table 4 in IORT table */
#define SMMUV3_0_ID_MAP1_ID_COUNT     0x8FF     /* Map1: The number of IDs in the range minus one */
#define SMMUV3_0_ID_MAP1_OUTPUT_BASE  0x40000   /* Map1: The lowest value in the output range     */
#define SMMUV3_0_ID_MAP1_OUTPUT_REF   0x18      /* Map1: Reference to the output IORT Node        */

#define SMMUV3_1_ID_MAP0_INPUT_BASE   0x0
#define SMMUV3_1_ID_MAP0_ID_COUNT     0x0
#define SMMUV3_1_ID_MAP0_OUTPUT_BASE  0x0
#define SMMUV3_1_ID_MAP0_OUTPUT_REF   0xDC
#define SMMUV3_1_ID_MAP1_INPUT_BASE   0x40900
#define SMMUV3_1_ID_MAP1_ID_COUNT     0xFF
#define SMMUV3_1_ID_MAP1_OUTPUT_BASE  0x40900
#define SMMUV3_1_ID_MAP1_OUTPUT_REF   0xDC

#define SMMUV3_2_ID_MAP0_INPUT_BASE   0x0
#define SMMUV3_2_ID_MAP0_ID_COUNT     0x0
#define SMMUV3_2_ID_MAP0_OUTPUT_BASE  0x0
#define SMMUV3_2_ID_MAP0_OUTPUT_REF   0x1A0
#define SMMUV3_2_ID_MAP1_INPUT_BASE   0x40A00
#define SMMUV3_2_ID_MAP1_ID_COUNT     0xFF
#define SMMUV3_2_ID_MAP1_OUTPUT_BASE  0x40A00
#define SMMUV3_2_ID_MAP1_OUTPUT_REF   0x1A0

#define SMMUV3_3_ID_MAP0_INPUT_BASE   0x0
#define SMMUV3_3_ID_MAP0_ID_COUNT     0x0
#define SMMUV3_3_ID_MAP0_OUTPUT_BASE  0x0
#define SMMUV3_3_ID_MAP0_OUTPUT_REF   0x264
#define SMMUV3_3_ID_MAP1_INPUT_BASE   0x40B00
#define SMMUV3_3_ID_MAP1_ID_COUNT     0xFF
#define SMMUV3_3_ID_MAP1_OUTPUT_BASE  0x40B00
#define SMMUV3_3_ID_MAP1_OUTPUT_REF   0x264

#define PLATFORM_OVERRIDE_MAX_SID     24           /* Max Stream-ID bits implemented            */
#define SMMU_MAP_SIZE                 0x20000      /* SMMU base memory map size                 */

/* Placeholder - Named component device details
#define IOVIRT_NAMED_0_SMMU_BASE      0x48000000
#define IOVIRT_NAMED_0_MEM_PROP       0x1
#define IOVIRT_NAMED_0_DEVICE_NAME    "\\_SB_.ETR0"
#define IOVIRT_NAMED_1_SMMU_BASE      0x48000000
#define IOVIRT_NAMED_1_MEM_PROP       0x1
#define IOVIRT_NAMED_1_DEVICE_NAME    "\\_SB_.DMA0"
 */

/* Placeholder - Named Component mappings details
#define NAMED_COMP0_MAP0_INPUT_BASE   0x0
#define NAMED_COMP0_MAP0_ID_COUNT     0x0
#define NAMED_COMP0_MAP0_OUTPUT_BASE  0x10000
#define NAMED_COMP0_MAP0_OUTPUT_REF   0xA54
#define NAMED_COMP0_MAP1_INPUT_BASE   0x1
#define NAMED_COMP0_MAP1_ID_COUNT     0x0
#define NAMED_COMP0_MAP1_OUTPUT_BASE  0x10001
#define NAMED_COMP0_MAP1_OUTPUT_REF   0xA54
*/

/* Placeholder - PMCG macros
#define IOVIRT_PMCG_0_BASE_ADDRESS      0x0
#define IOVIRT_PMCG_0_OVERFLOW_GSIV     0x0
#define IOVIRT_PMCG_0_NODE_REFERENCE    0x0
#define IOVIRT_PMCG_0_SMMU_BASE         0x0
*/

/* DMA platform config parameters */
#define PLATFORM_OVERRIDE_DMA_CNT   0              /* Number of DMA controllers                 */

/* ---------------------  Exerciser info  --------------------- */
#define EXERCISER_ID                0xED0113B5    /* Exerciser Vendor and Device ID             */
#define PCIE_CAP_CTRL_OFFSET        0x4           /* Offset from the extended capability header */
#define TEST_REG_COUNT              10            /* Used in pal_exerciser_get_data API         */

/* The values provided below are specific to a particular implementation of the PCIe exerciser */
/* For details, refer to the documentation at <sysarch-acs>/docs/pcie/Exerciser.md */
#define MSICTL          0x00
#define INTXCTL         0x04
#define DMACTL1         0x08
#define DMA_BUS_ADDR    0x10
#define DMA_LEN         0x18
#define DMASTATUS       0x1C
#define PASID_VAL       0x20
#define ATSCTL          0x24
#define ATS_ADDR        0x28
#define RID_CTL_REG     0x3C
#define TXN_TRACE       0x40
#define TXN_CTRL_BASE   0x44

#define DVSEC_CTRL      0x08
#define PASID           0x1B
#define PCI_E           0x01
#define DVSEC           0x23

/* PCI/PCIe express extended capability structure's
   next capability pointer mask and cap ID mask */

#define CLR_INTR_MASK       0xFFFFFFFE
#define PASID_TLP_STOP_MASK 0xFFFFFFBF
#define PASID_VAL_MASK      ((0x1ul << 20) - 1)
#define PASID_VAL_SHIFT     12
#define PASID_LEN_SHIFT     7
#define PASID_LEN_MASK      0x7ul
#define PASID_EN_SHIFT      6
#define DMA_TO_DEVICE_MASK  0xFFFFFFEF

/* Shift and Mask helper macros */
#define MASK_BIT               1
#define SHIFT_4BIT             4
#define SHITT_8BIT             8
#define PREFETCHABLE_BIT_SHIFT 3
#define ERROR_INJECT_BIT       17
#define ERR_CODE_SHIFT         20
#define FATAL_SHIFT            31
#define MSICTL_ID_MASK         ((1u << 11) - 1)
#define MSI_GENERATION_MASK    (1 << 31)
#define NO_SNOOP_START_MASK    0x20
#define NO_SNOOP_STOP_MASK     0xFFFFFFDF
#define PCIE_CAP_DIS_MASK      0xFFFEFFFF
#define PCIE_CAP_EN_MASK       (1 << 16)
#define PASID_EN_MASK          (1 << 6)
#define RID_VALUE_MASK         0xFFFF
#define RID_VALID_MASK         (1ul << 31)
#define ATS_TRIGGER            1
#define ATS_STATUS             (1ul << 7)

/* Memory config */
#define PLATFORM_OVERRIDE_MEMORY_ENTRY_COUNT        0x4
#define PLATFORM_OVERRIDE_MEMORY_ENTRY0_PHY_ADDR    0xF0A46000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY0_VIRT_ADDR   0xF0A46000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY0_SIZE        0x5000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY0_TYPE        MEMORY_TYPE_DEVICE
#define PLATFORM_OVERRIDE_MEMORY_ENTRY1_PHY_ADDR    0x8080000000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY1_VIRT_ADDR   0x8080000000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY1_SIZE        0x10000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY1_TYPE        MEMORY_TYPE_RESERVED
#define PLATFORM_OVERRIDE_MEMORY_ENTRY2_PHY_ADDR    0x80000000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY2_VIRT_ADDR   0x80000000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY2_SIZE        0x10000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY2_TYPE        MEMORY_TYPE_NORMAL
#define PLATFORM_OVERRIDE_MEMORY_ENTRY3_PHY_ADDR    0xC030000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY3_VIRT_ADDR   0xC030000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY3_SIZE        0x20000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY3_TYPE        MEMORY_TYPE_NOT_POPULATED

/* System Last-Level cache info
   0 - Unknown
   1 - PPTT PE-side LLC
   2 - HMAT mem-side LLC
*/
#define PLATFORM_OVERRRIDE_SLC 0x0

/* Coresight components config parameters*/
#define CS_COMPONENT_COUNT         0
/* Placeholder - Coresight components config parameters
#define CS_COMPONENT_0_IDENTIFIER    "ARMHC97C"
#define CS_COMPONENT_0_DEVICE_NAME   "\\_SB_.ETR0" */

/* Cache config and MASKS. Used in the implementation of PAL APIs. Modify if required. */
#define SIZE_MASK           0x1
#define CACHE_TYPE_MASK     0x10
#define CACHE_ID_MASK       0x80
#define CACHE_TYPE_SHIFT    4
#define CACHE_ID_SHIFT      7
#define DEFAULT_CACHE_IDX   0xFFFFFFFF
#define MAX_L1_CACHE_RES 2           /* Generally PE Level 1 have a data and a instruction cache */

/* ------------------------  Cache/PPTT description  ----------------------- */
#define PLATFORM_OVERRIDE_CACHE_CNT            48       /* Total cache descriptor entries        */

#define PLATFORM_CACHE0_FLAGS                  0xFF     /* PPTT table - Cache structure flags    */
#define PLATFORM_CACHE0_OFFSET                 0x68     /* Cache offset in the PPTT table        */
#define PLATFORM_CACHE0_NEXT_LEVEL_INDEX       1        /* Reference to next level of cache      */
#define PLATFORM_CACHE0_SIZE                   0x10000  /* Size of the cache in bytes            */
#define PLATFORM_CACHE0_CACHE_ID               0x1      /* Unique identifier for this cache      */
#define PLATFORM_CACHE0_IS_PRIVATE             0x1      /* Is the cache private to a core        */
#define PLATFORM_CACHE0_TYPE                   0        /* 0:Data; 1:Instruction; 2/3:Unified    */

#define PLATFORM_CACHE1_FLAGS                  0xFF
#define PLATFORM_CACHE1_OFFSET                 0xA0
#define PLATFORM_CACHE1_NEXT_LEVEL_INDEX       -1
#define PLATFORM_CACHE1_SIZE                   0x200000
#define PLATFORM_CACHE1_CACHE_ID               0x3
#define PLATFORM_CACHE1_IS_PRIVATE             0x1
#define PLATFORM_CACHE1_TYPE                   2

#define PLATFORM_CACHE2_FLAGS                  0xFF
#define PLATFORM_CACHE2_OFFSET                 0x84
#define PLATFORM_CACHE2_NEXT_LEVEL_INDEX       1
#define PLATFORM_CACHE2_SIZE                   0x10000
#define PLATFORM_CACHE2_CACHE_ID               0x2
#define PLATFORM_CACHE2_IS_PRIVATE             0x1
#define PLATFORM_CACHE2_TYPE                   1

#define PLATFORM_CACHE3_FLAGS                  0xFF
#define PLATFORM_CACHE3_OFFSET                 0xEC
#define PLATFORM_CACHE3_NEXT_LEVEL_INDEX       4
#define PLATFORM_CACHE3_SIZE                   0x10000
#define PLATFORM_CACHE3_CACHE_ID               0x1001
#define PLATFORM_CACHE3_IS_PRIVATE             0x1
#define PLATFORM_CACHE3_TYPE                   0

#define PLATFORM_CACHE4_FLAGS                  0xFF
#define PLATFORM_CACHE4_OFFSET                 0x124
#define PLATFORM_CACHE4_NEXT_LEVEL_INDEX       -1
#define PLATFORM_CACHE4_SIZE                   0x200000
#define PLATFORM_CACHE4_CACHE_ID               0x1003
#define PLATFORM_CACHE4_IS_PRIVATE             0x1
#define PLATFORM_CACHE4_TYPE                   2

#define PLATFORM_CACHE5_FLAGS                  0xFF
#define PLATFORM_CACHE5_OFFSET                 0x108
#define PLATFORM_CACHE5_NEXT_LEVEL_INDEX       4
#define PLATFORM_CACHE5_SIZE                   0x10000
#define PLATFORM_CACHE5_CACHE_ID               0x1002
#define PLATFORM_CACHE5_IS_PRIVATE             0x1
#define PLATFORM_CACHE5_TYPE                   1

#define PLATFORM_CACHE6_FLAGS                  0xFF
#define PLATFORM_CACHE6_OFFSET                 0x170
#define PLATFORM_CACHE6_NEXT_LEVEL_INDEX       7
#define PLATFORM_CACHE6_SIZE                   0x10000
#define PLATFORM_CACHE6_CACHE_ID               0x2001
#define PLATFORM_CACHE6_IS_PRIVATE             0x1
#define PLATFORM_CACHE6_TYPE                   0

#define PLATFORM_CACHE7_FLAGS                  0xFF
#define PLATFORM_CACHE7_OFFSET                 0x1A8
#define PLATFORM_CACHE7_NEXT_LEVEL_INDEX       -1
#define PLATFORM_CACHE7_SIZE                   0x200000
#define PLATFORM_CACHE7_CACHE_ID               0x2003
#define PLATFORM_CACHE7_IS_PRIVATE             0x1
#define PLATFORM_CACHE7_TYPE                   2

#define PLATFORM_CACHE8_FLAGS                  0xFF
#define PLATFORM_CACHE8_OFFSET                 0x18C
#define PLATFORM_CACHE8_NEXT_LEVEL_INDEX       7
#define PLATFORM_CACHE8_SIZE                   0x10000
#define PLATFORM_CACHE8_CACHE_ID               0x2002
#define PLATFORM_CACHE8_IS_PRIVATE             0x1
#define PLATFORM_CACHE8_TYPE                   1

#define PLATFORM_CACHE9_FLAGS                  0xFF
#define PLATFORM_CACHE9_OFFSET                 0x1F4
#define PLATFORM_CACHE9_NEXT_LEVEL_INDEX       10
#define PLATFORM_CACHE9_SIZE                   0x10000
#define PLATFORM_CACHE9_CACHE_ID               0x3001
#define PLATFORM_CACHE9_IS_PRIVATE             0x1
#define PLATFORM_CACHE9_TYPE                   0

#define PLATFORM_CACHE10_FLAGS                 0xFF
#define PLATFORM_CACHE10_OFFSET                0x22C
#define PLATFORM_CACHE10_NEXT_LEVEL_INDEX      -1
#define PLATFORM_CACHE10_SIZE                  0x200000
#define PLATFORM_CACHE10_CACHE_ID              0x3003
#define PLATFORM_CACHE10_IS_PRIVATE            0x1
#define PLATFORM_CACHE10_TYPE                  2

#define PLATFORM_CACHE11_FLAGS                 0xFF
#define PLATFORM_CACHE11_OFFSET                0x210
#define PLATFORM_CACHE11_NEXT_LEVEL_INDEX      10
#define PLATFORM_CACHE11_SIZE                  0x10000
#define PLATFORM_CACHE11_CACHE_ID              0x3002
#define PLATFORM_CACHE11_IS_PRIVATE            0x1
#define PLATFORM_CACHE11_TYPE                  1

#define PLATFORM_CACHE12_FLAGS                 0xFF
#define PLATFORM_CACHE12_OFFSET                0x278
#define PLATFORM_CACHE12_NEXT_LEVEL_INDEX      13
#define PLATFORM_CACHE12_SIZE                  0x10000
#define PLATFORM_CACHE12_CACHE_ID              0x4001
#define PLATFORM_CACHE12_IS_PRIVATE            0x1
#define PLATFORM_CACHE12_TYPE                  0

#define PLATFORM_CACHE13_FLAGS                 0xFF
#define PLATFORM_CACHE13_OFFSET                0x2B0
#define PLATFORM_CACHE13_NEXT_LEVEL_INDEX      -1
#define PLATFORM_CACHE13_SIZE                  0x200000
#define PLATFORM_CACHE13_CACHE_ID              0x4003
#define PLATFORM_CACHE13_IS_PRIVATE            0x1
#define PLATFORM_CACHE13_TYPE                  2

#define PLATFORM_CACHE14_FLAGS                 0xFF
#define PLATFORM_CACHE14_OFFSET                0x294
#define PLATFORM_CACHE14_NEXT_LEVEL_INDEX      13
#define PLATFORM_CACHE14_SIZE                  0x10000
#define PLATFORM_CACHE14_CACHE_ID              0x4002
#define PLATFORM_CACHE14_IS_PRIVATE            0x1
#define PLATFORM_CACHE14_TYPE                  1

#define PLATFORM_CACHE15_FLAGS                 0xFF
#define PLATFORM_CACHE15_OFFSET                0x2FC
#define PLATFORM_CACHE15_NEXT_LEVEL_INDEX      16
#define PLATFORM_CACHE15_SIZE                  0x10000
#define PLATFORM_CACHE15_CACHE_ID              0x5001
#define PLATFORM_CACHE15_IS_PRIVATE            0x1
#define PLATFORM_CACHE15_TYPE                  0

#define PLATFORM_CACHE16_FLAGS                 0xFF
#define PLATFORM_CACHE16_OFFSET                0x334
#define PLATFORM_CACHE16_NEXT_LEVEL_INDEX      -1
#define PLATFORM_CACHE16_SIZE                  0x200000
#define PLATFORM_CACHE16_CACHE_ID              0x5003
#define PLATFORM_CACHE16_IS_PRIVATE            0x1
#define PLATFORM_CACHE16_TYPE                  2

#define PLATFORM_CACHE17_FLAGS                 0xFF
#define PLATFORM_CACHE17_OFFSET                0x318
#define PLATFORM_CACHE17_NEXT_LEVEL_INDEX      16
#define PLATFORM_CACHE17_SIZE                  0x10000
#define PLATFORM_CACHE17_CACHE_ID              0x5002
#define PLATFORM_CACHE17_IS_PRIVATE            0x1
#define PLATFORM_CACHE17_TYPE                  1

#define PLATFORM_CACHE18_FLAGS                 0xFF
#define PLATFORM_CACHE18_OFFSET                0x380
#define PLATFORM_CACHE18_NEXT_LEVEL_INDEX      19
#define PLATFORM_CACHE18_SIZE                  0x10000
#define PLATFORM_CACHE18_CACHE_ID              0x6001
#define PLATFORM_CACHE18_IS_PRIVATE            0x1
#define PLATFORM_CACHE18_TYPE                  0

#define PLATFORM_CACHE19_FLAGS                 0xFF
#define PLATFORM_CACHE19_OFFSET                0x3B8
#define PLATFORM_CACHE19_NEXT_LEVEL_INDEX      -1
#define PLATFORM_CACHE19_SIZE                  0x200000
#define PLATFORM_CACHE19_CACHE_ID              0x6003
#define PLATFORM_CACHE19_IS_PRIVATE            0x1
#define PLATFORM_CACHE19_TYPE                  2

#define PLATFORM_CACHE20_FLAGS                 0xFF
#define PLATFORM_CACHE20_OFFSET                0x39C
#define PLATFORM_CACHE20_NEXT_LEVEL_INDEX      19
#define PLATFORM_CACHE20_SIZE                  0x10000
#define PLATFORM_CACHE20_CACHE_ID              0x6002
#define PLATFORM_CACHE20_IS_PRIVATE            0x1
#define PLATFORM_CACHE20_TYPE                  1

#define PLATFORM_CACHE21_FLAGS                 0xFF
#define PLATFORM_CACHE21_OFFSET                0x404
#define PLATFORM_CACHE21_NEXT_LEVEL_INDEX      22
#define PLATFORM_CACHE21_SIZE                  0x10000
#define PLATFORM_CACHE21_CACHE_ID              0x7001
#define PLATFORM_CACHE21_IS_PRIVATE            0x1
#define PLATFORM_CACHE21_TYPE                  0

#define PLATFORM_CACHE22_FLAGS                 0xFF
#define PLATFORM_CACHE22_OFFSET                0x43C
#define PLATFORM_CACHE22_NEXT_LEVEL_INDEX      -1
#define PLATFORM_CACHE22_SIZE                  0x200000
#define PLATFORM_CACHE22_CACHE_ID              0x7003
#define PLATFORM_CACHE22_IS_PRIVATE            0x1
#define PLATFORM_CACHE22_TYPE                  2

#define PLATFORM_CACHE23_FLAGS                 0xFF
#define PLATFORM_CACHE23_OFFSET                0x420
#define PLATFORM_CACHE23_NEXT_LEVEL_INDEX      22
#define PLATFORM_CACHE23_SIZE                  0x10000
#define PLATFORM_CACHE23_CACHE_ID              0x7002
#define PLATFORM_CACHE23_IS_PRIVATE            0x1
#define PLATFORM_CACHE23_TYPE                  1

#define PLATFORM_CACHE24_FLAGS                 0xFF
#define PLATFORM_CACHE24_OFFSET                0x488
#define PLATFORM_CACHE24_NEXT_LEVEL_INDEX      25
#define PLATFORM_CACHE24_SIZE                  0x10000
#define PLATFORM_CACHE24_CACHE_ID              0x8001
#define PLATFORM_CACHE24_IS_PRIVATE            0x1
#define PLATFORM_CACHE24_TYPE                  0

#define PLATFORM_CACHE25_FLAGS                 0xFF
#define PLATFORM_CACHE25_OFFSET                0x4C0
#define PLATFORM_CACHE25_NEXT_LEVEL_INDEX      -1
#define PLATFORM_CACHE25_SIZE                  0x200000
#define PLATFORM_CACHE25_CACHE_ID              0x8003
#define PLATFORM_CACHE25_IS_PRIVATE            0x1
#define PLATFORM_CACHE25_TYPE                  2

#define PLATFORM_CACHE26_FLAGS                 0xFF
#define PLATFORM_CACHE26_OFFSET                0x4A4
#define PLATFORM_CACHE26_NEXT_LEVEL_INDEX      25
#define PLATFORM_CACHE26_SIZE                  0x10000
#define PLATFORM_CACHE26_CACHE_ID              0x8002
#define PLATFORM_CACHE26_IS_PRIVATE            0x1
#define PLATFORM_CACHE26_TYPE                  1

#define PLATFORM_CACHE27_FLAGS                 0xFF
#define PLATFORM_CACHE27_OFFSET                0x50C
#define PLATFORM_CACHE27_NEXT_LEVEL_INDEX      28
#define PLATFORM_CACHE27_SIZE                  0x10000
#define PLATFORM_CACHE27_CACHE_ID              0x9001
#define PLATFORM_CACHE27_IS_PRIVATE            0x1
#define PLATFORM_CACHE27_TYPE                  0

#define PLATFORM_CACHE28_FLAGS                 0xFF
#define PLATFORM_CACHE28_OFFSET                0x544
#define PLATFORM_CACHE28_NEXT_LEVEL_INDEX      -1
#define PLATFORM_CACHE28_SIZE                  0x200000
#define PLATFORM_CACHE28_CACHE_ID              0x9003
#define PLATFORM_CACHE28_IS_PRIVATE            0x1
#define PLATFORM_CACHE28_TYPE                  2

#define PLATFORM_CACHE29_FLAGS                 0xFF
#define PLATFORM_CACHE29_OFFSET                0x528
#define PLATFORM_CACHE29_NEXT_LEVEL_INDEX      28
#define PLATFORM_CACHE29_SIZE                  0x10000
#define PLATFORM_CACHE29_CACHE_ID              0x9002
#define PLATFORM_CACHE29_IS_PRIVATE            0x1
#define PLATFORM_CACHE29_TYPE                  1

#define PLATFORM_CACHE30_FLAGS                 0xFF
#define PLATFORM_CACHE30_OFFSET                0x590
#define PLATFORM_CACHE30_NEXT_LEVEL_INDEX      31
#define PLATFORM_CACHE30_SIZE                  0x10000
#define PLATFORM_CACHE30_CACHE_ID              0xA001
#define PLATFORM_CACHE30_IS_PRIVATE            0x1
#define PLATFORM_CACHE30_TYPE                  0

#define PLATFORM_CACHE31_FLAGS                 0xFF
#define PLATFORM_CACHE31_OFFSET                0x5C8
#define PLATFORM_CACHE31_NEXT_LEVEL_INDEX      -1
#define PLATFORM_CACHE31_SIZE                  0x200000
#define PLATFORM_CACHE31_CACHE_ID              0xA003
#define PLATFORM_CACHE31_IS_PRIVATE            0x1
#define PLATFORM_CACHE31_TYPE                  2

#define PLATFORM_CACHE32_FLAGS                 0xFF
#define PLATFORM_CACHE32_OFFSET                0x5AC
#define PLATFORM_CACHE32_NEXT_LEVEL_INDEX      31
#define PLATFORM_CACHE32_SIZE                  0x10000
#define PLATFORM_CACHE32_CACHE_ID              0xA002
#define PLATFORM_CACHE32_IS_PRIVATE            0x1
#define PLATFORM_CACHE32_TYPE                  1

#define PLATFORM_CACHE33_FLAGS                 0xFF
#define PLATFORM_CACHE33_OFFSET                0x614
#define PLATFORM_CACHE33_NEXT_LEVEL_INDEX      34
#define PLATFORM_CACHE33_SIZE                  0x10000
#define PLATFORM_CACHE33_CACHE_ID              0xB001
#define PLATFORM_CACHE33_IS_PRIVATE            0x1
#define PLATFORM_CACHE33_TYPE                  0

#define PLATFORM_CACHE34_FLAGS                 0xFF
#define PLATFORM_CACHE34_OFFSET                0x64C
#define PLATFORM_CACHE34_NEXT_LEVEL_INDEX      -1
#define PLATFORM_CACHE34_SIZE                  0x200000
#define PLATFORM_CACHE34_CACHE_ID              0xB003
#define PLATFORM_CACHE34_IS_PRIVATE            0x1
#define PLATFORM_CACHE34_TYPE                  2

#define PLATFORM_CACHE35_FLAGS                 0xFF
#define PLATFORM_CACHE35_OFFSET                0x630
#define PLATFORM_CACHE35_NEXT_LEVEL_INDEX      34
#define PLATFORM_CACHE35_SIZE                  0x10000
#define PLATFORM_CACHE35_CACHE_ID              0xB002
#define PLATFORM_CACHE35_IS_PRIVATE            0x1
#define PLATFORM_CACHE35_TYPE                  1

#define PLATFORM_CACHE36_FLAGS                 0xFF
#define PLATFORM_CACHE36_OFFSET                0x698
#define PLATFORM_CACHE36_NEXT_LEVEL_INDEX      37
#define PLATFORM_CACHE36_SIZE                  0x10000
#define PLATFORM_CACHE36_CACHE_ID              0xC001
#define PLATFORM_CACHE36_IS_PRIVATE            0x1
#define PLATFORM_CACHE36_TYPE                  0

#define PLATFORM_CACHE37_FLAGS                 0xFF
#define PLATFORM_CACHE37_OFFSET                0x6D0
#define PLATFORM_CACHE37_NEXT_LEVEL_INDEX      -1
#define PLATFORM_CACHE37_SIZE                  0x200000
#define PLATFORM_CACHE37_CACHE_ID              0xC003
#define PLATFORM_CACHE37_IS_PRIVATE            0x1
#define PLATFORM_CACHE37_TYPE                  2

#define PLATFORM_CACHE38_FLAGS                 0xFF
#define PLATFORM_CACHE38_OFFSET                0x6B4
#define PLATFORM_CACHE38_NEXT_LEVEL_INDEX      37
#define PLATFORM_CACHE38_SIZE                  0x10000
#define PLATFORM_CACHE38_CACHE_ID              0xC002
#define PLATFORM_CACHE38_IS_PRIVATE            0x1
#define PLATFORM_CACHE38_TYPE                  1

#define PLATFORM_CACHE39_FLAGS                 0xFF
#define PLATFORM_CACHE39_OFFSET                0x71C
#define PLATFORM_CACHE39_NEXT_LEVEL_INDEX      40
#define PLATFORM_CACHE39_SIZE                  0x10000
#define PLATFORM_CACHE39_CACHE_ID              0xD001
#define PLATFORM_CACHE39_IS_PRIVATE            0x1
#define PLATFORM_CACHE39_TYPE                  0

#define PLATFORM_CACHE40_FLAGS                 0xFF
#define PLATFORM_CACHE40_OFFSET                0x754
#define PLATFORM_CACHE40_NEXT_LEVEL_INDEX      -1
#define PLATFORM_CACHE40_SIZE                  0x200000
#define PLATFORM_CACHE40_CACHE_ID              0xD003
#define PLATFORM_CACHE40_IS_PRIVATE            0x1
#define PLATFORM_CACHE40_TYPE                  2

#define PLATFORM_CACHE41_FLAGS                 0xFF
#define PLATFORM_CACHE41_OFFSET                0x738
#define PLATFORM_CACHE41_NEXT_LEVEL_INDEX      40
#define PLATFORM_CACHE41_SIZE                  0x10000
#define PLATFORM_CACHE41_CACHE_ID              0xD002
#define PLATFORM_CACHE41_IS_PRIVATE            0x1
#define PLATFORM_CACHE41_TYPE                  1

#define PLATFORM_CACHE42_FLAGS                 0xFF
#define PLATFORM_CACHE42_OFFSET                0x7A0
#define PLATFORM_CACHE42_NEXT_LEVEL_INDEX      43
#define PLATFORM_CACHE42_SIZE                  0x10000
#define PLATFORM_CACHE42_CACHE_ID              0xE001
#define PLATFORM_CACHE42_IS_PRIVATE            0x1
#define PLATFORM_CACHE42_TYPE                  0

#define PLATFORM_CACHE43_FLAGS                 0xFF
#define PLATFORM_CACHE43_OFFSET                0x7D8
#define PLATFORM_CACHE43_NEXT_LEVEL_INDEX      -1
#define PLATFORM_CACHE43_SIZE                  0x200000
#define PLATFORM_CACHE43_CACHE_ID              0xE003
#define PLATFORM_CACHE43_IS_PRIVATE            0x1
#define PLATFORM_CACHE43_TYPE                  2

#define PLATFORM_CACHE44_FLAGS                 0xFF
#define PLATFORM_CACHE44_OFFSET                0x7BC
#define PLATFORM_CACHE44_NEXT_LEVEL_INDEX      43
#define PLATFORM_CACHE44_SIZE                  0x10000
#define PLATFORM_CACHE44_CACHE_ID              0xE002
#define PLATFORM_CACHE44_IS_PRIVATE            0x1
#define PLATFORM_CACHE44_TYPE                  1

#define PLATFORM_CACHE45_FLAGS                 0xFF
#define PLATFORM_CACHE45_OFFSET                0x824
#define PLATFORM_CACHE45_NEXT_LEVEL_INDEX      46
#define PLATFORM_CACHE45_SIZE                  0x10000
#define PLATFORM_CACHE45_CACHE_ID              0xF001
#define PLATFORM_CACHE45_IS_PRIVATE            0x1
#define PLATFORM_CACHE45_TYPE                  0

#define PLATFORM_CACHE46_FLAGS                 0xFF
#define PLATFORM_CACHE46_OFFSET                0x85C
#define PLATFORM_CACHE46_NEXT_LEVEL_INDEX      -1
#define PLATFORM_CACHE46_SIZE                  0x200000
#define PLATFORM_CACHE46_CACHE_ID              0xF003
#define PLATFORM_CACHE46_IS_PRIVATE            0x1
#define PLATFORM_CACHE46_TYPE                  2

#define PLATFORM_CACHE47_FLAGS                 0xFF
#define PLATFORM_CACHE47_OFFSET                0x840
#define PLATFORM_CACHE47_NEXT_LEVEL_INDEX      46
#define PLATFORM_CACHE47_SIZE                  0x10000
#define PLATFORM_CACHE47_CACHE_ID              0xF002
#define PLATFORM_CACHE47_IS_PRIVATE            0x1
#define PLATFORM_CACHE47_TYPE                  1

/* PE-to-L1 cache mapping */
#define PLATFORM_PPTT0_CACHEID0  0x1          /* Processor 0: L1 Cache ID0 */
#define PLATFORM_PPTT0_CACHEID1  0x2          /* Processor 0: L1 Cache ID1 */

#define PLATFORM_PPTT1_CACHEID0  0x1001
#define PLATFORM_PPTT1_CACHEID1  0x1002

#define PLATFORM_PPTT2_CACHEID0  0x2001
#define PLATFORM_PPTT2_CACHEID1  0x2002

#define PLATFORM_PPTT3_CACHEID0  0x3001
#define PLATFORM_PPTT3_CACHEID1  0x3002

#define PLATFORM_PPTT4_CACHEID0  0x4001
#define PLATFORM_PPTT4_CACHEID1  0x4002

#define PLATFORM_PPTT5_CACHEID0  0x5001
#define PLATFORM_PPTT5_CACHEID1  0x5002

#define PLATFORM_PPTT6_CACHEID0  0x6001
#define PLATFORM_PPTT6_CACHEID1  0x6002

#define PLATFORM_PPTT7_CACHEID0  0x7001
#define PLATFORM_PPTT7_CACHEID1  0x7002

#define PLATFORM_PPTT8_CACHEID0  0x8001
#define PLATFORM_PPTT8_CACHEID1  0x8002

#define PLATFORM_PPTT9_CACHEID0  0x9001
#define PLATFORM_PPTT9_CACHEID1  0x9002

#define PLATFORM_PPTT10_CACHEID0 0xA001
#define PLATFORM_PPTT10_CACHEID1 0xA002

#define PLATFORM_PPTT11_CACHEID0 0xB001
#define PLATFORM_PPTT11_CACHEID1 0xB002

#define PLATFORM_PPTT12_CACHEID0 0xC001
#define PLATFORM_PPTT12_CACHEID1 0xC002

#define PLATFORM_PPTT13_CACHEID0 0xD001
#define PLATFORM_PPTT13_CACHEID1 0xD002

#define PLATFORM_PPTT14_CACHEID0 0xE001
#define PLATFORM_PPTT14_CACHEID1 0xE002

#define PLATFORM_PPTT15_CACHEID0 0xF001
#define PLATFORM_PPTT15_CACHEID1 0xF002

/* -----------------------------  SRAT / HMAT / APMT  ------------------------------ */

/* SRAT config */
#define PLATFORM_OVERRIDE_NUM_SRAT_ENTRIES  1            /* Number of SRAT entries               */
#define PLATFORM_OVERRIDE_MEM_AFF_CNT       1            /* Number of Memory affinity entries    */
#define PLATFORM_OVERRIDE_GICC_AFF_CNT      0            /* Number of GICC affinity entries      */

#define PLATFORM_SRAT_MEM0_PROX_DOMAIN      0x0         /* domain to which the mem range belongs  */
#define PLATFORM_SRAT_MEM0_FLAGS            0x1         /* Refer SRAT table (Flags - Mem Affinity)*/
#define PLATFORM_SRAT_MEM0_ADDR_BASE        0x8080000000  /* 64 bit base address of memory range  */
#define PLATFORM_SRAT_MEM0_ADDR_LEN         0x3F7F7FFFFFF /* 64 bit length of memory range        */

/* Placeholder
#define PLATFORM_SRAT_GICC0_PROX_DOMAIN     0x0         // Proximity domain belonging to logical PE
#define PLATFORM_SRAT_GICC0_PROC_UID        0x0         // Processor UID of the associated GICC
#define PLATFORM_SRAT_GICC0_FLAGS           0x1         // Flags - GICC affinity structure
#define PLATFORM_SRAT_GICC0_CLK_DOMAIN      0x0         // Clk domain belonging to logical processor
*/

#define PLATFORM_OVERRIDE_NUM_OF_HMAT_PROX_DOMAIN 4     /* Number of target proximity domains     */
#define PLATFORM_OVERRIDE_HMAT_MEM_ENTRIES    0x4

#define HMAT_NODE_MEM_SLLBIC                  0x1       /* System Locality Lat and BW Information */
#define HMAT_NODE_MEM_SLLBIC_DATA_TYPE        0x3       /* Type of Data represented by SLLBI node */
#define HMAT_NODE_MEM_SLLBIC_FLAGS            0x0       /* Mem hierarchy and access attr flags    */
#define HMAT_NODE_MEM_SLLBIC_ENTRY_BASE_UNIT  0x64      /* Base unit for data type (latency/ BW)  */

#define PLATFORM_HMAT_MEM0_PROX_DOMAIN        0x0       /* NODE0: Memory proximity domain        */
#define PLATFORM_HMAT_MEM0_MAX_WRITE_BW       0x32C8    /* NODE0: Write BW relative to base unit */
#define PLATFORM_HMAT_MEM0_MAX_READ_BW        0x32C8    /* NODE0: Read BW relative to base unit  */

#define PLATFORM_HMAT_MEM1_PROX_DOMAIN        0x1
#define PLATFORM_HMAT_MEM1_MAX_WRITE_BW       0x36B0
#define PLATFORM_HMAT_MEM1_MAX_READ_BW        0x36B0

#define PLATFORM_HMAT_MEM2_PROX_DOMAIN        0x2
#define PLATFORM_HMAT_MEM2_MAX_WRITE_BW       0x3A98
#define PLATFORM_HMAT_MEM2_MAX_READ_BW        0x3A98

#define PLATFORM_HMAT_MEM3_PROX_DOMAIN        0x3
#define PLATFORM_HMAT_MEM3_MAX_WRITE_BW       0x3E80
#define PLATFORM_HMAT_MEM3_MAX_READ_BW        0x3E80

/* APMT config */

#define MAX_NUM_OF_PMU_SUPPORTED              512           /* Max PMU supported. Used by ACS    */
#define PLATFORM_OVERRIDE_PMU_NODE_CNT        0x1           /* PMU Node count in the system      */

#define PLATFORM_PMU_NODE0_BASE0              0x60800000    /* NODE0: Base address 0             */
#define PLATFORM_PMU_NODE0_BASE1              0x0           /* NODE0: Base address 1             */
#define PLATFORM_PMU_NODE0_TYPE               0x2           /* NODE0: Node type                  */
#define PLATFORM_PMU_NODE0_PRI_INSTANCE       0x0           /* NODE0: Instance primary           */
#define PLATFORM_PMU_NODE0_SEC_INSTANCE       0x0           /* NODE0: Instance secondary         */
#define PLATFORM_PMU_NODE0_DUAL_PAGE_EXT      0x0           /* NODE0: Dual page extenson support */

/* ----------------------------  RAS info  ---------------------------------- */
#define RAS_MAX_NUM_NODES                     140           /* Max RAS node count. Used by ACS   */
#define RAS_MAX_INTR_TYPE                     0x2           /* Max size of node interrupt array  */
#define PLATFORM_OVERRIDE_NUM_RAS_NODES       0x1           /* Number of RAS nodes in the system */
#define PLATFORM_OVERRIDE_NUM_PE_RAS_NODES    0x1           /* Node type: Processor error nodes  */
#define PLATFORM_OVERRIDE_NUM_MC_RAS_NODES    0x0           /* Node type: Memory error nodes     */

/* RAS Node Data */
#define PLATFORM_RAS_NODE0_NUM_INTR_ENTRY     0x1           /* Entries in the interrupt array    */

#define PLATFORM_RAS_NODE0_PE_PROCESSOR_ID    0x0         /* Processor ID of node                */
#define PLATFORM_RAS_NODE0_PE_RES_TYPE        0x0         /* Res type- 0:Cache; 1:TLB; 2:Generic */
#define PLATFORM_RAS_NODE0_PE_FLAGS           0x0         /* Flags associated with the structure */
#define PLATFORM_RAS_NODE0_PE_AFF             0x0         /* Processor affinity level indicator  */
#define PLATFORM_RAS_NODE0_PE_RES_DATA        0x0         /* Resource specific data              */

#define PLATFORM_RAS_NODE0_INTF_TYPE          0x0         /* Interface type (0-2)                */
#define PLATFORM_RAS_NODE0_INTF_FLAGS         0x0         /* Flags associated with node interface*/
#define PLATFORM_RAS_NODE0_INTF_BASE          0x0         /* Base address of the first err record*/
#define PLATFORM_RAS_NODE0_INTF_START_REC     0x1         /* Index of first std err record       */
#define PLATFORM_RAS_NODE0_INTF_NUM_REC       0x1         /* Number of error records in the node */
#define PLATFORM_RAS_NODE0_INTF_ERR_REC_IMP   0x0         /* Error record implemented bitmap     */
#define PLATFORM_RAS_NODE0_INTF_ERR_STATUS    0x0         /* Error group based status reporting  */
#define PLATFORM_RAS_NODE0_INTF_ADDR_MODE     0x0         /* Addressing mode used by each err rec*/

#define PLATFORM_RAS_NODE0_INTR0_TYPE         0x0         /* Interrupt type (0-1)                */
#define PLATFORM_RAS_NODE0_INTR0_FLAG         0x1         /* Interrupt flags                     */
#define PLATFORM_RAS_NODE0_INTR0_GSIV         0x11        /* Interrupt GSIV                      */
#define PLATFORM_RAS_NODE0_INTR0_ITS_ID       0x0         /* ITS group ID                        */

#define RAS2_MAX_NUM_BLOCKS                   0x4         /* Used by ACS to define max arr size  */
#define PLATFORM_OVERRIDE_NUM_RAS2_BLOCK      0x0         /* Number of RAS2 blocks               */
#define PLATFORM_OVERRIDE_NUM_RAS2_MEM_BLOCK  0x0         /* Number of RAS2 feature memory nodes */

/* Placeholder: RAS2 node specific information
#define PLATFORM_OVERRIDE_RAS2_BLOCK0_PROXIMITY             0x0 // Prox domain of the RAS2 feature
#define PLATFORM_OVERRIDE_RAS2_BLOCK0_PATROL_SCRUB_SUPPORT  0x1 // Is Patrol-scrub supported
#define PLATFORM_OVERRIDE_RAS2_BLOCK1_PROXIMITY             0x1
#define PLATFORM_OVERRIDE_RAS2_BLOCK1_PATROL_SCRUB_SUPPORT  0x1
*/

/*MPAM Config*/
#define MPAM_MAX_MSC_NODE                     0x1    /* Used by ACS to define MSC array len      */
#define MPAM_MAX_RSRC_NODE                    0x1    /* Max resource nodes per MSC - Used by ACS */
#define PLATFORM_MPAM_MSC_COUNT               0x0    /* MSC nodes present in the system          */

/* Placeholder: MPAM Node specific information
#define PLATFORM_MPAM_MSC0_INTR_TYPE          0x0         // MSC Interface type - 0:MMIO 1:PCC
#define PLATFORM_MPAM_MSC0_ID                 0x1         // MPAM MSC Identifier
#define PLATFORM_MPAM_MSC0_BASE_ADDR          0x102A41000 // MPAM MSC Base address
#define PLATFORM_MPAM_MSC0_ADDR_LEN           0x1008      // MPAM MSC MMIO address length
#define PLATFORM_MPAM_MSC0_MAX_NRDY           0           // Max time (in ms) NRDY signal asserts
#define PLATFORM_MPAM_MSC0_RSRC_COUNT         0x1         // Resources described by MSC0
#define PLATFORM_MPAM_MSC0_RSRC0_RIS_INDEX    0x0         // RIS0: Index
#define PLATFORM_MPAM_MSC0_RSRC0_LOCATOR_TYPE 0x0         // RIS0: Locator type
#define PLATFORM_MPAM_MSC0_RSRC0_DESCRIPTOR1  0x0         // RIS0: Resource specific field #1
#define PLATFORM_MPAM_MSC0_RSRC0_DESCRIPTOR2  0x0         // RIS0: Resource specific field #2
*/

/* PCC Config */
/* Following values are for placeholding purpose, doesn't correspond to RDV3 system */
#define PLATFORM_PCC_SUBSPACE_COUNT                         0x1
#define PLATFORM_PCC_SUBSPACE0_INDEX                        0x0
#define PLATFORM_PCC_SUBSPACE0_TYPE                         0x3
#define PLATFORM_PCC_SUBSPACE0_BASE                         0x0
#define PLATFORM_PCC_SUBSPACE0_DOORBELL_PRESERVE            0x0
#define PLATFORM_PCC_SUBSPACE0_DOORBELL_WRITE               0x0
#define PLATFORM_PCC_SUBSPACE0_MIN_REQ_TURN_TIME            0x0
#define PLATFORM_PCC_SUBSPACE0_CMD_COMPLETE_CHK_MASK        0x0
#define PLATFORM_PCC_SUBSPACE0_CMD_UPDATE_PRESERVE          0x0
#define PLATFORM_PCC_SUBSPACE0_CMD_COMPLETE_UPDATE_SET      0x0

/* Following fields follow GENERIC_ADDRESS_STRUCTURE
   defined in platform_override_sbsa_struct.h */
#define PLATFORM_PCC_SUBSPACE0_DOORBELL_REG                 {0x0, 0x0, 0x0, 0x0, 0xDEADDEAD}
#define PLATFORM_PCC_SUBSPACE0_CMD_COMPLETE_UPDATE_REG      {0x0, 0x0, 0x0, 0x0, 0xDEADDEAD}
#define PLATFORM_PCC_SUBSPACE0_CMD_COMPLETE_CHK_REG         {0x0, 0x0, 0x0, 0x0, 0xDEADDEAD}

/* ------------------------  TPM2 Info  ----------------------- */
#define PLATFORM_TPM_PRESENT              0x0     /* TPM enable flag (set to 1 when present)  */
#define PLATFORM_TPM_VERSION              0x0     /* 2 = TPM 2.0, adjust if different */
#define PLATFORM_TPM_BASE_ADDR            0x0     /* FIFO locality 0 base address */
#define PLATFORM_TPM_INTERFACE_TYPE       0x0     /* StartMethod (6=TIS, 7=CRB, etc.) */

/** End config **/
