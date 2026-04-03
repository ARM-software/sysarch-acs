/** @file
 * Copyright (c) 2025-2026, Arm Limited or its affiliates. All rights reserved.
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

#ifndef _PAL_COMMON_INTF_H_
#define _PAL_COMMON_INTF_H_

#ifdef TARGET_LINUX
#include <linux/stdarg.h>
#include <linux/stddef.h>
#include <linux/types.h>
#else
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#endif


#define LOG_BUFFER_SIZE 8192
#ifndef static_assert
#define static_assert _Static_assert
#endif

void pal_uart_putc(char c);

#endif /* _PAL_COMMON_INTF_H_ */
