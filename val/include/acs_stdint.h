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

#ifndef __ACS_STDINT_H__
#define __ACS_STDINT_H__

#if defined(TARGET_UEFI)
#include <Base.h>

typedef INT8    int8_t;
typedef INT16   int16_t;
typedef INT32   int32_t;
typedef INT64   int64_t;
typedef UINT8   uint8_t;
typedef UINT16  uint16_t;
typedef UINT32  uint32_t;
typedef UINT64  uint64_t;
typedef UINTN   uintptr_t;
typedef INTN    intptr_t;
typedef INT64   intmax_t;
typedef UINT64  uintmax_t;
#elif defined(TARGET_LINUX) && defined(__KERNEL__)
#include <linux/types.h>

typedef s8      int8_t;
typedef s16     int16_t;
typedef s32     int32_t;
typedef s64     int64_t;
typedef u8      uint8_t;
typedef u16     uint16_t;
typedef u32     uint32_t;
typedef u64     uint64_t;
typedef unsigned long uintptr_t;
typedef long    intptr_t;
typedef s64     intmax_t;
typedef u64     uintmax_t;
#else
#include <stdint.h>
#endif

#endif /* __ACS_STDINT_H__ */
