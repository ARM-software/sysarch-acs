/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include "val_firme_abi_test.h"

/**
 * @brief Return the service-specific operations for an Alpha1 ABI.
 * @param abi_id Generated Alpha1 ABI identifier.
 * @return Adapter operations, or NULL when the ABI has not been enabled.
 */
const val_firme_abi_test_ops_t *val_firme_abi_adapter_get(
    val_firme_abi_id_t abi_id);

/**
 * @brief Return the stateful adapter for an IDE Key Management ABI.
 * @param abi_id Generated Alpha1 ABI identifier.
 * @return IDE adapter operations, or NULL for another service.
 */
const val_firme_abi_test_ops_t *val_firme_ide_abi_adapter_get(
    val_firme_abi_id_t abi_id);
