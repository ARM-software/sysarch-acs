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

#ifndef __ACS_STDBOOL_H__
#define __ACS_STDBOOL_H__

#if defined(TARGET_UEFI)
#include <stdbool.h>
#elif defined(TARGET_LINUX) && defined(__KERNEL__)
#include <linux/types.h>
#else
#include <stdbool.h>
#endif

#endif /* __ACS_STDBOOL_H__ */
