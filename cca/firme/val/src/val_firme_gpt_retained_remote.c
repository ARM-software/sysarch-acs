/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_gpt.h"

/** @brief Reject Host coordination in images without the Host-owned observer implementation. */
__attribute__((weak)) int val_host_gpt_retained_run(firme_rule_id_t rule,
    firme_acs_world_t world, firme_rule_res_t *out)
{
    (void)rule;
    (void)world;
    if (out)
        *out = (firme_rule_res_t){.status = ACS_STATUS_ERR};

    return VAL_ERROR;
}
