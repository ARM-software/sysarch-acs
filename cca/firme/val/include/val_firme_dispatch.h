/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

/*
 * Minimal dispatcher ABI.
 * - NS dispatcher initiates calls into other worlds.
 * - Secure/Realm agents implement command handlers and return a status.
 *
 * This harness uses TF-A plumbing for world transitions:
 * - Secure: SPMD direct messaging into the S-EL2 shim (BL32).
 * - Realm: RMMD forwarding of a private RMI-range SMC into the R-EL2 shim.
 */

typedef enum {
    FIRME_ACS_WORLD_NSEL2 = 0,
    FIRME_ACS_WORLD_SECURE = 1,
    FIRME_ACS_WORLD_REALM = 2,
} firme_acs_world_t;

typedef enum {
    FIRME_ACS_CMD_PING = 0x01,
    FIRME_ACS_CMD_RUN_DIAG = 0x02,
    FIRME_ACS_CMD_RUN_RULE = 0x03,
    /* Run the same command body from S-EL1 (within the Secure shim). */
    FIRME_ACS_CMD_RUN_DIAG_SEL1 = 0x04,
    FIRME_ACS_CMD_RUN_RULE_SEL1 = 0x05,
    FIRME_ACS_CMD_RUN_TEST_PAYLOAD = 0x06,
    FIRME_ACS_CMD_GET_TEST_RESULT = 0x07,
    FIRME_ACS_CMD_IDE_LINK = 0x08,
    FIRME_ACS_CMD_GET_RULE_RESULT = 0x09,
    FIRME_ACS_CMD_GPT_PROBE = 0x0a,
    FIRME_ACS_CMD_ABI_FRAME = 0x20,
} firme_acs_cmd_t;

typedef struct {
    uint64_t x0;
    uint64_t x1;
    uint64_t x2;
    uint64_t x3;
    uint64_t x4;
} firme_acs_call_res_t;

/*
 * World call interface used by the NS dispatcher.
 * Returns 0 on success, non-zero on failure/unimplemented.
 */
int val_firme_world_call(firme_acs_world_t world, firme_acs_cmd_t cmd,
                         uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                         firme_acs_call_res_t *res);
