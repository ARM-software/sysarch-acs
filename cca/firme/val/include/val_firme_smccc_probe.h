/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#define VAL_SMCCC_VECTOR_BYTES 0x12240
#define VAL_SMCCC_PREDICATE_OFFSET 0x2000
#define VAL_SMCCC_FFR_OFFSET 0x2200
#define VAL_SMCCC_FPCR_OFFSET 0x2220
#define VAL_SMCCC_FPSR_OFFSET 0x2228
#define VAL_SMCCC_ZA_OFFSET 0x2240
#define VAL_SMCCC_STATE_SVE 1
#define VAL_SMCCC_STATE_FFR 2
#define VAL_SMCCC_STATE_ZA 4
#define VAL_SMCCC_PROBE_STREAMING 8
#define VAL_SMCCC_PROBE_HINT 16

#ifndef __ASSEMBLER__
#include "val_firme_rules.h"

/**
 * @brief Check register and vector state across every advertised FIRME ABI.
 * @param modes State and SMCCC hint selection.
 * @param out Optional result record populated on failure.
 * @return VAL_SUCCESS when state is compliant, otherwise VAL_ERROR.
 */
int val_firme_smccc_check_state(uint64_t modes, firme_rule_res_t *out);

/**
 * @brief Check register and vector state for one advertised FIRME service.
 * @param service_id Service whose advertised ABIs are exercised.
 * @param modes State and SMCCC hint selection.
 * @param out Optional result record populated on failure.
 * @return VAL_SUCCESS when state is compliant, otherwise VAL_ERROR.
 */
int val_firme_smccc_check_service(uint32_t service_id, uint64_t modes,
                                  firme_rule_res_t *out);

/**
 * @brief Check that every ALP3 FIRME ABI uses an SMC64 function identifier.
 * @param out Optional result record populated on failure.
 * @return VAL_SUCCESS when every identifier is SMC64, otherwise VAL_ERROR.
 */
int val_firme_smccc_check_fid_encodings(firme_rule_res_t *out);
#endif
