/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi_adapter.h"

#include <stddef.h>

#include "aarch64/sysreg.h"
#include "val_common/val_mp.h"
#include "pal_firme_ide_abi.h"
#include "val_common/val_status.h"
#include "val_firme_abi.h"
#include "val_firme_ide.h"
#include "val_log.h"

#define IDE_ABI_ECAM        0U
#define IDE_ABI_BAD_ECAM    1U
#define IDE_ABI_KEYSET      2U
#define IDE_ABI_KEY_FIRST   3U
#define IDE_ABI_KEY_COUNT   4U
#define IDE_ABI_HANDLE      7U
#define IDE_ABI_TOKEN       8U
#define IDE_ABI_TYPE        9U
#define IDE_ABI_PREPARED   10U
#define IDE_ABI_PROGRAMMED 11U
#define IDE_ABI_ACTIVE     12U
#define IDE_ABI_PENDING    13U
#define IDE_ABI_PENDING_OP 14U
#define IDE_ABI_MAIN_TRACKED 15U
#define IDE_ABI_UNCERTAIN UINT64_MAX

static val_firme_abi_test_context_t g_ide_abi_retained;
static uint32_t g_ide_abi_retained_valid;
static uint64_t g_ide_abi_handle;
static uint32_t g_ide_abi_owner_pe;
static uint32_t g_ide_abi_owned;

static int val_firme_ide_abi_cleanup(val_firme_abi_test_context_t *context);

/** @brief Allocate a non-reused handle before issuing a new operation. */
static int val_firme_ide_abi_new_handle(val_firme_abi_test_context_t *context)
{
    if (!g_ide_abi_owned || g_ide_abi_owner_pe != val_pe_index() ||
        g_ide_abi_handle == UINT64_MAX || context->scratch[IDE_ABI_PENDING])
        return VAL_ERROR;
    context->scratch[IDE_ABI_HANDLE] = ++g_ide_abi_handle;
    return VAL_SUCCESS;
}

/** @brief Recover the exact PAL reservation from the case's retained scalar state. */
static void val_firme_ide_abi_scenario(const val_firme_abi_test_context_t *context,
                                       pal_firme_ide_abi_scenario_t *scenario)
{
    uint32_t key_index;

    scenario->ecam_address = context->scratch[IDE_ABI_ECAM];
    scenario->invalid_ecam_address = context->scratch[IDE_ABI_BAD_ECAM];
    scenario->keyset_id = context->scratch[IDE_ABI_KEYSET];
    scenario->handle = context->scratch[IDE_ABI_HANDLE];
    scenario->token = context->scratch[IDE_ABI_TOKEN];
    scenario->ide_type = (uint32_t)context->scratch[IDE_ABI_TYPE];
    for (key_index = 0U; key_index < IDE_ABI_KEY_COUNT; key_index++)
        scenario->keyqw[key_index] = context->scratch[IDE_ABI_KEY_FIRST + key_index];
}

/**
 * @brief Retain all case-owned state beyond the generated runner's stack frame.
 * @param context Failed cleanup or unresolved operation owner.
 * @return VAL_ERROR, without releasing its PAL reservation.
 */
static int val_firme_ide_abi_retain(val_firme_abi_test_context_t *context)
{
    g_ide_abi_retained = *context;
    g_ide_abi_retained_valid = VAL_TRUE;
    return VAL_ERROR;
}

/**
 * @brief Apply a known completed operation, including late cleanup completion.
 * @param context Owner whose pending operation type must not be lost.
 */
static void val_firme_ide_abi_acknowledge(val_firme_abi_test_context_t *context)
{
    uint64_t operation = context->scratch[IDE_ABI_PENDING_OP];

    if (operation == PAL_FIRME_IDE_ABI_PROG + 1U)
        context->scratch[IDE_ABI_PROGRAMMED] = VAL_TRUE;
    else if (operation == PAL_FIRME_IDE_ABI_GO + 1U)
        context->scratch[IDE_ABI_ACTIVE] = VAL_TRUE;
    else if (operation == PAL_FIRME_IDE_ABI_STOP + 1U) {
        context->scratch[IDE_ABI_ACTIVE] = VAL_FALSE;
        context->scratch[IDE_ABI_PROGRAMMED] = VAL_FALSE;
    }
    context->scratch[IDE_ABI_PENDING] = VAL_FALSE;
    context->scratch[IDE_ABI_PENDING_OP] = 0ULL;
}

/**
 * @brief Remember accepted operations even when generic expectation checks fail.
 * @param context Owner of the issued request.
 * @param operation Issued PROG, GO, STOP or POLL.
 * @param status Decoded architectural status.
 */
static void val_firme_ide_abi_track(val_firme_abi_test_context_t *context,
                                    pal_firme_ide_abi_operation_t operation,
                                    int64_t status)
{
    if (operation == PAL_FIRME_IDE_ABI_POLL)
        return;
    if (status == FIRME_SUCCESS || status == FIRME_INCOMPLETE) {
        context->scratch[IDE_ABI_PENDING_OP] = operation + 1U;
        context->scratch[IDE_ABI_PENDING] = VAL_TRUE;
        if (status == FIRME_SUCCESS)
            val_firme_ide_abi_acknowledge(context);
    } else if (status != FIRME_BUSY && status != FIRME_INVALID_PARAMETERS &&
               status != FIRME_NOT_SUPPORTED && status != FIRME_DENIED) {
        context->scratch[IDE_ABI_PENDING_OP] = IDE_ABI_UNCERTAIN;
        context->scratch[IDE_ABI_PENDING] = VAL_TRUE;
    }
}

/** @brief Quarantine unexpected acceptance of a request aimed outside its valid fixture. */
static void val_firme_ide_abi_track_main(val_firme_abi_test_context_t *context,
                                         pal_firme_ide_abi_operation_t operation,
                                         int64_t status)
{
    context->scratch[IDE_ABI_MAIN_TRACKED] = VAL_TRUE;
    if ((context->test_case->mutation == VAL_FIRME_MUTATION_IDE_INVALID_ECAM ||
         context->test_case->mutation == VAL_FIRME_MUTATION_IDE_INVALID_TYPE) &&
        (status == FIRME_SUCCESS || status == FIRME_INCOMPLETE)) {
        context->scratch[IDE_ABI_PENDING] = VAL_TRUE;
        context->scratch[IDE_ABI_PENDING_OP] = IDE_ABI_UNCERTAIN;
        return;
    }
    val_firme_ide_abi_track(context, operation, status);
}

/**
 * @brief Map a generated IDE ABI to its PAL scenario operation.
 * @param abi_id Generated ABI identifier.
 * @param operation Returned PAL operation.
 * @return VAL_SUCCESS for an IDE ABI, otherwise VAL_ERROR.
 */
static int val_firme_ide_abi_operation(
    val_firme_abi_id_t abi_id, pal_firme_ide_abi_operation_t *operation)
{
    if (!operation)
        return VAL_ERROR;

    if (abi_id == VAL_FIRME_ABI_ID_IDE_KEYSET_PROG)
        *operation = PAL_FIRME_IDE_ABI_PROG;
    else if (abi_id == VAL_FIRME_ABI_ID_IDE_KEYSET_GO)
        *operation = PAL_FIRME_IDE_ABI_GO;
    else if (abi_id == VAL_FIRME_ABI_ID_IDE_KEYSET_STOP)
        *operation = PAL_FIRME_IDE_ABI_STOP;
    else if (abi_id == VAL_FIRME_ABI_ID_IDE_KEYSET_POLL)
        *operation = PAL_FIRME_IDE_ABI_POLL;
    else
        return VAL_ERROR;

    return VAL_SUCCESS;
}

/**
 * @brief Map a generated mutation to a PAL scenario condition.
 * @param mutation Generated case mutation.
 * @param condition Returned PAL condition.
 * @return VAL_SUCCESS for an IDE case, otherwise VAL_ERROR.
 */
static int val_firme_ide_abi_condition(
    val_firme_abi_mutation_t mutation,
    pal_firme_ide_abi_condition_t *condition)
{
    if (!condition)
        return VAL_ERROR;

    if (mutation == VAL_FIRME_MUTATION_NONE)
        *condition = PAL_FIRME_IDE_ABI_VALID;
    else if (mutation == VAL_FIRME_MUTATION_IDE_INVALID_ECAM)
        *condition = PAL_FIRME_IDE_ABI_INVALID_ECAM;
    else if (mutation == VAL_FIRME_MUTATION_IDE_INVALID_TYPE)
        *condition = PAL_FIRME_IDE_ABI_INVALID_TYPE;
    else if (mutation == VAL_FIRME_MUTATION_IDE_NO_PENDING)
        *condition = PAL_FIRME_IDE_ABI_NO_PENDING;
    else if (mutation == VAL_FIRME_MUTATION_IDE_POLL_ANY)
        *condition = PAL_FIRME_IDE_ABI_POLL_ANY;
    else
        return VAL_ERROR;

    return VAL_SUCCESS;
}

/**
 * @brief Store a PAL-provided IDE path in the per-case scratch frame.
 * @param context Generated case context.
 * @param scenario Prepared PAL path and cleanup token.
 * @return VAL_SUCCESS when the path is valid, otherwise VAL_ERROR.
 */
static int val_firme_ide_abi_store_scenario(
    val_firme_abi_test_context_t *context,
    const pal_firme_ide_abi_scenario_t *scenario)
{
    uint32_t key_index;

    context->scratch[IDE_ABI_ECAM] = scenario->ecam_address;
    context->scratch[IDE_ABI_BAD_ECAM] = scenario->invalid_ecam_address;
    context->scratch[IDE_ABI_KEYSET] = scenario->keyset_id;
    context->scratch[IDE_ABI_HANDLE] = scenario->handle;
    context->scratch[IDE_ABI_TOKEN] = scenario->token;
    context->scratch[IDE_ABI_TYPE] = scenario->ide_type;
    context->scratch[IDE_ABI_PREPARED] = VAL_TRUE;

    for (key_index = 0U; key_index < IDE_ABI_KEY_COUNT; key_index++)
        context->scratch[IDE_ABI_KEY_FIRST + key_index] =
            scenario->keyqw[key_index];

    if ((scenario->ecam_address == scenario->invalid_ecam_address) ||
        (scenario->handle == 0ULL) ||
        ((scenario->keyset_id & ~VAL_FIRME_IDE_KEYSET_ID_INPUT_MASK) != 0ULL) ||
        ((scenario->ide_type != VAL_FIRME_IDE_TYPE_SELECTIVE) &&
         (scenario->ide_type != VAL_FIRME_IDE_TYPE_PCIE_LINK) &&
         (scenario->ide_type != VAL_FIRME_IDE_TYPE_CXL_CACHEMEM))) {
        LOG_ERROR(NULL, "%s: PAL returned an invalid IDE ABI path",
                  context->plan->name);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/**
 * @brief Poll a pending IDE operation and verify its preserved handle.
 * @param context Generated case context.
 * @param any_pending Whether to request any pending response.
 * @return VAL_SUCCESS only for a matching terminal SUCCESS.
 */
static int val_firme_ide_abi_poll_complete(
    val_firme_abi_test_context_t *context, uint32_t any_pending)
{
    firme_res_t response = {0};
    uint64_t keyset_id;

    if (!g_ide_abi_owned || g_ide_abi_owner_pe != val_pe_index() ||
        context->scratch[IDE_ABI_PENDING_OP] == IDE_ABI_UNCERTAIN ||
        context->scratch[IDE_ABI_PENDING_OP] == 0ULL)
        return VAL_ERROR;

    keyset_id = (any_pending == VAL_TRUE) ? 0ULL :
                context->scratch[IDE_ABI_KEYSET];
    val_firme_ide_keyset_poll_do_retry(
        context->scratch[IDE_ABI_ECAM],
        val_firme_ide_poll_flags(any_pending), keyset_id, &response);

    if ((val_firme_status_to_i64(response.x0) != (int64_t)FIRME_SUCCESS) ||
        (response.x1 != context->scratch[IDE_ABI_HANDLE]) ||
        ((any_pending == VAL_TRUE) &&
         ((response.x2 & VAL_FIRME_IDE_KEYSET_ID_INPUT_MASK) !=
          context->scratch[IDE_ABI_KEYSET]))) {
        LOG_ERROR(NULL,
                  "%s: POLL completion x0=%llx x1=%llx x2=%llx handle=%llx",
                  context->plan->name,
                  (unsigned long long)response.x0,
                  (unsigned long long)response.x1,
                  (unsigned long long)response.x2,
                  (unsigned long long)context->scratch[IDE_ABI_HANDLE]);
        if (val_firme_status_to_i64(response.x0) != FIRME_BUSY &&
            val_firme_status_to_i64(response.x0) != FIRME_INCOMPLETE)
            context->scratch[IDE_ABI_PENDING_OP] = IDE_ABI_UNCERTAIN;
        return VAL_ERROR;
    }

    val_firme_ide_abi_acknowledge(context);
    return VAL_SUCCESS;
}

/**
 * @brief Program the test keyset before GO, STOP or POLL cases.
 * @param context Generated case context.
 * @param require_pending Keep an asynchronous PROG pending for POLL.
 * @return VAL_SUCCESS, SCENARIO_UNAVAILABLE for synchronous POLL setup, or
 *         VAL_ERROR for an invalid setup response.
 */
static int val_firme_ide_abi_program_setup(
    val_firme_abi_test_context_t *context, uint32_t require_pending)
{
    val_firme_ide_prog_args_t arguments = {0};
    firme_res_t response = {0};
    int64_t status;

    if (val_firme_ide_abi_new_handle(context) != VAL_SUCCESS)
        return VAL_ERROR;
    val_firme_ide_prog_args_init(
        &arguments, context->scratch[IDE_ABI_ECAM],
        (uint32_t)context->scratch[IDE_ABI_TYPE],
        context->scratch[IDE_ABI_KEYSET],
        context->scratch[IDE_ABI_HANDLE]);
    arguments.keyqw0 = context->scratch[IDE_ABI_KEY_FIRST];
    arguments.keyqw1 = context->scratch[IDE_ABI_KEY_FIRST + 1U];
    arguments.keyqw2 = context->scratch[IDE_ABI_KEY_FIRST + 2U];
    arguments.keyqw3 = context->scratch[IDE_ABI_KEY_FIRST + 3U];
    val_firme_ide_keyset_prog_do_retry(&arguments, &response);
    status = val_firme_status_to_i64(response.x0);
    val_firme_ide_abi_track(context, PAL_FIRME_IDE_ABI_PROG, status);

    if (status == (int64_t)FIRME_INCOMPLETE) {
        context->scratch[IDE_ABI_PENDING] = VAL_TRUE;
        if (require_pending == VAL_TRUE)
            return VAL_SUCCESS;

        if (val_firme_ide_abi_poll_complete(context, VAL_FALSE) != VAL_SUCCESS)
            return VAL_ERROR;
    } else if (status == (int64_t)FIRME_SUCCESS) {
        context->scratch[IDE_ABI_PROGRAMMED] = VAL_TRUE;
        if (require_pending == VAL_TRUE)
            return VAL_FIRME_ABI_SCENARIO_UNAVAILABLE;
    } else {
        LOG_ERROR(NULL, "%s: PROG setup returned x0=%llx",
                  context->plan->name, (unsigned long long)response.x0);
        return VAL_ERROR;
    }

    context->scratch[IDE_ABI_PROGRAMMED] = VAL_TRUE;
    return VAL_SUCCESS;
}

/**
 * @brief Start the programmed keyset before a STOP case.
 * @param context Generated case context.
 * @return VAL_SUCCESS only after GO reaches terminal SUCCESS.
 */
static int val_firme_ide_abi_go_setup(
    val_firme_abi_test_context_t *context)
{
    firme_res_t response = {0};
    int64_t status;

    if (val_firme_ide_abi_new_handle(context) != VAL_SUCCESS)
        return VAL_ERROR;
    val_firme_ide_keyset_go_do_retry(
        context->scratch[IDE_ABI_ECAM],
        val_firme_ide_flags((uint32_t)context->scratch[IDE_ABI_TYPE]),
        context->scratch[IDE_ABI_KEYSET],
        context->scratch[IDE_ABI_HANDLE], &response);
    status = val_firme_status_to_i64(response.x0);

    val_firme_ide_abi_track(context, PAL_FIRME_IDE_ABI_GO, status);
    if (status == (int64_t)FIRME_INCOMPLETE) {
        context->scratch[IDE_ABI_PENDING] = VAL_TRUE;
        if (val_firme_ide_abi_poll_complete(context, VAL_FALSE) != VAL_SUCCESS)
            return VAL_ERROR;
    } else if (status != (int64_t)FIRME_SUCCESS) {
        LOG_ERROR(NULL, "%s: GO setup returned x0=%llx",
                  context->plan->name, (unsigned long long)response.x0);
        return VAL_ERROR;
    }

    context->scratch[IDE_ABI_ACTIVE] = VAL_TRUE;
    return VAL_SUCCESS;
}

/**
 * @brief Reserve a serial IDE scenario on the calling PE and establish its valid pre-state.
 * @param context Generated case context.
 * @return VAL_SUCCESS, SCENARIO_UNAVAILABLE, or VAL_ERROR.
 */
static int val_firme_ide_abi_prepare(
    val_firme_abi_test_context_t *context)
{
    pal_firme_ide_abi_scenario_t scenario = {0};
    pal_firme_ide_abi_operation_t operation;
    pal_firme_ide_abi_condition_t condition;
    pal_firme_ide_abi_status_t platform_status;
    int setup_status;

    if (!context || !context->abi || !context->test_case)
        return VAL_ERROR;

    if (g_ide_abi_retained_valid &&
        val_firme_ide_abi_cleanup(&g_ide_abi_retained) != VAL_SUCCESS)
        return VAL_ERROR;
    if (g_ide_abi_owned)
        return VAL_ERROR;

    if ((val_firme_ide_abi_operation(context->abi->id, &operation) !=
         VAL_SUCCESS) ||
        (val_firme_ide_abi_condition(context->test_case->mutation,
                                     &condition) != VAL_SUCCESS))
        return VAL_ERROR;

    platform_status = pal_firme_ide_abi_prepare(
        (uint32_t)context->world, operation, condition, &scenario);
    if (platform_status == PAL_FIRME_IDE_ABI_UNAVAILABLE) {
        LOG_WARN(NULL, "%s: IDE Root Port/endpoint scenario unavailable",
                 context->plan->name);
        return VAL_FIRME_ABI_SCENARIO_UNAVAILABLE;
    }
    if (platform_status != PAL_FIRME_IDE_ABI_READY)
        return VAL_ERROR;

    g_ide_abi_owner_pe = val_pe_index();
    g_ide_abi_owned = VAL_TRUE;
    if (val_firme_ide_abi_store_scenario(context, &scenario) != VAL_SUCCESS)
        return VAL_ERROR;

    if (operation == PAL_FIRME_IDE_ABI_GO ||
        operation == PAL_FIRME_IDE_ABI_STOP) {
        setup_status = val_firme_ide_abi_program_setup(context, VAL_FALSE);
        if (setup_status != VAL_SUCCESS)
            return setup_status;
    }

    if (operation == PAL_FIRME_IDE_ABI_STOP &&
        val_firme_ide_abi_go_setup(context) != VAL_SUCCESS)
        return VAL_ERROR;

    if ((operation == PAL_FIRME_IDE_ABI_POLL) &&
        (condition != PAL_FIRME_IDE_ABI_INVALID_ECAM) &&
        (condition != PAL_FIRME_IDE_ABI_NO_PENDING)) {
        setup_status = val_firme_ide_abi_program_setup(context, VAL_TRUE);
        if (setup_status != VAL_SUCCESS)
            return setup_status;
    }

    platform_status = pal_firme_ide_abi_ready((uint32_t)context->world,
                                              operation, condition, &scenario);
    if (platform_status == PAL_FIRME_IDE_ABI_UNAVAILABLE)
        return VAL_FIRME_ABI_SCENARIO_UNAVAILABLE;
    return platform_status == PAL_FIRME_IDE_ABI_READY ? VAL_SUCCESS : VAL_ERROR;
}

/** @brief Observe this generated invocation, never an unrelated replay of the ABI. */
static uint32_t val_firme_ide_abi_effect(val_firme_abi_test_context_t *context)
{
    pal_firme_ide_abi_scenario_t scenario = {0};
    pal_firme_ide_abi_operation_t operation;
    pal_firme_ide_abi_condition_t condition;

    if (!g_ide_abi_owned || g_ide_abi_owner_pe != val_pe_index() ||
        val_firme_ide_abi_operation(context->abi->id, &operation) != VAL_SUCCESS ||
        val_firme_ide_abi_condition(context->test_case->mutation, &condition) != VAL_SUCCESS)
        return VAL_FALSE;
    val_firme_ide_abi_scenario(context, &scenario);
    return pal_firme_ide_abi_observe((uint32_t)context->world, operation,
                                     condition, &scenario) == PAL_FIRME_IDE_ABI_READY ?
           VAL_TRUE : VAL_FALSE;
}

/**
 * @brief Build the ALP4 input frame for the selected IDE ABI.
 * @param context Generated case context.
 * @return VAL_SUCCESS for a complete input frame, otherwise VAL_ERROR.
 */
static int val_firme_ide_abi_build(
    val_firme_abi_test_context_t *context)
{
    pal_firme_ide_abi_operation_t operation;
    uint32_t key_index;

    if (!context || !context->abi || !g_ide_abi_owned ||
        g_ide_abi_owner_pe != val_pe_index() ||
        (val_firme_ide_abi_operation(context->abi->id, &operation) !=
         VAL_SUCCESS) ||
        (context->scratch[IDE_ABI_PREPARED] != VAL_TRUE))
        return VAL_ERROR;

    context->input.registers[1] = context->scratch[IDE_ABI_ECAM];
    context->input.registers[3] = context->scratch[IDE_ABI_KEYSET];

    if (operation == PAL_FIRME_IDE_ABI_POLL) {
        context->input.registers[2] = val_firme_ide_poll_flags(VAL_FALSE);
        return VAL_SUCCESS;
    }

    if (val_firme_ide_abi_new_handle(context) != VAL_SUCCESS)
        return VAL_ERROR;
    context->input.registers[2] = val_firme_ide_flags(
        (uint32_t)context->scratch[IDE_ABI_TYPE]);
    if (operation == PAL_FIRME_IDE_ABI_PROG) {
        for (key_index = 0U; key_index < IDE_ABI_KEY_COUNT; key_index++)
            context->input.registers[4U + key_index] =
                context->scratch[IDE_ABI_KEY_FIRST + key_index];
        context->input.registers[8] = context->scratch[IDE_ABI_HANDLE];
    } else {
        context->input.registers[4] = context->scratch[IDE_ABI_HANDLE];
    }

    return VAL_SUCCESS;
}

/**
 * @brief Apply an IDE input mutation without touching reserved registers.
 * @param context Generated case context.
 * @return VAL_SUCCESS for a supported case, otherwise VAL_ERROR.
 */
static int val_firme_ide_abi_mutate(
    val_firme_abi_test_context_t *context)
{
    if (!context || !context->test_case)
        return VAL_ERROR;

    if (context->test_case->mutation ==
        VAL_FIRME_MUTATION_IDE_INVALID_ECAM) {
        context->input.registers[1] = context->scratch[IDE_ABI_BAD_ECAM];
        return VAL_SUCCESS;
    }

    if (context->test_case->mutation ==
        VAL_FIRME_MUTATION_IDE_INVALID_TYPE) {
        context->input.registers[2] = VAL_FIRME_IDE_TYPE_UNUSED_TEST;
        return VAL_SUCCESS;
    }

    if (context->test_case->mutation ==
        VAL_FIRME_MUTATION_IDE_NO_PENDING)
        return VAL_SUCCESS;

    if (context->test_case->mutation ==
        VAL_FIRME_MUTATION_IDE_POLL_ANY) {
        context->input.registers[2] = val_firme_ide_poll_flags(VAL_TRUE);
        context->input.registers[3] = 0ULL;
        return VAL_SUCCESS;
    }

    return VAL_ERROR;
}

/**
 * @brief Retry a valid IDE request while the Root Port reports BUSY.
 * @param context Generated case context.
 * @return VAL_SUCCESS once the request is accepted, otherwise VAL_ERROR.
 */
static int val_firme_ide_abi_retry_main(
    val_firme_abi_test_context_t *context)
{
    uint32_t attempt;
    int64_t status;

    for (attempt = 0U; attempt < VAL_FIRME_IDE_RETRY_LIMIT; attempt++) {
        status = val_firme_status_to_i64(context->output.registers[0]);
        if (status != (int64_t)FIRME_BUSY)
            return VAL_SUCCESS;

        val_mp_relax();
        val_smccc_smc_call_frame(&context->input, &context->output);
    }

    status = val_firme_status_to_i64(context->output.registers[0]);
    if (status != (int64_t)FIRME_BUSY)
        return VAL_SUCCESS;

    LOG_ERROR(NULL, "%s: IDE request stayed BUSY", context->plan->name);
    return VAL_ERROR;
}

/**
 * @brief Validate exact IDE result status and conditional POLL output fields.
 * @param context Generated case context.
 * @return VAL_TRUE for the prepared architectural result, else VAL_FALSE.
 */
static uint32_t val_firme_ide_abi_validate(
    val_firme_abi_test_context_t *context)
{
    pal_firme_ide_abi_operation_t operation;
    int64_t status;
    uint32_t any_pending = VAL_FALSE;

    if (!context || !context->abi || !context->test_case || !g_ide_abi_owned ||
        g_ide_abi_owner_pe != val_pe_index() ||
        (val_firme_ide_abi_operation(context->abi->id, &operation) !=
         VAL_SUCCESS))
        return VAL_FALSE;

    status = val_firme_status_to_i64(context->output.registers[0]);
    val_firme_ide_abi_track_main(context, operation, status);

    if ((context->test_case->mutation ==
         VAL_FIRME_MUTATION_IDE_INVALID_ECAM) ||
        (context->test_case->mutation ==
         VAL_FIRME_MUTATION_IDE_INVALID_TYPE) ||
        (context->test_case->mutation ==
         VAL_FIRME_MUTATION_IDE_NO_PENDING))
        return val_firme_ide_abi_effect(context);

    if (val_firme_ide_abi_retry_main(context) != VAL_SUCCESS)
        return VAL_FALSE;

    status = val_firme_status_to_i64(context->output.registers[0]);
    val_firme_ide_abi_track(context, operation, status);
    if (status == (int64_t)FIRME_INCOMPLETE) {
        context->scratch[IDE_ABI_PENDING] = VAL_TRUE;
        if (context->test_case->mutation ==
            VAL_FIRME_MUTATION_IDE_POLL_ANY)
            any_pending = VAL_TRUE;
        if (val_firme_ide_abi_poll_complete(context, any_pending) !=
            VAL_SUCCESS)
            return VAL_FALSE;
    } else if (status == (int64_t)FIRME_SUCCESS) {
        if ((operation == PAL_FIRME_IDE_ABI_POLL) &&
            ((!context->scratch[IDE_ABI_PENDING] ||
              context->scratch[IDE_ABI_PENDING_OP] == 0ULL ||
              context->scratch[IDE_ABI_PENDING_OP] == IDE_ABI_UNCERTAIN) ||
             (context->output.registers[1] !=
              context->scratch[IDE_ABI_HANDLE]) ||
             ((context->test_case->mutation ==
               VAL_FIRME_MUTATION_IDE_POLL_ANY) &&
              ((context->output.registers[2] & VAL_FIRME_IDE_KEYSET_ID_INPUT_MASK) !=
               context->scratch[IDE_ABI_KEYSET])))) {
            LOG_ERROR(NULL,
                      "%s: POLL handle=%llx expected=%llx keyset=%llx expected=%llx",
                      context->plan->name,
                      (unsigned long long)context->output.registers[1],
                      (unsigned long long)context->scratch[IDE_ABI_HANDLE],
                      (unsigned long long)context->output.registers[2],
              (unsigned long long)context->scratch[IDE_ABI_KEYSET]);
            context->scratch[IDE_ABI_PENDING] = VAL_TRUE;
            context->scratch[IDE_ABI_PENDING_OP] = IDE_ABI_UNCERTAIN;
            return VAL_FALSE;
        }
        if (operation == PAL_FIRME_IDE_ABI_POLL)
            val_firme_ide_abi_acknowledge(context);
    } else {
        if (operation == PAL_FIRME_IDE_ABI_POLL && context->scratch[IDE_ABI_PENDING])
            context->scratch[IDE_ABI_PENDING_OP] = IDE_ABI_UNCERTAIN;
        LOG_ERROR(NULL, "%s: valid IDE request returned x0=%llx",
                  context->plan->name,
                  (unsigned long long)context->output.registers[0]);
        return VAL_FALSE;
    }

    if (operation == PAL_FIRME_IDE_ABI_PROG)
        context->scratch[IDE_ABI_PROGRAMMED] = VAL_TRUE;
    else if (operation == PAL_FIRME_IDE_ABI_GO)
        context->scratch[IDE_ABI_ACTIVE] = VAL_TRUE;
    else if (operation == PAL_FIRME_IDE_ABI_STOP) {
        context->scratch[IDE_ABI_ACTIVE] = VAL_FALSE;
        context->scratch[IDE_ABI_PROGRAMMED] = VAL_FALSE;
    }

    return val_firme_ide_abi_effect(context);
}

/**
 * @brief Stop a test stream that remains active after a generated case.
 * @param context Generated case context.
 * @return VAL_SUCCESS only when STOP completes successfully.
 */
static int val_firme_ide_abi_stop_cleanup(
    val_firme_abi_test_context_t *context)
{
    firme_res_t response = {0};
    int64_t status;

    if (val_firme_ide_abi_new_handle(context) != VAL_SUCCESS)
        return VAL_ERROR;
    val_firme_ide_keyset_stop_do_retry(
        context->scratch[IDE_ABI_ECAM],
        val_firme_ide_flags((uint32_t)context->scratch[IDE_ABI_TYPE]),
        context->scratch[IDE_ABI_KEYSET],
        context->scratch[IDE_ABI_HANDLE], &response);
    status = val_firme_status_to_i64(response.x0);

    val_firme_ide_abi_track(context, PAL_FIRME_IDE_ABI_STOP, status);
    if (status == (int64_t)FIRME_INCOMPLETE) {
        context->scratch[IDE_ABI_PENDING] = VAL_TRUE;
        if (val_firme_ide_abi_poll_complete(context, VAL_FALSE) != VAL_SUCCESS)
            return VAL_ERROR;
    } else if (status != (int64_t)FIRME_SUCCESS) {
        LOG_ERROR(NULL, "%s: cleanup STOP returned x0=%llx",
                  context->plan->name, (unsigned long long)response.x0);
        return VAL_ERROR;
    }

    context->scratch[IDE_ABI_ACTIVE] = VAL_FALSE;
    context->scratch[IDE_ABI_PROGRAMMED] = VAL_FALSE;
    return VAL_SUCCESS;
}

/**
 * @brief Drain operations and restore the PAL-reserved IDE test path.
 * @param context Generated case context.
 * @return VAL_SUCCESS only when all Root Port and endpoint state is restored.
 */
static int val_firme_ide_abi_cleanup(
    val_firme_abi_test_context_t *context)
{
    pal_firme_ide_abi_scenario_t scenario = {0};
    pal_firme_ide_abi_operation_t operation;
    pal_firme_ide_abi_condition_t condition;
    pal_firme_ide_abi_status_t platform_status;
    int64_t status;

    if (!context || !context->abi || !context->test_case)
        return VAL_ERROR;

    if (context->scratch[IDE_ABI_PREPARED] != VAL_TRUE)
        return VAL_SUCCESS;
    if (!g_ide_abi_owned || g_ide_abi_owner_pe != val_pe_index())
        return VAL_ERROR;

    if ((val_firme_ide_abi_operation(context->abi->id, &operation) !=
         VAL_SUCCESS) ||
        (val_firme_ide_abi_condition(context->test_case->mutation,
                                     &condition) != VAL_SUCCESS))
        return VAL_ERROR;

    if (!context->scratch[IDE_ABI_MAIN_TRACKED] &&
        context->call_issued == VAL_TRUE) {
        status = val_firme_status_to_i64(context->output.registers[0]);
        val_firme_ide_abi_track_main(context, operation, status);
        if (operation == PAL_FIRME_IDE_ABI_POLL && status == FIRME_SUCCESS) {
            if (context->scratch[IDE_ABI_PENDING] &&
                context->scratch[IDE_ABI_PENDING_OP] != 0ULL &&
                context->scratch[IDE_ABI_PENDING_OP] != IDE_ABI_UNCERTAIN &&
                context->output.registers[1] == context->scratch[IDE_ABI_HANDLE] &&
                (condition != PAL_FIRME_IDE_ABI_POLL_ANY ||
                 context->output.registers[2] == context->scratch[IDE_ABI_KEYSET]))
                val_firme_ide_abi_acknowledge(context);
            else {
                context->scratch[IDE_ABI_PENDING] = VAL_TRUE;
                context->scratch[IDE_ABI_PENDING_OP] = IDE_ABI_UNCERTAIN;
            }
        } else if (operation == PAL_FIRME_IDE_ABI_POLL &&
                   context->scratch[IDE_ABI_PENDING] &&
                   status != FIRME_BUSY && status != FIRME_INCOMPLETE) {
            context->scratch[IDE_ABI_PENDING_OP] = IDE_ABI_UNCERTAIN;
        }
    }

    if ((context->scratch[IDE_ABI_PENDING] == VAL_TRUE) &&
        (val_firme_ide_abi_poll_complete(context, VAL_FALSE) != VAL_SUCCESS))
        return val_firme_ide_abi_retain(context);

    val_firme_ide_abi_scenario(context, &scenario);
    if (pal_firme_ide_abi_quiesce((uint32_t)context->world, &scenario) !=
        PAL_FIRME_IDE_ABI_READY)
        return val_firme_ide_abi_retain(context);

    if ((context->scratch[IDE_ABI_PROGRAMMED] == VAL_TRUE ||
         context->scratch[IDE_ABI_ACTIVE] == VAL_TRUE) &&
        (val_firme_ide_abi_stop_cleanup(context) != VAL_SUCCESS))
        return val_firme_ide_abi_retain(context);

    platform_status = pal_firme_ide_abi_cleanup(
        (uint32_t)context->world, operation, condition, &scenario);
    if (platform_status != PAL_FIRME_IDE_ABI_READY) {
        LOG_ERROR(NULL, "%s: IDE PAL cleanup failed status=%u",
                  context->plan->name, (unsigned int)platform_status);
        return val_firme_ide_abi_retain(context);
    }

    context->scratch[IDE_ABI_PREPARED] = VAL_FALSE;
    g_ide_abi_retained_valid = VAL_FALSE;
    g_ide_abi_owned = VAL_FALSE;
    return VAL_SUCCESS;
}

static const val_firme_abi_test_ops_t g_ide_abi_ops = {
    .prepare_state = val_firme_ide_abi_prepare,
    .build_valid_input = val_firme_ide_abi_build,
    .apply_mutation = val_firme_ide_abi_mutate,
    .validate_output = val_firme_ide_abi_validate,
    .validate_effect = val_firme_ide_abi_effect,
    .cleanup = val_firme_ide_abi_cleanup,
};

/**
 * @brief Return the stateful adapter shared by the four IDE ABIs.
 * @param abi_id Generated Alpha1 ABI identifier.
 * @return IDE adapter operations, or NULL for another service.
 */
const val_firme_abi_test_ops_t *val_firme_ide_abi_adapter_get(
    val_firme_abi_id_t abi_id)
{
    if ((abi_id == VAL_FIRME_ABI_ID_IDE_KEYSET_PROG) ||
        (abi_id == VAL_FIRME_ABI_ID_IDE_KEYSET_GO) ||
        (abi_id == VAL_FIRME_ABI_ID_IDE_KEYSET_STOP) ||
        (abi_id == VAL_FIRME_ABI_ID_IDE_KEYSET_POLL))
        return &g_ide_abi_ops;

    return NULL;
}
