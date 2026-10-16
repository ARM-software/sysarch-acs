/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_ide_namespace.h"

#include "val_firme_abi.h"
#include "val_firme_ide.h"
#include "val_log.h"
#include "val_common/val_mp.h"
#include "val_common/val_mem.h"

#define IDE_NAMESPACE_PATH_COUNT 2U
#define IDE_NAMESPACE_KEY_QWORDS 4U
#define IDE_NAMESPACE_STEP_SERVICE UINT64_C(1)
#define IDE_NAMESPACE_STEP_FEATURE UINT64_C(2)
#define IDE_NAMESPACE_STEP_PREPARE UINT64_C(3)
#define IDE_NAMESPACE_STEP_PATHS UINT64_C(4)
#define IDE_NAMESPACE_STEP_PROGRAM UINT64_C(5)
#define IDE_NAMESPACE_STEP_GO UINT64_C(6)
#define IDE_NAMESPACE_STEP_BOTH_ACTIVE UINT64_C(7)
#define IDE_NAMESPACE_STEP_STOP_FIRST UINT64_C(8)
#define IDE_NAMESPACE_STEP_FIRST_STOPPED UINT64_C(9)
#define IDE_NAMESPACE_STEP_STOP_SECOND UINT64_C(10)
#define IDE_NAMESPACE_STEP_BOTH_STOPPED UINT64_C(11)
#define IDE_NAMESPACE_STEP_CLEANUP UINT64_C(12)

typedef enum {
    IDE_NAMESPACE_PROGRAM = 0,
    IDE_NAMESPACE_GO,
    IDE_NAMESPACE_STOP,
} ide_namespace_operation_t;

typedef struct {
    pal_firme_ide_namespace_scenario_t scenario;
    pal_firme_ide_namespace_kind_t kind;
    uint32_t prepared;
    uint32_t pe_index;
    uint32_t programmed[IDE_NAMESPACE_PATH_COUNT];
    uint32_t stopped[IDE_NAMESPACE_PATH_COUNT];
    uint32_t pending[IDE_NAMESPACE_PATH_COUNT];
    uint32_t quarantined[IDE_NAMESPACE_PATH_COUNT];
    ide_namespace_operation_t operation[IDE_NAMESPACE_PATH_COUNT];
} ide_namespace_owner_t;

static ide_namespace_owner_t g_ide_namespace_owner;
static uint64_t g_ide_namespace_handle;

/**
 * @brief Report missing host observation without claiming status-only coverage.
 * @param world Target FIRME instance, not the world executing the host observer.
 * @param out Explicit skip when no linked host SPDM backend is available.
 * @return VAL_SUCCESS for unavailable evidence, VAL_ERROR for invalid arguments.
 */
__attribute__((weak)) int val_firme_ide_namespace_host_run(firme_acs_world_t world,
                                                          firme_rule_res_t *out)
{
    if (!out)
        return VAL_ERROR;
    *out = (firme_rule_res_t){.status = ACS_STATUS_FAIL};
    if (world != FIRME_ACS_WORLD_NSEL2 && world != FIRME_ACS_WORLD_REALM)
        return VAL_ERROR;

    LOG_WARN(NULL, "R0212: world=%u has no paired keyset-identity observer\n",
             (unsigned int)world);
    out->status = ACS_STATUS_SKIP;
    return VAL_SUCCESS;
}

/**
 * @brief Record a failed IDE namespace check and the last DUT response.
 * @param out Rule result.
 * @param step Failing check number.
 * @param result Last DUT response, if the check invoked an ABI.
 * @return VAL_ERROR.
 */
static int ide_namespace_fail(firme_rule_res_t *out, uint64_t step,
                              const firme_res_t *result)
{
    out->status = (uint64_t)ACS_STATUS_FAIL;
    if (out->fail_step == 0ULL) {
        out->fail_step = step;
        if (result) {
            out->last_x0 = result->x0;
            out->last_x1 = result->x1;
        }
    }

    LOG_ERROR(NULL, "IDE namespace rule: step=%llu first=%llu x0=%llx x1=%llx",
              (unsigned long long)step, (unsigned long long)out->fail_step,
              (unsigned long long)out->last_x0, (unsigned long long)out->last_x1);
    return VAL_ERROR;
}

/**
 * @brief Reject a scenario that cannot distinguish two keyset contexts.
 * @param kind Namespace property under test.
 * @param scenario Platform-selected endpoint and Root Port paths.
 * @return VAL_TRUE for two independent paths, otherwise VAL_FALSE.
 */
static uint32_t ide_namespace_paths_valid(
    pal_firme_ide_namespace_kind_t kind,
    const pal_firme_ide_namespace_scenario_t *scenario)
{
    const pal_firme_ide_namespace_path_t *first;
    const pal_firme_ide_namespace_path_t *second;
    uint32_t key_index;
    uint32_t distinct_keys = VAL_FALSE;

    if (!scenario)
        return VAL_FALSE;

    first = &scenario->path[0];
    second = &scenario->path[1];

    if (kind == PAL_FIRME_IDE_NAMESPACE_BY_ECAM) {
        if ((first->ecam_address == second->ecam_address) ||
            (first->keyset_id != second->keyset_id))
            return VAL_FALSE;
    } else if (kind == PAL_FIRME_IDE_NAMESPACE_BY_KEYSET) {
        if ((first->ecam_address != second->ecam_address) ||
            (first->keyset_id == second->keyset_id))
            return VAL_FALSE;
    } else {
        return VAL_FALSE;
    }

    if ((first->keyset_id & ~VAL_FIRME_IDE_KEYSET_ID_INPUT_MASK) != 0ULL ||
        (second->keyset_id & ~VAL_FIRME_IDE_KEYSET_ID_INPUT_MASK) != 0ULL ||
        (first->handle == 0ULL) || (second->handle == 0ULL) ||
        (first->handle == second->handle))
        return VAL_FALSE;

    if (((first->ide_type != VAL_FIRME_IDE_TYPE_SELECTIVE) &&
         (first->ide_type != VAL_FIRME_IDE_TYPE_PCIE_LINK) &&
         (first->ide_type != VAL_FIRME_IDE_TYPE_CXL_CACHEMEM)) ||
        ((second->ide_type != VAL_FIRME_IDE_TYPE_SELECTIVE) &&
         (second->ide_type != VAL_FIRME_IDE_TYPE_PCIE_LINK) &&
         (second->ide_type != VAL_FIRME_IDE_TYPE_CXL_CACHEMEM)))
        return VAL_FALSE;

    if ((first->ide_type == VAL_FIRME_IDE_TYPE_CXL_CACHEMEM ||
         second->ide_type == VAL_FIRME_IDE_TYPE_CXL_CACHEMEM) &&
        first->ecam_address == second->ecam_address &&
        (first->keyset_id >> VAL_FIRME_IDE_KEYSET_ROOT_PORT_SHIFT) ==
        (second->keyset_id >> VAL_FIRME_IDE_KEYSET_ROOT_PORT_SHIFT))
        return VAL_FALSE;

    for (key_index = 0U; key_index < IDE_NAMESPACE_PATH_COUNT; key_index++) {
        if (scenario->path[key_index].ide_type == VAL_FIRME_IDE_TYPE_CXL_CACHEMEM &&
            (((scenario->path[key_index].keyset_id >> VAL_FIRME_IDE_KEYSET_STREAM_SHIFT) &
              VAL_FIRME_IDE_KEYSET_STREAM_MASK) != 0ULL ||
             ((scenario->path[key_index].keyset_id >> VAL_FIRME_IDE_KEYSET_SUBSTREAM_SHIFT) &
              VAL_FIRME_IDE_KEYSET_SUBSTREAM_MASK) != VAL_FIRME_IDE_CXL_SUBSTREAM_ID))
            return VAL_FALSE;
    }

    for (key_index = 0U; key_index < IDE_NAMESPACE_KEY_QWORDS; key_index++) {
        if (first->keyqw[key_index] != second->keyqw[key_index])
            distinct_keys = VAL_TRUE;
    }

    return distinct_keys;
}

/** @brief Apply a correlated completion to its persistent namespace owner. */
static void ide_namespace_acknowledge(uint32_t path_index)
{
    ide_namespace_owner_t *owner = &g_ide_namespace_owner;

    if (owner->operation[path_index] == IDE_NAMESPACE_PROGRAM)
        owner->programmed[path_index] = VAL_TRUE;
    owner->stopped[path_index] = owner->operation[path_index] == IDE_NAMESPACE_STOP;
    owner->pending[path_index] = VAL_FALSE;
}

/** @brief Drain a retained request without releasing or reusing its PAL path. */
static int ide_namespace_drain(uint32_t path_index, firme_res_t *result)
{
    ide_namespace_owner_t *owner = &g_ide_namespace_owner;
    const pal_firme_ide_namespace_path_t *path = &owner->scenario.path[path_index];
    int64_t status;

    if (owner->quarantined[path_index])
        return VAL_ERROR;
    if (!owner->pending[path_index])
        return VAL_SUCCESS;
    val_firme_ide_keyset_poll_do_retry(path->ecam_address,
        val_firme_ide_poll_flags(VAL_FALSE), path->keyset_id, result);
    status = val_firme_status_to_i64(result->x0);
    if (status == FIRME_SUCCESS && result->x1 == path->handle) {
        ide_namespace_acknowledge(path_index);
        return VAL_SUCCESS;
    }
    if (status != FIRME_BUSY && status != FIRME_INCOMPLETE)
        owner->quarantined[path_index] = VAL_TRUE;
    return VAL_ERROR;
}

/**
 * @brief Complete one Root Port operation, including asynchronous POLL.
 * @param path_index Persistent owner of the selected ECAM, keyset and key material.
 * @param operation FIRME operation to invoke.
 * @param result Last ABI response.
 * @return VAL_SUCCESS only after terminal SUCCESS and matching POLL handle.
 */
static int ide_namespace_invoke(
    uint32_t path_index,
    ide_namespace_operation_t operation, firme_res_t *result)
{
    ide_namespace_owner_t *owner = &g_ide_namespace_owner;
    pal_firme_ide_namespace_path_t *path;
    val_firme_ide_prog_args_t program = {0};
    uint64_t flags = 0ULL;
    int64_t status;

    if (path_index >= IDE_NAMESPACE_PATH_COUNT || !result ||
        owner->pending[path_index] || owner->quarantined[path_index] ||
        g_ide_namespace_handle == UINT64_MAX)
        return VAL_ERROR;

    path = &owner->scenario.path[path_index];
    path->handle = ++g_ide_namespace_handle;
    owner->operation[path_index] = operation;
    flags = val_firme_ide_flags(path->ide_type);

    if (operation == IDE_NAMESPACE_PROGRAM) {
        val_firme_ide_prog_args_init(&program, path->ecam_address,
                                     path->ide_type, path->keyset_id,
                                     path->handle);
        program.keyqw0 = path->keyqw[0];
        program.keyqw1 = path->keyqw[1];
        program.keyqw2 = path->keyqw[2];
        program.keyqw3 = path->keyqw[3];
        val_firme_ide_keyset_prog_do_retry(&program, result);
    } else if (operation == IDE_NAMESPACE_GO) {
        val_firme_ide_keyset_go_do_retry(path->ecam_address, flags,
                                         path->keyset_id, path->handle,
                                         result);
    } else if (operation == IDE_NAMESPACE_STOP) {
        val_firme_ide_keyset_stop_do_retry(path->ecam_address, flags,
                                           path->keyset_id, path->handle,
                                           result);
    } else {
        return VAL_ERROR;
    }

    status = val_firme_status_to_i64(result->x0);
    if (status == (int64_t)FIRME_INCOMPLETE) {
        owner->pending[path_index] = VAL_TRUE;
        return ide_namespace_drain(path_index, result);
    }
    if (status == FIRME_SUCCESS) {
        ide_namespace_acknowledge(path_index);
        return VAL_SUCCESS;
    }
    if (status != FIRME_BUSY && status != FIRME_DENIED &&
        status != FIRME_INVALID_PARAMETERS && status != FIRME_NOT_SUPPORTED)
        owner->quarantined[path_index] = VAL_TRUE;
    return VAL_ERROR;
}

/** @brief Release namespace fixtures only after every owned operation and STOP completes. */
static int ide_namespace_cleanup(firme_res_t *result)
{
    ide_namespace_owner_t *owner = &g_ide_namespace_owner;
    uint32_t path_index;
    int status = VAL_SUCCESS;

    if (!owner->prepared)
        return VAL_SUCCESS;
    if (owner->pe_index != val_pe_index())
        return VAL_ERROR;
    for (path_index = 0U; path_index < IDE_NAMESPACE_PATH_COUNT; path_index++) {
        if (ide_namespace_drain(path_index, result) != VAL_SUCCESS) {
            status = VAL_ERROR;
            continue;
        }
        if (owner->programmed[path_index] && !owner->stopped[path_index] &&
            ide_namespace_invoke(path_index, IDE_NAMESPACE_STOP, result) != VAL_SUCCESS)
            status = VAL_ERROR;
    }
    if (status != VAL_SUCCESS)
        return VAL_ERROR;
    if (pal_firme_ide_namespace_cleanup(owner->kind, &owner->scenario) !=
        PAL_FIRME_IDE_NAMESPACE_READY)
        return VAL_ERROR;
    memset(owner, 0, sizeof(*owner));
    return VAL_SUCCESS;
}

/**
 * @brief Check a PAL observation without accepting absent evidence as PASS.
 * @param kind Namespace property under test.
 * @param scenario Platform-selected paths.
 * @param first_active Expected first-path active state.
 * @param second_active Expected second-path active state.
 * @return VAL_SUCCESS only when protected traffic matches both states.
 */
static int ide_namespace_observe(
    pal_firme_ide_namespace_kind_t kind,
    const pal_firme_ide_namespace_scenario_t *scenario,
    uint32_t first_active, uint32_t second_active)
{
    pal_firme_ide_namespace_status_t status;

    if (!scenario)
        return VAL_ERROR;

    status = pal_firme_ide_namespace_observe(
        kind, scenario, first_active, second_active);
    return (status == PAL_FIRME_IDE_NAMESPACE_READY) ?
           VAL_SUCCESS : VAL_ERROR;
}

/**
 * @brief Exercise two keysets serially on the owning PE while checking independent links.
 * @param kind Equal IDs across ECAMs or different IDs within one ECAM.
 * @param out Rule result.
 * @return VAL_SUCCESS on PASS/SKIP, otherwise VAL_ERROR.
 */
int val_firme_ide_namespace_run(pal_firme_ide_namespace_kind_t kind,
                                firme_rule_res_t *out)
{
    ide_namespace_owner_t *owner = &g_ide_namespace_owner;
    pal_firme_ide_namespace_scenario_t *scenario = &owner->scenario;
    pal_firme_ide_namespace_status_t platform_status;
    firme_res_t result = {0};
    uint32_t path_index;
    int test_status = VAL_SUCCESS;

    if (!out)
        return VAL_ERROR;

    out->status = (uint64_t)ACS_STATUS_FAIL;
    out->fail_step = 0ULL;
    out->last_x0 = 0ULL;
    out->last_x1 = 0ULL;
    if (ide_namespace_cleanup(&result) != VAL_SUCCESS)
        return ide_namespace_fail(out, IDE_NAMESPACE_STEP_CLEANUP, &result);

    /* Step 1: Test only an instance that advertises IDE Key Management. */
    val_firme_service_version(FIRME_SERVICE_ID_IDE_KEY_MGMT, &result);
    if (val_firme_status_to_i64(result.x0) == (int64_t)FIRME_NOT_SUPPORTED) {
        out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }
    if (val_firme_status_to_i64(result.x0) < 0)
        return ide_namespace_fail(out, IDE_NAMESPACE_STEP_SERVICE, &result);

    /* Step 2: Require the mandatory PROG, GO, STOP and POLL ABIs. */
    val_firme_service_features(FIRME_SERVICE_ID_IDE_KEY_MGMT,
                               FIRME_IDE_FEATURE_REG0_INDEX, &result);
    if ((val_firme_status_to_i64(result.x0) != (int64_t)FIRME_SUCCESS) ||
        ((result.x1 & FIRME_IDE_FEATURE_REG0_ALL_ABI_BITS) !=
         FIRME_IDE_FEATURE_REG0_ALL_ABI_BITS))
        return ide_namespace_fail(out, IDE_NAMESPACE_STEP_FEATURE, &result);

    /* Step 3: Reserve two real Root Port and endpoint paths. */
    platform_status = pal_firme_ide_namespace_prepare(kind, scenario);
    if (platform_status == PAL_FIRME_IDE_NAMESPACE_UNAVAILABLE) {
        LOG_WARN(NULL, "IDE namespace: independent two-path observer unavailable kind=%u\n",
                 (unsigned int)kind);
        out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }
    if (platform_status != PAL_FIRME_IDE_NAMESPACE_READY)
        return ide_namespace_fail(out, IDE_NAMESPACE_STEP_PREPARE, NULL);
    owner->prepared = VAL_TRUE;
    owner->pe_index = val_pe_index();
    owner->kind = kind;

    /* Step 4: Verify the paths actually exercise the selected namespace. */
    if (ide_namespace_paths_valid(kind, scenario) == VAL_FALSE) {
        LOG_ERROR(NULL,
                  "IDE namespace setup: kind=%u ECAM=%llx/%llx keyset=%llx/%llx",
                  (unsigned int)kind,
                  (unsigned long long)scenario->path[0].ecam_address,
                  (unsigned long long)scenario->path[1].ecam_address,
                  (unsigned long long)scenario->path[0].keyset_id,
                  (unsigned long long)scenario->path[1].keyset_id);
        test_status = ide_namespace_fail(out, IDE_NAMESPACE_STEP_PATHS, NULL);
        goto cleanup;
    }

    /* Step 5: Program distinguishable keys into both Root Port contexts. */
    for (path_index = 0U; path_index < IDE_NAMESPACE_PATH_COUNT; path_index++) {
        if (ide_namespace_invoke(path_index,
                                 IDE_NAMESPACE_PROGRAM, &result) != VAL_SUCCESS) {
            LOG_ERROR(NULL, "IDE namespace PROG: path=%u ECAM=%llx keyset=%llx",
                      (unsigned int)path_index,
                      (unsigned long long)scenario->path[path_index].ecam_address,
                      (unsigned long long)scenario->path[path_index].keyset_id);
            test_status = ide_namespace_fail(out, IDE_NAMESPACE_STEP_PROGRAM,
                                              &result);
            goto cleanup;
        }
    }

    /* Step 6: Start both streams without changing the other keyset. */
    for (path_index = 0U; path_index < IDE_NAMESPACE_PATH_COUNT; path_index++) {
        if (ide_namespace_invoke(path_index,
                                 IDE_NAMESPACE_GO, &result) != VAL_SUCCESS) {
            LOG_ERROR(NULL, "IDE namespace GO: path=%u ECAM=%llx keyset=%llx",
                      (unsigned int)path_index,
                      (unsigned long long)scenario->path[path_index].ecam_address,
                      (unsigned long long)scenario->path[path_index].keyset_id);
            test_status = ide_namespace_fail(out, IDE_NAMESPACE_STEP_GO,
                                              &result);
            goto cleanup;
        }
    }

    /* Step 7: Observe protected traffic through both independent paths. */
    if (ide_namespace_observe(kind, scenario, VAL_TRUE, VAL_TRUE) != VAL_SUCCESS) {
        test_status = ide_namespace_fail(out, IDE_NAMESPACE_STEP_BOTH_ACTIVE,
                                          NULL);
        goto cleanup;
    }

    /* Step 8: Stop the first stream while the second remains active. */
    if (ide_namespace_invoke(0U, IDE_NAMESPACE_STOP,
                             &result) != VAL_SUCCESS) {
        test_status = ide_namespace_fail(out, IDE_NAMESPACE_STEP_STOP_FIRST,
                                          &result);
        goto cleanup;
    }

    /* Step 9: Require the first path to stop without affecting the second. */
    if (ide_namespace_observe(kind, scenario, VAL_FALSE, VAL_TRUE) != VAL_SUCCESS) {
        test_status = ide_namespace_fail(out, IDE_NAMESPACE_STEP_FIRST_STOPPED,
                                          NULL);
        goto cleanup;
    }

    /* Step 10: Stop the remaining stream and check both paths are idle. */
    if (ide_namespace_invoke(1U, IDE_NAMESPACE_STOP,
                             &result) != VAL_SUCCESS) {
        test_status = ide_namespace_fail(out, IDE_NAMESPACE_STEP_STOP_SECOND,
                                          &result);
        goto cleanup;
    }
    if (ide_namespace_observe(kind, scenario, VAL_FALSE, VAL_FALSE) != VAL_SUCCESS)
        test_status = ide_namespace_fail(out, IDE_NAMESPACE_STEP_BOTH_STOPPED,
                                          NULL);

cleanup:
    /* Restore EL3 and endpoint state even when an intermediate check fails. */
    if (ide_namespace_cleanup(&result) != VAL_SUCCESS)
        test_status = ide_namespace_fail(out, IDE_NAMESPACE_STEP_CLEANUP, &result);

    if (test_status != VAL_SUCCESS)
        return VAL_ERROR;

    out->status = (uint64_t)ACS_STATUS_PASS;
    return VAL_SUCCESS;
}
