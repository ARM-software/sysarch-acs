/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi_generated.h"
#include "val_common/val_status.h"

/**
 * @brief Reject host coordination from payloads without a host fixture owner.
 * @param plan_id Generated plan selected by the registry.
 * @param world Requested remote caller.
 * @param base_pa Platform-owned test address.
 * @param out Explicit error result; no local-world fallback is permitted.
 * @return VAL_ERROR; the host payload supplies the coordinating implementation.
 */
__attribute__((weak)) int val_firme_abi_generated_remote_execute(
    val_firme_abi_test_plan_id_t plan_id, firme_acs_world_t world,
    uint64_t base_pa, firme_rule_res_t *out)
{
    (void)plan_id;
    (void)world;
    (void)base_pa;

    if (out)
        *out = (firme_rule_res_t){.status = ACS_STATUS_ERR};
    return VAL_ERROR;
}
