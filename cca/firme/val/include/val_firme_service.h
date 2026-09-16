/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#include "val_firme_abi.h"
#include "val_firme_dispatch.h"

int val_firme_service_get_version(uint32_t service_id, uint32_t *implemented,
                                  firme_res_t *version_res);
int val_firme_service_get_feature_reg0(uint32_t service_id, uint32_t *advertised,
                                       uint64_t *feature_reg0, firme_res_t *features_res);
int val_firme_service_abi_is_advertised(uint32_t service_id, uint64_t abi_mask,
                                        uint32_t *advertised, uint64_t *feature_reg0,
                                        firme_res_t *features_res);
uint32_t val_firme_service_is_allowed_at_world(firme_acs_world_t world, uint32_t service_id);
uint64_t val_firme_service_allocated_abi_mask(uint32_t service_id);
uint64_t val_firme_service_mandatory_abi_mask(firme_acs_world_t world, uint32_t service_id);
uint64_t val_firme_service_optional_abi_mask(firme_acs_world_t world, uint32_t service_id);
uint64_t val_firme_service_supported_abi_mask(firme_acs_world_t world, uint32_t service_id);
