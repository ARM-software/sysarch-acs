/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../../val/host/val_host_spdm.c"

/**
 * @brief Bind the native buffer test directly to its requester owner.
 * @return The libspdm status for the requested application-context field.
 */
libspdm_return_t libspdm_get_data(void *spdm_context,
                                  libspdm_data_type_t data_type,
                                  const libspdm_data_parameter_t *parameter,
                                  void *data, size_t *data_size)
{
    if (!spdm_context || !data || !data_size ||
        data_type != LIBSPDM_DATA_APP_CONTEXT_DATA ||
        parameter->location != LIBSPDM_DATA_LOCATION_LOCAL ||
        *data_size != sizeof(spdm_context))
        return LIBSPDM_STATUS_INVALID_PARAMETER;
    memcpy(data, &spdm_context, sizeof(spdm_context));
    return LIBSPDM_STATUS_SUCCESS;
}

/**
 * @brief Exercise full-capacity sender and receiver acquisition and release.
 * @return VAL_SUCCESS after all buffer-contract assertions pass.
 */
int main(void)
{
    uint8_t sender[VAL_HOST_SPDM_BUFFER_BYTES + 1U] = {0};
    uint8_t receiver[VAL_HOST_SPDM_BUFFER_BYTES + 1U] = {0};
    val_host_spdm_context_t owner = {0};
    void *buffer = NULL;
    void *second = NULL;

    owner.sender_buffer = sender;
    owner.receiver_buffer = receiver;
    sender[VAL_HOST_SPDM_BUFFER_BYTES] = 0xa5U;
    receiver[VAL_HOST_SPDM_BUFFER_BYTES] = 0xa5U;

    assert(val_host_spdm_acquire_sender(&owner, &buffer) == LIBSPDM_STATUS_SUCCESS);
    assert(buffer == sender);
    memset(buffer, 0x5a, VAL_HOST_SPDM_BUFFER_BYTES);
    assert(sender[VAL_HOST_SPDM_BUFFER_BYTES] == 0xa5U);
    assert(val_host_spdm_acquire_sender(&owner, &second) == LIBSPDM_STATUS_ACQUIRE_FAIL);
    val_host_spdm_release_sender(&owner, sender + 1U);
    assert(owner.sender_in_use == VAL_TRUE);
    val_host_spdm_release_sender(&owner, buffer);
    assert(owner.sender_in_use == VAL_FALSE);
    assert(val_host_spdm_acquire_sender(&owner, &buffer) == LIBSPDM_STATUS_SUCCESS);
    val_host_spdm_release_sender(&owner, buffer);

    assert(val_host_spdm_acquire_receiver(&owner, &buffer) == LIBSPDM_STATUS_SUCCESS);
    assert(buffer == receiver);
    memset(buffer, 0x5a, VAL_HOST_SPDM_BUFFER_BYTES);
    assert(receiver[VAL_HOST_SPDM_BUFFER_BYTES] == 0xa5U);
    assert(val_host_spdm_acquire_receiver(&owner, &second) == LIBSPDM_STATUS_ACQUIRE_FAIL);
    val_host_spdm_release_receiver(&owner, receiver + 1U);
    assert(owner.receiver_in_use == VAL_TRUE);
    val_host_spdm_release_receiver(&owner, buffer);
    assert(owner.receiver_in_use == VAL_FALSE);
    puts("SPDM buffer contract: PASS");
    return VAL_SUCCESS;
}
