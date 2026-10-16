/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt_retained.h"

/**
 * @brief R0114: Validate the resumed start through independently observed granule progress.
 * @param base_pa Dispatcher address; the fixture selects exclusively reserved storage.
 * @param out Normal-world verdict and first-failure diagnostics.
 * @return VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_op_continue_start_granule_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;

    return val_firme_gpt_retained_run(FIRME_RULE_R0114, FIRME_ACS_WORLD_NSEL2, out);
}

/**
 * @brief R0114: Validate the resumed start through independently observed granule progress.
 * @param base_pa Dispatcher address; the fixture selects exclusively reserved storage.
 * @param out Secure-world verdict and first-failure diagnostics.
 * @return VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_op_continue_start_granule_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;

    return val_firme_gpt_retained_run(FIRME_RULE_R0114, FIRME_ACS_WORLD_SECURE, out);
}

/**
 * @brief R0114: Validate the resumed start through independently observed granule progress.
 * @param base_pa Dispatcher address; the fixture selects exclusively reserved storage.
 * @param out Realm-world verdict and first-failure diagnostics.
 * @return VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_op_continue_start_granule_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;

    return val_firme_gpt_retained_run(FIRME_RULE_R0114, FIRME_ACS_WORLD_REALM, out);
}
