/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"

#include "pal_platform.h"
#include "val_common/val_status.h"
#include "val_log.h"

#define GPT_PAR_STATE_ENCODING_TEST_NAME "gpt_par_state_encoding"

#define GPT_PAR_STATE_ENCODING_STEP_ADVERTISEMENT UINT64_C(1)
#define GPT_PAR_STATE_ENCODING_STEP_WINDOW        UINT64_C(2)
#define GPT_PAR_STATE_ENCODING_STEP_QUERY         UINT64_C(3)
#define GPT_PAR_STATE_ENCODING_STEP_EXPECTED      UINT64_C(4)

/**
 * @brief Validate the PAR-state encoding returned for the known test range.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when an architectural result is produced.
 */
static int gpt_par_state_encoding_run(uint64_t base_pa,
                                      firme_rule_res_t *out)
{
    val_firme_gpt_info_window_t window;
    firme_res_t result = {0};
    uint64_t expected_state = PLAT_FIRME_GPT_INFO_PAR_STATE;

    if (!out)
        return VAL_ERROR;

    out->status = (uint64_t)ACS_STATUS_FAIL;
    out->fail_step = 0ULL;
    out->last_x0 = 0ULL;
    out->last_x1 = 0ULL;

    /* Step 1: require the mandatory GPT_INFO discovery ABI. */
    if (val_firme_gpt_info_advertised(
            out, GPT_PAR_STATE_ENCODING_STEP_ADVERTISEMENT) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 2: identify the complete protected range containing the test PA. */
    if (val_firme_gpt_info_prepare_window(
            base_pa, &window, out,
            GPT_PAR_STATE_ENCODING_STEP_WINDOW) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 3: query exactly one range and validate allocated output fields. */
    val_firme_gm_gpt_info(window.range_base, window.range_top, &result);
    if (val_firme_gpt_info_success_valid(
            &window, &result, out,
            GPT_PAR_STATE_ENCODING_STEP_QUERY) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 4: compare the encoding with independent platform GPT metadata. */
    if (result.x2 != expected_state) {
        LOG_ERROR(NULL,
                  "%s: state=%llu expected=%llu base=%llx",
                  GPT_PAR_STATE_ENCODING_TEST_NAME,
                  (unsigned long long)result.x2,
                  (unsigned long long)expected_state,
                  (unsigned long long)window.range_base);
        out->fail_step = GPT_PAR_STATE_ENCODING_STEP_EXPECTED;
        out->last_x0 = result.x2;
        out->last_x1 = expected_state;
        return VAL_SUCCESS;
    }

    out->status = (uint64_t)ACS_STATUS_PASS;
    return VAL_SUCCESS;
}

/**
 * @brief Validate R0138 from the Normal-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when an architectural result is produced.
 */
int firme_gpt_par_state_encoding_run_ns(uint64_t base_pa,
                                        firme_rule_res_t *out)
{
    return gpt_par_state_encoding_run(base_pa, out);
}

/**
 * @brief Validate R0138 from the Secure-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when an architectural result is produced.
 */
int firme_gpt_par_state_encoding_run_secure(uint64_t base_pa,
                                            firme_rule_res_t *out)
{
    return gpt_par_state_encoding_run(base_pa, out);
}

/**
 * @brief Validate R0138 from the Realm-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when an architectural result is produced.
 */
int firme_gpt_par_state_encoding_run_realm(uint64_t base_pa,
                                           firme_rule_res_t *out)
{
    return gpt_par_state_encoding_run(base_pa, out);
}
