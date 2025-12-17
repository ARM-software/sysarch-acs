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

#ifndef __ACS_MMU_H__
#define __ACS_MMU_H__

extern void val_mair_write(uint64_t value, uint64_t el_num);
extern void val_tcr_write(uint64_t value, uint64_t el_num);
extern void val_ttbr0_write(uint64_t value, uint64_t el_num);
extern void val_sctlr_write(uint64_t value, uint64_t el_num);
extern uint64_t val_sctlr_read(uint64_t el_num);
extern uint64_t val_ttbr0_read(uint64_t el_num);
extern uint64_t val_read_current_el(void);

extern uint64_t tt_l0_base[];

/* upper and lower mem attribute shift definitions */
#define MEM_ATTR_INDX_SHIFT     2
#define MEM_ATTR_AP_SHIFT       6
#define MEM_ATTR_SH_SHIFT       8
#define MEM_ATTR_AF_SHIFT       10

#define MEM_ATTR_INDX_MASK      0x7

#define MAIR_DEVICE_nGnRnE      0x00  // Strong Device
#define MAIR_DEVICE_nGnRE       0x04  // Device nGnRE
#define MAIR_DEVICE_nGRE        0x08
#define MAIR_DEVICE_GRE         0x0C  // gather+reorder
#define MAIR_NORMAL_NC          0x44  // Normal Non-cacheable
#define MAIR_NORMAL_WT          0xAA  // Normal Write-Through Read-Allocate
#define MAIR_NORMAL_WT_AGR      0xBB  // Normal Write-Through Read-Allocate + Write-Allocate
#define MAIR_NORMAL_WB          0xFF  // Normal Write-back
#define MAIR_NOT_FOUND          1    // Not Found
#define MAIR_ATTR_UNSUPPORT     2    // unsupported

/* memory type MAIR register index definitions*/
#define ATTR_DEVICE_nGnRnE (0x0ULL << MEM_ATTR_INDX_SHIFT)

uint32_t val_mmu_check_for_entry(uint64_t base_addr);
uint32_t val_mmu_add_entry(uint64_t base_addr, uint64_t size, uint64_t attr);
uint32_t val_mmu_update_entry(uint64_t address, uint32_t size, uint64_t attr);
#endif
