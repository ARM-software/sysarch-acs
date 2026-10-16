/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifndef FIRME_REALM_PAT_DELEGATED_LEGACY
#define FIRME_REALM_PAT_DELEGATED_LEGACY 0
#endif

#define VAL_REALM_PAT_BINDING_SIZE 32U

/**
 * @brief Establish a Delegated-model binding while holding the boot-page lease lock.
 * @param buffer Exclusively owned, coherent, identity-mapped boot shared page.
 * @param size Backed page size; the legacy setup interface requires 4096 bytes.
 * @param pending Set when an unexpected response cannot establish terminal completion.
 * @return Shared-buffer probe status, never FIRME rule-coverage evidence.
 * The build must explicitly establish the Delegated model and availability of
 * the legacy private P-384 RAK setup interface. No FIRME feature bit selects it.
 */
int val_realm_pat_binding_prepare(uint8_t *buffer, uint64_t size, uint32_t *pending);

/** @brief Forget a terminal lease's binding while holding the boot-page lease lock. */
void val_realm_pat_binding_clear(void);

/** @brief Return the last setup status for prerequisite diagnostics, not rule evidence. */
uint64_t val_realm_pat_binding_status(void);

/** @brief Report whether the current PE implements architectural RNDR. */
uint32_t val_realm_pat_has_rndr(void);

/** @brief Obtain one RNDR value; return VAL_TRUE only on architectural success. */
uint32_t val_realm_pat_read_rndr(uint64_t *value);

/** @brief Stop on internal allocator corruption rather than continue with unsafe memory. */
_Noreturn void val_realm_pat_crypto_abort(int status);
