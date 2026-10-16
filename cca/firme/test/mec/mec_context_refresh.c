/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "mec_semantic_test_common.h"

#include "val_common/val_fault.h"
#include "val_firme_abi.h"


/**
 *   @brief    - Execute R0236 from the Realm FIRME instance.
 *   @param    - base_pa : Unused; the Realm payload owns a reserved test page.
 *   @param    - out     : Result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_mec_context_refresh_run_realm(
    uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return firme_mec_semantic_test_context_refresh(
        VAL_FIRME_MEC_OBSERVE_CONTEXT_REFRESH, out);
}
