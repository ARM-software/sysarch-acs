/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi_frame.h"
#include "val_firme_abi.h"
#include "val_common/val_mp.h"

#include <stddef.h>

#define VAL_ABI_FRAME_ALL_WORDS ((UINT32_C(1) << VAL_SMCCC_REGISTER_COUNT) - 1U)

typedef struct {
    smccc_call_frame_t input;
    smccc_call_frame_t output;
    uint32_t sequence;
    uint32_t written;
    uint32_t read;
    uint32_t locked;
    val_firme_abi_frame_phase_t phase;
} val_firme_abi_frame_slot_t;

_Static_assert(PAL_PE_COUNT <= VAL_ABI_FRAME_OWNER_MAX + 1U,
               "frame command must represent every physical PE");
_Static_assert(VAL_SMCCC_REGISTER_COUNT == 18U, "frame protocol transfers x0-x17");

static val_firme_abi_frame_slot_t g_abi_frame_slots[VAL_ABI_FRAME_WORLD_COUNT][PAL_PE_COUNT];

/**
 * @brief Recognize exact discovery/GM/IDE function IDs without accepting other SMCs.
 * @param fid Full-width function identifier.
 * @return VAL_TRUE for an allowlisted identifier, otherwise VAL_FALSE.
 */
int val_firme_abi_frame_fid_allowed(uint64_t fid)
{
    switch (fid) {
    case FIRME_FID_SERVICE_VERSION:
    case FIRME_FID_SERVICE_FEATURES:
    case FIRME_FID_GM_GPI_SET:
    case FIRME_FID_GM_GPI_OP_CONTINUE:
    case FIRME_FID_GM_GPT_INFO:
    case FIRME_FID_GM_L1_GPT_CREATE:
    case FIRME_FID_GM_L1_GPT_DESTROY:
    case FIRME_FID_IDE_KEYSET_PROG:
    case FIRME_FID_IDE_KEYSET_GO:
    case FIRME_FID_IDE_KEYSET_STOP:
    case FIRME_FID_IDE_KEYSET_POLL:
        return VAL_TRUE;
    default:
        return VAL_FALSE;
    }
}

/**
 * @brief Erase key-bearing registers while retaining the anti-replay sequence.
 * @param slot Locked local slot whose contents can safely be released.
 */
static void val_firme_abi_frame_scrub(val_firme_abi_frame_slot_t *slot)
{
    volatile uint64_t *input = slot->input.registers;
    volatile uint64_t *output = slot->output.registers;
    uint32_t word;

    for (word = 0U; word < VAL_SMCCC_REGISTER_COUNT; word++) {
        input[word] = 0ULL;
        output[word] = 0ULL;
    }

    slot->written = 0U;
    slot->read = 0U;
    slot->phase = VAL_ABI_FRAME_RELEASED;
}

/**
 * @brief Apply a validated command while holding the owning PE's slot lock.
 * @param slot Locked slot private to the physical PE executing this handler.
 * @param operation Decoded command operation.
 * @param sequence Nonzero bounded request identity.
 * @param word Validated word index, zero for operations without a word.
 * @param value Uploaded register value, zero for operations other than WRITE.
 * @param response ACS status and RMMD-safe response payload.
 */
static void val_firme_abi_frame_process(val_firme_abi_frame_slot_t *slot,
                                       uint32_t operation, uint32_t sequence,
                                       uint32_t word, uint64_t value,
                                       firme_acs_call_res_t *response)
{
    uint32_t bit = UINT32_C(1) << word;

    if (operation == VAL_ABI_FRAME_BEGIN) {
        if (slot->phase == VAL_ABI_FRAME_RELEASED) {
            if (sequence <= slot->sequence)
                return;

            slot->sequence = sequence;
            slot->phase = VAL_ABI_FRAME_UPLOADING;
        } else if ((slot->sequence != sequence) ||
                   (slot->phase != VAL_ABI_FRAME_UPLOADING)) {
            return;
        }
    } else {
        if (slot->sequence != sequence)
            return;

        switch (operation) {
        case VAL_ABI_FRAME_WRITE:
            if ((slot->phase != VAL_ABI_FRAME_UPLOADING) ||
                (((slot->written & bit) != 0U) &&
                 (slot->input.registers[word] != value)))
                return;

            slot->input.registers[word] = value;
            slot->written |= bit;
            break;
        case VAL_ABI_FRAME_EXECUTE:
            if (slot->phase == VAL_ABI_FRAME_COMPLETE)
                break;

            if ((slot->phase != VAL_ABI_FRAME_UPLOADING) ||
                (slot->written != VAL_ABI_FRAME_ALL_WORDS) ||
                !val_firme_abi_frame_fid_allowed(slot->input.registers[0]))
                return;

            slot->phase = VAL_ABI_FRAME_EXECUTING;
            val_smccc_smc_call_frame(&slot->input, &slot->output);
            slot->phase = VAL_ABI_FRAME_COMPLETE;
            break;
        case VAL_ABI_FRAME_READ:
            if (slot->phase != VAL_ABI_FRAME_COMPLETE)
                return;

            value = slot->output.registers[word];
            slot->read |= bit;
            response->x0 = ACS_STATUS_PASS;
            response->x1 = (uint32_t)value;
            response->x2 = (uint32_t)(value >> 32);
            return;
        case VAL_ABI_FRAME_RELEASE:
            if ((slot->phase == VAL_ABI_FRAME_EXECUTING) ||
                ((slot->phase == VAL_ABI_FRAME_COMPLETE) &&
                 (slot->read != VAL_ABI_FRAME_ALL_WORDS)))
                return;

            val_firme_abi_frame_scrub(slot);
            break;
        default:
            return;
        }
    }

    response->x0 = ACS_STATUS_PASS;
    response->x1 = sequence;
    response->x2 = slot->phase;
}

/**
 * @brief Handle a bounded by-value transaction without relying on RMMD x6/x7.
 * @param world Actual target world supplied by its private command handler.
 * @param control Packed operation, target world, word index and physical owner PE.
 * @param sequence Nonzero 32-bit token, never reused after release.
 * @param low Lower 32-bit register half, valid only for WRITE.
 * @param high Upper 32-bit register half, valid only for WRITE.
 * @param response ACS status and exactly two safe payload words.
 */
void val_firme_abi_frame_command(firme_acs_world_t world, uint64_t control, uint64_t sequence,
                                uint64_t low, uint64_t high,
                                firme_acs_call_res_t *response)
{
    val_firme_abi_frame_slot_t *slot;
    uint32_t owner = val_pe_index();
    uint32_t operation = (uint32_t)control & VAL_ABI_FRAME_OPERATION_MASK;
    uint32_t word = ((uint32_t)control >> VAL_ABI_FRAME_WORD_SHIFT) &
                    VAL_ABI_FRAME_WORD_MASK;

    if (response == NULL)
        return;

    *response = (firme_acs_call_res_t){.x0 = ACS_STATUS_ERR};
    if (((world != FIRME_ACS_WORLD_SECURE) && (world != FIRME_ACS_WORLD_REALM)) ||
        (control > UINT32_MAX) || (sequence == 0ULL) ||
        (sequence > UINT32_MAX) || (owner >= PAL_PE_COUNT) ||
        (((control >> VAL_ABI_FRAME_WORLD_SHIFT) & VAL_ABI_FRAME_WORLD_MASK) != world) ||
        ((control >> VAL_ABI_FRAME_OWNER_SHIFT) != owner) ||
        (operation < VAL_ABI_FRAME_BEGIN) || (operation > VAL_ABI_FRAME_RELEASE) ||
        (word >= VAL_SMCCC_REGISTER_COUNT) || (low > UINT32_MAX) ||
        (high > UINT32_MAX))
        return;

    if (((operation != VAL_ABI_FRAME_WRITE) && ((low != 0ULL) || (high != 0ULL))) ||
        (((operation != VAL_ABI_FRAME_WRITE) && (operation != VAL_ABI_FRAME_READ)) &&
         (word != 0U)))
        return;

    slot = &g_abi_frame_slots[(uint32_t)world - 1U][owner];
    if (__atomic_exchange_n(&slot->locked, VAL_TRUE, __ATOMIC_ACQUIRE))
        return;

    val_firme_abi_frame_process(slot, operation, (uint32_t)sequence, word,
                                low | (high << 32), response);
    __atomic_store_n(&slot->locked, VAL_FALSE, __ATOMIC_RELEASE);
}
