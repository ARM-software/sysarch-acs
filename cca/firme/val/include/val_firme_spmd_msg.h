/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

/*
 * Minimal SPMD direct-messaging (FF-A) SMC definitions used by FIRME-ACS.
 * The harness uses a small subset for direct messaging only.
 */

/* SMC calling convention bits. */
#define FUNCID_TYPE_SHIFT 31
#define FUNCID_CC_SHIFT 30
#define FUNCID_OEN_SHIFT 24
#define FUNCID_NUM_SHIFT 0

#define SMC_TYPE_FAST 1U
#define SMC_32 0U
#define SMC_64 1U
#define OEN_STD_START 4U

#define FFA_FID(smc_cc, func_num) \
    ((uint32_t)(SMC_TYPE_FAST << FUNCID_TYPE_SHIFT) | \
     (uint32_t)((smc_cc) << FUNCID_CC_SHIFT) | \
     (uint32_t)(OEN_STD_START << FUNCID_OEN_SHIFT) | \
     (uint32_t)((func_num) << FUNCID_NUM_SHIFT))

/* Function numbers. */
#define FFA_FNUM_ERROR 0x60U
#define FFA_FNUM_SUCCESS 0x61U
#define FFA_FNUM_VERSION 0x63U
#define FFA_FNUM_FEATURES 0x64U
#define FFA_FNUM_ID_GET 0x69U
#define FFA_FNUM_MSG_WAIT 0x6BU
#define FFA_FNUM_MSG_SEND_DIRECT_REQ 0x6FU
#define FFA_FNUM_MSG_SEND_DIRECT_RESP 0x70U
#define FFA_FNUM_SPM_ID_GET 0x85U /* FF-A v1.1 */

/* SMC32 FIDs. */
#define FFA_ERROR FFA_FID(SMC_32, FFA_FNUM_ERROR)
#define FFA_SUCCESS_SMC32 FFA_FID(SMC_32, FFA_FNUM_SUCCESS)
#define FFA_SUCCESS_SMC64 FFA_FID(SMC_64, FFA_FNUM_SUCCESS)
#define FFA_VERSION FFA_FID(SMC_32, FFA_FNUM_VERSION)
#define FFA_FEATURES FFA_FID(SMC_32, FFA_FNUM_FEATURES)
#define FFA_ID_GET FFA_FID(SMC_32, FFA_FNUM_ID_GET)
#define FFA_MSG_WAIT FFA_FID(SMC_32, FFA_FNUM_MSG_WAIT)
#define FFA_SPM_ID_GET FFA_FID(SMC_32, FFA_FNUM_SPM_ID_GET)

/* SMC64 FIDs for direct messaging. */
#define FFA_MSG_SEND_DIRECT_REQ_SMC32 FFA_FID(SMC_32, FFA_FNUM_MSG_SEND_DIRECT_REQ)
#define FFA_MSG_SEND_DIRECT_RESP_SMC32 FFA_FID(SMC_32, FFA_FNUM_MSG_SEND_DIRECT_RESP)
#define FFA_MSG_SEND_DIRECT_REQ_SMC64 FFA_FID(SMC_64, FFA_FNUM_MSG_SEND_DIRECT_REQ)
#define FFA_MSG_SEND_DIRECT_RESP_SMC64 FFA_FID(SMC_64, FFA_FNUM_MSG_SEND_DIRECT_RESP)

/* Framework message flag in x2 for SMC32 direct messages. */
#define FFA_FWK_MSG_BIT (1U << 31)
#define FFA_FWK_MSG_ID_MASK (0xFFU)

/* SPMD framework message IDs (TF-A implementation). */
#define SPMD_FWK_MSG_FFA_VERSION_REQ 0x8U
#define SPMD_FWK_MSG_FFA_VERSION_RESP 0x9U
#define SPMD_FWK_MSG_PSCI 0x0U
#define SPMD_FWK_MSG_PM_RESP 0x2U
#define SPMD_PSCI_CPU_OFF 0x84000002U

/* Common error codes (x2 for FFA_ERROR). */
typedef enum {
    FFA_ERROR_NOT_SUPPORTED = -1,
    FFA_ERROR_INVALID_PARAMETER = -2,
    FFA_ERROR_DENIED = -6,
} ffa_error_t;

/* Version field encoding (major<<16)|minor. */
#define FFA_MAKE_VERSION(major, minor) (((uint32_t)(major) << 16) | (uint32_t)(minor))
