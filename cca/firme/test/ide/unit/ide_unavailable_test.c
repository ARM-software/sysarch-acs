/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>

#include "pal_firme_ide_abi.h"
#include "pal_firme_ide_namespace.h"
#include "val_common/val_status.h"

/** @brief Verify that absent physical observers cannot return synthetic coverage. */
int main(void)
{
    pal_firme_ide_abi_scenario_t abi = {0};
    pal_firme_ide_namespace_scenario_t namespace = {0};

    assert(pal_firme_ide_abi_prepare(0U, PAL_FIRME_IDE_ABI_GO,
        PAL_FIRME_IDE_ABI_VALID, &abi) == PAL_FIRME_IDE_ABI_UNAVAILABLE);
    assert(pal_firme_ide_abi_ready(0U, PAL_FIRME_IDE_ABI_GO,
        PAL_FIRME_IDE_ABI_VALID, &abi) == PAL_FIRME_IDE_ABI_UNAVAILABLE);
    assert(pal_firme_ide_abi_observe(0U, PAL_FIRME_IDE_ABI_GO,
        PAL_FIRME_IDE_ABI_VALID, &abi) == PAL_FIRME_IDE_ABI_UNAVAILABLE);
    assert(pal_firme_ide_abi_quiesce(0U, &abi) == PAL_FIRME_IDE_ABI_UNAVAILABLE);
    assert(pal_firme_ide_abi_cleanup(0U, PAL_FIRME_IDE_ABI_GO,
        PAL_FIRME_IDE_ABI_VALID, &abi) == PAL_FIRME_IDE_ABI_UNAVAILABLE);
    assert(pal_firme_ide_namespace_prepare(PAL_FIRME_IDE_NAMESPACE_BY_KEYSET,
        &namespace) == PAL_FIRME_IDE_NAMESPACE_UNAVAILABLE);
    assert(pal_firme_ide_namespace_observe(PAL_FIRME_IDE_NAMESPACE_BY_KEYSET,
        &namespace, VAL_TRUE, VAL_TRUE) == PAL_FIRME_IDE_NAMESPACE_UNAVAILABLE);
    assert(pal_firme_ide_namespace_cleanup(PAL_FIRME_IDE_NAMESPACE_BY_KEYSET,
        &namespace) == PAL_FIRME_IDE_NAMESPACE_UNAVAILABLE);
    puts("Unbound IDE ABI and independent namespace observers: correctly unavailable");
    return VAL_SUCCESS;
}
