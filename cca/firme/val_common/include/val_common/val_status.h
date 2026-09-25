/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#ifndef __ASSEMBLER__
#include <stdint.h>
#endif

/*
 * ACS-style status codes (mirrors sysarch-acs conventions).
 * Keep these stable; they may be consumed by automation/log parsing.
 */
#define ACS_STATUS_PASS    UINT32_C(0x0)
#define ACS_STATUS_FAIL    UINT32_C(0x90000000)
#define ACS_STATUS_SKIP    UINT32_C(0x10000000)
#define ACS_STATUS_ERR     UINT32_C(0xEDCB1234)
#define ACS_STATUS_UNKNOWN UINT32_C(0xFFFFFFFF)

/* Common return codes for internal helper functions. */
#define VAL_SUCCESS 0
#define VAL_ERROR   (-1)
#define VAL_TRUE    1
#define VAL_FALSE   0
