/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#if FIRME_ENABLE_CXL_SPDM

#include "val_host_spdm.h"
#include "val_host_mp.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "firme_spdm_trust_anchor.h"
#include "industry_standard/spdm.h"
#include "industry_standard/pcidoe.h"
#include "industry_standard/cxl_idekm.h"
#include "library/spdm_common_lib.h"
#include "library/spdm_requester_lib.h"
#include "library/spdm_crypt_lib.h"
#include "library/spdm_transport_pcidoe_lib.h"
#include "library/cxl_ide_km_requester_lib.h"
#include "val_common/val_mem.h"
#include "val_common/val_status.h"
#include "val_common/val_mp.h"
#include "val_firme_ide.h"
#include "val_firme_ide_link.h"
#include "val_log.h"
#include "val_pcie.h"
#include "pal_cxl.h"
#include "pal_firme_ide_abi.h"

#define VAL_HOST_SPDM_MAX_MESSAGE 0x1200U
#define VAL_HOST_SPDM_BUFFER_BYTES \
    (LIBSPDM_PCI_DOE_TRANSPORT_HEADER_SIZE + VAL_HOST_SPDM_MAX_MESSAGE + \
     LIBSPDM_PCI_DOE_TRANSPORT_TAIL_SIZE)
#define VAL_HOST_SPDM_PORT_INDEX 0U
#define VAL_HOST_SPDM_STREAM_ID 0U
#define VAL_HOST_SPDM_ACK_RESERVED_BITS 0x0cU
#define VAL_HOST_SPDM_DIRECTIONS 2U

typedef enum {
    VAL_HOST_IDE_STEP_DISCOVERY = 1,
    VAL_HOST_IDE_STEP_SESSION,
    VAL_HOST_IDE_STEP_PROG,
    VAL_HOST_IDE_STEP_TRAFFIC,
    VAL_HOST_IDE_STEP_KEY_MISMATCH,
    VAL_HOST_IDE_STEP_ROOT_STOP,
    VAL_HOST_IDE_STEP_ROOT_GO,
    VAL_HOST_IDE_STEP_CLEANUP,
} val_host_ide_step_t;

typedef struct {
    const val_host_doe_endpoint_t *endpoint;
    void *spdm_context;
    void *scratch_buffer;
    uint8_t *sender_buffer;
    uint8_t *receiver_buffer;
    size_t scratch_size;
    uint32_t initialized;
    uint32_t sender_in_use;
    uint32_t receiver_in_use;
    uint32_t transport_failed;
    uint32_t session_uncertain;
} val_host_spdm_context_t;

typedef struct __attribute__((packed)) {
    cxl_ide_km_key_prog_t header;
    uint8_t key_buffer[sizeof(cxl_ide_km_aes_256_gcm_key_buffer_t)];
} val_host_cxl_key_prog_request_t;

typedef struct {
    uint64_t keyset_id;
    uint64_t key[4];
    cxl_ide_km_aes_256_gcm_key_buffer_t endpoint_key;
    uint8_t endpoint_direction;
    uint32_t endpoint_programmed;
    uint32_t root_open;
    uint32_t root_programmed;
    uint32_t root_stopped;
    uint32_t endpoint_stopped;
} val_host_spdm_link_direction_t;

typedef struct {
    val_host_doe_endpoint_t endpoint;
    val_host_spdm_context_t owner;
    val_host_spdm_link_direction_t link[VAL_HOST_SPDM_DIRECTIONS];
    firme_acs_world_t world;
    uint32_t session_id;
    uint32_t session_started;
    uint32_t open;
    uint32_t traffic_owned;
    uint32_t quarantined;
    uint32_t abi_owned;
    uint32_t abi_root_stopped;
    pal_firme_ide_abi_operation_t abi_operation;
    uint64_t abi_token;
    uint64_t ecam_address;
} val_host_spdm_transaction_t;

static val_host_spdm_transaction_t g_val_host_spdm_transaction;
static uint64_t g_val_host_spdm_abi_token;

extern bool val_host_spdm_has_rndr(void);

/**
 * @brief Recover the endpoint owner associated with one libspdm context.
 * @param spdm_context Active libspdm context.
 * @return Owner context or NULL when binding is missing.
 */
static val_host_spdm_context_t *val_host_spdm_owner(void *spdm_context)
{
    libspdm_data_parameter_t parameter = {0};
    libspdm_return_t status;
    val_host_spdm_context_t *owner = NULL;
    size_t data_size = sizeof(owner);

    parameter.location = LIBSPDM_DATA_LOCATION_LOCAL;
    status = libspdm_get_data(spdm_context, LIBSPDM_DATA_APP_CONTEXT_DATA,
                              &parameter, &owner, &data_size);
    if (!LIBSPDM_STATUS_IS_SUCCESS(status) || (data_size != sizeof(owner)))
        return NULL;
    return owner;
}

/**
 * @brief Transmit one libspdm-encoded DOE message to the selected endpoint.
 * @param spdm_context Active libspdm context.
 * @param message_size Encoded message length.
 * @param message Encoded message bytes.
 * @param timeout Requested timeout, bounded by the DOE mailbox polling loop.
 * @return libspdm transport status.
 */
static libspdm_return_t val_host_spdm_send(void *spdm_context, size_t message_size,
                                            const void *message, uint64_t timeout)
{
    val_host_spdm_context_t *owner;
    const uint8_t *bytes = message;

    owner = val_host_spdm_owner(spdm_context);
    (void)timeout;
    if (!owner || !owner->endpoint || !message ||
        (message_size > VAL_HOST_SPDM_BUFFER_BYTES))
        return LIBSPDM_STATUS_INVALID_PARAMETER;
    if ((message_size >= 12U) &&
        (bytes[2] == PCI_DOE_DATA_OBJECT_TYPE_SPDM))
        LOG_DEBUG(NULL, "SPDM request code=%x length=%u", bytes[9],
                  (unsigned int)message_size);
    if (owner->transport_failed)
        return LIBSPDM_STATUS_SEND_FAIL;
    if (val_host_doe_send(owner->endpoint, message, message_size) != VAL_SUCCESS) {
        owner->transport_failed = VAL_TRUE;
        return LIBSPDM_STATUS_SEND_FAIL;
    }
    return LIBSPDM_STATUS_SUCCESS;
}

/**
 * @brief Receive one libspdm-encoded DOE response from the selected endpoint.
 * @param spdm_context Active libspdm context.
 * @param message_size Returned message length.
 * @param message Returned receive-buffer address.
 * @param timeout Requested timeout, bounded by the DOE mailbox polling loop.
 * @return libspdm transport status.
 */
static libspdm_return_t val_host_spdm_receive(void *spdm_context,
                                               size_t *message_size,
                                               void **message,
                                               uint64_t timeout)
{
    val_host_spdm_context_t *owner;

    owner = val_host_spdm_owner(spdm_context);
    (void)timeout;
    if (!owner || !owner->endpoint || !message_size || !message)
        return LIBSPDM_STATUS_INVALID_PARAMETER;
    if (owner->transport_failed)
        return LIBSPDM_STATUS_RECEIVE_FAIL;
    if (val_host_doe_receive(owner->endpoint, owner->receiver_buffer,
                             VAL_HOST_SPDM_BUFFER_BYTES,
                             message_size) != VAL_SUCCESS) {
        owner->transport_failed = VAL_TRUE;
        return LIBSPDM_STATUS_RECEIVE_FAIL;
    }
    *message = owner->receiver_buffer;
    if ((*message_size >= 12U) &&
        (owner->receiver_buffer[2] == PCI_DOE_DATA_OBJECT_TYPE_SPDM)) {
        LOG_DEBUG(NULL, "SPDM response code=%x length=%u",
                  owner->receiver_buffer[9], (unsigned int)*message_size);
        if (owner->receiver_buffer[9] == SPDM_ERROR)
            LOG_ERROR(NULL, "SPDM responder error=%x data=%x",
                      owner->receiver_buffer[10], owner->receiver_buffer[11]);
    }
    return LIBSPDM_STATUS_SUCCESS;
}

/**
 * @brief Acquire the libspdm requester transmit buffer.
 * @param spdm_context Active libspdm context.
 * @param message_buffer Returned base of the full transport buffer.
 * @return libspdm buffer-acquisition status.
 */
static libspdm_return_t val_host_spdm_acquire_sender(void *spdm_context,
                                                      void **message_buffer)
{
    val_host_spdm_context_t *owner;

    owner = val_host_spdm_owner(spdm_context);
    if (!owner || !message_buffer || (owner->sender_in_use != VAL_FALSE))
        return LIBSPDM_STATUS_ACQUIRE_FAIL;
    owner->sender_in_use = VAL_TRUE;
    *message_buffer = owner->sender_buffer;
    return LIBSPDM_STATUS_SUCCESS;
}

/**
 * @brief Release the libspdm requester transmit buffer.
 * @param spdm_context Active libspdm context.
 * @param message_buffer Previously acquired buffer address.
 */
static void val_host_spdm_release_sender(void *spdm_context,
                                         const void *message_buffer)
{
    val_host_spdm_context_t *owner;

    owner = val_host_spdm_owner(spdm_context);
    if (owner && (message_buffer == owner->sender_buffer))
        owner->sender_in_use = VAL_FALSE;
}

/**
 * @brief Acquire the libspdm requester receive buffer.
 * @param spdm_context Active libspdm context.
 * @param message_buffer Returned transport receive-buffer address.
 * @return libspdm buffer-acquisition status.
 */
static libspdm_return_t val_host_spdm_acquire_receiver(void *spdm_context,
                                                        void **message_buffer)
{
    val_host_spdm_context_t *owner;

    owner = val_host_spdm_owner(spdm_context);
    if (!owner || !message_buffer || (owner->receiver_in_use != VAL_FALSE))
        return LIBSPDM_STATUS_ACQUIRE_FAIL;
    owner->receiver_in_use = VAL_TRUE;
    *message_buffer = owner->receiver_buffer;
    return LIBSPDM_STATUS_SUCCESS;
}

/**
 * @brief Release the libspdm requester receive buffer.
 * @param spdm_context Active libspdm context.
 * @param message_buffer Previously acquired buffer address.
 */
static void val_host_spdm_release_receiver(void *spdm_context,
                                           const void *message_buffer)
{
    val_host_spdm_context_t *owner;

    owner = val_host_spdm_owner(spdm_context);
    if (owner && (message_buffer == owner->receiver_buffer))
        owner->receiver_in_use = VAL_FALSE;
}

/**
 * @brief Set a local libspdm requester parameter and report configuration errors.
 * @param owner Endpoint SPDM requester context.
 * @param data_type libspdm context data selector.
 * @param data Context value.
 * @param data_size Context value length.
 * @return VAL_SUCCESS when accepted, otherwise VAL_ERROR.
 */
static int val_host_spdm_set(val_host_spdm_context_t *owner,
                              libspdm_data_type_t data_type,
                              void *data, size_t data_size)
{
    libspdm_data_parameter_t parameter = {0};
    libspdm_return_t status;

    parameter.location = LIBSPDM_DATA_LOCATION_LOCAL;
    status = libspdm_set_data(owner->spdm_context, data_type,
                              &parameter, data, data_size);
    if (!LIBSPDM_STATUS_IS_SUCCESS(status)) {
        LOG_ERROR(NULL, "SPDM context setting %u failed status=%x",
                  (unsigned int)data_type, (unsigned int)status);
        return VAL_ERROR;
    }
    return VAL_SUCCESS;
}

/**
 * @brief Configure protocol, algorithm, and trust-anchor policy for libspdm.
 * @param owner Endpoint SPDM requester context.
 * @return VAL_SUCCESS when all mandatory values are accepted.
 */
static int val_host_spdm_configure(val_host_spdm_context_t *owner)
{
    spdm_version_number_t version =
        (spdm_version_number_t)SPDM_MESSAGE_VERSION_12 <<
        SPDM_VERSION_NUMBER_SHIFT_BIT;
    spdm_version_number_t secured_version =
        (spdm_version_number_t)SECURED_SPDM_VERSION_11 <<
        SPDM_VERSION_NUMBER_SHIFT_BIT;
    uint32_t caps = SPDM_GET_CAPABILITIES_REQUEST_FLAGS_CERT_CAP |
                    SPDM_GET_CAPABILITIES_REQUEST_FLAGS_ENCRYPT_CAP |
                    SPDM_GET_CAPABILITIES_REQUEST_FLAGS_MAC_CAP |
                    SPDM_GET_CAPABILITIES_REQUEST_FLAGS_KEY_EX_CAP |
                    SPDM_GET_CAPABILITIES_REQUEST_FLAGS_HANDSHAKE_IN_THE_CLEAR_CAP;
    uint32_t asym = SPDM_ALGORITHMS_BASE_ASYM_ALGO_TPM_ALG_RSASSA_3072;
    uint16_t req_asym =
        SPDM_ALGORITHMS_BASE_ASYM_ALGO_TPM_ALG_RSASSA_3072;
    uint32_t hash = SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_256;
    uint16_t dhe = SPDM_ALGORITHMS_DHE_NAMED_GROUP_SECP_256_R1;
    uint16_t aead = SPDM_ALGORITHMS_AEAD_CIPHER_SUITE_AES_256_GCM;
    uint16_t schedule = SPDM_ALGORITHMS_KEY_SCHEDULE_HMAC_HASH;
    uint8_t ct_exponent = 0U;
    uint8_t measurement = SPDM_MEASUREMENT_SPECIFICATION_DMTF;
    uint8_t opaque = SPDM_ALGORITHMS_OPAQUE_DATA_FORMAT_1;
    val_host_spdm_context_t *owner_pointer = owner;

    if ((val_host_spdm_set(owner, LIBSPDM_DATA_APP_CONTEXT_DATA,
                            &owner_pointer, sizeof(owner_pointer)) != VAL_SUCCESS) ||
        (val_host_spdm_set(owner, LIBSPDM_DATA_SPDM_VERSION,
                            &version, sizeof(version)) != VAL_SUCCESS) ||
        (val_host_spdm_set(owner, LIBSPDM_DATA_SECURED_MESSAGE_VERSION,
                            &secured_version, sizeof(secured_version)) != VAL_SUCCESS) ||
        (val_host_spdm_set(owner, LIBSPDM_DATA_CAPABILITY_CT_EXPONENT,
                            &ct_exponent, sizeof(ct_exponent)) != VAL_SUCCESS) ||
        (val_host_spdm_set(owner, LIBSPDM_DATA_CAPABILITY_FLAGS,
                            &caps, sizeof(caps)) != VAL_SUCCESS) ||
        (val_host_spdm_set(owner, LIBSPDM_DATA_MEASUREMENT_SPEC,
                            &measurement, sizeof(measurement)) != VAL_SUCCESS) ||
        (val_host_spdm_set(owner, LIBSPDM_DATA_OTHER_PARAMS_SUPPORT,
                            &opaque, sizeof(opaque)) != VAL_SUCCESS) ||
        (val_host_spdm_set(owner, LIBSPDM_DATA_BASE_ASYM_ALGO,
                            &asym, sizeof(asym)) != VAL_SUCCESS) ||
        (val_host_spdm_set(owner, LIBSPDM_DATA_BASE_HASH_ALGO,
                            &hash, sizeof(hash)) != VAL_SUCCESS) ||
        (val_host_spdm_set(owner, LIBSPDM_DATA_DHE_NAME_GROUP,
                            &dhe, sizeof(dhe)) != VAL_SUCCESS) ||
        (val_host_spdm_set(owner, LIBSPDM_DATA_AEAD_CIPHER_SUITE,
                            &aead, sizeof(aead)) != VAL_SUCCESS) ||
        (val_host_spdm_set(owner, LIBSPDM_DATA_KEY_SCHEDULE,
                            &schedule, sizeof(schedule)) != VAL_SUCCESS) ||
        (val_host_spdm_set(owner, LIBSPDM_DATA_REQ_BASE_ASYM_ALG,
                            &req_asym, sizeof(req_asym)) != VAL_SUCCESS) ||
        (val_host_spdm_set(owner, LIBSPDM_DATA_PEER_PUBLIC_ROOT_CERT,
                            (void *)g_val_host_spdm_root_cert,
                            sizeof(g_val_host_spdm_root_cert)) != VAL_SUCCESS))
        return VAL_ERROR;

    return VAL_SUCCESS;
}

/**
 * @brief Initialize a bounded libspdm requester for one CXL DOE endpoint.
 * @param owner Caller-owned context to initialize.
 * @param endpoint Discovered CXL Type-2 endpoint.
 * @return VAL_SUCCESS after version/capability/algorithm negotiation.
 */
static int val_host_spdm_open(val_host_spdm_context_t *owner,
                               const val_host_doe_endpoint_t *endpoint)
{
    libspdm_return_t status;

    if (!owner || !endpoint || !val_host_spdm_has_rndr()) {
        LOG_ERROR(NULL, "SPDM requires FEAT_RNG on the requesting PE");
        return VAL_ERROR;
    }

    memset(owner, 0, sizeof(*owner));
    owner->endpoint = endpoint;
    owner->spdm_context = malloc(libspdm_get_context_size());
    owner->sender_buffer = malloc(VAL_HOST_SPDM_BUFFER_BYTES);
    owner->receiver_buffer = malloc(VAL_HOST_SPDM_BUFFER_BYTES);
    if (!owner->spdm_context || !owner->sender_buffer ||
        !owner->receiver_buffer)
        return VAL_ERROR;

    status = libspdm_init_context(owner->spdm_context);
    if (!LIBSPDM_STATUS_IS_SUCCESS(status)) {
        LOG_ERROR(NULL, "SPDM context init failed status=%x", (unsigned int)status);
        return VAL_ERROR;
    }
    owner->initialized = VAL_TRUE;
    libspdm_register_device_io_func(owner->spdm_context,
                                    val_host_spdm_send,
                                    val_host_spdm_receive);
    libspdm_register_transport_layer_func(
        owner->spdm_context, VAL_HOST_SPDM_MAX_MESSAGE,
        LIBSPDM_PCI_DOE_TRANSPORT_HEADER_SIZE,
        LIBSPDM_PCI_DOE_TRANSPORT_TAIL_SIZE,
        libspdm_transport_pci_doe_encode_message,
        libspdm_transport_pci_doe_decode_message);
    if (val_host_spdm_configure(owner) != VAL_SUCCESS)
        return VAL_ERROR;

    owner->scratch_size = libspdm_get_sizeof_required_scratch_buffer(
        owner->spdm_context);
    owner->scratch_buffer = malloc(owner->scratch_size);
    if (!owner->scratch_buffer)
        return VAL_ERROR;
    libspdm_register_device_buffer_func(
        owner->spdm_context, VAL_HOST_SPDM_BUFFER_BYTES,
        VAL_HOST_SPDM_BUFFER_BYTES, val_host_spdm_acquire_sender,
        val_host_spdm_release_sender, val_host_spdm_acquire_receiver,
        val_host_spdm_release_receiver);
    libspdm_set_scratch_buffer(owner->spdm_context, owner->scratch_buffer,
                               owner->scratch_size);

    status = libspdm_init_connection(owner->spdm_context, false);
    if (!LIBSPDM_STATUS_IS_SUCCESS(status)) {
        LOG_ERROR(NULL, "SPDM negotiation failed status=%x", (unsigned int)status);
        return VAL_ERROR;
    }
    LOG_TEST(NULL, "CXL SPDM requester negotiated protocol and algorithms");
    return VAL_SUCCESS;
}

/**
 * @brief Validate the endpoint certificate and establish a secured SPDM session.
 * @param owner Initialized requester context.
 * @param session_id Returned secured session identifier.
 * @return VAL_SUCCESS only after certificate verification and key exchange.
 */
static int val_host_spdm_start_session(val_host_spdm_context_t *owner,
                                        uint32_t *session_id)
{
    libspdm_return_t status;
    uint8_t slot_mask = 0U;
    uint8_t slot_id = 0U;
    uint8_t digests[LIBSPDM_MAX_HASH_SIZE * SPDM_MAX_SLOT_COUNT] = {0};
    uint8_t *cert_chain;
    size_t cert_size = LIBSPDM_MAX_CERT_CHAIN_SIZE;
    int result = VAL_ERROR;

    if (!owner || !session_id || owner->transport_failed || owner->session_uncertain)
        return VAL_ERROR;
    cert_chain = malloc(cert_size);
    if (!cert_chain)
        return VAL_ERROR;

    status = libspdm_get_digest(owner->spdm_context, NULL, &slot_mask, digests);
    if (!LIBSPDM_STATUS_IS_SUCCESS(status) || (slot_mask == 0U)) {
        LOG_ERROR(NULL, "SPDM GET_DIGESTS failed status=%x", (unsigned int)status);
        goto out;
    }
    while ((slot_id < SPDM_MAX_SLOT_COUNT) &&
           ((slot_mask & (1U << slot_id)) == 0U))
        slot_id++;
    if (slot_id == SPDM_MAX_SLOT_COUNT)
        goto out;

    status = libspdm_get_certificate(owner->spdm_context, NULL,
                                     slot_id, &cert_size, cert_chain);
    if (!LIBSPDM_STATUS_IS_SUCCESS(status)) {
        LOG_ERROR(NULL, "SPDM certificate verification failed status=%x",
                  (unsigned int)status);
        goto out;
    }
    owner->session_uncertain = VAL_TRUE;
    status = libspdm_start_session(owner->spdm_context, false, NULL, 0U,
                                   SPDM_REQUEST_NO_MEASUREMENT_SUMMARY_HASH,
                                   slot_id, 0U, session_id, NULL, NULL);
    if (!LIBSPDM_STATUS_IS_SUCCESS(status)) {
        LOG_ERROR(NULL, "SPDM key exchange failed status=%x",
                  (unsigned int)status);
        goto out;
    }

    owner->session_uncertain = VAL_FALSE;
    LOG_TEST(NULL, "CXL SPDM responder certificate verified; secured session active");
    result = VAL_SUCCESS;
out:
    free(cert_chain);
    return result;
}

/**
 * @brief Verify that CXL IDE KM QUERY identifies this endpoint and supports STOP.
 * @param owner Authenticated SPDM requester.
 * @param session_id Secured session identifier.
 * @return VAL_SUCCESS for an aligned endpoint and capability response.
 */
static int val_host_spdm_query_ide(val_host_spdm_context_t *owner,
                                    uint32_t session_id)
{
    uint32_t ide_registers[CXL_IDE_KM_IDE_CAP_REG_BLOCK_MAX_COUNT] = {0};
    uint32_t ide_register_count = CXL_IDE_KM_IDE_CAP_REG_BLOCK_MAX_COUNT;
    uint8_t device_function = 0U;
    uint8_t bus = 0U;
    uint8_t segment = 0U;
    uint8_t max_port_index = 0U;
    uint8_t capabilities = 0U;
    libspdm_return_t status;

    status = cxl_ide_km_query(NULL, owner->spdm_context, &session_id,
                              VAL_HOST_SPDM_PORT_INDEX, &device_function,
                              &bus, &segment, &max_port_index, &capabilities,
                              ide_registers, &ide_register_count);
    if (!LIBSPDM_STATUS_IS_SUCCESS(status) ||
        (bus != val_pcie_bdf_bus(owner->endpoint->endpoint_bdf)) ||
        (device_function !=
         ((val_pcie_bdf_device(owner->endpoint->endpoint_bdf) << 3U) |
          val_pcie_bdf_function(owner->endpoint->endpoint_bdf))) ||
        (segment != val_pcie_bdf_segment(owner->endpoint->endpoint_bdf)) ||
        ((capabilities & CXL_IDE_KM_QUERY_RESP_CAP_VERSION_MASK) !=
         CXL_IDE_KM_QUERY_RESP_CAP_VERSION_1) ||
        ((capabilities & CXL_IDE_KM_QUERY_RESP_K_SET_STOP_CAP_MASK) == 0U) ||
        (ide_register_count == 0U)) {
        LOG_ERROR(NULL, "CXL IDE QUERY failed status=%x caps=%x bus=%u",
                  (unsigned int)status, capabilities, bus);
        return VAL_ERROR;
    }

    LOG_TEST(NULL, "CXL IDE KM QUERY validated endpoint and capabilities");
    return VAL_SUCCESS;
}

/**
 * @brief Send KEY_PROG and validate only architecturally echoed ACK fields.
 * @param owner Authenticated SPDM requester.
 * @param session_id Secured session identifier.
 * @param direction CXL IDE RX or TX direction bit.
 * @param key_buffer AES-256 key and default-IV payload.
 * @return VAL_SUCCESS only for a successful, matching KP_ACK.
 */
static int val_host_spdm_key_prog(val_host_spdm_context_t *owner,
                                   uint32_t session_id, uint8_t direction,
                                   const cxl_ide_km_aes_256_gcm_key_buffer_t *key_buffer)
{
    val_host_cxl_key_prog_request_t request = {0};
    cxl_ide_km_kp_ack_t response = {0};
    size_t response_size = sizeof(response);
    libspdm_return_t status;

    request.header.header.object_id = CXL_IDE_KM_OBJECT_ID_KEY_PROG;
    request.header.stream_id = VAL_HOST_SPDM_STREAM_ID;
    request.header.key_sub_stream = CXL_IDE_KM_KEY_SUB_STREAM_CXL |
                                    CXL_IDE_KM_KEY_IV_DEFAULT | direction;
    request.header.port_index = VAL_HOST_SPDM_PORT_INDEX;
    memcpy(&request.key_buffer, key_buffer, sizeof(request.key_buffer));
    status = cxl_ide_km_send_receive_data(owner->spdm_context, &session_id,
                                          &request, sizeof(request),
                                          &response, &response_size);
    memset(&request, 0, sizeof(request));
    if (!LIBSPDM_STATUS_IS_SUCCESS(status) ||
        (response_size != sizeof(response)) ||
        (response.header.object_id != CXL_IDE_KM_OBJECT_ID_KP_ACK) ||
        (response.stream_id != VAL_HOST_SPDM_STREAM_ID) ||
        (response.port_index != VAL_HOST_SPDM_PORT_INDEX) ||
        ((response.key_sub_stream & VAL_HOST_SPDM_ACK_RESERVED_BITS) != 0U) ||
        (response.key_sub_stream !=
         (CXL_IDE_KM_KEY_SUB_STREAM_CXL | direction)) ||
        (response.status != CXL_IDE_KM_KP_ACK_STATUS_SUCCESS)) {
        LOG_ERROR(NULL, "CXL IDE KEY_PROG direction=%u failed status=%x ack=%u",
                  direction, (unsigned int)status, response.status);
        return VAL_ERROR;
    }
    LOG_TEST(NULL, "CXL IDE endpoint KEY_PROG direction=%u accepted", direction);
    return VAL_SUCCESS;
}

/**
 * @brief Send K_SET_GO and validate the non-reserved ACK fields.
 * @param owner Authenticated SPDM requester.
 * @param session_id Secured session identifier.
 * @param direction CXL IDE RX or TX direction bit.
 * @return VAL_SUCCESS only for a matching GO acknowledgment.
 */
static int val_host_spdm_key_go(val_host_spdm_context_t *owner,
                                 uint32_t session_id, uint8_t direction)
{
    cxl_ide_km_k_set_go_t request = {0};
    cxl_ide_km_k_gostop_ack_t response = {0};
    size_t response_size = sizeof(response);
    libspdm_return_t status;

    request.header.object_id = CXL_IDE_KM_OBJECT_ID_K_SET_GO;
    request.stream_id = VAL_HOST_SPDM_STREAM_ID;
    request.key_sub_stream = CXL_IDE_KM_KEY_SUB_STREAM_CXL |
                             CXL_IDE_KM_KEY_MODE_CONTAINMENT | direction;
    request.port_index = VAL_HOST_SPDM_PORT_INDEX;
    status = cxl_ide_km_send_receive_data(owner->spdm_context, &session_id,
                                          &request, sizeof(request),
                                          &response, &response_size);
    if (!LIBSPDM_STATUS_IS_SUCCESS(status) ||
        (response_size != sizeof(response)) ||
        (response.header.object_id != CXL_IDE_KM_OBJECT_ID_K_SET_GOSTOP_ACK) ||
        (response.stream_id != VAL_HOST_SPDM_STREAM_ID) ||
        (response.port_index != VAL_HOST_SPDM_PORT_INDEX) ||
        ((response.key_sub_stream & VAL_HOST_SPDM_ACK_RESERVED_BITS) != 0U) ||
        (response.key_sub_stream !=
         (CXL_IDE_KM_KEY_SUB_STREAM_CXL | direction))) {
        LOG_ERROR(NULL, "CXL IDE K_SET_GO direction=%u failed status=%x",
                  direction, (unsigned int)status);
        return VAL_ERROR;
    }
    LOG_TEST(NULL, "CXL IDE endpoint K_SET_GO direction=%u accepted", direction);
    return VAL_SUCCESS;
}

/**
 * @brief Send K_SET_STOP for a direction that was successfully programmed.
 * @param owner Authenticated SPDM requester.
 * @param session_id Secured session identifier.
 * @param direction CXL IDE RX or TX direction bit.
 * @return VAL_SUCCESS only for a matching STOP acknowledgment.
 */
static int val_host_spdm_key_stop(val_host_spdm_context_t *owner,
                                   uint32_t session_id, uint8_t direction)
{
    libspdm_return_t status;

    status = cxl_ide_km_key_set_stop(NULL, owner->spdm_context, &session_id,
                                     VAL_HOST_SPDM_STREAM_ID,
                                     CXL_IDE_KM_KEY_SUB_STREAM_CXL | direction,
                                     VAL_HOST_SPDM_PORT_INDEX);
    if (!LIBSPDM_STATUS_IS_SUCCESS(status)) {
        LOG_ERROR(NULL, "CXL IDE K_SET_STOP direction=%u failed status=%x",
                  direction, (unsigned int)status);
        return VAL_ERROR;
    }
    LOG_TEST(NULL, "CXL IDE endpoint K_SET_STOP direction=%u accepted", direction);
    return VAL_SUCCESS;
}

/**
 * @brief Erase and free all allocations owned by one SPDM requester.
 * @param owner Initialized or partially initialized requester context.
 */
static void val_host_spdm_close(val_host_spdm_context_t *owner)
{
    if (!owner)
        return;
    if (owner->initialized != VAL_FALSE)
        libspdm_deinit_context(owner->spdm_context);
    free(owner->scratch_buffer);
    free(owner->sender_buffer);
    free(owner->receiver_buffer);
    free(owner->spdm_context);
    memset(owner, 0, sizeof(*owner));
}

/**
 * @brief Route one link operation to the world advertising IDE management.
 * @param world Selected caller world, Normal or Realm.
 * @param operation ACS link command.
 * @param keyset_id Test-owned CXL keyset identifier.
 * @param data0 Command data word zero.
 * @param data1 Command data word one.
 * @return VAL_SUCCESS only for an acknowledged, successful operation.
 */
static int val_host_spdm_root_command(firme_acs_world_t world,
                                       uint64_t operation, uint64_t keyset_id,
                                       uint64_t data0, uint64_t data1)
{
    firme_acs_call_res_t result = {0};
    int transport = VAL_SUCCESS;

    if (world == FIRME_ACS_WORLD_NSEL2)
        val_firme_ide_link_command(operation, keyset_id, data0, data1, &result);
    else
        transport = val_firme_world_call(world, FIRME_ACS_CMD_IDE_LINK,
                                          operation, keyset_id, data0, data1,
                                          &result);

    if (transport != VAL_SUCCESS || result.x0 != ACS_STATUS_PASS) {
        LOG_ERROR(NULL, "CXL FIRME operation=%llu keyset=%llx transport=%d status=%llx x0=%llx",
                  (unsigned long long)operation, (unsigned long long)keyset_id,
                  transport, (unsigned long long)result.x0,
                  (unsigned long long)result.x1);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/**
 * @brief Stop one Root Port direction, draining any previously accepted operation.
 * @param transaction Persistent owner of both ends of the link.
 * @param direction Root Port direction to quiesce.
 * @return VAL_ERROR leaves ownership intact for later drain or quarantine.
 */
static int val_host_spdm_stop_root(val_host_spdm_transaction_t *transaction,
                                   uint32_t direction)
{
    val_host_spdm_link_direction_t *link = &transaction->link[direction];

    if (!link->root_open || link->root_stopped)
        return VAL_SUCCESS;
    if (val_host_spdm_root_command(transaction->world, VAL_IDE_LINK_QUIESCE,
                                    link->keyset_id, 0ULL, 0ULL) != VAL_SUCCESS)
        return VAL_ERROR;
    link->root_stopped = VAL_TRUE;
    return VAL_SUCCESS;
}

/**
 * @brief Stop one endpoint direction without discarding its secured session.
 * @param transaction Persistent owner of both ends of the link.
 * @param direction Corresponding Root Port direction.
 * @return VAL_SUCCESS only when STOP is acknowledged or was already acknowledged.
 */
static int val_host_spdm_stop_endpoint(val_host_spdm_transaction_t *transaction,
                                       uint32_t direction)
{
    val_host_spdm_link_direction_t *link = &transaction->link[direction];

    if (!link->endpoint_programmed || link->endpoint_stopped)
        return VAL_SUCCESS;
    if (!transaction->session_started || transaction->owner.transport_failed ||
        val_host_spdm_key_stop(&transaction->owner, transaction->session_id,
                                link->endpoint_direction) != VAL_SUCCESS)
        return VAL_ERROR;
    link->endpoint_stopped = VAL_TRUE;
    return VAL_SUCCESS;
}

/**
 * @brief Restore an owned fixture without releasing live or uncertain resources.
 * @param transaction Persistent fixture, including partial discovery and setup.
 * @return VAL_ERROR retains session, buffers, keys and configuration for recovery.
 */
static int val_host_spdm_cleanup(val_host_spdm_transaction_t *transaction)
{
    libspdm_return_t status;
    uint32_t direction;
    int root_tx;
    int endpoint_tx;
    int result = VAL_SUCCESS;

    if (!transaction->open)
        return VAL_SUCCESS;
    if (transaction->quarantined || transaction->owner.transport_failed ||
        transaction->owner.session_uncertain)
        return VAL_ERROR;
    if (transaction->abi_owned && !transaction->abi_root_stopped)
        return VAL_ERROR;

    root_tx = val_host_spdm_stop_root(transaction, 1U);
    endpoint_tx = val_host_spdm_stop_endpoint(transaction, 0U);
    if (endpoint_tx == VAL_SUCCESS &&
        val_host_spdm_stop_root(transaction, 0U) != VAL_SUCCESS)
        result = VAL_ERROR;
    if (root_tx == VAL_SUCCESS &&
        val_host_spdm_stop_endpoint(transaction, 1U) != VAL_SUCCESS)
        result = VAL_ERROR;
    if (root_tx != VAL_SUCCESS || endpoint_tx != VAL_SUCCESS || result != VAL_SUCCESS)
        return VAL_ERROR;

    if (transaction->traffic_owned) {
        if (pal_cxl_traffic_release() != VAL_SUCCESS) {
            transaction->quarantined = VAL_TRUE;
            return VAL_ERROR;
        }
        transaction->traffic_owned = VAL_FALSE;
    }
    for (direction = 0U; direction < VAL_HOST_SPDM_DIRECTIONS; direction++) {
        if (!transaction->link[direction].root_open)
            continue;
        if (val_host_spdm_root_command(transaction->world, VAL_IDE_LINK_RELEASE,
                transaction->link[direction].keyset_id, 0ULL, 0ULL) != VAL_SUCCESS)
            return VAL_ERROR;
        transaction->link[direction].root_open = VAL_FALSE;
    }
    if (transaction->session_started) {
        status = libspdm_stop_session(transaction->owner.spdm_context,
                                       transaction->session_id, 0U);
        if (!LIBSPDM_STATUS_IS_SUCCESS(status)) {
            transaction->quarantined = VAL_TRUE;
            return VAL_ERROR;
        }
        transaction->session_started = VAL_FALSE;
    }
    if (val_host_doe_release_endpoint(&transaction->endpoint) != VAL_SUCCESS)
        return VAL_ERROR;
    val_host_spdm_close(&transaction->owner);
    memset(transaction, 0, sizeof(*transaction));
    return VAL_SUCCESS;
}

/**
 * @brief Change a paired direction without enabling a transmitter before its receiver.
 * @param owner Authenticated endpoint requester.
 * @param session_id Still-active SPDM session.
 * @param world FIRME caller world.
 * @param link Paired direction and keyset.
 * @param root_direction Zero for Root Port RX, one for TX.
 * @param start Nonzero for GO, zero for STOP.
 * @return VAL_SUCCESS after both ends acknowledge the requested operation.
 */
static int val_host_spdm_pair_state(val_host_spdm_context_t *owner,
                                    uint32_t session_id, firme_acs_world_t world,
                                    val_host_spdm_link_direction_t *link,
                                    uint32_t root_direction, uint32_t start)
{
    int first;
    int second;
    uint32_t root_first = start ? root_direction == 0U : root_direction != 0U;
    uint64_t command = start ? VAL_IDE_LINK_GO : VAL_IDE_LINK_STOP;

    if (start) {
        link->root_stopped = VAL_FALSE;
        link->endpoint_stopped = VAL_FALSE;
    }
    if (root_first) {
        first = val_host_spdm_root_command(world, command, link->keyset_id, 0ULL, 0ULL);
        if (first != VAL_SUCCESS)
            return VAL_ERROR;
        if (!start)
            link->root_stopped = VAL_TRUE;
        second = start ? val_host_spdm_key_go(owner, session_id, link->endpoint_direction) :
                         val_host_spdm_key_stop(owner, session_id, link->endpoint_direction);
        if (!start && second == VAL_SUCCESS)
            link->endpoint_stopped = VAL_TRUE;
    } else {
        first = start ? val_host_spdm_key_go(owner, session_id, link->endpoint_direction) :
                        val_host_spdm_key_stop(owner, session_id, link->endpoint_direction);
        if (first != VAL_SUCCESS)
            return VAL_ERROR;
        if (!start)
            link->endpoint_stopped = VAL_TRUE;
        second = val_host_spdm_root_command(world, command, link->keyset_id, 0ULL, 0ULL);
        if (!start && second == VAL_SUCCESS)
            link->root_stopped = VAL_TRUE;
    }
    return (first == VAL_SUCCESS && second == VAL_SUCCESS) ? VAL_SUCCESS : VAL_ERROR;
}

/**
 * @brief Validate paired Root Port and endpoint programming in one secured session.
 * @param endpoint Discovered Type-2 endpoint with a working DOE capability.
 * @param operation Operation requiring fresh programming or GO/STOP evidence.
 * @param world Caller instance already exercised by the owning rule.
 * @param out Observation outcome and the first failing stage/direction.
 * @return VAL_SUCCESS only after both directions complete and cleanup succeeds.
 */
static int val_host_spdm_validate_cxl_key_programming(
    const val_host_doe_endpoint_t *endpoint, uint64_t operation,
    firme_acs_world_t world, firme_rule_res_t *out)
{
    val_host_spdm_transaction_t *transaction = &g_val_host_spdm_transaction;
    val_host_spdm_context_t *owner = &transaction->owner;
    val_host_spdm_link_direction_t *link = transaction->link;
    val_firme_ide_keyset_fields_t fields = {0};
    uint32_t direction;
    int result = VAL_ERROR;

    if (!endpoint || !out)
        return VAL_ERROR;
    out->fail_step = VAL_HOST_IDE_STEP_DISCOVERY;
    if (val_host_spdm_root_command(world, VAL_IDE_LINK_DISCOVER,
                                    0ULL, 0ULL, 0ULL) != VAL_SUCCESS)
        goto cleanup;
    out->fail_step = VAL_HOST_IDE_STEP_SESSION;
    if (val_host_spdm_open(owner, endpoint) != VAL_SUCCESS)
        goto cleanup;
    if (val_host_spdm_start_session(owner, &transaction->session_id) != VAL_SUCCESS)
        goto cleanup;
    transaction->session_started = VAL_TRUE;
    if (val_host_spdm_query_ide(owner, transaction->session_id) != VAL_SUCCESS)
        goto cleanup;

    fields.segment = val_pcie_bdf_segment(endpoint->root_bdf);
    fields.root_port_id = val_pcie_requester_id_from_bdf(endpoint->root_bdf);
    fields.substream_id = VAL_FIRME_IDE_CXL_SUBSTREAM_ID;
    for (direction = 0U; direction < VAL_HOST_SPDM_DIRECTIONS; direction++) {
        out->fail_step = VAL_HOST_IDE_STEP_PROG;
        out->last_x0 = direction;
        fields.direction = direction;
        link[direction].keyset_id = val_firme_ide_keyset_id(&fields);
        link[direction].endpoint_direction = (direction == 0U) ?
            CXL_IDE_KM_KEY_DIRECTION_TX : CXL_IDE_KM_KEY_DIRECTION_RX;
        if (!libspdm_get_random_number(sizeof(link[direction].key),
                                        (uint8_t *)link[direction].key))
            goto cleanup;
        memcpy(link[direction].endpoint_key.key, link[direction].key,
               sizeof(link[direction].key));

        link[direction].root_open = VAL_TRUE;
        if (val_host_spdm_root_command(world, VAL_IDE_LINK_BEGIN,
                link[direction].keyset_id, transaction->ecam_address, 0ULL) != VAL_SUCCESS)
            goto cleanup;
        if (val_host_spdm_root_command(world, VAL_IDE_LINK_KEY_LOW,
                link[direction].keyset_id, link[direction].key[0],
                link[direction].key[1]) != VAL_SUCCESS ||
            val_host_spdm_root_command(world, VAL_IDE_LINK_KEY_HIGH,
                link[direction].keyset_id, link[direction].key[2],
                link[direction].key[3]) != VAL_SUCCESS ||
            val_host_spdm_root_command(world, VAL_IDE_LINK_PROG,
                link[direction].keyset_id, 0ULL, 0ULL) != VAL_SUCCESS)
            goto cleanup;
        link[direction].root_programmed = VAL_TRUE;
        link[direction].endpoint_programmed = VAL_TRUE;
        if (val_host_spdm_key_prog(owner, transaction->session_id,
                link[direction].endpoint_direction,
                &link[direction].endpoint_key) != VAL_SUCCESS)
            goto cleanup;

        if (val_host_spdm_pair_state(owner, transaction->session_id, world, &link[direction],
                                      direction, VAL_TRUE) != VAL_SUCCESS)
            goto cleanup;
        LOG_TEST(NULL, "CXL paired link Root Port direction=%u programmed through FIRME world=%u",
                 direction, (unsigned int)world);
    }

    out->fail_step = VAL_HOST_IDE_STEP_TRAFFIC;
    transaction->traffic_owned = VAL_TRUE;
    if (pal_cxl_traffic_prepare(endpoint->root_bdf, endpoint->endpoint_bdf) != VAL_SUCCESS) {
        LOG_ERROR(NULL, "CXL.mem test-owned aperture setup failed");
        goto cleanup;
    }
    if (pal_cxl_traffic_check(VAL_FALSE) != VAL_SUCCESS)
        goto cleanup;

    /* Step 1: Attribute key-use evidence only to the PROG rule. */
    if (operation == VAL_IDE_LINK_PROG) {
        for (direction = 0U; direction < VAL_HOST_SPDM_DIRECTIONS; direction++) {
            out->fail_step = VAL_HOST_IDE_STEP_KEY_MISMATCH;
            out->last_x0 = direction;
            if (val_host_spdm_pair_state(owner, transaction->session_id, world, &link[direction],
                                          direction, VAL_FALSE) != VAL_SUCCESS)
                goto cleanup;
            link[direction].endpoint_key.key[7] ^= UINT32_C(1);
            if (val_host_spdm_key_prog(owner, transaction->session_id,
                                        link[direction].endpoint_direction,
                                        &link[direction].endpoint_key) != VAL_SUCCESS ||
                val_host_spdm_pair_state(owner, transaction->session_id, world, &link[direction],
                                          direction, VAL_TRUE) != VAL_SUCCESS ||
                pal_cxl_traffic_check(VAL_TRUE) != VAL_SUCCESS)
                goto cleanup;

            if (val_host_spdm_pair_state(owner, transaction->session_id, world, &link[direction],
                                          direction, VAL_FALSE) != VAL_SUCCESS)
                goto cleanup;
            link[direction].endpoint_key.key[7] ^= UINT32_C(1);
            if (val_host_spdm_key_prog(owner, transaction->session_id,
                                        link[direction].endpoint_direction,
                                        &link[direction].endpoint_key) != VAL_SUCCESS ||
                val_host_spdm_pair_state(owner, transaction->session_id, world, &link[direction],
                                          direction, VAL_TRUE) != VAL_SUCCESS ||
                pal_cxl_traffic_check(VAL_FALSE) != VAL_SUCCESS)
                goto cleanup;
        }
    }

    /* Step 2: Keep the endpoint active so it cannot mask a broken Root Port STOP. */
    if (operation == VAL_IDE_LINK_GO || operation == VAL_IDE_LINK_STOP) {
        for (direction = 0U; direction < VAL_HOST_SPDM_DIRECTIONS; direction++) {
            out->fail_step = VAL_HOST_IDE_STEP_ROOT_STOP;
            out->last_x0 = direction;
            if (val_host_spdm_root_command(world, VAL_IDE_LINK_STOP,
                    link[direction].keyset_id, 0ULL, 0ULL) != VAL_SUCCESS ||
                pal_cxl_traffic_check(VAL_TRUE) != VAL_SUCCESS)
                goto cleanup;

            /* Step 3: Change only Root Port GO and require traffic to recover. */
            out->fail_step = VAL_HOST_IDE_STEP_ROOT_GO;
            if (val_host_spdm_root_command(world, VAL_IDE_LINK_GO,
                    link[direction].keyset_id, 0ULL, 0ULL) != VAL_SUCCESS ||
                pal_cxl_traffic_check(VAL_FALSE) != VAL_SUCCESS)
                goto cleanup;
            LOG_TEST(NULL, "IDE operation=%u: Root Port direction=%u STOP/GO effects PASS; "
                     "endpoint stayed active", (unsigned int)operation, direction);
        }
    }

    result = VAL_SUCCESS;
    out->fail_step = VAL_HOST_IDE_STEP_CLEANUP;

cleanup:
    if (val_host_spdm_cleanup(transaction) != VAL_SUCCESS)
        result = VAL_ERROR;
    if (result == VAL_SUCCESS) {
        *out = (firme_rule_res_t){.status = ACS_STATUS_PASS};
        LOG_TEST(NULL, "IDE operation=%u: CXL functional observation and cleanup PASS",
                 (unsigned int)operation);
    } else {
        out->status = ACS_STATUS_FAIL;
        LOG_ERROR(NULL, "IDE operation=%u: CXL observation failed step=%llu direction=%llu",
                  (unsigned int)operation, (unsigned long long)out->fail_step,
                  (unsigned long long)out->last_x0);
    }
    return result;
}

/**
 * @brief Run functional IDE evidence inside the owning rule, not as a preflight.
 * @param operation PROG, GO or STOP operation from the link interface.
 * @param world The same FIRME instance used by the rule's ABI checks.
 * @param out Result including endpoint/session/setup/cleanup failures.
 * @return VAL_SUCCESS only after the selected observation completes successfully.
 */
int val_firme_ide_observe_operation(uint64_t operation,
                                   firme_acs_world_t world,
                                   firme_rule_res_t *out)
{
    val_host_spdm_transaction_t *transaction = &g_val_host_spdm_transaction;
    int status = VAL_ERROR;

    if (!out)
        return VAL_ERROR;
    *out = (firme_rule_res_t){.status = ACS_STATUS_FAIL,
                            .fail_step = VAL_HOST_IDE_STEP_DISCOVERY};
    if ((operation != VAL_IDE_LINK_PROG && operation != VAL_IDE_LINK_GO &&
         operation != VAL_IDE_LINK_STOP) ||
        (world != FIRME_ACS_WORLD_NSEL2 && world != FIRME_ACS_WORLD_REALM) ||
        val_pe_index() != val_host_mp_primary_index())
        return VAL_ERROR;

    if (val_host_spdm_cleanup(transaction) != VAL_SUCCESS) {
        out->fail_step = VAL_HOST_IDE_STEP_CLEANUP;
        LOG_ERROR(NULL, "IDE fixture remains owned after failed cleanup; refusing reuse");
        return VAL_ERROR;
    }
    transaction->open = VAL_TRUE;
    transaction->world = world;
    LOG_TEST(NULL, "IDE operation=%u: observe CXL Root Port effects through FIRME world=%u",
             (unsigned int)operation, (unsigned int)world);
    if (val_host_doe_prepare_endpoint(&transaction->endpoint) == VAL_SUCCESS &&
        val_pcie_ecam_base_for_bdf(transaction->endpoint.root_bdf,
                                    &transaction->ecam_address) == VAL_SUCCESS &&
        val_host_doe_discover_spdm(&transaction->endpoint) == VAL_SUCCESS)
        status = val_host_spdm_validate_cxl_key_programming(&transaction->endpoint, operation,
                                                           world, out);
    else
        LOG_ERROR(NULL, "IDE operation=%u: configured CXL endpoint discovery failed",
                  (unsigned int)operation);

    if (val_host_spdm_cleanup(transaction) != VAL_SUCCESS) {
        if (status == VAL_SUCCESS)
            out->fail_step = VAL_HOST_IDE_STEP_CLEANUP;
        out->status = ACS_STATUS_FAIL;
        status = VAL_ERROR;
    }
    return status;
}

/** @brief Bind a generated case to its exact retained host fixture. */
static uint32_t val_host_spdm_abi_matches(uint32_t world,
                                          const pal_firme_ide_abi_scenario_t *scenario)
{
    val_host_spdm_transaction_t *transaction = &g_val_host_spdm_transaction;

    return scenario && val_pe_index() == val_host_mp_primary_index() &&
           transaction->open && transaction->abi_owned &&
           world == FIRME_ACS_WORLD_NSEL2 &&
           transaction->world == FIRME_ACS_WORLD_NSEL2 &&
           scenario->token != 0ULL && scenario->token == transaction->abi_token &&
           scenario->ecam_address == transaction->ecam_address &&
           scenario->ide_type == VAL_FIRME_IDE_TYPE_CXL_CACHEMEM &&
           scenario->keyset_id == transaction->link[0].keyset_id ? VAL_TRUE : VAL_FALSE;
}

/**
 * @brief Reserve the existing paired observer for a real NS GO or STOP invocation.
 * @param world Caller world; Realm needs a host coordination channel not supplied here.
 * @param operation GO/STOP use same-invocation traffic, not an independent replay.
 * @param condition Only the valid case has qualified pre-state/effect support.
 * @param scenario Selected receive keyset, matching peer key, and ownership token.
 * @return UNAVAILABLE for missing scenarios; ERROR retains failed cleanup ownership.
 */
pal_firme_ide_abi_status_t pal_firme_ide_abi_prepare(
    uint32_t world, pal_firme_ide_abi_operation_t operation,
    pal_firme_ide_abi_condition_t condition, pal_firme_ide_abi_scenario_t *scenario)
{
    val_host_spdm_transaction_t *transaction = &g_val_host_spdm_transaction;
    val_host_spdm_link_direction_t *link = transaction->link;
    val_firme_ide_keyset_fields_t fields = {0};
    uint32_t direction;

    if (!scenario || val_pe_index() != val_host_mp_primary_index())
        return PAL_FIRME_IDE_ABI_ERROR;
    *scenario = (pal_firme_ide_abi_scenario_t){0};
    if (world != FIRME_ACS_WORLD_NSEL2 || condition != PAL_FIRME_IDE_ABI_VALID ||
        (operation != PAL_FIRME_IDE_ABI_GO && operation != PAL_FIRME_IDE_ABI_STOP))
        return PAL_FIRME_IDE_ABI_UNAVAILABLE;
    if (val_host_spdm_cleanup(transaction) != VAL_SUCCESS ||
        g_val_host_spdm_abi_token == UINT64_MAX)
        return PAL_FIRME_IDE_ABI_ERROR;

    transaction->open = VAL_TRUE;
    transaction->world = FIRME_ACS_WORLD_NSEL2;
    if (val_host_doe_prepare_endpoint(&transaction->endpoint) != VAL_SUCCESS ||
        val_pcie_ecam_base_for_bdf(transaction->endpoint.root_bdf,
                                    &transaction->ecam_address) != VAL_SUCCESS ||
        val_host_doe_discover_spdm(&transaction->endpoint) != VAL_SUCCESS ||
        val_host_spdm_open(&transaction->owner, &transaction->endpoint) != VAL_SUCCESS ||
        val_host_spdm_start_session(&transaction->owner, &transaction->session_id) != VAL_SUCCESS)
        goto failure;
    transaction->session_started = VAL_TRUE;
    if (val_host_spdm_query_ide(&transaction->owner, transaction->session_id) != VAL_SUCCESS)
        goto failure;

    fields.segment = val_pcie_bdf_segment(transaction->endpoint.root_bdf);
    fields.root_port_id = val_pcie_requester_id_from_bdf(transaction->endpoint.root_bdf);
    fields.substream_id = VAL_FIRME_IDE_CXL_SUBSTREAM_ID;
    for (direction = 0U; direction < VAL_HOST_SPDM_DIRECTIONS; direction++) {
        fields.direction = direction;
        link[direction].keyset_id = val_firme_ide_keyset_id(&fields);
        link[direction].endpoint_direction = direction == 0U ?
            CXL_IDE_KM_KEY_DIRECTION_TX : CXL_IDE_KM_KEY_DIRECTION_RX;
        if (!libspdm_get_random_number(sizeof(link[direction].key),
                                        (uint8_t *)link[direction].key))
            goto failure;
        memcpy(link[direction].endpoint_key.key, link[direction].key,
                 sizeof(link[direction].key));
        link[direction].endpoint_programmed = VAL_TRUE;
        if (val_host_spdm_key_prog(&transaction->owner, transaction->session_id,
                link[direction].endpoint_direction, &link[direction].endpoint_key) != VAL_SUCCESS)
            goto failure;
    }

    link[1].root_open = VAL_TRUE;
    if (val_host_spdm_root_command(transaction->world, VAL_IDE_LINK_BEGIN,
            link[1].keyset_id, transaction->ecam_address, 0ULL) != VAL_SUCCESS ||
        val_host_spdm_root_command(transaction->world, VAL_IDE_LINK_KEY_LOW,
            link[1].keyset_id, link[1].key[0], link[1].key[1]) != VAL_SUCCESS ||
        val_host_spdm_root_command(transaction->world, VAL_IDE_LINK_KEY_HIGH,
            link[1].keyset_id, link[1].key[2], link[1].key[3]) != VAL_SUCCESS ||
        val_host_spdm_root_command(transaction->world, VAL_IDE_LINK_PROG,
            link[1].keyset_id, 0ULL, 0ULL) != VAL_SUCCESS ||
        val_host_spdm_pair_state(&transaction->owner, transaction->session_id,
            transaction->world, &link[1], 1U, VAL_TRUE) != VAL_SUCCESS)
        goto failure;

    transaction->abi_owned = VAL_TRUE;
    transaction->abi_operation = operation;
    transaction->abi_token = ++g_val_host_spdm_abi_token;
    scenario->ecam_address = transaction->ecam_address;
    scenario->invalid_ecam_address = val_pcie_invalid_ecam_base();
    scenario->keyset_id = link[0].keyset_id;
    scenario->handle = transaction->abi_token;
    scenario->token = transaction->abi_token;
    scenario->ide_type = VAL_FIRME_IDE_TYPE_CXL_CACHEMEM;
    memcpy(scenario->keyqw, link[0].key, sizeof(scenario->keyqw));
    return PAL_FIRME_IDE_ABI_READY;

failure:
    val_host_spdm_cleanup(transaction);
    return PAL_FIRME_IDE_ABI_ERROR;
}

/** @brief Establish traffic preconditions after the adapter programs/starts its keyset. */
pal_firme_ide_abi_status_t pal_firme_ide_abi_ready(
    uint32_t world, pal_firme_ide_abi_operation_t operation,
    pal_firme_ide_abi_condition_t condition, const pal_firme_ide_abi_scenario_t *scenario)
{
    val_host_spdm_transaction_t *transaction = &g_val_host_spdm_transaction;

    if (!val_host_spdm_abi_matches(world, scenario) ||
        transaction->abi_operation != operation || condition != PAL_FIRME_IDE_ABI_VALID)
        return PAL_FIRME_IDE_ABI_ERROR;
    transaction->traffic_owned = VAL_TRUE;
    if (pal_cxl_traffic_prepare(transaction->endpoint.root_bdf,
                                 transaction->endpoint.endpoint_bdf) != VAL_SUCCESS)
        return PAL_FIRME_IDE_ABI_ERROR;
    if (val_host_spdm_key_go(&transaction->owner, transaction->session_id,
                              CXL_IDE_KM_KEY_DIRECTION_TX) != VAL_SUCCESS ||
        pal_cxl_traffic_check(operation == PAL_FIRME_IDE_ABI_GO) != VAL_SUCCESS)
        return PAL_FIRME_IDE_ABI_ERROR;
    return PAL_FIRME_IDE_ABI_READY;
}

/** @brief Observe the generated GO/STOP itself using the still-owned paired path. */
pal_firme_ide_abi_status_t pal_firme_ide_abi_observe(
    uint32_t world, pal_firme_ide_abi_operation_t operation,
    pal_firme_ide_abi_condition_t condition, const pal_firme_ide_abi_scenario_t *scenario)
{
    val_host_spdm_transaction_t *transaction = &g_val_host_spdm_transaction;

    if (!val_host_spdm_abi_matches(world, scenario) ||
        operation != transaction->abi_operation || condition != PAL_FIRME_IDE_ABI_VALID)
        return PAL_FIRME_IDE_ABI_ERROR;
    return pal_cxl_traffic_check(operation == PAL_FIRME_IDE_ABI_STOP) == VAL_SUCCESS ?
           PAL_FIRME_IDE_ABI_READY : PAL_FIRME_IDE_ABI_ERROR;
}

/** @brief Stop the endpoint transmitter before the adapter stops its receive keyset. */
pal_firme_ide_abi_status_t pal_firme_ide_abi_quiesce(
    uint32_t world, const pal_firme_ide_abi_scenario_t *scenario)
{
    val_host_spdm_transaction_t *transaction = &g_val_host_spdm_transaction;

    if (!val_host_spdm_abi_matches(world, scenario) || transaction->owner.transport_failed)
        return PAL_FIRME_IDE_ABI_ERROR;
    return val_host_spdm_stop_endpoint(transaction, 0U) == VAL_SUCCESS ?
           PAL_FIRME_IDE_ABI_READY : PAL_FIRME_IDE_ABI_ERROR;
}

/** @brief Release the pair only after the adapter proves its raw keyset is stopped. */
pal_firme_ide_abi_status_t pal_firme_ide_abi_cleanup(
    uint32_t world, pal_firme_ide_abi_operation_t operation,
    pal_firme_ide_abi_condition_t condition, const pal_firme_ide_abi_scenario_t *scenario)
{
    val_host_spdm_transaction_t *transaction = &g_val_host_spdm_transaction;

    if (!val_host_spdm_abi_matches(world, scenario) || operation != transaction->abi_operation ||
        condition != PAL_FIRME_IDE_ABI_VALID || !transaction->link[0].endpoint_stopped)
        return PAL_FIRME_IDE_ABI_ERROR;
    transaction->abi_root_stopped = VAL_TRUE;
    return val_host_spdm_cleanup(transaction) == VAL_SUCCESS ?
           PAL_FIRME_IDE_ABI_READY : PAL_FIRME_IDE_ABI_ERROR;
}

#endif
