/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"
#include "val_firme_rules.h"
#include "val_log.h"

#define GPT_TRANS_FEAT_GDI_TEST_NAME "gpt_transition_feat_gdi"
#define GPT_TRANS_FEAT_GDI_MATRIX_BASELINE_STEP UINT64_C(10)
#define GPT_TRANS_FEAT_GDI_TRANSITION_STEP      UINT64_C(20)
#define GPT_TRANS_FEAT_GDI_MATRIX_STEP          UINT64_C(40)
#define GPT_TRANS_FEAT_GDI_CLEANUP_STEP         UINT64_C(130)

/**
 * @brief Validate all NS/NSO to NSP/SA transitions, or reject unallocated targets.
 * @param base_pa Owned NS test granule physical address.
 * @param out Required rule result and failure diagnostics.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_transition_feat_gdi_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    const uint64_t transition_targets[] = {
        FIRME_GPI_NSP, FIRME_GPI_NONSECURE,
        FIRME_GPI_SA, FIRME_GPI_NONSECURE,
        FIRME_GPI_NSO, FIRME_GPI_NSP, FIRME_GPI_NSO,
        FIRME_GPI_SA, FIRME_GPI_NSO,
    };
    const uint64_t unallocated_targets[] = {FIRME_GPI_NSP, FIRME_GPI_SA};
    firme_res_t result = {0};
    val_fault_info_t fault_info = {0};
    uint64_t current_gpi = FIRME_GPI_NONSECURE;
    uint64_t target_gpi;
    uint64_t step;
    size_t transition_index;

    if (!out)
        return VAL_ERROR;

    val_firme_gpt_rule_result_init(out);
    if (!val_firme_feat_rme_present() || (base_pa == 0ULL)) {
        out->status = ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }

    /* Step 1: discover the service and establish NS access to the owned granule. */
    if (val_firme_gpt_gpi_set_advertised(out, UINT64_C(1000)) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    if (val_probe_write64(base_pa, FIRME_TEST_DATA_PATTERN, &fault_info) != VAL_SUCCESS) {
        LOG_ERROR(NULL, "%s: baseline write fault esr=%llx far=%llx",
                  GPT_TRANS_FEAT_GDI_TEST_NAME,
                  (unsigned long long)fault_info.esr, (unsigned long long)fault_info.far);
        val_firme_gpt_rule_result_fail(out, GPT_TRANS_FEAT_GDI_MATRIX_BASELINE_STEP,
                                       fault_info.esr, fault_info.far);
        goto cleanup;
    }

    if (!val_firme_check_access_matrix(base_pa, current_gpi, FIRME_TEST_DATA_PATTERN,
                                       VAL_TRUE, out, GPT_TRANS_FEAT_GDI_MATRIX_BASELINE_STEP))
        goto cleanup;

    /* Step 2: without GDI, both unallocated GPI encodings must be rejected. */
    if (!val_firme_feat_rme_gdi_present()) {
        for (transition_index = 0U;
             transition_index < sizeof(unallocated_targets) / sizeof(unallocated_targets[0]);
             transition_index++) {
            target_gpi = unallocated_targets[transition_index];
            val_firme_gm_gpi_set_complete(base_pa, UINT64_C(1), target_gpi, &result);
            if (!val_firme_status_invalid_parameters(result.x0)) {
                LOG_ERROR(NULL, "%s: unallocated target=%llx expected INVALID_PARAMETERS "
                          "x0=%llx x1=%llx", GPT_TRANS_FEAT_GDI_TEST_NAME,
                          (unsigned long long)target_gpi,
                          (unsigned long long)result.x0, (unsigned long long)result.x1);
                val_firme_gpt_rule_result_fail(out, transition_index + UINT64_C(1),
                                               result.x0, result.x1);
                goto cleanup;
            }
        }

        out->status = ACS_STATUS_PASS;
        goto cleanup;
    }

    /* Step 3: GDI includes GPC2, so its NSO transitions must not be silently omitted. */
    if (!val_firme_feat_rme_gpc2_present()) {
        LOG_ERROR(NULL, "%s: GDI advertised without required GPC2", GPT_TRANS_FEAT_GDI_TEST_NAME);
        val_firme_gpt_rule_result_fail(out, GPT_TRANS_FEAT_GDI_TRANSITION_STEP, 0ULL, 0ULL);
        goto cleanup;
    }

    /* Step 4: exercise every Table 3.3 edge, checking PAS enforcement after each call. */
    for (transition_index = 0U;
         transition_index < sizeof(transition_targets) / sizeof(transition_targets[0]);
         transition_index++) {
        target_gpi = transition_targets[transition_index];
        step = GPT_TRANS_FEAT_GDI_TRANSITION_STEP + transition_index;
        val_firme_gm_gpi_set_complete(base_pa, UINT64_C(1), target_gpi, &result);
        if (!val_firme_status_success(result.x0, result.x1, UINT64_C(1))) {
            LOG_ERROR(NULL, "%s: GPI %llx->%llx expected SUCCESS/count=1 x0=%llx x1=%llx",
                      GPT_TRANS_FEAT_GDI_TEST_NAME,
                      (unsigned long long)current_gpi, (unsigned long long)target_gpi,
                      (unsigned long long)result.x0, (unsigned long long)result.x1);
            val_firme_gpt_rule_result_fail(out, step, result.x0, result.x1);
            goto cleanup;
        }

        current_gpi = target_gpi;
        if (!val_firme_check_access_matrix(base_pa, current_gpi, 0ULL, VAL_FALSE, out,
                                           GPT_TRANS_FEAT_GDI_MATRIX_STEP +
                                           transition_index * UINT64_C(3))) {
            LOG_ERROR(NULL, "%s: target=%llx access matrix failed step=%llu x0=%llx x1=%llx",
                      GPT_TRANS_FEAT_GDI_TEST_NAME, (unsigned long long)current_gpi,
                      (unsigned long long)out->fail_step,
                      (unsigned long long)out->last_x0, (unsigned long long)out->last_x1);
            goto cleanup;
        }
    }

    out->status = ACS_STATUS_PASS;

cleanup:
    /* Step 5: scrub NSO to PoC before release; PAS transitions do not promise data retention. */
    if ((current_gpi == FIRME_GPI_NSO) &&
        !val_firme_gpt_scrub_ns(base_pa, out, GPT_TRANS_FEAT_GDI_CLEANUP_STEP))
        return VAL_ERROR;

    if (!val_firme_gpt_restore_ns(base_pa, out, GPT_TRANS_FEAT_GDI_CLEANUP_STEP))
        return VAL_ERROR;

    if (!val_firme_gpt_scrub_ns(base_pa, out, GPT_TRANS_FEAT_GDI_CLEANUP_STEP + UINT64_C(1)))
        return VAL_ERROR;

    return val_firme_gpt_rule_result_status(out);
}
