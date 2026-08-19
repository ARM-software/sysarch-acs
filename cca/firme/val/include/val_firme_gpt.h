/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#include "val_firme_abi.h"
#include "val_firme_dispatch.h"
#include "val_common/val_fault.h"
#include "val_firme_rules.h"
#include "val_firme_test.h"

/*
 * Single test data pattern used across all rules.
 *
 * This is written into the test granule before GPT transitions and read back
 * when the granule is expected to be accessible. It also makes debugging
 * easier when inspecting memory dumps.
 */
#define FIRME_TEST_DATA_PATTERN UINT64_C(0xDEADC0DEDEADC0DE)

/*
 * World-level primitives used by rule tests.
 * These are implemented in VAL so test sources stay policy-focused.
 */
int val_firme_mem_read64_world(firme_acs_world_t world,
                               uint64_t addr,
                               uint64_t *out_value,
                               val_fault_info_t *out_fault);

void val_firme_ns_write64(uint64_t addr, uint64_t value);

/* Status helpers (FIRME returns are signed, but often transported as unsigned). */
int64_t val_firme_as_i64(uint64_t v);
uint32_t val_firme_status_success(uint64_t x0, uint64_t x1, uint64_t exp_cnt);
uint32_t val_firme_status_deniedish(uint64_t x0);
uint32_t val_firme_status_not_supported(uint64_t x0);
uint32_t val_firme_status_invalid_parameters(uint64_t x0);

/* Granule-management feature helpers. */
uint32_t val_firme_gpt_gpi_set_advertised(firme_rule_res_t *out, uint64_t fail_step);
uint32_t val_firme_gpt_feature_reg1(uint64_t *feature_reg1, firme_rule_res_t *out,
                                    uint64_t fail_step);
uint64_t val_firme_gpt_pgs_size_from_feature_reg1(uint64_t feature_reg1);

/* Access validation helpers. */
uint32_t val_firme_expect_access_world(uint64_t gpi, firme_acs_world_t world);

uint32_t val_firme_check_access_matrix(uint64_t addr,
                                       uint64_t gpi,
                                       uint64_t expected_value,
                                       uint32_t check_value,
                                       firme_rule_res_t *out,
                                       uint64_t fail_step_base);
