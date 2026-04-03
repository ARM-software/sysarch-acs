/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef VAL_ARCH_H
#define VAL_ARCH_H

/* CPSR/SPSR definitions */
#define DAIF_FIQ_BIT        (1U << 0)
#define DAIF_IRQ_BIT        (1U << 1)
#define DAIF_ABT_BIT        (1U << 2)
#define DAIF_DBG_BIT        (1U << 3)
#define DAIF_CONFIG         (0x7U)

#define PMSELR_EL0_SEL_MASK (0x1fU)

#define EL_IMPL_NONE        (0ULL)
#define MODE_EL_SHIFT       (0x2U)
#define MODE_EL_MASK        (0x3U)
#define GET_EL(mode)        (((mode) >> MODE_EL_SHIFT) & MODE_EL_MASK)

#define ID_AA64PFR0_EL1_SHIFT (4U)
#define ID_AA64PFR0_ELX_MASK  (0xfULL)

#endif /* VAL_ARCH_H */
