/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_ide_observation.h"
#include "val_log.h"

/**
 * @brief Keep status-only execution from passing a rule requiring an IDE effect.
 * @param operation PROG, GO or STOP operation from the link interface.
 * @param world Caller instance used by the rule body.
 * @param out Result supplied by the host observation backend when available.
 * @return VAL_SUCCESS for an explicit coverage skip, VAL_ERROR for invalid input.
 */
__attribute__((weak)) int val_firme_ide_observe_operation(uint64_t operation,
                                                        firme_acs_world_t world,
                                                        firme_rule_res_t *out)
{
    if (!out)
        return VAL_ERROR;

    *out = (firme_rule_res_t){.status = ACS_STATUS_SKIP};
    LOG_WARN(NULL, "IDE operation=%u: world=%u has no IDE traffic observation backend; "
             "ABI status checks alone are incomplete",
             (unsigned int)operation, (unsigned int)world);
    return VAL_SUCCESS;
}

/**
 * @brief Include fresh host-observed effects in the owning rule's final result.
 * @param operation PROG, GO or STOP operation from the link interface.
 * @param world The same caller instance used by the status checks.
 * @param overall Existing rule result, replaced only after successful status checks.
 * @return VAL_ERROR for failed observation or contradictory backend results.
 */
int val_firme_ide_check_effect(uint64_t operation,
                              firme_acs_world_t world,
                              firme_acs_call_res_t *overall)
{
    firme_rule_res_t observation = {.status = ACS_STATUS_FAIL};
    int status;

    if (!overall)
        return VAL_ERROR;

    if (overall->x0 != ACS_STATUS_PASS)
        return overall->x0 == ACS_STATUS_SKIP ? VAL_SUCCESS : VAL_ERROR;

    status = val_firme_ide_observe_operation(operation, world, &observation);
    if (observation.status != ACS_STATUS_PASS && observation.status != ACS_STATUS_SKIP &&
        observation.status != ACS_STATUS_FAIL && observation.status != ACS_STATUS_ERR)
        observation.status = ACS_STATUS_ERR;

    if (status != VAL_SUCCESS &&
        (observation.status == ACS_STATUS_PASS || observation.status == ACS_STATUS_SKIP))
        observation.status = ACS_STATUS_ERR;

    overall->x0 = observation.status;
    overall->x1 = observation.fail_step;
    overall->x2 = observation.last_x0;
    overall->x3 = observation.last_x1;
    return (observation.status == ACS_STATUS_PASS || observation.status == ACS_STATUS_SKIP) ?
           VAL_SUCCESS : VAL_ERROR;
}
