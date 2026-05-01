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

#ifndef __ACS_APP_DEFS_H__
#define __ACS_APP_DEFS_H__

#include "acs_app_versions.h"
#include "rule_based_execution_enum.h"

#define SIZE_4K 0x1000

#define G_BSA_LEVEL             1
#define BSA_MIN_LEVEL_SUPPORTED 1
#define BSA_MAX_LEVEL_SUPPORTED 1

#define G_SBSA_LEVEL             4
#define SBSA_MIN_LEVEL_SUPPORTED 3
#define SBSA_MAX_LEVEL_SUPPORTED 9

#define G_PCBSA_LEVEL             1
#define PCBSA_MIN_LEVEL_SUPPORTED 1
#define PCBSA_MAX_LEVEL_SUPPORTED 1

#define LEVEL_PRINT_FORMAT(level, filter_mode, fr_level) ((filter_mode == LVL_FILTER_FR) ? \
    ((filter_mode == LVL_FILTER_ONLY && level == fr_level) ? \
    "\n Starting tests for only level FR " : "\n Starting tests for level FR ") : \
    ((filter_mode == LVL_FILTER_ONLY) ? \
    "\n Starting tests for only level %2d " : "\n Starting tests for level %2d "))

#endif /* __ACS_APP_DEFS_H__ */
