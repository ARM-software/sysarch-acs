/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#if FIRME_ENABLE_CXL_SPDM

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "val_common/val_mem.h"

#define VAL_HOST_SPDM_HEAP_BYTES (384U * 1024U)
#define VAL_HOST_SPDM_ALIGNMENT 16U
#define VAL_HOST_SPDM_RNDR_RETRIES 16U
#define VAL_HOST_SPDM_RNDR_SHIFT 60U
#define VAL_HOST_SPDM_TIMER_MHZ 1000000ULL

typedef struct val_host_spdm_heap_block {
    size_t size;
    struct val_host_spdm_heap_block *next;
    uint32_t free;
} __attribute__((aligned(VAL_HOST_SPDM_ALIGNMENT))) val_host_spdm_heap_block_t;

static uint8_t g_val_host_spdm_heap[VAL_HOST_SPDM_HEAP_BYTES]
    __attribute__((aligned(VAL_HOST_SPDM_ALIGNMENT)));
static val_host_spdm_heap_block_t *g_val_host_spdm_heap_head;

/**
 * @brief Supply the mbedTLS zeroization hook without a hosted C library dependency.
 * @param buffer Secret storage to erase, or NULL when size is zero.
 * @param size Number of bytes to erase using non-elidable stores.
 */
void mbedtls_platform_zeroize(void *buffer, size_t size)
{
    volatile uint8_t *bytes = buffer;
    size_t index;

    if (!buffer)
        return;
    for (index = 0U; index < size; index++)
        bytes[index] = 0U;
}

/**
 * @brief Return an aligned allocation from the bounded host-only SPDM heap.
 * @param size Number of bytes to allocate.
 * @return Allocated memory or NULL when the heap is exhausted.
 */
void *malloc(size_t size)
{
    val_host_spdm_heap_block_t *block;
    val_host_spdm_heap_block_t *remainder;
    size_t aligned_size;

    if (size == 0U)
        size = 1U;
    if (size > sizeof(g_val_host_spdm_heap) -
               sizeof(val_host_spdm_heap_block_t) -
               (VAL_HOST_SPDM_ALIGNMENT - 1U))
        return NULL;

    aligned_size = (size + VAL_HOST_SPDM_ALIGNMENT - 1U) &
                   ~(size_t)(VAL_HOST_SPDM_ALIGNMENT - 1U);
    if (!g_val_host_spdm_heap_head) {
        g_val_host_spdm_heap_head =
            (val_host_spdm_heap_block_t *)g_val_host_spdm_heap;
        g_val_host_spdm_heap_head->size =
            sizeof(g_val_host_spdm_heap) - sizeof(val_host_spdm_heap_block_t);
        g_val_host_spdm_heap_head->next = NULL;
        g_val_host_spdm_heap_head->free = 1U;
    }

    for (block = g_val_host_spdm_heap_head; block; block = block->next) {
        if ((block->free == 0U) || (block->size < aligned_size))
            continue;

        if (block->size >= aligned_size + sizeof(*block) +
                           VAL_HOST_SPDM_ALIGNMENT) {
            remainder = (val_host_spdm_heap_block_t *)
                        ((uint8_t *)(block + 1) + aligned_size);
            remainder->size = block->size - aligned_size - sizeof(*block);
            remainder->next = block->next;
            remainder->free = 1U;
            block->next = remainder;
            block->size = aligned_size;
        }
        block->free = 0U;
        return block + 1;
    }

    return NULL;
}

/**
 * @brief Erase and release an allocation from the bounded host-only SPDM heap.
 * @param pointer Pointer returned by malloc or NULL.
 */
void free(void *pointer)
{
    val_host_spdm_heap_block_t *block;
    val_host_spdm_heap_block_t *next;
    volatile uint8_t *bytes;
    size_t index;

    if (!pointer || !g_val_host_spdm_heap_head)
        return;

    for (block = g_val_host_spdm_heap_head; block; block = block->next) {
        if ((void *)(block + 1) != pointer)
            continue;
        if (block->free != 0U)
            return;
        bytes = (volatile uint8_t *)pointer;
        for (index = 0U; index < block->size; index++)
            bytes[index] = 0U;
        block->free = 1U;
        break;
    }
    if (!block)
        return;

    for (block = g_val_host_spdm_heap_head; block && block->next;) {
        next = block->next;
        if ((block->free != 0U) && (next->free != 0U)) {
            block->size += sizeof(*next) + next->size;
            block->next = next->next;
        } else {
            block = next;
        }
    }
}

/**
 * @brief Allocate and zero memory for the mbedTLS platform layer.
 * @param count Number of elements.
 * @param size Size of one element.
 * @return Zeroed allocation or NULL on overflow/exhaustion.
 */
void *val_host_spdm_calloc(size_t count, size_t size)
{
    void *pointer;
    size_t total;

    if ((size != 0U) && (count > SIZE_MAX / size))
        return NULL;
    total = count * size;
    pointer = malloc(total);
    if (pointer)
        memset(pointer, 0, total);
    return pointer;
}

/**
 * @brief Release mbedTLS platform memory.
 * @param pointer Pointer returned by val_host_spdm_calloc.
 */
void val_host_spdm_free(void *pointer)
{
    free(pointer);
}

/**
 * @brief Determine whether the running PE exposes the RNDR instruction.
 * @return True when FEAT_RNG is present.
 */
bool val_host_spdm_has_rndr(void)
{
    uint64_t features;

    __asm__ volatile("mrs %0, id_aa64isar0_el1" : "=r"(features));
    return ((features >> VAL_HOST_SPDM_RNDR_SHIFT) & 0xfU) != 0U;
}

/**
 * @brief Obtain one 64-bit random value from the architectural RNDR source.
 * @param random_value Returned random value.
 * @return True on a successful RNDR instruction; false otherwise.
 */
bool libspdm_get_random_number_64(uint64_t *random_value)
{
    uint64_t value = 0U;
    uint32_t success = 0U;
    uint32_t attempt;

    if (!random_value || !val_host_spdm_has_rndr())
        return false;

    for (attempt = 0U; attempt < VAL_HOST_SPDM_RNDR_RETRIES; attempt++) {
        __asm__ volatile("mrs %0, S3_3_C2_C4_0\n\tcset %w1, ne"
                         : "=r"(value), "=r"(success) :: "cc");
        if (success != 0U) {
            *random_value = value;
            return true;
        }
    }
    return false;
}

/**
 * @brief Wait for a bounded SPDM retry interval using the virtual counter.
 * @param microseconds Requested wait duration in microseconds.
 */
void libspdm_sleep(uint64_t microseconds)
{
    uint64_t frequency;
    uint64_t start;
    uint64_t now;
    uint64_t ticks;

    if (microseconds == 0U)
        return;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(start));
    if (frequency == 0U)
        return;
    if (microseconds > UINT64_MAX / frequency)
        microseconds = UINT64_MAX / frequency;
    ticks = (frequency * microseconds) / VAL_HOST_SPDM_TIMER_MHZ;
    do {
        __asm__ volatile("mrs %0, cntvct_el0" : "=r"(now));
    } while ((now - start) < ticks);
}

/**
 * @brief Compare two byte sequences without interpreting their encoding.
 * @param left First byte sequence.
 * @param right Second byte sequence.
 * @param length Number of bytes to compare.
 * @return Negative, zero, or positive according to the first difference.
 */
int memcmp(const void *left, const void *right, size_t length)
{
    const unsigned char *left_bytes = left;
    const unsigned char *right_bytes = right;
    size_t index;

    for (index = 0U; index < length; index++) {
        if (left_bytes[index] != right_bytes[index])
            return (int)left_bytes[index] - (int)right_bytes[index];
    }
    return 0;
}

/**
 * @brief Copy bytes safely when the source and destination overlap.
 * @param destination Destination buffer.
 * @param source Source buffer.
 * @param length Number of bytes to copy.
 * @return Destination buffer.
 */
void *memmove(void *destination, const void *source, size_t length)
{
    unsigned char *destination_bytes = destination;
    const unsigned char *source_bytes = source;
    size_t index;

    if ((uintptr_t)destination > (uintptr_t)source &&
        (uintptr_t)destination - (uintptr_t)source < length) {
        for (index = length; index > 0U; index--)
            destination_bytes[index - 1U] = source_bytes[index - 1U];
    } else {
        for (index = 0U; index < length; index++)
            destination_bytes[index] = source_bytes[index];
    }
    return destination;
}

/**
 * @brief Find the first occurrence of a character, including NUL.
 * @param value NUL-terminated string.
 * @param character Character to find.
 * @return Pointer to the character, or NULL if absent.
 */
char *strchr(const char *value, int character)
{
    unsigned char target = (unsigned char)character;

    for (;;) {
        if ((unsigned char)*value == target)
            return (char *)value;
        if (*value == '\0')
            return NULL;
        value++;
    }
}

#endif
