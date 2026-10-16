/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_realm_pat_binding.h"
#include "val_firme_shared_buffer.h"
#include "val_common/val_status.h"

#if FIRME_REALM_PAT_DELEGATED_LEGACY

#include "aarch64/sysreg.h"
#include "val_common/val_mem.h"
#include "val_common/val_smccc.h"
#include "mbedtls/ecp.h"
#include "mbedtls/memory_buffer_alloc.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/sha256.h"

#define VAL_REALM_PAT_LEGACY_RAK_FID UINT64_C(0xc40001b2)
#define VAL_REALM_PAT_LEGACY_CURVE UINT64_C(0)
#define VAL_REALM_PAT_LEGACY_SUCCESS INT64_C(0)
#define VAL_REALM_PAT_LEGACY_UNKNOWN INT64_C(-1)
#define VAL_REALM_PAT_LEGACY_BAD_ADDRESS INT64_C(-2)
#define VAL_REALM_PAT_LEGACY_INVALID INT64_C(-5)
#define VAL_REALM_PAT_SHARED_BYTES 4096U
#define VAL_REALM_PAT_SCALAR_BYTES 48U
#define VAL_REALM_PAT_PUBLIC_BYTES (1U + 2U * VAL_REALM_PAT_SCALAR_BYTES)
#define VAL_REALM_PAT_COSE_BYTES (11U + 2U * VAL_REALM_PAT_SCALAR_BYTES)
#define VAL_REALM_PAT_RNDR_RETRIES 16U
#define VAL_REALM_PAT_NO_ENTROPY UINT64_C(0x100000000)
#ifndef VAL_REALM_PAT_HEAP_BYTES
#define VAL_REALM_PAT_HEAP_BYTES (64U * 1024U)
#endif

static uint8_t g_binding[VAL_REALM_PAT_BINDING_SIZE];
static uint32_t g_binding_valid;
static uint64_t g_binding_status;
static uint8_t g_crypto_heap[VAL_REALM_PAT_HEAP_BYTES] __attribute__((aligned(16)));

/** @brief Implement the mbedTLS runtime erasure contract without hosted-libc dependencies. */
void mbedtls_platform_zeroize(void *buffer, size_t size)
{
    volatile uint8_t *bytes = buffer;
    size_t index;

    if (bytes == NULL)
        return;
    for (index = 0U; index < size; index++)
        bytes[index] = 0U;
}

/** @brief Do not continue execution if the crypto allocator detects memory corruption. */
_Noreturn void val_realm_pat_crypto_abort(int status)
{
    g_binding_status = (uint64_t)(int64_t)status;
    __builtin_trap();
}

/** @brief Supply bounded architectural randomness for private-scalar blinding only. */
static int val_realm_pat_crypto_random(void *context, unsigned char *output, size_t size)
{
    uint64_t random_value = 0ULL;
    size_t offset = 0U;
    size_t count;
    uint32_t attempt;

    if (context != NULL || output == NULL)
        return VAL_ERROR;
    while (offset < size) {
        for (attempt = 0U; attempt < VAL_REALM_PAT_RNDR_RETRIES; attempt++) {
            if (val_realm_pat_read_rndr(&random_value) == VAL_TRUE)
                break;
        }

        if (attempt == VAL_REALM_PAT_RNDR_RETRIES) {
            mbedtls_platform_zeroize(output, size);
            mbedtls_platform_zeroize(&random_value, sizeof(random_value));
            return VAL_ERROR;
        }

        count = size - offset;
        if (count > sizeof(random_value))
            count = sizeof(random_value);
        memcpy(output + offset, &random_value, count);
        offset += count;
    }

    mbedtls_platform_zeroize(&random_value, sizeof(random_value));
    return VAL_SUCCESS;
}

/**
 * @brief Derive P-384 public coordinates and hash their deterministic COSE_Key encoding.
 * The encoding is {1:2, -1:2, -2:bstr(X), -3:bstr(Y)}, as used by TF-RMM.
 * All EC arithmetic, private-key validation and hashing use pinned mbedTLS.
 */
static int val_realm_pat_hash_public_key(const uint8_t *private_key)
{
    static const uint8_t cose_prefix[] = {0xa4U, 0x01U, 0x02U, 0x20U,
                                          0x02U, 0x21U, 0x58U, 0x30U};
    static const uint8_t cose_y_header[] = {0x22U, 0x58U, 0x30U};
    mbedtls_ecp_group group;
    mbedtls_ecp_point public_key;
    mbedtls_mpi scalar;
    uint8_t public_bytes[VAL_REALM_PAT_PUBLIC_BYTES] = {0};
    uint8_t cose[VAL_REALM_PAT_COSE_BYTES] = {0};
    size_t public_size = 0U;
    int result;

    mbedtls_memory_buffer_alloc_init(g_crypto_heap, sizeof(g_crypto_heap));
    mbedtls_ecp_group_init(&group);
    mbedtls_ecp_point_init(&public_key);
    mbedtls_mpi_init(&scalar);
    result = mbedtls_ecp_group_load(&group, MBEDTLS_ECP_DP_SECP384R1);
    if (result != VAL_SUCCESS)
        goto cleanup;
    result = mbedtls_mpi_read_binary(&scalar, private_key, VAL_REALM_PAT_SCALAR_BYTES);
    if (result != VAL_SUCCESS)
        goto cleanup;
    result = mbedtls_ecp_check_privkey(&group, &scalar);
    if (result != VAL_SUCCESS)
        goto cleanup;
    result = mbedtls_ecp_mul(&group, &public_key, &scalar, &group.G,
                             val_realm_pat_crypto_random, NULL);
    if (result != VAL_SUCCESS)
        goto cleanup;
    result = mbedtls_ecp_check_pubkey(&group, &public_key);
    if (result != VAL_SUCCESS)
        goto cleanup;
    result = mbedtls_ecp_point_write_binary(&group, &public_key,
                  MBEDTLS_ECP_PF_UNCOMPRESSED, &public_size, public_bytes,
                  sizeof(public_bytes));
    if (result != VAL_SUCCESS)
        goto cleanup;
    if (public_size != sizeof(public_bytes) || public_bytes[0] != 0x04U) {
        result = VAL_ERROR;
        goto cleanup;
    }

    memcpy(cose, cose_prefix, sizeof(cose_prefix));
    memcpy(cose + sizeof(cose_prefix), public_bytes + 1U, VAL_REALM_PAT_SCALAR_BYTES);
    memcpy(cose + sizeof(cose_prefix) + VAL_REALM_PAT_SCALAR_BYTES,
           cose_y_header, sizeof(cose_y_header));
    memcpy(cose + sizeof(cose_prefix) + VAL_REALM_PAT_SCALAR_BYTES + sizeof(cose_y_header),
           public_bytes + 1U + VAL_REALM_PAT_SCALAR_BYTES,
           VAL_REALM_PAT_SCALAR_BYTES);
    result = mbedtls_sha256(cose, sizeof(cose), g_binding, 0);

cleanup:
    mbedtls_mpi_free(&scalar);
    mbedtls_ecp_point_free(&public_key);
    mbedtls_ecp_group_free(&group);
    mbedtls_memory_buffer_alloc_free();
    mbedtls_platform_zeroize(g_crypto_heap, sizeof(g_crypto_heap));
    mbedtls_platform_zeroize(public_bytes, sizeof(public_bytes));
    mbedtls_platform_zeroize(cose, sizeof(cose));
    return result;
}

/** @brief Remove the public binding after terminal release or failed setup. */
void val_realm_pat_binding_clear(void)
{
    mbedtls_platform_zeroize(g_binding, sizeof(g_binding));
    g_binding_valid = VAL_FALSE;
}

/** @brief Retrieve the live delegated private RAK solely to prepare the PAT fixture. */
int val_realm_pat_binding_prepare(uint8_t *buffer, uint64_t size, uint32_t *pending)
{
    smccc_res_t response = {0};
    int64_t status;
    int result;

    if (pending == NULL)
        return VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    *pending = VAL_FALSE;
    val_realm_pat_binding_clear();
    g_binding_status = 0ULL;
    if (buffer == NULL || size != VAL_REALM_PAT_SHARED_BYTES ||
        ((uintptr_t)buffer & (VAL_REALM_PAT_SHARED_BYTES - 1U)) != 0U)
        return VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    if (val_realm_pat_has_rndr() != VAL_TRUE) {
        g_binding_status = VAL_REALM_PAT_NO_ENTROPY;
        return VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE;
    }

    mbedtls_platform_zeroize(buffer, size);
    dsb_sy();
    val_smccc_smc_call(VAL_REALM_PAT_LEGACY_RAK_FID, (uint64_t)(uintptr_t)buffer,
                      size, VAL_REALM_PAT_LEGACY_CURVE, 0ULL, 0ULL, 0ULL, 0ULL,
                      &response);
    dsb_sy();
    status = (int64_t)response.x0;
    g_binding_status = response.x0;
    if (status != VAL_REALM_PAT_LEGACY_SUCCESS) {
        if (status == VAL_REALM_PAT_LEGACY_BAD_ADDRESS ||
            status == VAL_REALM_PAT_LEGACY_INVALID ||
            status == VAL_REALM_PAT_LEGACY_UNKNOWN) {
            mbedtls_platform_zeroize(buffer, size);
            dsb_sy();
        } else {
            *pending = VAL_TRUE;
        }

        return VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    }

    result = VAL_ERROR;
    if (response.x1 == VAL_REALM_PAT_SCALAR_BYTES)
        result = val_realm_pat_hash_public_key(buffer);
    mbedtls_platform_zeroize(buffer, size);
    dsb_sy();
    if (result != VAL_SUCCESS) {
        g_binding_status = (uint64_t)(int64_t)result;
        val_realm_pat_binding_clear();
        return VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    }

    g_binding_valid = VAL_TRUE;
    return VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE;
}

/** @brief Return only a binding derived from the current lease's successful RAK setup. */
int val_firme_shared_buffer_get_realm_binding(const uint8_t **binding, uint32_t *size)
{
    if (binding == NULL || size == NULL)
        return VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    *binding = NULL;
    *size = 0U;
    if (g_binding_valid != VAL_TRUE)
        return VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE;
    *binding = g_binding;
    *size = sizeof(g_binding);
    return VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE;
}

/** @brief Preserve the actual setup failure for diagnostics rather than claim ABI absence. */
uint64_t val_realm_pat_binding_status(void)
{
    return g_binding_status;
}

#else

/** @brief Leave the fixture unavailable when the attestation model is not established. */
int val_realm_pat_binding_prepare(uint8_t *buffer, uint64_t size, uint32_t *pending)
{
    if (buffer == NULL || size == 0ULL || pending == NULL)
        return VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    *pending = VAL_FALSE;
    return VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE;
}

/** @brief No key material exists in the model-unconfigured build. */
void val_realm_pat_binding_clear(void)
{
}

/** @brief No binding is fabricated for the model-unconfigured build. */
int val_firme_shared_buffer_get_realm_binding(const uint8_t **binding, uint32_t *size)
{
    if (binding == NULL || size == NULL)
        return VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    *binding = NULL;
    *size = 0U;
    return VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE;
}

/** @brief No setup ABI was invoked in the model-unconfigured build. */
uint64_t val_realm_pat_binding_status(void)
{
    return 0ULL;
}

#endif
