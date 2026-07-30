/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#define VAL_SMCCC_INPUT_REGISTER_COUNT 18U

typedef struct {
    uint64_t x0;
    uint64_t x1;
    uint64_t x2;
    uint64_t x3;
    uint64_t x4;
    uint64_t x5;
    uint64_t x6;
    uint64_t x7;
} smccc_res_t;

typedef struct __attribute__((aligned(16))) {
    uint64_t registers[VAL_SMCCC_INPUT_REGISTER_COUNT];
} smccc_input_registers_t;

int val_smccc_test_input_capture_enable(void);
void val_smccc_test_input_capture_disable(void);
int val_smccc_test_input_capture_read(smccc_input_registers_t *input);

void val_smccc_smc_call(uint64_t fid,
                        uint64_t arg1, uint64_t arg2, uint64_t arg3,
                        uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7,
                        smccc_res_t *res);
