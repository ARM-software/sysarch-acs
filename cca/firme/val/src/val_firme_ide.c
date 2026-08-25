/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_ide.h"

#include "aarch64/sysreg.h"
#include "val_pcie.h"
#include "val_common/val_smccc.h"
#include "val_common/val_status.h"

#define VAL_FIRME_IDE_TEST_KEY_BASE UINT64_C(0x4649524d45494445)
#define VAL_FIRME_IDE_TEST_KEY_STEP UINT64_C(0x0101010101010101)

/**
 *   @brief    - Encode an IDE type into FIRME IDE Input flags.
 *   @param    - ide_type : IDE type encoding from DEN0149.
 *   @return   - Encoded Input flags value.
 **/
uint64_t val_firme_ide_flags(uint32_t ide_type)
{
    return (uint64_t)(ide_type & VAL_FIRME_IDE_TYPE_MASK);
}

/**
 *   @brief    - Encode FIRME_IDE_KEYSET_POLL Input flags.
 *   @param    - any_pending_response : Non-zero requests any pending response.
 *   @return   - Encoded poll Input flags value.
 **/
uint64_t val_firme_ide_poll_flags(uint32_t any_pending_response)
{
    return (any_pending_response == VAL_FALSE) ? 0ULL : 1ULL;
}

/**
 *   @brief    - Encode a FIRME IDE keyset ID from named fields.
 *   @param    - fields : Keyset ID fields.
 *   @return   - Encoded 64-bit keyset ID, or zero if fields is NULL.
 **/
uint64_t val_firme_ide_keyset_id(const val_firme_ide_keyset_fields_t *fields)
{
    uint64_t keyset_id = 0ULL;

    if (!fields)
        return 0ULL;

    keyset_id |= ((uint64_t)fields->key_set & VAL_FIRME_IDE_KEYSET_KEY_SET_MASK)
                 << VAL_FIRME_IDE_KEYSET_KEY_SET_SHIFT;
    keyset_id |= ((uint64_t)fields->direction & VAL_FIRME_IDE_KEYSET_DIRECTION_MASK)
                 << VAL_FIRME_IDE_KEYSET_DIRECTION_SHIFT;
    keyset_id |= ((uint64_t)fields->substream_id & VAL_FIRME_IDE_KEYSET_SUBSTREAM_MASK)
                 << VAL_FIRME_IDE_KEYSET_SUBSTREAM_SHIFT;
    keyset_id |= ((uint64_t)fields->stream_id & VAL_FIRME_IDE_KEYSET_STREAM_MASK)
                 << VAL_FIRME_IDE_KEYSET_STREAM_SHIFT;
    keyset_id |= ((uint64_t)fields->root_port_id & VAL_FIRME_IDE_KEYSET_ROOT_PORT_MASK)
                 << VAL_FIRME_IDE_KEYSET_ROOT_PORT_SHIFT;
    keyset_id |= ((uint64_t)fields->segment & VAL_FIRME_IDE_KEYSET_SEGMENT_MASK)
                 << VAL_FIRME_IDE_KEYSET_SEGMENT_SHIFT;

    return keyset_id;
}

/**
 *   @brief    - Return the default platform keyset ID for IDE tests.
 *   @return   - Encoded keyset ID using PAL-provided PCIe fields.
 **/
uint64_t val_firme_ide_default_keyset_id(void)
{
    val_firme_ide_keyset_fields_t fields = {0};

    fields.segment = val_pcie_default_segment();
    fields.root_port_id = val_pcie_default_root_port_id();
    fields.stream_id = val_pcie_default_stream_id();
    fields.substream_id = val_pcie_default_substream_id();
    fields.direction = val_pcie_default_direction();
    fields.key_set = val_pcie_default_key_set();

    return val_firme_ide_keyset_id(&fields);
}

/**
 *   @brief    - Return a CXL.cachemem Link IDE keyset ID for field checks.
 *   @param    - key_set      : Key Set field value.
 *   @param    - stream_id    : Stream ID field value.
 *   @param    - substream_id : Substream ID field value.
 *   @return   - Encoded CXL.cachemem keyset ID.
 **/
uint64_t val_firme_ide_cxl_keyset_id(uint32_t key_set, uint32_t stream_id,
                                     uint32_t substream_id)
{
    val_firme_ide_keyset_fields_t fields = {0};

    fields.segment = val_pcie_default_segment();
    fields.root_port_id = val_pcie_default_root_port_id();
    fields.stream_id = stream_id;
    fields.substream_id = substream_id;
    fields.direction = val_pcie_default_direction();
    fields.key_set = key_set;

    return val_firme_ide_keyset_id(&fields);
}

/**
 *   @brief    - Return deterministic test key material for IDE key programming.
 *   @param    - index : Key quad-word index.
 *   @return   - Test key quad-word.
 **/
uint64_t val_firme_ide_test_key_qw(uint32_t index)
{
    return VAL_FIRME_IDE_TEST_KEY_BASE + (VAL_FIRME_IDE_TEST_KEY_STEP * (uint64_t)index);
}

/**
 *   @brief    - Initialize FIRME_IDE_KEYSET_PROG arguments with test key material.
 *   @param    - args         : Programming arguments to initialize.
 *   @param    - ecam_address : ECAM base address of the target Root port.
 *   @param    - ide_type     : IDE type encoding from DEN0149.
 *   @param    - keyset_id    : FIRME IDE keyset ID.
 *   @param    - handle       : Caller handle for non-blocking tracking.
 *   @return   - void
 **/
void val_firme_ide_prog_args_init(val_firme_ide_prog_args_t *args,
                                  uint64_t ecam_address, uint32_t ide_type,
                                  uint64_t keyset_id, uint64_t handle)
{
    if (!args)
        return;

    args->ecam_address = ecam_address;
    args->flags = val_firme_ide_flags(ide_type);
    args->keyset_id = keyset_id;
    /* Keep these as scalar stores; some worlds enforce strict alignment. */
    ((volatile uint64_t *)&args->keyqw0)[0] = val_firme_ide_test_key_qw(0U);
    ((volatile uint64_t *)&args->keyqw0)[1] = val_firme_ide_test_key_qw(1U);
    ((volatile uint64_t *)&args->keyqw0)[2] = val_firme_ide_test_key_qw(2U);
    ((volatile uint64_t *)&args->keyqw0)[3] = val_firme_ide_test_key_qw(3U);
    args->handle = handle;
}

/**
 *   @brief    - Check whether an IDE ABI status represents accepted operation progress.
 *   @param    - status : FIRME status value.
 *   @return   - VAL_TRUE for SUCCESS or INCOMPLETE, otherwise VAL_FALSE.
 **/
uint32_t val_firme_ide_status_can_complete(int64_t status)
{
    if (status == (int64_t)FIRME_SUCCESS)
        return VAL_TRUE;

    if (status == (int64_t)FIRME_INCOMPLETE)
        return VAL_TRUE;

    return VAL_FALSE;
}

/**
 *   @brief    - Query whether the IDE key management service is present.
 *   @param    - out_res : Optional raw SERVICE_VERSION result.
 *   @return   - VAL_TRUE if implemented, otherwise VAL_FALSE.
 **/
uint32_t val_firme_ide_service_present(firme_res_t *out_res)
{
    firme_res_t res = {0};

    val_firme_service_version(FIRME_SERVICE_ID_IDE_KEY_MGMT, &res);
    if (out_res)
        *out_res = res;

    return (val_firme_status_to_i64(res.x0) >= 0) ? VAL_TRUE : VAL_FALSE;
}

/**
 *   @brief    - Query whether FIRME_IDE_KEYSET_POLL is advertised.
 *   @param    - out_res  : Optional raw SERVICE_FEATURES result.
 *   @param    - out_reg0 : Optional feature register 0 value.
 *   @return   - VAL_TRUE if POLL is advertised, otherwise VAL_FALSE.
 **/
uint32_t val_firme_ide_poll_advertised(firme_res_t *out_res, uint64_t *out_reg0)
{
    firme_res_t res = {0};

    val_firme_service_features(FIRME_SERVICE_ID_IDE_KEY_MGMT,
                               FIRME_IDE_FEATURE_REG0_INDEX, &res);
    if (out_res)
        *out_res = res;
    if (out_reg0)
        *out_reg0 = res.x1;

    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS)
        return VAL_FALSE;

    return ((res.x1 & FIRME_IDE_FEATURE_REG0_POLL_BIT) != 0ULL) ? VAL_TRUE : VAL_FALSE;
}

/**
 *   @brief    - Invoke FIRME_IDE_KEYSET_PROG from the current world.
 *   @param    - args : FIRME IDE keyset programming arguments.
 *   @param    - out  : Returned FIRME registers.
 *   @return   - void
 **/
void val_firme_ide_keyset_prog(const val_firme_ide_prog_args_t *args, firme_res_t *out)
{
    smccc_res_t res = {0};
    uint64_t flags = 0ULL;
    uint64_t keyset_id = 0ULL;

    if (!args) {
        if (out)
            out->x0 = (uint64_t)FIRME_INVALID_PARAMETERS;
        return;
    }

    flags = args->flags & VAL_FIRME_IDE_INPUT_FLAGS_MASK;
    keyset_id = args->keyset_id & VAL_FIRME_IDE_KEYSET_ID_INPUT_MASK;
    val_smccc_smc_call_x8(FIRME_FID_IDE_KEYSET_PROG, args->ecam_address, flags,
                          keyset_id, args->keyqw0, args->keyqw1, args->keyqw2,
                          args->keyqw3, args->handle, &res);
    if (out) {
        out->x0 = res.x0;
        out->x1 = res.x1;
        out->x2 = res.x2;
        out->x3 = res.x3;
    }
}

/**
 *   @brief    - Invoke FIRME_IDE_KEYSET_GO from the current world.
 *   @param    - ecam_address : ECAM base address of the target Root port.
 *   @param    - flags        : FIRME IDE Input flags.
 *   @param    - keyset_id    : FIRME IDE keyset ID.
 *   @param    - handle       : Caller handle for non-blocking operation tracking.
 *   @param    - out          : Returned FIRME registers.
 *   @return   - void
 **/
void val_firme_ide_keyset_go(uint64_t ecam_address, uint64_t flags, uint64_t keyset_id,
                             uint64_t handle, firme_res_t *out)
{
    smccc_res_t res = {0};

    flags &= VAL_FIRME_IDE_INPUT_FLAGS_MASK;
    keyset_id &= VAL_FIRME_IDE_KEYSET_ID_INPUT_MASK;
    val_smccc_smc_call(FIRME_FID_IDE_KEYSET_GO, ecam_address, flags, keyset_id,
                       handle, 0, 0, 0, &res);
    if (out) {
        out->x0 = res.x0;
        out->x1 = res.x1;
        out->x2 = res.x2;
        out->x3 = res.x3;
    }
}

/**
 *   @brief    - Invoke FIRME_IDE_KEYSET_STOP from the current world.
 *   @param    - ecam_address : ECAM base address of the target Root port.
 *   @param    - flags        : FIRME IDE Input flags.
 *   @param    - keyset_id    : FIRME IDE keyset ID.
 *   @param    - handle       : Caller handle for non-blocking operation tracking.
 *   @param    - out          : Returned FIRME registers.
 *   @return   - void
 **/
void val_firme_ide_keyset_stop(uint64_t ecam_address, uint64_t flags, uint64_t keyset_id,
                               uint64_t handle, firme_res_t *out)
{
    smccc_res_t res = {0};

    flags &= VAL_FIRME_IDE_INPUT_FLAGS_MASK;
    keyset_id &= VAL_FIRME_IDE_KEYSET_ID_INPUT_MASK;
    val_smccc_smc_call(FIRME_FID_IDE_KEYSET_STOP, ecam_address, flags, keyset_id,
                       handle, 0, 0, 0, &res);
    if (out) {
        out->x0 = res.x0;
        out->x1 = res.x1;
        out->x2 = res.x2;
        out->x3 = res.x3;
    }
}

/**
 *   @brief    - Invoke FIRME_IDE_KEYSET_POLL from the current world.
 *   @param    - ecam_address : ECAM base address of the target Root port.
 *   @param    - flags        : FIRME IDE poll Input flags.
 *   @param    - keyset_id    : FIRME IDE keyset ID.
 *   @param    - out          : Returned FIRME registers.
 *   @return   - void
 **/
void val_firme_ide_keyset_poll(uint64_t ecam_address, uint64_t flags, uint64_t keyset_id,
                               firme_res_t *out)
{
    smccc_res_t res = {0};

    flags &= VAL_FIRME_IDE_POLL_FLAGS_MASK;
    keyset_id &= VAL_FIRME_IDE_KEYSET_ID_INPUT_MASK;
    val_smccc_smc_call(FIRME_FID_IDE_KEYSET_POLL, ecam_address, flags, keyset_id,
                       0, 0, 0, 0, &res);
    if (out) {
        out->x0 = res.x0;
        out->x1 = res.x1;
        out->x2 = res.x2;
        out->x3 = res.x3;
    }
}

/**
 *   @brief    - Invoke PROG and retry while the Root port reports BUSY.
 *   @param    - args : FIRME IDE keyset programming arguments.
 *   @param    - out  : Returned FIRME registers.
 *   @return   - void
 **/
void val_firme_ide_keyset_prog_do_retry(const val_firme_ide_prog_args_t *args,
                                        firme_res_t *out)
{
    firme_res_t local = {0};
    uint32_t attempt = 0U;

    for (attempt = 0U; attempt < VAL_FIRME_IDE_RETRY_LIMIT; attempt++) {
        val_firme_ide_keyset_prog(args, &local);
        if (val_firme_status_to_i64(local.x0) != (int64_t)FIRME_BUSY)
            break;
        val_mp_relax();
    }

    if (out)
        *out = local;
}

/**
 *   @brief    - Invoke GO and retry while the Root port reports BUSY.
 *   @param    - ecam_address : ECAM base address of the target Root port.
 *   @param    - flags        : FIRME IDE Input flags.
 *   @param    - keyset_id    : FIRME IDE keyset ID.
 *   @param    - handle       : Caller handle for non-blocking operation tracking.
 *   @param    - out          : Returned FIRME registers.
 *   @return   - void
 **/
void val_firme_ide_keyset_go_do_retry(uint64_t ecam_address, uint64_t flags,
                                      uint64_t keyset_id, uint64_t handle,
                                      firme_res_t *out)
{
    firme_res_t local = {0};
    uint32_t attempt = 0U;

    for (attempt = 0U; attempt < VAL_FIRME_IDE_RETRY_LIMIT; attempt++) {
        val_firme_ide_keyset_go(ecam_address, flags, keyset_id, handle, &local);
        if (val_firme_status_to_i64(local.x0) != (int64_t)FIRME_BUSY)
            break;
        val_mp_relax();
    }

    if (out)
        *out = local;
}

/**
 *   @brief    - Invoke STOP and retry while the Root port reports BUSY.
 *   @param    - ecam_address : ECAM base address of the target Root port.
 *   @param    - flags        : FIRME IDE Input flags.
 *   @param    - keyset_id    : FIRME IDE keyset ID.
 *   @param    - handle       : Caller handle for non-blocking operation tracking.
 *   @param    - out          : Returned FIRME registers.
 *   @return   - void
 **/
void val_firme_ide_keyset_stop_do_retry(uint64_t ecam_address, uint64_t flags,
                                        uint64_t keyset_id, uint64_t handle,
                                        firme_res_t *out)
{
    firme_res_t local = {0};
    uint32_t attempt = 0U;

    for (attempt = 0U; attempt < VAL_FIRME_IDE_RETRY_LIMIT; attempt++) {
        val_firme_ide_keyset_stop(ecam_address, flags, keyset_id, handle, &local);
        if (val_firme_status_to_i64(local.x0) != (int64_t)FIRME_BUSY)
            break;
        val_mp_relax();
    }

    if (out)
        *out = local;
}

/**
 *   @brief    - Invoke POLL and retry while the Root port reports BUSY/INCOMPLETE.
 *   @param    - ecam_address : ECAM base address of the target Root port.
 *   @param    - flags        : FIRME IDE poll Input flags.
 *   @param    - keyset_id    : FIRME IDE keyset ID.
 *   @param    - out          : Returned FIRME registers.
 *   @return   - void
 **/
void val_firme_ide_keyset_poll_do_retry(uint64_t ecam_address, uint64_t flags,
                                        uint64_t keyset_id, firme_res_t *out)
{
    firme_res_t local = {0};
    uint32_t attempt = 0U;
    int64_t status = 0;

    for (attempt = 0U; attempt < VAL_FIRME_IDE_POLL_RETRY_LIMIT; attempt++) {
        val_firme_ide_keyset_poll(ecam_address, flags, keyset_id, &local);
        status = val_firme_status_to_i64(local.x0);
        if ((status != (int64_t)FIRME_BUSY) && (status != (int64_t)FIRME_INCOMPLETE))
            break;
        val_mp_relax();
    }

    if (out)
        *out = local;
}
