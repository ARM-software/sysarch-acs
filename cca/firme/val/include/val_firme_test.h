/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#include "val_firme_abi.h"
#include "val_firme_gpt_defs.h"

/* Feature discovery helpers. */
uint32_t val_firme_feat_rme_level(void);
uint32_t val_firme_feat_rme_present(void);
uint32_t val_firme_feat_rme_gpc2_present(void);
uint32_t val_firme_feat_rme_gdi_present(void);

/* FIRME_GM_GPI_SET helpers used by GPT rule tests. */
void val_firme_gm_gpi_set_do(uint64_t base_pa, uint64_t granule_count, uint64_t target_gpi,
                             firme_res_t *out);
int val_firme_gm_gpi_set_complete(uint64_t base_pa, uint64_t granule_count,
                                    uint64_t target_gpi, firme_res_t *out);
