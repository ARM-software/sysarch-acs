/** @file
 * Copyright (c) 2016-2021, 2023-2025, Arm Limited or its affiliates. All rights reserved.
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

#ifndef __ACS_MEMORY_H__
#define __ACS_MEMORY_H__

#define MEM_MAP_SUCCESS  0x0
#define MEM_MAP_NO_MEM   0x1
#define MEM_MAP_FAILURE  0x2
#define MEM_SIZE_64KB    65536

uint32_t val_memory_ioremap(void *addr, uint32_t size, uint32_t attr, void **baseptr);
addr_t val_memory_get_addr(MEMORY_INFO_e mem_type, uint32_t instance, uint64_t *attr);


void val_mmu_add_mmap(void);
void *val_mmu_get_mmap_list(void);
void val_memory_unmap(void *ptr);
void *val_memory_alloc(uint32_t size);
void *val_memory_calloc(uint32_t num, uint32_t size);
void val_memory_free(void *addr);
int  val_memory_compare(void *src, void *dest, uint32_t len);
void val_memory_set(void *buf, uint32_t size, uint8_t value);
void *val_memory_virt_to_phys(void *va);
void *val_memory_phys_to_virt(uint64_t pa);
void *val_memory_alloc_pages(uint32_t num_pages);
void val_memory_free_pages(void *page_base, uint32_t num_pages);
void *val_aligned_alloc(uint32_t alignment, uint32_t size);
void val_memory_free_aligned(void *addr);
void *val_memory_alloc_cacheable(uint32_t bdf, uint32_t size, void **pa);
void val_memory_free_cacheable(uint32_t bdf, uint32_t size, void *va, void *pa);
void AA64IssueDSB(void);
void val_mem_issue_dsb(void);

uint32_t val_memory_region_has_52bit_addr(void);
uint32_t val_mmu_get_mapping_count(void);
uint32_t val_memory_page_size(void);
uint32_t val_memory_check_for_persistent_mem(void);
uint32_t val_memory_set_wb_executable(void *addr, uint32_t size);

uint32_t m001_entry(uint32_t num_pe);
uint32_t m002_entry(uint32_t num_pe);
uint32_t m003_entry(uint32_t num_pe);
uint32_t m004_entry(uint32_t num_pe);
uint32_t m005_entry(uint32_t num_pe);
uint32_t m006_entry(uint32_t num_pe);
uint32_t m007_entry(uint32_t num_pe);
uint32_t m008_entry(uint32_t num_pe);

#endif // __ACS_MEMORY_H__
