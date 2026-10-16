/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "val_common/val_smccc.h"
#include "val_realm_pat_binding.c"

#define CHECK_PAGE_SIZE 4096U
#define CHECK_KEY_SIZE 48U
#define CHECK_SETUP_FID UINT64_C(0xc40001b2)

uint8_t __text_start__[1];
uint8_t __mec_test_end__[1];
static uint8_t g_shared_page[CHECK_PAGE_SIZE] __attribute__((aligned(CHECK_PAGE_SIZE)));
static uint64_t g_reply_status;
static uint64_t g_reply_size = CHECK_KEY_SIZE;
static uint8_t g_private_scalar = 1U;
static uint32_t g_invalid_scalar;
static uint32_t g_calls;
static uint32_t g_random_available = VAL_TRUE;
static uint32_t g_random_fails;
static uint32_t g_random_calls;
static uint32_t g_check_reentrancy;
static uint32_t g_mapping_ready = VAL_TRUE;

/** @brief Assert that setup cleared every byte of the private page or crypto workspace. */
static void check_zero_bytes(const uint8_t *bytes, size_t size)
{
    size_t index;

    for (index = 0U; index < size; index++)
        assert(bytes[index] == 0U);
}

/** @brief Replace only the architectural barrier in the native build. */
void dsb_sy(void)
{
}

/** @brief Allow only the actual bounded native shared page to be mapped. */
uint32_t pal_firme_shared_buffer_realm_mapping_ready(uint64_t base, uint64_t size)
{
    assert(base == (uint64_t)(uintptr_t)g_shared_page && size == sizeof(g_shared_page));
    return g_mapping_ready;
}

/** @brief Inject hardware entropy availability, without adding a production fallback. */
uint32_t val_realm_pat_has_rndr(void)
{
    return g_random_available;
}

/** @brief Use deterministic native-only blinding bytes; production uses RNDR. */
uint32_t val_realm_pat_read_rndr(uint64_t *value)
{
    assert(value != NULL);
    g_random_calls++;
    if (g_random_fails == VAL_TRUE)
        return VAL_FALSE;
    *value = UINT64_C(0x83971abc7582f45d) ^ g_random_calls;
    return VAL_TRUE;
}

/** @brief Simulate the setup ABI only; no PAT or FIRME RAK compliance is claimed. */
void val_smccc_smc_call(uint64_t fid, uint64_t base, uint64_t size, uint64_t curve,
                       uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7,
                       smccc_res_t *response)
{
    val_firme_shared_buffer_lease_t nested = {0};

    assert(fid == CHECK_SETUP_FID && base == (uint64_t)(uintptr_t)g_shared_page);
    assert(size == sizeof(g_shared_page) && curve == 0ULL);
    assert((arg4 | arg5 | arg6 | arg7) == 0ULL && response != NULL);
    check_zero_bytes(g_shared_page, sizeof(g_shared_page));
    g_calls++;
    if (g_check_reentrancy == VAL_TRUE)
        assert(val_firme_shared_buffer_acquire_realm_pat(CHECK_PAGE_SIZE, CHECK_PAGE_SIZE,
                   &nested) == VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
    memset(g_shared_page, 0xab, sizeof(g_shared_page));
    memset(g_shared_page, 0, CHECK_KEY_SIZE);
    g_shared_page[CHECK_KEY_SIZE - 1U] = g_private_scalar;
    if (g_invalid_scalar == VAL_TRUE)
        memset(g_shared_page, 0xff, CHECK_KEY_SIZE);
    *response = (smccc_res_t){.x0 = g_reply_status, .x1 = g_reply_size};
}

/** @brief Compare production output with an independently derived OpenSSL reference hash. */
static void check_binding(const val_firme_shared_buffer_lease_t *lease, const char *expected)
{
    static const char hex_digits[] = "0123456789abcdef";
    size_t index;

    assert(lease->workload_binding != NULL && lease->workload_binding_size == 32U);
    for (index = 0U; index < lease->workload_binding_size; index++) {
        assert(expected[index * 2U] == hex_digits[lease->workload_binding[index] >> 4U]);
        assert(expected[index * 2U + 1U] == hex_digits[lease->workload_binding[index] & 0xfU]);
    }
    assert(expected[lease->workload_binding_size * 2U] == '\0');
}

/** @brief Failed setup must neither leave private material nor return a usable binding. */
static void check_failed_setup(int expected)
{
    val_firme_shared_buffer_lease_t lease = {0};
    const uint8_t *binding = NULL;
    uint32_t size = 0U;

    assert(val_firme_shared_buffer_acquire_realm_pat(CHECK_PAGE_SIZE, CHECK_PAGE_SIZE,
               &lease) == expected);
    assert(lease.owner_token == 0ULL && lease.workload_binding == NULL);
    assert(val_firme_shared_buffer_get_realm_binding(&binding, &size) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
    assert(binding == NULL && size == 0U);
    check_zero_bytes(g_shared_page, sizeof(g_shared_page));
#if FIRME_REALM_PAT_DELEGATED_LEGACY
    check_zero_bytes(g_crypto_heap, sizeof(g_crypto_heap));
#endif
}

/** @brief Cover actual derivation, terminal errors, ownership, renewal, and quarantine. */
int main(int count, char **arguments)
{
    static const int64_t terminal_errors[] = {INT64_C(-1), INT64_C(-2), INT64_C(-5)};
    val_firme_shared_buffer_lease_t first = {0};
    val_firme_shared_buffer_lease_t second = {0};
    val_firme_shared_buffer_lease_t wrong = {0};
    const uint8_t *binding = NULL;
    uint32_t size = 0U;
    uint32_t calls;
    uint32_t pending = VAL_TRUE;
    size_t error_index;

    assert(count == 2);
    assert(val_firme_shared_buffer_get_realm_binding(NULL, &size) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    assert(val_firme_shared_buffer_get_realm_binding(&binding, NULL) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    assert(val_realm_pat_binding_prepare(NULL, CHECK_PAGE_SIZE, &pending) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    assert(val_realm_pat_binding_prepare(g_shared_page, CHECK_PAGE_SIZE, NULL) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    val_firme_shared_buffer_init_realm_pat(0x20000ULL,
                                          (uint64_t)(uintptr_t)g_shared_page);

    if (strncmp(arguments[1], "disabled", sizeof("disabled")) == 0) {
        check_failed_setup(VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
        assert(g_calls == 0U && g_random_calls == 0U);
        return VAL_SUCCESS;
    }

    if (strncmp(arguments[1], "tiny-heap", sizeof("tiny-heap")) == 0) {
        check_failed_setup(VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
        assert(g_calls == 1U);
        return VAL_SUCCESS;
    }

    if (strncmp(arguments[1], "pending", sizeof("pending")) == 0 ||
        strncmp(arguments[1], "unexpected-negative", sizeof("unexpected-negative")) == 0 ||
        strncmp(arguments[1], "positive-status", sizeof("positive-status")) == 0) {
        g_reply_status = strncmp(arguments[1], "pending", sizeof("pending")) == 0 ?
                         (uint64_t)INT64_C(-8) : UINT64_C(1);
        if (strncmp(arguments[1], "unexpected-negative", sizeof("unexpected-negative")) == 0)
            g_reply_status = (uint64_t)INT64_C(-6);
        assert(val_firme_shared_buffer_acquire_realm_pat(CHECK_PAGE_SIZE, CHECK_PAGE_SIZE,
                   &first) == VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
        assert(g_calls == 1U && first.owner_token == 0ULL);
        assert(g_shared_page[CHECK_KEY_SIZE] == 0xabU);
        val_firme_shared_buffer_init_realm_pat(0x20000ULL,
                                              (uint64_t)(uintptr_t)g_shared_page);
        assert(val_firme_shared_buffer_acquire_realm_pat(CHECK_PAGE_SIZE, CHECK_PAGE_SIZE,
                   &second) == VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
        assert(g_calls == 1U);
        return VAL_SUCCESS;
    }

    assert(strncmp(arguments[1], "normal", sizeof("normal")) == 0);
    g_random_available = VAL_FALSE;
    check_failed_setup(VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
    assert(g_calls == 0U);
    g_random_available = VAL_TRUE;
    for (error_index = 0U; error_index < sizeof(terminal_errors) / sizeof(terminal_errors[0]);
         error_index++) {
        g_reply_status = (uint64_t)terminal_errors[error_index];
        check_failed_setup(VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
        assert(val_realm_pat_binding_status() == (uint64_t)terminal_errors[error_index]);
    }

    g_reply_status = 0ULL;
    g_reply_size = 0ULL;
    check_failed_setup(VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    g_reply_size = CHECK_KEY_SIZE - 1U;
    check_failed_setup(VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    g_reply_size = UINT64_MAX;
    check_failed_setup(VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    g_reply_size = CHECK_KEY_SIZE;
    g_private_scalar = 0U;
    check_failed_setup(VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    g_private_scalar = 1U;
    g_invalid_scalar = VAL_TRUE;
    check_failed_setup(VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    g_invalid_scalar = VAL_FALSE;
    g_random_fails = VAL_TRUE;
    check_failed_setup(VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    g_random_fails = VAL_FALSE;
    g_check_reentrancy = VAL_TRUE;
    assert(val_firme_shared_buffer_acquire_realm_pat(CHECK_PAGE_SIZE, CHECK_PAGE_SIZE,
               &first) == VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE);
    check_binding(&first, "18d4f417c4f5de8cc84b31e14f45e44a9167bd6dad0ea430b3e0f43e33098f33");
    check_zero_bytes(g_shared_page, sizeof(g_shared_page));
#if FIRME_REALM_PAT_DELEGATED_LEGACY
    check_zero_bytes(g_crypto_heap, sizeof(g_crypto_heap));
#endif
    calls = g_calls;
    assert(val_firme_shared_buffer_acquire_realm_pat(CHECK_PAGE_SIZE, CHECK_PAGE_SIZE,
               &second) == VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
    assert(g_calls == calls);
    wrong = first;
    wrong.workload_binding_size--;
    assert(val_firme_shared_buffer_release_realm_pat(&wrong, VAL_FALSE) == VAL_ERROR);
    check_binding(&first, "18d4f417c4f5de8cc84b31e14f45e44a9167bd6dad0ea430b3e0f43e33098f33");
    assert(val_firme_shared_buffer_release_realm_pat(&first, VAL_FALSE) == VAL_SUCCESS);
    assert(val_firme_shared_buffer_release_realm_pat(&first, VAL_FALSE) == VAL_ERROR);
    assert(val_firme_shared_buffer_get_realm_binding(&binding, &size) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
    g_private_scalar = 2U;
    assert(val_firme_shared_buffer_acquire_realm_pat(CHECK_PAGE_SIZE, CHECK_PAGE_SIZE,
               &second) == VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE);
    assert(first.owner_token != second.owner_token);
    check_binding(&second, "9099ca53d3dd42b1d30eee5e36a1538c67d368758b22f30a229c8f5b69863bb9");
    assert(val_firme_shared_buffer_release_realm_pat(&first, VAL_FALSE) == VAL_ERROR);
    assert(val_firme_shared_buffer_release_realm_pat(&second, VAL_TRUE) == VAL_SUCCESS);
    check_binding(&second, "9099ca53d3dd42b1d30eee5e36a1538c67d368758b22f30a229c8f5b69863bb9");
    calls = g_calls;
    assert(val_firme_shared_buffer_acquire_realm_pat(CHECK_PAGE_SIZE, CHECK_PAGE_SIZE,
               &first) == VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
    assert(g_calls == calls);
    return VAL_SUCCESS;
}
