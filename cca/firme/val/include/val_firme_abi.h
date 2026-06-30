/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

/*
 * FIRME ABI FIDs (DEN0149 FIRME specification).
 * These are invoked as SMC calls into EL3.
 */

#define FIRME_FID_SERVICE_VERSION  0xC4000400ULL
#define FIRME_FID_SERVICE_FEATURES 0xC4000401ULL
#define FIRME_FID_GM_GPI_SET       0xC4000402ULL
#define FIRME_FID_MEC_REFRESH      0xC4000407ULL

/* Backward-compatible aliases used by older diagnostics. */
#define FIRME_FID_VERSION  FIRME_FID_SERVICE_VERSION
#define FIRME_FID_FEATURES FIRME_FID_SERVICE_FEATURES

/* FIRME return codes (DEN0149). */
#define FIRME_SUCCESS               0
#define FIRME_NOT_SUPPORTED         -1
#define FIRME_INVALID_PARAMETERS    -2
#define FIRME_ABORTED               -3
#define FIRME_INCOMPLETE            -4
#define FIRME_DENIED                -5
#define FIRME_BUSY                  -6
#define FIRME_OP_CONFLICT           -7
#define FIRME_ALREADY_EXISTS        -8
#define FIRME_NOT_FOUND             -9
#define FIRME_NO_MEMORY             -10
#define FIRME_BAD_DATA              -11

/* Legacy spelling retained by existing GPT retry helpers. */
#define FIRME_RETRY FIRME_BUSY

/* FIRME service IDs (DEN0149 Table 2.2). */
#define FIRME_SERVICE_ID_BASE              0x0U
#define FIRME_SERVICE_ID_GRANULE_MGMT      0x1U
#define FIRME_SERVICE_ID_IDE_KEY_MGMT      0x2U
#define FIRME_SERVICE_ID_MECID_MGMT        0x3U
#define FIRME_SERVICE_ID_ATTESTATION       0x4U
#define FIRME_SERVICE_ID_INTEGRATED_DEVICE 0x5U
#define FIRME_SERVICE_ID_MAX               0x6U
#define FIRME_SERVICE_ID_8BIT_MAX          0xffU

/* FIRME service version encoding. */
#define FIRME_VERSION_MAJOR_SHIFT 16U
#define FIRME_VERSION_MAJOR_MASK  0x7FFFU
#define FIRME_VERSION_MINOR_SHIFT 0U
#define FIRME_VERSION_MINOR_MASK  0xFFFFU
#define FIRME_SERVICE_VERSION_MAJOR(v) \
    (uint32_t)(((uint64_t)(v) >> FIRME_VERSION_MAJOR_SHIFT) & FIRME_VERSION_MAJOR_MASK)
#define FIRME_SERVICE_VERSION_MINOR(v) \
    (uint32_t)(((uint64_t)(v) >> FIRME_VERSION_MINOR_SHIFT) & FIRME_VERSION_MINOR_MASK)

/* FIRME Base service feature register indices. */
#define FIRME_BASE_FEATURE_REG0_INDEX     0U
#define FIRME_BASE_FEATURE_REG1_INDEX     1U
#define FIRME_BASE_FEATURE_REG_COUNT      2U
#define FIRME_FEATURE_REG_INDEX_8BIT_MAX  0xffU

/* FIRME Base service feature register fields. */
#define FIRME_BASE_FEATURE_REG_VERSION_BIT  (UINT64_C(1) << 0)
#define FIRME_BASE_FEATURE_REG_FEATURES_BIT (UINT64_C(1) << 1)
#define FIRME_BASE_FEATURE_REG0_MANDATORY_BITS \
    (FIRME_BASE_FEATURE_REG_VERSION_BIT | FIRME_BASE_FEATURE_REG_FEATURES_BIT)

#define FIRME_BASE_MIN_SH_BUF_SZ_SHIFT         0U
#define FIRME_BASE_MIN_SH_BUF_SZ_MASK          0x3U
#define FIRME_BASE_MIN_SH_BUF_SZ_MAX_ALLOCATED 2U
#define FIRME_BASE_MAX_SH_BUF_PG_CNT_SHIFT 2U
#define FIRME_BASE_MAX_SH_BUF_PG_CNT_MASK  0x3FFFU
#define FIRME_BASE_SERVICE_LIST_SHIFT      16U
#define FIRME_BASE_SERVICE_LIST_MASK       0xFFFFU


/* FIRME_MEC_REFRESH parameter encoding. */
#define FIRME_MEC_REFRESH_REASON_CREATE  UINT64_C(0)
#define FIRME_MEC_REFRESH_REASON_DESTROY UINT64_C(1)
#define FIRME_MEC_REFRESH_REASON_MASK    UINT64_C(1)
#define FIRME_MEC_REFRESH_MECID_SHIFT    32U
#define FIRME_MEC_REFRESH_MECID_WIDTH    16U
#define FIRME_MEC_REFRESH_MECID_MASK     UINT64_C(0xffff)

/* FIRME MECID management service feature register fields. */
#define FIRME_MECID_FEATURE_REG0_INDEX           0U
#define FIRME_MECID_FEATURE_REG0_MEC_REFRESH_BIT (UINT64_C(1) << 0)
#define FIRME_MECID_FEATURE_REG1_INDEX 1U
#define FIRME_MECID_FEATURE_REG1_COMMON_MECID_WIDTH_SHIFT 0U
#define FIRME_MECID_FEATURE_REG1_COMMON_MECID_WIDTH_MASK  0xfU

typedef struct {
    uint64_t x0;
    uint64_t x1;
    uint64_t x2;
    uint64_t x3;
} firme_res_t;

void val_firme_version(firme_res_t *out);
void val_firme_features(uint64_t feature_id, firme_res_t *out);
void val_firme_service_version(uint64_t service_id, firme_res_t *out);
void val_firme_service_features(uint64_t service_id, uint64_t feature_reg_index,
                                firme_res_t *out);
void val_firme_gm_gpi_set(uint64_t base_address, uint64_t granule_count, uint64_t attributes,
                          firme_res_t *out);
void val_firme_mec_refresh(uint64_t mec_params, firme_res_t *out);
int64_t val_firme_status_to_i64(uint64_t value);
int val_firme_status_is_architected(int64_t status);
uint64_t val_firme_base_service_list_bit(uint32_t service_id);
