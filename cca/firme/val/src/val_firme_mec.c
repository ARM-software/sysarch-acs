/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_mec.h"
#include "aarch64/sysreg.h"


/**
 *   @brief    - Read the common MECID width advertised by the DUT.
 *   @param    - out_width : Returned advertised common MECID width in bits.
 *   @param    - out_res   : Raw SERVICE_FEATURES result (optional).
 *   @return   - 0 on success, non-zero otherwise.
 **/
int val_firme_mec_advertised_common_mecid_width(uint32_t *out_width, firme_res_t *out_res)
{
    firme_res_t res = {0};
    uint32_t common_widthm1 = 0U;

    if (!out_width)
        return VAL_ERROR;

    *out_width = 0U;

    val_firme_service_features(FIRME_SERVICE_ID_MECID_MGMT,
                               FIRME_MECID_FEATURE_REG1_INDEX, &res);
    if (out_res)
        *out_res = res;

    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS)
        return VAL_ERROR;

    common_widthm1 =
        (uint32_t)((res.x1 >> FIRME_MECID_FEATURE_REG1_COMMON_MECID_WIDTH_SHIFT) &
                   FIRME_MECID_FEATURE_REG1_COMMON_MECID_WIDTH_MASK);
    *out_width = common_widthm1 + 1U;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Derive the local PE MECID width from MECIDR_EL2.
 *   @param    - out_width : Returned local PE MECID width in bits.
 *   @return   - 0 on success, non-zero otherwise.
 **/
static int val_firme_mec_local_pe_mecid_width(uint32_t *out_width)
{
    uint64_t mecidr = 0ULL;
    uint32_t widthm1 = 0U;
    uint32_t width = 0U;

    if (!out_width)
        return VAL_ERROR;

    *out_width = 0U;
    mecidr = read_mecidr_el2();
    widthm1 = (uint32_t)((mecidr >> VAL_FIRME_MECIDR_WIDTH_SHIFT) &
                         VAL_FIRME_MECIDR_WIDTH_MASK);
    width = widthm1 + 1U;

    if ((width < VAL_FIRME_MECID_WIDTH_MIN) ||
        (width > VAL_FIRME_MECID_WIDTH_MAX)) {
        return VAL_ERROR;
    }

    *out_width = width;
    return VAL_SUCCESS;
}

/**
 *   @brief    - Derive the expected common MECID width from discovered requesters.
 *   @param    - out_width : Returned expected common MECID width in bits.
 *   @return   - 0 on success, non-zero otherwise.
 **/
int val_firme_mec_expected_common_mecid_width(uint32_t *out_width)
{
    uint32_t common_width = 0U;

    if (!out_width)
        return VAL_ERROR;

    *out_width = 0U;

    /*
     * Current FIRME-ACS bring-up executes on one discovered PE, so the common
     * width is the local MECIDR_EL2 width. When MP execution and SMMU discovery
     * are added, extend this helper to compute the minimum width across every
     * discovered PE and MEC-capable requester, following the RME ACS method.
     */
    if (val_firme_mec_local_pe_mecid_width(&common_width) != VAL_SUCCESS)
        return VAL_ERROR;

    *out_width = common_width;
    return VAL_SUCCESS;
}

/**
 *   @brief    - Encode FIRME_MEC_REFRESH MEC params.
 *   @param    - mecid  : MECID value.
 *   @param    - reason : MEC refresh reason.
 *   @return   - Encoded MEC params value.
 **/
uint64_t val_firme_mec_params(uint64_t mecid, uint64_t reason)
{
    uint64_t encoded_mecid = mecid & FIRME_MEC_REFRESH_MECID_MASK;
    uint64_t encoded_reason = reason & FIRME_MEC_REFRESH_REASON_MASK;

    return (encoded_mecid << FIRME_MEC_REFRESH_MECID_SHIFT) | encoded_reason;
}

/**
 *   @brief    - Invoke FIRME_MEC_REFRESH and retry while BUSY is reported.
 *   @param    - mec_params : Encoded MEC params input value.
 *   @param    - out        : Returned FIRME result.
 *   @return   - void
 **/
void val_firme_mec_refresh_do_retry(uint64_t mec_params, firme_res_t *out)
{
    uint32_t attempt = 0U;
    firme_res_t local = {0};

    for (; attempt < VAL_FIRME_MEC_REFRESH_RETRY_LIMIT; attempt++) {
        val_firme_mec_refresh(mec_params, &local);
        if (val_firme_status_to_i64(local.x0) != (int64_t)FIRME_BUSY)
            break;
    }

    if (out)
        *out = local;
}
