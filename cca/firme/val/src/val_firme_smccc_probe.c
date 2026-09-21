/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_smccc_probe.h"
#include "val_firme_smccc_probe_private.h"
#include "val_firme_abi.h"
#include "aarch64/sysreg.h"
#include "val_log.h"
#include <stddef.h>

#define VAL_SMCCC_SMC64_BIT (UINT64_C(1) << 30)
#define VAL_SMCCC_STATUS_BIT(status) (UINT32_C(1) << (uint32_t)(-(status)))
#define VAL_SMCCC_STATUS_SUCCESS VAL_SMCCC_STATUS_BIT(FIRME_SUCCESS)
#define VAL_SMCCC_STATUS_INVALID VAL_SMCCC_STATUS_BIT(FIRME_INVALID_PARAMETERS)
#define VAL_SMCCC_STATUS_ABORTED VAL_SMCCC_STATUS_BIT(FIRME_ABORTED)
#define VAL_SMCCC_STATUS_INCOMPLETE VAL_SMCCC_STATUS_BIT(FIRME_INCOMPLETE)
#define VAL_SMCCC_STATUS_DENIED VAL_SMCCC_STATUS_BIT(FIRME_DENIED)
#define VAL_SMCCC_STATUS_BUSY VAL_SMCCC_STATUS_BIT(FIRME_BUSY)
#define VAL_SMCCC_STATUS_CONFLICT VAL_SMCCC_STATUS_BIT(FIRME_OP_CONFLICT)
#define VAL_SMCCC_STATUS_EXISTS VAL_SMCCC_STATUS_BIT(FIRME_ALREADY_EXISTS)
#define VAL_SMCCC_STATUS_NOT_FOUND VAL_SMCCC_STATUS_BIT(FIRME_NOT_FOUND)
#define VAL_SMCCC_STATUS_NO_MEMORY VAL_SMCCC_STATUS_BIT(FIRME_NO_MEMORY)
#define VAL_SMCCC_STATUS_BAD_DATA VAL_SMCCC_STATUS_BIT(FIRME_BAD_DATA)
#define VAL_SMCCC_IDEV_OUTPUT_MASK ((uint32_t)UINT32_C(1) << 4)

#define VAL_SMCCC_STATUS_GPI \
    (VAL_SMCCC_STATUS_SUCCESS | VAL_SMCCC_STATUS_INVALID | \
     VAL_SMCCC_STATUS_INCOMPLETE | VAL_SMCCC_STATUS_DENIED | \
     VAL_SMCCC_STATUS_BUSY | VAL_SMCCC_STATUS_CONFLICT | \
     VAL_SMCCC_STATUS_NOT_FOUND)
#define VAL_SMCCC_STATUS_L1_CREATE \
    (VAL_SMCCC_STATUS_GPI | VAL_SMCCC_STATUS_EXISTS | VAL_SMCCC_STATUS_NO_MEMORY)
#define VAL_SMCCC_STATUS_IDE_PROG \
    (VAL_SMCCC_STATUS_SUCCESS | VAL_SMCCC_STATUS_INVALID | \
     VAL_SMCCC_STATUS_INCOMPLETE | VAL_SMCCC_STATUS_BUSY | \
     VAL_SMCCC_STATUS_CONFLICT)
#define VAL_SMCCC_STATUS_IDE_CONTROL \
    (VAL_SMCCC_STATUS_IDE_PROG | VAL_SMCCC_STATUS_DENIED)
#define VAL_SMCCC_STATUS_IDE_POLL \
    (VAL_SMCCC_STATUS_SUCCESS | VAL_SMCCC_STATUS_INVALID | \
     VAL_SMCCC_STATUS_ABORTED | VAL_SMCCC_STATUS_INCOMPLETE | \
     VAL_SMCCC_STATUS_DENIED | VAL_SMCCC_STATUS_BUSY)
#define VAL_SMCCC_STATUS_ATTEST_GET \
    (VAL_SMCCC_STATUS_SUCCESS | VAL_SMCCC_STATUS_INVALID | \
     VAL_SMCCC_STATUS_ABORTED | VAL_SMCCC_STATUS_INCOMPLETE | \
     VAL_SMCCC_STATUS_BUSY)
#define VAL_SMCCC_STATUS_IDEV \
    (VAL_SMCCC_STATUS_SUCCESS | VAL_SMCCC_STATUS_INVALID | \
     VAL_SMCCC_STATUS_INCOMPLETE | VAL_SMCCC_STATUS_DENIED | \
     VAL_SMCCC_STATUS_BUSY | VAL_SMCCC_STATUS_NO_MEMORY)

_Static_assert(offsetof(val_smccc_probe_context_t, registers) == 24,
               "Probe register offset");
_Static_assert(offsetof(val_smccc_probe_context_t, vector_length) == 272,
               "Probe VL offset");
_Static_assert(offsetof(val_smccc_probe_context_t, fault_esr) == 280,
               "Probe ESR offset");
_Static_assert(offsetof(val_smccc_probe_context_t, streaming_length) == 288,
               "Probe SVL offset");
_Static_assert(offsetof(val_smccc_probe_context_t, returned_svcr) == 296,
               "Probe SVCR offset");
_Static_assert(offsetof(val_smccc_probe_context_t, returned_length) == 304,
               "Probe return VL offset");
_Static_assert(offsetof(val_smccc_probe_context_t, arguments) == 320,
               "Probe argument offset");

typedef struct {
    const char *name;
    uint32_t service_id;
    uint64_t feature_bit;
    uint64_t fid;
    uint32_t allowed_statuses;
    uint32_t output_register_mask;
} val_smccc_abi_descriptor_t;

static const val_smccc_abi_descriptor_t g_smccc_abis[] = {
    {"FIRME_SERVICE_VERSION", FIRME_SERVICE_ID_BASE,
     FIRME_BASE_FEATURE_REG_VERSION_BIT, FIRME_FID_SERVICE_VERSION, 0U, 0U},
    {"FIRME_SERVICE_FEATURES", FIRME_SERVICE_ID_BASE,
     FIRME_BASE_FEATURE_REG_FEATURES_BIT, FIRME_FID_SERVICE_FEATURES,
     VAL_SMCCC_STATUS_SUCCESS, 0U},
    {"FIRME_GM_GPI_SET", FIRME_SERVICE_ID_GRANULE_MGMT,
     FIRME_GM_FEATURE_GPI_SET_BIT, FIRME_FID_GM_GPI_SET,
     VAL_SMCCC_STATUS_GPI, 0U},
    {"FIRME_GM_GPI_OP_CONTINUE", FIRME_SERVICE_ID_GRANULE_MGMT,
     FIRME_GM_FEATURE_GPI_CONTINUE_BIT, FIRME_FID_GM_GPI_OP_CONTINUE,
     VAL_SMCCC_STATUS_GPI, 0U},
    {"FIRME_GM_L1_GPT_CREATE", FIRME_SERVICE_ID_GRANULE_MGMT,
     FIRME_GM_FEATURE_L1_CREATE_BIT, FIRME_FID_GM_L1_GPT_CREATE,
     VAL_SMCCC_STATUS_L1_CREATE, 0U},
    {"FIRME_GM_L1_GPT_DESTROY", FIRME_SERVICE_ID_GRANULE_MGMT,
     FIRME_GM_FEATURE_L1_DESTROY_BIT, FIRME_FID_GM_L1_GPT_DESTROY,
     VAL_SMCCC_STATUS_GPI, 0U},
    {"FIRME_IDE_KEYSET_PROG", FIRME_SERVICE_ID_IDE_KEY_MGMT,
     FIRME_IDE_FEATURE_KEYSET_PROG_BIT, FIRME_FID_IDE_KEYSET_PROG,
     VAL_SMCCC_STATUS_IDE_PROG, 0U},
    {"FIRME_IDE_KEYSET_GO", FIRME_SERVICE_ID_IDE_KEY_MGMT,
     FIRME_IDE_FEATURE_KEYSET_GO_BIT, FIRME_FID_IDE_KEYSET_GO,
     VAL_SMCCC_STATUS_IDE_CONTROL, 0U},
    {"FIRME_IDE_KEYSET_STOP", FIRME_SERVICE_ID_IDE_KEY_MGMT,
     FIRME_IDE_FEATURE_KEYSET_STOP_BIT, FIRME_FID_IDE_KEYSET_STOP,
     VAL_SMCCC_STATUS_IDE_CONTROL, 0U},
    {"FIRME_IDE_KEYSET_POLL", FIRME_SERVICE_ID_IDE_KEY_MGMT,
     FIRME_IDE_FEATURE_KEYSET_POLL_BIT, FIRME_FID_IDE_KEYSET_POLL,
     VAL_SMCCC_STATUS_IDE_POLL, 0U},
    {"FIRME_MEC_REFRESH", FIRME_SERVICE_ID_MECID_MGMT,
     FIRME_MECID_FEATURE_REG0_MEC_REFRESH_BIT, FIRME_FID_MEC_REFRESH,
     VAL_SMCCC_STATUS_SUCCESS | VAL_SMCCC_STATUS_INVALID | VAL_SMCCC_STATUS_BUSY, 0U},
    {"FIRME_ATTEST_PAT_GET", FIRME_SERVICE_ID_ATTESTATION,
     FIRME_ATTEST_FEATURE_PAT_GET_BIT, FIRME_FID_ATTEST_PAT_GET,
     VAL_SMCCC_STATUS_ATTEST_GET, 0U},
    {"FIRME_ATTEST_RAK_GET", FIRME_SERVICE_ID_ATTESTATION,
     FIRME_ATTEST_FEATURE_RAK_GET_BIT, FIRME_FID_ATTEST_RAK_GET,
     VAL_SMCCC_STATUS_ATTEST_GET, 0U},
    {"FIRME_ATTEST_RAT_SIGN", FIRME_SERVICE_ID_ATTESTATION,
     FIRME_ATTEST_FEATURE_RAT_SIGN_BIT, FIRME_FID_ATTEST_RAT_SIGN,
     VAL_SMCCC_STATUS_SUCCESS | VAL_SMCCC_STATUS_INVALID |
     VAL_SMCCC_STATUS_ABORTED | VAL_SMCCC_STATUS_DENIED | VAL_SMCCC_STATUS_BUSY, 0U},
    {"FIRME_ATTEST_PAT_EXT_CLAIMS_STAGE", FIRME_SERVICE_ID_ATTESTATION,
     FIRME_ATTEST_FEATURE_EXT_STAGE_BIT, FIRME_FID_ATTEST_PAT_EXT_CLAIMS_STAGE,
     VAL_SMCCC_STATUS_SUCCESS | VAL_SMCCC_STATUS_INVALID |
     VAL_SMCCC_STATUS_ABORTED | VAL_SMCCC_STATUS_BUSY, 0U},
    {"FIRME_ATTEST_PAT_EXT_CLAIMS_FINALISE", FIRME_SERVICE_ID_ATTESTATION,
     FIRME_ATTEST_FEATURE_EXT_FINAL_BIT, FIRME_FID_ATTEST_PAT_EXT_FINALISE,
     VAL_SMCCC_STATUS_SUCCESS | VAL_SMCCC_STATUS_ABORTED |
     VAL_SMCCC_STATUS_BUSY | VAL_SMCCC_STATUS_BAD_DATA, 0U},
    {"FIRME_ATTEST_PAT_EXT_CLAIMS_CLEAR", FIRME_SERVICE_ID_ATTESTATION,
     FIRME_ATTEST_FEATURE_EXT_CLEAR_BIT, FIRME_FID_ATTEST_PAT_EXT_CLAIMS_CLEAR,
     VAL_SMCCC_STATUS_SUCCESS | VAL_SMCCC_STATUS_ABORTED | VAL_SMCCC_STATUS_BUSY, 0U},
    {"FIRME_IDEV_OP_START", FIRME_SERVICE_ID_INTEGRATED_DEVICE,
     FIRME_IDEV_FEATURE_OP_START_BIT, FIRME_FID_IDEV_OP_START,
     VAL_SMCCC_STATUS_IDEV, VAL_SMCCC_IDEV_OUTPUT_MASK},
    {"FIRME_IDEV_OP_CONTINUE", FIRME_SERVICE_ID_INTEGRATED_DEVICE,
     FIRME_IDEV_FEATURE_OP_CONTINUE_BIT, FIRME_FID_IDEV_OP_CONTINUE,
     VAL_SMCCC_STATUS_IDEV, VAL_SMCCC_IDEV_OUTPUT_MASK},
};

static _Alignas(16) unsigned char g_smccc_saved[VAL_SMCCC_VECTOR_BYTES];
static _Alignas(16) unsigned char g_smccc_returned[VAL_SMCCC_VECTOR_BYTES];
static _Alignas(16) unsigned char g_smccc_seed[VAL_SMCCC_VECTOR_BYTES];

/**
 * @brief Store a 64-bit little-endian value without violating strict aliasing.
 * @param buffer Destination buffer.
 * @param offset Byte offset.
 * @param value Value to store.
 * @return None.
 */
static void val_smccc_store_u64(unsigned char *buffer, size_t offset, uint64_t value)
{
    size_t index;

    for (index = 0; index < sizeof(value); index++)
        buffer[offset + index] = (unsigned char)(value >> (index * 8U));
}

/**
 * @brief Compare a live component, or accept zero for hint-discardable state.
 * @param offset Buffer offset.
 * @param length Component size.
 * @param allow_zero Whether SMCCC permits the state to be discarded.
 * @return VAL_TRUE for valid preservation, otherwise VAL_FALSE.
 */
static int val_smccc_component_matches(size_t offset, size_t length, int allow_zero)
{
    size_t index;
    int identical = VAL_TRUE;
    int zero = VAL_TRUE;

    for (index = offset; index < offset + length; index++) {
        if (g_smccc_returned[index] != g_smccc_seed[index])
            identical = VAL_FALSE;
        if (g_smccc_returned[index] != 0)
            zero = VAL_FALSE;
    }

    return identical || (allow_zero && zero);
}

/**
 * @brief Report the exact state component corrupted across a FIRME call.
 * @param out Result record.
 * @param fid Invoked ABI function identifier.
 * @param component Register or buffer component identifier.
 * @param actual Observed value.
 * @param expected Required value.
 * @return VAL_ERROR.
 */
static int val_smccc_probe_fail(firme_rule_res_t *out, uint64_t fid, uint64_t component,
                                uint64_t actual, uint64_t expected)
{
    LOG_ERROR(NULL, "SMCCC fid=%llx component=%llu actual=%llx expected=%llx",
              (unsigned long long)fid, (unsigned long long)component,
              (unsigned long long)actual, (unsigned long long)expected);
    if (out) {
        out->status = ACS_STATUS_FAIL;
        out->fail_step = component;
        out->last_x0 = actual;
        out->last_x1 = expected;
    }

    return VAL_ERROR;
}

/**
 * @brief Populate side-effect-safe arguments for one advertised ABI probe.
 * @param descriptor ABI descriptor.
 * @param context Probe context whose x1-x17 arguments are populated.
 * @return None.
 */
static void val_smccc_prepare_arguments(const val_smccc_abi_descriptor_t *descriptor,
                                        val_smccc_probe_context_t *context)
{
    size_t index;

    for (index = 0; index < 17U; index++)
        context->arguments[index] = 0ULL;

    if (descriptor->fid == FIRME_FID_SERVICE_VERSION) {
        context->arguments[0] = descriptor->service_id;
    } else if (descriptor->fid == FIRME_FID_SERVICE_FEATURES) {
        context->arguments[0] = descriptor->service_id;
        context->arguments[1] = FIRME_BASE_FEATURE_REG0_INDEX;
    } else if (descriptor->fid == FIRME_FID_GM_GPI_SET) {
        context->arguments[0] = 1ULL;
        context->arguments[1] = 1ULL;
    } else if (descriptor->fid == FIRME_FID_GM_L1_GPT_CREATE) {
        context->arguments[0] = 1ULL;
        context->arguments[1] = 1ULL;
    } else if ((descriptor->fid == FIRME_FID_GM_GPI_OP_CONTINUE) ||
               (descriptor->fid == FIRME_FID_GM_L1_GPT_DESTROY)) {
        context->arguments[0] = 1ULL;
    } else if (descriptor->service_id == FIRME_SERVICE_ID_IDE_KEY_MGMT) {
        context->arguments[0] = 1ULL;
        if (descriptor->fid == FIRME_FID_IDE_KEYSET_PROG) {
            context->arguments[3] = UINT64_C(0x1111111111111111);
            context->arguments[4] = UINT64_C(0x2222222222222222);
            context->arguments[5] = UINT64_C(0x3333333333333333);
            context->arguments[6] = UINT64_C(0x4444444444444444);
            context->arguments[7] = UINT64_C(0x5555555555555555);
        }
    } else if (descriptor->fid == FIRME_FID_MEC_REFRESH) {
        context->arguments[0] = UINT64_C(0xFFFF) << FIRME_MEC_REFRESH_MECID_SHIFT;
    } else if ((descriptor->fid == FIRME_FID_ATTEST_PAT_GET) ||
               (descriptor->fid == FIRME_FID_ATTEST_RAK_GET) ||
               (descriptor->fid == FIRME_FID_ATTEST_RAT_SIGN) ||
               (descriptor->fid == FIRME_FID_ATTEST_PAT_EXT_CLAIMS_STAGE)) {
        context->arguments[0] = 1ULL;
    } else if (descriptor->fid == FIRME_FID_IDEV_OP_START) {
        context->arguments[2] = 1ULL;
    } else if (descriptor->fid == FIRME_FID_IDEV_OP_CONTINUE) {
        context->arguments[1] = 1ULL;
        context->arguments[2] = 1ULL;
    }
}

/**
 * @brief Validate one advertised ABI's architected return value.
 * @param descriptor ABI descriptor.
 * @param context Captured result registers.
 * @param out Optional result record.
 * @return VAL_SUCCESS for a legal result, otherwise VAL_ERROR.
 */
static int val_smccc_check_result(const val_smccc_abi_descriptor_t *descriptor,
                                  const val_smccc_probe_context_t *context,
                                  firme_rule_res_t *out)
{
    uint32_t status_bit;
    int64_t status;

    status = val_firme_status_to_i64(context->registers[0]);
    if (descriptor->fid == FIRME_FID_SERVICE_VERSION) {
        if (status > 0)
            return VAL_SUCCESS;
        return val_smccc_probe_fail(out, descriptor->fid, 101,
                                    context->registers[0], 1U);
    }

    if ((status > FIRME_SUCCESS) || (status < FIRME_BAD_DATA) ||
        (val_firme_status_is_architected(status) != VAL_TRUE)) {
        return val_smccc_probe_fail(out, descriptor->fid, 101,
                                    context->registers[0], 0U);
    }

    status_bit = VAL_SMCCC_STATUS_BIT(status);
    if ((descriptor->allowed_statuses & status_bit) == 0U) {
        return val_smccc_probe_fail(out, descriptor->fid, 101,
                                    context->registers[0],
                                    descriptor->allowed_statuses);
    }

    return VAL_SUCCESS;
}

/**
 * @brief Exercise one advertised ABI with controlled architectural state.
 * @param descriptor ABI descriptor.
 * @param modes SVE/FFR/ZA/streaming/hint selection.
 * @param out Optional result record.
 * @return VAL_SUCCESS after comparison, otherwise VAL_ERROR.
 */
static int val_smccc_check_abi(const val_smccc_abi_descriptor_t *descriptor,
                               uint64_t modes, firme_rule_res_t *out)
{
    val_smccc_probe_context_t context = {
        .saved = g_smccc_saved, .returned = g_smccc_returned, .seed = g_smccc_seed
    };
    size_t index;
    size_t offset;
    size_t vector_length;
    uint64_t expected;
    uint64_t state_modes = modes;
    int hint = (modes & VAL_SMCCC_PROBE_HINT) != 0;

    if (((read_id_aa64pfr0_el1() >> 32) & 0xFU) != 0U)
        modes |= VAL_SMCCC_STATE_SVE | VAL_SMCCC_STATE_FFR;
    if ((modes & VAL_SMCCC_PROBE_STREAMING) != 0U)
        state_modes = (modes | VAL_SMCCC_STATE_SVE) & ~VAL_SMCCC_STATE_FFR;
    else
        state_modes = modes;

    for (index = 0; index < VAL_SMCCC_VECTOR_BYTES; index++) {
        g_smccc_seed[index] =
            (unsigned char)(1U + ((index * 17U + (index / 256U) * 29U) % 251U));
        g_smccc_returned[index] = 0U;
    }

    val_smccc_store_u64(g_smccc_seed, VAL_SMCCC_FPCR_OFFSET, UINT64_C(1) << 24);
    val_smccc_store_u64(g_smccc_seed, VAL_SMCCC_FPSR_OFFSET, UINT64_C(1));
    for (index = 0; index < 32U; index++)
        g_smccc_seed[VAL_SMCCC_FFR_OFFSET + index] = 0xFFU;

    val_smccc_prepare_arguments(descriptor, &context);
    if (val_smccc_probe_state(descriptor->fid, modes, &context) != VAL_SUCCESS)
        return val_smccc_probe_fail(out, descriptor->fid, 100, context.fault_esr, 0U);

    if (val_smccc_check_result(descriptor, &context, out) != VAL_SUCCESS)
        return VAL_ERROR;

    if (context.returned_length != context.vector_length) {
        return val_smccc_probe_fail(out, descriptor->fid, 103,
                                    context.returned_length, context.vector_length);
    }

    if ((modes & VAL_SMCCC_STATE_ZA) != 0U) {
        expected = (modes & VAL_SMCCC_PROBE_STREAMING) != 0U ? 3U : 2U;
        if (context.returned_svcr != expected) {
            return val_smccc_probe_fail(out, descriptor->fid, 104,
                                        context.returned_svcr, expected);
        }
    }

    for (index = 4U; index < 18U; index++) {
        if ((descriptor->output_register_mask & (UINT32_C(1) << index)) != 0U)
            continue;
        expected = context.arguments[index - 1U];
        if (context.registers[index] != expected) {
            return val_smccc_probe_fail(out, descriptor->fid, index,
                                        context.registers[index], expected);
        }
    }

    for (index = 18U; index < 31U; index++) {
        if (context.registers[index] != index) {
            return val_smccc_probe_fail(out, descriptor->fid, index,
                                        context.registers[index], index);
        }
    }

    vector_length = context.vector_length;
    if ((vector_length < 16U) || (vector_length > 256U) ||
        ((vector_length % 16U) != 0U)) {
        return val_smccc_probe_fail(out, descriptor->fid, 102, vector_length, 256U);
    }

    for (index = 0; index < 32U; index++) {
        offset = index * 256U;
        if (!val_smccc_component_matches(offset, 16U, VAL_FALSE) ||
            !val_smccc_component_matches(offset + 16U, vector_length - 16U, hint)) {
            return val_smccc_probe_fail(out, descriptor->fid, 200U + index,
                                        modes, vector_length);
        }
    }

    if ((state_modes & VAL_SMCCC_STATE_SVE) != 0U) {
        for (index = 0; index < 16U; index++) {
            offset = VAL_SMCCC_PREDICATE_OFFSET + index * 32U;
            if (!val_smccc_component_matches(offset, vector_length / 8U, hint)) {
                return val_smccc_probe_fail(out, descriptor->fid, 300U + index,
                                            modes, vector_length);
            }
        }
    }

    if (((state_modes & VAL_SMCCC_STATE_FFR) != 0U) &&
        !val_smccc_component_matches(VAL_SMCCC_FFR_OFFSET,
                                     vector_length / 8U, hint)) {
        return val_smccc_probe_fail(out, descriptor->fid, 400, modes, vector_length);
    }

    if (!val_smccc_component_matches(VAL_SMCCC_FPCR_OFFSET, 16U, VAL_FALSE))
        return val_smccc_probe_fail(out, descriptor->fid, 401, modes, 0U);

    if ((modes & VAL_SMCCC_STATE_ZA) != 0U) {
        vector_length = context.streaming_length;
        if ((vector_length < 16U) || (vector_length > 256U) ||
            ((vector_length % 16U) != 0U)) {
            return val_smccc_probe_fail(out, descriptor->fid, 402,
                                        vector_length, 256U);
        }
        if (!val_smccc_component_matches(VAL_SMCCC_ZA_OFFSET,
                                         vector_length * vector_length, VAL_FALSE)) {
            return val_smccc_probe_fail(out, descriptor->fid, 403,
                                        modes, vector_length);
        }
    }

    return VAL_SUCCESS;
}

/**
 * @brief Exercise every ABI advertised for one FIRME service.
 * @param service_id Service ID.
 * @param modes State and SMCCC hint selection.
 * @param out Optional result record.
 * @param allow_absent Whether an unavailable service is acceptable.
 * @return VAL_SUCCESS when all advertised ABIs comply, otherwise VAL_ERROR.
 */
static int val_smccc_check_service_internal(uint32_t service_id, uint64_t modes,
                                            firme_rule_res_t *out, int allow_absent)
{
    firme_res_t result = {0};
    uint64_t feature_bits = 0ULL;
    size_t index;
    int64_t version;

    val_firme_service_version(service_id, &result);
    version = val_firme_status_to_i64(result.x0);
    if (version == FIRME_NOT_SUPPORTED)
        return allow_absent ? VAL_SUCCESS : VAL_ERROR;
    if (version <= 0)
        return val_smccc_probe_fail(out, FIRME_FID_SERVICE_VERSION, 500,
                                    result.x0, service_id);

    val_firme_service_features(service_id, FIRME_BASE_FEATURE_REG0_INDEX, &result);
    if (val_firme_status_to_i64(result.x0) != FIRME_SUCCESS) {
        return val_smccc_probe_fail(out, FIRME_FID_SERVICE_FEATURES, 501,
                                    result.x0, service_id);
    }
    feature_bits = result.x1;
    if (service_id == FIRME_SERVICE_ID_BASE)
        feature_bits |= FIRME_BASE_FEATURE_REG0_MANDATORY_BITS;

    for (index = 0; index < sizeof(g_smccc_abis) / sizeof(g_smccc_abis[0]); index++) {
        if ((g_smccc_abis[index].service_id != service_id) ||
            ((feature_bits & g_smccc_abis[index].feature_bit) == 0ULL)) {
            continue;
        }

        if (val_smccc_check_abi(&g_smccc_abis[index], modes, out) != VAL_SUCCESS)
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/**
 * @brief Check register and vector state across every advertised FIRME ABI.
 * @param modes State and SMCCC hint selection.
 * @param out Optional result record populated on failure.
 * @return VAL_SUCCESS when state is compliant, otherwise VAL_ERROR.
 */
int val_firme_smccc_check_state(uint64_t modes, firme_rule_res_t *out)
{
    uint32_t service_id;

    for (service_id = FIRME_SERVICE_ID_BASE; service_id < FIRME_SERVICE_ID_MAX;
         service_id++) {
        if (val_smccc_check_service_internal(service_id, modes, out,
                                             VAL_TRUE) != VAL_SUCCESS) {
            return VAL_ERROR;
        }
    }

    LOG_TEST(NULL, "SMCCC state checked across advertised FIRME ABIs: modes=%llx",
             (unsigned long long)modes);
    return VAL_SUCCESS;
}

/**
 * @brief Check register and vector state for one advertised FIRME service.
 * @param service_id Service whose advertised ABIs are exercised.
 * @param modes State and SMCCC hint selection.
 * @param out Optional result record populated on failure.
 * @return VAL_SUCCESS when state is compliant, otherwise VAL_ERROR.
 */
int val_firme_smccc_check_service(uint32_t service_id, uint64_t modes,
                                  firme_rule_res_t *out)
{
    if (service_id >= FIRME_SERVICE_ID_MAX)
        return VAL_ERROR;

    return val_smccc_check_service_internal(service_id, modes, out, VAL_FALSE);
}

/**
 * @brief Check that every ALP4 FIRME ABI uses an SMC64 function identifier.
 * @param out Optional result record populated on failure.
 * @return VAL_SUCCESS when every identifier is SMC64, otherwise VAL_ERROR.
 */
int val_firme_smccc_check_fid_encodings(firme_rule_res_t *out)
{
    size_t index;

    for (index = 0; index < sizeof(g_smccc_abis) / sizeof(g_smccc_abis[0]); index++) {
        if ((g_smccc_abis[index].fid & VAL_SMCCC_SMC64_BIT) == 0ULL) {
            LOG_ERROR(NULL, "%s uses a non-SMC64 FID 0x%llx",
                      g_smccc_abis[index].name,
                      (unsigned long long)g_smccc_abis[index].fid);
            return val_smccc_probe_fail(out, g_smccc_abis[index].fid,
                                        502, g_smccc_abis[index].fid,
                                        VAL_SMCCC_SMC64_BIT);
        }
    }

    return VAL_SUCCESS;
}
