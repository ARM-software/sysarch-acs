/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt_l1.h"

/**
 * @brief Validate R0179 from the Normal-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when the shared strict algorithm produces a result.
 */
int firme_gpt_l1_destroy_transition_policy_run_ns(
    uint64_t base_pa, firme_rule_res_t *out)
{
    /* Execute the complete R0179 setup, DUT check, and cleanup sequence. */
    return val_firme_gpt_l1_rule_run(FIRME_RULE_R0179,
                                      FIRME_ACS_WORLD_NSEL2,
                                      base_pa, out);
}
/**
 * @brief Validate R0179 from the Secure-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when the shared strict algorithm produces a result.
 */
int firme_gpt_l1_destroy_transition_policy_run_secure(
    uint64_t base_pa, firme_rule_res_t *out)
{
    /* Execute the complete R0179 setup, DUT check, and cleanup sequence. */
    return val_firme_gpt_l1_rule_run(FIRME_RULE_R0179,
                                      FIRME_ACS_WORLD_SECURE,
                                      base_pa, out);
}

/**
 * @brief Validate R0179 from the Realm-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when the shared strict algorithm produces a result.
 */
int firme_gpt_l1_destroy_transition_policy_run_realm(
    uint64_t base_pa, firme_rule_res_t *out)
{
    /* Execute the complete R0179 setup, DUT check, and cleanup sequence. */
    return val_firme_gpt_l1_rule_run(FIRME_RULE_R0179,
                                      FIRME_ACS_WORLD_REALM,
                                      base_pa, out);
}
