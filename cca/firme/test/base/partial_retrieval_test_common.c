/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "partial_retrieval_test_common.h"

#include "val_firme_abi.h"
#include "val_firme_service.h"
#include "val_log.h"

#define PARTIAL_RETRIEVAL_TEST_NAME "partial_retrieval"

/**
 * @brief Supply the NS/Realm PAT adapter's Table 8.26 status/size-validity contract.
 * @param world Caller world.
 * @param test_case Requested chunk scenario.
 * @param evidence Selected service and completion evidence.
 * @param expected_status Specification-derived status, independent of observation.
 * @return PROBE_AVAILABLE for supported PAT cases, UNAVAILABLE otherwise, or ERROR.
 */
__attribute__((weak)) int firme_partial_retrieval_test_expected_status(
    firme_acs_world_t world, val_firme_partial_retrieval_case_t test_case,
    const val_firme_partial_retrieval_evidence_t *evidence,
    int64_t *expected_status)
{
    if ((evidence == 0) || (expected_status == 0))
        return VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_ERROR;
    if (((world == FIRME_ACS_WORLD_NSEL2) || (world == FIRME_ACS_WORLD_REALM)) &&
        (evidence->service_id == FIRME_SERVICE_ID_ATTESTATION) &&
        (evidence->abi_fid == FIRME_FID_ATTEST_PAT_GET) &&
        ((test_case == VAL_FIRME_PARTIAL_RETRIEVAL_INTERMEDIATE_CHUNK) ||
         (test_case == VAL_FIRME_PARTIAL_RETRIEVAL_NO_CHUNK))) {
        *expected_status = (int64_t)FIRME_INCOMPLETE;
        return VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_AVAILABLE;
    }
    return VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_UNAVAILABLE;
}

/**
 *   @brief    - Initialize a partial-retrieval rule result.
 *   @param    - out : Result record.
 *   @return   - void.
 **/
static void partial_retrieval_init_result(firme_rule_res_t *out)
{
    if (out == 0)
        return;

    out->status = (uint64_t)ACS_STATUS_FAIL;
    out->fail_step = 0ULL;
    out->last_x0 = 0ULL;
    out->last_x1 = 0ULL;
}

/**
 *   @brief    - Record a partial-retrieval rule failure.
 *   @param    - out       : Result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : First diagnostic value.
 *   @param    - last_x1   : Second diagnostic value.
 *   @return   - void.
 **/
void firme_partial_retrieval_test_fail(firme_rule_res_t *out,
                                       uint64_t fail_step,
                                       uint64_t last_x0,
                                       uint64_t last_x1)
{
    LOG_ERROR(NULL, "%s: step=%llu x0=%llx x1=%llx",
              PARTIAL_RETRIEVAL_TEST_NAME, (unsigned long long)fail_step,
              (unsigned long long)last_x0, (unsigned long long)last_x1);

    if (out == 0)
        return;

    out->fail_step = fail_step;
    out->last_x0 = last_x0;
    out->last_x1 = last_x1;
}

/**
 *   @brief    - Record that no partial-retrieval service adapter is available.
 *   @param    - world : FIRME caller world.
 *   @param    - out   : Result record.
 *   @return   - void.
 **/
static void partial_retrieval_skip(firme_acs_world_t world,
                                   firme_rule_res_t *out)
{
    LOG_WARN(NULL, "%s: service, resources or requested chunk evidence unavailable world=%u",
             PARTIAL_RETRIEVAL_TEST_NAME, (unsigned int)world);

    if (out == 0)
        return;

    out->status = (uint64_t)ACS_STATUS_SKIP;
    out->fail_step = PARTIAL_RETRIEVAL_STEP_PROBE_UNAVAILABLE;
    out->last_x0 = (uint64_t)world;
    out->last_x1 = 0ULL;
}

/**
 *   @brief    - Prepare a partial-retrieval rule at one FIRME instance.
 *   @param    - world : FIRME caller world.
 *   @param    - out   : Result record.
 *   @return   - VAL_SUCCESS for usable setup or SKIP, otherwise VAL_ERROR.
 **/
int firme_partial_retrieval_test_prepare(firme_acs_world_t world,
                                         firme_rule_res_t *out)
{
    if (out == 0)
        return VAL_ERROR;

    partial_retrieval_init_result(out);
    if (val_firme_partial_retrieval_probe_available(world) == VAL_FALSE) {
        partial_retrieval_skip(world, out);
        return VAL_SUCCESS;
    }

    return VAL_SUCCESS;
}

/**
 *   @brief    - Collect evidence from one partial-retrieval invocation.
 *   @param    - world     : FIRME caller world.
 *   @param    - test_case : Retrieval scenario to exercise.
 *   @param    - out       : Result record.
 *   @param    - evidence  : Returned ABI and size evidence.
 *   @return   - VAL_SUCCESS when evidence was collected, else VAL_ERROR.
 **/
int firme_partial_retrieval_test_collect(
    firme_acs_world_t world,
    val_firme_partial_retrieval_case_t test_case,
    firme_rule_res_t *out,
    val_firme_partial_retrieval_evidence_t *evidence)
{
    uint32_t required_fields =
        VAL_FIRME_PARTIAL_RETRIEVAL_EVIDENCE_STATUS |
        VAL_FIRME_PARTIAL_RETRIEVAL_EVIDENCE_WRITTEN |
        VAL_FIRME_PARTIAL_RETRIEVAL_EVIDENCE_REMAINING |
        VAL_FIRME_PARTIAL_RETRIEVAL_EVIDENCE_SERVICE;
    int probe_result = VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_ERROR;
    int64_t expected_status = INT64_MAX;

    if ((out == 0) || (evidence == 0))
        return VAL_ERROR;

    *evidence = (val_firme_partial_retrieval_evidence_t){0};
    probe_result = val_firme_partial_retrieval_probe(world, test_case,
                                                      evidence);
    if (probe_result == VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_UNAVAILABLE) {
        if (((world == FIRME_ACS_WORLD_NSEL2) || (world == FIRME_ACS_WORLD_REALM)) &&
            (test_case == VAL_FIRME_PARTIAL_RETRIEVAL_LAST_CHUNK))
            LOG_WARN(NULL, "%s: PAT SUCCESS has no Remaining output in Table 8.26",
                     PARTIAL_RETRIEVAL_TEST_NAME);
        partial_retrieval_skip(world, out);
        if (evidence->prerequisite_reason != 0ULL) {
            out->last_x0 = evidence->prerequisite_reason;
            out->last_x1 = evidence->prerequisite_detail;
        }
        return VAL_SUCCESS;
    }

    if (probe_result == VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_FAILED) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        firme_partial_retrieval_test_fail(out, PARTIAL_RETRIEVAL_STEP_PROBE,
                                           (uint64_t)evidence->abi_status, test_case);
        return VAL_ERROR;
    }

    if (probe_result != VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_AVAILABLE) {
        out->status = (uint64_t)ACS_STATUS_ERR;
        firme_partial_retrieval_test_fail(
            out, PARTIAL_RETRIEVAL_STEP_PROBE,
            (uint64_t)(int64_t)probe_result, (uint64_t)test_case);
        return VAL_ERROR;
    }

    if ((evidence->valid_fields & required_fields) != required_fields) {
        out->status = (uint64_t)ACS_STATUS_ERR;
        firme_partial_retrieval_test_fail(
            out, PARTIAL_RETRIEVAL_STEP_EVIDENCE,
            evidence->valid_fields, required_fields);
        return VAL_ERROR;
    }

    if ((evidence->service_id != FIRME_SERVICE_ID_ATTESTATION) &&
        (evidence->service_id != FIRME_SERVICE_ID_INTEGRATED_DEVICE)) {
        out->status = (uint64_t)ACS_STATUS_ERR;
        firme_partial_retrieval_test_fail(
            out, PARTIAL_RETRIEVAL_STEP_EVIDENCE,
            evidence->service_id, FIRME_SERVICE_ID_INTEGRATED_DEVICE);
        return VAL_ERROR;
    }

    if (val_firme_service_is_allowed_at_world(
            world, evidence->service_id) == VAL_FALSE) {
        out->status = (uint64_t)ACS_STATUS_ERR;
        firme_partial_retrieval_test_fail(
            out, PARTIAL_RETRIEVAL_STEP_EVIDENCE,
            evidence->service_id, (uint64_t)world);
        return VAL_ERROR;
    }

    probe_result = firme_partial_retrieval_test_expected_status(
        world, test_case, evidence, &expected_status);
    if (probe_result == VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_UNAVAILABLE) {
        LOG_WARN(NULL, "%s: selected ABI has no status/size-validity contract",
                 PARTIAL_RETRIEVAL_TEST_NAME);
        out->status = (uint64_t)ACS_STATUS_SKIP;
        out->fail_step = PARTIAL_RETRIEVAL_STEP_STATUS;
        return VAL_SUCCESS;
    }

    if ((probe_result != VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_AVAILABLE) ||
        (val_firme_status_is_architected(expected_status) != VAL_TRUE)) {
        out->status = (uint64_t)ACS_STATUS_ERR;
        firme_partial_retrieval_test_fail(
            out, PARTIAL_RETRIEVAL_STEP_EVIDENCE,
            (uint64_t)expected_status, (uint64_t)(int64_t)probe_result);
        return VAL_ERROR;
    }

    if (evidence->abi_status != expected_status) {
        firme_partial_retrieval_test_fail(
            out, PARTIAL_RETRIEVAL_STEP_STATUS,
            (uint64_t)evidence->abi_status, (uint64_t)expected_status);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}
