/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_ide_namespace.h"

/**
 * @brief Test R0212 using distinct keyset IDs in one ECAM space.
 * @param base_pa Unused platform test address.
 * @param out Rule result.
 * @return VAL_SUCCESS on PASS/SKIP, otherwise VAL_ERROR.
 */
int firme_ide_keyset_id_uniqueness_run_realm(uint64_t base_pa,
                                             firme_rule_res_t *out)
{
    (void)base_pa;
    return val_firme_ide_namespace_run(PAL_FIRME_IDE_NAMESPACE_BY_KEYSET, out);
}

/**
 * @brief Test R0212 at an NS FIRME instance when it provides the service.
 * @param base_pa Unused platform test address.
 * @param out Rule result.
 * @return VAL_SUCCESS on PASS/SKIP, otherwise VAL_ERROR.
 */
int firme_ide_keyset_id_uniqueness_run_ns(uint64_t base_pa,
                                          firme_rule_res_t *out)
{
    (void)base_pa;
    /* Step 1: Discover, program and independently observe two real NS keysets. */
    return val_firme_ide_namespace_host_run(FIRME_ACS_WORLD_NSEL2, out);
}

/**
 * @brief Retain the endpoint session in NS while Realm invokes the FIRME ABIs.
 * @param base_pa Unused platform test address.
 * @param out Rule result including observation and cleanup failures.
 * @return VAL_SUCCESS on PASS/SKIP, otherwise VAL_ERROR.
 */
int firme_ide_keyset_id_uniqueness_run_host_realm(uint64_t base_pa,
                                                 firme_rule_res_t *out)
{
    (void)base_pa;
    /* Step 1: Discover, program and independently observe two real Realm keysets. */
    return val_firme_ide_namespace_host_run(FIRME_ACS_WORLD_REALM, out);
}
