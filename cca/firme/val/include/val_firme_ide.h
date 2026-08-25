/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#include "val_firme_abi.h"
#include "val_common/val_status.h"

#define VAL_FIRME_IDE_TYPE_SELECTIVE       0U
#define VAL_FIRME_IDE_TYPE_PCIE_LINK       1U
#define VAL_FIRME_IDE_TYPE_UNUSED_TEST     2U
#define VAL_FIRME_IDE_TYPE_CXL_CACHEMEM    3U
#define VAL_FIRME_IDE_TYPE_MASK            0x3U
#define VAL_FIRME_IDE_INPUT_FLAGS_MASK     UINT64_C(0x3)
#define VAL_FIRME_IDE_POLL_FLAGS_MASK      UINT64_C(0x1)
#define VAL_FIRME_IDE_KEYSET_ID_INPUT_MASK UINT64_C(0x3fffffffff)

#define VAL_FIRME_IDE_KEYSET_KEY_SET_SHIFT     0U
#define VAL_FIRME_IDE_KEYSET_DIRECTION_SHIFT   1U
#define VAL_FIRME_IDE_KEYSET_SUBSTREAM_SHIFT   2U
#define VAL_FIRME_IDE_KEYSET_STREAM_SHIFT      6U
#define VAL_FIRME_IDE_KEYSET_ROOT_PORT_SHIFT   14U
#define VAL_FIRME_IDE_KEYSET_SEGMENT_SHIFT     30U
#define VAL_FIRME_IDE_KEYSET_KEY_SET_MASK      0x1ULL
#define VAL_FIRME_IDE_KEYSET_DIRECTION_MASK    0x1ULL
#define VAL_FIRME_IDE_KEYSET_SUBSTREAM_MASK    0xfULL
#define VAL_FIRME_IDE_KEYSET_STREAM_MASK       0xffULL
#define VAL_FIRME_IDE_KEYSET_ROOT_PORT_MASK    0xffffULL
#define VAL_FIRME_IDE_KEYSET_SEGMENT_MASK      0xffULL
#define VAL_FIRME_IDE_CXL_SUBSTREAM_ID         0x8U

#define VAL_FIRME_IDE_RETRY_LIMIT      64U
#define VAL_FIRME_IDE_POLL_RETRY_LIMIT 256U

typedef struct {
    uint32_t segment;
    uint32_t root_port_id;
    uint32_t stream_id;
    uint32_t substream_id;
    uint32_t direction;
    uint32_t key_set;
} val_firme_ide_keyset_fields_t;

typedef struct {
    uint64_t ecam_address;
    uint64_t flags;
    uint64_t keyset_id;
    uint64_t keyqw0;
    uint64_t keyqw1;
    uint64_t keyqw2;
    uint64_t keyqw3;
    uint64_t handle;
} val_firme_ide_prog_args_t;

uint64_t val_firme_ide_flags(uint32_t ide_type);
uint64_t val_firme_ide_poll_flags(uint32_t any_pending_response);
uint64_t val_firme_ide_keyset_id(const val_firme_ide_keyset_fields_t *fields);
uint64_t val_firme_ide_default_keyset_id(void);
uint64_t val_firme_ide_cxl_keyset_id(uint32_t key_set, uint32_t stream_id,
                                     uint32_t substream_id);
uint64_t val_firme_ide_test_key_qw(uint32_t index);
void val_firme_ide_prog_args_init(val_firme_ide_prog_args_t *args,
                                  uint64_t ecam_address, uint32_t ide_type,
                                  uint64_t keyset_id, uint64_t handle);
uint32_t val_firme_ide_status_can_complete(int64_t status);
uint32_t val_firme_ide_service_present(firme_res_t *out_res);
uint32_t val_firme_ide_poll_advertised(firme_res_t *out_res, uint64_t *out_reg0);
void val_firme_ide_keyset_prog(const val_firme_ide_prog_args_t *args, firme_res_t *out);
void val_firme_ide_keyset_go(uint64_t ecam_address, uint64_t flags, uint64_t keyset_id,
                             uint64_t handle, firme_res_t *out);
void val_firme_ide_keyset_stop(uint64_t ecam_address, uint64_t flags, uint64_t keyset_id,
                               uint64_t handle, firme_res_t *out);
void val_firme_ide_keyset_poll(uint64_t ecam_address, uint64_t flags, uint64_t keyset_id,
                               firme_res_t *out);
void val_firme_ide_keyset_prog_do_retry(const val_firme_ide_prog_args_t *args,
                                        firme_res_t *out);
void val_firme_ide_keyset_go_do_retry(uint64_t ecam_address, uint64_t flags,
                                      uint64_t keyset_id, uint64_t handle,
                                      firme_res_t *out);
void val_firme_ide_keyset_stop_do_retry(uint64_t ecam_address, uint64_t flags,
                                        uint64_t keyset_id, uint64_t handle,
                                        firme_res_t *out);
void val_firme_ide_keyset_poll_do_retry(uint64_t ecam_address, uint64_t flags,
                                        uint64_t keyset_id, firme_res_t *out);
