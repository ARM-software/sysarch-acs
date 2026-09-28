/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_firme_rules.h"
#include "val_log.h"
#include "val_firme_suite.h"

#define SERVICE_VERSION_COMPAT_TEST_NAME "service_version_compatibility"
#define SERVICE_VERSION_COMPAT_MAJOR     1U
#define SERVICE_VERSION_COMPAT_STEP_SERVICE_MAJOR UINT64_C(1)
#define SERVICE_VERSION_COMPAT_STEP_BASE_ABIS     UINT64_C(2)

/**
 *   @brief    - Initialize a service-version compatibility result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void service_version_compat_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a service-version compatibility failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void.
 **/
static void service_version_compat_fail(firme_rule_res_t *out, uint64_t fail_step,
                                        uint64_t last_x0, uint64_t last_x1)
{
    if (out) {
        out->status = ACS_STATUS_FAIL;
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Exercise Base 1.0 ABI algorithms against a newer minor revision.
 *   @param    - world : Current caller world; out : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
static int service_version_compat_base(firme_acs_world_t world, firme_rule_res_t *out)
{
    static const firme_rule_id_t baseline_rules[] = {
        FIRME_RULE_R0038, FIRME_RULE_R0051, FIRME_RULE_R0053, FIRME_RULE_R0055
    };
    const firme_test_case_t *test;
    firme_rule_fn_t body;
    firme_rule_res_t result;
    uint32_t index;
    int status;

    for (index = 0; index < sizeof(baseline_rules) / sizeof(baseline_rules[0]); index++) {
        test = val_firme_suite_find(baseline_rules[index]);
        body = 0;
        if (test) {
            if (world == FIRME_ACS_WORLD_NSEL2)
                body = test->run_ns;
            else if (world == FIRME_ACS_WORLD_SECURE)
                body = test->run_secure;
            else if (world == FIRME_ACS_WORLD_REALM)
                body = test->run_realm;
        }

        if (!body) {
            LOG_WARN(NULL, "%s: Base baseline rule=%u is unavailable or registry-gated",
                     SERVICE_VERSION_COMPAT_TEST_NAME, (unsigned int)baseline_rules[index]);
            if (out)
                out->status = ACS_STATUS_SKIP;
            continue;
        }

        result = (firme_rule_res_t){0};
        result.status = ACS_STATUS_ERR;
        status = body(0, &result);
        if (status != VAL_SUCCESS || result.status == ACS_STATUS_FAIL ||
            result.status == ACS_STATUS_ERR) {
            LOG_ERROR(NULL, "%s: Base baseline rule=%u failed at step=%llu",
                      SERVICE_VERSION_COMPAT_TEST_NAME, (unsigned int)baseline_rules[index],
                      (unsigned long long)result.fail_step);
            if (result.status == ACS_STATUS_ERR) {
                if (out)
                    *out = result;
            } else {
                service_version_compat_fail(out, SERVICE_VERSION_COMPAT_STEP_BASE_ABIS,
                                            result.last_x0, result.last_x1);
            }
            return VAL_ERROR;
        }

        if (result.status != ACS_STATUS_PASS && out)
            out->status = ACS_STATUS_SKIP;
    }

    return VAL_SUCCESS;
}

/**
 *   @brief    - Validate available baseline behavior, not just version equality.
 *   @param    - world : Current caller world; out : Required result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
static int service_version_compat_run(firme_acs_world_t world, firme_rule_res_t *out)
{
    firme_res_t res = {0};
    firme_rule_res_t baseline_result = {0};
    uint32_t service_id;
    uint32_t major;
    uint32_t minor;
    uint32_t newer_minor_seen = VAL_FALSE;
    uint32_t incomplete = VAL_FALSE;
    int64_t status;

    if (!out)
        return VAL_ERROR;

    service_version_compat_init_result(out);

    /* Step 1: Discover each service independently; do not equate unrelated major versions. */
    for (service_id = FIRME_SERVICE_ID_BASE; service_id < FIRME_SERVICE_ID_MAX; service_id++) {
        val_firme_service_version(service_id, &res);
        status = val_firme_status_to_i64(res.x0);
        if (status == FIRME_NOT_SUPPORTED && service_id != FIRME_SERVICE_ID_BASE)
            continue;
        if (status <= 0) {
            LOG_ERROR(NULL, "%s: service=%u invalid version status=%lld",
                      SERVICE_VERSION_COMPAT_TEST_NAME, service_id, (long long)status);
            service_version_compat_fail(out, SERVICE_VERSION_COMPAT_STEP_SERVICE_MAJOR,
                                        res.x0, service_id);
            return VAL_ERROR;
        }

        major = FIRME_SERVICE_VERSION_MAJOR(res.x0);
        minor = FIRME_SERVICE_VERSION_MINOR(res.x0);
        if (major != SERVICE_VERSION_COMPAT_MAJOR) {
            LOG_WARN(NULL, "%s: service=%u needs a baseline for major=%u",
                     SERVICE_VERSION_COMPAT_TEST_NAME, service_id, major);
            incomplete = VAL_TRUE;
            continue;
        }

        if (minor == 0)
            continue;

        newer_minor_seen = VAL_TRUE;

        /* Step 2: A newer minor can pass only after its service baseline runs. */
        if (service_id == FIRME_SERVICE_ID_BASE) {
            baseline_result.status = ACS_STATUS_PASS;
            if (service_version_compat_base(world, &baseline_result) != VAL_SUCCESS) {
                if (out)
                    *out = baseline_result;
                return VAL_ERROR;
            }

            if (baseline_result.status == ACS_STATUS_SKIP)
                incomplete = VAL_TRUE;
        } else {
            LOG_WARN(NULL,
                     "%s: service=%u version=%u.%u has no complete 1.0 baseline driver",
                     SERVICE_VERSION_COMPAT_TEST_NAME, service_id, major, minor);
            incomplete = VAL_TRUE;
        }
    }

    /* Step 3: Baseline-only services pass; an untested newer minor must not pass. */
    if (out)
        out->status = incomplete ? ACS_STATUS_SKIP : ACS_STATUS_PASS;
    if (incomplete)
        LOG_WARN(NULL, "%s: backward-compatibility coverage is incomplete",
                 SERVICE_VERSION_COMPAT_TEST_NAME);
    else if (!newer_minor_seen)
        LOG_TEST(NULL, "%s: no newer minor service revision is advertised",
                 SERVICE_VERSION_COMPAT_TEST_NAME);

    return VAL_SUCCESS;
}

/**
 *   @brief    - Service version compatibility rule body for NS caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Required result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_base_service_version_compatibility_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_version_compat_run(FIRME_ACS_WORLD_NSEL2, out);
}

/**
 *   @brief    - Service version compatibility rule body for Secure caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Required result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_base_service_version_compatibility_run_secure(uint64_t base_pa,
                                                        firme_rule_res_t *out)
{
    (void)base_pa;
    return service_version_compat_run(FIRME_ACS_WORLD_SECURE, out);
}

/**
 *   @brief    - Service version compatibility rule body for Realm caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Required result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_base_service_version_compatibility_run_realm(uint64_t base_pa,
                                                       firme_rule_res_t *out)
{
    (void)base_pa;
    return service_version_compat_run(FIRME_ACS_WORLD_REALM, out);
}
