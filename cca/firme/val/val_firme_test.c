/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_test.h"

#include "aarch64/sysreg.h"
#include "val_common/val_status.h"
#include "val_log.h"

/* Feature discovery (RME/GDI discovery via ID registers). */
#define ID_AA64PFR0_EL1_RME_SHIFT 52U
#define ID_AA64PFR0_EL1_RME_MASK  0xFU

#define ID_AA64MMFR4_EL1_GDI_SHIFT 28U
#define ID_AA64MMFR4_EL1_GDI_MASK  0xFU

#define VAL_FIRME_GPI_MAX_CALLS   32U
#define VAL_FIRME_GPT_PGS_SIZE_4K UINT64_C(0x1000)
#define VAL_FIRME_GPT_PGS_SIZE_16K UINT64_C(0x4000)
#define VAL_FIRME_GPT_PGS_SIZE_64K UINT64_C(0x10000)

/**
 *   @brief    - Return the RME feature level from ID_AA64PFR0_EL1.
 *   @return   - RME feature level (0..15).
 **/
uint32_t val_firme_feat_rme_level(void)
{
    uint64_t feature_register = read_id_aa64pfr0_el1();

    return (uint32_t)((feature_register >> ID_AA64PFR0_EL1_RME_SHIFT) &
                      ID_AA64PFR0_EL1_RME_MASK);
}

/**
 *   @brief    - Check whether FEAT_RME is present.
 *   @return   - 1 if present, 0 otherwise.
 **/
uint32_t val_firme_feat_rme_present(void)
{
    return val_firme_feat_rme_level() >= 1U;
}

/**
 *   @brief    - Check whether FEAT_RME_GPC2 is present (TF-A convention).
 *   @return   - 1 if present, 0 otherwise.
 **/
uint32_t val_firme_feat_rme_gpc2_present(void)
{
    /* TF-A convention: RME level >= 2 indicates FEAT_RME_GPC2. */
    return val_firme_feat_rme_level() >= 2U;
}

/**
 *   @brief    - Check whether FEAT_RME_GDI is present.
 *   @return   - 1 if present, 0 otherwise.
 **/
uint32_t val_firme_feat_rme_gdi_present(void)
{
    uint64_t feature_register = read_id_aa64mmfr4_el1();

    return (((feature_register >> ID_AA64MMFR4_EL1_GDI_SHIFT) &
             ID_AA64MMFR4_EL1_GDI_MASK) != 0U);
}

/**
 *   @brief    - Invoke FIRME_GM_GPI_SET with a target GPI attribute.
 *   @param    - base_pa       : Base physical address of the window.
 *   @param    - granule_count : Number of GPT granules in the window.
 *   @param    - target_gpi    : Target GPI (FIRME_GPI_*).
 *   @param    - out           : Optional result record.
 *   @return   - void (SMC status is returned in out->x0).
 **/
void val_firme_gm_gpi_set_do(uint64_t base_pa, uint64_t granule_count,
                             uint64_t target_gpi, firme_res_t *out)
{
    val_firme_gm_gpi_set(base_pa, granule_count,
                         FIRME_GPI_ATTR_TARGET(target_gpi), out);
}

/**
 *   @brief    - Decode the Granule Management service PGS into bytes.
 *   @param    - granule_size : Returned granule size in bytes.
 *   @return   - VAL_SUCCESS for a valid advertised PGS, otherwise VAL_ERROR.
 **/
static int val_firme_gm_get_granule_size(uint64_t *granule_size)
{
    firme_res_t result = {0};
    uint64_t pgs = 0ULL;

    if (!granule_size)
        return VAL_ERROR;

    *granule_size = 0ULL;
    val_firme_service_features(FIRME_SERVICE_ID_GRANULE_MGMT,
                               FIRME_GM_FEATURE_REG1_INDEX, &result);
    if (val_firme_status_to_i64(result.x0) != (int64_t)FIRME_SUCCESS)
        return VAL_ERROR;

    pgs = (result.x1 >> FIRME_GM_FEATURE_REG1_PGS_SHIFT) &
          FIRME_GM_FEATURE_REG1_PGS_MASK;
    switch (pgs) {
    case FIRME_GM_PGS_4KB:
        *granule_size = VAL_FIRME_GPT_PGS_SIZE_4K;
        break;
    case FIRME_GM_PGS_16KB:
        *granule_size = VAL_FIRME_GPT_PGS_SIZE_16K;
        break;
    case FIRME_GM_PGS_64KB:
        *granule_size = VAL_FIRME_GPT_PGS_SIZE_64K;
        break;
    default:
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/**
 *   @brief    - Advance a GPI operation after a valid progress report.
 *   @param    - base_pa      : Current base PA, updated by progress.
 *   @param    - remaining    : Remaining granules, updated by progress.
 *   @param    - total        : Aggregate completed granules, updated by progress.
 *   @param    - progress     : Granules completed by the last invocation.
 *   @param    - granule_size : GPT granule size in bytes.
 *   @return   - VAL_SUCCESS for valid bounded progress, otherwise VAL_ERROR.
 **/
static int val_firme_gm_apply_progress(uint64_t *base_pa, uint64_t *remaining,
                                       uint64_t *total, uint64_t progress,
                                       uint64_t granule_size)
{
    uint64_t offset = 0ULL;

    if (!base_pa || !remaining || !total || progress > *remaining)
        return VAL_ERROR;

    if ((granule_size == 0ULL) || (progress > (UINT64_MAX / granule_size)))
        return VAL_ERROR;

    offset = progress * granule_size;
    if (*base_pa > (UINT64_MAX - offset))
        return VAL_ERROR;

    *base_pa += offset;
    *remaining -= progress;
    *total += progress;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Complete a GPI transition across stateless and stateful phases.
 *   @param    - base_pa       : Base physical address of the transition range.
 *   @param    - granule_count : Number of granules in the transition range.
 *   @param    - target_gpi    : Target GPI (FIRME_GPI_*).
 *   @param    - out           : Optional composite result with aggregate progress.
 *   @return   - VAL_SUCCESS when a terminal DUT result is produced, otherwise VAL_ERROR.
 **/
int val_firme_gm_gpi_set_complete(uint64_t base_pa, uint64_t granule_count,
                                  uint64_t target_gpi, firme_res_t *out)
{
    firme_res_t result = {0};
    uint64_t current_base = base_pa;
    uint64_t remaining = granule_count;
    uint64_t total = 0ULL;
    uint64_t cookie = 0ULL;
    uint64_t granule_size = 0ULL;
    uint32_t call_count = 0U;
    uint32_t use_continue = VAL_FALSE;
    int64_t status = (int64_t)FIRME_INVALID_PARAMETERS;

    if (out)
        *out = result;

    if ((granule_count == 0ULL) ||
        (val_firme_gm_get_granule_size(&granule_size) != VAL_SUCCESS)) {
        LOG_ERROR(NULL, "gpi_set_complete: invalid range or unavailable PGS");
        return VAL_ERROR;
    }

    while ((remaining > 0ULL) && (call_count < VAL_FIRME_GPI_MAX_CALLS)) {
        call_count++;
        if (use_continue == VAL_TRUE) {
            val_firme_gm_gpi_op_continue(cookie, &result);
        } else {
            val_firme_gm_gpi_set_do(current_base, remaining, target_gpi, &result);
        }

        status = val_firme_status_to_i64(result.x0);
        if (status == (int64_t)FIRME_BUSY) {
            if (use_continue == VAL_TRUE)
                cookie = result.x2;
            continue;
        }

        if ((status != (int64_t)FIRME_SUCCESS) &&
            (status != (int64_t)FIRME_DENIED) &&
            (status != (int64_t)FIRME_OP_CONFLICT) &&
            (status != (int64_t)FIRME_NOT_FOUND) &&
            (status != (int64_t)FIRME_INCOMPLETE)) {
            if (out)
                *out = result;
            return VAL_SUCCESS;
        }

        if ((status == (int64_t)FIRME_SUCCESS) && (result.x1 == 0ULL)) {
            LOG_ERROR(NULL, "gpi_set_complete: SUCCESS reported zero progress");
            if (out)
                *out = result;
            return VAL_ERROR;
        }

        if (val_firme_gm_apply_progress(&current_base, &remaining, &total,
                                        result.x1, granule_size) != VAL_SUCCESS) {
            LOG_ERROR(NULL, "gpi_set_complete: invalid progress x1=%llx remaining=%llx",
                      (unsigned long long)result.x1,
                      (unsigned long long)remaining);
            if (out)
                *out = result;
            return VAL_ERROR;
        }

        if (remaining == 0ULL) {
            if (status != (int64_t)FIRME_SUCCESS) {
                LOG_ERROR(NULL, "gpi_set_complete: non-SUCCESS consumed full range");
                if (out)
                    *out = result;
                return VAL_ERROR;
            }

            result.x1 = total;
            if (out)
                *out = result;
            return VAL_SUCCESS;
        }

        if (status == (int64_t)FIRME_INCOMPLETE) {
            cookie = result.x2;
            use_continue = VAL_TRUE;
            continue;
        }

        if ((status == (int64_t)FIRME_SUCCESS) ||
            (status == (int64_t)FIRME_OP_CONFLICT)) {
            use_continue = VAL_FALSE;
            continue;
        }

        result.x1 = total;
        if (out)
            *out = result;
        return VAL_SUCCESS;
    }

    LOG_ERROR(NULL, "gpi_set_complete: operation exceeded %u calls",
              VAL_FIRME_GPI_MAX_CALLS);
    if (out)
        *out = result;
    return VAL_ERROR;
}
