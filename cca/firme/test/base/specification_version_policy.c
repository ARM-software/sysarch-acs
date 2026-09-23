/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_firme_rules.h"
#include "val_firme_spec_policy.h"
#include "val_log.h"

#define SPEC_POLICY_TEST_NAME "specification_version_policy"

#define SPEC_POLICY_STEP_PROBE       UINT64_C(1)
#define SPEC_POLICY_STEP_METADATA    UINT64_C(2)
#define SPEC_POLICY_STEP_REGRESSION  UINT64_C(3)
#define SPEC_POLICY_STEP_MAJOR       UINT64_C(4)
#define SPEC_POLICY_STEP_MINOR       UINT64_C(5)

#define SPEC_POLICY_CHANGE_MINOR UINT32_C(1)
#define SPEC_POLICY_CHANGE_MAJOR UINT32_C(2)

/**
 *   @brief    - Initialize the specification-policy result record.
 *   @param    - out : Result record.
 *   @return   - void.
 **/
static void specification_policy_init_result(firme_rule_res_t *out)
{
    if (out == 0)
        return;

    out->status = (uint64_t)ACS_STATUS_FAIL;
    out->fail_step = 0ULL;
    out->last_x0 = 0ULL;
    out->last_x1 = 0ULL;
}

/**
 *   @brief    - Record a specification-policy validation failure.
 *   @param    - out       : Result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : First diagnostic value.
 *   @param    - last_x1   : Second diagnostic value.
 *   @return   - void.
 **/
static void specification_policy_fail(firme_rule_res_t *out,
                                      uint64_t fail_step,
                                      uint64_t last_x0,
                                      uint64_t last_x1)
{
    LOG_ERROR(NULL, "%s: step=%llu x0=%llx x1=%llx",
              SPEC_POLICY_TEST_NAME, (unsigned long long)fail_step,
              (unsigned long long)last_x0, (unsigned long long)last_x1);

    if (out == 0)
        return;

    out->fail_step = fail_step;
    out->last_x0 = last_x0;
    out->last_x1 = last_x1;
}

/**
 *   @brief    - Find one service in specification release metadata.
 *   @param    - release    : Release metadata to search.
 *   @param    - service_id : Service ID to find.
 *   @return   - Service metadata, or NULL when absent.
 **/
static const val_firme_spec_policy_service_t *specification_policy_find_service(
    const val_firme_spec_policy_release_t *release, uint32_t service_id)
{
    uint32_t index = 0U;

    if (release == 0)
        return 0;

    for (index = 0U; index < release->service_count; index++) {
        if (release->services[index].service_id == service_id)
            return &release->services[index];
    }

    return 0;
}

/**
 *   @brief    - Validate one specification release-metadata record.
 *   @param    - release : Release metadata to validate.
 *   @return   - VAL_TRUE when the record is valid, otherwise VAL_FALSE.
 **/
static uint32_t specification_policy_release_valid(
    const val_firme_spec_policy_release_t *release)
{
    uint32_t index = 0U;
    uint32_t next_index = 0U;

    if ((release == 0) ||
        (release->service_count > VAL_FIRME_SPEC_POLICY_MAX_SERVICES))
        return VAL_FALSE;

    for (index = 0U; index < release->service_count; index++) {
        if (release->services[index].service_id >= FIRME_SERVICE_ID_MAX)
            return VAL_FALSE;

        for (next_index = index + 1U;
             next_index < release->service_count; next_index++) {
            if (release->services[index].service_id ==
                release->services[next_index].service_id)
                return VAL_FALSE;
        }
    }

    return VAL_TRUE;
}

/**
 *   @brief    - Validate ALP4 R0031 specification version policy.
 *   @param    - out : Result record.
 *   @return   - VAL_SUCCESS when the rule produced an outcome, else VAL_ERROR.
 **/
static int specification_version_policy_run(firme_rule_res_t *out)
{
    val_firme_spec_policy_evidence_t evidence = {0};
    const val_firme_spec_policy_service_t *previous_service = 0;
    const val_firme_spec_policy_service_t *current_service = 0;
    uint32_t change_mask = 0U;
    uint32_t index = 0U;
    int probe_result = VAL_FIRME_SPEC_POLICY_PROBE_ERROR;

    if (out == 0)
        return VAL_ERROR;

    specification_policy_init_result(out);
    probe_result = val_firme_spec_policy_probe(&evidence);
    if (probe_result == VAL_FIRME_SPEC_POLICY_PROBE_UNAVAILABLE) {
        out->status = (uint64_t)ACS_STATUS_SKIP;
        out->fail_step = SPEC_POLICY_STEP_PROBE;
        return VAL_SUCCESS;
    }

    if (probe_result != VAL_FIRME_SPEC_POLICY_PROBE_AVAILABLE) {
        out->status = (uint64_t)ACS_STATUS_ERR;
        specification_policy_fail(out, SPEC_POLICY_STEP_PROBE,
                                  (uint64_t)(int64_t)probe_result, 0ULL);
        return VAL_ERROR;
    }

    if ((specification_policy_release_valid(&evidence.previous) == VAL_FALSE) ||
        (specification_policy_release_valid(&evidence.current) == VAL_FALSE)) {
        out->status = (uint64_t)ACS_STATUS_ERR;
        specification_policy_fail(out, SPEC_POLICY_STEP_METADATA,
                                  evidence.previous.service_count,
                                  evidence.current.service_count);
        return VAL_ERROR;
    }

    if ((evidence.current.major_version < evidence.previous.major_version) ||
        ((evidence.current.major_version == evidence.previous.major_version) &&
         (evidence.current.minor_version < evidence.previous.minor_version))) {
        specification_policy_fail(out, SPEC_POLICY_STEP_REGRESSION,
                                  evidence.previous.major_version,
                                  evidence.current.major_version);
        return VAL_SUCCESS;
    }

    for (index = 0U; index < evidence.previous.service_count; index++) {
        previous_service = &evidence.previous.services[index];
        current_service = specification_policy_find_service(
            &evidence.current, previous_service->service_id);
        if (current_service == 0) {
            change_mask |= SPEC_POLICY_CHANGE_MAJOR;
            continue;
        }

        if (current_service->major_version < previous_service->major_version) {
            specification_policy_fail(out, SPEC_POLICY_STEP_REGRESSION,
                                      previous_service->service_id,
                                      current_service->major_version);
            return VAL_SUCCESS;
        }

        if (current_service->major_version > previous_service->major_version) {
            change_mask |= SPEC_POLICY_CHANGE_MAJOR;
            continue;
        }

        if (current_service->minor_version < previous_service->minor_version) {
            specification_policy_fail(out, SPEC_POLICY_STEP_REGRESSION,
                                      previous_service->service_id,
                                      current_service->minor_version);
            return VAL_SUCCESS;
        }

        if (current_service->minor_version > previous_service->minor_version)
            change_mask |= SPEC_POLICY_CHANGE_MINOR;
    }

    for (index = 0U; index < evidence.current.service_count; index++) {
        current_service = &evidence.current.services[index];
        previous_service = specification_policy_find_service(
            &evidence.previous, current_service->service_id);
        if (previous_service == 0)
            change_mask |= SPEC_POLICY_CHANGE_MINOR;
    }

    if (((change_mask & SPEC_POLICY_CHANGE_MAJOR) != 0U) &&
        (evidence.current.major_version <= evidence.previous.major_version)) {
        specification_policy_fail(out, SPEC_POLICY_STEP_MAJOR,
                                  evidence.previous.major_version,
                                  evidence.current.major_version);
        return VAL_SUCCESS;
    }

    if (((change_mask & SPEC_POLICY_CHANGE_MINOR) != 0U) &&
        (evidence.current.major_version == evidence.previous.major_version) &&
        (evidence.current.minor_version <= evidence.previous.minor_version)) {
        specification_policy_fail(out, SPEC_POLICY_STEP_MINOR,
                                  evidence.previous.minor_version,
                                  evidence.current.minor_version);
        return VAL_SUCCESS;
    }

    out->status = (uint64_t)ACS_STATUS_PASS;
    out->last_x0 = (uint64_t)change_mask;
    return VAL_SUCCESS;
}

/**
 *   @brief    - Execute R0031 using release metadata supplied to the NS payload.
 *   @param    - base_pa : Unused for specification-policy validation.
 *   @param    - out     : Result record.
 *   @return   - VAL_SUCCESS when the rule produced an outcome, else VAL_ERROR.
 **/
int firme_base_specification_version_policy_run_ns(
    uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return specification_version_policy_run(out);
}
