/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include "val_common/val_mp.h"
#include "val_firme_mp_test.h"

typedef int (*val_host_mp_callback_t)(uint32_t pe_index, void *context);

/* Only the boot PE controls workers. Callbacks must use private/static storage. */
int val_host_mp_start(uint32_t pe_index);
int val_host_mp_dispatch(uint32_t pe_index, val_host_mp_callback_t callback,
                         void *context);
int val_host_mp_wait(uint32_t pe_index, int *callback_result);
int val_host_mp_stop(uint32_t pe_index);
int val_host_mp_self_test(void);
void val_host_mp_init(void);
uint32_t val_host_mp_primary_index(void);
void val_host_secondary_main(uint64_t context_id);
int val_host_run_test_payload(uint32_t num_pe, const val_test_payload_t *payload,
                              val_test_pe_result_t *results);
int val_host_test_payload_healthy(void);
