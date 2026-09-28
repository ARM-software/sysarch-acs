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

typedef struct {
    uint64_t range_base;
    uint64_t range_top;
    uint64_t range_size;
    uint64_t granule_size;
    uint64_t protected_pa_size;
} val_firme_gpt_info_window_t;

#define VAL_FIRME_GPT_TRACE_MAX_PHASES 32U

typedef enum {
    VAL_FIRME_GPT_PHASE_GPI_SET = 0,
    VAL_FIRME_GPT_PHASE_OP_CONTINUE = 1,
} val_firme_gpt_phase_abi_t;

typedef struct {
    val_firme_gpt_phase_abi_t abi;
    uint64_t start_granule;
    uint64_t remaining_before;
    uint64_t input_cookie;
    int64_t status;
    uint64_t granule_count;
    uint64_t output_cookie;
} val_firme_gpt_phase_t;

typedef struct {
    val_firme_gpt_phase_t phases[VAL_FIRME_GPT_TRACE_MAX_PHASES];
    uint64_t requested_count;
    uint64_t completed_count;
    uint32_t phase_count;
    uint32_t malformed;
    uint32_t exhausted;
    uint32_t unexpected_status;
    uint32_t observation_failed;
} val_firme_gpt_trace_t;

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
uint32_t val_firme_gpt_restore_ns(uint64_t base_pa, firme_rule_res_t *out,
                                   uint64_t fail_step);

/* Granule-management feature helpers. */
uint32_t val_firme_gpt_gpi_set_advertised(firme_rule_res_t *out, uint64_t fail_step);
uint32_t val_firme_gpt_feature_reg1(uint64_t *feature_reg1, firme_rule_res_t *out,
                                    uint64_t fail_step);
uint64_t val_firme_gpt_pgs_size_from_feature_reg1(uint64_t feature_reg1);
uint64_t val_firme_gpt_range_size_from_feature_reg1(uint64_t feature_reg1);
uint64_t val_firme_gpt_pps_size_from_feature_reg1(uint64_t feature_reg1);
uint32_t val_firme_gpt_info_advertised(firme_rule_res_t *out,
                                       uint64_t fail_step);
uint32_t val_firme_gpt_op_continue_advertised(firme_rule_res_t *out,
                                              uint64_t fail_step);
int val_firme_gpt_trace_transition(uint64_t base_pa,
                                   uint64_t granule_count,
                                   uint64_t target_gpi,
                                   uint32_t observe_prefix,
                                   firme_rule_res_t *out,
                                   uint64_t fail_step,
                                   val_firme_gpt_trace_t *trace);
uint32_t val_firme_gpt_operation_status_valid(int64_t status);
uint32_t val_firme_gpt_operation_count_valid(int64_t status,
                                              uint64_t count,
                                              uint64_t remaining);
void val_firme_gpt_rule_result_init(firme_rule_res_t *out);
int val_firme_gpt_rule_result_status(const firme_rule_res_t *out);
void val_firme_gpt_rule_result_fail(firme_rule_res_t *out,
                                    uint64_t fail_step,
                                    uint64_t last_x0,
                                    uint64_t last_x1);
uint32_t val_firme_gpt_trace_has_status(
    const val_firme_gpt_trace_t *trace, val_firme_gpt_phase_abi_t abi,
    int64_t status);
uint32_t val_firme_gpt_local_prefix_matches(
    uint64_t base_pa, uint64_t requested_count, uint64_t completed_count,
    firme_rule_res_t *out, uint64_t fail_step);
uint32_t val_firme_gpt_info_prepare_window(
    uint64_t test_pa, val_firme_gpt_info_window_t *window,
    firme_rule_res_t *out, uint64_t fail_step);
uint32_t val_firme_gpt_info_success_valid(
    const val_firme_gpt_info_window_t *window, const firme_res_t *result,
    firme_rule_res_t *out, uint64_t fail_step);

/* Access validation helpers. */
uint32_t val_firme_expect_access_world(uint64_t gpi, firme_acs_world_t world);

uint32_t val_firme_check_access_matrix(uint64_t addr,
                                       uint64_t gpi,
                                       uint64_t expected_value,
                                       uint32_t check_value,
                                       firme_rule_res_t *out,
                                       uint64_t fail_step_base);
