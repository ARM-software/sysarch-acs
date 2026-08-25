/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t root_bdf;
    uint32_t endpoint_bdf;
    uint32_t doe_offset;
    uint32_t original_buses;
    uint32_t buses_changed;
} val_host_doe_endpoint_t;

int val_host_doe_prepare_endpoint(val_host_doe_endpoint_t *endpoint);
int val_host_doe_release_endpoint(val_host_doe_endpoint_t *endpoint);
int val_host_doe_discover_spdm(const val_host_doe_endpoint_t *endpoint);
int val_host_doe_send(const val_host_doe_endpoint_t *endpoint,
                      const void *message, size_t message_size);
int val_host_doe_receive(const val_host_doe_endpoint_t *endpoint,
                         void *message, size_t capacity, size_t *message_size);
