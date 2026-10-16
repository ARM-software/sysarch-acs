/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include "pal_firme_ide_namespace.h"
#include "val_firme_rules.h"
#include "val_firme_dispatch.h"

/**
 * @brief Exercise two IDE paths and require independent Root Port evidence.
 * @param kind Compare equal IDs across ECAMs or distinct IDs within one ECAM.
 * @param out Rule result.
 * @return VAL_SUCCESS on PASS/SKIP, otherwise VAL_ERROR.
 */
int val_firme_ide_namespace_run(pal_firme_ide_namespace_kind_t kind,
                                firme_rule_res_t *out);

/**
 * @brief Observe two distinct keysets with the endpoint session retained by the host.
 * @param world NS or Realm instance performing the Root Port FIRME operations.
 * @param out Rule result including setup, observation and cleanup failures.
 * @return VAL_SUCCESS on PASS/SKIP, otherwise VAL_ERROR.
 */
int val_firme_ide_namespace_host_run(firme_acs_world_t world, firme_rule_res_t *out);
