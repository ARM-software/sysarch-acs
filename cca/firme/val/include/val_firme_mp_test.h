/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include "val_firme_dispatch.h"
#include "val_firme_rules.h"

#define VAL_TEST_ALL_PES UINT32_MAX
#define VAL_TEST_RESULT_WORDS 4U

typedef struct {
    firme_rule_id_t rule_id;
    firme_acs_world_t world;
    uint32_t phase;
    uint32_t argument;
} val_test_payload_t;

typedef struct {
    uint32_t pe_index;
    firme_rule_res_t result;
} val_test_pe_result_t;

typedef int (*val_test_run_payload_t)(uint32_t num_pe,
                                      const val_test_payload_t *payload,
                                      val_test_pe_result_t *results);

typedef int (*val_test_mp_entry_t)(firme_acs_world_t world, uint32_t num_pe,
                                  val_test_run_payload_t run_payload,
                                  firme_rule_res_t *out);

typedef int (*val_test_mp_payload_t)(firme_acs_world_t world, uint32_t phase,
                                    uint32_t argument, firme_rule_res_t *out);

void val_firme_test_payload_command(firme_acs_world_t world, firme_acs_cmd_t cmd,
                                     uint64_t rule_id, uint64_t phase,
                                     uint64_t argument, uint64_t pe_index,
                                     firme_acs_call_res_t *response);
