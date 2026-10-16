/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include "val_firme_dispatch.h"

/**
 * @brief Probe one PAL-reserved PA in the current world's native PAS.
 * @param address Naturally aligned address in a declared disposable RAM extent.
 * @param response PASS plus fault flag/value-or-ESR, or ERR for unsafe setup.
 */
void val_firme_gpt_probe_command(uint64_t address, firme_acs_call_res_t *response);
