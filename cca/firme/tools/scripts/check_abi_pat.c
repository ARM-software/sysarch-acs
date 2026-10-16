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
#include "val_common/val_status.h"
#include "val_firme_abi.h"
#include "val_firme_gpt_defs.h"
#include "val_firme_rules.h"
#include "val_firme_shared_buffer.h"
#include "val_firme_suite.h"
#include "val_log.h"

int firme_base_shared_buffer_alignment_run_ns(uint64_t base_pa, firme_rule_res_t *out);
int firme_base_shared_buffer_size_bounds_run_ns(uint64_t base_pa, firme_rule_res_t *out);
int firme_base_shared_buffer_mapping_attributes_run_ns(uint64_t base_pa, firme_rule_res_t *out);
int firme_base_shared_buffer_pas_gpi_run_ns(uint64_t base_pa, firme_rule_res_t *out);
int firme_base_partial_retrieval_last_chunk_run_ns(uint64_t base_pa, firme_rule_res_t *out);
int firme_base_partial_retrieval_intermediate_chunk_run_ns(uint64_t base_pa, firme_rule_res_t *out);
int firme_base_partial_retrieval_intermediate_chunk_run_realm(uint64_t base_pa,
                                                            firme_rule_res_t *out);
int firme_base_partial_retrieval_no_chunk_run_ns(uint64_t base_pa, firme_rule_res_t *out);

typedef struct {
    int64_t status;
    uint64_t written;
    uint64_t remaining;
} native_reply_t;

static uint8_t g_buffer[65536];
static const uint8_t g_challenge[32] = {0x41U, 0x43U, 0x53U};
static const uint8_t g_binding[32] = {0x54U, 0x45U, 0x53U, 0x54U};
static const uint64_t g_buffer_pa = UINT64_C(0x90000000);
static uint64_t g_poison;
static native_reply_t g_replies[3];
static uint32_t g_reply_count;
static uint32_t g_calls;
static uint32_t g_acquires;
static uint32_t g_releases;
static uint32_t g_owned;
static uint32_t g_pending;
static uint32_t g_bad_lease;
static uint32_t g_bad_observation;
static uint32_t g_partial_observation;
static uint32_t g_attributes;
static uint32_t g_gpi;
static uint32_t g_advertised;
static int g_observation_result;
static int g_release_result;
static int64_t g_feature_status;
static uint64_t g_requested_capacity;
static uint64_t g_first_offset;
static uint32_t g_realm_enabled;
static uint32_t g_realm_releases;

/** @brief Replace the architectural barrier in the native-only SMC fixture. */
void dsb_sy(void)
{
}

/** @brief Isolate the native caller tests from UART logging. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *format, ...)
{
    (void)verbosity;
    (void)tag;
    (void)format;
    return VAL_SUCCESS;
}

/** @brief Supply a native PE identity to the production wrappers. */
uint64_t read_mpidr_el1(void)
{
    return 0ULL;
}

/** @brief Reset native fixtures; none of these observations are firmware evidence. */
static void reset_fixture(void)
{
    g_replies[0] = (native_reply_t){FIRME_INCOMPLETE, 0ULL, 0ULL};
    g_replies[1] = (native_reply_t){FIRME_INCOMPLETE, 1ULL, 4ULL};
    g_replies[2] = (native_reply_t){FIRME_SUCCESS, 4ULL, 0ULL};
    g_reply_count = 3U;
    g_calls = 0U;
    g_acquires = 0U;
    g_releases = 0U;
    g_owned = VAL_FALSE;
    g_pending = VAL_FALSE;
    g_bad_lease = 0U;
    g_bad_observation = 0U;
    g_partial_observation = 0U;
    g_attributes = VAL_FIRME_SHARED_BUFFER_REQUIRED_ATTRS;
    g_gpi = FIRME_GPI_NONSECURE;
    g_advertised = VAL_TRUE;
    g_observation_result = VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE;
    g_release_result = VAL_SUCCESS;
    g_feature_status = FIRME_SUCCESS;
    g_requested_capacity = 0ULL;
    g_first_offset = 0ULL;
    g_realm_enabled = VAL_FALSE;
    g_realm_releases = 0U;
}

/** @brief Model allocation/ownership only; the real adapter still issues PAT SMCs. */
int val_firme_shared_buffer_acquire_ns_pat(uint64_t capacity, uint64_t alignment,
                                          val_firme_shared_buffer_lease_t *lease)
{
    assert(alignment == 4096ULL);
    if ((g_owned == VAL_TRUE) || (capacity > sizeof(g_buffer)))
        return VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE;
    g_acquires++;
    g_calls = 0U;
    g_owned = VAL_TRUE;
    g_requested_capacity = capacity;
    *lease = (val_firme_shared_buffer_lease_t){
        .base_va = g_buffer, .base_pa = g_buffer_pa, .capacity = sizeof(g_buffer),
        .challenge = g_challenge, .challenge_size = sizeof(g_challenge),
        .workload_binding = g_binding, .workload_binding_size = sizeof(g_binding),
    };
    switch (g_bad_lease) {
    case 1U:
        lease->base_va = NULL;
        break;
    case 2U:
        lease->base_pa = UINT64_MAX & ~UINT64_C(4095);
        break;
    case 3U:
        lease->capacity = capacity - 1ULL;
        break;
    case 4U:
        lease->base_pa++;
        break;
    case 5U:
        lease->challenge = g_buffer;
        break;
    case 6U:
        lease->base_va = (uint8_t *)UINTPTR_MAX;
        break;
    case 7U:
        lease->challenge_size = 0U;
        break;
    case 8U:
        lease->challenge = NULL;
        break;
    case 9U:
        lease->challenge = (const uint8_t *)UINTPTR_MAX;
        break;
    default:
        break;
    }
    return VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE;
}

/** @brief Supply independently controlled observation fixtures, never inferred status. */
int val_firme_shared_buffer_observe_ns_pat(
    const val_firme_shared_buffer_lease_t *lease,
    const val_firme_shared_buffer_evidence_t *invocation,
    val_firme_shared_buffer_evidence_t *observation)
{
    assert(g_owned == VAL_TRUE && lease->base_va == g_buffer);
    if ((g_observation_result == VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE) &&
        (g_partial_observation == 0U)) {
        *observation = (val_firme_shared_buffer_evidence_t){0};
        return g_observation_result;
    }

    *observation = (val_firme_shared_buffer_evidence_t){
        .base_pa = invocation->base_pa, .page_count = invocation->page_count,
        .caller_attributes = g_attributes, .el3_attributes = g_attributes,
        .pas_world = FIRME_ACS_WORLD_NSEL2, .gpi = g_gpi,
        .valid_fields = VAL_FIRME_SHARED_BUFFER_EVIDENCE_BASE |
            VAL_FIRME_SHARED_BUFFER_EVIDENCE_PAGE_COUNT |
            VAL_FIRME_SHARED_BUFFER_EVIDENCE_CALLER_ATTRS |
            VAL_FIRME_SHARED_BUFFER_EVIDENCE_EL3_ATTRS |
            VAL_FIRME_SHARED_BUFFER_EVIDENCE_PAS | VAL_FIRME_SHARED_BUFFER_EVIDENCE_GPI,
    };
    if (g_bad_observation == 1U)
        observation->base_pa++;
    if (g_bad_observation == 2U)
        observation->valid_fields &= ~VAL_FIRME_SHARED_BUFFER_EVIDENCE_EL3_ATTRS;
    if (g_bad_observation == 3U)
        observation->pas_world = FIRME_ACS_WORLD_REALM;
    if (g_partial_observation != 0U)
        observation->valid_fields &= g_partial_observation |
            VAL_FIRME_SHARED_BUFFER_EVIDENCE_BASE | VAL_FIRME_SHARED_BUFFER_EVIDENCE_PAGE_COUNT;
    return g_observation_result;
}

/** @brief Require ownership until release; quarantine pending retrievals durably. */
int val_firme_shared_buffer_release_ns_pat(const val_firme_shared_buffer_lease_t *lease,
                                          uint32_t pending)
{
    assert(lease != NULL && g_owned == VAL_TRUE);
    g_releases++;
    g_pending = pending;
    if ((pending == VAL_FALSE) && (g_release_result == VAL_SUCCESS))
        g_owned = VAL_FALSE;
    return g_release_result;
}

/** @brief Model Realm availability separately while sharing native backing storage. */
int val_firme_shared_buffer_acquire_realm_pat(uint64_t capacity, uint64_t alignment,
                                             val_firme_shared_buffer_lease_t *lease)
{
    if (g_realm_enabled != VAL_TRUE)
        return VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE;
    return val_firme_shared_buffer_acquire_ns_pat(capacity, alignment, lease);
}

/** @brief Confirm that the Realm invocation releases the matching world's lease. */
int val_firme_shared_buffer_release_realm_pat(const val_firme_shared_buffer_lease_t *lease,
                                             uint32_t pending)
{
    g_realm_releases++;
    return val_firme_shared_buffer_release_ns_pat(lease, pending);
}

/** @brief Inject discovery outputs into actual production discovery decoders. */
void val_smccc_smc_call(uint64_t fid, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                       uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7,
                       smccc_res_t *output)
{
    assert(arg3 == 0ULL && arg4 == 0ULL && arg5 == 0ULL && arg6 == 0ULL && arg7 == 0ULL);
    *output = (smccc_res_t){.x1 = g_poison, .x2 = g_poison, .x3 = g_poison};
    if (fid == FIRME_FID_SERVICE_VERSION) {
        assert(arg1 == FIRME_SERVICE_ID_ATTESTATION && arg2 == 0ULL);
        output->x0 = g_advertised == VAL_TRUE ? UINT64_C(0x10000) :
                        (uint32_t)FIRME_NOT_SUPPORTED;
    } else {
        assert(fid == FIRME_FID_SERVICE_FEATURES);
        output->x0 = (uint32_t)g_feature_status;
        if (arg1 == FIRME_SERVICE_ID_BASE) {
            assert(arg2 == FIRME_BASE_FEATURE_REG1_INDEX);
            output->x1 = (UINT64_C(7) << FIRME_BASE_MAX_SH_BUF_PG_CNT_SHIFT) |
                          (g_poison & UINT64_C(0xffffffff00000000));
        } else {
            assert(arg1 == FIRME_SERVICE_ID_ATTESTATION && arg2 == 0ULL);
            output->x1 = FIRME_ATTEST_FEATURE_PAT_GET_BIT |
                          (g_poison & ~FIRME_ATTEST_FEATURE_REG0_ALL_ABI_BITS);
        }
    }
    output->x0 |= g_poison & UINT64_C(0xffffffff00000000);
}

/** @brief Check world-specific PAT frames and poison only status-unused outputs. */
void val_smccc_smc_call_frame(const smccc_call_frame_t *input, smccc_call_frame_t *output)
{
    native_reply_t reply = {0};
    uint32_t index = 0U;
    uint64_t displacement = 0ULL;

    assert(g_owned == VAL_TRUE);
    assert(input->registers[0] == FIRME_FID_ATTEST_PAT_GET);
    for (index = 6U; index < VAL_SMCCC_REGISTER_COUNT; index++)
        assert(input->registers[index] == 0ULL);
    assert(input->registers[3] <= FIRME_BASE_MAX_SH_BUF_PG_CNT_MASK);
    assert(input->registers[2] < (input->registers[3] + 1ULL) * 4096ULL);
    if (g_calls == 0U) {
        displacement = input->registers[1] - g_buffer_pa;
        assert(displacement <= 1ULL);
        assert(input->registers[4] == sizeof(g_challenge));
        assert(memcmp(g_buffer + displacement, g_challenge, sizeof(g_challenge)) == 0);
        if (g_realm_enabled == VAL_TRUE) {
            assert(input->registers[5] == sizeof(g_binding));
            assert(memcmp(g_buffer + displacement + VAL_FIRME_PAT_WORKLOAD_BINDING_OFFSET,
                          g_binding, sizeof(g_binding)) == 0);
        } else {
            assert(input->registers[5] == 0ULL);
        }
        g_first_offset = input->registers[2];
    } else {
        assert(input->registers[1] == g_buffer_pa && input->registers[2] == 0ULL);
        assert(input->registers[3] == 0ULL && input->registers[4] == 0ULL);
        assert(input->registers[5] == 0ULL);
    }
    reply = g_replies[g_calls < g_reply_count ? g_calls : g_reply_count - 1U];
    if ((input->registers[1] != g_buffer_pa) || (input->registers[3] > 7ULL))
        reply.status = FIRME_INVALID_PARAMETERS;
    g_calls++;
    for (index = 0U; index < VAL_SMCCC_REGISTER_COUNT; index++)
        output->registers[index] = g_poison;
    output->registers[0] = (uint32_t)reply.status | (g_poison & UINT64_C(0xffffffff00000000));
    if ((reply.status == FIRME_SUCCESS) || (reply.status == FIRME_INCOMPLETE))
        output->registers[1] = reply.written;
    if (reply.status == FIRME_INCOMPLETE)
        output->registers[2] = reply.remaining;
}

/** @brief Run a buffer case through the production adapter and preserve raw evidence. */
static int probe(val_firme_shared_buffer_case_t test_case,
                 val_firme_shared_buffer_evidence_t *evidence)
{
    return val_firme_shared_buffer_probe(FIRME_ACS_WORLD_NSEL2, test_case,
                                         4096ULL, 7ULL, evidence);
}

/** @brief Require consistent rule returns/statuses, including null-result rejection. */
static void check_rule(firme_rule_fn_t body, int expected_return, uint64_t expected_status)
{
    firme_rule_res_t result = {.status = ACS_STATUS_PASS};

    assert(body(0ULL, &result) == expected_return);
    assert(result.status == expected_status);
    assert(body(0ULL, NULL) == VAL_ERROR);
}

/** @brief Regress real driver decoding, ownership, SBZ frames and honest coverage gaps. */
int main(void)
{
    const uint64_t poisons[] = {0ULL, UINT64_MAX, UINT64_C(0xa5a5a5a55a5a5a5a)};
    const int64_t errors[] = {FIRME_NOT_SUPPORTED, FIRME_INVALID_PARAMETERS, FIRME_NO_MEMORY,
                             FIRME_BUSY, FIRME_ABORTED, FIRME_DENIED};
    val_firme_shared_buffer_evidence_t evidence = {0};
    val_firme_partial_retrieval_evidence_t chunk = {0};
    uint32_t index = 0U;
    uint32_t status_index = 0U;

    for (index = 0U; index < sizeof(poisons) / sizeof(poisons[0]); index++) {
        g_poison = poisons[index];
        reset_fixture();
        assert(probe(VAL_FIRME_SHARED_BUFFER_CASE_VALID, &evidence) ==
               VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE);
        assert(evidence.abi_status == FIRME_INCOMPLETE && g_calls == 3U);
        assert(g_owned == VAL_FALSE && g_releases == 1U);
        assert((evidence.valid_fields & VAL_FIRME_SHARED_BUFFER_EVIDENCE_EL3_ATTRS) == 0U);
        assert((evidence.valid_fields & VAL_FIRME_SHARED_BUFFER_OBSERVATION_UNAVAILABLE) != 0U);
        reset_fixture();
        check_rule(firme_base_partial_retrieval_intermediate_chunk_run_ns,
                   VAL_SUCCESS, ACS_STATUS_PASS);
        assert(g_first_offset == 4095ULL && g_calls == 3U && g_pending == VAL_FALSE);
        reset_fixture();
        check_rule(firme_base_partial_retrieval_no_chunk_run_ns, VAL_SUCCESS, ACS_STATUS_PASS);
        reset_fixture();
        check_rule(firme_base_partial_retrieval_last_chunk_run_ns, VAL_SUCCESS, ACS_STATUS_SKIP);
        assert(g_calls == 0U && g_acquires == 0U);
        reset_fixture();
        g_reply_count = 1U;
        g_replies[0] = (native_reply_t){FIRME_SUCCESS, 8ULL, 0ULL};
        assert(probe(VAL_FIRME_SHARED_BUFFER_CASE_VALID, &evidence) ==
               VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE);
        check_rule(firme_base_partial_retrieval_no_chunk_run_ns, VAL_SUCCESS, ACS_STATUS_SKIP);
        for (status_index = 0U; status_index < sizeof(errors) / sizeof(errors[0]); status_index++) {
            reset_fixture();
            g_reply_count = 1U;
            g_replies[0].status = errors[status_index];
            assert(probe(VAL_FIRME_SHARED_BUFFER_CASE_VALID, &evidence) ==
                   VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE);
            assert(evidence.abi_status == errors[status_index] && g_calls == 1U);
            assert(g_pending == VAL_FALSE && g_owned == VAL_FALSE);
            check_rule(firme_base_partial_retrieval_intermediate_chunk_run_ns,
                       VAL_ERROR, ACS_STATUS_FAIL);
            assert(g_owned == VAL_FALSE);
        }
    }

    reset_fixture();
    check_rule(firme_base_shared_buffer_alignment_run_ns, VAL_SUCCESS, ACS_STATUS_PASS);
    assert(g_requested_capacity == 4097ULL);
    reset_fixture();
    check_rule(firme_base_shared_buffer_size_bounds_run_ns, VAL_SUCCESS, ACS_STATUS_PASS);
    assert(g_requested_capacity == 9ULL * 4096ULL && g_releases == 3U);
    reset_fixture();
    check_rule(firme_base_shared_buffer_mapping_attributes_run_ns, VAL_SUCCESS, ACS_STATUS_SKIP);
    check_rule(firme_base_shared_buffer_pas_gpi_run_ns, VAL_SUCCESS, ACS_STATUS_SKIP);
    reset_fixture();
    g_reply_count = 1U;
    g_replies[0] = (native_reply_t){FIRME_INVALID_PARAMETERS, 0ULL, 0ULL};
    check_rule(firme_base_shared_buffer_mapping_attributes_run_ns, VAL_ERROR, ACS_STATUS_FAIL);
    reset_fixture();
    g_reply_count = 1U;
    g_replies[0] = (native_reply_t){FIRME_INVALID_PARAMETERS, 0ULL, 0ULL};
    check_rule(firme_base_shared_buffer_pas_gpi_run_ns, VAL_ERROR, ACS_STATUS_FAIL);
    reset_fixture();
    g_observation_result = VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE;
    check_rule(firme_base_shared_buffer_mapping_attributes_run_ns, VAL_SUCCESS, ACS_STATUS_PASS);
    check_rule(firme_base_shared_buffer_pas_gpi_run_ns, VAL_SUCCESS, ACS_STATUS_PASS);
    g_attributes = 0U;
    check_rule(firme_base_shared_buffer_mapping_attributes_run_ns, VAL_ERROR, ACS_STATUS_FAIL);
    g_gpi = FIRME_GPI_REALM;
    check_rule(firme_base_shared_buffer_pas_gpi_run_ns, VAL_ERROR, ACS_STATUS_FAIL);
    g_bad_observation = 1U;
    check_rule(firme_base_shared_buffer_pas_gpi_run_ns, VAL_ERROR, ACS_STATUS_ERR);
    assert(g_pending == VAL_FALSE && g_calls == 3U);
    g_bad_observation = 2U;
    g_attributes = VAL_FIRME_SHARED_BUFFER_REQUIRED_ATTRS;
    check_rule(firme_base_shared_buffer_mapping_attributes_run_ns, VAL_ERROR, ACS_STATUS_ERR);
    g_gpi = FIRME_GPI_NONSECURE;
    g_bad_observation = 3U;
    check_rule(firme_base_shared_buffer_pas_gpi_run_ns, VAL_ERROR, ACS_STATUS_FAIL);
    reset_fixture();
    g_partial_observation = VAL_FIRME_SHARED_BUFFER_EVIDENCE_CALLER_ATTRS;
    check_rule(firme_base_shared_buffer_mapping_attributes_run_ns, VAL_SUCCESS, ACS_STATUS_SKIP);
    g_attributes = 0U;
    check_rule(firme_base_shared_buffer_mapping_attributes_run_ns, VAL_ERROR, ACS_STATUS_FAIL);
    reset_fixture();
    g_partial_observation = VAL_FIRME_SHARED_BUFFER_EVIDENCE_GPI;
    check_rule(firme_base_shared_buffer_pas_gpi_run_ns, VAL_SUCCESS, ACS_STATUS_SKIP);
    g_gpi = FIRME_GPI_REALM;
    check_rule(firme_base_shared_buffer_pas_gpi_run_ns, VAL_ERROR, ACS_STATUS_FAIL);
    g_bad_observation = 1U;
    check_rule(firme_base_shared_buffer_pas_gpi_run_ns, VAL_ERROR, ACS_STATUS_ERR);
    reset_fixture();
    g_replies[1] = (native_reply_t){FIRME_ABORTED, 0ULL, 0ULL};
    check_rule(firme_base_shared_buffer_alignment_run_ns, VAL_ERROR, ACS_STATUS_FAIL);
    assert(g_calls == 2U && g_owned == VAL_FALSE);
    reset_fixture();
    g_replies[1] = (native_reply_t){FIRME_INVALID_PARAMETERS, 0ULL, 0ULL};
    check_rule(firme_base_shared_buffer_size_bounds_run_ns, VAL_ERROR, ACS_STATUS_FAIL);
    assert(g_calls == 3U && g_owned == VAL_FALSE);
    reset_fixture();
    g_observation_result = VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    check_rule(firme_base_shared_buffer_alignment_run_ns, VAL_ERROR, ACS_STATUS_ERR);
    assert(g_pending == VAL_FALSE && g_calls == 3U);
    reset_fixture();
    g_release_result = VAL_ERROR;
    check_rule(firme_base_shared_buffer_alignment_run_ns, VAL_ERROR, ACS_STATUS_ERR);

    for (index = 1U; index <= 9U; index++) {
        reset_fixture();
        g_bad_lease = index;
        check_rule(firme_base_shared_buffer_alignment_run_ns, VAL_ERROR, ACS_STATUS_ERR);
        assert(g_calls == 0U && g_releases == 1U && g_owned == VAL_FALSE);
    }
    reset_fixture();
    g_advertised = VAL_FALSE;
    check_rule(firme_base_shared_buffer_alignment_run_ns, VAL_SUCCESS, ACS_STATUS_SKIP);
    assert(g_calls == 0U && g_acquires == 0U);
    reset_fixture();
    g_feature_status = FIRME_BUSY;
    assert(probe(VAL_FIRME_SHARED_BUFFER_CASE_VALID, &evidence) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    assert(g_acquires == 0U);
    reset_fixture();
    assert(val_firme_shared_buffer_probe(FIRME_ACS_WORLD_NSEL2,
        VAL_FIRME_SHARED_BUFFER_CASE_ABOVE_MAXIMUM, 4096ULL,
        FIRME_BASE_MAX_SH_BUF_PG_CNT_MASK, &evidence) == VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
    assert(val_firme_shared_buffer_probe(FIRME_ACS_WORLD_NSEL2,
        VAL_FIRME_SHARED_BUFFER_CASE_VALID, 0ULL, UINT64_MAX, &evidence) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    assert(val_firme_shared_buffer_probe(FIRME_ACS_WORLD_REALM,
        VAL_FIRME_SHARED_BUFFER_CASE_VALID, 4096ULL, 7ULL, &evidence) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
    assert(probe(VAL_FIRME_SHARED_BUFFER_CASE_VALID, NULL) == VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    assert(val_firme_partial_retrieval_probe(FIRME_ACS_WORLD_NSEL2,
        VAL_FIRME_PARTIAL_RETRIEVAL_NO_CHUNK, NULL) == VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_ERROR);
    assert(val_firme_partial_retrieval_probe(FIRME_ACS_WORLD_REALM,
        VAL_FIRME_PARTIAL_RETRIEVAL_NO_CHUNK, &chunk) ==
           VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_UNAVAILABLE);
    assert(g_calls == 0U && g_acquires == 0U);

    reset_fixture();
    g_realm_enabled = VAL_TRUE;
    assert(val_firme_shared_buffer_probe(FIRME_ACS_WORLD_REALM,
        VAL_FIRME_SHARED_BUFFER_CASE_VALID, 4096ULL, 7ULL, &evidence) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE);
    assert(g_calls == 3U && g_realm_releases == 1U && g_owned == VAL_FALSE);
    reset_fixture();
    g_realm_enabled = VAL_TRUE;
    g_replies[0] = (native_reply_t){FIRME_INCOMPLETE, 1ULL, 5ULL};
    assert(val_firme_partial_retrieval_probe(FIRME_ACS_WORLD_REALM,
        VAL_FIRME_PARTIAL_RETRIEVAL_INTERMEDIATE_CHUNK, &chunk) ==
           VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_AVAILABLE);
    assert(g_first_offset == 4095ULL && g_calls == 3U && g_realm_releases == 1U);
    assert(g_owned == VAL_FALSE);

    reset_fixture();
    g_realm_enabled = VAL_TRUE;
    check_rule(firme_base_partial_retrieval_intermediate_chunk_run_realm,
               VAL_SUCCESS, ACS_STATUS_PASS);
    assert(g_first_offset == 4095ULL && g_calls == 3U && g_realm_releases == 1U);

    reset_fixture();
    g_replies[0] = (native_reply_t){FIRME_BAD_DATA, 0ULL, 0ULL};
    check_rule(firme_base_shared_buffer_alignment_run_ns, VAL_ERROR, ACS_STATUS_ERR);
    assert(g_pending == VAL_TRUE && g_calls == 1U && g_owned == VAL_TRUE);
    reset_fixture();
    g_replies[0] = (native_reply_t){FIRME_SUCCESS, 0ULL, 0ULL};
    check_rule(firme_base_shared_buffer_alignment_run_ns, VAL_ERROR, ACS_STATUS_FAIL);
    reset_fixture();
    g_replies[0] = (native_reply_t){FIRME_INCOMPLETE, 1ULL, 0ULL};
    check_rule(firme_base_partial_retrieval_intermediate_chunk_run_ns, VAL_ERROR, ACS_STATUS_FAIL);
    assert(g_calls == 3U && g_owned == VAL_FALSE);
    reset_fixture();
    g_replies[0] = (native_reply_t){FIRME_INCOMPLETE, 2ULL, 4ULL};
    check_rule(firme_base_partial_retrieval_intermediate_chunk_run_ns, VAL_ERROR, ACS_STATUS_FAIL);
    reset_fixture();
    g_replies[0] = (native_reply_t){FIRME_INCOMPLETE, 1ULL, UINT64_MAX};
    check_rule(firme_base_partial_retrieval_intermediate_chunk_run_ns, VAL_ERROR, ACS_STATUS_FAIL);
    reset_fixture();
    g_reply_count = 1U;
    check_rule(firme_base_partial_retrieval_no_chunk_run_ns, VAL_ERROR, ACS_STATUS_ERR);
    assert(g_calls == VAL_FIRME_PAT_MAX_INVOCATIONS && g_pending == VAL_TRUE);
    assert(g_owned == VAL_TRUE && g_releases == 1U);
    assert(probe(VAL_FIRME_SHARED_BUFFER_CASE_VALID, &evidence) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
    assert(g_acquires == 1U && g_releases == 1U);
    puts("PASS: native PAT caller/cleanup checks; not firmware compliance evidence");
    return VAL_SUCCESS;
}
