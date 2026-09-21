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
 * FIRME rule IDs map 1:1 to tests (DEN0149 FIRME 1.0 ALP4).
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
    FIRME_RULE_R0064 = 64,
    FIRME_RULE_R0065 = 65,
    FIRME_RULE_R0070 = 70,
    FIRME_RULE_R0071 = 71,
    FIRME_RULE_R0076 = 76,
    FIRME_RULE_R0077 = 77,
    FIRME_RULE_R0080 = 80,
    FIRME_RULE_R0081 = 81,
    FIRME_RULE_R0082 = 82,
    FIRME_RULE_R0086 = 86,
    FIRME_RULE_R0090 = 90,
    FIRME_RULE_R0092 = 92,
    FIRME_RULE_R0094 = 94,
    FIRME_RULE_R0095 = 95,
    FIRME_RULE_R0098 = 98,
    FIRME_RULE_R0102 = 102,
    FIRME_RULE_R0103 = 103,
    FIRME_RULE_R0111 = 111,
    FIRME_RULE_R0113 = 113,
    FIRME_RULE_R0114 = 114,
    FIRME_RULE_R0115 = 115,
    FIRME_RULE_R0117 = 117,
    FIRME_RULE_R0118 = 118,
    FIRME_RULE_R0120 = 120,
    FIRME_RULE_R0122 = 122,
    FIRME_RULE_R0123 = 123,
    FIRME_RULE_R0124 = 124,
    FIRME_RULE_R0126 = 126,
    FIRME_RULE_R0127 = 127,
    FIRME_RULE_R0129 = 129,
    FIRME_RULE_R0130 = 130,
    FIRME_RULE_R0134 = 134,
    FIRME_RULE_R0138 = 138,
    FIRME_RULE_R0139 = 139,
    FIRME_RULE_R0140 = 140,
    FIRME_RULE_R0141 = 141,
    FIRME_RULE_R0144 = 144,
    FIRME_RULE_R0156 = 156,
    FIRME_RULE_R0161 = 161,
    FIRME_RULE_R0167 = 167,
    FIRME_RULE_R0168 = 168,
    FIRME_RULE_R0170 = 170,
    FIRME_RULE_R0171 = 171,
    FIRME_RULE_R0173 = 173,
    FIRME_RULE_R0179 = 179,
    FIRME_RULE_R0180 = 180,
    FIRME_RULE_R0184 = 184,
    FIRME_RULE_R0185 = 185,
    FIRME_RULE_R0186 = 186,
    FIRME_RULE_R0188 = 188,
    FIRME_RULE_R0189 = 189,
    FIRME_RULE_R0191 = 191,
    FIRME_RULE_R0194 = 194,
    FIRME_RULE_R0195 = 195,
    FIRME_RULE_R0205 = 205,
    FIRME_RULE_R0207 = 207,
    FIRME_RULE_R0209 = 209,
    FIRME_RULE_R0210 = 210,
    FIRME_RULE_R0211 = 211,
    FIRME_RULE_R0212 = 212,
    FIRME_RULE_R0214 = 214,
    FIRME_RULE_R0216 = 216,
    FIRME_RULE_R0218 = 218,
    FIRME_RULE_R0225 = 225,
    FIRME_RULE_R0227 = 227,
    FIRME_RULE_R0231 = 231,
    FIRME_RULE_R0232 = 232,
    FIRME_RULE_R0236 = 236,
    FIRME_RULE_R0238 = 238,
    FIRME_RULE_R0239 = 239,
    FIRME_RULE_R0241 = 241,
    FIRME_RULE_R0245 = 245,
    FIRME_RULE_R0246 = 246,
    FIRME_RULE_R0344 = 344,
    FIRME_RULE_R0345 = 345,
    FIRME_RULE_R0346 = 346,
    FIRME_RULE_R0347 = 347,
    FIRME_RULE_R0348 = 348,
    FIRME_RULE_R0349 = 349,
    FIRME_RULE_R0350 = 350,
    FIRME_RULE_R0351 = 351,
    FIRME_RULE_R0352 = 352,
    FIRME_RULE_R0353 = 353,
    FIRME_RULE_R0354 = 354,
    FIRME_RULE_R0355 = 355,
    FIRME_RULE_R0356 = 356,
    FIRME_RULE_R0357 = 357,
    FIRME_RULE_R0358 = 358,
    FIRME_RULE_R0359 = 359,
    FIRME_RULE_R0360 = 360,
    FIRME_RULE_R0361 = 361,
    FIRME_RULE_R0362 = 362,
    FIRME_RULE_R0363 = 363,
    FIRME_RULE_R0364 = 364,
    FIRME_RULE_R0365 = 365,
    FIRME_RULE_R0366 = 366,
    FIRME_RULE_R0367 = 367,
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
