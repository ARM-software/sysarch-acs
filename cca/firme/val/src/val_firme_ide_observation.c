/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_ide_observation.h"
#include "val_firme_ide.h"
#include "val_log.h"

/**
 * @brief Keep status-only execution from passing a rule requiring an IDE effect.
 * @param operation PROG, GO or STOP operation from the link interface.
 * @param world Caller instance used by the rule body.
 * @param selection Still-owned stream and the exact completed request.
 * @param out Result supplied by the host observation backend when available.
 * @return VAL_SUCCESS for an explicit coverage skip, VAL_ERROR for invalid input.
 */
__attribute__((weak))
int val_firme_ide_observe_operation(uint64_t operation,
                                    firme_acs_world_t world,
                                    const val_firme_ide_observation_t *selection,
                                    firme_rule_res_t *out)
{
    if (!out || !selection || !selection->stream || !selection->stream->acquired)
        return VAL_ERROR;

    *out = (firme_rule_res_t){.status = ACS_STATUS_SKIP};
    LOG_WARN(NULL, "Selective IDE operation=%u world=%u ECAM=%llx RP=%x EP=%x "
             "keyset=%llx handle=%llx lacks same-operation traffic evidence\n",
             (unsigned int)operation, (unsigned int)world,
             (unsigned long long)selection->stream->path.ecam_base,
             selection->stream->path.root_port_bdf, selection->stream->path.endpoint_bdf,
             (unsigned long long)selection->keyset_id, (unsigned long long)selection->handle);
    return VAL_SUCCESS;
}

/**
 * @brief Observe the exact completed operation before its stream is cleaned up.
 * @param operation PROG, GO or STOP operation from the link interface.
 * @param world The same caller instance used by the status checks.
 * @param selection Owned stream, key, keyset and handle from that invocation.
 * @param overall Existing rule result, replaced only after successful status checks.
 * @return VAL_ERROR for failed observation or contradictory backend results.
 */
int val_firme_ide_check_effect(uint64_t operation,
                              firme_acs_world_t world,
                              const val_firme_ide_observation_t *selection,
                              firme_rule_res_t *overall)
{
    firme_rule_res_t observation = {.status = ACS_STATUS_FAIL};
    val_firme_ide_keyset_fields_t fields = {0};
    int status;

    if (!overall)
        return VAL_ERROR;

    if (overall->status != ACS_STATUS_PASS)
        return overall->status == ACS_STATUS_SKIP ? VAL_SUCCESS : VAL_ERROR;

    if (!selection || !selection->stream || !selection->stream->acquired ||
        (world != FIRME_ACS_WORLD_REALM && world != FIRME_ACS_WORLD_NSEL2) ||
        (operation != VAL_IDE_LINK_PROG && operation != VAL_IDE_LINK_GO &&
         operation != VAL_IDE_LINK_STOP && operation != VAL_FIRME_IDE_OBSERVE_KEYSET_ENCODING)) {
        overall->status = ACS_STATUS_ERR;
        return VAL_ERROR;
    }

    fields.segment = selection->stream->path.segment;
    fields.root_port_id = selection->stream->path.root_port_id;
    fields.stream_id = selection->stream->stream_id;
    if (selection->keyset_id != val_firme_ide_keyset_id(&fields)) {
        overall->status = ACS_STATUS_ERR;
        return VAL_ERROR;
    }

    status = val_firme_ide_observe_operation(operation, world, selection, &observation);
    if (observation.status != ACS_STATUS_PASS && observation.status != ACS_STATUS_SKIP &&
        observation.status != ACS_STATUS_FAIL && observation.status != ACS_STATUS_ERR)
        observation.status = ACS_STATUS_ERR;

    if (status != VAL_SUCCESS &&
        (observation.status == ACS_STATUS_PASS || observation.status == ACS_STATUS_SKIP))
        observation.status = ACS_STATUS_ERR;

    *overall = observation;
    return (observation.status == ACS_STATUS_PASS || observation.status == ACS_STATUS_SKIP) ?
           VAL_SUCCESS : VAL_ERROR;
}
