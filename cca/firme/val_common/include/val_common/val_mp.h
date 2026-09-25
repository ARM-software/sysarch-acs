/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include "pal_pe.h"

#define VAL_PE_STACK_SIZE 32768

#ifndef __ASSEMBLER__
#include <stddef.h>
#include <stdint.h>

uint32_t val_pe_index(void);
uint64_t val_read_counter(void);
uint64_t val_read_counter_frequency(void);
void val_mp_relax(void);
void val_cache_clean_poc(uintptr_t start, size_t size);
#endif
