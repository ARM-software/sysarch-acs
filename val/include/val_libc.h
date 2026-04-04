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

#ifndef __VAL_LIBC_H__
#define __VAL_LIBC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "pal_interface.h"

int val_memory_compare(void *s1, void *s2, uint32_t len);

void *val_memcpy(void *dst, void *src, uint32_t len);

void val_memory_set(void *dst, uint32_t size, uint8_t value);

uint32_t val_strncmp(char8_t *str1, char8_t *str2, uint32_t length);

char *val_strcat(char *dest, const char *src, size_t output_buff_size);

size_t val_strlen(char *str);

char *val_strcpy(char *dest, const char *src);

char *val_strncpy(char *dest, const char *src, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* __VAL_LIBC_H__ */
