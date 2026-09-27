/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_pcie.h"
#include "val_firme_ide.h"
#include "val_firme_rules.h"
#include "val_log.h"

#define IDE_KEYSET_BITMAP_TEST_NAME "ide_keyset_id_bitmap_encoding"

#define IDE_KEYSET_BITMAP_STEP_ENCODE       UINT64_C(1)
#define IDE_KEYSET_BITMAP_STEP_UNSUPPORTED  UINT64_C(2)
#define IDE_KEYSET_BITMAP_STEP_PROG_ACCEPT  UINT64_C(3)
#define IDE_KEYSET_BITMAP_STEP_POLL         UINT64_C(4)
#define IDE_KEYSET_BITMAP_HANDLE            UINT64_C(0x4944454b53494401)

/**
 *   @brief    - Initialize a rule result for keyset bitmap encoding.
 *   @param    - out : Optional result record.
 *   @return   - void
 **/
static void ide_keyset_bitmap_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a keyset bitmap encoding failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void
 **/
static void ide_keyset_bitmap_fail(firme_rule_res_t *out, uint64_t fail_step,
                                   uint64_t last_x0, uint64_t last_x1)
{
    LOG_ERROR(NULL, "%s: failed step=%llu x0=%llx x1=%llx",
              IDE_KEYSET_BITMAP_TEST_NAME, (unsigned long long)fail_step,
              (unsigned long long)last_x0, (unsigned long long)last_x1);

    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Validate that an encoded keyset field can be extracted correctly.
 *   @param    - keyset_id : Encoded keyset ID.
 *   @param    - shift     : Field shift.
 *   @param    - mask      : Field mask.
 *   @param    - expected  : Expected field value.
 *   @return   - VAL_TRUE on match, otherwise VAL_FALSE.
 **/
static uint32_t ide_keyset_bitmap_field_matches(uint64_t keyset_id, uint32_t shift,
                                                uint64_t mask, uint64_t expected)
{
    uint64_t actual = (keyset_id >> shift) & mask;

    return (actual == expected) ? VAL_TRUE : VAL_FALSE;
}

/**
 *   @brief    - Validate the FIRME IDE keyset ID bitmap encoding.
 *   @param    - base_pa : Unused for IDE tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_ide_keyset_id_bitmap_encoding_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    val_firme_ide_keyset_fields_t fields = {0};
    val_firme_ide_prog_args_t prog_args = {0};
    firme_res_t res = {0};
    uint64_t keyset_id = 0ULL;
    uint64_t operation_keyset_id = val_firme_ide_default_keyset_id();
    int64_t status = 0;

    (void)base_pa;
    ide_keyset_bitmap_init_result(out);

    fields.segment = 1U;
    fields.root_port_id = 2U;
    fields.stream_id = 3U;
    fields.substream_id = 4U;
    fields.direction = 1U;
    fields.key_set = 1U;
    keyset_id = val_firme_ide_keyset_id(&fields);

    /* Step 1: Verify ACS encodes each field at the DEN0149 bit position. */
    if ((ide_keyset_bitmap_field_matches(keyset_id,
                                         VAL_FIRME_IDE_KEYSET_SEGMENT_SHIFT,
                                         VAL_FIRME_IDE_KEYSET_SEGMENT_MASK,
                                         fields.segment) == VAL_FALSE) ||
        (ide_keyset_bitmap_field_matches(keyset_id,
                                         VAL_FIRME_IDE_KEYSET_ROOT_PORT_SHIFT,
                                         VAL_FIRME_IDE_KEYSET_ROOT_PORT_MASK,
                                         fields.root_port_id) == VAL_FALSE) ||
        (ide_keyset_bitmap_field_matches(keyset_id,
                                         VAL_FIRME_IDE_KEYSET_STREAM_SHIFT,
                                         VAL_FIRME_IDE_KEYSET_STREAM_MASK,
                                         fields.stream_id) == VAL_FALSE) ||
        (ide_keyset_bitmap_field_matches(keyset_id,
                                         VAL_FIRME_IDE_KEYSET_SUBSTREAM_SHIFT,
                                         VAL_FIRME_IDE_KEYSET_SUBSTREAM_MASK,
                                         fields.substream_id) == VAL_FALSE) ||
        (ide_keyset_bitmap_field_matches(keyset_id,
                                         VAL_FIRME_IDE_KEYSET_DIRECTION_SHIFT,
                                         VAL_FIRME_IDE_KEYSET_DIRECTION_MASK,
                                         fields.direction) == VAL_FALSE) ||
        (ide_keyset_bitmap_field_matches(keyset_id,
                                         VAL_FIRME_IDE_KEYSET_KEY_SET_SHIFT,
                                         VAL_FIRME_IDE_KEYSET_KEY_SET_MASK,
                                         fields.key_set) == VAL_FALSE)) {
        ide_keyset_bitmap_fail(out, IDE_KEYSET_BITMAP_STEP_ENCODE, keyset_id, 0);
        return VAL_ERROR;
    }

    /* Step 2: Skip if the current Realm FIRME instance does not implement IDE KM. */
    val_firme_service_version(FIRME_SERVICE_ID_IDE_KEY_MGMT, &res);
    status = val_firme_status_to_i64(res.x0);
    if (status == (int64_t)FIRME_NOT_SUPPORTED) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }
    if (status < 0) {
        ide_keyset_bitmap_fail(out, IDE_KEYSET_BITMAP_STEP_UNSUPPORTED, res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 3: A valid platform keyset must not be rejected as INVALID_PARAMETERS. */
    val_firme_ide_prog_args_init(&prog_args, val_pcie_ecam_base(),
                                 VAL_FIRME_IDE_TYPE_SELECTIVE,
                                 operation_keyset_id,
                                 IDE_KEYSET_BITMAP_HANDLE);
    val_firme_ide_keyset_prog_do_retry(&prog_args, &res);
    status = val_firme_status_to_i64(res.x0);
    if (val_firme_ide_status_can_complete(status) == VAL_FALSE) {
        ide_keyset_bitmap_fail(out, IDE_KEYSET_BITMAP_STEP_PROG_ACCEPT,
                               res.x0, operation_keyset_id);
        return VAL_ERROR;
    }
    if (status == (int64_t)FIRME_INCOMPLETE) {
        val_firme_ide_keyset_poll_do_retry(val_pcie_ecam_base(),
                                           val_firme_ide_poll_flags(VAL_FALSE),
                                           operation_keyset_id, &res);
        if ((val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) ||
            (res.x1 != IDE_KEYSET_BITMAP_HANDLE)) {
            ide_keyset_bitmap_fail(out, IDE_KEYSET_BITMAP_STEP_POLL, res.x0, res.x1);
            return VAL_ERROR;
        }
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}
