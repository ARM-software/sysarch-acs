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
#include "../keyset_id_bitmap_encoding.c"

#define TEST_ECAM UINT64_C(0x40000000)
#define TEST_UNUSED (UINT64_C(1) << 63)
#define TEST_STREAM 73U
#define TEST_KEYSET ((uint64_t)TEST_STREAM << VAL_FIRME_IDE_KEYSET_STREAM_SHIFT)

static uint32_t g_async;
static uint32_t g_pending;
static uint32_t g_stop_count;
static uint32_t g_bad_keyset;
static uint32_t g_bad_handle;
static uint32_t g_bad_stop;
static uint32_t g_missing_abi;
static int64_t g_reg1_status = FIRME_SUCCESS;
static uint64_t g_maximum_operations;
static uint32_t g_unallocated_index = UINT32_MAX;
static uint32_t g_feature_calls[FIRME_FEATURE_REG_INDEX_8BIT_MAX + 1U];
static uint32_t g_bad_prog;
static uint32_t g_accept_invalid;
static uint32_t g_release_count;
static uint64_t g_handle;
static uint32_t g_observations;
static uint64_t g_observation_status = ACS_STATUS_PASS;

/** @brief Verify observation precedes cleanup and names the issued request, not a replay. */
int val_firme_ide_observe_operation(uint64_t operation, firme_acs_world_t world,
                                    const val_firme_ide_observation_t *selection,
                                    firme_rule_res_t *out)
{
    assert(world == FIRME_ACS_WORLD_REALM && selection && selection->stream);
    assert(selection->stream->acquired && !g_pending);
    assert(selection->stream->path.ecam_base == TEST_ECAM);
    assert(selection->keyset_id == TEST_KEYSET);
    assert(selection->key[0] == val_firme_ide_test_key_qw(0U));
    if (operation == VAL_FIRME_IDE_OBSERVE_KEYSET_ENCODING)
        assert(selection->handle == IDE_KEYSET_BITMAP_HANDLE);
    else if (operation == VAL_IDE_LINK_PROG)
        assert(selection->handle == IDE_KEYSET_PROG_HANDLE_PROG);
    else if (operation == VAL_IDE_LINK_GO)
        assert(selection->handle == IDE_KEYSET_GO_HANDLE_OP);
    else {
        assert(operation == VAL_IDE_LINK_STOP);
        assert(selection->handle == IDE_KEYSET_STOP_HANDLE_OP);
    }

    g_observations++;
    *out = (firme_rule_res_t){.status = g_observation_status};
    return g_observation_status == ACS_STATUS_FAIL ? VAL_ERROR : VAL_SUCCESS;
}

/** @brief Isolate rule verdicts from the separately tested stream allocator. */
uint32_t val_pcie_ide_stream_discover(uint32_t realm_caller, val_pcie_ide_stream_t *stream)
{
    assert(realm_caller);
    *stream = (val_pcie_ide_stream_t){.path = {.ecam_base = TEST_ECAM},
        .stream_id = TEST_STREAM,
        .acquired = VAL_TRUE};
    return ACS_STATUS_PASS;
}

/** @brief Ensure rule cleanup releases its configuration after draining keys. */
int val_pcie_ide_stream_release(val_pcie_ide_stream_t *stream)
{
    assert(!g_pending && stream->acquired);
    stream->acquired = VAL_FALSE;
    g_release_count++;
    return VAL_SUCCESS;
}

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
    assert(service == FIRME_SERVICE_ID_IDE_KEY_MGMT);
    assert(index <= FIRME_FEATURE_REG_INDEX_8BIT_MAX);
    g_feature_calls[index]++;
    *res = (firme_res_t){.x0 = (uint64_t)FIRME_NOT_SUPPORTED, .x1 = TEST_UNUSED};
    if (index == FIRME_IDE_FEATURE_REG0_INDEX) {
        res->x0 = FIRME_SUCCESS;
        res->x1 |= FIRME_IDE_FEATURE_REG0_ALL_ABI_BITS;
        if (g_missing_abi)
            res->x1 &= ~FIRME_IDE_FEATURE_REG0_PROG_BIT;
    } else if (index == FIRME_IDE_FEATURE_REG1_INDEX) {
        res->x0 = (uint64_t)g_reg1_status;
        res->x1 |= g_maximum_operations;
    } else if (index == g_unallocated_index) {
        res->x0 = (uint64_t)FIRME_INVALID_PARAMETERS;
    }
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
        res->x0 = g_accept_invalid ? (uint64_t)FIRME_INCOMPLETE :
                  (uint64_t)FIRME_INVALID_PARAMETERS;
        return;
    }
    if (fid == FIRME_FID_IDE_KEYSET_POLL) {
        if (!g_pending) {
            res->x0 = (uint64_t)FIRME_DENIED;
            return;
        }
        assert(flags == 1ULL || keyset == TEST_KEYSET);
        g_pending = VAL_FALSE;
        res->x1 = g_handle ^ g_bad_handle;
        res->x2 = TEST_UNUSED | (TEST_KEYSET ^ g_bad_keyset);
        return;
    }
    assert(keyset == TEST_KEYSET);
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
    firme_ide_test_state_t completion = {0};
    firme_res_t wrong_handle = {0};
    uint32_t mode;
    uint32_t feature_index;
    uint32_t releases_before;
    uint32_t stops_before;
    uint32_t observations_before;

    for (mode = 0; mode < 2U; mode++) {
        g_async = mode;
        assert(firme_ide_keyset_id_bitmap_encoding_run_realm(0, &result) == VAL_SUCCESS);
        assert(result.status == ACS_STATUS_PASS && !g_pending);
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
    assert(g_observations == 8U);
    g_observation_status = ACS_STATUS_SKIP;
    releases_before = g_release_count;
    assert(firme_ide_keyset_prog_status_run_realm(0, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_SKIP && g_release_count == releases_before + 1U);
    g_observation_status = ACS_STATUS_FAIL;
    releases_before = g_release_count;
    assert(firme_ide_keyset_go_status_run_realm(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && g_release_count == releases_before + 1U);
    g_observation_status = UINT64_MAX;
    assert(firme_ide_keyset_stop_status_run_realm(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_ERR);
    g_observation_status = ACS_STATUS_PASS;
    observations_before = g_observations;
    g_bad_keyset = VAL_TRUE;
    assert(firme_ide_keyset_poll_status_run_realm(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && !g_pending);
    g_bad_keyset = VAL_FALSE;
    completion.pending = VAL_TRUE;
    completion.pending_handle = UINT64_C(0x1234);
    wrong_handle.x0 = FIRME_SUCCESS;
    wrong_handle.x1 = completion.pending_handle + UINT64_C(1);
    assert(firme_ide_test_polled(&completion, &wrong_handle) == VAL_ERROR);
    assert(completion.pending && completion.quarantined);
    g_bad_handle = VAL_TRUE;
    releases_before = g_release_count;
    stops_before = g_stop_count;
    assert(firme_ide_keyset_go_status_run_realm(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    assert(g_release_count == releases_before && g_stop_count == stops_before);
    g_bad_handle = VAL_FALSE;
    g_bad_stop = VAL_TRUE;
    assert(firme_ide_keyset_prog_status_run_realm(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    g_bad_stop = VAL_FALSE;
    g_bad_prog = VAL_TRUE;
    assert(firme_ide_keyset_poll_status_run_realm(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    assert(g_observations == observations_before + 1U);
    g_bad_prog = VAL_FALSE;
    assert(firme_ide_feature_register_run_realm(0, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    for (feature_index = 0U; feature_index <= FIRME_FEATURE_REG_INDEX_8BIT_MAX;
         feature_index++)
        assert(g_feature_calls[feature_index] != 0U);

    g_maximum_operations = FIRME_IDE_FEATURE_MAX_OP_COUNT_MASK;
    assert(firme_ide_feature_register_run_ns(0, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    g_reg1_status = FIRME_NOT_SUPPORTED;
    assert(firme_ide_feature_register_run_realm(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    assert(result.fail_step == IDE_FEATURE_REGISTER_STEP_REG1_STATUS);
    g_reg1_status = FIRME_INVALID_PARAMETERS;
    assert(firme_ide_feature_register_run_ns(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    g_reg1_status = FIRME_SUCCESS;
    for (feature_index = FIRME_IDE_FEATURE_REG_COUNT;
         feature_index <= FIRME_FEATURE_REG_INDEX_8BIT_MAX; feature_index++) {
        g_unallocated_index = feature_index;
        assert(firme_ide_feature_register_run_realm(0, &result) == VAL_ERROR);
        assert(result.status == ACS_STATUS_FAIL);
        assert(result.fail_step == IDE_FEATURE_REGISTER_STEP_UNALLOCATED);
        assert(result.last_x1 == feature_index);
    }

    g_unallocated_index = UINT32_MAX;
    assert(firme_ide_abi_compliance_run_realm(0, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    g_missing_abi = VAL_TRUE;
    assert(firme_ide_feature_register_run_realm(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    assert(firme_ide_abi_compliance_run_realm(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    g_missing_abi = VAL_FALSE;
    g_accept_invalid = VAL_TRUE;
    g_release_count = 0U;
    assert(firme_ide_keyset_prog_status_run_realm(0, &result) == VAL_ERROR);
    assert(firme_ide_keyset_go_status_run_realm(0, &result) == VAL_ERROR);
    assert(firme_ide_keyset_stop_status_run_realm(0, &result) == VAL_ERROR);
    assert(firme_ide_keyset_poll_status_run_realm(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && !g_release_count);
    puts("IDE rules: configured stream, completion, unused bits and cleanup PASS");
    return VAL_SUCCESS;
}
