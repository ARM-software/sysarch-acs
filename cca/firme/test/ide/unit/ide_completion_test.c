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
#include "../keyset_id_bitmap_encoding.c"
#include "../cxl_substream_id.c"
#include "../cxl_stream_id_zero.c"
#include "../cxl_key_set_unused.c"
#include "../cxl_test_common.c"
#include "../secure_caller_not_supported.c"
#include "../../../val/src/val_firme_ide_abi_adapter.c"
#include "../../../val/src/val_firme_ide_link.c"
#include "../../../val/src/val_firme_ide_namespace.c"

#define TEST_ECAM UINT64_C(0x40000000)
#define TEST_BAD_ECAM UINT64_C(0x50000000)
#define TEST_UNUSED UINT64_C(0xfeedfacecafebeef)
#define TEST_UNKNOWN_STATUS INT64_C(42)

static int64_t g_service_status;
static int64_t g_program_status;
static int64_t g_operation_status;
static int64_t g_poll_status;
static uint64_t g_pending_keyset;
static uint64_t g_pending_handle;
static uint32_t g_pending;
static uint32_t g_bad_exact_handle;
static uint32_t g_bad_any_handle;
static uint32_t g_bad_any_keyset;
static uint64_t g_bad_handle_request;
static uint32_t g_frame_calls;
static uint32_t g_frame_busy_count;
static uint32_t g_secure_caller;
static uint64_t g_secure_unexpected_fid;
static uint32_t g_test_pe;
static uint32_t g_test_controller;
static uint32_t g_stop_calls;
static uint32_t g_cxl_accept_invalid;
static uint32_t g_cxl_reject_invalid_status;
static uint32_t g_cxl_bad_go;
static uint32_t g_cxl_bad_stop;
static uint32_t g_cxl_invalid_calls;
static uint32_t g_poll_calls;
static uint32_t g_pal_prepare_calls;
static uint32_t g_pal_cleanup_calls;
static uint32_t g_pal_observe_calls;
static pal_firme_ide_abi_status_t g_pal_cleanup_status = PAL_FIRME_IDE_ABI_READY;
static pal_firme_ide_abi_status_t g_pal_observe_status = PAL_FIRME_IDE_ABI_READY;
static uint32_t g_namespace_available;
static uint32_t g_namespace_prepares;
static uint32_t g_namespace_releases;
static pal_firme_ide_namespace_status_t g_namespace_observation = PAL_FIRME_IDE_NAMESPACE_READY;

static uint64_t test_link_prepare(uint32_t direction);

/** @brief Provide a discovered Selective pair for rule completion checks. */
uint32_t val_pcie_ide_stream_discover(uint32_t realm_caller, val_pcie_ide_stream_t *stream)
{
    assert(realm_caller && stream);
    *stream = (val_pcie_ide_stream_t){
        .path = {.ecam_base = TEST_ECAM, .root_port_id = 8U},
        .stream_id = 1U, .acquired = VAL_TRUE};
    return ACS_STATUS_PASS;
}

/** @brief Require pending responses to be consumed before releasing a pair. */
int val_pcie_ide_stream_release(val_pcie_ide_stream_t *stream)
{
    assert(stream && stream->acquired && !g_pending);
    stream->acquired = VAL_FALSE;
    return VAL_SUCCESS;
}

/** @brief Supply mock-only independent Root Ports without claiming FVP coverage. */
pal_firme_ide_namespace_status_t pal_firme_ide_namespace_prepare(
    pal_firme_ide_namespace_kind_t kind, pal_firme_ide_namespace_scenario_t *scenario)
{
    (void)kind;
    g_namespace_prepares++;
    *scenario = (pal_firme_ide_namespace_scenario_t){0};
    if (!g_namespace_available)
        return PAL_FIRME_IDE_NAMESPACE_UNAVAILABLE;
    scenario->token = 12ULL;
    scenario->path[0] = (pal_firme_ide_namespace_path_t){.ecam_address = TEST_ECAM,
        .keyset_id = 0x20020ULL, .handle = 1ULL, .keyqw = {1ULL, 2ULL, 3ULL, 4ULL},
        .ide_type = VAL_FIRME_IDE_TYPE_CXL_CACHEMEM};
    scenario->path[1] = scenario->path[0];
    scenario->path[1].keyset_id = 0x24020ULL;
    scenario->path[1].handle = 2ULL;
    scenario->path[1].keyqw[0] = 5ULL;
    return PAL_FIRME_IDE_NAMESPACE_READY;
}

/** @brief Inject absent namespace evidence independently of architectural responses. */
pal_firme_ide_namespace_status_t pal_firme_ide_namespace_observe(
    pal_firme_ide_namespace_kind_t kind, const pal_firme_ide_namespace_scenario_t *scenario,
    uint32_t first_active, uint32_t second_active)
{
    (void)kind;
    (void)first_active;
    (void)second_active;
    assert(scenario->token == 12ULL);
    return g_namespace_observation;
}

/** @brief Count namespace releases that must never occur while operations remain pending. */
pal_firme_ide_namespace_status_t pal_firme_ide_namespace_cleanup(
    pal_firme_ide_namespace_kind_t kind, const pal_firme_ide_namespace_scenario_t *scenario)
{
    (void)kind;
    assert(scenario->token == 12ULL);
    g_namespace_releases++;
    return PAL_FIRME_IDE_NAMESPACE_READY;
}

/** @brief Reserve a synthetic ABI path solely for lifecycle regression tests. */
pal_firme_ide_abi_status_t pal_firme_ide_abi_prepare(
    uint32_t world, pal_firme_ide_abi_operation_t operation,
    pal_firme_ide_abi_condition_t condition, pal_firme_ide_abi_scenario_t *scenario)
{
    (void)world;
    (void)operation;
    (void)condition;
    g_pal_prepare_calls++;
    *scenario = (pal_firme_ide_abi_scenario_t){.ecam_address = TEST_ECAM,
        .invalid_ecam_address = TEST_BAD_ECAM, .keyset_id = 0x20020ULL,
        .handle = 1ULL, .token = 99ULL, .ide_type = VAL_FIRME_IDE_TYPE_CXL_CACHEMEM,
        .keyqw = {1ULL, 2ULL, 3ULL, 4ULL}
    };
    return PAL_FIRME_IDE_ABI_READY;
}

/** @brief Accept only the reservation token selected by the native fixture. */
pal_firme_ide_abi_status_t pal_firme_ide_abi_ready(
    uint32_t world, pal_firme_ide_abi_operation_t operation,
    pal_firme_ide_abi_condition_t condition, const pal_firme_ide_abi_scenario_t *scenario)
{
    (void)world;
    (void)operation;
    (void)condition;
    assert(scenario->token == 99ULL);
    return PAL_FIRME_IDE_ABI_READY;
}

/** @brief Inject missing effect evidence without fabricating architectural PASS. */
pal_firme_ide_abi_status_t pal_firme_ide_abi_observe(
    uint32_t world, pal_firme_ide_abi_operation_t operation,
    pal_firme_ide_abi_condition_t condition, const pal_firme_ide_abi_scenario_t *scenario)
{
    assert(pal_firme_ide_abi_ready(world, operation, condition, scenario) ==
           PAL_FIRME_IDE_ABI_READY);
    g_pal_observe_calls++;
    return g_pal_observe_status;
}

/** @brief Model acknowledged endpoint transmitter quiescence. */
pal_firme_ide_abi_status_t pal_firme_ide_abi_quiesce(
    uint32_t world, const pal_firme_ide_abi_scenario_t *scenario)
{
    (void)world;
    assert(scenario->token == 99ULL);
    return PAL_FIRME_IDE_ABI_READY;
}

/** @brief Count platform releases and inject restoration failures. */
pal_firme_ide_abi_status_t pal_firme_ide_abi_cleanup(
    uint32_t world, pal_firme_ide_abi_operation_t operation,
    pal_firme_ide_abi_condition_t condition, const pal_firme_ide_abi_scenario_t *scenario)
{
    assert(pal_firme_ide_abi_ready(world, operation, condition, scenario) ==
           PAL_FIRME_IDE_ABI_READY);
    g_pal_cleanup_calls++;
    return g_pal_cleanup_status;
}

/** @brief Model a segment-aware requester conversion for the link fixture. */
uint32_t val_pcie_bdf_from_requester_id(uint32_t segment, uint32_t requester_id)
{
    return (segment << 24U) | requester_id;
}

/** @brief Resolve only the fixture's actual Root Port allocation. */
int val_pcie_ecam_base_for_bdf(uint32_t bdf, uint64_t *ecam_base)
{
    if (!ecam_base || bdf != 8U)
        return VAL_ERROR;
    *ecam_base = TEST_ECAM;
    return VAL_SUCCESS;
}

/** @brief Decode the architected signed W0 status for the mock ABI. */
int64_t val_firme_status_to_i64(uint64_t value)
{
    return (int64_t)(int32_t)(uint32_t)value;
}

/** @brief Supply a configurable IDE service-discovery response. */
void val_firme_service_version(uint64_t service_id, firme_res_t *out)
{
    assert(service_id == FIRME_SERVICE_ID_IDE_KEY_MGMT);
    *out = (firme_res_t){.x0 = (uint32_t)g_service_status};
}

/** @brief Advertise all mandatory IDE operations in the mock. */
void val_firme_service_features(uint64_t service_id, uint64_t feature_index,
                                 firme_res_t *out)
{
    assert(service_id == FIRME_SERVICE_ID_IDE_KEY_MGMT);
    assert(feature_index == FIRME_IDE_FEATURE_REG0_INDEX);
    *out = (firme_res_t){.x0 = FIRME_SUCCESS,
                         .x1 = FIRME_IDE_FEATURE_REG0_ALL_ABI_BITS};
}

/** @brief Return the isolated test ECAM base. */
uint64_t val_pcie_ecam_base(void)
{
    return TEST_ECAM;
}

/** @brief Return an ECAM base rejected by the mock Root Port. */
uint64_t val_pcie_invalid_ecam_base(void)
{
    return TEST_BAD_ECAM;
}

/** @brief Select the mock PCI segment. */
uint32_t val_pcie_default_segment(void)
{
    return 0U;
}

/** @brief Select the mock Root Port requester ID. */
uint32_t val_pcie_default_root_port_id(void)
{
    return 8U;
}

/** @brief Select the mock Selective IDE stream. */
uint32_t val_pcie_default_stream_id(void)
{
    return 1U;
}

/** @brief Select the mock PCIe substream. */
uint32_t val_pcie_default_substream_id(void)
{
    return 0U;
}

/** @brief Select the mock Root Port receive direction. */
uint32_t val_pcie_default_direction(void)
{
    return 0U;
}

/** @brief Select the mock key set. */
uint32_t val_pcie_default_key_set(void)
{
    return 0U;
}

/** @brief Avoid platform waits in a deterministic native retry loop. */
void val_mp_relax(void)
{
}

/** @brief Select the caller PE independently of any fixed primary index. */
uint32_t val_pe_index(void)
{
    return g_test_pe;
}

/** @brief Suppress expected error logs from negative regression cases. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *format, ...)
{
    (void)verbosity;
    (void)tag;
    (void)format;
    return 0U;
}

/**
 * @brief Model synchronous or asynchronous Root Port operations and POLL outputs.
 * @param result Returns poisoned unused registers and a signed W0 status.
 */
static void test_smc(uint64_t fid, uint64_t ecam, uint64_t flags, uint64_t keyset,
                      uint64_t handle, smccc_res_t *result)
{
    int64_t status = FIRME_INVALID_PARAMETERS;

    if (flags == VAL_FIRME_IDE_TYPE_CXL_CACHEMEM && fid != FIRME_FID_IDE_KEYSET_POLL)
        assert((keyset & VAL_FIRME_IDE_KEYSET_KEY_SET_MASK) == 0ULL);

    if (fid == FIRME_FID_IDE_KEYSET_STOP)
        g_stop_calls++;
    if (fid == FIRME_FID_IDE_KEYSET_POLL)
        g_poll_calls++;
    *result = (smccc_res_t){.x1 = TEST_UNUSED, .x2 = TEST_UNUSED, .x3 = TEST_UNUSED};
    if (g_secure_caller == VAL_TRUE) {
        status = fid == g_secure_unexpected_fid ? FIRME_SUCCESS : FIRME_NOT_SUPPORTED;
    } else if (ecam == TEST_ECAM) {
        if (fid == FIRME_FID_IDE_KEYSET_POLL) {
            status = g_pending ? g_poll_status : FIRME_DENIED;
            if (status == FIRME_SUCCESS) {
                result->x1 = g_pending_handle;
                if (flags == val_firme_ide_poll_flags(VAL_TRUE)) {
                    result->x1 ^= g_bad_any_handle;
                    result->x2 = g_pending_keyset ^ g_bad_any_keyset;
                } else {
                    assert(keyset == g_pending_keyset);
                    result->x1 ^= g_bad_exact_handle ||
                                  (g_bad_handle_request != 0ULL &&
                                   g_bad_handle_request == g_pending_handle);
                }
                g_pending = VAL_FALSE;
            }
            if (status == FIRME_ABORTED)
                g_pending = VAL_FALSE;
        } else if (flags == VAL_FIRME_IDE_TYPE_CXL_CACHEMEM &&
                   (((keyset >> VAL_FIRME_IDE_KEYSET_STREAM_SHIFT) &
                     VAL_FIRME_IDE_KEYSET_STREAM_MASK) != 0ULL ||
                    ((keyset >> VAL_FIRME_IDE_KEYSET_SUBSTREAM_SHIFT) &
                     VAL_FIRME_IDE_KEYSET_SUBSTREAM_MASK) != VAL_FIRME_IDE_CXL_SUBSTREAM_ID)) {
            g_cxl_invalid_calls++;
            status = g_cxl_accept_invalid ? FIRME_INCOMPLETE :
                     g_cxl_reject_invalid_status ? FIRME_NOT_SUPPORTED :
                     FIRME_INVALID_PARAMETERS;
        } else if (flags != VAL_FIRME_IDE_TYPE_UNUSED_TEST) {
            status = g_pending ? FIRME_OP_CONFLICT :
                     fid == FIRME_FID_IDE_KEYSET_PROG ? g_program_status :
                     g_operation_status;
            if ((fid == FIRME_FID_IDE_KEYSET_GO && g_cxl_bad_go) ||
                (fid == FIRME_FID_IDE_KEYSET_STOP && g_cxl_bad_stop))
                status = FIRME_DENIED;
            if (status == FIRME_INCOMPLETE) {
                g_pending = VAL_TRUE;
                g_pending_keyset = keyset;
                g_pending_handle = handle;
            }
        }
    }
    result->x0 = UINT64_C(0x8765432100000000) | (uint32_t)status;
}

/** @brief Route GO, STOP and POLL through the native Root Port model. */
void val_smccc_smc_call(uint64_t fid, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                        uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7,
                        smccc_res_t *result)
{
    (void)arg5;
    (void)arg6;
    (void)arg7;
    test_smc(fid, arg1, arg2, arg3, arg4, result);
}

/** @brief Route PROG and its x8 caller handle through the native model. */
void val_smccc_smc_call_x8(uint64_t fid, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                           uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7,
                           uint64_t arg8, smccc_res_t *result)
{
    (void)arg4;
    (void)arg5;
    (void)arg6;
    (void)arg7;
    test_smc(fid, arg1, arg2, arg3, arg8, result);
}

/** @brief Complete a generated-ABI request after a configurable BUSY prefix. */
void val_smccc_smc_call_frame(const smccc_call_frame_t *input,
                              smccc_call_frame_t *output)
{
    (void)input;
    g_frame_calls++;
    output->registers[0] = g_frame_calls <= g_frame_busy_count ?
                          (uint64_t)FIRME_BUSY : (uint64_t)FIRME_SUCCESS;
}

/** @brief Model the boot/controller identity independently of the current PE. */
uint32_t val_mmu_is_controller_pe(void)
{
    return g_test_pe == g_test_controller;
}

/** @brief Restore an asynchronous success scenario between independent cases. */
static void test_reset(void)
{
    g_service_status = FIRME_SUCCESS;
    g_program_status = FIRME_INCOMPLETE;
    g_operation_status = FIRME_INCOMPLETE;
    g_poll_status = FIRME_SUCCESS;
    g_pending = VAL_FALSE;
    g_bad_exact_handle = VAL_FALSE;
    g_bad_any_handle = VAL_FALSE;
    g_bad_any_keyset = VAL_FALSE;
    g_bad_handle_request = 0ULL;
    g_secure_caller = VAL_FALSE;
    g_secure_unexpected_fid = 0ULL;
    g_stop_calls = 0U;
    g_poll_calls = 0U;
    g_cxl_test_reserved = VAL_FALSE;
    g_cxl_accept_invalid = VAL_FALSE;
    g_cxl_reject_invalid_status = VAL_FALSE;
    g_cxl_bad_go = VAL_FALSE;
    g_cxl_bad_stop = VAL_FALSE;
    g_cxl_invalid_calls = 0U;
}

/** @brief Retain timed-out generated owners and never release after failed STOP. */
static void test_abi_lifetime(void)
{
    static const val_firme_abi_descriptor_t abi = {.id = VAL_FIRME_ABI_ID_IDE_KEYSET_GO};
    static const val_firme_abi_case_t test_case = {.mutation = VAL_FIRME_MUTATION_NONE};
    static const val_firme_abi_test_plan_t plan = {.name = "native retained IDE owner"};
    static const val_firme_abi_case_t bad_ecam = {.mutation = VAL_FIRME_MUTATION_IDE_INVALID_ECAM};
    val_firme_abi_test_context_t context = {.abi = &abi, .test_case = &test_case,
                                            .plan = &plan};
    val_firme_abi_test_context_t next = context;
    uint64_t handle;
    uint32_t calls;
    uint32_t polls;

    test_reset();
    g_test_pe = 5U;
    g_poll_status = FIRME_INCOMPLETE;
    assert(val_firme_ide_abi_prepare(&context) == VAL_ERROR);
    assert(context.call_issued == VAL_FALSE);
    assert(context.output.registers[0] == FIRME_SUCCESS);
    assert(val_firme_ide_abi_cleanup(&context) == VAL_ERROR);
    assert(g_ide_abi_retained_valid && g_pal_cleanup_calls == 0U);
    assert(g_ide_abi_retained.scratch[IDE_ABI_MAIN_TRACKED] == VAL_FALSE);
    assert(g_ide_abi_retained.scratch[IDE_ABI_ACTIVE] == VAL_FALSE);
    assert(g_ide_abi_retained.scratch[IDE_ABI_PENDING_OP] == PAL_FIRME_IDE_ABI_PROG + 1U);
    handle = context.scratch[IDE_ABI_HANDLE];
    calls = g_pal_prepare_calls;
    assert(val_firme_ide_abi_prepare(&next) == VAL_ERROR);
    assert(g_pal_prepare_calls == calls);
    assert(g_ide_abi_retained.scratch[IDE_ABI_HANDLE] == handle);
    assert(g_ide_abi_retained.scratch[IDE_ABI_KEY_FIRST] == 1ULL);
    g_poll_status = FIRME_SUCCESS;
    polls = g_poll_calls;
    g_test_pe = 0U;
    assert(val_firme_ide_abi_cleanup(&g_ide_abi_retained) == VAL_ERROR);
    assert(val_firme_ide_abi_prepare(&next) == VAL_ERROR);
    assert(g_poll_calls == polls && g_pal_prepare_calls == calls);
    assert(g_ide_abi_owned && g_ide_abi_owner_pe == 5U);
    g_test_pe = 5U;
    assert(val_firme_ide_abi_cleanup(&g_ide_abi_retained) == VAL_SUCCESS);
    assert(g_stop_calls == 1U && g_pal_cleanup_calls == 1U);
    assert(!g_ide_abi_retained_valid);

    context = next;
    assert(val_firme_ide_abi_prepare(&context) == VAL_SUCCESS);
    g_test_pe = 0U;
    assert(val_firme_ide_abi_build(&context) == VAL_ERROR);
    assert(val_firme_ide_abi_cleanup(&context) == VAL_ERROR);
    g_test_pe = 5U;
    assert(val_firme_ide_abi_build(&context) == VAL_SUCCESS);
    assert(context.scratch[IDE_ABI_HANDLE] != handle);
    context.call_issued = VAL_TRUE;
    context.output.registers[0] = FIRME_SUCCESS;
    g_pal_observe_status = PAL_FIRME_IDE_ABI_UNAVAILABLE;
    assert(val_firme_ide_abi_validate(&context) == VAL_FALSE);
    assert(g_pal_observe_calls == 1U);
    g_operation_status = FIRME_DENIED;
    calls = g_pal_cleanup_calls;
    assert(val_firme_ide_abi_cleanup(&context) == VAL_ERROR);
    assert(g_pal_cleanup_calls == calls);
    assert(g_ide_abi_retained.scratch[IDE_ABI_ACTIVE]);
    assert(context.output.registers[0] == FIRME_SUCCESS);
    g_operation_status = FIRME_INCOMPLETE;
    g_poll_status = FIRME_INCOMPLETE;
    assert(val_firme_ide_abi_cleanup(&g_ide_abi_retained) == VAL_ERROR);
    calls = g_stop_calls;
    g_poll_status = FIRME_SUCCESS;
    g_pal_cleanup_status = PAL_FIRME_IDE_ABI_ERROR;
    assert(val_firme_ide_abi_cleanup(&g_ide_abi_retained) == VAL_ERROR);
    assert(g_stop_calls == calls);
    assert(g_ide_abi_retained.scratch[IDE_ABI_TOKEN] == 99ULL);
    g_pal_cleanup_status = PAL_FIRME_IDE_ABI_READY;
    assert(val_firme_ide_abi_cleanup(&g_ide_abi_retained) == VAL_SUCCESS);
    assert(g_stop_calls == calls);

    context = next;
    assert(val_firme_ide_abi_prepare(&context) == VAL_SUCCESS);
    assert(val_firme_ide_abi_build(&context) == VAL_SUCCESS);
    context.call_issued = VAL_TRUE;
    context.output.registers[0] = FIRME_SUCCESS;
    g_operation_status = FIRME_DENIED;
    assert(val_firme_ide_abi_cleanup(&context) == VAL_ERROR);
    assert(g_ide_abi_retained.call_issued == VAL_TRUE);
    assert(g_ide_abi_retained.scratch[IDE_ABI_MAIN_TRACKED] == VAL_TRUE);
    assert(g_ide_abi_retained.scratch[IDE_ABI_ACTIVE] == VAL_TRUE);
    assert(context.output.registers[0] == FIRME_SUCCESS);
    g_operation_status = FIRME_INCOMPLETE;
    assert(val_firme_ide_abi_cleanup(&g_ide_abi_retained) == VAL_SUCCESS);

    context = next;
    g_test_pe = 6U;
    g_bad_exact_handle = VAL_TRUE;
    assert(val_firme_ide_abi_prepare(&context) == VAL_ERROR);
    assert(val_firme_ide_abi_cleanup(&context) == VAL_ERROR);
    calls = g_poll_calls;
    assert(val_firme_ide_abi_prepare(&next) == VAL_ERROR);
    assert(g_poll_calls == calls);
    assert(g_ide_abi_retained.scratch[IDE_ABI_PENDING_OP] == IDE_ABI_UNCERTAIN);
    context = next;
    context.test_case = &bad_ecam;
    val_firme_ide_abi_track_main(&context, PAL_FIRME_IDE_ABI_GO, FIRME_SUCCESS);
    assert(context.scratch[IDE_ABI_PENDING_OP] == IDE_ABI_UNCERTAIN);
    assert(context.scratch[IDE_ABI_PENDING]);
    g_test_pe = 0U;
}

/** @brief Drain late completions without reissuing accepted operations. */
static void test_link_drain(void)
{
    firme_acs_call_res_t result;
    uint64_t keyset;
    uint64_t handle;
    uint32_t calls;

    memset(g_val_ide_link, 0, sizeof(g_val_ide_link));
    test_reset();
    g_test_pe = 3U;
    keyset = test_link_prepare(0U);
    g_poll_status = FIRME_INCOMPLETE;
    val_firme_ide_link_command(VAL_IDE_LINK_PROG, keyset, 0ULL, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_FAIL && g_val_ide_link[0].pending);
    handle = g_val_ide_link[0].args.handle;
    g_poll_status = FIRME_SUCCESS;
    calls = g_poll_calls;
    g_test_pe = 0U;
    val_firme_ide_link_command(VAL_IDE_LINK_DRAIN, keyset, 0ULL, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_FAIL && g_poll_calls == calls);
    assert(g_val_ide_link[0].pending && g_val_ide_link[0].pe_index == 3U);
    g_test_pe = 3U;
    val_firme_ide_link_command(VAL_IDE_LINK_DRAIN, keyset, 0ULL, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_PASS && g_val_ide_link[0].programmed);
    assert(g_val_ide_link[0].args.handle == handle);
    assert(g_val_ide_link[0].args.keyqw0 == 0ULL);
    g_poll_status = FIRME_INCOMPLETE;
    val_firme_ide_link_command(VAL_IDE_LINK_GO, keyset, 0ULL, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_FAIL && g_val_ide_link[0].pending);
    assert(g_val_ide_link[0].args.handle != handle);
    g_poll_status = FIRME_SUCCESS;
    val_firme_ide_link_command(VAL_IDE_LINK_QUIESCE, keyset, 0ULL, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_PASS && g_stop_calls == 1U);
    assert(g_val_ide_link[0].stopped && !g_val_ide_link[0].pending);
    val_firme_ide_link_command(VAL_IDE_LINK_RELEASE, keyset, 0ULL, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_PASS);

    g_test_pe = 4U;
    keyset = test_link_prepare(0U);
    val_firme_ide_link_command(VAL_IDE_LINK_PROG, keyset, 0ULL, 0ULL, &result);
    g_poll_status = FIRME_INCOMPLETE;
    val_firme_ide_link_command(VAL_IDE_LINK_STOP, keyset, 0ULL, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_FAIL);
    calls = g_stop_calls;
    g_poll_status = FIRME_SUCCESS;
    val_firme_ide_link_command(VAL_IDE_LINK_QUIESCE, keyset, 0ULL, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_PASS && g_stop_calls == calls);
    val_firme_ide_link_command(VAL_IDE_LINK_RELEASE, keyset, 0ULL, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_PASS);

    keyset = test_link_prepare(0U);
    g_poll_status = FIRME_ABORTED;
    val_firme_ide_link_command(VAL_IDE_LINK_PROG, keyset, 0ULL, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_FAIL && g_val_ide_link[0].quarantined);
    calls = g_poll_calls;
    val_firme_ide_link_command(VAL_IDE_LINK_DRAIN, keyset, 0ULL, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_FAIL && g_poll_calls == calls);
    assert(g_val_ide_link[0].args.keyqw0 == 1ULL);
    g_test_pe = 0U;
}

/** @brief Preserve namespace owners and reject one bidirectional link as two paths. */
static void test_namespace_lifetime(void)
{
    firme_rule_res_t result;
    firme_res_t response;
    pal_firme_ide_namespace_scenario_t scenario;
    uint32_t prepares;
    uint32_t releases;
    uint32_t polls;

    test_reset();
    g_test_pe = 7U;
    assert(val_firme_ide_namespace_run(PAL_FIRME_IDE_NAMESPACE_BY_KEYSET, &result) ==
           VAL_SUCCESS);
    assert(result.status == ACS_STATUS_SKIP);
    g_namespace_available = VAL_TRUE;
    assert(pal_firme_ide_namespace_prepare(PAL_FIRME_IDE_NAMESPACE_BY_KEYSET,
                                           &scenario) == PAL_FIRME_IDE_NAMESPACE_READY);
    assert(ide_namespace_paths_valid(PAL_FIRME_IDE_NAMESPACE_BY_KEYSET, &scenario));
    scenario.path[1].keyset_id = scenario.path[0].keyset_id ^ 2ULL;
    assert(!ide_namespace_paths_valid(PAL_FIRME_IDE_NAMESPACE_BY_KEYSET, &scenario));

    g_poll_status = FIRME_INCOMPLETE;
    assert(val_firme_ide_namespace_run(PAL_FIRME_IDE_NAMESPACE_BY_KEYSET, &result) == VAL_ERROR);
    assert(result.fail_step == IDE_NAMESPACE_STEP_PROGRAM && !g_namespace_releases);
    assert(g_ide_namespace_owner.pending[0]);
    assert(g_ide_namespace_owner.scenario.path[0].keyqw[0] == 1ULL);
    prepares = g_namespace_prepares;
    assert(val_firme_ide_namespace_run(PAL_FIRME_IDE_NAMESPACE_BY_KEYSET, &result) == VAL_ERROR);
    assert(g_namespace_prepares == prepares && !g_namespace_releases);
    g_poll_status = FIRME_SUCCESS;
    polls = g_poll_calls;
    g_test_pe = 0U;
    assert(val_firme_ide_namespace_run(PAL_FIRME_IDE_NAMESPACE_BY_KEYSET, &result) == VAL_ERROR);
    assert(g_namespace_prepares == prepares && g_poll_calls == polls);
    assert(g_ide_namespace_owner.pending[0] && g_ide_namespace_owner.pe_index == 7U);
    g_test_pe = 7U;
    g_operation_status = FIRME_DENIED;
    assert(ide_namespace_cleanup(&response) == VAL_ERROR);
    assert(g_ide_namespace_owner.programmed[0] && !g_namespace_releases);
    g_operation_status = FIRME_INCOMPLETE;
    assert(ide_namespace_cleanup(&response) == VAL_SUCCESS);
    assert(g_namespace_releases == 1U && !g_ide_namespace_owner.prepared);

    g_test_pe = 6U;
    assert(val_firme_ide_namespace_run(PAL_FIRME_IDE_NAMESPACE_BY_KEYSET, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    g_namespace_observation = PAL_FIRME_IDE_NAMESPACE_UNAVAILABLE;
    assert(val_firme_ide_namespace_run(PAL_FIRME_IDE_NAMESPACE_BY_KEYSET, &result) == VAL_ERROR);
    assert(result.fail_step == IDE_NAMESPACE_STEP_BOTH_ACTIVE);

    g_bad_exact_handle = VAL_TRUE;
    releases = g_namespace_releases;
    assert(val_firme_ide_namespace_run(PAL_FIRME_IDE_NAMESPACE_BY_KEYSET, &result) == VAL_ERROR);
    assert(g_ide_namespace_owner.quarantined[0]);
    assert(g_namespace_releases == releases);
    prepares = g_namespace_prepares;
    assert(val_firme_ide_namespace_run(PAL_FIRME_IDE_NAMESPACE_BY_KEYSET, &result) == VAL_ERROR);
    assert(g_namespace_prepares == prepares && g_namespace_releases == releases);
    g_test_pe = 0U;
}

/** @brief Keep Secure-caller function returns consistent with every rule result. */
static void test_secure_caller_return(void)
{
    const struct {
        uint64_t fid;
        uint64_t step;
    } cases[] = {
        {FIRME_FID_IDE_KEYSET_PROG, IDE_SECURE_CALLER_STEP_PROG},
        {FIRME_FID_IDE_KEYSET_GO, IDE_SECURE_CALLER_STEP_GO},
        {FIRME_FID_IDE_KEYSET_STOP, IDE_SECURE_CALLER_STEP_STOP},
        {FIRME_FID_IDE_KEYSET_POLL, IDE_SECURE_CALLER_STEP_POLL},
    };
    firme_rule_res_t result;
    uint32_t case_index;

    for (case_index = 0U; case_index < sizeof(cases) / sizeof(cases[0]); case_index++) {
        test_reset();
        g_secure_caller = VAL_TRUE;
        g_secure_unexpected_fid = cases[case_index].fid;
        assert(firme_ide_secure_caller_not_supported_run_secure(0ULL, &result) == VAL_ERROR);
        assert(result.status == ACS_STATUS_FAIL);
        assert(result.fail_step == cases[case_index].step);
        assert(val_firme_status_to_i64(result.last_x0) == FIRME_SUCCESS);
    }

    test_reset();
    g_secure_caller = VAL_TRUE;
    assert(firme_ide_secure_caller_not_supported_run_secure(0ULL, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    assert(result.fail_step == 0ULL);
}

/** @brief Validate every CXL PROG, GO and STOP completion handle independently. */
static void test_cxl_completion(void)
{
    const struct {
        int (*run)(uint64_t base_pa, firme_rule_res_t *out);
        uint64_t handle;
        uint64_t step;
    } cases[] = {
        {firme_ide_cxl_substream_id_run_realm, IDE_CXL_SUBSTREAM_HANDLE_BASELINE,
         IDE_CXL_SUBSTREAM_STEP_BASELINE},
        {firme_ide_cxl_stream_id_zero_run_realm, IDE_CXL_STREAM_HANDLE_BASELINE,
         IDE_CXL_STREAM_STEP_BASELINE},
        {firme_ide_cxl_key_set_unused_run_realm, IDE_CXL_KEY_SET_HANDLE_BASELINE,
         IDE_CXL_KEY_SET_STEP_BASELINE},
        {firme_ide_cxl_key_set_unused_run_realm, IDE_CXL_KEY_SET_HANDLE_GO,
         IDE_CXL_KEY_SET_STEP_GO},
        {firme_ide_cxl_key_set_unused_run_realm, IDE_CXL_KEY_SET_HANDLE_STOP,
         IDE_CXL_KEY_SET_STEP_STOP},
    };
    firme_rule_res_t result;
    uint32_t case_index;

    for (case_index = 0U; case_index < sizeof(cases) / sizeof(cases[0]); case_index++) {
        test_reset();
        g_bad_handle_request = cases[case_index].handle;
        assert(cases[case_index].run(0ULL, &result) == VAL_ERROR);
        assert(result.status == ACS_STATUS_FAIL);
        assert(result.fail_step == cases[case_index].step);

        test_reset();
        assert(cases[case_index].run(0ULL, &result) == VAL_SUCCESS);
        assert(result.status == ACS_STATUS_PASS);
        assert(g_stop_calls == 1U && !g_pending && !g_cxl_test_reserved);
        if (case_index == 0U)
            assert(g_cxl_invalid_calls == VAL_FIRME_IDE_KEYSET_SUBSTREAM_MASK);
        if (case_index == 1U)
            assert(g_cxl_invalid_calls == VAL_FIRME_IDE_KEYSET_STREAM_MASK);

        test_reset();
        g_program_status = FIRME_SUCCESS;
        g_operation_status = FIRME_SUCCESS;
        assert(cases[case_index].run(0ULL, &result) == VAL_SUCCESS);
        assert(result.status == ACS_STATUS_PASS);
        assert(g_stop_calls == 1U && !g_pending && !g_cxl_test_reserved);
    }

    for (case_index = 0U; case_index < 3U; case_index++) {
        test_reset();
        g_poll_status = FIRME_ABORTED;
        assert(cases[case_index].run(0ULL, &result) == VAL_ERROR);
        assert(result.status == ACS_STATUS_FAIL);
        assert(result.fail_step == cases[case_index].step);
    }

    /* An error after accepted PROG must still STOP and preserve its diagnosis. */
    test_reset();
    g_cxl_bad_go = VAL_TRUE;
    assert(firme_ide_cxl_key_set_unused_run_realm(0, &result) == VAL_ERROR);
    assert(result.fail_step == IDE_CXL_KEY_SET_STEP_GO);
    assert(g_stop_calls == 1U && !g_pending && !g_cxl_test_reserved);

    for (case_index = 0U; case_index < 2U; case_index++) {
        test_reset();
        g_cxl_reject_invalid_status = VAL_TRUE;
        assert(cases[case_index].run(0, &result) == VAL_ERROR);
        assert(result.status == ACS_STATUS_FAIL);
        assert(g_stop_calls == 1U && !g_pending && !g_cxl_test_reserved);

        /* Unexpected acceptance is quarantined, not polled using malformed input. */
        test_reset();
        g_cxl_accept_invalid = VAL_TRUE;
        assert(cases[case_index].run(0, &result) == VAL_ERROR);
        assert(g_cxl_test_reserved && g_poll_calls == 1U && !g_stop_calls);
        assert(cases[case_index].run(0, &result) == VAL_ERROR);
        assert(g_poll_calls == 1U);
    }

    /* Failed STOP or a persistent timeout prevents later baseline reuse. */
    test_reset();
    g_cxl_bad_stop = VAL_TRUE;
    assert(firme_ide_cxl_stream_id_zero_run_realm(0, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && g_cxl_test_reserved);
    assert(result.fail_step == IDE_CXL_STREAM_STEP_CLEANUP);
    test_reset();
    g_poll_status = FIRME_BUSY;
    assert(firme_ide_cxl_stream_id_zero_run_realm(0, &result) == VAL_ERROR);
    assert(result.fail_step == IDE_CXL_STREAM_STEP_BASELINE);
    assert(g_pending && g_cxl_test_reserved && !g_stop_calls);
    test_reset();
    g_bad_exact_handle = VAL_TRUE;
    assert(firme_ide_cxl_stream_id_zero_run_realm(0, &result) == VAL_ERROR);
    assert(g_cxl_test_reserved && !g_stop_calls);
}

/**
 * @brief Load a distinct CXL direction without programming the mock Root Port.
 * @return The encoded keyset owned by that direction.
 */
static uint64_t test_link_prepare(uint32_t direction)
{
    val_firme_ide_keyset_fields_t fields = {0};
    firme_acs_call_res_t result;
    uint64_t keyset;

    g_test_controller = g_test_pe;
    fields.root_port_id = val_pcie_default_root_port_id();
    fields.substream_id = VAL_FIRME_IDE_CXL_SUBSTREAM_ID;
    fields.direction = direction;
    keyset = val_firme_ide_keyset_id(&fields);
    val_firme_ide_link_command(VAL_IDE_LINK_BEGIN, keyset, TEST_ECAM, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_PASS);
    val_firme_ide_link_command(VAL_IDE_LINK_KEY_LOW, keyset, 1ULL, 2ULL, &result);
    assert(result.x0 == ACS_STATUS_PASS);
    val_firme_ide_link_command(VAL_IDE_LINK_KEY_HIGH, keyset, 3ULL, 4ULL, &result);
    assert(result.x0 == ACS_STATUS_PASS);
    return keyset;
}

/** @brief Retain key ownership on bad echoes and unfinished link operations. */
static void test_link_retention(void)
{
    firme_acs_call_res_t result;
    uint64_t keyset;

    test_reset();
    keyset = test_link_prepare(0U);
    g_bad_exact_handle = VAL_TRUE;
    val_firme_ide_link_command(VAL_IDE_LINK_PROG, keyset, 0ULL, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_FAIL);
    assert(g_val_ide_link[0].pending == VAL_TRUE);
    assert(g_val_ide_link[0].programmed == VAL_FALSE);
    assert(g_val_ide_link[0].args.keyqw0 == 1ULL);
    assert(g_val_ide_link[0].args.keyqw3 == 4ULL);
    val_firme_ide_link_command(VAL_IDE_LINK_RELEASE, keyset, 0ULL, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_FAIL);
    assert(g_val_ide_link[0].open == VAL_TRUE);

    test_reset();
    keyset = test_link_prepare(1U);
    g_poll_status = FIRME_INCOMPLETE;
    val_firme_ide_link_command(VAL_IDE_LINK_PROG, keyset, 0ULL, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_FAIL);
    assert(g_val_ide_link[1].pending == VAL_TRUE);
    assert(g_val_ide_link[1].args.keyqw0 == 1ULL);
    assert(g_val_ide_link[1].args.handle != g_val_ide_link[0].args.handle);
    val_firme_ide_link_command(VAL_IDE_LINK_KEY_LOW, keyset, 5ULL, 6ULL, &result);
    assert(result.x0 == ACS_STATUS_FAIL);
    assert(g_val_ide_link[1].args.keyqw0 == 1ULL);

    memset(g_val_ide_link, 0, sizeof(g_val_ide_link));
    test_reset();
    keyset = test_link_prepare(0U);
    val_firme_ide_link_command(VAL_IDE_LINK_PROG, keyset, 0ULL, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_PASS);
    assert(g_val_ide_link[0].pending == VAL_FALSE);
    assert(g_val_ide_link[0].programmed == VAL_TRUE);
    assert(g_val_ide_link[0].args.keyqw0 == 0ULL);
    g_bad_exact_handle = VAL_TRUE;
    val_firme_ide_link_command(VAL_IDE_LINK_STOP, keyset, 0ULL, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_FAIL);
    assert(g_val_ide_link[0].pending == VAL_TRUE);
    assert(g_val_ide_link[0].stopped == VAL_FALSE);
    val_firme_ide_link_command(VAL_IDE_LINK_RELEASE, keyset, 0ULL, 0ULL, &result);
    assert(result.x0 == ACS_STATUS_FAIL);
}

/**
 * @brief Check handle correlation, conditional x2, and synchronous POLL skips.
 * @return VAL_SUCCESS after all rule-result assertions pass.
 */
int main(void)
{
    firme_rule_res_t result;
    val_firme_abi_test_plan_t plan = {.name = "native IDE retry"};
    val_firme_abi_test_context_t context = {.plan = &plan};
    uint32_t status_index;
    const int64_t failures[] = {FIRME_BUSY, FIRME_ABORTED, FIRME_DENIED,
                                FIRME_OP_CONFLICT, FIRME_NOT_SUPPORTED,
                                TEST_UNKNOWN_STATUS};

    test_reset();
    g_bad_exact_handle = VAL_TRUE;
    assert(firme_ide_keyset_poll_status_run_realm(0ULL, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    assert(result.fail_step == IDE_KEYSET_POLL_STEP_EXACT_SUCCESS);

    test_reset();
    assert(firme_ide_keyset_poll_status_run_realm(0ULL, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);

    test_reset();
    g_bad_any_handle = VAL_TRUE;
    assert(firme_ide_keyset_poll_status_run_realm(0ULL, &result) == VAL_ERROR);
    assert(result.fail_step == IDE_KEYSET_POLL_STEP_ANY_SUCCESS);
    test_reset();
    g_bad_any_keyset = VAL_TRUE;
    assert(firme_ide_keyset_poll_status_run_realm(0ULL, &result) == VAL_ERROR);
    assert(result.fail_step == IDE_KEYSET_POLL_STEP_ANY_SUCCESS);

    test_reset();
    g_poll_status = FIRME_ABORTED;
    assert(firme_ide_keyset_poll_status_run_realm(0ULL, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);

    test_reset();
    g_bad_exact_handle = VAL_TRUE;
    assert(firme_ide_keyset_prog_status_run_realm(0ULL, &result) == VAL_ERROR);
    assert(result.fail_step == IDE_KEYSET_PROG_STEP_POLL);
    test_reset();
    assert(firme_ide_keyset_prog_status_run_realm(0ULL, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);

    test_reset();
    g_bad_exact_handle = VAL_TRUE;
    assert(firme_ide_keyset_go_status_run_realm(0ULL, &result) == VAL_ERROR);
    assert(result.fail_step == IDE_KEYSET_GO_STEP_PROGRAM);
    test_reset();
    g_program_status = FIRME_SUCCESS;
    g_bad_exact_handle = VAL_TRUE;
    assert(firme_ide_keyset_go_status_run_realm(0ULL, &result) == VAL_ERROR);
    assert(result.fail_step == IDE_KEYSET_GO_STEP_POLL);
    test_reset();
    assert(firme_ide_keyset_go_status_run_realm(0ULL, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);

    test_reset();
    g_bad_exact_handle = VAL_TRUE;
    assert(firme_ide_keyset_stop_status_run_realm(0ULL, &result) == VAL_ERROR);
    assert(result.fail_step == IDE_KEYSET_STOP_STEP_PROGRAM);
    test_reset();
    g_program_status = FIRME_SUCCESS;
    g_bad_exact_handle = VAL_TRUE;
    assert(firme_ide_keyset_stop_status_run_realm(0ULL, &result) == VAL_ERROR);
    assert(result.fail_step == IDE_KEYSET_STOP_STEP_POLL);
    test_reset();
    assert(firme_ide_keyset_stop_status_run_realm(0ULL, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);

    test_reset();
    g_program_status = FIRME_SUCCESS;
    g_operation_status = FIRME_SUCCESS;
    assert(firme_ide_keyset_prog_status_run_realm(0ULL, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    assert(firme_ide_keyset_go_status_run_realm(0ULL, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    assert(firme_ide_keyset_stop_status_run_realm(0ULL, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    assert(firme_ide_keyset_poll_status_run_realm(0ULL, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_SKIP);

    test_reset();
    g_service_status = FIRME_INVALID_PARAMETERS;
    assert(firme_ide_keyset_poll_status_run_realm(0ULL, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    test_reset();
    g_service_status = FIRME_NOT_SUPPORTED;
    assert(firme_ide_keyset_poll_status_run_realm(0ULL, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_SKIP);
    assert(firme_ide_keyset_poll_status_run_realm(0ULL, NULL) == VAL_ERROR);

    for (status_index = 0U; status_index < sizeof(failures) / sizeof(failures[0]);
         status_index++) {
        test_reset();
        g_program_status = failures[status_index];
        assert(firme_ide_keyset_id_bitmap_encoding_run_realm(0ULL, &result) == VAL_ERROR);
        assert(result.status == ACS_STATUS_FAIL);
    }
    test_reset();
    g_poll_status = FIRME_ABORTED;
    assert(firme_ide_keyset_id_bitmap_encoding_run_realm(0ULL, &result) == VAL_ERROR);
    test_reset();
    g_bad_exact_handle = VAL_TRUE;
    assert(firme_ide_keyset_id_bitmap_encoding_run_realm(0ULL, &result) == VAL_ERROR);
    test_reset();
    assert(firme_ide_keyset_id_bitmap_encoding_run_realm(0ULL, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    test_reset();
    g_program_status = FIRME_SUCCESS;
    assert(firme_ide_keyset_id_bitmap_encoding_run_realm(0ULL, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);

    context.output.registers[0] = (uint64_t)FIRME_BUSY;
    g_frame_busy_count = VAL_FIRME_IDE_RETRY_LIMIT - 1U;
    g_frame_calls = 0U;
    assert(val_firme_ide_abi_retry_main(&context) == VAL_SUCCESS);
    assert(g_frame_calls == VAL_FIRME_IDE_RETRY_LIMIT);
    context.output.registers[0] = (uint64_t)FIRME_BUSY;
    g_frame_busy_count = VAL_FIRME_IDE_RETRY_LIMIT;
    g_frame_calls = 0U;
    assert(val_firme_ide_abi_retry_main(&context) == VAL_ERROR);
    assert(g_frame_calls == VAL_FIRME_IDE_RETRY_LIMIT);
    test_link_retention();
    test_link_drain();
    test_cxl_completion();
    test_secure_caller_return();
    test_abi_lifetime();
    test_namespace_lifetime();
    puts("IDE completion, retained ownership and namespace contracts: PASS");
    return VAL_SUCCESS;
}
