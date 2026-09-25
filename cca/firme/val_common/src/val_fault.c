/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_common/val_fault.h"

#include "aarch64/sysreg.h"
#include "val_common/val_status.h"
#include "val_common/val_mp.h"

typedef struct {
    volatile uint64_t active;
    volatile uint64_t resume_pc;
    volatile uint64_t faulted;
    volatile uint64_t esr;
    volatile uint64_t far;
} fault_ctx_t;

static fault_ctx_t g_fault_contexts[PAL_PE_COUNT];

/** @brief Obtain this PE's recovery scope without aliasing unknown affinities. */
static fault_ctx_t *val_fault_context(void)
{
    uint32_t pe_index = val_pe_index();

    if (pe_index >= PAL_PE_COUNT) {
        for (;;)
            val_wfe();
    }

    return &g_fault_contexts[pe_index];
}

/** @brief Clear this PE's recovery state on cold or warm boot, never remotely. */
void val_fault_reset(void)
{
    fault_ctx_t *context = val_fault_context();

    context->active = 0U;
    context->resume_pc = 0U;
    context->faulted = 0U;
    context->esr = 0U;
    context->far = 0U;
}

/**
 *   @brief    - Update ELR_ELx for the current exception level.
 *   @param    - v : Return address to install into ELR_EL1/ELR_EL2.
 *   @return   - void
**/
static inline void write_elr_current(uint64_t v)
{
    uint64_t el = (read_currentel() >> 2) & 0x3u;
    if (el == 2u) {
        write_elr_el2(v);
    } else if (el == 1u) {
        write_elr_el1(v);
    }
}

/**
 *   @brief    - Handle a synchronous exception triggered during a probe access.
 *   @param    - esr : ESR_ELx value.
 *   @param    - far : FAR_ELx value.
 *   @return   - 1 if the exception was handled (probe resumed), 0 otherwise.
**/
int val_fault_handle_sync(uint64_t esr, uint64_t far)
{
    fault_ctx_t *context = val_fault_context();

    if (context->active == 0U) {
        return VAL_FALSE;
    }

    /*
     * When running a probe access, always recover so the test can report what
     * happened and the suite does not hang.
     *
     * In the expected case, the exception is an abort (or SError) and `esr`
     * encodes the fault class. If `esr` is unexpected/zero (e.g. due to a bad
     * vector layout or an unusual trap), still resume and let the test logic
     * mark it as FAIL because it won't match the expected GPF encoding.
     */
    context->faulted = 1u;
    context->esr = esr;
    context->far = far;

    write_elr_current(context->resume_pc);
    return VAL_TRUE;
}

/**
 *   @brief    - Probe a 64-bit read from a physical address and recover from expected faults.
 *   @param    - addr      : Address to read.
 *   @param    - out_value : Returned value if the access succeeds (optional).
 *   @param    - out_fault : Fault info if the access faults (optional).
 *   @return   - 0 on success, non-zero if a fault was observed.
**/
int val_probe_read64(uint64_t addr, uint64_t *out_value, val_fault_info_t *out_fault)
{
    fault_ctx_t *context = val_fault_context();
    uint64_t value = 0;
    volatile uint64_t *p = (volatile uint64_t *)(uintptr_t)addr;

    if (out_fault) {
        out_fault->faulted = 0;
        out_fault->esr = 0;
        out_fault->far = 0;
    }

    if (context->active != 0U)
        return VAL_ERROR;

    context->faulted = 0u;
    context->esr = 0;
    context->far = 0;
    context->resume_pc = (uint64_t)(uintptr_t)&&resume;
    context->active = 1U;

    /*
     * Some implementations may report protection violations as asynchronous
     * SError rather than a synchronous data abort. To keep probes robust and
     * avoid halts:
     * - mask SError during the access,
     * - unmask while the probe context is still active so a pending SError
     *   is taken immediately and recovered by the probe handler.
     */
    mask_serror();
    value = *p;
    dsb_sy();
    isb_sy();
    unmask_serror();
    dsb_sy();
    isb_sy();

    /*
     * Give asynchronous errors (SError) a short window to be delivered while
     * the probe context is still active. This avoids halts when a protection
     * violation is reported slightly after the faulting access.
     */
    for (volatile uint32_t i = 0; i < 256u; i++) { /* delay for async SError delivery */
    }
    dsb_sy();
    isb_sy();

resume:
    context->active = 0u;

    if (context->faulted != 0u) {
        if (out_fault) {
            out_fault->faulted = 1u;
            out_fault->esr = context->esr;
            out_fault->far = context->far;
        }
        return VAL_ERROR;
    }

    if (out_value) {
        *out_value = value;
    }
    return VAL_SUCCESS;
}

/**
 *   @brief    - Probe a 64-bit write to a physical address and recover from expected faults.
 *   @param    - addr      : Address to write.
 *   @param    - value     : Value to store.
 *   @param    - out_fault : Fault info if the access faults (optional).
 *   @return   - 0 on success, non-zero if a fault was observed.
**/
int val_probe_write64(uint64_t addr, uint64_t value, val_fault_info_t *out_fault)
{
    fault_ctx_t *context = val_fault_context();
    volatile uint64_t *p = (volatile uint64_t *)(uintptr_t)addr;
    if (out_fault) {
        out_fault->faulted = 0;
        out_fault->esr = 0;
        out_fault->far = 0;
    }

    if (context->active != 0U)
        return VAL_ERROR;

    context->faulted = 0u;
    context->esr = 0;
    context->far = 0;
    context->resume_pc = (uint64_t)(uintptr_t)&&resume;
    context->active = 1U;

    mask_serror();
    *p = value;
    dsb_sy();
    isb_sy();
    unmask_serror();
    dsb_sy();
    isb_sy();

    for (volatile uint32_t i = 0; i < 256u; i++) { /* delay for async SError delivery */
    }
    dsb_sy();
    isb_sy();

resume:
    context->active = 0u;

    if (context->faulted != 0u) {
        if (out_fault) {
            out_fault->faulted = 1u;
            out_fault->esr = context->esr;
            out_fault->far = context->far;
        }
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/**
 * @brief Arm a synchronous recovery scope for the current physical PE.
 * @param resume_pc Aligned recovery instruction within the caller's active frame.
 * @return VAL_SUCCESS, or VAL_ERROR if recovery is already armed or invalid.
 */
int val_fault_begin(uint64_t resume_pc)
{
    fault_ctx_t *context = val_fault_context();

    if ((context->active != 0U) || (resume_pc == 0U) ||
        ((resume_pc & UINT64_C(3)) != 0U))
        return VAL_ERROR;

    context->faulted = 0U;
    context->esr = 0U;
    context->far = 0U;
    context->resume_pc = resume_pc;
    context->active = 1U;
    return VAL_SUCCESS;
}

/**
 * @brief Disarm the current recovery scope after the caller restores its state.
 * @return Recorded ESR; a trapped undefined instruction can have ESR zero.
 */
uint64_t val_fault_end(void)
{
    fault_ctx_t *context = val_fault_context();

    context->active = 0U;
    context->resume_pc = 0U;
    return context->esr;
}
