/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#include "val_firme_abi.h"
#include "val_common/val_status.h"

#define VAL_FIRME_MECID_WIDTH_MIN       1U
#define VAL_FIRME_MECID_WIDTH_MAX       16U
#define VAL_FIRME_MECID_U64_SHIFT_MAX       63U
#define VAL_FIRME_MECIDR_WIDTH_SHIFT       0U
#define VAL_FIRME_MECIDR_WIDTH_MASK        0xfU
#define VAL_FIRME_MEC_REFRESH_RETRY_LIMIT  16U

/**
 *   @brief    - Read the common MECID width advertised by the DUT.
 *   @param    - out_width : Returned advertised common MECID width in bits.
 *   @param    - out_res   : Raw SERVICE_FEATURES result (optional).
 *   @return   - 0 on success, non-zero otherwise.
 **/
int val_firme_mec_advertised_common_mecid_width(uint32_t *out_width, firme_res_t *out_res);

/**
 *   @brief    - Read this PE's MECID width, not the system-wide common width.
 *   @param    - out_width : Returned local PE MECID width in bits.
 *   @return   - 0 on success, non-zero otherwise.
 **/
int val_firme_mec_local_pe_mecid_width(uint32_t *out_width);

/**
 *   @brief    - Encode FIRME_MEC_REFRESH MEC params.
 *   @param    - mecid  : MECID value.
 *   @param    - reason : MEC refresh reason.
 *   @return   - Encoded MEC params value.
 **/
uint64_t val_firme_mec_params(uint64_t mecid, uint64_t reason);

/**
 *   @brief    - Invoke FIRME_MEC_REFRESH and retry while BUSY is reported.
 *   @param    - mec_params : Encoded MEC params input value.
 *   @param    - out        : Returned FIRME result.
 *   @return   - void
 **/
void val_firme_mec_refresh_do_retry(uint64_t mec_params, firme_res_t *out);
