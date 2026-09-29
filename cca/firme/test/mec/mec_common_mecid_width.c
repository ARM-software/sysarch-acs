/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_mec.h"
#include "val_firme_rules.h"
#include "val_log.h"
#include "val_firme_mp_test.h"
#include "val_common/val_mp.h"
#include "val_iovirt.h"
#include "val_smmu.h"

#define MEC_COMMON_WIDTH_TEST_NAME "mec_common_mecid_width"

#define MEC_COMMON_WIDTH_STEP_RANGE        UINT64_C(1)
#define MEC_COMMON_WIDTH_STEP_DUT_WIDTH    UINT64_C(2)
#define MEC_COMMON_WIDTH_STEP_UNSUPPORTED  UINT64_C(3)
#define MEC_COMMON_WIDTH_STEP_INVALID_ID   UINT64_C(4)
#define MEC_COMMON_WIDTH_STEP_VALID_ID     UINT64_C(5)
#define MEC_COMMON_WIDTH_STEP_EXPECTED     UINT64_C(6)
#define MEC_COMMON_WIDTH_STEP_MISMATCH     UINT64_C(7)
#define MEC_COMMON_WIDTH_STEP_SMMU         UINT64_C(8)
#define MEC_COMMON_WIDTH_STEP_TOPOLOGY     UINT64_C(9)

typedef enum {
    MEC_COMMON_WIDTH_COLLECT_PE = 1U,
    MEC_COMMON_WIDTH_COLLECT_SMMU,
    MEC_COMMON_WIDTH_CHECK_BOUNDARIES,
} mec_common_width_phase_t;

static val_test_pe_result_t g_mec_pe_results[PAL_PE_COUNT];

/**
 *   @brief    - Initialize a rule result record for the common MECID width test.
 *   @param    - out : Optional result record.
 *   @return   - void
 **/
static void mec_common_width_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a common MECID width test failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void
 **/
static void mec_common_width_fail(firme_rule_res_t *out, uint64_t fail_step,
                                  uint64_t last_x0, uint64_t last_x1)
{
    LOG_ERROR(NULL, "%s: failed step=%llu x0=%llx x1=%llx",
              MEC_COMMON_WIDTH_TEST_NAME, (unsigned long long)fail_step,
              (unsigned long long)last_x0, (unsigned long long)last_x1);

    if (out) {
        out->status = ACS_STATUS_FAIL;
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Validate MEC_REFRESH against the independently collected width.
 *   @param    - expected_width : Minimum over all discovered PEs and SMMUs.
 *   @param    - out : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, VAL_ERROR for a failed check.
 **/
static int mec_common_width_run(uint32_t expected_width, firme_rule_res_t *out)
{
    firme_res_t res = {0};
    firme_res_t width_res = {0};
    uint32_t advertised_width = 0U;
    uint64_t valid_mecid = 0ULL;
    uint64_t valid_params = 0ULL;
    uint64_t invalid_mecid = 0ULL;
    uint64_t invalid_params = 0ULL;

    mec_common_width_init_result(out);

    /* Step 1: If MECID management is absent, this platform cannot run the rule. */
    val_firme_service_version(FIRME_SERVICE_ID_MECID_MGMT, &res);
    if (val_firme_status_to_i64(res.x0) == (int64_t)FIRME_NOT_SUPPORTED) {
        LOG_DEBUG(NULL, "%s: MECID management service not implemented",
                  MEC_COMMON_WIDTH_TEST_NAME);
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }

    if (val_firme_status_to_i64(res.x0) < 0) {
        mec_common_width_fail(out, MEC_COMMON_WIDTH_STEP_UNSUPPORTED, res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 2: Read the DUT advertised common width from MEC feature register 1. */
    if (val_firme_mec_advertised_common_mecid_width(&advertised_width, &width_res) !=
        VAL_SUCCESS) {
        mec_common_width_fail(out, MEC_COMMON_WIDTH_STEP_DUT_WIDTH, width_res.x0,
                              width_res.x1);
        return VAL_ERROR;
    }
    if ((expected_width < VAL_FIRME_MECID_WIDTH_MIN) ||
        (expected_width > VAL_FIRME_MECID_WIDTH_MAX) ||
        (expected_width > VAL_FIRME_MECID_U64_SHIFT_MAX) ||
        (advertised_width < VAL_FIRME_MECID_WIDTH_MIN) ||
        (advertised_width > VAL_FIRME_MECID_WIDTH_MAX) ||
        (advertised_width > VAL_FIRME_MECID_U64_SHIFT_MAX)) {
        mec_common_width_fail(out, MEC_COMMON_WIDTH_STEP_RANGE,
                              (uint64_t)expected_width, (uint64_t)advertised_width);
        return VAL_ERROR;
    }

    /* Step 3: The DUT advertised width must match the ACS-derived common width. */
    if (advertised_width != expected_width) {
        mec_common_width_fail(out, MEC_COMMON_WIDTH_STEP_MISMATCH,
                              (uint64_t)expected_width, (uint64_t)advertised_width);
        return VAL_ERROR;
    }

    /* Step 4: The largest MECID within the common width must be accepted. */
    valid_mecid = (UINT64_C(1) << expected_width) - 1ULL;
    valid_params = val_firme_mec_params(valid_mecid, FIRME_MEC_REFRESH_REASON_CREATE);
    val_firme_mec_refresh_do_retry(valid_params, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        mec_common_width_fail(out, MEC_COMMON_WIDTH_STEP_VALID_ID, res.x0, valid_mecid);
        return VAL_ERROR;
    }

    /* Step 5: The first MECID outside the common width is rejected when encodable. */
    if (expected_width < VAL_FIRME_MECID_WIDTH_MAX) {
        invalid_mecid = UINT64_C(1) << expected_width;
        invalid_params = val_firme_mec_params(invalid_mecid,
                                              FIRME_MEC_REFRESH_REASON_CREATE);
        val_firme_mec_refresh(invalid_params, &res);
        if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_INVALID_PARAMETERS) {
            mec_common_width_fail(out, MEC_COMMON_WIDTH_STEP_INVALID_ID, res.x0,
                                  invalid_mecid);
            return VAL_ERROR;
        }
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 * @brief Execute one R0239 phase on the selected physical PE in Realm.
 * @param world Caller world; only Realm is valid for this test.
 * @param phase Collect PE width, collect SMMU width, or check MECID bounds.
 * @param argument SMMU index or independently derived system minimum width.
 * @param out Full result, retained per PE until the host retrieves every word.
 * @return VAL_SUCCESS for PASS/SKIP, VAL_ERROR for failures.
 **/
int firme_mec_common_mecid_width_payload(firme_acs_world_t world, uint32_t phase,
                                         uint32_t argument, firme_rule_res_t *out)
{
    firme_res_t res = {0};
    uint32_t local_width = 0U;
    uint32_t advertised_width = 0U;
    int smmu_status = VAL_ERROR;

    if (out == NULL)
        return VAL_ERROR;

    mec_common_width_init_result(out);
    if (world != FIRME_ACS_WORLD_REALM)
        return VAL_ERROR;

    /* Step 1: Discover the service on this PE; unexpected errors are failures. */
    val_firme_service_version(FIRME_SERVICE_ID_MECID_MGMT, &res);
    if (val_firme_status_to_i64(res.x0) == FIRME_NOT_SUPPORTED) {
        out->status = ACS_STATUS_SKIP;
        out->fail_step = MEC_COMMON_WIDTH_STEP_UNSUPPORTED;
        return VAL_SUCCESS;
    }

    if (val_firme_status_to_i64(res.x0) < 0) {
        mec_common_width_fail(out, MEC_COMMON_WIDTH_STEP_UNSUPPORTED, res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 2: Sample this PE's own register before any host-side reduction. */
    if (phase == MEC_COMMON_WIDTH_COLLECT_PE) {
        if (val_firme_mec_local_pe_mecid_width(&local_width) != VAL_SUCCESS) {
            mec_common_width_fail(out, MEC_COMMON_WIDTH_STEP_EXPECTED, 0U, 0U);
            return VAL_ERROR;
        }

        if (val_firme_mec_advertised_common_mecid_width(&advertised_width, &res) !=
            VAL_SUCCESS) {
            mec_common_width_fail(out, MEC_COMMON_WIDTH_STEP_DUT_WIDTH, res.x0, res.x1);
            return VAL_ERROR;
        }

        out->last_x0 = local_width;
        out->last_x1 = advertised_width;
        out->status = ACS_STATUS_PASS;
        return VAL_SUCCESS;
    }

    /* Step 3: Read a discovered SMMU's Realm capability and MECID width. */
    if (phase == MEC_COMMON_WIDTH_COLLECT_SMMU) {
        smmu_status = val_smmu_mecid_width(argument, &local_width);
        if (smmu_status == VAL_SMMU_MEC_UNAVAILABLE) {
            out->status = ACS_STATUS_SKIP;
            out->fail_step = MEC_COMMON_WIDTH_STEP_SMMU;
            out->last_x0 = argument;
            return VAL_SUCCESS;
        }

        if (smmu_status != VAL_SUCCESS) {
            mec_common_width_fail(out, MEC_COMMON_WIDTH_STEP_SMMU, argument, 0U);
            return VAL_ERROR;
        }

        out->last_x0 = local_width;
        out->last_x1 = argument;
        out->status = ACS_STATUS_PASS;
        return VAL_SUCCESS;
    }

    /* Step 4: Exercise the real ABI against the system minimum supplied by ACS. */
    if (phase == MEC_COMMON_WIDTH_CHECK_BOUNDARIES)
        return mec_common_width_run(argument, out);

    return VAL_ERROR;
}

/** @brief Propagate failed/incomplete PE results without losing diagnostics. */
static int mec_common_width_collect_status(uint32_t num_pe, firme_rule_res_t *out)
{
    uint32_t slot = 0U;

    *out = (firme_rule_res_t){.status = ACS_STATUS_PASS};
    for (slot = 0U; slot < num_pe; slot++) {
        if ((g_mec_pe_results[slot].result.status != ACS_STATUS_PASS) &&
            (g_mec_pe_results[slot].result.status != ACS_STATUS_SKIP)) {
            *out = g_mec_pe_results[slot].result;
            LOG_ERROR(NULL, "%s: PE=%u failed", MEC_COMMON_WIDTH_TEST_NAME,
                      g_mec_pe_results[slot].pe_index);
            return VAL_ERROR;
        }

        if (g_mec_pe_results[slot].result.status == ACS_STATUS_SKIP)
            *out = g_mec_pe_results[slot].result;
    }

    return VAL_SUCCESS;
}

/**
 * @brief Coordinate R0239 in the normal suite using all platform PEs and SMMUs.
 * @param world Selected caller world; this rule executes in Realm only.
 * @param num_pe Number selected by the registry; this rule requires every PE.
 * @param run_payload VAL-owned boot/dispatch/wait/off lifecycle for each phase.
 * @param out One aggregated rule result, independent of the number of PEs.
 * @return VAL_SUCCESS for PASS/SKIP, VAL_ERROR for any test/infrastructure error.
 */
int firme_mec_common_mecid_width_entry(firme_acs_world_t world, uint32_t num_pe,
                                       val_test_run_payload_t run_payload,
                                       firme_rule_res_t *out)
{
    val_test_payload_t payload = {
        .rule_id = FIRME_RULE_R0239,
        .world = FIRME_ACS_WORLD_REALM,
        .phase = MEC_COMMON_WIDTH_COLLECT_PE,
    };
    uint32_t minimum_width = VAL_FIRME_MECID_WIDTH_MAX;
    uint32_t advertised[PAL_PE_COUNT] = {0};
    uint32_t smmu_count = 0U;
    uint32_t smmu_index = 0U;
    uint32_t slot = 0U;
    uint32_t width = 0U;
    uint32_t missing_smmu = 0U;
    uint32_t incomplete = VAL_FALSE;
    int run_status = VAL_ERROR;

    if (out == NULL)
        return VAL_ERROR;

    mec_common_width_init_result(out);
    if ((world != FIRME_ACS_WORLD_REALM) || (num_pe != PAL_PE_COUNT) ||
        (run_payload == NULL) ||
        (val_iovirt_smmu_count(&smmu_count) != VAL_SUCCESS)) {
        mec_common_width_fail(out, MEC_COMMON_WIDTH_STEP_TOPOLOGY, num_pe, PAL_PE_COUNT);
        return VAL_ERROR;
    }

    /* Step 1: Collect MECIDR_EL2 and DUT advertisement independently on every PE. */
    run_status = run_payload(num_pe, &payload, g_mec_pe_results);
    if (mec_common_width_collect_status(num_pe, out) != VAL_SUCCESS)
        return VAL_ERROR;

    if (run_status != VAL_SUCCESS) {
        out->status = ACS_STATUS_ERR;
        return VAL_ERROR;
    }

    if (out->status == ACS_STATUS_SKIP)
        return VAL_SUCCESS;

    for (slot = 0U; slot < num_pe; slot++) {
        width = (uint32_t)g_mec_pe_results[slot].result.last_x0;
        advertised[slot] = (uint32_t)g_mec_pe_results[slot].result.last_x1;
        LOG_TEST(NULL, "MEC width: PE=%u hardware=%u advertised=%u",
                 g_mec_pe_results[slot].pe_index, width, advertised[slot]);
        if ((width < VAL_FIRME_MECID_WIDTH_MIN) || (width > VAL_FIRME_MECID_WIDTH_MAX)) {
            mec_common_width_fail(out, MEC_COMMON_WIDTH_STEP_RANGE, slot, width);
            return VAL_ERROR;
        }

        if (width < minimum_width)
            minimum_width = width;
    }

    /* Step 2: Include every SMMU, not just the controllers attached to an endpoint. */
    payload.phase = MEC_COMMON_WIDTH_COLLECT_SMMU;
    for (smmu_index = 0U; smmu_index < smmu_count; smmu_index++) {
        payload.argument = smmu_index;
        run_status = run_payload(1U, &payload, g_mec_pe_results);
        *out = g_mec_pe_results[0].result;
        if (run_status != VAL_SUCCESS) {
            if ((out->status == ACS_STATUS_PASS) || (out->status == ACS_STATUS_SKIP))
                out->status = ACS_STATUS_ERR;

            return VAL_ERROR;
        }

        if (out->status == ACS_STATUS_SKIP) {
            LOG_WARN(NULL, "MEC width: SMMU=%u has no MEC support; coverage incomplete",
                     smmu_index);
            incomplete = VAL_TRUE;
            missing_smmu = smmu_index;
            continue;
        }

        width = (uint32_t)out->last_x0;
        LOG_TEST(NULL, "MEC width: SMMU=%u hardware=%u", smmu_index, width);
        if ((width < VAL_FIRME_MECID_WIDTH_MIN) || (width > VAL_FIRME_MECID_WIDTH_MAX)) {
            mec_common_width_fail(out, MEC_COMMON_WIDTH_STEP_RANGE, smmu_index, width);
            return VAL_ERROR;
        }

        if (width < minimum_width)
            minimum_width = width;
    }

    /* Step 3: Compare each advertisement with the independent system minimum. */
    LOG_TEST(NULL, "MEC width: %s minimum=%u (%u PEs, %u SMMUs)",
             incomplete ? "observed" : "system", minimum_width, num_pe, smmu_count);
    for (slot = 0U; slot < num_pe; slot++) {
        if ((advertised[slot] > minimum_width) ||
            (!incomplete && (advertised[slot] != minimum_width))) {
            mec_common_width_fail(out, MEC_COMMON_WIDTH_STEP_MISMATCH,
                                  minimum_width, advertised[slot]);
            return VAL_ERROR;
        }
    }

    if (incomplete) {
        *out = (firme_rule_res_t){.status = ACS_STATUS_SKIP,
                                 .fail_step = MEC_COMMON_WIDTH_STEP_SMMU,
                                 .last_x0 = missing_smmu};
        return VAL_SUCCESS;
    }

    /* Step 4: Validate accepted/rejected MECID bounds from every PE in Realm. */
    payload.phase = MEC_COMMON_WIDTH_CHECK_BOUNDARIES;
    payload.argument = minimum_width;
    run_status = run_payload(num_pe, &payload, g_mec_pe_results);
    if (mec_common_width_collect_status(num_pe, out) != VAL_SUCCESS)
        return VAL_ERROR;

    if (run_status != VAL_SUCCESS) {
        out->status = ACS_STATUS_ERR;
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}
