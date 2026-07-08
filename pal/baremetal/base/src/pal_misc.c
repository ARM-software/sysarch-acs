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

#include "acs_stdint.h"
#include <stdarg.h>
#include "pal_pcie_enum.h"
#include "pal_common_support.h"
#include "platform_image_def.h"
#include "pal_pl011_uart.h"

extern void* g_sbsa_log_file_handle;
uint8_t   *gSharedMemory;

static MEM_BLOCK_HEADER *g_free_list;
static uint64_t          heap_base;
static uint64_t          heap_top;
static uint8_t           heap_init_done;

/**
  @brief  Check whether a value is a power of two.

  @param  value  Value to check.

  @return 1 if value is a power of two, 0 otherwise.
**/
static uint32_t
is_power_of_2(size_t value)
{
  return value && ((value & (value - 1)) == 0);
}

/*
 * Heap allocator overview:
 * - The heap is represented as address-ordered blocks. Each block has a
 *   header at the start and a footer at the end.
 * - Free blocks are also linked through g_free_list, so allocation scans only
 *   currently free blocks.
 * - mem_alloc() finds a free block, places the payload after the header and
 *   hidden owner pointer, aligns the payload, and splits any usable remainder
 *   into a new free block.
 * - mem_free() recovers the owning block from the hidden pointer stored before
 *   the payload, validates header/footer metadata, marks the block free, then
 *   coalesces adjacent free blocks before reinserting the final block into
 *   g_free_list.
 */

/**
  @brief  Initialize the baremetal heap allocator state.

  @param  None

  @return 1 if heap initialization succeeds, 0 otherwise.
**/
static uint32_t mem_alloc_init(void)
{
  uintptr_t aligned_heap_base;
  uintptr_t aligned_heap_top;
  size_t    heap_size;
  MEM_BLOCK_FOOTER *footer;

  aligned_heap_base = ADDR_ALIGN(PLATFORM_HEAP_REGION_BASE, MEM_MIN_ALIGNMENT);
  aligned_heap_top = (PLATFORM_HEAP_REGION_BASE + PLATFORM_HEAP_REGION_SIZE) &
                     ~((uintptr_t)MEM_MIN_ALIGNMENT - 1U);

  g_free_list = NULL;
  heap_base = aligned_heap_base;
  heap_top = aligned_heap_top;

  if (aligned_heap_top <= aligned_heap_base) {
    return HEAP_INIT_FAILED;
  }

  heap_size = aligned_heap_top - aligned_heap_base;
  if (heap_size < MEM_MIN_BLOCK_SIZE) {
    return HEAP_INIT_FAILED;
  }

  g_free_list = (MEM_BLOCK_HEADER *)aligned_heap_base;
  g_free_list->signature = MEM_BLOCK_SIGNATURE;
  g_free_list->is_free = 1;
  g_free_list->size = heap_size;
  g_free_list->payload = 0;
  g_free_list->prev_phys = NULL;
  g_free_list->next_phys = NULL;
  g_free_list->prev_free = NULL;
  g_free_list->next_free = NULL;

  footer = (MEM_BLOCK_FOOTER *)(aligned_heap_base + heap_size -
                                sizeof(MEM_BLOCK_FOOTER));
  footer->signature = MEM_FOOT_SIGNATURE;
  footer->reserved = 0;
  footer->size = g_free_list->size;

  heap_init_done = HEAP_INITIALISED;
  return HEAP_INIT_SUCCESS;
}

/**
  @brief  Allocates contiguous memory of requested size and alignment.

  @param  alignment  Alignment for the returned address. It must be a power of two.
  @param  size       Size of the memory region to allocate. It must not be zero.

  @return Allocated memory base address if successful, otherwise NULL.
 **/
void *mem_alloc(size_t alignment, size_t size)
{
  MEM_BLOCK_HEADER *block;
  MEM_BLOCK_HEADER *remainder;
  MEM_BLOCK_FOOTER *footer;
  size_t           alloc_size;
  size_t           remainder_size;
  size_t           reserve_size;
  uintptr_t        block_start;
  uintptr_t        block_end;
  uintptr_t        payload;
  uintptr_t        payload_base;
  uintptr_t        footer_end;
  uintptr_t        used_end;

  if ((size == 0) || !is_power_of_2(alignment)) {
    return NULL;
  }

  if (alignment < MEM_MIN_ALIGNMENT) {
    alignment = MEM_MIN_ALIGNMENT;
  }

  if (size > (SIZE_MAX - alignment)) {
    return NULL;
  }

  reserve_size = size + alignment;

  if (heap_init_done != HEAP_INITIALISED) {
    if (mem_alloc_init() != HEAP_INIT_SUCCESS) {
      return NULL;
    }
  }

  for (block = g_free_list; block != NULL; block = block->next_free) {
    block_start = (uintptr_t)block;
    if (block->size > (UINTPTR_MAX - block_start)) {
      continue;
    }

    block_end = block_start + block->size;

    /*
     * Payload is placed after the block header and a hidden back pointer.
     * mem_free() uses that back pointer to recover the owning block.
     */
    if ((block_start > (UINTPTR_MAX - sizeof(MEM_BLOCK_HEADER))) ||
        ((block_start + sizeof(MEM_BLOCK_HEADER)) >
         (UINTPTR_MAX - sizeof(MEM_BLOCK_HEADER *)))) {
      continue;
    }

    payload_base = block_start + sizeof(MEM_BLOCK_HEADER) +
                   sizeof(MEM_BLOCK_HEADER *);
    if (payload_base > (UINTPTR_MAX - (alignment - 1U))) {
      continue;
    }

    payload = ADDR_ALIGN(payload_base, alignment);

    if ((reserve_size > (UINTPTR_MAX - payload)) ||
        ((payload + reserve_size) > (UINTPTR_MAX - sizeof(MEM_BLOCK_FOOTER)))) {
      continue;
    }

    footer_end = payload + reserve_size + sizeof(MEM_BLOCK_FOOTER);
    if (footer_end > (UINTPTR_MAX - (MEM_MIN_ALIGNMENT - 1U))) {
      continue;
    }

    used_end = ADDR_ALIGN(footer_end, MEM_MIN_ALIGNMENT);
    if (used_end > block_end) {
      continue;
    }

    alloc_size = used_end - block_start;

    /* The selected free block is now owned by this allocation. */
    if (block->prev_free != NULL) {
      block->prev_free->next_free = block->next_free;
    } else {
      g_free_list = block->next_free;
    }

    if (block->next_free != NULL) {
      block->next_free->prev_free = block->prev_free;
    }

    block->prev_free = NULL;
    block->next_free = NULL;

    remainder_size = block->size - alloc_size;
    if (remainder_size >= MEM_MIN_BLOCK_SIZE) {
      /* Keep the unused tail as a free block for later allocations. */
      remainder = (MEM_BLOCK_HEADER *)((uintptr_t)block + alloc_size);
      remainder->signature = MEM_BLOCK_SIGNATURE;
      remainder->is_free = 1;
      remainder->size = remainder_size;
      remainder->payload = 0;
      remainder->prev_phys = block;
      remainder->next_phys = block->next_phys;
      remainder->prev_free = NULL;
      remainder->next_free = NULL;
      if (remainder->next_phys != NULL) {
        remainder->next_phys->prev_phys = remainder;
      }

      block->size = alloc_size;
      block->next_phys = remainder;

      /* Footer is used to validate and coalesce the free block. */
      footer = (MEM_BLOCK_FOOTER *)((uintptr_t)remainder + remainder->size -
                                    sizeof(MEM_BLOCK_FOOTER));
      footer->signature = MEM_FOOT_SIGNATURE;
      footer->reserved = 0;
      footer->size = remainder->size;

      remainder->next_free = g_free_list;
      if (g_free_list != NULL) {
        g_free_list->prev_free = remainder;
      }
      g_free_list = remainder;
    }

    block->payload = payload;
    block->is_free = 0;
    block->prev_free = NULL;
    block->next_free = NULL;

    /* Store footer metadata for validation during free. */
    footer = (MEM_BLOCK_FOOTER *)((uintptr_t)block + block->size -
                                  sizeof(MEM_BLOCK_FOOTER));
    footer->signature = MEM_FOOT_SIGNATURE;
    footer->reserved = 0;
    footer->size = block->size;

    /* Store owning block immediately before the returned payload. */
    ((MEM_BLOCK_HEADER **)payload)[-1] = block;
    return (void *)payload;
  }

  return NULL;
}

/**
  @brief  Free memory allocated by mem_alloc.

  @param  ptr  Pointer returned by mem_alloc.

  @return None
 **/
void mem_free(void *ptr)
{
  MEM_BLOCK_HEADER *block;
  MEM_BLOCK_HEADER *next;
  MEM_BLOCK_HEADER *prev;
  MEM_BLOCK_FOOTER *footer;
  uintptr_t        ptr_addr;

  if (ptr == NULL) {
    return;
  }

  if (heap_init_done != HEAP_INITIALISED) {
    return;
  }

  ptr_addr = (uintptr_t)ptr;

  /* Reject invalid heap range or alignment before reading the back pointer. */
  if ((ptr_addr < (heap_base + sizeof(MEM_BLOCK_HEADER *))) ||
      (ptr_addr >= heap_top) ||
      ((ptr_addr & (MEM_MIN_ALIGNMENT - 1U)) != 0)) {
    return;
  }

  /* Recover and validate the block that produced this exact payload pointer. */
  block = ((MEM_BLOCK_HEADER **)ptr)[-1];
  if ((block == NULL) ||
      ((uintptr_t)block < heap_base) ||
      ((uintptr_t)block >= heap_top) ||
      (block->signature != MEM_BLOCK_SIGNATURE) ||
      (block->size < MEM_MIN_BLOCK_SIZE) ||
      (block->size > (heap_top - (uintptr_t)block)) ||
      (block->payload != ptr_addr)) {
    return;
  }

  /* Header/footer agreement protects the free list from stale/corrupt input. */
  footer = (MEM_BLOCK_FOOTER *)((uintptr_t)block + block->size -
                                sizeof(MEM_BLOCK_FOOTER));
  if ((footer->signature != MEM_FOOT_SIGNATURE) ||
      (footer->size != block->size)) {
    return;
  }

  if (block->is_free) {
    return;
  }

  block->payload = 0;

  block->is_free = 1;
  footer->signature = MEM_FOOT_SIGNATURE;
  footer->reserved = 0;
  footer->size = block->size;

  if ((block->prev_phys != NULL) && block->prev_phys->is_free) {
    prev = block->prev_phys;

    /* Remove previous block from free list before merging with it. */
    if (prev->prev_free != NULL) {
      prev->prev_free->next_free = prev->next_free;
    } else {
      g_free_list = prev->next_free;
    }

    if (prev->next_free != NULL) {
      prev->next_free->prev_free = prev->prev_free;
    }

    prev->prev_free = NULL;
    prev->next_free = NULL;

    prev->size += block->size;
    prev->next_phys = block->next_phys;
    if (prev->next_phys != NULL) {
      prev->next_phys->prev_phys = prev;
    }
    block = prev;

    footer = (MEM_BLOCK_FOOTER *)((uintptr_t)block + block->size -
                                  sizeof(MEM_BLOCK_FOOTER));
    footer->signature = MEM_FOOT_SIGNATURE;
    footer->reserved = 0;
    footer->size = block->size;
  }

  next = block->next_phys;
  if ((next != NULL) && next->is_free) {
    /* Remove next block from free list before merging with it. */
    if (next->prev_free != NULL) {
      next->prev_free->next_free = next->next_free;
    } else {
      g_free_list = next->next_free;
    }

    if (next->next_free != NULL) {
      next->next_free->prev_free = next->prev_free;
    }

    next->prev_free = NULL;
    next->next_free = NULL;

    block->size += next->size;
    block->next_phys = next->next_phys;
    if (block->next_phys != NULL) {
      block->next_phys->prev_phys = block;
    }

    footer = (MEM_BLOCK_FOOTER *)((uintptr_t)block + block->size -
                                  sizeof(MEM_BLOCK_FOOTER));
    footer->signature = MEM_FOOT_SIGNATURE;
    footer->reserved = 0;
    footer->size = block->size;
  }

  /* Insert the final free/coalesced block at the head of the free list. */
  block->is_free = 1;
  block->prev_free = NULL;
  block->next_free = g_free_list;
  if (g_free_list != NULL) {
    g_free_list->prev_free = block;
  }
  g_free_list = block;
}

#define get_num_va_args(_args, _lcount)             \
    (((_lcount) > 1)  ? va_arg(_args, long long int) :  \
    (((_lcount) == 1) ? va_arg(_args, long int) :       \
                va_arg(_args, int)))

#define get_unum_va_args(_args, _lcount)                \
    (((_lcount) > 1)  ? va_arg(_args, unsigned long long int) : \
    (((_lcount) == 1) ? va_arg(_args, unsigned long int) :      \
                va_arg(_args, unsigned int)))

/**
  @brief  Provides a single point of abstraction to read from all
          Memory Mapped IO address

  @param  addr 64-bit address

  @return 8-bit data read from the input address
**/
uint8_t
pal_mmio_read8(uint64_t addr)
{
  uint8_t data;

  data = (*(volatile uint8_t *)addr);
  if (acs_policy_get_print_mmio() || (g_curr_module & g_enable_module))
      pal_print_msg(ACS_PRINT_INFO,
                    " %s Address = %llx  Data = %lx\n",
                    __func__,
                    addr,
                    data);

  return data;
}

/**
  @brief  Provides a single point of abstraction to read from all
          Memory Mapped IO address

  @param  addr 64-bit address

  @return 16-bit data read from the input address
**/
uint16_t
pal_mmio_read16(uint64_t addr)
{
  uint16_t data;

  data = (*(volatile uint16_t *)addr);
  if (acs_policy_get_print_mmio() || (g_curr_module & g_enable_module))
      pal_print_msg(ACS_PRINT_INFO,
                    " %s Address = %llx  Data = %lx\n",
                    __func__,
                    addr,
                    data);

  return data;
}

/**
  @brief  Provides a single point of abstraction to read from all
          Memory Mapped IO address

  @param  addr 64-bit address

  @return 64-bit data read from the input address
**/
uint64_t
pal_mmio_read64(uint64_t addr)
{
  uint64_t data;

  data = (*(volatile uint64_t *)addr);
  if (acs_policy_get_print_mmio() || (g_curr_module & g_enable_module))
      pal_print_msg(ACS_PRINT_INFO,
                    " %s Address = %llx  Data = %llx\n",
                    __func__,
                    addr,
                    data);

  return data;
}

/**
  @brief  Provides a single point of abstraction to read from all
          Memory Mapped IO address

  @param  addr 64-bit address

  @return 32-bit data read from the input address
**/
uint32_t
pal_mmio_read(uint64_t addr)
{

  uint32_t data;

  data = (*(volatile uint32_t *)addr);
  if (acs_policy_get_print_mmio() || (g_curr_module & g_enable_module))
      pal_print_msg(ACS_PRINT_INFO,
                    " %s Address = %8x  Data = %x\n",
                    __func__,
                    addr,
                    data);

  return data;

}

/**
  @brief  Provides a single point of abstraction to write to all
          Memory Mapped IO address

  @param  addr  64-bit address
  @param  data  8-bit data to write to address

  @return None
**/
void
pal_mmio_write8(uint64_t addr, uint8_t data)
{
  if (acs_policy_get_print_mmio() || (g_curr_module & g_enable_module))
      pal_print_msg(ACS_PRINT_INFO,
                    " %s Address = %llx  Data = %lx\n",
                    __func__,
                    addr,
                    data);

  *(volatile uint8_t *)addr = data;
}

/**
  @brief  Provides a single point of abstraction to write to all
          Memory Mapped IO address

  @param  addr  64-bit address
  @param  data  16-bit data to write to address

  @return None
**/
void
pal_mmio_write16(uint64_t addr, uint16_t data)
{
  if (acs_policy_get_print_mmio() || (g_curr_module & g_enable_module))
      pal_print_msg(ACS_PRINT_INFO,
                    " %s Address = %llx  Data = %lx\n",
                    __func__,
                    addr,
                    data);

  *(volatile uint16_t *)addr = data;
}

/**
  @brief  Provides a single point of abstraction to write to all
          Memory Mapped IO address

  @param  addr  64-bit address
  @param  data  64-bit data to write to address

  @return None
**/
void
pal_mmio_write64(uint64_t addr, uint64_t data)
{
  if (acs_policy_get_print_mmio() || (g_curr_module & g_enable_module))
      pal_print_msg(ACS_PRINT_INFO,
                    " %s Address = %llx  Data = %llx\n",
                    __func__,
                    addr,
                    data);

  *(volatile uint64_t *)addr = data;
}

/**
  @brief  Provides a single point of abstraction to write to all
          Memory Mapped IO address

  @param  addr  64-bit address
  @param  data  32-bit data to write to address

  @return None
**/
void
pal_mmio_write(uint64_t addr, uint32_t data)
{

  if (acs_policy_get_print_mmio() || (g_curr_module & g_enable_module))
      pal_print_msg(ACS_PRINT_INFO,
                    " %s Address = %8x  Data = %x\n",
                    __func__,
                    addr,
                    data);

    *(volatile uint32_t *)addr = data;
}

/**
  @brief  Sends a string to the output console without using Baremetal print function
          This function will get COMM port address and directly writes to the addr char-by-char

  @param  string  An ASCII string
  @param  data    data for the formatted output

  @return None
**/
void
pal_print_raw(uint64_t addr, char *string, uint64_t data)
{
    uint8_t j, buffer[16];
    uint8_t  i=0;
    for(;*string!='\0';++string){
        if(*string == '%'){
            ++string;
            if(*string == 'd'){
                while(data != 0){
                    j = data%10;
                    data = data/10;
                    buffer[i]= j + 48 ;
                    i = i+1;
                }
            } else if(*string == 'x' || *string == 'X'){
                while(data != 0){
                    j = data & 0xf;
                    data = data >> 4;
                    buffer[i]= j + ((j > 9) ? 55 : 48) ;
                    i = i+1;
                }
            }
            if(i>0) {
                while(i!=0)
                    *(volatile uint8_t *)addr = buffer[--i];
            } else
                *(volatile uint8_t *)addr = 48;

        } else
            *(volatile uint8_t *)addr = *string;
    }
}

/**
  @brief  Emit a warning indicating the given PAL API is not implemented.
  @param  api_name  Name of the unimplemented API (typically __func__).
**/
void
pal_warn_not_implemented(const char *api_name)
{
    if (api_name == NULL)
        return;

    pal_print_msg(ACS_PRINT_WARN,
                  "\n       %s is not implemented.",
                  api_name);
    pal_print_msg(ACS_PRINT_WARN,
                  "\n       Please implement the PAL function in test suite or");
    pal_print_msg(ACS_PRINT_WARN,
                  "\n       conduct an offline review for this rule.");
}

/**
  @brief  Free the memory allocated by UEFI Framework APIs
  @param  Buffer the base address of the memory range to be freed

  @return None
**/
void
pal_mem_free(void *Buffer)
{
  pal_mem_free_aligned(Buffer);
}

uint64_t
pal_mem_get_shared_addr()
{
  return (uint64_t)(gSharedMemory);
}

/**
  @brief  Free the shared memory region allocated above

  @param  None

  @return  None
**/
void
pal_mem_free_shared()
{
  pal_mem_free_aligned((void *)gSharedMemory);
}

/**
  @brief  Allocates requested buffer size in bytes in a contiguous memory
          and returns the base address of the range.

  @param  Size         allocation size in bytes
  @retval if SUCCESS   pointer to allocated memory
  @retval if FAILURE   NULL
**/
void *
pal_mem_alloc(uint32_t Size)
{
  uint32_t alignment = 0x08;
  return (void *)mem_alloc(alignment, Size);
}

/**
  @brief  Allocates requested buffer size in bytes with zeros in a contiguous memory
          and returns the base address of the range.

  @param  Size         allocation size in bytes
  @retval if SUCCESS   pointer to allocated memory
  @retval if FAILURE   NULL
**/
void *
pal_mem_calloc(uint32_t num, uint32_t Size)
{
  void* ptr;
  uint32_t alignment = 0x08;

  ptr = mem_alloc(alignment, num * Size);

  if (ptr != NULL)
  {
    pal_mem_set(ptr, num * Size, 0);
  }
  return ptr;
}

/**
  @brief  Returns the memory page size.

  @param  None

  @return Page size being used.
**/
uint32_t
pal_mem_page_size(void)
{
  return PLATFORM_PAGE_SIZE;
}

/**
  @brief  Allocates contiguous pages.

  @param  NumPages  Number of pages to allocate.

  @return Start address of base page.
**/
void *
pal_mem_alloc_pages(uint32_t NumPages)
{
  size_t alloc_size;

  if (NumPages == 0U) {
    return NULL;
  }

  alloc_size = (size_t)NumPages * (size_t)PLATFORM_PAGE_SIZE;
  /* Reject wrapped page-count multiplication. */
  if ((alloc_size / (size_t)PLATFORM_PAGE_SIZE) != (size_t)NumPages) {
    return NULL;
  }

  return (void *)mem_alloc(MEM_ALIGN_4K, alloc_size);
}

/**
  @brief  Frees contiguous pages starting at PageBase.

  @param  PageBase  Base address of the page range to free.
  @param  NumPages  Number of pages to free.

  @return None.
**/
void
pal_mem_free_pages(void *PageBase, uint32_t NumPages)
{
  (void) NumPages;
  mem_free(PageBase);
}

/**
  @brief  Allocates memory with the given alignment.

  @param  alignment  Requested alignment.
  @param  size       Requested allocation size.

  @return Pointer to the allocated memory.
**/
void *
pal_aligned_alloc(uint32_t alignment, uint32_t size)
{
  return (void *)mem_alloc(alignment, size);
}

/**
  @brief  Frees aligned memory allocated by pal_aligned_alloc.

  @param  Buffer  Base address of the aligned memory range.

  @return None.
**/
void
pal_mem_free_aligned(void *Buffer)
{
  mem_free(Buffer);
}

/**
  @brief  Frees cacheable memory.

  @param  Bdf   BDF of the requesting PCIe device.
  @param  Size  Size of the memory region to free.
  @param  Va    Virtual address of the memory to free.
  @param  Pa    Physical address of the memory to free.

  @return None.
**/
void
pal_mem_free_cacheable(uint32_t Bdf, uint32_t Size, void *Va, void *Pa)
{
  (void) Bdf;
  (void) Size;
  (void) Pa;
  mem_free(Va);
}


/**
  @brief  Allocate memory which is to be used to share data across PEs

  @param  num_pe      - Number of PEs in the system
  @param  sizeofentry - Size of memory region allocated to each PE

  @return None
**/
void
pal_mem_allocate_shared(uint32_t num_pe, uint32_t sizeofentry)
{
   gSharedMemory = 0;
   gSharedMemory = pal_mem_alloc(num_pe * sizeofentry);
   pal_pe_data_cache_ops_by_va((uint64_t)&gSharedMemory, CLEAN_AND_INVALIDATE);
}

/**
  @brief   Checks if System information is passed using Baremetal (BM)
           This api is also used to check if GIC/Interrupt Init ACS Code
           is used or not. In case of BM, ACS Code is used for INIT

  @param  None

  @return True/False
*/
uint32_t
pal_target_is_bm()
{
  return 1;
}

/**
  Copies a source buffer to a destination buffer, and returns the destination buffer.

  @param  DestinationBuffer   The pointer to the destination buffer of the memory copy.
  @param  SourceBuffer        The pointer to the source buffer of the memory copy.
  @param  Length              The number of bytes to copy from SourceBuffer to DestinationBuffer.

  @return DestinationBuffer.

**/
void *
pal_memcpy(void *DestinationBuffer, const void *SourceBuffer, uint32_t Length)
{

    uint32_t i;
    const char *s = (char *)SourceBuffer;
    char *d = (char *) DestinationBuffer;

    for(i = 0; i < Length; i++)
    {
        d[i] = s[i];
    }

    return d;
}

uint32_t pal_strncmp(const char8_t *str1, const char8_t *str2, uint32_t len)
{
    while ( len && *str1 && ( *str1 == *str2 ) )
    {
        ++str1;
        ++str2;
        --len;
    }
    if ( len == 0 )
    {
        return 0;
    }
    else
    {
        return ( *(unsigned char *)str1 - *(unsigned char *)str2 );
    }
}

void *pal_strncpy(void *DestinationStr, const void *SourceStr, uint32_t Length)
{
  const char *s = SourceStr;
  char *d = DestinationStr;

  if (d == NULL) {
      return NULL;
  }

  char* ptr = d;

  while (*s && Length--)
  {
      *d = *s;
      d++;
      s++;
  }
  *d = '\0';

  return ptr;
}

void
pal_mem_set(void *buf, uint32_t size, uint8_t value)
{
    unsigned char *ptr = buf;

    while (size--)
    {
        *ptr++ = (unsigned char)value;
    }

    return (void) buf;
}

/* The functions implemented below are to enable console prints via UART driver */

static int string_print(const char *str)
{
    int count = 0;

    for ( ; *str != '\0'; str++) {
        (void)pal_uart_putc(*str);
        count++;
    }

    return count;
}

static int unsigned_num_print(unsigned long long int unum, unsigned int radix,
                  char padc, int padn)
{
    /* Just need enough space to store 64 bit decimal integer */
    char num_buf[20];
    int i = 0, count = 0;
    unsigned int rem;

    /* num_buf is only large enough for radix >= 10 */
    if (radix < 10) {
        return 0;
    }

    do {
        rem = unum % radix;
        if (rem < 0xa)
            num_buf[i] = '0' + rem;
        else
            num_buf[i] = 'a' + (rem - 0xa);
        i++;
        unum /= radix;
    } while (unum > 0U);

    if (padn > 0) {
        while (i < padn) {
            (void)pal_uart_putc(padc);
            count++;
            padn--;
        }
    }

    while (--i >= 0) {
        (void)pal_uart_putc(num_buf[i]);
        count++;
    }

    return count;
}

int vprintf(const char *fmt, va_list args)
{
    int l_count;
    long long int num;
    unsigned long long int unum;
    char *str;
    char padc = '\0'; /* Padding character */
    int padn;         /* Number of characters to pad */
    int count = 0;    /* Number of printed characters */

    while (*fmt != '\0') {
        l_count = 0;
        padn = 0;

        if (*fmt == '%') {
            fmt++;
            /* Check the format specifier */
loop:
            switch (*fmt) {
            case '%':
                (void)pal_uart_putc('%');
                break;
            case 'i': /* Fall through to next one */
            case 'd':
                num = get_num_va_args(args, l_count);
                if (num < 0) {
                    (void)pal_uart_putc('-');
                    unum = (unsigned long long int)-num;
                    padn--;
                } else
                    unum = (unsigned long long int)num;

                count += unsigned_num_print(unum, 10,
                                padc, padn);
                break;
            case 's':
                str = va_arg(args, char *);
                count += string_print(str);
                break;
            case 'p':
                unum = (uintptr_t)va_arg(args, void *);
                if (unum > 0U) {
                    count += string_print("0x");
                    padn -= 2;
                }

                count += unsigned_num_print(unum, 16,
                                padc, padn);
                break;
            case 'x':
                unum = get_unum_va_args(args, l_count);
                count += unsigned_num_print(unum, 16,
                                padc, padn);
                break;
            case 'z':
                if (sizeof(size_t) == 8U)
                    l_count = 2;

                fmt++;
                goto loop;
            case 'l':
                l_count++;
                fmt++;
                goto loop;
            case 'u':
                unum = get_unum_va_args(args, l_count);
                count += unsigned_num_print(unum, 10,
                                padc, padn);
                break;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '0':
                padc = '0';
                padn = 0;
                fmt++;

                for (;;) {
                    char ch = *fmt;
                    if ((ch < '0') || (ch > '9')) {
                        goto loop;
                    }
                    padn = (padn * 10) + (ch - '0');
                    fmt++;
                }

            default:
                /* Exit on any other format specifier */
                return -1;
            }

            fmt++;
            continue;
        }
        else
        {
            (void)pal_uart_putc(*fmt);
            if (*fmt == '\n')
            {
                (void)pal_uart_putc('\r');
            }
        }

        fmt++;
        count++;
    }

    return count;
}

static const char *prefix_str[] = {
        "", "", "", "", ""};

const char *log_get_prefix(int log_level)
{
        int level;

        if (log_level > ACS_PRINT_ERR) {
                level = ACS_PRINT_ERR;
        } else if (log_level < ACS_PRINT_INFO) {
                level = ACS_PRINT_TEST;
        } else {
                level = log_level;
        }

        return prefix_str[level - 1];
}

void pal_uart_print(int log, const char *fmt, ...)
{
        va_list args;
        const char *prefix_str;

        prefix_str = log_get_prefix(log);

        while (*prefix_str != '\0') {
                pal_uart_putc(*prefix_str);
                prefix_str++;
        }

        va_start(args, fmt);
        (void)vprintf(fmt, args);
        va_end(args);
        (void) log;
}

/**
  @brief Dump DTB to file

  @param None

  @return None
**/
void
pal_dump_dtb()
{
  pal_print_msg(ACS_PRINT_ERR,
                " DTB dump not available for platform initialized with ACPI table\n",
                0);
}

/**
 * @brief  Changes requested buffer memory attributes to executable region
 *         and returns Success/Failure.
 *
 * @param  addr         Address of the buffer
 * @param  Size         size in bytes
 * @retval if FAILURE   1
 */
uint32_t pal_mem_set_wb_executable(void *addr, uint32_t size)
{
  (void) addr;
  (void) size;
  return 0;
}
