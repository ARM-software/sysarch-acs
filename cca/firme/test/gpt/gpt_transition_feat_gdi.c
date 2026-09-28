/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"
#include "val_firme_rules.h"
#include "val_log.h"

/* Fail-step bases for cross-world enforcement matrix checks. */
#define GPT_TRANS_FEAT_GDI_MATRIX_BASELINE_STEP_BASE        UINT64_C(10)
#define GPT_TRANS_FEAT_GDI_MATRIX_NSP_STEP_BASE             UINT64_C(40)
#define GPT_TRANS_FEAT_GDI_MATRIX_RESTORE_AFTER_NSP_STEP    UINT64_C(50)
#define GPT_TRANS_FEAT_GDI_MATRIX_SA_STEP_BASE              UINT64_C(60)
#define GPT_TRANS_FEAT_GDI_MATRIX_RESTORE_AFTER_SA_STEP     UINT64_C(70)
#define GPT_TRANS_FEAT_GDI_MATRIX_NSO_STEP_BASE             UINT64_C(80)
#define GPT_TRANS_FEAT_GDI_MATRIX_NSP_FROM_NSO_STEP_BASE    UINT64_C(90)
#define GPT_TRANS_FEAT_GDI_MATRIX_NSO_RESTORE_STEP_BASE     UINT64_C(100)
#define GPT_TRANS_FEAT_GDI_MATRIX_SA_FROM_NSO_STEP_BASE     UINT64_C(110)
#define GPT_TRANS_FEAT_GDI_MATRIX_NSO_FROM_SA_STEP_BASE     UINT64_C(120)
#define GPT_TRANS_FEAT_GDI_CLEANUP_STEP_BASE               UINT64_C(130)

#define GPT_TRANS_FEAT_GDI_TEST_NAME "gpt_transition_feat_gdi"

/**
 *   @brief    - GPT transition policy (FEAT_RME_GDI): NSP/SA transitions
 *               (and NSO interplay via FEAT_RME_GPC2).
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
**/
int firme_gpt_transition_feat_gdi_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    uint32_t gdi = 0;
    uint32_t gpc2 = 0;
    firme_res_t res = (firme_res_t){0};
    val_fault_info_t fault_info = (val_fault_info_t){0};
    const uint64_t test_data = FIRME_TEST_DATA_PATTERN;
    const uint64_t granule_count = 1ULL;
    const uint64_t expected_done = granule_count;
    const uint32_t check_value = 1U;
    const uint64_t clear_value = 0ULL;

    if (!out)
        return VAL_ERROR;

    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }

    if (!val_firme_feat_rme_present() || base_pa == 0ULL) {
        if (out) {
            out->status = (uint64_t)ACS_STATUS_SKIP;
        }
        return val_firme_gpt_rule_result_status(out);
    }

    if (val_firme_gpt_gpi_set_advertised(out, UINT64_C(1000)) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    gdi = val_firme_feat_rme_gdi_present();
    gpc2 = val_firme_feat_rme_gpc2_present();

    /* Step 1: seed the test granule from NS. */
    if (val_probe_write64(base_pa, test_data, &fault_info) != 0) {
        LOG_ERROR(NULL, "%s: baseline write fault esr=%llx far=%llx",
                  GPT_TRANS_FEAT_GDI_TEST_NAME,
                  (unsigned long long)fault_info.esr, (unsigned long long)fault_info.far);
        if (out) {
            out->fail_step = 0;
            out->last_x0 = fault_info.esr;
            out->last_x1 = fault_info.far;
        }
        goto cleanup;
    }

    /* Step 2: validate baseline enforcement for Non-secure GPI. */
    if (!val_firme_check_access_matrix(base_pa, FIRME_GPI_NONSECURE, test_data, check_value, out,
                                       GPT_TRANS_FEAT_GDI_MATRIX_BASELINE_STEP_BASE)) {
        if (out) {
            LOG_ERROR(NULL,
                      "%s: baseline matrix failed step=%llu x0=%llx x1=%llx",
                      GPT_TRANS_FEAT_GDI_TEST_NAME,
                      (unsigned long long)out->fail_step,
                      (unsigned long long)out->last_x0,
                      (unsigned long long)out->last_x1);
        }
        goto cleanup;
    }

    if (gdi == 0U) {
        /* Step 3: without GDI, NSP/SA target encodings are invalid. */
        val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NSP, &res);
        if (!val_firme_status_invalid_parameters(res.x0)) {
            LOG_ERROR(NULL, "%s: NS->NSP expected INVALID_PARAMETERS x0=%llx x1=%llx",
                      GPT_TRANS_FEAT_GDI_TEST_NAME,
                      (unsigned long long)res.x0, (unsigned long long)res.x1);
            if (out) {
                out->fail_step = 1;
                out->last_x0 = res.x0;
                out->last_x1 = res.x1;
            }
            goto cleanup;
        }

        val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_SA, &res);
        if (!val_firme_status_invalid_parameters(res.x0)) {
            LOG_ERROR(NULL, "%s: NS->SA expected INVALID_PARAMETERS x0=%llx x1=%llx",
                      GPT_TRANS_FEAT_GDI_TEST_NAME,
                      (unsigned long long)res.x0, (unsigned long long)res.x1);
            if (out) {
                out->fail_step = 2;
                out->last_x0 = res.x0;
                out->last_x1 = res.x1;
            }
            goto cleanup;
        }

        if (out) {
            out->status = (uint64_t)ACS_STATUS_PASS;
        }
        goto cleanup;
    }

    /* Step 4: GDI present: NS -> NSP -> NS. */
    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NSP, &res);
    if (!val_firme_status_success(res.x0, res.x1, expected_done)) {
        LOG_ERROR(NULL, "%s: NS->NSP failed x0=%llx x1=%llx",
                  GPT_TRANS_FEAT_GDI_TEST_NAME,
                  (unsigned long long)res.x0, (unsigned long long)res.x1);
        if (out) {
            out->fail_step = 3;
            out->last_x0 = res.x0;
            out->last_x1 = res.x1;
        }
        goto cleanup;
    }

    if (!val_firme_check_access_matrix(base_pa, FIRME_GPI_NSP, test_data, check_value, out,
                                       GPT_TRANS_FEAT_GDI_MATRIX_NSP_STEP_BASE)) {
        if (out) {
            LOG_ERROR(NULL,
                      "%s: NSP matrix failed step=%llu x0=%llx x1=%llx",
                      GPT_TRANS_FEAT_GDI_TEST_NAME,
                      (unsigned long long)out->fail_step,
                      (unsigned long long)out->last_x0,
                      (unsigned long long)out->last_x1);
        }
        goto cleanup;
    }

    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NONSECURE, &res);
    if (!val_firme_status_success(res.x0, res.x1, expected_done)) {
        LOG_ERROR(NULL, "%s: NSP->NS failed x0=%llx x1=%llx",
                  GPT_TRANS_FEAT_GDI_TEST_NAME,
                  (unsigned long long)res.x0, (unsigned long long)res.x1);
        if (out) {
            out->fail_step = 4;
            out->last_x0 = res.x0;
            out->last_x1 = res.x1;
        }
        goto cleanup;
    }

    if (!val_firme_check_access_matrix(base_pa, FIRME_GPI_NONSECURE, test_data, check_value, out,
                                       GPT_TRANS_FEAT_GDI_MATRIX_RESTORE_AFTER_NSP_STEP)) {
        if (out) {
            LOG_ERROR(NULL,
                      "%s: NS restore matrix failed step=%llu x0=%llx x1=%llx",
                      GPT_TRANS_FEAT_GDI_TEST_NAME,
                      (unsigned long long)out->fail_step,
                      (unsigned long long)out->last_x0,
                      (unsigned long long)out->last_x1);
        }
        goto cleanup;
    }

    /* Step 5: GDI present: NS -> SA -> NS. */
    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_SA, &res);
    if (!val_firme_status_success(res.x0, res.x1, expected_done)) {
        LOG_ERROR(NULL, "%s: NS->SA failed x0=%llx x1=%llx",
                  GPT_TRANS_FEAT_GDI_TEST_NAME,
                  (unsigned long long)res.x0, (unsigned long long)res.x1);
        if (out) {
            out->fail_step = 5;
            out->last_x0 = res.x0;
            out->last_x1 = res.x1;
        }
        goto cleanup;
    }

    if (!val_firme_check_access_matrix(base_pa, FIRME_GPI_SA, test_data, check_value, out,
                                       GPT_TRANS_FEAT_GDI_MATRIX_SA_STEP_BASE)) {
        if (out) {
            LOG_ERROR(NULL,
                      "%s: SA matrix failed step=%llu x0=%llx x1=%llx",
                      GPT_TRANS_FEAT_GDI_TEST_NAME,
                      (unsigned long long)out->fail_step,
                      (unsigned long long)out->last_x0,
                      (unsigned long long)out->last_x1);
        }
        goto cleanup;
    }

    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NONSECURE, &res);
    if (!val_firme_status_success(res.x0, res.x1, expected_done)) {
        LOG_ERROR(NULL, "%s: SA->NS failed x0=%llx x1=%llx",
                  GPT_TRANS_FEAT_GDI_TEST_NAME,
                  (unsigned long long)res.x0, (unsigned long long)res.x1);
        if (out) {
            out->fail_step = 6;
            out->last_x0 = res.x0;
            out->last_x1 = res.x1;
        }
        goto cleanup;
    }

    if (!val_firme_check_access_matrix(base_pa, FIRME_GPI_NONSECURE, test_data, check_value, out,
                                       GPT_TRANS_FEAT_GDI_MATRIX_RESTORE_AFTER_SA_STEP)) {
        if (out) {
            LOG_ERROR(NULL,
                      "%s: NS restore matrix failed step=%llu x0=%llx x1=%llx",
                      GPT_TRANS_FEAT_GDI_TEST_NAME,
                      (unsigned long long)out->fail_step,
                      (unsigned long long)out->last_x0,
                      (unsigned long long)out->last_x1);
        }
        goto cleanup;
    }

    if (gpc2 != 0U) {
        /*
         * Step 6: GPC2 present: validate additional NSO-related transitions.
         *
         * These checks are only performed when NS->NSO succeeds.
         */
        val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NSO, &res);
        if (val_firme_status_success(res.x0, res.x1, expected_done)) {
            if (!val_firme_check_access_matrix(base_pa, FIRME_GPI_NSO, test_data, check_value, out,
                                               GPT_TRANS_FEAT_GDI_MATRIX_NSO_STEP_BASE)) {
                if (out) {
                    LOG_ERROR(NULL,
                              "%s: NSO matrix failed step=%llu x0=%llx x1=%llx",
                              GPT_TRANS_FEAT_GDI_TEST_NAME,
                              (unsigned long long)out->fail_step,
                              (unsigned long long)out->last_x0,
                              (unsigned long long)out->last_x1);
                }
                goto cleanup;
            }

            val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NSP, &res);
            if (!val_firme_status_success(res.x0, res.x1, expected_done)) {
                LOG_ERROR(NULL, "%s: NSO->NSP failed x0=%llx x1=%llx",
                          GPT_TRANS_FEAT_GDI_TEST_NAME,
                          (unsigned long long)res.x0, (unsigned long long)res.x1);
                if (out) {
                    out->fail_step = 7;
                    out->last_x0 = res.x0;
                    out->last_x1 = res.x1;
                }
                goto cleanup;
            }

            if (!val_firme_check_access_matrix(base_pa, FIRME_GPI_NSP, test_data, check_value, out,
                                               GPT_TRANS_FEAT_GDI_MATRIX_NSP_FROM_NSO_STEP_BASE)) {
                if (out) {
                    LOG_ERROR(NULL,
                              "%s: NSP matrix failed step=%llu x0=%llx x1=%llx",
                              GPT_TRANS_FEAT_GDI_TEST_NAME,
                              (unsigned long long)out->fail_step,
                              (unsigned long long)out->last_x0,
                              (unsigned long long)out->last_x1);
                }
                goto cleanup;
            }

            val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NSO, &res);
            if (!val_firme_status_success(res.x0, res.x1, expected_done)) {
                LOG_ERROR(NULL, "%s: NSP->NSO failed x0=%llx x1=%llx",
                          GPT_TRANS_FEAT_GDI_TEST_NAME,
                          (unsigned long long)res.x0, (unsigned long long)res.x1);
                if (out) {
                    out->fail_step = 8;
                    out->last_x0 = res.x0;
                    out->last_x1 = res.x1;
                }
                goto cleanup;
            }

            if (!val_firme_check_access_matrix(base_pa, FIRME_GPI_NSO, test_data, check_value, out,
                                               GPT_TRANS_FEAT_GDI_MATRIX_NSO_RESTORE_STEP_BASE)) {
                if (out) {
                    LOG_ERROR(NULL,
                              "%s: NSO matrix failed step=%llu x0=%llx x1=%llx",
                              GPT_TRANS_FEAT_GDI_TEST_NAME,
                              (unsigned long long)out->fail_step,
                              (unsigned long long)out->last_x0,
                              (unsigned long long)out->last_x1);
                }
                goto cleanup;
            }

            val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_SA, &res);
            if (!val_firme_status_success(res.x0, res.x1, expected_done)) {
                LOG_ERROR(NULL, "%s: NSO->SA failed x0=%llx x1=%llx",
                          GPT_TRANS_FEAT_GDI_TEST_NAME,
                          (unsigned long long)res.x0, (unsigned long long)res.x1);
                if (out) {
                    out->fail_step = 9;
                    out->last_x0 = res.x0;
                    out->last_x1 = res.x1;
                }
                goto cleanup;
            }

            if (!val_firme_check_access_matrix(base_pa, FIRME_GPI_SA, test_data, check_value, out,
                                               GPT_TRANS_FEAT_GDI_MATRIX_SA_FROM_NSO_STEP_BASE)) {
                if (out) {
                    LOG_ERROR(NULL,
                              "%s: SA matrix failed step=%llu x0=%llx x1=%llx",
                              GPT_TRANS_FEAT_GDI_TEST_NAME,
                              (unsigned long long)out->fail_step,
                              (unsigned long long)out->last_x0,
                              (unsigned long long)out->last_x1);
                }
                goto cleanup;
            }

            val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NSO, &res);
            if (!val_firme_status_success(res.x0, res.x1, expected_done)) {
                LOG_ERROR(NULL, "%s: SA->NSO failed x0=%llx x1=%llx",
                          GPT_TRANS_FEAT_GDI_TEST_NAME,
                          (unsigned long long)res.x0, (unsigned long long)res.x1);
                if (out) {
                    out->fail_step = 10;
                    out->last_x0 = res.x0;
                    out->last_x1 = res.x1;
                }
                goto cleanup;
            }

            if (!val_firme_check_access_matrix(base_pa, FIRME_GPI_NSO, test_data, check_value, out,
                                               GPT_TRANS_FEAT_GDI_MATRIX_NSO_FROM_SA_STEP_BASE)) {
                if (out) {
                    LOG_ERROR(NULL,
                              "%s: NSO matrix failed step=%llu x0=%llx x1=%llx",
                              GPT_TRANS_FEAT_GDI_TEST_NAME,
                              (unsigned long long)out->fail_step,
                              (unsigned long long)out->last_x0,
                              (unsigned long long)out->last_x1);
                }
                goto cleanup;
            }
        }
    }

    if (out) {
        out->status = (uint64_t)ACS_STATUS_PASS;
    }

cleanup:
    if (val_firme_gpt_restore_ns(base_pa, out,
                                GPT_TRANS_FEAT_GDI_CLEANUP_STEP_BASE) == VAL_FALSE)
        return VAL_ERROR;
    if (val_probe_write64(base_pa, clear_value, &fault_info) != VAL_SUCCESS) {
        val_firme_gpt_rule_result_fail(out,
            GPT_TRANS_FEAT_GDI_CLEANUP_STEP_BASE + UINT64_C(1),
            fault_info.esr, fault_info.far);
        return VAL_ERROR;
    }
    return val_firme_gpt_rule_result_status(out);
}
