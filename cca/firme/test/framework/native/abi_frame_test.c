/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>

#include "pal_platform.h"
#include "val_firme_abi_test.h"
#include "val_firme_diag.h"
#include "val_firme_gpt_probe.h"
#include "val_firme_ide_link.h"
#include "val_firme_mp_test.h"
#include "val_firme_rmmd.h"
#include "val_firme_spmd_msg.h"
#include "val_common/val_smc_args.h"

#include "../../../val/src/val_firme_abi_frame.c"
#include "../../../val/host/val_host_abi_frame.c"

#define TEST_OWNER_PE 1U
#define TEST_OTHER_PE 0U
#define TEST_POISON UINT64_C(0xdecafbad12345678)
#define TEST_OUTPUT_XOR UINT64_C(0xfedcba9876543210)
#define TEST_SEQUENCE 1U
#define TEST_LAST_WORD (VAL_SMCCC_REGISTER_COUNT - 1U)
#define TEST_FRAME_DISPATCH_COUNT (2U * VAL_SMCCC_REGISTER_COUNT + 3U)

typedef enum {
    TEST_FAULT_NONE,
    TEST_DROP_BEFORE,
    TEST_DROP_AFTER,
    TEST_BAD_ACK,
    TEST_BAD_HALF,
    TEST_ERROR_AFTER,
} val_test_frame_fault_t;

static uint32_t g_test_pe;
static uint32_t g_test_dispatches;
static uint32_t g_test_smc_calls;
static uint32_t g_test_fault_operation;
static uint32_t g_test_fault_word;
static uint32_t g_test_fault_remaining;
static uint32_t g_test_reenter;
static firme_acs_world_t g_test_world;
static firme_acs_world_t g_test_executing_world;
static val_test_frame_fault_t g_test_fault;
static smccc_call_frame_t g_test_expected_input;

void rel2_handle_smc(smc_args_t *args);

/** @brief Select an arbitrary physical PE, rather than assuming the boot PE is zero. */
uint32_t val_pe_index(void)
{
    return g_test_pe;
}

/** @brief Construct one bounded command against the current physical owner. */
static firme_acs_call_res_t val_test_frame_command(uint32_t operation, uint64_t sequence,
                                                  uint32_t word, uint64_t low,
                                                  uint64_t high)
{
    firme_acs_call_res_t response;
    uint64_t control = ((uint64_t)g_test_pe << VAL_ABI_FRAME_OWNER_SHIFT) |
                       ((uint64_t)word << VAL_ABI_FRAME_WORD_SHIFT) |
                       ((uint64_t)g_test_world << VAL_ABI_FRAME_WORLD_SHIFT) | operation;

    val_firme_abi_frame_command(g_test_world, control, sequence, low, high, &response);
    return response;
}

/** @brief Execute a mock local SMC and exercise reentrancy without replaying it. */
void val_smccc_smc_call_frame(const smccc_call_frame_t *input, smccc_call_frame_t *output)
{
    firme_acs_call_res_t response;
    uint32_t word;

    g_test_smc_calls++;
    assert(g_test_executing_world == g_test_world);
    for (word = 0U; word < VAL_SMCCC_REGISTER_COUNT; word++) {
        assert(input->registers[word] == g_test_expected_input.registers[word]);
        output->registers[word] = input->registers[word] ^ TEST_OUTPUT_XOR;
    }

    if (g_test_reenter) {
        response = val_test_frame_command(VAL_ABI_FRAME_EXECUTE, TEST_SEQUENCE,
                                           0U, 0ULL, 0ULL);
        assert(response.x0 == ACS_STATUS_ERR);
        response = val_test_frame_command(VAL_ABI_FRAME_RELEASE, TEST_SEQUENCE,
                                           0U, 0ULL, 0ULL);
        assert(response.x0 == ACS_STATUS_ERR);
    }
}

/** @brief Emulate RMMD's limited returns and SMC32 truncation on the Secure path. */
void val_smccc_smc_call(uint64_t fid, uint64_t arg1, uint64_t arg2,
                        uint64_t arg3, uint64_t arg4, uint64_t arg5,
                        uint64_t arg6, uint64_t arg7, smccc_res_t *result)
{
    smc_args_t args;
    firme_acs_call_res_t response;
    uint64_t command = arg1;
    uint64_t control = arg2;
    uint64_t sequence = arg3;
    uint64_t low = arg4;
    uint64_t high = arg5;
    uint32_t operation;
    uint32_t word;
    uint32_t inject;

    if (fid == FFA_MSG_SEND_DIRECT_REQ_SMC32) {
        assert(arg1 == (((uint32_t)PLAT_FFA_NS_ENDPOINT_ID << 16) |
                        PLAT_FFA_ACS_SECURE_ENDPOINT_ID));
        assert(arg2 == 0ULL);
        command = (uint32_t)arg3;
        control = (uint32_t)arg4;
        sequence = (uint32_t)arg5;
        low = (uint32_t)arg6;
        high = (uint32_t)arg7;
        g_test_executing_world = FIRME_ACS_WORLD_SECURE;
    } else {
        assert(fid == FIRME_ACS_RMI_DISPATCH && arg6 == 0ULL && arg7 == 0ULL);
        g_test_executing_world = FIRME_ACS_WORLD_REALM;
    }

    assert(command == FIRME_ACS_CMD_ABI_FRAME);
    operation = (uint32_t)control & VAL_ABI_FRAME_OPERATION_MASK;
    word = ((uint32_t)control >> VAL_ABI_FRAME_WORD_SHIFT) & VAL_ABI_FRAME_WORD_MASK;
    inject = (operation == g_test_fault_operation) &&
             (word == g_test_fault_word) && (g_test_fault_remaining != 0U);
    g_test_dispatches++;
    if (inject)
        g_test_fault_remaining--;

    if (inject && g_test_fault == TEST_DROP_BEFORE) {
        *result = (smccc_res_t){.x0 = SMCCC_SMC_UNK};
        return;
    }

    args = (smc_args_t){
        .x = {fid, command, control, sequence, low, high, TEST_POISON, TEST_POISON}
    };
    if (g_test_executing_world == FIRME_ACS_WORLD_SECURE) {
        val_firme_abi_frame_command(FIRME_ACS_WORLD_SECURE, control, sequence,
                                    low, high, &response);
        args.x[3] = response.x0;
        args.x[4] = response.x1;
        args.x[5] = response.x2;
    } else {
        rel2_handle_smc(&args);
        assert(args.x[0] == RMM_RMI_REQ_COMPLETE);
    }

    if (inject && g_test_fault == TEST_DROP_AFTER) {
        *result = (smccc_res_t){.x0 = SMCCC_SMC_UNK};
        return;
    }

    if (inject && g_test_fault == TEST_BAD_ACK)
        args.x[4]++;
    if (inject && g_test_fault == TEST_BAD_HALF)
        args.x[5] = UINT64_C(1) << 32;
    if (inject && g_test_fault == TEST_ERROR_AFTER)
        args.x[3] = ACS_STATUS_ERR;

    if (g_test_executing_world == FIRME_ACS_WORLD_SECURE) {
        *result = (smccc_res_t){.x0 = FFA_MSG_SEND_DIRECT_RESP_SMC32,
            .x3 = (uint32_t)args.x[3], .x4 = (uint32_t)args.x[4],
            .x5 = (uint32_t)args.x[5], .x6 = TEST_POISON, .x7 = TEST_POISON};
    } else {
        *result = (smccc_res_t){.x0 = args.x[1], .x1 = args.x[2], .x2 = args.x[3],
            .x3 = args.x[4], .x4 = args.x[5], .x5 = TEST_POISON,
            .x6 = TEST_POISON, .x7 = TEST_POISON};
    }
}

/** @brief Reject diagnostics: generated ABI frames must use only the new command. */
int val_firme_diag_run(uint64_t test_id, uint64_t arg0, uint64_t arg1,
                       uint64_t arg2, uint64_t arg3, firme_diag_out_t *out)
{
    (void)test_id;
    (void)arg0;
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)out;
    assert(VAL_FALSE && "unexpected diagnostic command");
    return VAL_ERROR;
}

/** @brief Reject legacy link commands rather than silently providing a fallback. */
void val_firme_ide_link_command(uint64_t operation, uint64_t keyset_id,
                                uint64_t data0, uint64_t data1,
                                firme_acs_call_res_t *out)
{
    (void)operation;
    (void)keyset_id;
    (void)data0;
    (void)data1;
    (void)out;
    assert(VAL_FALSE && "unexpected legacy link command");
}

/** @brief Reject memory probes: a frame must execute only its explicitly uploaded ABI. */
void val_firme_gpt_probe_command(uint64_t address, firme_acs_call_res_t *response)
{
    (void)address;
    (void)response;
    assert(VAL_FALSE && "unexpected GPT probe command");
}

/** @brief Reject execution of whole remote rules instead of individual ABI calls. */
int val_firme_rule_run_realm(firme_rule_id_t rule, uint64_t base_pa, firme_rule_res_t *out)
{
    (void)rule;
    (void)base_pa;
    (void)out;
    assert(VAL_FALSE && "unexpected RUN_RULE command");
    return VAL_ERROR;
}

/** @brief Reject MP-test dispatch; frame ownership comes from the physical PE. */
void val_firme_test_payload_command(firme_acs_world_t world, firme_acs_cmd_t command,
                                     uint64_t rule_id, uint64_t phase,
                                     uint64_t argument, uint64_t pe_index,
                                     firme_acs_call_res_t *response)
{
    (void)world;
    (void)command;
    (void)rule_id;
    (void)phase;
    (void)argument;
    (void)pe_index;
    (void)response;
    assert(VAL_FALSE && "unexpected MP-test command");
}

/** @brief Reset only the native fixture, including intentionally quarantined slots. */
static void val_test_frame_reset(void)
{
    uint32_t world_index;
    uint32_t pe_index;
    uint32_t word;

    for (world_index = 0U; world_index < VAL_ABI_FRAME_WORLD_COUNT; world_index++) {
        for (pe_index = 0U; pe_index < PAL_PE_COUNT; pe_index++) {
            g_abi_frame_slots[world_index][pe_index] = (val_firme_abi_frame_slot_t){0};
            g_host_abi_frame_owners[world_index][pe_index] = (val_host_abi_frame_owner_t){0};
        }
    }

    g_test_pe = TEST_OWNER_PE;
    g_test_world = FIRME_ACS_WORLD_REALM;
    g_test_executing_world = FIRME_ACS_WORLD_REALM;
    g_test_dispatches = 0U;
    g_test_smc_calls = 0U;
    g_test_fault_operation = 0U;
    g_test_fault_word = 0U;
    g_test_fault_remaining = 0U;
    g_test_fault = TEST_FAULT_NONE;
    g_test_reenter = VAL_FALSE;
    for (word = 0U; word < VAL_SMCCC_REGISTER_COUNT; word++)
        g_test_expected_input.registers[word] = TEST_POISON ^
            ((uint64_t)word << 32) ^ word;

    g_test_expected_input.registers[0] = FIRME_FID_IDE_KEYSET_PROG;
}

/** @brief Verify that release erased all retained key-bearing register words. */
static void val_test_frame_scrubbed(void)
{
    const val_firme_abi_frame_slot_t *slot =
        &g_abi_frame_slots[(uint32_t)g_test_world - 1U][g_test_pe];
    uint32_t word;

    assert(slot->phase == VAL_ABI_FRAME_RELEASED);
    assert(slot->written == 0U && slot->read == 0U && !slot->locked);
    for (word = 0U; word < VAL_SMCCC_REGISTER_COUNT; word++) {
        assert(slot->input.registers[word] == 0ULL);
        assert(slot->output.registers[word] == 0ULL);
    }
}

/** @brief Upload a complete frame directly for adversarial protocol sequencing. */
static void val_test_frame_upload(uint32_t sequence)
{
    firme_acs_call_res_t response;
    uint64_t value;
    uint32_t word;

    response = val_test_frame_command(VAL_ABI_FRAME_BEGIN, sequence, 0U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_PASS);
    for (word = 0U; word < VAL_SMCCC_REGISTER_COUNT; word++) {
        value = g_test_expected_input.registers[word];
        response = val_test_frame_command(VAL_ABI_FRAME_WRITE, sequence, word,
                                           (uint32_t)value, (uint32_t)(value >> 32));
        assert(response.x0 == ACS_STATUS_PASS);
    }
}

/** @brief Require a full 18-register round-trip and callback-type compatibility. */
static void val_test_frame_round_trip(firme_acs_world_t world)
{
    val_firme_abi_invoke_frame_fn_t callback = val_host_abi_frame_invoke;
    val_host_abi_frame_context_t context;
    smccc_call_frame_t input;
    smccc_call_frame_t output = {0};
    uint32_t issued = VAL_FALSE;
    uint32_t word;

    val_test_frame_reset();
    g_test_world = world;
    input = g_test_expected_input;
    assert(val_host_abi_frame_init_world(&context, world) == VAL_SUCCESS);
    assert(context.world == world);
    assert(callback(&input, &output, &issued, &context) == VAL_SUCCESS);
    assert(issued && context.outcome == VAL_ABI_FRAME_OUTPUT_VALID);
    assert(g_test_smc_calls == 1U && g_test_dispatches == TEST_FRAME_DISPATCH_COUNT);
    for (word = 0U; word < VAL_SMCCC_REGISTER_COUNT; word++) {
        assert(input.registers[word] == g_test_expected_input.registers[word]);
        assert(output.registers[word] == (input.registers[word] ^ TEST_OUTPUT_XOR));
    }

    val_test_frame_scrubbed();
    assert(callback(&input, &output, &issued, &context) == VAL_SUCCESS);
    assert(context.sequence == TEST_SEQUENCE + 1U && g_test_smc_calls == 2U);
    val_test_frame_scrubbed();
}

/** @brief Reject malformed input, stale identities and foreign physical owners. */
static void val_test_frame_rejections(void)
{
    firme_acs_call_res_t response;
    uint64_t control = ((uint64_t)TEST_OWNER_PE << VAL_ABI_FRAME_OWNER_SHIFT) |
                       ((uint64_t)FIRME_ACS_WORLD_REALM << VAL_ABI_FRAME_WORLD_SHIFT) |
                       VAL_ABI_FRAME_BEGIN;

    val_test_frame_reset();
    response = val_test_frame_command(VAL_ABI_FRAME_BEGIN, 0ULL, 0U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_ERR);
    response = val_test_frame_command(VAL_ABI_FRAME_BEGIN, UINT64_C(1) << 32,
                                       0U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_ERR);
    val_firme_abi_frame_command(g_test_world, control | (UINT64_C(1) << 32), TEST_SEQUENCE,
                                0ULL, 0ULL, &response);
    assert(response.x0 == ACS_STATUS_ERR);
    g_test_pe = TEST_OTHER_PE;
    val_firme_abi_frame_command(g_test_world, control, TEST_SEQUENCE, 0ULL, 0ULL, &response);
    assert(response.x0 == ACS_STATUS_ERR);
    g_test_pe = TEST_OWNER_PE;

    response = val_test_frame_command(VAL_ABI_FRAME_BEGIN, TEST_SEQUENCE, 0U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_PASS);
    response = val_test_frame_command(VAL_ABI_FRAME_BEGIN, TEST_SEQUENCE, 1U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_ERR);
    response = val_test_frame_command(VAL_ABI_FRAME_BEGIN, TEST_SEQUENCE, 0U, 1ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_ERR);
    response = val_test_frame_command(VAL_ABI_FRAME_BEGIN, TEST_SEQUENCE + 1U,
                                       0U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_ERR);
    response = val_test_frame_command(VAL_ABI_FRAME_WRITE, TEST_SEQUENCE,
                                       VAL_SMCCC_REGISTER_COUNT, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_ERR);
    response = val_test_frame_command(VAL_ABI_FRAME_WRITE, TEST_SEQUENCE,
                                       0U, UINT64_C(1) << 32, 0ULL);
    assert(response.x0 == ACS_STATUS_ERR);
    response = val_test_frame_command(VAL_ABI_FRAME_WRITE, TEST_SEQUENCE,
                                       0U, 0ULL, UINT64_C(1) << 32);
    assert(response.x0 == ACS_STATUS_ERR);
    response = val_test_frame_command(VAL_ABI_FRAME_WRITE, TEST_SEQUENCE,
                                       0U, FIRME_FID_IDE_KEYSET_PROG, 0ULL);
    assert(response.x0 == ACS_STATUS_PASS);
    response = val_test_frame_command(VAL_ABI_FRAME_WRITE, TEST_SEQUENCE,
                                       0U, FIRME_FID_IDE_KEYSET_PROG, 0ULL);
    assert(response.x0 == ACS_STATUS_PASS);
    response = val_test_frame_command(VAL_ABI_FRAME_WRITE, TEST_SEQUENCE,
                                       0U, FIRME_FID_IDE_KEYSET_STOP, 0ULL);
    assert(response.x0 == ACS_STATUS_ERR);
    response = val_test_frame_command(VAL_ABI_FRAME_EXECUTE, TEST_SEQUENCE,
                                       0U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_ERR && g_test_smc_calls == 0U);
    response = val_test_frame_command(VAL_ABI_FRAME_READ, TEST_SEQUENCE, 0U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_ERR);
    response = val_test_frame_command(VAL_ABI_FRAME_RELEASE, TEST_SEQUENCE,
                                       0U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_PASS);
    val_test_frame_scrubbed();
    response = val_test_frame_command(VAL_ABI_FRAME_BEGIN, TEST_SEQUENCE, 0U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_ERR);
    response = val_test_frame_command(VAL_ABI_FRAME_WRITE, TEST_SEQUENCE,
                                       0U, FIRME_FID_IDE_KEYSET_PROG, 0ULL);
    assert(response.x0 == ACS_STATUS_ERR);
}

/** @brief Prove cached EXECUTE and READ responses never replay the underlying SMC. */
static void val_test_frame_execute_once(void)
{
    firme_acs_call_res_t response;
    uint64_t value;
    uint32_t word;

    val_test_frame_reset();
    val_test_frame_upload(TEST_SEQUENCE);
    g_test_reenter = VAL_TRUE;
    response = val_test_frame_command(VAL_ABI_FRAME_EXECUTE, TEST_SEQUENCE,
                                       0U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_PASS && g_test_smc_calls == 1U);
    response = val_test_frame_command(VAL_ABI_FRAME_EXECUTE, TEST_SEQUENCE,
                                       0U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_PASS && g_test_smc_calls == 1U);
    response = val_test_frame_command(VAL_ABI_FRAME_WRITE, TEST_SEQUENCE,
                                       0U, FIRME_FID_IDE_KEYSET_PROG, 0ULL);
    assert(response.x0 == ACS_STATUS_ERR);
    response = val_test_frame_command(VAL_ABI_FRAME_BEGIN, TEST_SEQUENCE, 0U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_ERR);
    response = val_test_frame_command(VAL_ABI_FRAME_RELEASE, TEST_SEQUENCE,
                                       0U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_ERR);
    for (word = 0U; word < VAL_SMCCC_REGISTER_COUNT; word++) {
        response = val_test_frame_command(VAL_ABI_FRAME_READ, TEST_SEQUENCE,
                                           word, 0ULL, 0ULL);
        assert(response.x0 == ACS_STATUS_PASS);
        value = response.x1 | (response.x2 << 32);
        assert(value == (g_test_expected_input.registers[word] ^ TEST_OUTPUT_XOR));
    }

    response = val_test_frame_command(VAL_ABI_FRAME_READ, TEST_SEQUENCE,
                                       TEST_LAST_WORD, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_PASS);
    response = val_test_frame_command(VAL_ABI_FRAME_RELEASE, TEST_SEQUENCE,
                                       0U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_PASS);
    val_test_frame_scrubbed();
    response = val_test_frame_command(VAL_ABI_FRAME_RELEASE, TEST_SEQUENCE,
                                       0U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_PASS);
    val_test_frame_upload(TEST_SEQUENCE + 1U);
    response = val_test_frame_command(VAL_ABI_FRAME_RELEASE, TEST_SEQUENCE,
                                       0U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_ERR);
    assert(g_abi_frame_slots[(uint32_t)g_test_world - 1U][g_test_pe].phase ==
           VAL_ABI_FRAME_UPLOADING);
}

/** @brief Recover one dropped acknowledgement at every protocol stage. */
static void val_test_frame_retries(firme_acs_world_t world)
{
    val_host_abi_frame_context_t context;
    smccc_call_frame_t output;
    uint32_t issued;
    uint32_t operation;

    for (operation = VAL_ABI_FRAME_BEGIN; operation <= VAL_ABI_FRAME_RELEASE; operation++) {
        val_test_frame_reset();
        g_test_world = world;
        g_test_fault_operation = operation;
        g_test_fault_word = operation == VAL_ABI_FRAME_WRITE ||
                            operation == VAL_ABI_FRAME_READ ? TEST_LAST_WORD : 0U;
        g_test_fault_remaining = 1U;
        g_test_fault = TEST_DROP_AFTER;
        assert(val_host_abi_frame_init_world(&context, world) == VAL_SUCCESS);
        assert(val_host_abi_frame_invoke(&g_test_expected_input, &output,
                                          &issued, &context) == VAL_SUCCESS);
        assert(issued && g_test_smc_calls == 1U && !context.quarantined);
        assert(g_test_dispatches == TEST_FRAME_DISPATCH_COUNT + 1U);
        val_test_frame_scrubbed();
    }
}

/** @brief Retain a transaction and its fixture after unrecoverable issued failures. */
static void val_test_frame_uncertain(firme_acs_world_t world, uint32_t operation,
                                      val_test_frame_fault_t fault)
{
    val_host_abi_frame_context_t context;
    val_host_abi_frame_context_t replacement;
    smccc_call_frame_t output;
    uint32_t issued;
    uint32_t dispatches;
    uint32_t word;

    val_test_frame_reset();
    g_test_world = world;
    output = g_test_expected_input;
    assert(val_host_abi_frame_init_world(&context, world) == VAL_SUCCESS);
    assert(val_host_abi_frame_init_world(&replacement, world) == VAL_SUCCESS);
    g_test_fault_operation = operation;
    g_test_fault_word = operation == VAL_ABI_FRAME_READ ? TEST_LAST_WORD : 0U;
    g_test_fault_remaining = VAL_ABI_FRAME_RETRY_COUNT;
    g_test_fault = fault;
    assert(val_host_abi_frame_invoke(&g_test_expected_input, &output,
                                      &issued, &context) == VAL_ERROR);
    assert(issued && context.outcome == VAL_ABI_FRAME_UNCERTAIN && context.quarantined);
    assert(g_test_smc_calls == 1U);
    for (word = 0U; word < VAL_SMCCC_REGISTER_COUNT; word++)
        assert(output.registers[word] == g_test_expected_input.registers[word]);

    dispatches = g_test_dispatches;
    assert(val_host_abi_frame_invoke(&g_test_expected_input, &output,
                                      &issued, &context) == VAL_ERROR && issued);
    assert(val_host_abi_frame_init_world(&context, world) == VAL_ERROR);
    assert(val_host_abi_frame_invoke(&g_test_expected_input, &output,
                                      &issued, &replacement) == VAL_ERROR);
    assert(val_host_abi_frame_init_world(&replacement, world) == VAL_ERROR);
    assert(g_test_dispatches == dispatches && g_test_smc_calls == 1U);
}

/** @brief Keep pre-execution errors known-unissued and abort their upload safely. */
static void val_test_frame_unissued(void)
{
    val_host_abi_frame_context_t context;
    smccc_call_frame_t output;
    uint32_t issued;

    val_test_frame_reset();
    assert(val_host_abi_frame_init(&context) == VAL_SUCCESS);
    g_test_fault_operation = VAL_ABI_FRAME_WRITE;
    g_test_fault_word = TEST_LAST_WORD;
    g_test_fault_remaining = VAL_ABI_FRAME_RETRY_COUNT;
    g_test_fault = TEST_DROP_AFTER;
    assert(val_host_abi_frame_invoke(&g_test_expected_input, &output,
                                      &issued, &context) == VAL_ERROR);
    assert(!issued && context.outcome == VAL_ABI_FRAME_UNISSUED && !context.quarantined);
    assert(g_test_smc_calls == 0U);
    val_test_frame_scrubbed();
    assert(val_host_abi_frame_invoke(&g_test_expected_input, &output,
                                      &issued, &context) == VAL_SUCCESS);
    assert(issued && g_test_smc_calls == 1U);

    val_test_frame_reset();
    assert(val_host_abi_frame_init(&context) == VAL_SUCCESS);
    g_test_fault_operation = VAL_ABI_FRAME_BEGIN;
    g_test_fault_remaining = VAL_ABI_FRAME_RETRY_COUNT;
    g_test_fault = TEST_DROP_BEFORE;
    assert(val_host_abi_frame_invoke(&g_test_expected_input, &output,
                                      &issued, &context) == VAL_ERROR);
    assert(!issued && context.quarantined && g_test_smc_calls == 0U);
    assert(val_host_abi_frame_init(&context) == VAL_ERROR);
}

/** @brief Reject unsupported FIDs at both ends, not merely in the host callback. */
static void val_test_frame_allowlist(firme_acs_world_t world)
{
    const uint64_t allowed[] = {FIRME_FID_SERVICE_VERSION, FIRME_FID_SERVICE_FEATURES,
        FIRME_FID_GM_GPI_SET, FIRME_FID_GM_GPI_OP_CONTINUE, FIRME_FID_GM_GPT_INFO,
        FIRME_FID_GM_L1_GPT_CREATE, FIRME_FID_GM_L1_GPT_DESTROY,
        FIRME_FID_IDE_KEYSET_PROG, FIRME_FID_IDE_KEYSET_GO,
        FIRME_FID_IDE_KEYSET_STOP, FIRME_FID_IDE_KEYSET_POLL};
    const uint64_t denied[] = {FIRME_FID_MEC_REFRESH,
        FIRME_FID_ATTEST_PAT_GET, FIRME_FID_IDEV_OP_START, FIRME_ACS_RMI_DISPATCH,
        FIRME_FID_SERVICE_VERSION | (UINT64_C(1) << 32)};
    val_host_abi_frame_context_t context;
    firme_acs_call_res_t response;
    smccc_call_frame_t output;
    uint32_t issued;
    size_t index;

    for (index = 0U; index < sizeof(allowed) / sizeof(allowed[0]); index++) {
        val_test_frame_reset();
        g_test_world = world;
        g_test_expected_input.registers[0] = allowed[index];
        assert(val_host_abi_frame_init_world(&context, world) == VAL_SUCCESS);
        assert(val_host_abi_frame_invoke(&g_test_expected_input, &output,
                                          &issued, &context) == VAL_SUCCESS && issued);
    }

    for (index = 0U; index < sizeof(denied) / sizeof(denied[0]); index++) {
        val_test_frame_reset();
        g_test_world = world;
        g_test_expected_input.registers[0] = denied[index];
        assert(val_host_abi_frame_init_world(&context, world) == VAL_SUCCESS);
        assert(val_host_abi_frame_invoke(&g_test_expected_input, &output,
                                          &issued, &context) == VAL_ERROR && !issued);
        assert(g_test_dispatches == 0U);
        val_test_frame_upload(TEST_SEQUENCE);
        response = val_test_frame_command(VAL_ABI_FRAME_EXECUTE, TEST_SEQUENCE,
                                           0U, 0ULL, 0ULL);
        assert(response.x0 == ACS_STATUS_ERR && g_test_smc_calls == 0U);
    }
}

/** @brief Refuse token wrap and preserve each physical PE's independent sequence. */
static void val_test_frame_owner_and_wrap(void)
{
    val_host_abi_frame_context_t context;
    val_host_abi_frame_context_t other;
    firme_acs_call_res_t response;
    smccc_call_frame_t output;
    uint32_t issued;

    val_test_frame_reset();
    assert(val_host_abi_frame_init(&context) == VAL_SUCCESS);
    g_test_pe = TEST_OTHER_PE;
    assert(val_host_abi_frame_invoke(&g_test_expected_input, &output,
                                      &issued, &context) == VAL_ERROR && !issued);
    assert(val_host_abi_frame_init(&other) == VAL_SUCCESS);
    assert(val_host_abi_frame_invoke(&g_test_expected_input, &output,
                                      &issued, &other) == VAL_SUCCESS);
    assert(other.sequence == TEST_SEQUENCE);
    g_test_pe = TEST_OWNER_PE;
    assert(val_host_abi_frame_invoke(&g_test_expected_input, &output,
                                      &issued, &context) == VAL_SUCCESS);
    assert(context.sequence == TEST_SEQUENCE);
    g_host_abi_frame_owners[(uint32_t)g_test_world - 1U][g_test_pe].sequence = UINT32_MAX - 1U;
    assert(val_host_abi_frame_invoke(&g_test_expected_input, &output,
                                      &issued, &context) == VAL_SUCCESS);
    assert(context.sequence == UINT32_MAX);
    assert(val_host_abi_frame_invoke(&g_test_expected_input, &output,
                                      &issued, &context) == VAL_ERROR && !issued);
    assert(val_host_abi_frame_init(&context) == VAL_ERROR);
    response = val_test_frame_command(VAL_ABI_FRAME_BEGIN, TEST_SEQUENCE, 0U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_ERR);
    g_test_pe = PAL_PE_COUNT;
    assert(val_host_abi_frame_init(&other) == VAL_ERROR);
}

/** @brief Reject invalid callback arguments without publishing an earlier valid result. */
static void val_test_frame_argument_guards(void)
{
    val_host_abi_frame_context_t context = {0};
    smccc_call_frame_t output;
    uint32_t issued = VAL_TRUE;
    uint32_t dispatches;

    val_test_frame_reset();
    assert(val_host_abi_frame_init(NULL) == VAL_ERROR);
    assert(val_host_abi_frame_init_world(NULL, FIRME_ACS_WORLD_SECURE) == VAL_ERROR);
    assert(val_host_abi_frame_init_world(&context, FIRME_ACS_WORLD_NSEL2) == VAL_ERROR);
    assert(val_host_abi_frame_init_world(&context, (firme_acs_world_t)UINT32_MAX) == VAL_ERROR);
    assert(val_host_abi_frame_invoke(&g_test_expected_input, &output,
                                      &issued, &context) == VAL_ERROR && !issued);
    assert(val_host_abi_frame_init(&context) == VAL_SUCCESS);
    assert(context.world == FIRME_ACS_WORLD_REALM);
    assert(val_host_abi_frame_invoke(&g_test_expected_input, &output,
                                      &issued, &context) == VAL_SUCCESS);
    dispatches = g_test_dispatches;
    assert(val_host_abi_frame_invoke(NULL, &output, &issued, &context) == VAL_ERROR);
    assert(!issued && context.outcome == VAL_ABI_FRAME_UNISSUED);
    assert(val_host_abi_frame_invoke(&g_test_expected_input, NULL,
                                      &issued, &context) == VAL_ERROR && !issued);
    assert(val_host_abi_frame_invoke(&g_test_expected_input, &output,
                                      NULL, &context) == VAL_ERROR);
    assert(val_host_abi_frame_invoke(&g_test_expected_input, &output,
                                      &issued, NULL) == VAL_ERROR && !issued);
    assert(g_test_dispatches == dispatches);
}

/** @brief Keep equal PE/sequence values distinct across Secure and Realm slots. */
static void val_test_frame_world_isolation(void)
{
    val_host_abi_frame_context_t secure;
    val_host_abi_frame_context_t realm;
    firme_acs_call_res_t response;
    smccc_call_frame_t output;
    uint64_t control;
    uint32_t issued;

    val_test_frame_reset();
    val_test_frame_upload(TEST_SEQUENCE);
    g_test_world = FIRME_ACS_WORLD_SECURE;
    g_test_expected_input.registers[TEST_LAST_WORD] ^= TEST_OUTPUT_XOR;
    val_test_frame_upload(TEST_SEQUENCE);
    assert(g_abi_frame_slots[FIRME_ACS_WORLD_REALM - 1U][g_test_pe].input
               .registers[TEST_LAST_WORD] !=
           g_abi_frame_slots[FIRME_ACS_WORLD_SECURE - 1U][g_test_pe].input
               .registers[TEST_LAST_WORD]);

    control = ((uint64_t)g_test_pe << VAL_ABI_FRAME_OWNER_SHIFT) |
              ((uint64_t)FIRME_ACS_WORLD_REALM << VAL_ABI_FRAME_WORLD_SHIFT) |
              VAL_ABI_FRAME_RELEASE;
    val_firme_abi_frame_command(FIRME_ACS_WORLD_SECURE, control, TEST_SEQUENCE,
                                0ULL, 0ULL, &response);
    assert(response.x0 == ACS_STATUS_ERR);
    val_firme_abi_frame_command(FIRME_ACS_WORLD_NSEL2, control, TEST_SEQUENCE,
                                0ULL, 0ULL, &response);
    assert(response.x0 == ACS_STATUS_ERR);
    assert(g_abi_frame_slots[FIRME_ACS_WORLD_SECURE - 1U][g_test_pe].phase ==
           VAL_ABI_FRAME_UPLOADING);
    g_test_world = FIRME_ACS_WORLD_REALM;
    response = val_test_frame_command(VAL_ABI_FRAME_RELEASE, TEST_SEQUENCE, 0U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_PASS);
    val_test_frame_scrubbed();
    response = val_test_frame_command(VAL_ABI_FRAME_BEGIN, TEST_SEQUENCE, 0U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_ERR);
    g_test_world = FIRME_ACS_WORLD_SECURE;
    response = val_test_frame_command(VAL_ABI_FRAME_BEGIN, TEST_SEQUENCE, 0U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_PASS);
    response = val_test_frame_command(VAL_ABI_FRAME_RELEASE, TEST_SEQUENCE, 0U, 0ULL, 0ULL);
    assert(response.x0 == ACS_STATUS_PASS);
    val_test_frame_scrubbed();

    val_test_frame_reset();
    assert(val_host_abi_frame_init_world(&secure, FIRME_ACS_WORLD_SECURE) == VAL_SUCCESS);
    assert(val_host_abi_frame_init(&realm) == VAL_SUCCESS);
    g_test_world = FIRME_ACS_WORLD_SECURE;
    assert(val_host_abi_frame_invoke(&g_test_expected_input, &output, &issued,
                                      &secure) == VAL_SUCCESS);
    assert(secure.sequence == TEST_SEQUENCE);
    assert(val_host_abi_frame_invoke(&g_test_expected_input, &output, &issued,
                                      &secure) == VAL_SUCCESS);
    assert(secure.sequence == TEST_SEQUENCE + 1U);
    g_test_world = FIRME_ACS_WORLD_REALM;
    assert(val_host_abi_frame_invoke(&g_test_expected_input, &output, &issued,
                                      &realm) == VAL_SUCCESS);
    assert(realm.sequence == TEST_SEQUENCE);

    g_test_world = FIRME_ACS_WORLD_SECURE;
    g_test_fault_operation = VAL_ABI_FRAME_EXECUTE;
    g_test_fault_remaining = VAL_ABI_FRAME_RETRY_COUNT;
    g_test_fault = TEST_DROP_AFTER;
    assert(val_host_abi_frame_invoke(&g_test_expected_input, &output, &issued,
                                      &secure) == VAL_ERROR);
    assert(issued && secure.quarantined && secure.outcome == VAL_ABI_FRAME_UNCERTAIN);
    assert(val_host_abi_frame_init_world(&secure, FIRME_ACS_WORLD_SECURE) == VAL_ERROR);
    assert(g_test_smc_calls == 4U);
    g_test_world = FIRME_ACS_WORLD_REALM;
    assert(val_host_abi_frame_invoke(&g_test_expected_input, &output, &issued,
                                      &realm) == VAL_SUCCESS);
    assert(issued && realm.sequence == TEST_SEQUENCE + 1U && !realm.quarantined);
    assert(g_test_smc_calls == 5U);
    val_test_frame_scrubbed();
    assert(g_abi_frame_slots[FIRME_ACS_WORLD_SECURE - 1U][g_test_pe].phase ==
           VAL_ABI_FRAME_COMPLETE);
}

/** @brief Exercise frame fidelity, replay protection and bounded failure recovery. */
int main(void)
{
    const firme_acs_world_t worlds[] = {FIRME_ACS_WORLD_SECURE, FIRME_ACS_WORLD_REALM};
    size_t index;

    for (index = 0U; index < sizeof(worlds) / sizeof(worlds[0]); index++) {
        val_test_frame_round_trip(worlds[index]);
        val_test_frame_retries(worlds[index]);
        val_test_frame_uncertain(worlds[index], VAL_ABI_FRAME_EXECUTE, TEST_DROP_AFTER);
        val_test_frame_uncertain(worlds[index], VAL_ABI_FRAME_EXECUTE, TEST_BAD_ACK);
        val_test_frame_uncertain(worlds[index], VAL_ABI_FRAME_EXECUTE, TEST_ERROR_AFTER);
        val_test_frame_uncertain(worlds[index], VAL_ABI_FRAME_READ, TEST_DROP_AFTER);
        val_test_frame_uncertain(worlds[index], VAL_ABI_FRAME_RELEASE, TEST_DROP_AFTER);
        val_test_frame_allowlist(worlds[index]);
    }

    val_test_frame_rejections();
    val_test_frame_execute_once();
    val_test_frame_uncertain(FIRME_ACS_WORLD_REALM, VAL_ABI_FRAME_READ, TEST_BAD_HALF);
    val_test_frame_unissued();
    val_test_frame_owner_and_wrap();
    val_test_frame_argument_guards();
    val_test_frame_world_isolation();
    puts("ABI frame native checks: PASS (18 registers, replay, PE/world isolation, faults)");
    return VAL_SUCCESS;
}
