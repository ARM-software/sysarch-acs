/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#include "val_firme_dispatch.h"

uint32_t val_firme_service_is_allowed_at_world(firme_acs_world_t world, uint32_t service_id);
