/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_abi_frame.h"
#include "val_common/val_mp.h"

#include <stddef.h>

typedef struct {
    uint32_t locked;
    uint32_t sequence;
    uint32_t quarantined;
} val_host_abi_frame_owner_t;

static val_host_abi_frame_owner_t
    g_host_abi_frame_owners[VAL_ABI_FRAME_WORLD_COUNT][PAL_PE_COUNT];

/**
 * @brief Retry an idempotent protocol command within a fixed dispatch budget.
 * @param context PE and token identifying this invocation.
 * @param operation Frame protocol operation.
 * @param word Register index, zero for commands without a word.
 * @param value Input register for WRITE, otherwise zero.
 * @param phase Expected acknowledged phase for commands other than READ.
 * @param output Reconstructed READ value, otherwise NULL.
 * @return VAL_SUCCESS for a structurally valid acknowledgement, otherwise VAL_ERROR.
 */
static int val_host_abi_frame_command(const val_host_abi_frame_context_t *context,
                                      uint32_t operation, uint32_t word, uint64_t value,
                                      val_firme_abi_frame_phase_t phase, uint64_t *output)
{
    firme_acs_call_res_t response;
    uint64_t control = ((uint64_t)context->owner_pe << VAL_ABI_FRAME_OWNER_SHIFT) |
                       ((uint64_t)word << VAL_ABI_FRAME_WORD_SHIFT) |
                       ((uint64_t)context->world << VAL_ABI_FRAME_WORLD_SHIFT) | operation;
    uint32_t attempt;

    for (attempt = 0U; attempt < VAL_ABI_FRAME_RETRY_COUNT; attempt++) {
        response = (firme_acs_call_res_t){.x0 = ACS_STATUS_ERR};
        if ((val_firme_world_call(context->world, FIRME_ACS_CMD_ABI_FRAME,
                                  control, context->sequence, (uint32_t)value,
                                  (uint32_t)(value >> 32), &response) != VAL_SUCCESS) ||
            (response.x0 != ACS_STATUS_PASS))
            continue;

        if (operation == VAL_ABI_FRAME_READ) {
            if ((output == NULL) || (response.x1 > UINT32_MAX) ||
                (response.x2 > UINT32_MAX))
                continue;

            *output = response.x1 | (response.x2 << 32);
            return VAL_SUCCESS;
        }

        if ((response.x1 == context->sequence) && (response.x2 == phase))
            return VAL_SUCCESS;
    }

    return VAL_ERROR;
}

/**
 * @brief Initialize a cookie without clearing a retained world/PE slot.
 * @param context Persistent caller storage for the callback cookie.
 * @param world Explicit Secure or Realm target.
 * @return VAL_SUCCESS on an available world/PE slot, otherwise VAL_ERROR.
 */
int val_host_abi_frame_init_world(val_host_abi_frame_context_t *context,
                                 firme_acs_world_t world)
{
    val_host_abi_frame_owner_t *owner;
    uint32_t pe_index = val_pe_index();

    if ((context == NULL) || (pe_index >= PAL_PE_COUNT) ||
        ((world != FIRME_ACS_WORLD_SECURE) && (world != FIRME_ACS_WORLD_REALM)))
        return VAL_ERROR;

    owner = &g_host_abi_frame_owners[(uint32_t)world - 1U][pe_index];
    if (__atomic_exchange_n(&owner->locked, VAL_TRUE, __ATOMIC_ACQUIRE))
        return VAL_ERROR;

    if (owner->quarantined || (owner->sequence == UINT32_MAX)) {
        __atomic_store_n(&owner->locked, VAL_FALSE, __ATOMIC_RELEASE);
        return VAL_ERROR;
    }

    *context = (val_host_abi_frame_context_t){
        .world = world,
        .owner_pe = pe_index,
        .initialized = VAL_TRUE,
        .outcome = VAL_ABI_FRAME_UNISSUED,
    };
    __atomic_store_n(&owner->locked, VAL_FALSE, __ATOMIC_RELEASE);
    return VAL_SUCCESS;
}

/**
 * @brief Preserve the Realm-only initializer for existing IDE callers.
 * @param context Persistent caller storage for the callback cookie.
 * @return VAL_SUCCESS for an available Realm/PE slot, otherwise VAL_ERROR.
 */
int val_host_abi_frame_init(val_host_abi_frame_context_t *context)
{
    return val_host_abi_frame_init_world(context, FIRME_ACS_WORLD_REALM);
}

/**
 * @brief Erase the private temporary result without publishing partial output.
 * @param frame Local full-register frame which can contain IDE key material.
 */
static void val_host_abi_frame_scrub(smccc_call_frame_t *frame)
{
    volatile uint64_t *registers = frame->registers;
    uint32_t word;

    for (word = 0U; word < VAL_SMCCC_REGISTER_COUNT; word++)
        registers[word] = 0ULL;
}

/**
 * @brief Execute once in the explicit target world and recover acknowledgements.
 * @param input Immutable x0-x17 values for the requested SMC.
 * @param output Full output, written only after successful result retrieval/release.
 * @param issued Whether this invocation may have executed an SMC at its target.
 * @param cookie Initialized val_host_abi_frame_context_t on the current PE.
 * @return VAL_SUCCESS only with valid output and an acknowledged released slot.
 */
int val_host_abi_frame_invoke(const smccc_call_frame_t *input,
                             smccc_call_frame_t *output, uint32_t *issued,
                             void *cookie)
{
    val_host_abi_frame_context_t *context = cookie;
    val_host_abi_frame_owner_t *owner;
    smccc_call_frame_t result = {0};
    uint32_t pe_index = val_pe_index();
    uint32_t word;
    int status = VAL_ERROR;

    if (issued == NULL)
        return VAL_ERROR;

    *issued = VAL_FALSE;
    if ((context != NULL) && context->initialized &&
        (context->outcome == VAL_ABI_FRAME_UNCERTAIN))
        *issued = VAL_TRUE;

    if ((context == NULL) || !context->initialized || context->quarantined ||
        (pe_index >= PAL_PE_COUNT) || (context->owner_pe != pe_index) ||
        ((context->world != FIRME_ACS_WORLD_SECURE) &&
         (context->world != FIRME_ACS_WORLD_REALM)))
        return VAL_ERROR;

    owner = &g_host_abi_frame_owners[(uint32_t)context->world - 1U][pe_index];
    if (__atomic_exchange_n(&owner->locked, VAL_TRUE, __ATOMIC_ACQUIRE))
        return VAL_ERROR;

    if (owner->quarantined)
        goto unlock;

    *issued = VAL_FALSE;
    context->outcome = VAL_ABI_FRAME_UNISSUED;
    if ((input == NULL) || (output == NULL) || (owner->sequence == UINT32_MAX) ||
        !val_firme_abi_frame_fid_allowed(input->registers[0]))
        goto unlock;

    context->sequence = ++owner->sequence;
    if (val_host_abi_frame_command(context, VAL_ABI_FRAME_BEGIN, 0U, 0ULL,
                                   VAL_ABI_FRAME_UPLOADING, NULL) != VAL_SUCCESS)
        goto abort;

    for (word = 0U; word < VAL_SMCCC_REGISTER_COUNT; word++) {
        if (val_host_abi_frame_command(context, VAL_ABI_FRAME_WRITE, word,
                                       input->registers[word], VAL_ABI_FRAME_UPLOADING,
                                       NULL) != VAL_SUCCESS)
            goto abort;
    }

    *issued = VAL_TRUE;
    context->outcome = VAL_ABI_FRAME_UNCERTAIN;
    if (val_host_abi_frame_command(context, VAL_ABI_FRAME_EXECUTE, 0U, 0ULL,
                                   VAL_ABI_FRAME_COMPLETE, NULL) != VAL_SUCCESS)
        goto quarantine;

    for (word = 0U; word < VAL_SMCCC_REGISTER_COUNT; word++) {
        if (val_host_abi_frame_command(context, VAL_ABI_FRAME_READ, word, 0ULL,
                                       VAL_ABI_FRAME_COMPLETE,
                                       &result.registers[word]) != VAL_SUCCESS)
            goto quarantine;
    }

    if (val_host_abi_frame_command(context, VAL_ABI_FRAME_RELEASE, 0U, 0ULL,
                                   VAL_ABI_FRAME_RELEASED, NULL) != VAL_SUCCESS)
        goto quarantine;

    *output = result;
    context->outcome = VAL_ABI_FRAME_OUTPUT_VALID;
    status = VAL_SUCCESS;
    goto unlock;

abort:
    if (val_host_abi_frame_command(context, VAL_ABI_FRAME_RELEASE, 0U, 0ULL,
                                   VAL_ABI_FRAME_RELEASED, NULL) == VAL_SUCCESS)
        goto unlock;

quarantine:
    context->quarantined = VAL_TRUE;
    owner->quarantined = VAL_TRUE;

unlock:
    val_host_abi_frame_scrub(&result);
    __atomic_store_n(&owner->locked, VAL_FALSE, __ATOMIC_RELEASE);
    return status;
}
