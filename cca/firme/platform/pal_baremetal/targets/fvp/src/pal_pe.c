/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_pe.h"

/* Immutable affinities are available before a secondary enables its MMU. */
const uint64_t g_pal_pe_mpidrs[PAL_PE_COUNT] = {PAL_PE_MPIDRS};
