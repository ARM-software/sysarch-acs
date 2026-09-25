/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

/* FVP default MPIDR.MT=1: one thread/core, four cores/cluster, two clusters. */
#ifndef PAL_PE_COUNT
#define PAL_PE_COUNT 8
#endif
#ifndef PAL_PE_MPIDRS
#define PAL_PE_MPIDRS 0x0, 0x100, 0x200, 0x300, 0x10000, 0x10100, 0x10200, 0x10300
#endif
#if PAL_PE_COUNT < 1
#error "PAL_PE_COUNT must describe at least the boot PE"
#endif
#define PAL_PE_AFFINITY_MASK 0xff00ffffff

#ifndef __ASSEMBLER__
#include <stdint.h>

extern const uint64_t g_pal_pe_mpidrs[PAL_PE_COUNT];
#endif
