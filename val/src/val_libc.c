/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
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

#include "val_libc.h"

/**
  @brief  Compare two memory buffers

  @param  s1   First buffer
  @param  s2   Second buffer
  @param  len  Number of bytes to compare

  @return 0  If buffers are identical
  @return Non-zero  If buffers differ
**/
int val_memory_compare(void *s1, void *s2, uint32_t len)
{
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;

    while (len--) {
        if (*p1 != *p2)
            return (int)(*p1 - *p2);
        p1++;
        p2++;
    }
    return 0;
}

/**
  @brief  Copy memory from source to destination

  @param  dst  Destination buffer
  @param  src  Source buffer
  @param  len  Number of bytes to copy

  @return Pointer to destination buffer
**/
void *val_memcpy(void *dst, void *src, uint32_t len)
{
    const unsigned char *s = src;
    unsigned char *d = dst;

    while (len--) {
        *d++ = *s++;
    }

    return dst;  // ␛return start of destination
}

/**
  @brief  Set memory with a byte value

  @param  dst    Buffer to fill
  @param  value  Byte value to set
  @param  count  Number of bytes to set

  @return Pointer to destination buffer
**/
void val_memory_set(void *dst, uint32_t size, uint8_t value)
{
    unsigned char *ptr = dst;
    while (size--)
        *ptr++ = (unsigned char)value;

    return (void) dst;
}

/**
  @brief  Compare two strings up to given length

  @param  str1    First string
  @param  str2    Second string
  @param  length  Maximum characters to compare

  @return 0  If strings match within length
  @return Non-zero  If strings differ
**/
uint32_t val_strncmp(char8_t *str1, char8_t *str2, uint32_t length)
{
    while (length && *str1 && (*str1 == *str2))
    {
        ++str1;
        ++str2;
        --length;
    }

    if (length == 0)
    {
        // Reached comparison limit — considered equal up to 'length'
        return 0;
    }

    return (*(const unsigned char *)str1 - *(const unsigned char *)str2);
}

/**
  @brief  Append source string to destination buffer

  @param  dest              Destination buffer
  @param  src               Source string
  @param  output_buff_size  Size of destination buffer

  @return Pointer to destination buffer
**/
char *val_strcat(char *dest, const char *src, size_t output_buff_size)
{
    size_t dest_len = 0;

    // Find the end of the destination string
    while (dest_len < output_buff_size && dest[dest_len] != '\0')
        dest_len++;

    // If dest is already full, do nothing
    if (dest_len == output_buff_size)
        return dest;

    size_t i = 0;
    // Append src to dest while ensuring buffer space remains
    while (src[i] != '\0' && dest_len < output_buff_size - 1)
    {
        dest[dest_len++] = src[i++];
    }

    // Null-terminate the resulting string
    dest[dest_len] = '\0';

    return dest;
}

/**
  @brief  Get length of a string

  @param  str  Input string

  @return Length of string (excluding null terminator)
**/
size_t val_strlen(char *str)
{
  size_t length = 0;

  while (str[length] != '\0')
  {
    ++length;
  }

  return length;
}

/**
  @brief  Copy source string to destination

  @param  dest  Destination buffer
  @param  src   Source string

  @return Pointer to destination buffer
**/
char *val_strcpy(char *dest, const char *src)
{
    char *ret = dest;

    while ((*dest++ = *src++));

    return ret;
}

/**
  @brief  Copy string with length limit

  @param  dest  Destination buffer
  @param  src   Source string
  @param  n     Maximum number of characters

  @return Pointer to destination buffer
**/
char *val_strncpy(char *dest, const char *src, size_t n)
{
    char *ret = dest;

    if (!dest || !src)
        return NULL;

    if (n == 0)
        return ret;

    while ((n > 1) && (*src != '\0'))
    {
        *dest++ = *src++;
        n--;
    }

    *dest = '\0';

    return ret;
}
