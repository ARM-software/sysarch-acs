/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>
#include "val_common/val_status.h"

typedef struct {
    uint64_t faulted;
    uint64_t esr;
    uint64_t far;
} val_fault_info_t;

/*
 * Armv8 ESR.EC values.
 * - 0x25: Data Abort taken without change in Exception level.
 * - 0x24: Data Abort from a lower EL.
 * - 0x21: Instruction Abort taken without change in Exception level.
 */
/* Extract the ESR exception class (EC). */
#define val_esr_ec(esr) ((uint32_t)(((uint64_t)(esr) >> 26) & 0x3fu))

/* Check whether the ESR encodes an abort-class exception. */
#define val_fault_is_abort(esr) \
    ((val_esr_ec(esr) == 0x25u) || (val_esr_ec(esr) == 0x24u) || (val_esr_ec(esr) == 0x21u))

/* Check whether the ESR encodes an SError exception. */
#define val_fault_is_serror(esr) (val_esr_ec(esr) == 0x2fu)

/*
 * Granule Protection Fault (GPF) encoding:
 * - Data Abort: EC = 0x25 (cur) or 0x24 (lower)
 * - DFSC in ISS[5:0] = 0x28 (DFSC_GPF_DABORT)
 */
/* Check whether the ESR encodes a Granule Protection Fault (GPF). */
#define val_fault_is_gpf(esr) \
    (((val_esr_ec(esr) == 0x25u) || (val_esr_ec(esr) == 0x24u)) && \
     (((uint32_t)(esr) & 0x3fu) == 0x28u))

/*
 * Handle synchronous exceptions that occur during a probe.
 *
 * Returns VAL_TRUE if the exception was handled (ELR is advanced to a recovery PC),
 * otherwise VAL_FALSE.
 */
int val_fault_handle_sync(uint64_t esr, uint64_t far);

/**
 * @brief Arm scoped synchronous-exception recovery for the current PE.
 * @param resume_pc Aligned instruction address to resume at after a fault.
 * @return VAL_SUCCESS when armed, otherwise VAL_ERROR.
 */
int val_fault_begin(uint64_t resume_pc);

/**
 * @brief Disarm scoped exception recovery and return the captured ESR.
 * @return ESR recorded by the scope, or zero when no exception occurred.
 */
uint64_t val_fault_end(void);

/*
 * Best-effort memory probe: attempt a 64-bit load from `addr`.
 * - Returns VAL_SUCCESS if the access completed (no exception).
 * - Returns VAL_ERROR if a synchronous exception was raised (fault info filled).
 */
int val_probe_read64(uint64_t addr, uint64_t *out_value, val_fault_info_t *out_fault);

/*
 * Best-effort memory probe: attempt a 64-bit store to `addr`.
 * - Returns VAL_SUCCESS if the access completed (no exception).
 * - Returns VAL_ERROR if a synchronous exception was raised (fault info filled).
 */
int val_probe_write64(uint64_t addr, uint64_t value, val_fault_info_t *out_fault);
