/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

typedef struct {
    unsigned char *saved;
    unsigned char *returned;
    unsigned char *seed;
    uint64_t registers[31];
    uint64_t vector_length;
    uint64_t fault_esr;
    uint64_t streaming_length;
    uint64_t returned_svcr;
    uint64_t returned_length;
    _Alignas(16) uint64_t arguments[17];
} val_smccc_probe_context_t;

/**
 * @brief Invoke one FIRME ABI while capturing live architectural state.
 * @param fid FIRME function identifier.
 * @param modes State and SMCCC hint selection.
 * @param context Probe buffers and captured result state.
 * @return VAL_SUCCESS on completion, otherwise VAL_ERROR after a trapped access.
 */
int val_smccc_probe_state(uint64_t fid, uint64_t modes,
                          val_smccc_probe_context_t *context);
