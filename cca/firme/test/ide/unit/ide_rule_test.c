/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>

#include "val_common/val_smccc.h"
#include "../keyset_prog_status.c"
#include "../keyset_go_status.c"
#include "../keyset_stop_status.c"
#include "../keyset_poll_status.c"
#include "../feature_register.c"
#include "../abi_compliance.c"

#define TEST_ECAM UINT64_C(0x40000000)
#define TEST_UNUSED (UINT64_C(1) << 63)
#define TEST_STREAM 73U

static uint32_t g_async;
static uint32_t g_pending;
static uint32_t g_stop_count;
static uint32_t g_bad_keyset;
static uint32_t g_bad_handle;
static uint32_t g_bad_stop;
static uint32_t g_missing_abi;
static uint32_t g_bad_prog;
static uint64_t g_handle;

/** @brief Supply the native retry-loop relaxation hook. */
void val_mp_relax(void)
{
}

/** @brief Decode the mock FIRME status. */
int64_t val_firme_status_to_i64(uint64_t value)
{
    return (int64_t)value;
}

/** @brief Advertise an implemented IDE service. */
void val_firme_service_version(uint64_t service, firme_res_t *res)
{
    assert(service == FIRME_SERVICE_ID_IDE_KEY_MGMT);
    *res = (firme_res_t){.x0 = UINT64_C(0x10000)};
}

/** @brief Poison unused feature bits while preserving mandatory bits. */
void val_firme_service_features(uint64_t service, uint64_t index, firme_res_t *res)
{
    assert(service == FIRME_SERVICE_ID_IDE_KEY_MGMT && index == 0);
    *res = (firme_res_t){.x0 = FIRME_SUCCESS,
        .x1 = TEST_UNUSED | FIRME_IDE_FEATURE_REG0_ALL_ABI_BITS};
    if (g_missing_abi)
        res->x1 &= ~FIRME_IDE_FEATURE_REG0_PROG_BIT;
}

/** @brief Supply the architected mandatory mask to the rule body. */
uint64_t val_firme_service_mandatory_abi_mask(firme_acs_world_t world, uint32_t service)
{
    (void)world;
    assert(service == FIRME_SERVICE_ID_IDE_KEY_MGMT);
    return FIRME_IDE_FEATURE_REG0_ALL_ABI_BITS;
}

/** @brief Supply the only valid mock ECAM. */
uint64_t val_pcie_ecam_base(void) { return TEST_ECAM; }
/** @brief Supply a deliberately invalid ECAM. */
uint64_t val_pcie_invalid_ecam_base(void) { return TEST_ECAM + 1ULL; }
/** @brief Supply the configured segment. */
uint32_t val_pcie_default_segment(void) { return 0U; }
/** @brief Supply the configured Root Port. */
uint32_t val_pcie_default_root_port_id(void) { return 2U; }
/** @brief Reject assumptions about FVP's default stream. */
uint32_t val_pcie_default_stream_id(void) { return TEST_STREAM; }
/** @brief Supply the configured substream. */
uint32_t val_pcie_default_substream_id(void) { return 0U; }
/** @brief Supply the configured direction. */
uint32_t val_pcie_default_direction(void) { return 0U; }
/** @brief Supply the configured key set. */
uint32_t val_pcie_default_key_set(void) { return 0U; }

/** @brief Leave expected negative-test diagnostics silent. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *format, ...)
{
    (void)verbosity;
    (void)tag;
    (void)format;
    return VAL_SUCCESS;
}

/** @brief Model one provisioned stream and synchronous or asynchronous requests. */
static void test_smc(uint64_t fid, uint64_t ecam, uint64_t flags,
                     uint64_t keyset, uint64_t handle, smccc_res_t *res)
{
    *res = (smccc_res_t){.x0 = FIRME_SUCCESS, .x1 = TEST_UNUSED, .x2 = TEST_UNUSED};
    if (ecam != TEST_ECAM ||
        (fid != FIRME_FID_IDE_KEYSET_POLL && flags != VAL_FIRME_IDE_TYPE_SELECTIVE)) {
        res->x0 = (uint64_t)FIRME_INVALID_PARAMETERS;
        return;
    }
    if (fid == FIRME_FID_IDE_KEYSET_POLL) {
        if (!g_pending) {
            res->x0 = (uint64_t)FIRME_DENIED;
            return;
        }
        assert(flags == 1ULL || keyset == val_firme_ide_default_keyset_id());
        g_pending = VAL_FALSE;
        res->x1 = g_handle ^ g_bad_handle;
        res->x2 = TEST_UNUSED | (val_firme_ide_default_keyset_id() ^ g_bad_keyset);
        return;
    }
    assert(keyset == val_firme_ide_default_keyset_id());
    if (g_pending) {
        res->x0 = (uint64_t)FIRME_OP_CONFLICT;
        return;
    }
    if (fid == FIRME_FID_IDE_KEYSET_PROG && g_bad_prog) {
        res->x0 = (uint64_t)FIRME_INVALID_PARAMETERS;
        return;
    }
    if (fid == FIRME_FID_IDE_KEYSET_STOP) {
        g_stop_count++;
        if (g_bad_stop) {
            res->x0 = (uint64_t)FIRME_DENIED;
            return;
        }
    }
    if (g_async) {
        g_pending = VAL_TRUE;
        g_handle = handle;
        res->x0 = (uint64_t)FIRME_INCOMPLETE;
    }
}

/** @brief Capture the production GO/STOP/POLL wrapper calls. */
void val_smccc_smc_call(uint64_t fid, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                        uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7,
                        smccc_res_t *res)
{
    assert(arg5 == 0 && arg6 == 0 && arg7 == 0);
    test_smc(fid, arg1, arg2, arg3, arg4, res);
}

/** @brief Capture the production PROG wrapper including its caller handle. */
void val_smccc_smc_call_x8(uint64_t fid, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                           uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7,
                           uint64_t arg8, smccc_res_t *res)
{
    (void)arg4;
    (void)arg5;
    (void)arg6;
    (void)arg7;
    test_smc(fid, arg1, arg2, arg3, arg8, res);
}

/** @brief Exercise real rule bodies with valid and deliberately faulty responses. */
int main(void)
{
    firme_rule_res_t result = {0};
    uint32_t mode;

    for (mode = 0; mode < 2U; mode++) {
        g_async = mode;
        assert(firme_ide_keyset_prog_status_run_realm(0, &result) == VAL_SUCCESS);
        assert(result.status == ACS_STATUS_PASS && !g_pending);
        assert(firme_ide_keyset_go_status_run_realm(0, &result) == VAL_SUCCESS);
        assert(result.status == ACS_STATUS_PASS && !g_pending);
        assert(firme_ide_keyset_stop_status_run_realm(0, &result) == VAL_SUCCESS);
        assert(result.status == ACS_STATUS_PASS && !g_pending);
        assert(firme_ide_keyset_poll_status_run_realm(0, &result) == VAL_SUCCESS);
        assert(result.status == (mode ? ACS_STATUS_PASS : ACS_STATUS_SKIP));
        assert(!g_pending && g_stop_count >= 4U);
        g_stop_count = 0;
    }
    g_bad_keyset = VAL_TRUE;
    assert(firme_ide_keyset_poll_status_run_realm(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && !g_pending);
    g_bad_keyset = VAL_FALSE;
    g_bad_handle = VAL_TRUE;
    assert(firme_ide_keyset_go_status_run_realm(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    g_bad_handle = VAL_FALSE;
    g_bad_stop = VAL_TRUE;
    assert(firme_ide_keyset_prog_status_run_realm(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    g_bad_stop = VAL_FALSE;
    g_bad_prog = VAL_TRUE;
    assert(firme_ide_keyset_poll_status_run_realm(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    g_bad_prog = VAL_FALSE;
    assert(firme_ide_feature_register_run_realm(0, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    assert(firme_ide_abi_compliance_run_realm(0, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    g_missing_abi = VAL_TRUE;
    assert(firme_ide_feature_register_run_realm(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    assert(firme_ide_abi_compliance_run_realm(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    puts("IDE rules: configured stream, completion, unused bits and cleanup PASS");
    return VAL_SUCCESS;
}
