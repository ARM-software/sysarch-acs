/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#include "val_common/val_status.h"

/*
 * FIRME rule IDs map 1:1 to tests (DEN0149).
 * Keep IDs stable so logs and automation can key off them.
 */
typedef enum {
    FIRME_RULE_R0007 = 7,
    FIRME_RULE_R0008 = 8,
    FIRME_RULE_R0009 = 9,
    FIRME_RULE_R0010 = 10,
    FIRME_RULE_R0011 = 11,
    FIRME_RULE_R0012 = 12,
    FIRME_RULE_R0013 = 13,
    FIRME_RULE_R0014 = 14,
    FIRME_RULE_R0015 = 15,
    FIRME_RULE_R0023 = 23,
    FIRME_RULE_R0024 = 24,
    FIRME_RULE_R0025 = 25,
    FIRME_RULE_R0026 = 26,
    FIRME_RULE_R0031 = 31,
    FIRME_RULE_R0033 = 33,
    FIRME_RULE_R0034 = 34,
    FIRME_RULE_R0035 = 35,
    FIRME_RULE_R0038 = 38,
    FIRME_RULE_R0041 = 41,
    FIRME_RULE_R0044 = 44,
    FIRME_RULE_R0047 = 47,
    FIRME_RULE_R0048 = 48,
    FIRME_RULE_R0051 = 51,
    FIRME_RULE_R0053 = 53,
    FIRME_RULE_R0054 = 54,
    FIRME_RULE_R0055 = 55,
    FIRME_RULE_R0063 = 63,
    FIRME_RULE_R0064 = 64,
    FIRME_RULE_R0069 = 69,
    FIRME_RULE_R0070 = 70,
    FIRME_RULE_R0085 = 85,
    FIRME_RULE_R0086 = 86,
    FIRME_RULE_R0089 = 89,
    FIRME_RULE_R0206 = 206,
    FIRME_RULE_R0208 = 208,
} firme_rule_id_t;

typedef struct {
    uint64_t status;     /* ACS_STATUS_PASS/FAIL/SKIP */
    uint64_t fail_step;  /* step index for debugging */
    uint64_t last_x0;    /* last FIRME x0 (status) observed */
    uint64_t last_x1;    /* last FIRME x1 (count/value) observed */
} firme_rule_res_t;

/*
 * Rule bodies return VAL_SUCCESS when the test body executed and recorded the
 * architectural outcome in firme_rule_res_t. Return VAL_ERROR only for ACS
 * framework errors that prevent the rule body from producing a result.
 */
int val_firme_rule_run_secure(firme_rule_id_t rule, uint64_t base_pa, firme_rule_res_t *out);
int val_firme_rule_run_realm(firme_rule_id_t rule, uint64_t base_pa, firme_rule_res_t *out);
int val_firme_rule_run_ns(firme_rule_id_t rule, uint64_t base_pa, firme_rule_res_t *out);
