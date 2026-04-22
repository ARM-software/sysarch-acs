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

#include "acs_run_request.h"
#include "val_interface.h"
#include "acs_memory.h"

/* Private backing storage for the current process-wide ACS run request. */
static acs_run_request_t g_run_request;

void acs_reset_run_request(void)
{
    g_run_request = (acs_run_request_t){0};
}

void acs_release_run_request(acs_run_request_t *ctx)
{
    if (ctx == NULL)
        return;

    if (ctx->rule_list_owned && ctx->rule_list != NULL)
        val_memory_free(ctx->rule_list);
    if (ctx->skip_rule_list_owned && ctx->skip_rule_list != NULL)
        val_memory_free(ctx->skip_rule_list);
    if (ctx->execute_modules_owned && ctx->execute_modules != NULL)
        val_memory_free(ctx->execute_modules);
    if (ctx->skip_modules_owned && ctx->skip_modules != NULL)
        val_memory_free(ctx->skip_modules);

    *ctx = (acs_run_request_t){0};
}

acs_run_request_t *acs_get_run_request_mut(void)
{
    return &g_run_request;
}

const acs_run_request_t *acs_get_run_request(void)
{
    return &g_run_request;
}
