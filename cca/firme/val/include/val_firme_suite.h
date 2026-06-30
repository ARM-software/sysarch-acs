/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#include "val_firme_dispatch.h"
#include "val_firme_rules.h"

/* FIRME-ACS release string printed by the host (NS-EL2) banner. */
#define FIRME_ACS_VERSION_STR "FIRME Alpha1 ACS v0.5 Alpha"

typedef enum {
    FIRME_CHAPTER_BASE = 5,
    FIRME_CHAPTER_GPT = 1,
    FIRME_CHAPTER_MEC = 2,
    FIRME_CHAPTER_IDE = 3,
    FIRME_CHAPTER_ATTEST = 4,
    FIRME_CHAPTER_OTHER = 0xff,
} firme_chapter_id_t;

typedef enum {
    FIRME_WORLD_MASK_NS = (1u << 0),
    FIRME_WORLD_MASK_SECURE = (1u << 1),
    FIRME_WORLD_MASK_REALM = (1u << 2),
} firme_world_mask_t;

typedef int (*firme_rule_fn_t)(uint64_t base_pa, firme_rule_res_t *out);

typedef struct {
    firme_chapter_id_t chapter;
    firme_rule_id_t rule_id;          /* Numeric rule ID (DEN0149) */
    const char *rule_str;             /* Printable rule string (e.g. "R0085") */
    const char *suite;                /* Suite name (e.g. "firme") */
    const char *test_name;            /* Test name grouping (e.g. "gpt_transition_policy") */
    const char *desc;                 /* One-line description */

    uint32_t required_worlds_mask;    /* Worlds that must run for coverage */
    uint32_t optional_worlds_mask;    /* Worlds to attempt if available */

    /* Optional per-world check labels printed by the NS dispatcher. */
    const char *check_ns;
    const char *check_secure;
    const char *check_realm;

    /* Per-world bodies (NULL means not implemented for that world). */
    firme_rule_fn_t run_ns;
    firme_rule_fn_t run_secure;
    firme_rule_fn_t run_realm;
} firme_test_case_t;

/*
 * Global test registry (ordered by chapter). Prefer the accessors below.
 * Exported to simplify debugging and external tooling.
 */
extern const firme_test_case_t g_firme_test_cases[];
extern const uint32_t g_firme_test_case_count;

/**
 *   @brief    - Return the FIRME-ACS test registry (ordered by chapter).
 *   @param    - out_count : Returned number of test cases.
 *   @return   - Pointer to a read-only array of test cases.
**/
const firme_test_case_t *val_firme_suite_cases(uint32_t *out_count);

/**
 *   @brief    - Lookup a test case by rule ID.
 *   @param    - rule_id : Rule ID.
 *   @return   - Pointer to the test case, or NULL if not found.
**/
const firme_test_case_t *val_firme_suite_find(firme_rule_id_t rule_id);

/**
 *   @brief    - Map a chapter ID to a printable string.
 *   @param    - chapter : Chapter identifier.
 *   @return   - Chapter name string.
**/
const char *val_firme_chapter_str(firme_chapter_id_t chapter);

/**
 *   @brief    - Return the default GPT base PA for the given caller world.
 *   @param    - world : Target world selector.
 *   @return   - Platform base PA for GPT tests (0 if not configured).
**/
uint64_t val_firme_gpt_base_for_world(firme_acs_world_t world);
