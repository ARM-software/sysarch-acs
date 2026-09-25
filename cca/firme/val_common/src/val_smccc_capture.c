/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_common/val_smccc.h"
#include "val_common/val_status.h"
#include "aarch64/sysreg.h"

volatile uint64_t g_val_smccc_input_capture_enabled;
volatile uint64_t g_val_smccc_input_capture_valid;
volatile uint64_t g_val_smccc_input_capture_owner;
volatile uint64_t g_val_smccc_input_capture_registers[
    VAL_SMCCC_INPUT_REGISTER_COUNT] __attribute__((aligned(16)));

/**
 *   @brief    - Enable capture-only mode for the next SMCCC call.
 *   @return   - VAL_SUCCESS when enabled, otherwise VAL_ERROR.
 **/
int val_smccc_test_input_capture_enable(void)
{
    uint32_t register_index = 0U;

    if (g_val_smccc_input_capture_enabled == VAL_TRUE)
        return VAL_ERROR;

    g_val_smccc_input_capture_owner = read_mpidr_el1();
    g_val_smccc_input_capture_valid = VAL_FALSE;
    for (register_index = 0U;
         register_index < VAL_SMCCC_INPUT_REGISTER_COUNT;
         register_index++)
        g_val_smccc_input_capture_registers[register_index] = 0ULL;

    __atomic_store_n(&g_val_smccc_input_capture_enabled, VAL_TRUE, __ATOMIC_RELEASE);
    return VAL_SUCCESS;
}

/**
 *   @brief    - Disable SMCCC input capture-only mode.
 *   @return   - void.
 **/
void val_smccc_test_input_capture_disable(void)
{
    if (g_val_smccc_input_capture_owner == read_mpidr_el1())
        __atomic_store_n(&g_val_smccc_input_capture_enabled, VAL_FALSE, __ATOMIC_RELEASE);
}

/**
 *   @brief    - Read the input registers captured before SMCCC dispatch.
 *   @param    - input : Destination for x0-x17.
 *   @return   - VAL_SUCCESS when a frame is available, otherwise VAL_ERROR.
 **/
int val_smccc_test_input_capture_read(smccc_input_registers_t *input)
{
    uint32_t register_index = 0U;

    if (!input ||
        (g_val_smccc_input_capture_owner != read_mpidr_el1()) ||
        (g_val_smccc_input_capture_enabled != VAL_TRUE) ||
        (g_val_smccc_input_capture_valid != VAL_TRUE))
        return VAL_ERROR;

    for (register_index = 0U;
         register_index < VAL_SMCCC_INPUT_REGISTER_COUNT;
         register_index++) {
        input->registers[register_index] =
            g_val_smccc_input_capture_registers[register_index];
    }

    return VAL_SUCCESS;
}
