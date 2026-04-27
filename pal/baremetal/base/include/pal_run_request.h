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

#ifndef __PAL_RUN_REQUEST_H__
#define __PAL_RUN_REQUEST_H__

#include "acs_stdint.h"

#include "rule_based_execution_enum.h"

/*
 * Target-owned baremetal run-request defaults. These are borrowed/static
 * inputs used to seed the mutable shared run request before compile-time or
 * EL3 overrides are applied.
 */
typedef struct {
    RULE_ID_e *rule_list;
    uint32_t   rule_count;
    RULE_ID_e *skip_rule_list;
    uint32_t   skip_rule_count;
    uint32_t  *execute_modules;
    uint32_t   num_modules;
    uint32_t  *skip_modules;
    uint32_t   num_skip_modules;
    uint32_t   level_filter_mode; /* LEVEL_FILTER_MODE_e */
} acs_platform_run_request_defaults_t;

const acs_platform_run_request_defaults_t *acs_get_platform_run_request_defaults(void);

#endif /* __PAL_RUN_REQUEST_H__ */
