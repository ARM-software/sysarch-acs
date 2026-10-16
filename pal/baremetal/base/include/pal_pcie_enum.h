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

#ifndef __PAL_PCIE_ENUM_H__
#define __PAL_PCIE_ENUM_H__

#include <stdio.h>
#include "acs_stdint.h"


/* Header Offset and Type*/
#define HEADER_OFFSET      0xC
#define TYPE0_HEADER       0
#define TYPE1_HEADER       1

#define TYPE01_RIDR        0x8

#define PCIE_HEADER_TYPE(header_value) ((header_value >> 16) & 0x3)
#define BUS_NUM_REG_CFG(sub_bus, sec_bus, pri_bus) (sub_bus << 16 | sec_bus << 8 | bus)

#define DEVICE_ID_OFFSET   16

/*Initial BUS definitions*/
#define PRI_BUS            0
#define SEC_BUS            1
#define BUS_NUM_REG_OFFSET 0x18

/*BAR offset */
#define BAR0_OFFSET        0x10
#define TYPE1_BAR_MAX_OFF  0x14
#define TYPE0_BAR_MAX_OFF  0x24
#define BAR_64_BIT         1
#define BAR_32_BIT         0
#define BAR_32B_OFFSET     4

#define BAR_NON_PRE_MEM    0
#define BAR_PRE_MEM        0x1

#define MEM_BASE32_LIM_MASK      0xFFF00000
#define MEM_BASE32_REG_ADDR_MASK 0xFFF0FFF0
#define MEM_BASE64_LIM_MASK      0xFFFFFFFFFFF00000
#define NON_PRE_FET_OFFSET       0x20
#define PRE_FET_OFFSET           0x24
#define BAR_INCREMENT            0x100000

#define PRI_BUS_CLEAR_MASK       0xFFFFFF00
#define BAR_REG(bar_reg_value) ((bar_reg_value >> 2) & 0x1)
#define BAR_MEM(bar_reg_value) ((bar_reg_value & 0xF) >> 3)
#define REG_MASK_SHIFT(bar_value) ((bar_value & MEM_BASE32_LIM_MASK) >> 16)

#define TYPE0_MAX_BARS  6
#define TYPE1_MAX_BARS  2

/* BAR register masks */
#define BAR_MIT_MASK    0x1
#define BAR_MDT_MASK    0x3
#define BAR_MT_MASK     0x1
#define BAR_BASE_MASK   0xfffffff
#define BAR_MEM_TYPE_RESERVED 0x3

/* BAR register shifts */
#define BAR_MIT_SHIFT   0
#define BAR_MDT_SHIFT   1
#define BAR_MT_SHIFT    3
#define BAR_BASE_SHIFT  4

/* Command register Memory Space Enable */
#define CR_MSE_SHIFT    1
#define CR_MSE_MASK     0x1

/* Standard capability traversal is bounded by the config-space DWORDs */
#define PCIE_DWORD_SIZE    0x04
#define PCIE_CAP_START     0x40
#define PCIE_CAP_END       0xFC
#define PCIE_CAP_MAX_COUNT ((PCIE_CAP_END - PCIE_CAP_START) / PCIE_DWORD_SIZE + 1)

/* MSI capability control fields and message offsets */
#define MSI_ENABLE_SHIFT                16
#define MSI_MMC_SHIFT                   17
#define MSI_MME_SHIFT                   20
#define MSI_MULTI_MSG_MASK              0x7U
#define MSI_MAX_MME                     5U /* Up to 32 enabled messages */
#define MSI_64BIT_ADDR_SHIFT            23
#define MSI_PER_VECTOR_MASK_SHIFT       24
#define MSI_MSG_TBL_LOWER_ADDR_OFFSET   0x4
#define MSI_MSG_TBL_HIGHER_ADDR_OFFSET  0x8
#define MSI_MSG_TBL_DATA_OFFSET         0xC
#define MSI_MSG_32BIT_DATA_OFFSET       MSI_MSG_TBL_HIGHER_ADDR_OFFSET
#define MSI_MSG_DATA_MASK               0xFFFFU
#define MSI_VECTOR_MASK                 0x1U

/* MSI-X capability fields and table-entry layout */
#define MSI_X_ENABLE_SHIFT               31
#define MSI_X_FUNCTION_MASK_SHIFT        30
#define MSI_X_TABLE_SIZE_SHIFT           16
#define MSI_X_TABLE_SIZE_MASK            0x7FFU
#define MSI_X_TOR_OFFSET                 0x4
#define MSI_X_MSG_TBL_LOWER_ADDR_OFFSET   0x0
#define MSI_X_MSG_TBL_HIGHER_ADDR_OFFSET  0x4
#define MSI_X_MSG_TBL_DATA_OFFSET         0x8
#define MSI_X_MSG_TBL_MVC_OFFSET          0xC
#define MSI_X_TABLE_BIR_MASK              0x7
#define MSI_X_TABLE_OFFSET_MASK           0xFFFFFFF8U
#define MSI_X_ENTRY_SIZE                  16
#define MSI_X_ADDR_SHIFT                  32

uint8_t  pal_mmio_read8(uint64_t addr);
uint16_t pal_mmio_read16(uint64_t addr);
uint32_t pal_mmio_read(uint64_t addr);
uint64_t pal_mmio_read64(uint64_t addr);
void     pal_mmio_write8(uint64_t addr, uint8_t data);
void     pal_mmio_write16(uint64_t addr, uint16_t data);
void     pal_mmio_write(uint64_t addr, uint32_t data);
void     pal_mmio_write64(uint64_t addr, uint64_t data);
void     *pal_mem_alloc(uint32_t size);
void     *pal_mem_calloc(uint32_t num, uint32_t size);

uint32_t pal_increment_bus_dev(uint32_t StartBdf);

uint32_t pal_pcie_get_bdf(uint32_t class_code, uint32_t start_busdev);

uint64_t pal_pcie_get_base(uint32_t bdf, uint32_t bar_index);

uint32_t pal_pci_cfg_read(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t func,
                          uint32_t offset, uint32_t *value);

uint32_t pal_pcie_read_cfg(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t func, uint32_t offset, uint32_t *value);

uint64_t pal_pcie_ecam_base(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t func);

#endif
