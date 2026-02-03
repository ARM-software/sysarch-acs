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

#ifndef __RULE_BASED_EXE_H__
#define __RULE_BASED_EXE_H__

#include "acs_val.h"
#include "acs_common.h"
#include "val_interface.h"
#include "rule_based_execution_enum.h"

#define RULE_STRING_SIZE 10
#define RULE_DESC_SIZE   44
#define INVALID_IDX 0xFFFFFFFF

/* ----------------------------  Struct  Definations --------------------------------------------*/

typedef uint32_t (*test_entry_fn_t)(uint32_t);
typedef uint32_t (*module_init_entry_t)(uint32_t);

/* Base rule id to test entry mapping structure definition*/
typedef struct {
    TEST_ENTRY_ID_e  test_entry_id;
    MODULE_NAME_e    module_id;     /* Module to which this test belongs */
    char8_t          rule_desc[RULE_DESC_SIZE];
    char8_t          platform_bitmask;
    RULE_FLAG_e      flag;
    uint32_t         test_num;
} rule_test_map_t;

/* Alias rules to Base rule mapping definition
 * base_rule_list must be terminated with RULE_ID_SENTINEL */
typedef struct {
    RULE_ID_e   alias_rule_id;
    RULE_ID_e   *base_rule_list; /* sentinel-terminated */
} alias_rule_map_t;

/* Module init status struct */
typedef struct {
    module_init_entry_t module_init;
    uint32_t            init_status;
} module_init_t;

/* rule list entry struct for rule_lookup.c */
typedef struct {
    RULE_ID_e rule_id;
    SBSA_LEVEL_e level;
} sbsa_rule_entry_t;

typedef struct {
    RULE_ID_e rule_id;
    BSA_LEVEL_e level;
    SOFTWARE_VIEW_e sw_view;
} bsa_rule_entry_t;

typedef struct {
    RULE_ID_e rule_id;
    PCBSA_LEVEL_e level;
} pcbsa_rule_entry_t;

typedef struct {
    RULE_ID_e rule_id;
    VBSA_LEVEL_e level;
} vbsa_rule_entry_t;

typedef struct {
    RULE_ID_e rule_id;
    PFDI_LEVEL_e level;
} pfdi_rule_entry_t;

/* ---------------------------- Helper functions declarations ---------------------------------- */
void     quick_sort_rule_list(RULE_ID_e *rule_list, uint32_t list_size);
uint32_t check_module_init(MODULE_NAME_e module_id);
uint32_t alias_rule_map_get_index(RULE_ID_e alias_rule_id);
void     print_rule_test_start(uint32_t rule_enum, uint32_t indent);
void     print_rule_test_status(uint32_t rule_enum, uint32_t indent, uint32_t status);
void     rule_status_map_reset(void);
bool     rule_in_list(RULE_ID_e rid, const RULE_ID_e *list, uint32_t count);

/* ---------------------------- Externs ---------------------------- */
extern uint32_t rule_status_map[RULE_ID_SENTINEL];
extern RULE_ID_e *g_skip_rule_list;
extern uint32_t   g_skip_rule_count;
extern uint32_t   g_arch_selection;

/* Rule lookup tables (defined in rule_lookup.c) */
extern const bsa_rule_entry_t bsa_rule_list[];
extern const sbsa_rule_entry_t sbsa_rule_list[];
extern const pcbsa_rule_entry_t pcbsa_rule_list[];
extern const vbsa_rule_entry_t vbsa_rule_list[];
extern const pfdi_rule_entry_t pfdi_rule_list[];

/* Global selections configured by the app */
extern uint32_t g_level_filter_mode;  /* LEVEL_FILTER_MODE_e */
extern uint32_t g_level_value;        /* numeric value interpreted per-arch */
/* BSA-only software view selector bitmask; 0 means no filtering */
extern uint32_t g_bsa_sw_view_mask; /* bit (1<<SW_OS | 1<<SW_HYP | 1<<SW_PS) */

/* ------------------------------------ VAL APIs ------------------------------------------------*/
uint32_t filter_rule_list_by_cli(RULE_ID_e **rule_list, uint32_t list_size);
void run_tests(RULE_ID_e *rule_list, uint32_t list_size);

#endif /* __RULE_BASED_EXE_H__ */
