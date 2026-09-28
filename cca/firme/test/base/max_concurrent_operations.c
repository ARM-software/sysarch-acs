/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi_catalog.h"
#include "val_firme_service.h"
#include "val_firme_suite.h"
#include "val_log.h"

#define MAX_CONCURRENT_TEST_NAME "max_concurrent_operations"

#define MAX_CONCURRENT_STEP_SERVICE_VERSION_BASE UINT64_C(0x100)
#define MAX_CONCURRENT_STEP_METADATA_BASE        UINT64_C(0x200)
#define MAX_CONCURRENT_STEP_FEATURE_REG1_BASE    UINT64_C(0x300)
#define MAX_CONCURRENT_STEP_INSTANCE_MISMATCH    UINT64_C(0x400)

#define MAX_CONCURRENT_FIELD_WIDTH         16U
#define MAX_CONCURRENT_FIELD_MASK          UINT64_C(0xFFFF)
#define MAX_CONCURRENT_LOW_SERVICE_MAX     4U
#define MAX_CONCURRENT_HIGH_COUNT_SHIFT    0U
#define MAX_CONCURRENT_PRESENT_SHIFT       16U
#define MAX_CONCURRENT_SKIP_STRESS_PENDING UINT64_C(1)

/**
 *   @brief    - Initialize a concurrent-operation rule result.
 *   @param    - out : Result record.
 *   @return   - void
 **/
static void max_concurrent_init_result(firme_rule_res_t *out)
{
    if (out == 0)
        return;

    out->status = (uint64_t)ACS_STATUS_FAIL;
    out->fail_step = 0ULL;
    out->last_x0 = 0ULL;
    out->last_x1 = 0ULL;
}

/**
 *   @brief    - Record a concurrent-operation rule failure.
 *   @param    - out       : Result record.
 *   @param    - fail_step : Failing step.
 *   @param    - last_x0   : First diagnostic value.
 *   @param    - last_x1   : Second diagnostic value.
 *   @return   - void
 **/
static void max_concurrent_fail(firme_rule_res_t *out, uint64_t fail_step,
                                uint64_t last_x0, uint64_t last_x1)
{
    LOG_ERROR(NULL, "%s: step=%llu x0=%llx x1=%llx",
              MAX_CONCURRENT_TEST_NAME, (unsigned long long)fail_step,
              (unsigned long long)last_x0, (unsigned long long)last_x1);

    if (out == 0)
        return;

    out->fail_step = fail_step;
    out->last_x0 = last_x0;
    out->last_x1 = last_x1;
}

/**
 *   @brief    - Pack one service's count and presence into two evidence words.
 *   @param    - service_id  : Non-Base FIRME service ID.
 *   @param    - count       : Advertised maximum concurrent operation count.
 *   @param    - evidence_lo : Counts for services 1 through 4.
 *   @param    - evidence_hi : Service 5 count and service-presence bitmap.
 *   @return   - void
 **/
static void max_concurrent_pack(uint32_t service_id, uint64_t count,
                                uint64_t *evidence_lo, uint64_t *evidence_hi)
{
    uint32_t shift = 0U;

    if ((evidence_lo == 0) || (evidence_hi == 0))
        return;

    if (service_id <= MAX_CONCURRENT_LOW_SERVICE_MAX) {
        shift = (service_id - 1U) * MAX_CONCURRENT_FIELD_WIDTH;
        *evidence_lo |= (count & MAX_CONCURRENT_FIELD_MASK) << shift;
    } else {
        *evidence_hi |=
            (count & MAX_CONCURRENT_FIELD_MASK) << MAX_CONCURRENT_HIGH_COUNT_SHIFT;
    }

    shift = MAX_CONCURRENT_PRESENT_SHIFT + service_id - 1U;
    *evidence_hi |= UINT64_C(1) << shift;
}

/**
 *   @brief    - Return whether evidence reports a service as implemented.
 *   @param    - evidence_hi : Packed high evidence word.
 *   @param    - service_id  : Non-Base FIRME service ID.
 *   @return   - VAL_TRUE when present, otherwise VAL_FALSE.
 **/
static uint32_t max_concurrent_is_present(uint64_t evidence_hi,
                                          uint32_t service_id)
{
    uint32_t shift = MAX_CONCURRENT_PRESENT_SHIFT + service_id - 1U;

    return ((evidence_hi & (UINT64_C(1) << shift)) != 0ULL) ?
           VAL_TRUE : VAL_FALSE;
}

/**
 *   @brief    - Decode a service count from packed evidence.
 *   @param    - evidence_lo : Packed low evidence word.
 *   @param    - evidence_hi : Packed high evidence word.
 *   @param    - service_id  : Non-Base FIRME service ID.
 *   @return   - Advertised maximum concurrent operation count.
 **/
static uint64_t max_concurrent_unpack(uint64_t evidence_lo,
                                      uint64_t evidence_hi,
                                      uint32_t service_id)
{
    uint32_t shift = MAX_CONCURRENT_HIGH_COUNT_SHIFT;

    if (service_id <= MAX_CONCURRENT_LOW_SERVICE_MAX) {
        shift = (service_id - 1U) * MAX_CONCURRENT_FIELD_WIDTH;
        return (evidence_lo >> shift) & MAX_CONCURRENT_FIELD_MASK;
    }

    return (evidence_hi >> shift) & MAX_CONCURRENT_FIELD_MASK;
}

/**
 *   @brief    - Collect implemented-service concurrency fields at one instance.
 *   @param    - out         : Result record.
 *   @param    - evidence_lo : Returned packed low evidence word.
 *   @param    - evidence_hi : Returned packed high evidence word.
 *   @return   - VAL_SUCCESS when all applicable fields are valid, otherwise VAL_ERROR.
 **/
static int max_concurrent_collect(firme_rule_res_t *out,
                                  uint64_t *evidence_lo,
                                  uint64_t *evidence_hi)
{
    const val_firme_feature_register_descriptor_t *descriptor = 0;
    firme_res_t version_res = {0};
    firme_res_t feature_res = {0};
    uint64_t count = 0ULL;
    uint64_t field_mask = 0ULL;
    uint32_t implemented = VAL_FALSE;
    uint32_t service_id = FIRME_SERVICE_ID_GRANULE_MGMT;

    if ((out == 0) || (evidence_lo == 0) || (evidence_hi == 0))
        return VAL_ERROR;

    *evidence_lo = 0ULL;
    *evidence_hi = 0ULL;

    for (; service_id < FIRME_SERVICE_ID_MAX; service_id++) {
        if (val_firme_service_get_version(service_id, &implemented,
                                          &version_res) != VAL_SUCCESS) {
            max_concurrent_fail(
                out, MAX_CONCURRENT_STEP_SERVICE_VERSION_BASE + service_id,
                version_res.x0, (uint64_t)service_id);
            return VAL_ERROR;
        }

        if (implemented == VAL_FALSE)
            continue;

        descriptor = val_firme_feature_register_catalog_get(
            service_id, FIRME_BASE_FEATURE_REG1_INDEX);
        if ((descriptor == 0) ||
            (descriptor->size_bits == 0U) ||
            ((descriptor->size_bits % 64U) != 0U) ||
            (descriptor->max_concurrent_op_width !=
             MAX_CONCURRENT_FIELD_WIDTH)) {
            out->status = (uint64_t)ACS_STATUS_ERR;
            max_concurrent_fail(
                out, MAX_CONCURRENT_STEP_METADATA_BASE + service_id,
                descriptor == 0 ? 0ULL : (uint64_t)descriptor->size_bits,
                descriptor == 0 ?
                    0ULL : (uint64_t)descriptor->max_concurrent_op_width);
            return VAL_ERROR;
        }

        val_firme_service_features(
            (uint64_t)service_id, (uint64_t)descriptor->index, &feature_res);
        if (val_firme_status_to_i64(feature_res.x0) !=
            (int64_t)FIRME_SUCCESS) {
            max_concurrent_fail(
                out, MAX_CONCURRENT_STEP_FEATURE_REG1_BASE + service_id,
                feature_res.x0, (uint64_t)service_id);
            return VAL_ERROR;
        }

        field_mask = (UINT64_C(1) <<
                      descriptor->max_concurrent_op_width) - 1ULL;
        count = (feature_res.x1 >> descriptor->max_concurrent_op_shift) &
                field_mask;
        max_concurrent_pack(service_id, count, evidence_lo, evidence_hi);

        LOG_DEBUG(NULL, "%s: service=%u max=%llu",
                  MAX_CONCURRENT_TEST_NAME, (unsigned int)service_id,
                  (unsigned long long)count);
    }

    return VAL_SUCCESS;
}

/**
 *   @brief    - Validate R0064 at the current FIRME instance.
 *   @param    - base_pa : Unused for Base concept tests.
 *   @param    - out     : Required result record.
 *   @return   - VAL_SUCCESS for PASS, otherwise VAL_ERROR.
 **/
static int max_concurrent_r0064_run(uint64_t base_pa, firme_rule_res_t *out)
{
    uint64_t evidence_lo = 0ULL;
    uint64_t evidence_hi = 0ULL;
    int result = VAL_ERROR;

    (void)base_pa;
    max_concurrent_init_result(out);
    result = max_concurrent_collect(out, &evidence_lo, &evidence_hi);
    if (result != VAL_SUCCESS)
        return VAL_ERROR;
    if ((out == 0) ||
        (out->status != (uint64_t)ACS_STATUS_FAIL) ||
        (out->fail_step != 0ULL))
        return VAL_ERROR;

    out->status = (uint64_t)ACS_STATUS_PASS;
    return VAL_SUCCESS;
}

/**
 *   @brief    - Collect R0065 evidence at the current FIRME instance.
 *   @param    - base_pa : Unused for Base concept tests.
 *   @param    - out     : Required result record.
 *   @return   - VAL_SUCCESS for PASS, otherwise VAL_ERROR.
 **/
static int max_concurrent_r0065_run(uint64_t base_pa, firme_rule_res_t *out)
{
    uint64_t evidence_lo = 0ULL;
    uint64_t evidence_hi = 0ULL;
    int result = VAL_ERROR;

    (void)base_pa;
    max_concurrent_init_result(out);
    result = max_concurrent_collect(out, &evidence_lo, &evidence_hi);
    if (result != VAL_SUCCESS)
        return VAL_ERROR;
    if ((out == 0) ||
        (out->status != (uint64_t)ACS_STATUS_FAIL) ||
        (out->fail_step != 0ULL))
        return VAL_ERROR;

    out->status = (uint64_t)ACS_STATUS_PASS;
    out->fail_step = evidence_lo;
    out->last_x0 = evidence_hi;
    out->last_x1 = 0ULL;
    return VAL_SUCCESS;
}

/**
 *   @brief    - R0064 body for the NS FIRME instance.
 *   @param    - base_pa : Unused.
 *   @param    - out     : Required result record.
 *   @return   - VAL_SUCCESS for PASS, otherwise VAL_ERROR.
 **/
int firme_base_max_concurrent_r0064_run_ns(uint64_t base_pa,
                                           firme_rule_res_t *out)
{
    return max_concurrent_r0064_run(base_pa, out);
}

/**
 *   @brief    - R0064 body for the Secure FIRME instance.
 *   @param    - base_pa : Unused.
 *   @param    - out     : Required result record.
 *   @return   - VAL_SUCCESS for PASS, otherwise VAL_ERROR.
 **/
int firme_base_max_concurrent_r0064_run_secure(uint64_t base_pa,
                                               firme_rule_res_t *out)
{
    return max_concurrent_r0064_run(base_pa, out);
}

/**
 *   @brief    - R0064 body for the Realm FIRME instance.
 *   @param    - base_pa : Unused.
 *   @param    - out     : Required result record.
 *   @return   - VAL_SUCCESS for PASS, otherwise VAL_ERROR.
 **/
int firme_base_max_concurrent_r0064_run_realm(uint64_t base_pa,
                                              firme_rule_res_t *out)
{
    return max_concurrent_r0064_run(base_pa, out);
}

/**
 *   @brief    - R0065 evidence body for the NS FIRME instance.
 *   @param    - base_pa : Unused.
 *   @param    - out     : Required result record.
 *   @return   - VAL_SUCCESS for PASS, otherwise VAL_ERROR.
 **/
int firme_base_max_concurrent_r0065_run_ns(uint64_t base_pa,
                                           firme_rule_res_t *out)
{
    return max_concurrent_r0065_run(base_pa, out);
}

/**
 *   @brief    - R0065 evidence body for the Secure FIRME instance.
 *   @param    - base_pa : Unused.
 *   @param    - out     : Required result record.
 *   @return   - VAL_SUCCESS for PASS, otherwise VAL_ERROR.
 **/
int firme_base_max_concurrent_r0065_run_secure(uint64_t base_pa,
                                               firme_rule_res_t *out)
{
    return max_concurrent_r0065_run(base_pa, out);
}

/**
 *   @brief    - R0065 evidence body for the Realm FIRME instance.
 *   @param    - base_pa : Unused.
 *   @param    - out     : Required result record.
 *   @return   - VAL_SUCCESS for PASS, otherwise VAL_ERROR.
 **/
int firme_base_max_concurrent_r0065_run_realm(uint64_t base_pa,
                                              firme_rule_res_t *out)
{
    return max_concurrent_r0065_run(base_pa, out);
}

/**
 *   @brief    - Compare R0065 evidence across implemented FIRME instances.
 *   @param    - res_ns     : NS instance result and evidence.
 *   @param    - res_secure : Secure instance result and evidence.
 *   @param    - res_realm  : Realm instance result and evidence.
 *   @param    - overall    : Combined rule result to update.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_base_max_concurrent_r0065_post(
    const firme_acs_call_res_t *res_ns,
    const firme_acs_call_res_t *res_secure,
    const firme_acs_call_res_t *res_realm,
    firme_acs_call_res_t *overall)
{
    const firme_acs_call_res_t *results[3] = {
        res_ns, res_secure, res_realm
    };
    const char *worlds[3] = {"NS", "Secure", "Realm"};
    uint64_t count = 0ULL;
    uint64_t reference_count = 0ULL;
    uint64_t nonzero_service_mask = 0ULL;
    uint64_t first_nonzero_count = 0ULL;
    uint32_t have_reference = VAL_FALSE;
    uint32_t service_id = FIRME_SERVICE_ID_GRANULE_MGMT;
    uint32_t world_index = 0U;

    if (overall == 0)
        return VAL_ERROR;

    if ((res_ns == 0) || (res_secure == 0) || (res_realm == 0)) {
        *overall = (firme_acs_call_res_t){.x0 = ACS_STATUS_ERR};
        return VAL_ERROR;
    }

    for (; service_id < FIRME_SERVICE_ID_MAX; service_id++) {
        have_reference = VAL_FALSE;
        reference_count = 0ULL;

        for (world_index = 0U; world_index < 3U; world_index++) {
            if (results[world_index]->x0 != (uint64_t)ACS_STATUS_PASS)
                continue;

            if (max_concurrent_is_present(
                    results[world_index]->x2, service_id) == VAL_FALSE)
                continue;

            count = max_concurrent_unpack(
                results[world_index]->x1,
                results[world_index]->x2,
                service_id);
            if (have_reference == VAL_FALSE) {
                reference_count = count;
                have_reference = VAL_TRUE;
            } else if (count != reference_count) {
                LOG_ERROR(NULL,
                          "%s: service=%u %s count=%llu expected=%llu",
                          MAX_CONCURRENT_TEST_NAME,
                          (unsigned int)service_id, worlds[world_index],
                          (unsigned long long)count,
                          (unsigned long long)reference_count);
                overall->x0 = (uint64_t)ACS_STATUS_FAIL;
                overall->x1 =
                    MAX_CONCURRENT_STEP_INSTANCE_MISMATCH + service_id;
                overall->x2 = reference_count;
                overall->x3 = count;
                overall->x4 = (uint64_t)world_index;
                return VAL_ERROR;
            }

            if (count != 0ULL) {
                nonzero_service_mask |= UINT64_C(1) << service_id;
                if (first_nonzero_count == 0ULL)
                    first_nonzero_count = count;
            }
        }
    }

    if (nonzero_service_mask != 0ULL) {
        LOG_WARN(NULL,
                 "%s: collective non-zero limit stress needs retained operations",
                 MAX_CONCURRENT_TEST_NAME);
        overall->x0 = (uint64_t)ACS_STATUS_SKIP;
        overall->x1 = MAX_CONCURRENT_SKIP_STRESS_PENDING;
        overall->x2 = nonzero_service_mask;
        overall->x3 = first_nonzero_count;
        overall->x4 = 0ULL;
        return VAL_SUCCESS;
    }

    overall->x0 = (uint64_t)ACS_STATUS_PASS;
    overall->x1 = 0ULL;
    overall->x2 = 0ULL;
    overall->x3 = 0ULL;
    overall->x4 = 0ULL;
    return VAL_SUCCESS;
}
