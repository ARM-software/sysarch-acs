/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"
#include "val_firme_rules.h"
#include "val_log.h"

#define GPT_TRANS_FEAT_RME_FAILSTEP_PROBE_WRITE UINT64_C(20)
#define GPT_TRANS_FEAT_RME_FAILSTEP_PROBE_READ  UINT64_C(21)
#define GPT_TRANS_FEAT_RME_FAILSTEP_PROBE_VALUE UINT64_C(22)

#define GPT_TRANS_FEAT_RME_TEST_NAME "gpt_transition_feat_rme"

/**
 *   @brief    - GPT transition policy (FEAT_RME): Secure-world caller coverage.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Optional result record.
 *   @return   - 0 (status stored in `out->status` when provided).
**/
int firme_gpt_transition_feat_rme_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    firme_res_t res = (firme_res_t){0};
    const uint64_t granule_count = 1ULL;
    const uint64_t expected_done = granule_count;

    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }

    if (!val_firme_feat_rme_present() || base_pa == 0ULL) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return 0;
    }

    if (val_firme_gpt_gpi_set_advertised(out, UINT64_C(1000)) == VAL_FALSE)
        return 0;

    LOG_DEBUG(NULL, GPT_TRANS_FEAT_RME_TEST_NAME ": secure start base=%llx",
              (unsigned long long)base_pa);

    /* Step 1: force known state (Non-secure). */
    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NONSECURE, &res);
    LOG_DEBUG(NULL, GPT_TRANS_FEAT_RME_TEST_NAME ": step1 set->NS x0=%llx x1=%llx",
              (unsigned long long)res.x0, (unsigned long long)res.x1);
    if (out) {
        out->last_x0 = res.x0;
        out->last_x1 = res.x1;
    }

    /* Step 2: Secure caller: Non-secure -> Secure (permitted). */
    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_SECURE, &res);
    LOG_DEBUG(NULL, GPT_TRANS_FEAT_RME_TEST_NAME ": step2 NS->S x0=%llx x1=%llx",
              (unsigned long long)res.x0, (unsigned long long)res.x1);
    if (!val_firme_status_success(res.x0, res.x1, expected_done)) {
        LOG_ERROR(NULL, "%s: secure step2 NS->S failed x0=%llx x1=%llx",
                  GPT_TRANS_FEAT_RME_TEST_NAME,
                  (unsigned long long)res.x0, (unsigned long long)res.x1);
        if (out) {
            out->fail_step = 2;
            out->last_x0 = res.x0;
            out->last_x1 = res.x1;
        }
        return 0;
    }

    /* Validate the transition took effect by accessing the granule from Secure. */
    {
        val_fault_info_t fault_info_write = {0};
        val_fault_info_t fault_info_read = {0};
        uint64_t value = 0;
        const uint64_t test_data = FIRME_TEST_DATA_PATTERN;

        if (val_probe_write64(base_pa, test_data, &fault_info_write) != 0) {
            LOG_ERROR(NULL, "%s: secure probe write failed esr=%llx far=%llx",
                      GPT_TRANS_FEAT_RME_TEST_NAME,
                      (unsigned long long)fault_info_write.esr,
                      (unsigned long long)fault_info_write.far);
            if (out) {
                out->fail_step = GPT_TRANS_FEAT_RME_FAILSTEP_PROBE_WRITE;
                out->last_x0 = fault_info_write.esr;
                out->last_x1 = fault_info_write.far;
            }
            return 0;
        }
        if (val_probe_read64(base_pa, &value, &fault_info_read) != 0) {
            LOG_ERROR(NULL, "%s: secure probe read fault esr=%llx far=%llx",
                      GPT_TRANS_FEAT_RME_TEST_NAME,
                      (unsigned long long)fault_info_read.esr,
                      (unsigned long long)fault_info_read.far);
            if (out) {
                out->fail_step = GPT_TRANS_FEAT_RME_FAILSTEP_PROBE_READ;
                out->last_x0 = fault_info_read.esr;
                out->last_x1 = fault_info_read.far;
            }
            return 0;
        }
        if (value != test_data) {
            LOG_ERROR(NULL, "%s: secure probe read mismatch v=%llx exp=%llx",
                      GPT_TRANS_FEAT_RME_TEST_NAME,
                      (unsigned long long)value,
                      (unsigned long long)test_data);
            if (out) {
                out->fail_step = GPT_TRANS_FEAT_RME_FAILSTEP_PROBE_VALUE;
                out->last_x0 = value;
                out->last_x1 = test_data;
            }
            return 0;
        }
    }

    /* Step 3: Secure caller: Secure -> Non-secure (permitted). */
    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NONSECURE, &res);
    LOG_DEBUG(NULL, GPT_TRANS_FEAT_RME_TEST_NAME ": step3 S->NS x0=%llx x1=%llx",
              (unsigned long long)res.x0, (unsigned long long)res.x1);
    if (!val_firme_status_success(res.x0, res.x1, expected_done)) {
        LOG_ERROR(NULL, GPT_TRANS_FEAT_RME_TEST_NAME ": secure step3 S->NS failed x0=%llx x1=%llx",
                  (unsigned long long)res.x0, (unsigned long long)res.x1);
        if (out) {
            out->fail_step = 3;
            out->last_x0 = res.x0;
            out->last_x1 = res.x1;
        }
        return 0;
    }

    /* Step 4: Disallowed: Secure -> Realm. */
    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_SECURE, &res);
    if (val_firme_status_success(res.x0, res.x1, expected_done)) {
        val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_REALM, &res);
        LOG_DEBUG(NULL, GPT_TRANS_FEAT_RME_TEST_NAME ": step4 S->R x0=%llx x1=%llx",
                  (unsigned long long)res.x0, (unsigned long long)res.x1);
        if (!val_firme_status_deniedish(res.x0) && !val_firme_status_not_supported(res.x0)) {
            LOG_ERROR(NULL, "%s: secure step4 S->R expected DENIED/NSUP x0=%llx x1=%llx",
                      GPT_TRANS_FEAT_RME_TEST_NAME,
                      (unsigned long long)res.x0,
                      (unsigned long long)res.x1);
            if (out) {
                out->fail_step = 4;
                out->last_x0 = res.x0;
                out->last_x1 = res.x1;
            }
            return 0;
        }
    }

    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NONSECURE, &res);
    LOG_DEBUG(NULL, GPT_TRANS_FEAT_RME_TEST_NAME ": secure done");
    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;
    return 0;
}

/**
 *   @brief    - GPT transition policy (FEAT_RME): Realm-world caller coverage.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Optional result record.
 *   @return   - 0 (status stored in `out->status` when provided).
**/
int firme_gpt_transition_feat_rme_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    firme_res_t res = (firme_res_t){0};
    const uint64_t granule_count = 1ULL;
    const uint64_t expected_done = granule_count;

    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }

    if (!val_firme_feat_rme_present() || base_pa == 0ULL) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return 0;
    }

    if (val_firme_gpt_gpi_set_advertised(out, UINT64_C(1000)) == VAL_FALSE)
        return 0;

    /* Step 1: force known state (Non-secure). */
    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NONSECURE, &res);

    /* Step 2: Realm caller: Non-secure -> Realm (permitted). */
    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_REALM, &res);
    if (!val_firme_status_success(res.x0, res.x1, expected_done)) {
        LOG_ERROR(NULL, GPT_TRANS_FEAT_RME_TEST_NAME ": realm step2 NS->R failed x0=%llx x1=%llx",
                  (unsigned long long)res.x0, (unsigned long long)res.x1);
        if (out) {
            out->fail_step = 2;
            out->last_x0 = res.x0;
            out->last_x1 = res.x1;
        }
        return 0;
    }

    /* Validate the transition took effect by accessing the granule from Realm. */
    {
        val_fault_info_t fault_info_write = {0};
        val_fault_info_t fault_info_read = {0};
        uint64_t value = 0;
        const uint64_t test_data = FIRME_TEST_DATA_PATTERN;

        if (val_probe_write64(base_pa, test_data, &fault_info_write) != 0) {
            LOG_ERROR(NULL, "%s: realm probe write failed esr=%llx far=%llx",
                      GPT_TRANS_FEAT_RME_TEST_NAME,
                      (unsigned long long)fault_info_write.esr,
                      (unsigned long long)fault_info_write.far);
            if (out) {
                out->fail_step = GPT_TRANS_FEAT_RME_FAILSTEP_PROBE_WRITE;
                out->last_x0 = fault_info_write.esr;
                out->last_x1 = fault_info_write.far;
            }
            return 0;
        }
        if (val_probe_read64(base_pa, &value, &fault_info_read) != 0) {
            LOG_ERROR(NULL, "%s: realm probe read fault esr=%llx far=%llx",
                      GPT_TRANS_FEAT_RME_TEST_NAME,
                      (unsigned long long)fault_info_read.esr,
                      (unsigned long long)fault_info_read.far);
            if (out) {
                out->fail_step = GPT_TRANS_FEAT_RME_FAILSTEP_PROBE_READ;
                out->last_x0 = fault_info_read.esr;
                out->last_x1 = fault_info_read.far;
            }
            return 0;
        }
        if (value != test_data) {
            LOG_ERROR(NULL, "%s: realm probe read mismatch v=%llx exp=%llx",
                      GPT_TRANS_FEAT_RME_TEST_NAME,
                      (unsigned long long)value,
                      (unsigned long long)test_data);
            if (out) {
                out->fail_step = GPT_TRANS_FEAT_RME_FAILSTEP_PROBE_VALUE;
                out->last_x0 = value;
                out->last_x1 = test_data;
            }
            return 0;
        }
    }

    /* Step 3: Realm caller: Realm -> Non-secure (permitted). */
    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NONSECURE, &res);
    if (!val_firme_status_success(res.x0, res.x1, expected_done)) {
        LOG_ERROR(NULL, GPT_TRANS_FEAT_RME_TEST_NAME ": realm step3 R->NS failed x0=%llx x1=%llx",
                  (unsigned long long)res.x0, (unsigned long long)res.x1);
        if (out) {
            out->fail_step = 3;
            out->last_x0 = res.x0;
            out->last_x1 = res.x1;
        }
        return 0;
    }

    /* Step 4: Disallowed: Realm -> Secure. */
    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_REALM, &res);
    if (val_firme_status_success(res.x0, res.x1, expected_done)) {
        val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_SECURE, &res);
        if (!val_firme_status_deniedish(res.x0) && !val_firme_status_not_supported(res.x0)) {
            LOG_ERROR(NULL, "%s: realm step4 R->S expected DENIED/NSUP x0=%llx x1=%llx",
                      GPT_TRANS_FEAT_RME_TEST_NAME,
                      (unsigned long long)res.x0,
                      (unsigned long long)res.x1);
            if (out) {
                out->fail_step = 4;
                out->last_x0 = res.x0;
                out->last_x1 = res.x1;
            }
            return 0;
        }
    }

    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NONSECURE, &res);
    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;
    return 0;
}

/**
 *   @brief    - GPT transition policy (FEAT_RME): NS dispatcher body orchestrating the scenario
 *               and verify access faults.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Optional result record.
 *   @return   - 0 (status stored in `out->status` when provided).
**/
int firme_gpt_transition_feat_rme_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    uint32_t gpc2 = 0;
    uint32_t gdi = 0;
    firme_res_t res = (firme_res_t){0};
    val_fault_info_t fault_info = (val_fault_info_t){0};
    const uint64_t test_data = FIRME_TEST_DATA_PATTERN;
    const uint64_t granule_count = 1ULL;
    const uint32_t check_value = 1U;
    const uint64_t matrix_step_base = UINT64_C(0);
    const uint64_t clear_value = 0ULL;

    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }

    if (!val_firme_feat_rme_present() || base_pa == 0ULL) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return 0;
    }

    if (val_firme_gpt_gpi_set_advertised(out, UINT64_C(1000)) == VAL_FALSE)
        return 0;

    gpc2 = val_firme_feat_rme_gpc2_present();
    gdi = val_firme_feat_rme_gdi_present();

    /* Step 1: seed the test granule from NS. */
    if (val_probe_write64(base_pa, test_data, &fault_info) != 0) {
        LOG_ERROR(NULL, "%s: ns baseline write fault esr=%llx far=%llx",
                  GPT_TRANS_FEAT_RME_TEST_NAME,
                  (unsigned long long)fault_info.esr,
                  (unsigned long long)fault_info.far);
        if (out) {
            out->fail_step = 0;
            out->last_x0 = fault_info.esr;
            out->last_x1 = fault_info.far;
        }
        goto cleanup;
    }

    /* Step 2: validate baseline enforcement for Non-secure GPI. */
    if (!val_firme_check_access_matrix(base_pa, FIRME_GPI_NONSECURE, test_data, check_value,
                                       out, matrix_step_base)) {
        if (out) {
            LOG_ERROR(NULL, "%s: ns baseline matrix failed step=%llu x0=%llx x1=%llx",
                      GPT_TRANS_FEAT_RME_TEST_NAME,
                      (unsigned long long)out->fail_step,
                      (unsigned long long)out->last_x0,
                      (unsigned long long)out->last_x1);
        }
        goto cleanup;
    }

    /* Step 3: NS caller: NS->Secure is disallowed. */
    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_SECURE, &res);
    if (!val_firme_status_deniedish(res.x0) && !val_firme_status_not_supported(res.x0)) {
        LOG_ERROR(NULL, "%s: ns NS->S expected DENIED/NSUP x0=%llx x1=%llx",
                  GPT_TRANS_FEAT_RME_TEST_NAME,
                  (unsigned long long)res.x0,
                  (unsigned long long)res.x1);
        if (out) {
            out->fail_step = 1;
            out->last_x0 = res.x0;
            out->last_x1 = res.x1;
        }
        goto cleanup;
    }

    /* Step 4: NS caller: NS->Realm is disallowed. */
    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_REALM, &res);
    if (!val_firme_status_deniedish(res.x0) && !val_firme_status_not_supported(res.x0)) {
        LOG_ERROR(NULL, "%s: ns NS->R expected DENIED/NSUP x0=%llx x1=%llx",
                  GPT_TRANS_FEAT_RME_TEST_NAME,
                  (unsigned long long)res.x0,
                  (unsigned long long)res.x1);
        if (out) {
            out->fail_step = 2;
            out->last_x0 = res.x0;
            out->last_x1 = res.x1;
        }
        goto cleanup;
    }

    /* Step 5: NS caller: NS->Root is disallowed. */
    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_ROOT, &res);
    if (!val_firme_status_deniedish(res.x0) && !val_firme_status_not_supported(res.x0)) {
        LOG_ERROR(NULL, "%s: ns NS->ROOT expected DENIED/NSUP x0=%llx x1=%llx",
                  GPT_TRANS_FEAT_RME_TEST_NAME,
                  (unsigned long long)res.x0,
                  (unsigned long long)res.x1);
        if (out) {
            out->fail_step = 3;
            out->last_x0 = res.x0;
            out->last_x1 = res.x1;
        }
        goto cleanup;
    }

    if (!gpc2) {
        /* Step 6: NS caller: NS->NSO is disallowed when GPC2 is absent. */
        val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NSO, &res);
        if (!val_firme_status_deniedish(res.x0) && !val_firme_status_not_supported(res.x0)) {
            LOG_ERROR(NULL, "%s: ns NS->NSO expected DENIED/NSUP x0=%llx x1=%llx",
                      GPT_TRANS_FEAT_RME_TEST_NAME,
                      (unsigned long long)res.x0,
                      (unsigned long long)res.x1);
            if (out) {
                out->fail_step = 4;
                out->last_x0 = res.x0;
                out->last_x1 = res.x1;
            }
            goto cleanup;
        }
    }

    if (!gdi) {
        /* Step 7: NS caller: NS->NSP is disallowed when GDI is absent. */
        val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NSP, &res);
        if (!val_firme_status_deniedish(res.x0) && !val_firme_status_not_supported(res.x0)) {
            LOG_ERROR(NULL, "%s: ns NS->NSP expected DENIED/NSUP x0=%llx x1=%llx",
                      GPT_TRANS_FEAT_RME_TEST_NAME,
                      (unsigned long long)res.x0,
                      (unsigned long long)res.x1);
            if (out) {
                out->fail_step = 5;
                out->last_x0 = res.x0;
                out->last_x1 = res.x1;
            }
            goto cleanup;
        }
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;
cleanup:
    /*
     * Best-effort cleanup:
     * - Restore the granule to Non-secure from whichever world currently owns it.
     * - Clear test data using a fault-tolerant write so the suite does not halt
     *   even if cleanup isn't possible on a broken implementation.
     */
    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NONSECURE, &res);
    val_probe_write64(base_pa, clear_value, &fault_info);
    return 0;
}
