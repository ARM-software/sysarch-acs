/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef VAL_LOG_H
#define VAL_LOG_H

#include "pal_interface.h"
#include "pal_common_intf.h"

/* Verbosity enums, Lower the value, higher the verbosity */
typedef enum {
    TRACE = 1,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
} print_verbosity_t;

/**
 *   @brief    - This function prints the given string and data onto the uart
 *   @param    - verbosity  : Print Verbosity level
 *   @param    - msg        : Input String
 *   @param    - ...        : ellipses for variadic args
 *   @return   - SUCCESS((Any positive number for character written)/FAILURE(0))
**/
uint32_t val_printf(print_verbosity_t verbosity, const char *msg, ...);

void val_mem_copy(char *dest, const char *src, size_t len);

#endif /* VAL_LOG_H */

