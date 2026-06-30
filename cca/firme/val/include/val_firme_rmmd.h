/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

/*
 * Minimal constants for interacting with TF-A RMMD forwarding plumbing.
 *
 * TF-A forwards RMI SMCs (0xC4000150-0xC400018F, 0xC40001D0-0xC40002CF) from
 * the Normal world to the Realm world (R-EL2 payload). The payload returns to the Normal world
 * by issuing RMM_RMI_REQ_COMPLETE, which RMMD uses to forward up to 5 return
 * values back to the Normal world.
 */

/* RMMD EL3 interface: RMM -> EL3. */
#define RMM_BOOT_COMPLETE 0xC40001CFU

/* RMI return path: RMM -> EL3 -> Normal world. */
#define RMM_RMI_REQ_COMPLETE 0xC400018FU

/*
 * Private dispatch SMC in the RMI "FNUM1" range (0x1D0..0x2CF).
 * - Normal world issues this SMC; TF-A forwards it to the Realm payload (R-EL2).
 * - Realm payload returns results via RMM_RMI_REQ_COMPLETE.
 *
 * Args:
 *   x1=cmd, x2..x5=payload args
 *
 * Return to Normal world (after RMMD forwarding):
 *   x0=RMI_SUCCESS, x1=RMI_SUCCESS, x2=ACS_STATUS_*, x3..x4=payload results
 */
#define FIRME_ACS_RMI_DISPATCH 0xC40001D0U

/* SMCCC "unknown" return value. */
#define SMCCC_SMC_UNK 0xFFFFFFFFU
