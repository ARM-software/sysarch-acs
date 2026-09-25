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

#define FIRME_FID_SERVICE_VERSION             0xC4000400ULL
#define FIRME_FID_SERVICE_FEATURES            0xC4000401ULL
#define FIRME_FID_GM_GPI_SET                  0xC4000402ULL
#define FIRME_FID_IDE_KEYSET_PROG             0xC4000403ULL
#define FIRME_FID_IDE_KEYSET_GO               0xC4000404ULL
#define FIRME_FID_IDE_KEYSET_STOP             0xC4000405ULL
#define FIRME_FID_IDE_KEYSET_POLL             0xC4000406ULL
#define FIRME_FID_MEC_REFRESH                 0xC4000407ULL
#define FIRME_FID_ATTEST_PAT_GET              0xC4000408ULL
#define FIRME_FID_ATTEST_RAK_GET              0xC4000409ULL
#define FIRME_FID_ATTEST_RAT_SIGN             0xC400040AULL
#define FIRME_FID_ATTEST_PAT_EXT_CLAIMS_STAGE 0xC400040BULL
#define FIRME_FID_ATTEST_PAT_EXT_CLAIMS_CLEAR 0xC400040CULL
#define FIRME_FID_ATTEST_PAT_EXT_FINALISE     0xC400040DULL
#define FIRME_FID_GM_L1_GPT_CREATE            0xC400040EULL
#define FIRME_FID_GM_L1_GPT_DESTROY           0xC400040FULL
#define FIRME_FID_IDEV_OP_START               0xC4000410ULL
#define FIRME_FID_IDEV_OP_CONTINUE            0xC4000411ULL
#define FIRME_FID_GM_GPI_OP_CONTINUE          0xC4000412ULL
#define FIRME_FID_GM_GPT_INFO                 0xC4000413ULL

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
#define FIRME_SERVICE_ID_INPUT_MASK        UINT64_C(0xFF)
#define FIRME_FEATURE_REG_INDEX_INPUT_MASK UINT64_C(0xFF)

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
#define FIRME_BASE_SERVICE_LIST_ALLOCATED_MASK \
    ((UINT64_C(1) << (FIRME_SERVICE_ID_MAX - 1U)) - 1ULL)

/* Service feature-register 0 ABI-presence bits. */
#define FIRME_GM_FEATURE_REG0_INDEX           0U
#define FIRME_GM_FEATURE_GPI_SET_BIT           (UINT64_C(1) << 0)
#define FIRME_GM_FEATURE_GPI_CONTINUE_BIT      (UINT64_C(1) << 1)
#define FIRME_GM_FEATURE_L1_CREATE_BIT         (UINT64_C(1) << 2)
#define FIRME_GM_FEATURE_L1_DESTROY_BIT        (UINT64_C(1) << 3)
#define FIRME_GM_FEATURE_GPT_INFO_BIT          (UINT64_C(1) << 4)
#define FIRME_GM_FEATURE_REG0_GPI_SET_BIT      FIRME_GM_FEATURE_GPI_SET_BIT
#define FIRME_GM_FEATURE_REG0_OP_CONTINUE_BIT  FIRME_GM_FEATURE_GPI_CONTINUE_BIT
#define FIRME_GM_FEATURE_REG0_L1_CREATE_BIT    FIRME_GM_FEATURE_L1_CREATE_BIT
#define FIRME_GM_FEATURE_REG0_L1_DESTROY_BIT   FIRME_GM_FEATURE_L1_DESTROY_BIT
#define FIRME_GM_FEATURE_REG0_GPT_INFO_BIT     FIRME_GM_FEATURE_GPT_INFO_BIT
#define FIRME_GM_FEATURE_REG0_ALLOCATED_BITS \
    (FIRME_GM_FEATURE_GPI_SET_BIT | FIRME_GM_FEATURE_GPI_CONTINUE_BIT | \
     FIRME_GM_FEATURE_L1_CREATE_BIT | FIRME_GM_FEATURE_L1_DESTROY_BIT | \
     FIRME_GM_FEATURE_GPT_INFO_BIT)
#define FIRME_GM_GPI_SET_ATTRIBUTES_MASK UINT64_C(0xf)
#define FIRME_GM_FEATURE_REG1_INDEX         1U
#define FIRME_GM_FEATURE_REG1_PGS_SHIFT     0U
#define FIRME_GM_FEATURE_REG1_PGS_MASK      0x3U
#define FIRME_GM_FEATURE_REG1_L0GPTSZ_SHIFT 2U
#define FIRME_GM_FEATURE_REG1_L0GPTSZ_MASK  0xfU
#define FIRME_GM_FEATURE_REG1_PPS_SHIFT     6U
#define FIRME_GM_FEATURE_REG1_PPS_MASK      0x7U

/* GPCCR_EL3.L0GPTSZ encodings surfaced via GM feature register 1. */
#define FIRME_GM_L0GPTSZ_30BITS 0U
#define FIRME_GM_L0GPTSZ_34BITS 4U
#define FIRME_GM_L0GPTSZ_36BITS 6U
#define FIRME_GM_L0GPTSZ_39BITS 9U

/* GPCCR_EL3.PPS encodings surfaced via GM feature register 1. */
#define FIRME_GM_PPS_4GB   0U
#define FIRME_GM_PPS_64GB  1U
#define FIRME_GM_PPS_1TB   2U
#define FIRME_GM_PPS_4TB   3U
#define FIRME_GM_PPS_16TB  4U
#define FIRME_GM_PPS_256TB 5U
#define FIRME_GM_PPS_4PB   6U

/* FIRME_GM_GPT_INFO PAR state encodings. */
#define FIRME_GPT_PAR_STATE_EL3_BLOCK  UINT64_C(0)
#define FIRME_GPT_PAR_STATE_EL3_TABLE  UINT64_C(1)
#define FIRME_GPT_PAR_STATE_HOST_BLOCK UINT64_C(2)
#define FIRME_GPT_PAR_STATE_HOST_TABLE UINT64_C(3)
#define FIRME_GPT_PAR_STATE_MAX        FIRME_GPT_PAR_STATE_HOST_TABLE
#define FIRME_GPT_PAR_STATE_OWNER_MASK UINT64_C(2)

/* GPCCR_EL3.PGS encodings surfaced via GM feature register 1. */
#define FIRME_GM_PGS_4KB  0U
#define FIRME_GM_PGS_64KB 1U
#define FIRME_GM_PGS_16KB 2U

#define FIRME_IDE_FEATURE_REG0_INDEX           0U
#define FIRME_IDE_FEATURE_KEYSET_PROG_BIT      (UINT64_C(1) << 0)
#define FIRME_IDE_FEATURE_KEYSET_GO_BIT        (UINT64_C(1) << 1)
#define FIRME_IDE_FEATURE_KEYSET_STOP_BIT      (UINT64_C(1) << 2)
#define FIRME_IDE_FEATURE_KEYSET_POLL_BIT      (UINT64_C(1) << 3)
#define FIRME_IDE_FEATURE_REG0_PROG_BIT        FIRME_IDE_FEATURE_KEYSET_PROG_BIT
#define FIRME_IDE_FEATURE_REG0_GO_BIT          FIRME_IDE_FEATURE_KEYSET_GO_BIT
#define FIRME_IDE_FEATURE_REG0_STOP_BIT        FIRME_IDE_FEATURE_KEYSET_STOP_BIT
#define FIRME_IDE_FEATURE_REG0_POLL_BIT        FIRME_IDE_FEATURE_KEYSET_POLL_BIT
#define FIRME_IDE_FEATURE_REG0_ALL_ABI_BITS \
    (FIRME_IDE_FEATURE_KEYSET_PROG_BIT | FIRME_IDE_FEATURE_KEYSET_GO_BIT | \
     FIRME_IDE_FEATURE_KEYSET_STOP_BIT | FIRME_IDE_FEATURE_KEYSET_POLL_BIT)

#define FIRME_ATTEST_FEATURE_PAT_GET_BIT       (UINT64_C(1) << 0)
#define FIRME_ATTEST_FEATURE_RAK_GET_BIT       (UINT64_C(1) << 1)
#define FIRME_ATTEST_FEATURE_RAT_SIGN_BIT      (UINT64_C(1) << 2)
#define FIRME_ATTEST_FEATURE_EXT_STAGE_BIT     (UINT64_C(1) << 3)
#define FIRME_ATTEST_FEATURE_EXT_FINAL_BIT     (UINT64_C(1) << 4)
#define FIRME_ATTEST_FEATURE_EXT_CLEAR_BIT     (UINT64_C(1) << 5)
#define FIRME_IDEV_FEATURE_OP_START_BIT        (UINT64_C(1) << 0)
#define FIRME_IDEV_FEATURE_OP_CONTINUE_BIT     (UINT64_C(1) << 1)

/* FIRME_MEC_REFRESH parameter encoding. */
#define FIRME_MEC_REFRESH_REASON_CREATE  UINT64_C(0)
#define FIRME_MEC_REFRESH_REASON_DESTROY UINT64_C(1)
#define FIRME_MEC_REFRESH_REASON_MASK    UINT64_C(1)
#define FIRME_MEC_REFRESH_MECID_SHIFT    32U
#define FIRME_MEC_REFRESH_MECID_WIDTH    16U
#define FIRME_MEC_REFRESH_MECID_MASK     UINT64_C(0xFFFF)
#define FIRME_MEC_REFRESH_PARAMS_MASK \
    ((FIRME_MEC_REFRESH_MECID_MASK << FIRME_MEC_REFRESH_MECID_SHIFT) | \
     FIRME_MEC_REFRESH_REASON_MASK)

/* FIRME MECID management service feature register fields. */
#define FIRME_MECID_FEATURE_REG0_INDEX           0U
#define FIRME_MECID_FEATURE_REG0_MEC_REFRESH_BIT (UINT64_C(1) << 0)
/*
 * ALP4 I0240 and Table 5.1 encode COMMON_MECID_WIDTH minus one in
 * feature register 1 bits[3:0].
 */
#define FIRME_MECID_FEATURE_REG1_INDEX 1U
#define FIRME_MECID_FEATURE_REG1_COMMON_MECID_WIDTHM1_SHIFT 0U
#define FIRME_MECID_FEATURE_REG1_COMMON_MECID_WIDTHM1_MASK  0xFU

/* FIRME Attestation service feature register fields. */
#define FIRME_ATTEST_FEATURE_REG0_INDEX                0U
#define FIRME_ATTEST_FEATURE_REG0_PAT_GET_BIT          (UINT64_C(1) << 0)
#define FIRME_ATTEST_FEATURE_REG0_RAK_GET_BIT          (UINT64_C(1) << 1)
#define FIRME_ATTEST_FEATURE_REG0_RAT_SIGN_BIT         (UINT64_C(1) << 2)
#define FIRME_ATTEST_FEATURE_REG0_EXT_CLAIMS_STAGE_BIT (UINT64_C(1) << 3)
#define FIRME_ATTEST_FEATURE_REG0_EXT_CLAIMS_FINAL_BIT (UINT64_C(1) << 4)
#define FIRME_ATTEST_FEATURE_REG0_EXT_CLAIMS_CLEAR_BIT (UINT64_C(1) << 5)
#define FIRME_ATTEST_FEATURE_REG0_ALL_ABI_BITS         \
    (FIRME_ATTEST_FEATURE_REG0_PAT_GET_BIT | FIRME_ATTEST_FEATURE_REG0_RAK_GET_BIT | \
     FIRME_ATTEST_FEATURE_REG0_RAT_SIGN_BIT | \
     FIRME_ATTEST_FEATURE_REG0_EXT_CLAIMS_STAGE_BIT | \
     FIRME_ATTEST_FEATURE_REG0_EXT_CLAIMS_FINAL_BIT | \
     FIRME_ATTEST_FEATURE_REG0_EXT_CLAIMS_CLEAR_BIT)

/* FIRME Integrated device management service feature register fields. */
#define FIRME_IDEV_FEATURE_REG0_INDEX           0U
#define FIRME_IDEV_FEATURE_REG0_OP_START_BIT    (UINT64_C(1) << 0)
#define FIRME_IDEV_FEATURE_REG0_OP_CONTINUE_BIT (UINT64_C(1) << 1)
#define FIRME_IDEV_FEATURE_REG0_ALL_ABI_BITS    \
    (FIRME_IDEV_FEATURE_REG0_OP_START_BIT | FIRME_IDEV_FEATURE_REG0_OP_CONTINUE_BIT)

typedef struct {
    uint64_t x0;
    uint64_t x1;
    uint64_t x2;
    uint64_t x3;
} firme_res_t;

int val_firme_test_output_poison_enable(uint64_t fid, const firme_res_t *mask,
                                        const firme_res_t *value);
void val_firme_test_output_poison_disable(void);

void val_firme_version(firme_res_t *out);
void val_firme_features(uint64_t feature_id, firme_res_t *out);
void val_firme_service_version(uint64_t service_id, firme_res_t *out);
void val_firme_service_features(uint64_t service_id, uint64_t feature_reg_index,
                                firme_res_t *out);
void val_firme_gm_gpi_set(uint64_t base_address, uint64_t granule_count, uint64_t attributes,
                          firme_res_t *out);
void val_firme_gm_gpi_op_continue(uint64_t cookie, firme_res_t *out);
void val_firme_gm_l1_gpt_create(uint64_t range_base, uint64_t l1_gpt_base,
                                firme_res_t *out);
void val_firme_gm_l1_gpt_destroy(uint64_t range_base, firme_res_t *out);
void val_firme_gm_gpt_info(uint64_t range_base, uint64_t range_top,
                           firme_res_t *out);
void val_firme_mec_refresh(uint64_t mec_params, firme_res_t *out);
int64_t val_firme_status_to_i64(uint64_t value);
int val_firme_status_is_architected(int64_t status);
uint64_t val_firme_base_service_list_bit(uint32_t service_id);
