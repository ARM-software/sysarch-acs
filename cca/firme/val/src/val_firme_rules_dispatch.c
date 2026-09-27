/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_rules.h"

#include "val_log.h"
#include "val_firme_suite.h"

/**
 * @brief Execute a rule body without accepting an unwritten or contradictory result.
 * @param body Registered caller-specific body, or NULL when unavailable.
 * @param base_pa Test-owned physical address.
 * @param out Required result record.
 * @return VAL_SUCCESS for PASS/SKIP, VAL_ERROR for FAIL or framework errors.
 */
static int val_firme_rule_execute(firme_rule_fn_t body, uint64_t base_pa,
                                 firme_rule_res_t *out)
{
    int result;

    if (!out)
        return VAL_ERROR;

    *out = (firme_rule_res_t){.status = ACS_STATUS_ERR};
    if (!body) {
        out->status = ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }

    result = body(base_pa, out);
    if (out->status != ACS_STATUS_PASS && out->status != ACS_STATUS_SKIP &&
        out->status != ACS_STATUS_FAIL && out->status != ACS_STATUS_ERR)
        out->status = ACS_STATUS_ERR;

    if (result != VAL_SUCCESS &&
        (out->status == ACS_STATUS_PASS || out->status == ACS_STATUS_SKIP))
        out->status = ACS_STATUS_ERR;

    return (out->status == ACS_STATUS_PASS || out->status == ACS_STATUS_SKIP) ?
           VAL_SUCCESS : VAL_ERROR;
}

/**
 *   @brief    - Dispatch a rule test implementation for Secure world execution.
 *   @param    - rule    : Rule ID.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Required result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
**/
int val_firme_rule_run_secure(firme_rule_id_t rule, uint64_t base_pa, firme_rule_res_t *out)
{
    const firme_test_case_t *tc = val_firme_suite_find(rule);

    return val_firme_rule_execute(tc ? tc->run_secure : NULL, base_pa, out);
}

/**
 *   @brief    - Dispatch a rule test implementation for Realm world execution.
 *   @param    - rule    : Rule ID.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Required result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
**/
int val_firme_rule_run_realm(firme_rule_id_t rule, uint64_t base_pa, firme_rule_res_t *out)
{
    const firme_test_case_t *tc = val_firme_suite_find(rule);

    return val_firme_rule_execute(tc ? tc->run_realm : NULL, base_pa, out);
}

/**
 *   @brief    - Dispatch a rule test implementation for NS world execution.
 *   @param    - rule    : Rule ID.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Required result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int val_firme_rule_run_ns(firme_rule_id_t rule, uint64_t base_pa, firme_rule_res_t *out)
{
    const firme_test_case_t *tc = val_firme_suite_find(rule);

    LOG_DEBUG(NULL, "dispatch ns rule=%u base=0x%llx",
              (unsigned int)rule, (unsigned long long)base_pa);
    return val_firme_rule_execute(tc ? tc->run_ns : NULL, base_pa, out);
}
