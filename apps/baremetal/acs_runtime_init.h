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

#ifndef __ACS_RUNTIME_INIT_H__
#define __ACS_RUNTIME_INIT_H__

#include "acs_execution_policy.h"
#include "acs_run_request.h"

void acs_load_run_request_defaults(acs_run_request_t *ctx);
void acs_load_execution_policy_defaults(acs_execution_policy_t *policy);
void acs_apply_el3_params(acs_run_request_t *ctx, acs_execution_policy_t *policy);
bool acs_is_module_enabled(uint32_t module_base);
void acs_apply_compile_params(acs_run_request_t *ctx, acs_execution_policy_t *policy);

#endif /* __ACS_RUNTIME_INIT_H__ */
