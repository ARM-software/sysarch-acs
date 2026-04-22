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

#ifndef __ACS_RUN_REQUEST_H__
#define __ACS_RUN_REQUEST_H__

#include "pal_interface.h"
#include "rule_based_execution_enum.h"

/*
 * This module owns a single process-wide ACS run request. Callers build or
 * update that shared request through the mutable accessor, and consume it
 * through the read-only accessor when mutation is not intended.
 *
 * acs_create_run_request() is different: it constructs a detached by-value
 * request object and does not touch the shared singleton managed here.
 */

/*
 * acs_run_request_t captures the run-selection request for one ACS invocation.
 * It contains only "what to run" inputs gathered from platform defaults, build
 * overrides, CLI parsing, or EL3-provided parameters:
 * - explicit rule and skip-rule lists
 * - explicit module and skip-module lists
 * - requested architecture selection
 * - requested level filter mode and value
 * - requested software-view mask
 *
 * The *_owned flags indicate whether this request object owns the dynamically
 * allocated list storage and therefore must free it during release.
 */
typedef struct acs_run_request {
    RULE_ID_e *rule_list;
    uint32_t   rule_count;
    RULE_ID_e *skip_rule_list;
    uint32_t   skip_rule_count;
    uint32_t  *execute_modules;
    uint32_t   num_modules;
    uint32_t  *skip_modules;
    uint32_t   num_skip_modules;
    uint32_t   arch_selection;
    uint32_t   level_filter_mode; /* LEVEL_FILTER_MODE_e */
    uint32_t   level_value;       /* numeric value interpreted per-arch */
    uint32_t   bsa_sw_view_mask;  /* bit (1<<SW_OS | 1<<SW_HYP | 1<<SW_PS) */
    bool       rule_list_owned;
    bool       skip_rule_list_owned;
    bool       execute_modules_owned;
    bool       skip_modules_owned;
} acs_run_request_t;

/*
 * Construct a detached run request value from the provided selection fields.
 *
 * The returned object starts with all ownership flags cleared, so it is
 * suitable for borrowed/static storage unless the caller explicitly marks
 * owned lists later.
 */
static inline acs_run_request_t acs_create_run_request(
    RULE_ID_e *rule_list,
    uint32_t rule_count,
    RULE_ID_e *skip_rule_list,
    uint32_t skip_rule_count,
    uint32_t *execute_modules,
    uint32_t num_modules,
    uint32_t *skip_modules,
    uint32_t num_skip_modules,
    uint32_t arch_selection,
    uint32_t level_filter_mode,
    uint32_t level_value,
    uint32_t bsa_sw_view_mask)
{
    acs_run_request_t ctx = {0};

    ctx.rule_list = rule_list;
    ctx.rule_count = rule_count;
    ctx.skip_rule_list = skip_rule_list;
    ctx.skip_rule_count = skip_rule_count;
    ctx.execute_modules = execute_modules;
    ctx.num_modules = num_modules;
    ctx.skip_modules = skip_modules;
    ctx.num_skip_modules = num_skip_modules;
    ctx.arch_selection = arch_selection;
    ctx.level_filter_mode = level_filter_mode;
    ctx.level_value = level_value;
    ctx.bsa_sw_view_mask = bsa_sw_view_mask;
    ctx.rule_list_owned = false;
    ctx.skip_rule_list_owned = false;
    ctx.execute_modules_owned = false;
    ctx.skip_modules_owned = false;

    return ctx;
}

/*
 * Reset the shared run request singleton to an all-zero state.
 *
 * This clears fields only; it does not free any dynamically allocated list
 * storage that may still be referenced by the current request.
 */
void acs_reset_run_request(void);

/*
 * Release any owned list storage referenced by the provided run request and
 * then reset the request to an all-zero state.
 *
 * Borrowed/static list storage is left untouched. Passing the shared singleton
 * returned by acs_get_run_request_mut() is the normal use case.
 */
void acs_release_run_request(acs_run_request_t *ctx);

/*
 * Return mutable access to the shared run request singleton.
 *
 * This is the only accessor intended for in-place construction or mutation of
 * the current ACS run request.
 */
acs_run_request_t *acs_get_run_request_mut(void);

/*
 * Return read-only access to the shared run request singleton.
 *
 * Use this accessor when consuming request state and mutation is not intended.
 */
const acs_run_request_t *acs_get_run_request(void);

#endif /* __ACS_RUN_REQUEST_H__ */
