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

static uint32_t g_root_fail_mask;
static uint32_t g_endpoint_fail_mask;
static uint32_t g_root_stops[2];
static uint32_t g_endpoint_stops[2];
static uint32_t g_root_releases;
static uint32_t g_session_closes;
static uint32_t g_context_closes;
static uint32_t g_traffic_releases;
static uint32_t g_doe_releases;
static uint32_t g_go_calls;
static uint32_t g_traffic_checks;
static uint32_t g_expect_blocked;
static int g_traffic_status;
static int g_doe_status;
static libspdm_return_t g_session_status;
static uint32_t g_test_pe = 3U;
static libspdm_return_t g_handshake_status;
static uint32_t g_handshake_calls;

/** @brief Supply certificate discovery solely to reach the mocked handshake boundary. */
libspdm_return_t libspdm_get_digest(void *spdm_context, const uint32_t *session_id,
                                    uint8_t *slot_mask, void *digests)
{
    assert(spdm_context && !session_id && slot_mask && digests);
    *slot_mask = 1U;
    return LIBSPDM_STATUS_SUCCESS;
}

/** @brief Bypass cryptographic verification in the isolated lifetime fixture. */
libspdm_return_t libspdm_get_certificate(void *spdm_context,
    const uint32_t *session_id, uint8_t slot_id, size_t *cert_size, void *certificate)
{
    assert(spdm_context && !session_id && slot_id == 0U && cert_size && certificate);
    return LIBSPDM_STATUS_SUCCESS;
}

/** @brief Model a session allocated before FINISH returns BUSY or a malformed response. */
libspdm_return_t libspdm_start_session(void *spdm_context, bool use_psk,
    const void *psk_hint, uint16_t psk_hint_size, uint8_t measurement_hash_type,
    uint8_t slot_id, uint8_t session_policy, uint32_t *session_id,
    uint8_t *heartbeat_period, void *measurement_hash)
{
    assert(spdm_context && !use_psk && !psk_hint && psk_hint_size == 0U);
    assert(measurement_hash_type == SPDM_REQUEST_NO_MEASUREMENT_SUMMARY_HASH);
    assert(slot_id == 0U && session_policy == 0U && session_id);
    assert(!heartbeat_period && !measurement_hash);
    *session_id = 7U;
    g_handshake_calls++;
    return g_handshake_status;
}

/** @brief Keep persistent host fixtures on their owning primary PE. */
uint32_t val_pe_index(void)
{
    return g_test_pe;
}

/** @brief Model a runtime primary that is not PE index zero. */
uint32_t val_host_mp_primary_index(void)
{
    return 3U;
}

/** @brief Suppress intentional failure diagnostics from native cleanup tests. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *format, ...)
{
    (void)verbosity;
    (void)tag;
    (void)format;
    return VAL_SUCCESS;
}

/** @brief Inject a direction-specific Root Port quiesce failure. */
void val_firme_ide_link_command(uint64_t operation, uint64_t keyset_id,
                                uint64_t data0, uint64_t data1,
                                firme_acs_call_res_t *out)
{
    uint32_t direction = (uint32_t)((keyset_id >> 1U) & 1U);

    (void)data0;
    (void)data1;
    *out = (firme_acs_call_res_t){.x0 = ACS_STATUS_PASS};
    if (operation == VAL_IDE_LINK_QUIESCE || operation == VAL_IDE_LINK_STOP) {
        g_root_stops[direction]++;
        if (g_root_fail_mask & (1U << direction))
            out->x0 = ACS_STATUS_FAIL;
    } else if (operation == VAL_IDE_LINK_RELEASE) {
        g_root_releases++;
    } else {
        assert(operation == VAL_IDE_LINK_GO);
    }
}

/** @brief Route a mocked cross-world link command without real firmware. */
int val_firme_world_call(firme_acs_world_t world, firme_acs_cmd_t command,
                         uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                         firme_acs_call_res_t *out)
{
    (void)world;
    assert(command == FIRME_ACS_CMD_IDE_LINK);
    val_firme_ide_link_command(arg0, arg1, arg2, arg3, out);
    return VAL_SUCCESS;
}

/** @brief Refuse an endpoint STOP while keeping its session owner observable. */
libspdm_return_t cxl_ide_km_key_set_stop(const void *doe_context, void *spdm_context,
    const uint32_t *session_id, uint8_t stream_id, uint8_t key_sub_stream, uint8_t port_index)
{
    uint32_t direction = (key_sub_stream & CXL_IDE_KM_KEY_DIRECTION_TX) ? 0U : 1U;

    (void)doe_context;
    assert(spdm_context && session_id && *session_id == 7U);
    assert(stream_id == 0U && port_index == 0U);
    g_endpoint_stops[direction]++;
    return g_endpoint_fail_mask & (1U << direction) ?
           LIBSPDM_STATUS_INVALID_PARAMETER : LIBSPDM_STATUS_SUCCESS;
}

/** @brief Produce a matched endpoint GO acknowledgment for observer wiring tests. */
libspdm_return_t cxl_ide_km_send_receive_data(void *spdm_context,
    const uint32_t *session_id, const void *request, size_t request_size,
    void *response, size_t *response_size)
{
    const cxl_ide_km_k_set_go_t *go = request;
    cxl_ide_km_k_gostop_ack_t *ack = response;

    assert(spdm_context && session_id && *session_id == 7U);
    assert(request_size == sizeof(*go) && *response_size == sizeof(*ack));
    assert(go->header.object_id == CXL_IDE_KM_OBJECT_ID_K_SET_GO);
    *ack = (cxl_ide_km_k_gostop_ack_t){0};
    ack->header.object_id = CXL_IDE_KM_OBJECT_ID_K_SET_GOSTOP_ACK;
    ack->stream_id = go->stream_id;
    ack->port_index = go->port_index;
    ack->key_sub_stream = go->key_sub_stream & ~CXL_IDE_KM_KEY_MODE_CONTAINMENT;
    g_go_calls++;
    return LIBSPDM_STATUS_SUCCESS;
}

/** @brief Count END_SESSION and inject an uncertain session-close result. */
libspdm_return_t libspdm_stop_session(void *spdm_context, uint32_t session_id,
                                      uint8_t attributes)
{
    assert(spdm_context && session_id == 7U && attributes == 0U);
    g_session_closes++;
    return g_session_status;
}

/** @brief Detect premature context disposal. */
void libspdm_deinit_context(void *spdm_context)
{
    assert(spdm_context);
    g_context_closes++;
}

/** @brief Model a prepared observation aperture. */
int pal_cxl_traffic_prepare(uint32_t root_bdf, uint32_t endpoint_bdf)
{
    (void)root_bdf;
    (void)endpoint_bdf;
    return VAL_SUCCESS;
}

/** @brief Record which actual invocation effect the hook asks to observe. */
int pal_cxl_traffic_check(uint32_t expect_blocked)
{
    g_traffic_checks++;
    g_expect_blocked = expect_blocked;
    return g_traffic_status;
}

/** @brief Inject aperture restoration failures before session disposal. */
int pal_cxl_traffic_release(void)
{
    g_traffic_releases++;
    return g_traffic_status;
}

/** @brief Model retryable bus-configuration restoration. */
int val_host_doe_release_endpoint(val_host_doe_endpoint_t *endpoint)
{
    assert(endpoint);
    g_doe_releases++;
    return g_doe_status;
}

/** @brief Allocate an entirely synthetic active fixture for cleanup regressions. */
static void test_transaction_init(val_host_spdm_transaction_t *transaction)
{
    uint32_t direction;

    memset(transaction, 0, sizeof(*transaction));
    transaction->open = VAL_TRUE;
    transaction->world = FIRME_ACS_WORLD_NSEL2;
    transaction->session_id = 7U;
    transaction->session_started = VAL_TRUE;
    transaction->traffic_owned = VAL_TRUE;
    transaction->owner.initialized = VAL_TRUE;
    transaction->owner.endpoint = &transaction->endpoint;
    transaction->owner.spdm_context = calloc(1U, 16U);
    transaction->owner.scratch_buffer = calloc(1U, 16U);
    transaction->owner.sender_buffer = calloc(1U, 16U);
    transaction->owner.receiver_buffer = calloc(1U, 16U);
    assert(transaction->owner.spdm_context && transaction->owner.scratch_buffer &&
           transaction->owner.sender_buffer && transaction->owner.receiver_buffer);
    for (direction = 0U; direction < 2U; direction++) {
        transaction->link[direction].keyset_id = direction << 1U;
        transaction->link[direction].root_open = VAL_TRUE;
        transaction->link[direction].endpoint_programmed = VAL_TRUE;
        transaction->link[direction].endpoint_direction = direction == 0U ?
            CXL_IDE_KM_KEY_DIRECTION_TX : CXL_IDE_KM_KEY_DIRECTION_RX;
    }
    g_root_fail_mask = 0U;
    g_endpoint_fail_mask = 0U;
    memset(g_root_stops, 0, sizeof(g_root_stops));
    memset(g_endpoint_stops, 0, sizeof(g_endpoint_stops));
    g_root_releases = 0U;
    g_session_closes = 0U;
    g_context_closes = 0U;
    g_traffic_releases = 0U;
    g_doe_releases = 0U;
    g_go_calls = 0U;
    g_traffic_checks = 0U;
    g_session_status = LIBSPDM_STATUS_SUCCESS;
    g_traffic_status = VAL_SUCCESS;
    g_doe_status = VAL_SUCCESS;
    g_handshake_status = LIBSPDM_STATUS_SUCCESS;
    g_handshake_calls = 0U;
}

/** @brief Destroy mock-only quarantined allocations between independent tests. */
static void test_transaction_dispose(val_host_spdm_transaction_t *transaction)
{
    val_host_spdm_close(&transaction->owner);
    memset(transaction, 0, sizeof(*transaction));
}

/** @brief Preserve a possibly established peer session when local FINISH fails. */
static void test_handshake_lifetime(void)
{
    const libspdm_return_t failures[] = {LIBSPDM_STATUS_BUSY_PEER,
                                        LIBSPDM_STATUS_INVALID_MSG_FIELD};
    val_host_spdm_transaction_t transaction;
    void *saved_context;
    uint32_t failure_index;

    for (failure_index = 0U; failure_index < sizeof(failures) / sizeof(failures[0]);
         failure_index++) {
        test_transaction_init(&transaction);
        memset(transaction.link, 0, sizeof(transaction.link));
        transaction.traffic_owned = VAL_FALSE;
        transaction.session_started = VAL_FALSE;
        transaction.session_id = 0U;
        saved_context = transaction.owner.spdm_context;
        g_handshake_status = failures[failure_index];
        assert(val_host_spdm_start_session(&transaction.owner,
                                            &transaction.session_id) == VAL_ERROR);
        assert(transaction.session_id == 7U && !transaction.owner.transport_failed);
        assert(val_host_spdm_cleanup(&transaction) == VAL_ERROR);
        assert(transaction.open && transaction.owner.spdm_context == saved_context);
        assert(!g_session_closes && !g_context_closes && !g_doe_releases);
        g_handshake_status = LIBSPDM_STATUS_SUCCESS;
        assert(val_host_spdm_start_session(&transaction.owner,
                                            &transaction.session_id) == VAL_ERROR);
        assert(g_handshake_calls == 1U);
        test_transaction_dispose(&transaction);
    }

    test_transaction_init(&transaction);
    memset(transaction.link, 0, sizeof(transaction.link));
    transaction.traffic_owned = VAL_FALSE;
    transaction.session_started = VAL_FALSE;
    assert(val_host_spdm_start_session(&transaction.owner,
                                        &transaction.session_id) == VAL_SUCCESS);
    transaction.session_started = VAL_TRUE;
    assert(val_host_spdm_cleanup(&transaction) == VAL_SUCCESS);
    assert(g_session_closes == 1U && g_context_closes == 1U);
}

/** @brief Keep transmitters, receivers, sessions and buffers owned after failed STOP. */
static void test_cleanup_lifetime(void)
{
    val_host_spdm_transaction_t transaction;
    void *saved_context;

    test_transaction_init(&transaction);
    saved_context = transaction.owner.spdm_context;
    g_root_fail_mask = 2U;
    assert(val_host_spdm_cleanup(&transaction) == VAL_ERROR);
    assert(g_endpoint_stops[1] == 0U && g_root_stops[0] == 1U);
    assert(!g_session_closes && !g_context_closes && !g_root_releases);
    assert(transaction.owner.spdm_context == saved_context && transaction.session_started);
    g_root_fail_mask = 0U;
    assert(val_host_spdm_cleanup(&transaction) == VAL_SUCCESS);
    assert(g_root_stops[0] == 1U && g_endpoint_stops[0] == 1U);
    assert(g_session_closes == 1U && g_context_closes == 1U && !transaction.open);

    test_transaction_init(&transaction);
    g_endpoint_fail_mask = 1U;
    assert(val_host_spdm_cleanup(&transaction) == VAL_ERROR);
    assert(g_root_stops[0] == 0U && !g_session_closes && !g_context_closes);
    g_endpoint_fail_mask = 0U;
    assert(val_host_spdm_cleanup(&transaction) == VAL_SUCCESS);
    assert(g_root_stops[1] == 1U && g_endpoint_stops[1] == 1U);

    test_transaction_init(&transaction);
    g_traffic_status = VAL_ERROR;
    assert(val_host_spdm_cleanup(&transaction) == VAL_ERROR);
    assert(transaction.quarantined && !g_session_closes && !g_context_closes);
    g_traffic_status = VAL_SUCCESS;
    assert(val_host_spdm_cleanup(&transaction) == VAL_ERROR);
    assert(g_traffic_releases == 1U);
    test_transaction_dispose(&transaction);

    test_transaction_init(&transaction);
    g_session_status = LIBSPDM_STATUS_INVALID_PARAMETER;
    assert(val_host_spdm_cleanup(&transaction) == VAL_ERROR);
    assert(transaction.quarantined && transaction.session_started && !g_context_closes);
    assert(val_host_spdm_cleanup(&transaction) == VAL_ERROR);
    assert(g_session_closes == 1U);
    test_transaction_dispose(&transaction);

    test_transaction_init(&transaction);
    g_doe_status = VAL_ERROR;
    assert(val_host_spdm_cleanup(&transaction) == VAL_ERROR);
    assert(!transaction.session_started && !g_context_closes);
    g_doe_status = VAL_SUCCESS;
    assert(val_host_spdm_cleanup(&transaction) == VAL_SUCCESS);
    assert(g_session_closes == 1U && g_context_closes == 1U && g_doe_releases == 2U);

    test_transaction_init(&transaction);
    transaction.owner.transport_failed = VAL_TRUE;
    assert(val_host_spdm_cleanup(&transaction) == VAL_ERROR);
    assert(!g_session_closes && !g_context_closes && !g_endpoint_stops[0]);
    test_transaction_dispose(&transaction);

    test_transaction_init(&transaction);
    g_root_fail_mask = 2U;
    assert(val_host_spdm_pair_state(&transaction.owner, transaction.session_id,
        transaction.world, &transaction.link[1], 1U, VAL_FALSE) == VAL_ERROR);
    assert(g_endpoint_stops[1] == 0U);
    g_root_fail_mask = 0U;
    assert(val_host_spdm_cleanup(&transaction) == VAL_SUCCESS);
}

/** @brief Bind GO/STOP evidence to the generated invocation without Root Port replay. */
static void test_generated_observer(void)
{
    val_host_spdm_transaction_t *transaction = &g_val_host_spdm_transaction;
    pal_firme_ide_abi_scenario_t scenario = {.token = 9ULL,
        .ide_type = VAL_FIRME_IDE_TYPE_CXL_CACHEMEM};

    test_transaction_init(transaction);
    transaction->abi_owned = VAL_TRUE;
    transaction->abi_operation = PAL_FIRME_IDE_ABI_GO;
    transaction->abi_token = scenario.token;
    transaction->link[0].root_open = VAL_FALSE;
    assert(val_host_spdm_cleanup(transaction) == VAL_ERROR);
    assert(!g_session_closes && !g_root_stops[1]);
    assert(pal_firme_ide_abi_ready(FIRME_ACS_WORLD_NSEL2, PAL_FIRME_IDE_ABI_GO,
        PAL_FIRME_IDE_ABI_VALID, &scenario) == PAL_FIRME_IDE_ABI_READY);
    assert(g_go_calls == 1U && g_traffic_checks == 1U && g_expect_blocked);
    g_test_pe = 0U;
    assert(pal_firme_ide_abi_observe(FIRME_ACS_WORLD_NSEL2, PAL_FIRME_IDE_ABI_GO,
        PAL_FIRME_IDE_ABI_VALID, &scenario) == PAL_FIRME_IDE_ABI_ERROR);
    assert(g_traffic_checks == 1U);
    g_test_pe = 3U;
    assert(pal_firme_ide_abi_observe(FIRME_ACS_WORLD_REALM, PAL_FIRME_IDE_ABI_GO,
        PAL_FIRME_IDE_ABI_VALID, &scenario) == PAL_FIRME_IDE_ABI_ERROR);
    scenario.token++;
    assert(pal_firme_ide_abi_quiesce(FIRME_ACS_WORLD_NSEL2, &scenario) ==
           PAL_FIRME_IDE_ABI_ERROR);
    scenario.token--;
    assert(pal_firme_ide_abi_observe(FIRME_ACS_WORLD_NSEL2, PAL_FIRME_IDE_ABI_GO,
        PAL_FIRME_IDE_ABI_VALID, &scenario) == PAL_FIRME_IDE_ABI_READY);
    assert(g_go_calls == 1U && g_traffic_checks == 2U && !g_expect_blocked);
    assert(!g_root_stops[0] && !g_root_stops[1]);
    assert(pal_firme_ide_abi_cleanup(FIRME_ACS_WORLD_NSEL2, PAL_FIRME_IDE_ABI_GO,
        PAL_FIRME_IDE_ABI_VALID, &scenario) == PAL_FIRME_IDE_ABI_ERROR);
    assert(pal_firme_ide_abi_quiesce(FIRME_ACS_WORLD_NSEL2, &scenario) ==
           PAL_FIRME_IDE_ABI_READY);
    assert(pal_firme_ide_abi_cleanup(FIRME_ACS_WORLD_NSEL2, PAL_FIRME_IDE_ABI_GO,
        PAL_FIRME_IDE_ABI_VALID, &scenario) == PAL_FIRME_IDE_ABI_READY);

    test_transaction_init(transaction);
    transaction->abi_owned = VAL_TRUE;
    transaction->abi_operation = PAL_FIRME_IDE_ABI_STOP;
    transaction->abi_token = scenario.token;
    transaction->link[0].root_open = VAL_FALSE;
    assert(pal_firme_ide_abi_ready(FIRME_ACS_WORLD_NSEL2, PAL_FIRME_IDE_ABI_STOP,
        PAL_FIRME_IDE_ABI_VALID, &scenario) == PAL_FIRME_IDE_ABI_READY);
    assert(g_go_calls == 1U && g_traffic_checks == 1U && !g_expect_blocked);
    g_traffic_status = VAL_ERROR;
    assert(pal_firme_ide_abi_observe(FIRME_ACS_WORLD_NSEL2, PAL_FIRME_IDE_ABI_STOP,
        PAL_FIRME_IDE_ABI_VALID, &scenario) == PAL_FIRME_IDE_ABI_ERROR);
    assert(g_expect_blocked && g_go_calls == 1U && !g_root_stops[1]);
    g_traffic_status = VAL_SUCCESS;
    assert(pal_firme_ide_abi_quiesce(FIRME_ACS_WORLD_NSEL2, &scenario) ==
           PAL_FIRME_IDE_ABI_READY);
    assert(pal_firme_ide_abi_cleanup(FIRME_ACS_WORLD_NSEL2, PAL_FIRME_IDE_ABI_STOP,
        PAL_FIRME_IDE_ABI_VALID, &scenario) == PAL_FIRME_IDE_ABI_READY);
}

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
    test_cleanup_lifetime();
    test_handshake_lifetime();
    test_generated_observer();
    puts("SPDM buffers, cleanup lifetime and generated observer: PASS");
    return VAL_SUCCESS;
}
