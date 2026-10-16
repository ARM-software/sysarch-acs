/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "shared_buffer_test_common.h"

#include "val_firme_abi.h"
#include "val_firme_service.h"
#include "val_log.h"

#define SHARED_BUFFER_TEST_NAME "shared_buffer_properties"

/**
 *   @brief    - Initialize a shared-buffer rule result.
 *   @param    - out : Result record.
 *   @return   - void.
 **/
static void shared_buffer_init_result(firme_rule_res_t *out)
{
    if (out == 0)
        return;

    out->status = (uint64_t)ACS_STATUS_FAIL;
    out->fail_step = 0ULL;
    out->last_x0 = 0ULL;
    out->last_x1 = 0ULL;
}

/**
 *   @brief    - Record a shared-buffer rule failure.
 *   @param    - out       : Result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : First diagnostic value.
 *   @param    - last_x1   : Second diagnostic value.
 *   @return   - void.
 **/
void firme_shared_buffer_test_fail(firme_rule_res_t *out, uint64_t fail_step,
                                   uint64_t last_x0, uint64_t last_x1)
{
    LOG_ERROR(NULL, "%s: step=%llu x0=%llx x1=%llx",
              SHARED_BUFFER_TEST_NAME, (unsigned long long)fail_step,
              (unsigned long long)last_x0, (unsigned long long)last_x1);

    if (out == 0)
        return;

    out->fail_step = fail_step;
    out->last_x0 = last_x0;
    out->last_x1 = last_x1;
}

/**
 *   @brief    - Record that no shared-buffer service probe is available.
 *   @param    - out   : Result record.
 *   @param    - world : FIRME caller world.
 *   @return   - void.
 **/
static void shared_buffer_skip(firme_rule_res_t *out, firme_acs_world_t world)
{
    LOG_WARN(NULL, "%s: service or safe shared-buffer resources unavailable world=%u",
             SHARED_BUFFER_TEST_NAME, (unsigned int)world);

    if (out == 0)
        return;

    out->status = (uint64_t)ACS_STATUS_SKIP;
    out->fail_step = SHARED_BUFFER_STEP_PROBE_UNAVAILABLE;
    out->last_x0 = (uint64_t)world;
    out->last_x1 = 0ULL;
}

/**
 *   @brief    - Prepare a shared-buffer rule at one FIRME instance.
 *   @param    - world  : FIRME caller world.
 *   @param    - out    : Result record.
 *   @param    - limits : Returned Base-advertised limits.
 *   @return   - VAL_SUCCESS for usable setup or SKIP, otherwise VAL_ERROR.
 **/
int firme_shared_buffer_test_setup(firme_acs_world_t world, firme_rule_res_t *out,
                                   shared_buffer_limits_t *limits)
{
    if ((out == 0) || (limits == 0))
        return VAL_ERROR;

    shared_buffer_init_result(out);
    if (val_firme_shared_buffer_probe_available(world) == VAL_FALSE) {
        shared_buffer_skip(out, world);
        return VAL_SUCCESS;
    }

    if (val_firme_shared_buffer_get_limits(
            &limits->minimum_size_bytes, &limits->maximum_page_count,
            &limits->maximum_size_bytes) != VAL_SUCCESS) {
        firme_shared_buffer_test_fail(out, SHARED_BUFFER_STEP_LIMITS, 0ULL, 0ULL);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/**
 *   @brief    - Collect one service-specific shared-buffer invocation.
 *   @param    - world     : FIRME caller world.
 *   @param    - test_case : Shared-buffer case to exercise.
 *   @param    - limits    : Base-advertised limits.
 *   @param    - out       : Result record.
 *   @param    - evidence  : Returned invocation evidence.
 *   @return   - VAL_SUCCESS when evidence was collected, else VAL_ERROR.
 **/
int firme_shared_buffer_test_collect(
    firme_acs_world_t world,
    val_firme_shared_buffer_case_t test_case,
    const shared_buffer_limits_t *limits,
    firme_rule_res_t *out,
    val_firme_shared_buffer_evidence_t *evidence)
{
    int probe_result = VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    uint32_t required_fields =
        VAL_FIRME_SHARED_BUFFER_EVIDENCE_STATUS |
        VAL_FIRME_SHARED_BUFFER_EVIDENCE_SERVICE;

    if ((limits == 0) || (out == 0) || (evidence == 0))
        return VAL_ERROR;

    *evidence = (val_firme_shared_buffer_evidence_t){0};
    probe_result = val_firme_shared_buffer_probe(
        world, test_case, limits->minimum_size_bytes,
        limits->maximum_page_count, evidence);
    if (probe_result == VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE) {
        shared_buffer_skip(out, world);
        if (evidence->prerequisite_reason != 0ULL) {
            out->last_x0 = evidence->prerequisite_reason;
            out->last_x1 = evidence->prerequisite_detail;
        }
        return VAL_SUCCESS;
    }

    if (probe_result == VAL_FIRME_SHARED_BUFFER_PROBE_FAILED) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        firme_shared_buffer_test_fail(out, SHARED_BUFFER_STEP_PROBE,
                                       (uint64_t)evidence->abi_status, test_case);
        return VAL_ERROR;
    }

    if (probe_result != VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE) {
        out->status = (uint64_t)ACS_STATUS_ERR;
        firme_shared_buffer_test_fail(out, SHARED_BUFFER_STEP_PROBE,
                                      (uint64_t)(int64_t)probe_result,
                                      (uint64_t)test_case);
        return VAL_ERROR;
    }

    if ((evidence->valid_fields & required_fields) != required_fields) {
        out->status = (uint64_t)ACS_STATUS_ERR;
        firme_shared_buffer_test_fail(out, SHARED_BUFFER_STEP_EVIDENCE,
                                      evidence->valid_fields, required_fields);
        return VAL_ERROR;
    }

    if (((evidence->valid_fields & VAL_FIRME_SHARED_BUFFER_EVIDENCE_PAGE_COUNT) != 0U) &&
        (evidence->page_count > FIRME_BASE_MAX_SH_BUF_PG_CNT_MASK)) {
        out->status = (uint64_t)ACS_STATUS_ERR;
        firme_shared_buffer_test_fail(out, SHARED_BUFFER_STEP_EVIDENCE,
                                      evidence->page_count,
                                      FIRME_BASE_MAX_SH_BUF_PG_CNT_MASK);
        return VAL_ERROR;
    }

    if ((evidence->service_id != FIRME_SERVICE_ID_ATTESTATION) &&
        (evidence->service_id != FIRME_SERVICE_ID_INTEGRATED_DEVICE)) {
        out->status = (uint64_t)ACS_STATUS_ERR;
        firme_shared_buffer_test_fail(out, SHARED_BUFFER_STEP_EVIDENCE,
                                      (uint64_t)evidence->service_id,
                                      FIRME_SERVICE_ID_INTEGRATED_DEVICE);
        return VAL_ERROR;
    }

    if (val_firme_service_is_allowed_at_world(
            world, evidence->service_id) == VAL_FALSE) {
        out->status = (uint64_t)ACS_STATUS_ERR;
        firme_shared_buffer_test_fail(out, SHARED_BUFFER_STEP_EVIDENCE,
                                      (uint64_t)evidence->service_id,
                                      (uint64_t)world);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}
