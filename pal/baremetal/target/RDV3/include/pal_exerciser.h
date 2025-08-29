
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

#include <stdio.h>
#include <stdint.h>
#include "platform_override_fvp.h"

extern uint32_t g_print_level;
#define ACS_PRINT_ERR   5      /* Only Errors. Use this to trim output to key info */
#define ACS_PRINT_WARN  4      /* Only warnings & errors. Use this to trim output to key info */
#define ACS_PRINT_TEST  3      /* Test description and result descriptions. THIS is DEFAULT */
#define ACS_PRINT_DEBUG 2      /* For Debug statements. contains register dumps etc */
#define ACS_PRINT_INFO  1      /* Print all statements. Do not use unless really needed */

#define PCIE_SUCCESS            0x00000000  /* Operation completed successfully */
#define PCIE_NO_MAPPING         0x10000001  /* A mapping to a Function does not exist */
#define PCIE_CAP_NOT_FOUND      0x10000010  /* The specified capability was not found */
#define PCIE_UNKNOWN_RESPONSE   0xFFFFFFFF  /* Function not found or UR response from completer */

#ifdef TARGET_BAREMETAL
void pal_uart_print(int log, const char *fmt, ...);
void *mem_alloc(size_t alignment, size_t size);
#define print(verbose, string, ...) \
    do { \
        if ((verbose) >= g_print_level) \
            pal_uart_print((verbose), (string), ##__VA_ARGS__); \
    } while (0)
#else
#include <Library/UefiLib.h>

#define print(verbose, string, ...) \
    do { \
        if ((verbose) >= g_print_level) \
            Print(L##string, ##__VA_ARGS__); \
    } while (0)
#endif

/* Exerciser PAL API declarations */
uint64_t pal_exerciser_get_ecam(uint32_t Bdf);
uint64_t pal_exerciser_get_ecsr_base(uint32_t Bdf, uint32_t BarIndex);
uint64_t pal_exerciser_get_pcie_config_offset(uint32_t Bdf);
uint32_t pal_exerciser_find_pcie_capability (uint32_t ID, uint32_t Bdf,
                                             uint32_t Value, uint32_t *Offset);

#define PCIE_CREATE_BDF(Seg, Bus, Dev, Func) ((Seg << 24) | (Bus << 16) | (Dev << 8) | Func)
#define PCIE_EXTRACT_BDF_SEG(bdf)  ((bdf >> 24) & 0xFF)
#define PCIE_EXTRACT_BDF_BUS(bdf)  ((bdf >> 16) & 0xFF)
#define PCIE_EXTRACT_BDF_DEV(bdf)  ((bdf >> 8) & 0xFF)
#define PCIE_EXTRACT_BDF_FUNC(bdf) (bdf & 0xFF)

#define BAR_REG(bar_reg_value) ((bar_reg_value >> 2) & 0x1)
#define TYPE0_MAX_BARS  6
#define TYPE1_MAX_BARS  2

#define BAR_MASK        0xFFFFFFF0
#define BAR64_MASK      0xFFFFFFFFFFFFFFF0

#define PCIE_MAX_BUS   256
#define PCIE_MAX_DEV    32
#define PCIE_MAX_FUNC    8

/* Status helper macros */
#define TXN_INVALID    0xFFFFFFFF
#define TXN_START      1
#define TXN_STOP       0
#define RID_VALID      1
#define RID_NOT_VALID  0

/*BAR offset */
#define BAR0_OFFSET        0x10
#define TYPE1_BAR_MAX_OFF  0x14
#define TYPE0_BAR_MAX_OFF  0x24
#define BAR_64_BIT         1
#define BAR_32_BIT         0

/* BAR register masks */
#define BAR_MIT_MASK    0x1
#define BAR_MDT_MASK    0x3
#define BAR_MT_MASK     0x1
#define BAR_BASE_MASK   0xfffffff

/* BAR register shifts */
#define BAR_MIT_SHIFT   0
#define BAR_MDT_SHIFT   1
#define BAR_MT_SHIFT    3
#define BAR_BASE_SHIFT  4

/* PCI/PCIe express extended capability structure's
   next capability pointer mask and cap ID mask */
#define PCIE_NXT_CAP_PTR_MASK 0x0FFF
#define PCIE_CAP_ID_MASK      0xFFFF
#define PCI_CAP_ID_MASK       0x00FF
#define PCI_NXT_CAP_PTR_MASK  0x00FF
#define CAP_PTR_MASK          0x00FF
#define PCIE_CAP_OFFSET       0x100
#define CAP_PTR_OFFSET        0x34

#define PCI_CAP_PTR_OFFSET    8
#define PCIE_CAP_PTR_OFFSET   20

typedef struct {
  uint64_t   ecam_base;     ///< ECAM Base address
  uint32_t   segment_num;   ///< Segment number of this ECAM
  uint32_t   start_bus_num; ///< Start Bus number for this ecam space
  uint32_t   end_bus_num;   ///< Last Bus number
} PCIE_INFO_BLOCK;

typedef struct {
  uint32_t  num_entries;
  PCIE_INFO_BLOCK block[];
} PCIE_INFO_TABLE;

typedef enum {
    EDMA_NO_SUPPORT   = 0x0,
    EDMA_COHERENT     = 0x1,
    EDMA_NOT_COHERENT = 0x2,
    EDMA_FROM_DEVICE  = 0x3,
    EDMA_TO_DEVICE    = 0x4
} EXERCISER_DMA_ATTR;

typedef enum {
    SNOOP_ATTRIBUTES    = 0x1,
    LEGACY_IRQ          = 0x2,
    MSIX_ATTRIBUTES     = 0x3,
    DMA_ATTRIBUTES      = 0x4,
    P2P_ATTRIBUTES      = 0x5,
    PASID_ATTRIBUTES    = 0x6,
    CFG_TXN_ATTRIBUTES  = 0x7,
    ATS_RES_ATTRIBUTES  = 0x8,
    TRANSACTION_TYPE    = 0x9,
    NUM_TRANSACTIONS    = 0xA,
    ADDRESS_ATTRIBUTES  = 0xB,
    DATA_ATTRIBUTES     = 0xC,
    ERROR_INJECT_TYPE   = 0xD,
    ENABLE_POISON_MODE  = 0xE,
    ENABLE_RAS_CTRL     = 0xF,
    DISABLE_POISON_MODE = 0x10,
    CLEAR_TXN           = 0x11
} EXERCISER_PARAM_TYPE;

typedef enum {
    START_DMA            = 0x1,
    GENERATE_MSI         = 0x2,
    GENERATE_L_INTR      = 0x3,  //Legacy interrupt
    MEM_READ             = 0x4,
    MEM_WRITE            = 0x5,
    CLEAR_INTR           = 0x6,
    PASID_TLP_START      = 0x7,
    PASID_TLP_STOP       = 0x8,
    TXN_NO_SNOOP_ENABLE  = 0x9,
    TXN_NO_SNOOP_DISABLE = 0xA,
    START_TXN_MONITOR    = 0xB,
    STOP_TXN_MONITOR     = 0xC,
    ATS_TXN_REQ          = 0xD,
    INJECT_ERROR         = 0xE
} EXERCISER_OPS;

typedef enum {
    EXERCISER_RESET = 0x1,
    EXERCISER_ON    = 0x2,
    EXERCISER_OFF   = 0x3,
    EXERCISER_ERROR = 0x4
} EXERCISER_STATE;

struct ecam_reg_data {
    uint32_t offset;    //Offset into 4096 bytes ecam config reg space
    uint32_t attribute;
    uint32_t value;
};

struct exerciser_data_cfg_space {
    struct ecam_reg_data reg[10];
};

typedef enum {
    MMIO_PREFETCHABLE = 0x0,
    MMIO_NON_PREFETCHABLE = 0x1
} BAR_MEM_TYPE;

struct exerciser_data_bar_space {
    void *base_addr;
    BAR_MEM_TYPE type;
};

typedef union exerciser_data {
    struct exerciser_data_cfg_space cfg_space;
    struct exerciser_data_bar_space bar_space;
} exerciser_data_t;

typedef enum {
    EXERCISER_DATA_CFG_SPACE  = 0x1,
    EXERCISER_DATA_BAR0_SPACE = 0x2,
    EXERCISER_DATA_MMIO_SPACE = 0x3,
} EXERCISER_DATA_TYPE;

typedef enum {
    ACCESS_TYPE_RD = 0x0,
    ACCESS_TYPE_RW = 0x1
} ECAM_REG_ATTRIBUTE;

typedef enum {
    TYPE0 = 0x0,
    TYPE1 = 0x1,
} EXERCISER_CFG_HEADER_TYPE;

typedef enum {
    CFG_READ   = 0x0,
    CFG_WRITE  = 0x1,
} EXERCISER_CFG_TXN_ATTR;

typedef enum {
  MMIO = 0,
  IO = 1
} BAR_MEM_INDICATOR_TYPE;

typedef enum {
  BITS_32 = 0,
  BITS_64 = 2
} BAR_MEM_DECODE_TYPE;

typedef enum {
    TXN_REQ_ID       = 0x0,
    TXN_ADDR_TYPE    = 0x1,
    TXN_REQ_ID_VALID = 0x2,
} EXERCISER_TXN_ATTR;

typedef enum {
    AT_UNTRANSLATED = 0x0,
    AT_TRANS_REQ    = 0x1,
    AT_TRANSLATED   = 0x2,
    AT_RESERVED     = 0x3
} EXERCISER_TXN_ADDR_TYPE;
