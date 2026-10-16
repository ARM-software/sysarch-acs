/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>

#define pal_firme_ide_abi_observe test_pal_ide_observe
#include "../../../val/src/val_firme_ide_abi_adapter.c"
#undef pal_firme_ide_abi_observe

#define TEST_ECAM UINT64_C(0x40000000)
#define TEST_KEYSET UINT64_C(0x12345)
#define TEST_HANDLE UINT64_C(0xabcdef)
#define TEST_UNUSED (UINT64_C(1) << 63)

static uint64_t g_keyset_error;
static uint64_t g_handle_error;
static int64_t g_go_status = FIRME_SUCCESS;
static uint32_t g_go_calls;

/** @brief Expose the mock namespace to the linked adapter's negative-case path. */
uint32_t pal_pcie_ecam_count(void)
{
    return 1U;
}

/** @brief Keep the adapter's inventory dependency independent of real platform MMIO. */
int pal_pcie_ecam_info(uint32_t index, pal_pcie_ecam_info_t *info)
{
    if (index || !info)
        return VAL_ERROR;
    *info = (pal_pcie_ecam_info_t){.ecam_base = TEST_ECAM,
                                 .ecam_size = UINT64_C(0x100000), .end_bus = 0U};
    return VAL_SUCCESS;
}

/** @brief Execute the decoding fixture on its owning primary PE. */
uint32_t val_pe_index(void)
{
    return 0U;
}

/** @brief Supply independent mock evidence after a correctly decoded POLL. */
pal_firme_ide_abi_status_t test_pal_ide_observe(
    uint32_t world, pal_firme_ide_abi_operation_t operation,
    pal_firme_ide_abi_condition_t condition, const pal_firme_ide_abi_scenario_t *scenario)
{
    (void)world;
    assert(operation == PAL_FIRME_IDE_ABI_POLL);
    assert(condition == PAL_FIRME_IDE_ABI_POLL_ANY);
    assert(scenario->keyset_id == TEST_KEYSET && scenario->handle == g_ide_abi_handle);
    return PAL_FIRME_IDE_ABI_READY;
}

/** @brief Reset ownership and pending state before each independent decode. */
static void test_pending(val_firme_abi_test_context_t *context)
{
    g_ide_abi_owned = VAL_TRUE;
    g_ide_abi_owner_pe = 0U;
    context->scratch[IDE_ABI_PENDING] = VAL_TRUE;
    context->scratch[IDE_ABI_PENDING_OP] = PAL_FIRME_IDE_ABI_PROG + 1U;
    context->scratch[IDE_ABI_HANDLE] = TEST_HANDLE;
    context->scratch[IDE_ABI_PROGRAMMED] = VAL_FALSE;
    context->scratch[IDE_ABI_ACTIVE] = VAL_FALSE;
}

/** @brief Decode the architected status from the mock transport. */
int64_t val_firme_status_to_i64(uint64_t value)
{
    return (int64_t)(int32_t)(uint32_t)value;
}

/** @brief Return a completed request with poisoned unused Keyset ID bits. */
void val_smccc_smc_call(uint64_t fid, uint64_t ecam, uint64_t flags,
                        uint64_t keyset, uint64_t arg4, uint64_t arg5,
                        uint64_t arg6, uint64_t arg7, smccc_res_t *res)
{
    if (fid == FIRME_FID_IDE_KEYSET_GO) {
        assert(ecam == TEST_ECAM && flags == VAL_FIRME_IDE_TYPE_SELECTIVE);
        assert(keyset == TEST_KEYSET && arg4 == g_ide_abi_handle);
        assert(arg5 == 0ULL && arg6 == 0ULL && arg7 == 0ULL);
        g_go_calls++;
        *res = (smccc_res_t){.x0 = (uint64_t)g_go_status};
        return;
    }
    assert(fid == FIRME_FID_IDE_KEYSET_POLL);
    assert(arg4 == 0ULL && arg5 == 0ULL && arg6 == 0ULL && arg7 == 0ULL);
    assert(ecam == TEST_ECAM && flags == 1ULL && keyset == 0ULL);
    *res = (smccc_res_t){.x0 = FIRME_SUCCESS, .x1 = TEST_HANDLE ^ g_handle_error,
        .x2 = TEST_UNUSED | (TEST_KEYSET ^ g_keyset_error)};
}

/** @brief Reject unexpected programming in these POLL-only decoding cases. */
void val_smccc_smc_call_x8(uint64_t fid, uint64_t arg1, uint64_t arg2,
                           uint64_t arg3, uint64_t arg4, uint64_t arg5,
                           uint64_t arg6, uint64_t arg7, uint64_t arg8,
                           smccc_res_t *res)
{
    (void)fid;
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    (void)arg6;
    (void)arg7;
    (void)arg8;
    (void)res;
    assert(0 && "POLL decoding must not program a keyset");
}

/** @brief Reject an unexpected retry in these terminal-response cases. */
void val_smccc_smc_call_frame(const smccc_call_frame_t *input, smccc_call_frame_t *output)
{
    (void)input;
    (void)output;
    assert(0 && "terminal response must not be retried");
}

/** @brief Supply the pre-MP retry wait hook without executing target instructions. */
void val_wfe(void)
{
}

/** @brief Supply the MP-aware retry relaxation hook. */
void val_mp_relax(void)
{
}

/** @brief Suppress expected negative-test diagnostics. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *format, ...)
{
    (void)verbosity;
    (void)tag;
    (void)format;
    return VAL_SUCCESS;
}

/** @brief Test both POLL decoding paths without relaxing defined output fields. */
int main(void)
{
    val_firme_abi_descriptor_t abi = {.id = VAL_FIRME_ABI_ID_IDE_KEYSET_POLL};
    val_firme_abi_case_t test_case = {.mutation = VAL_FIRME_MUTATION_IDE_POLL_ANY};
    val_firme_abi_test_plan_t plan = {.name = "native IDE ABI POLL"};
    val_firme_abi_test_context_t context = {
        .abi = &abi, .test_case = &test_case, .plan = &plan};

    context.scratch[IDE_ABI_ECAM] = TEST_ECAM;
    context.scratch[IDE_ABI_KEYSET] = TEST_KEYSET;
    context.scratch[IDE_ABI_HANDLE] = TEST_HANDLE;
    test_pending(&context);
    assert(val_firme_ide_abi_poll_complete(&context, VAL_TRUE) == VAL_SUCCESS);
    g_keyset_error = UINT64_C(1);
    test_pending(&context);
    assert(val_firme_ide_abi_poll_complete(&context, VAL_TRUE) == VAL_ERROR);
    g_keyset_error = 0ULL;
    g_handle_error = UINT64_C(1);
    test_pending(&context);
    assert(val_firme_ide_abi_poll_complete(&context, VAL_TRUE) == VAL_ERROR);
    g_handle_error = 0ULL;

    context.output.registers[0] = FIRME_SUCCESS;
    context.output.registers[1] = TEST_HANDLE;
    context.output.registers[2] = TEST_UNUSED | TEST_KEYSET;
    test_pending(&context);
    assert(val_firme_ide_abi_validate(&context) == VAL_TRUE);
    assert(g_go_calls == 1U && context.scratch[IDE_ABI_ACTIVE] == VAL_TRUE);
    context.output.registers[2] ^= UINT64_C(1);
    test_pending(&context);
    assert(val_firme_ide_abi_validate(&context) == VAL_FALSE);
    context.output.registers[1] = TEST_HANDLE;
    context.output.registers[2] = TEST_UNUSED | TEST_KEYSET;
    g_go_status = FIRME_DENIED;
    test_pending(&context);
    assert(val_firme_ide_abi_validate(&context) == VAL_FALSE);
    assert(g_go_calls == 2U && context.scratch[IDE_ABI_ACTIVE] == VAL_FALSE);
    context.output.registers[2] = TEST_UNUSED | TEST_KEYSET;
    context.output.registers[1] ^= UINT64_C(1);
    test_pending(&context);
    assert(val_firme_ide_abi_validate(&context) == VAL_FALSE);
    puts("Generated IDE ABI POLL masks: PASS");
    return VAL_SUCCESS;
}
