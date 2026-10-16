/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>

#include "val_firme_suite.h"

#include "../reserved_input_registers.c"
#include "../reserved_input_bits.c"
#include "../service_features.c"
#include "../service_id_allocation.c"
#include "../feature_register_index.c"
#include "../specification_version_policy.c"

#define TEST_EXPECTED_WRAPPER_COUNT 12U
#define TEST_FIRST_UNUSED_REGISTER 17U
#define TEST_ADDRESS UINT64_C(0x90000000)
#define TEST_VERSION UINT64_C(0x10000)

static smccc_input_registers_t g_captured;
static val_firme_spec_policy_evidence_t g_policy;
static uint64_t g_versions[FIRME_SERVICE_ID_MAX];
static uint64_t g_features[FIRME_SERVICE_ID_MAX][FIRME_BASE_FEATURE_REG_COUNT];
static uint64_t g_corrupt_fid;
static uint32_t g_capture_enabled;
static uint32_t g_capture_count;
static uint32_t g_unallocated_index;
static uint32_t g_missing_reg1;

/** @brief Supply a stable PE identity to production output-poison infrastructure. */
uint64_t read_mpidr_el1(void)
{
    return 0ULL;
}

/** @brief Suppress expected diagnostics from intentionally noncompliant fixtures. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *format, ...)
{
    (void)verbosity;
    (void)tag;
    (void)format;
    return VAL_SUCCESS;
}

/** @brief Capture production wrapper arguments or answer discovery from the fixture. */
void val_smccc_smc_call(uint64_t fid, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                        uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7,
                        smccc_res_t *result)
{
    g_captured = (smccc_input_registers_t){
        .registers = {fid, arg1, arg2, arg3, arg4, arg5, arg6, arg7}
    };
    *result = (smccc_res_t){0};
    if (g_capture_enabled == VAL_TRUE)
        return;

    result->x0 = (uint32_t)FIRME_NOT_SUPPORTED;
    if (arg1 >= FIRME_SERVICE_ID_MAX)
        return;

    if (fid == FIRME_FID_SERVICE_VERSION) {
        result->x0 = g_versions[arg1];
        return;
    }

    assert(fid == FIRME_FID_SERVICE_FEATURES);
    if ((int32_t)g_versions[arg1] <= 0)
        return;

    if (arg2 < FIRME_BASE_FEATURE_REG_COUNT) {
        if ((arg1 == FIRME_SERVICE_ID_GRANULE_MGMT) &&
            (arg2 == FIRME_BASE_FEATURE_REG1_INDEX) && (g_missing_reg1 == VAL_TRUE))
            return;

        result->x0 = FIRME_SUCCESS;
        result->x1 = g_features[arg1][arg2];
    } else if ((arg1 == FIRME_SERVICE_ID_GRANULE_MGMT) &&
               (arg2 == g_unallocated_index)) {
        result->x0 = FIRME_SUCCESS;
    }
}

/** @brief Include IDE_PROG's ninth input in the same production capture fixture. */
void val_smccc_smc_call_x8(uint64_t fid, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                           uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7,
                           uint64_t arg8, smccc_res_t *result)
{
    assert(g_capture_enabled == VAL_TRUE);
    val_smccc_smc_call(fid, arg1, arg2, arg3, arg4, arg5, arg6, arg7, result);
    g_captured.registers[8] = arg8;
}

/** @brief Require one capture-only session per wrapper invocation. */
int val_smccc_test_input_capture_enable(void)
{
    assert(g_capture_enabled == VAL_FALSE);
    g_capture_enabled = VAL_TRUE;
    g_capture_count++;
    return VAL_SUCCESS;
}

/** @brief Observe the real C wrapper frame and optionally corrupt an unused register. */
int val_smccc_test_input_capture_read(smccc_input_registers_t *input)
{
    assert(g_capture_enabled == VAL_TRUE);
    *input = g_captured;
    if (g_captured.registers[0] == g_corrupt_fid)
        input->registers[TEST_FIRST_UNUSED_REGISTER] = UINT64_MAX;
    return VAL_SUCCESS;
}

/** @brief Check that both successful and failing captures restore ordinary calls. */
void val_smccc_test_input_capture_disable(void)
{
    assert(g_capture_enabled == VAL_TRUE);
    g_capture_enabled = VAL_FALSE;
}

/** @brief Supply reviewed-release metadata substitutes for native policy validation. */
int val_firme_spec_policy_probe(val_firme_spec_policy_evidence_t *evidence)
{
    *evidence = g_policy;
    return VAL_FIRME_SPEC_POLICY_PROBE_AVAILABLE;
}

/** @brief Reset discovery to a valid NS Base/GM instance without unused-index aliases. */
static void test_reset_discovery(void)
{
    uint32_t service_id;

    for (service_id = 0U; service_id < FIRME_SERVICE_ID_MAX; service_id++) {
        g_versions[service_id] = (uint32_t)FIRME_NOT_SUPPORTED;
        g_features[service_id][0] = 0ULL;
        g_features[service_id][1] = 0ULL;
    }

    g_versions[FIRME_SERVICE_ID_BASE] = TEST_VERSION;
    g_versions[FIRME_SERVICE_ID_GRANULE_MGMT] = TEST_VERSION;
    g_features[FIRME_SERVICE_ID_BASE][0] = FIRME_BASE_FEATURE_REG0_MANDATORY_BITS;
    g_features[FIRME_SERVICE_ID_BASE][1] =
        val_firme_base_service_list_bit(FIRME_SERVICE_ID_GRANULE_MGMT);
    g_features[FIRME_SERVICE_ID_GRANULE_MGMT][0] =
        val_firme_service_mandatory_abi_mask(FIRME_ACS_WORLD_NSEL2,
                                            FIRME_SERVICE_ID_GRANULE_MGMT);
    g_missing_reg1 = VAL_FALSE;
    g_unallocated_index = UINT32_MAX;
}

/** @brief Exercise every wrapper and inject violations into all four previously omitted ABIs. */
static void test_input_coverage(void)
{
    const firme_rule_fn_t bodies[] = {
        firme_base_reserved_input_registers_run_ns,
        firme_base_reserved_input_bits_run_ns
    };
    const uint64_t gpt_fids[] = {
        FIRME_FID_GM_GPI_OP_CONTINUE, FIRME_FID_GM_L1_GPT_CREATE,
        FIRME_FID_GM_L1_GPT_DESTROY, FIRME_FID_GM_GPT_INFO
    };
    firme_rule_res_t result = {0};
    uint32_t body_index;
    uint32_t fid_index;

    for (body_index = 0U; body_index < sizeof(bodies) / sizeof(bodies[0]); body_index++) {
        g_corrupt_fid = 0ULL;
        g_capture_count = 0U;
        assert(bodies[body_index](TEST_ADDRESS, &result) == VAL_SUCCESS);
        assert(result.status == ACS_STATUS_PASS);
        assert(g_capture_count == TEST_EXPECTED_WRAPPER_COUNT);
        assert(g_capture_enabled == VAL_FALSE);
        assert(bodies[body_index](TEST_ADDRESS, NULL) == VAL_ERROR);

        for (fid_index = 0U; fid_index < sizeof(gpt_fids) / sizeof(gpt_fids[0]); fid_index++) {
            g_corrupt_fid = gpt_fids[fid_index];
            assert(bodies[body_index](TEST_ADDRESS, &result) == VAL_ERROR);
            assert(result.status == ACS_STATUS_FAIL);
            assert(result.fail_step == RESERVED_INPUT_REGS_STEP_GM_CONTINUE + fid_index);
            assert(g_capture_enabled == VAL_FALSE);
        }
    }
}

/** @brief Catch version-zero, missing register-1 and high-index discovery false passes. */
static void test_service_coverage(void)
{
    firme_rule_res_t result = {0};
    firme_res_t response = {0};
    uint64_t features = UINT64_MAX;
    uint32_t advertised = VAL_TRUE;

    test_reset_discovery();
    assert(service_features_run(FIRME_ACS_WORLD_NSEL2, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    assert(feature_register_index_run(&result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_SKIP);
    g_unallocated_index = 200U;
    assert(service_features_run(FIRME_ACS_WORLD_NSEL2, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    test_reset_discovery();
    g_missing_reg1 = VAL_TRUE;
    assert(service_features_run(FIRME_ACS_WORLD_NSEL2, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);

    test_reset_discovery();
    assert(service_id_allocation_run(FIRME_ACS_WORLD_NSEL2, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_SKIP);
    g_versions[FIRME_SERVICE_ID_GRANULE_MGMT] = 0ULL;
    assert(service_id_allocation_run(FIRME_ACS_WORLD_NSEL2, &result) == VAL_ERROR);
    test_reset_discovery();
    g_features[FIRME_SERVICE_ID_GRANULE_MGMT][0] &= ~FIRME_GM_FEATURE_REG0_GPI_SET_BIT;
    assert(service_id_allocation_run(FIRME_ACS_WORLD_NSEL2, &result) == VAL_ERROR);
    assert(result.fail_step == SERVICE_ID_ALLOCATION_STEP_ALLOCATED_ABIS);
    test_reset_discovery();
    assert(val_firme_service_get_version(FIRME_SERVICE_ID_8BIT_MAX + 1U,
                                         &advertised, &response) == VAL_ERROR);
    assert(advertised == VAL_FALSE);
    assert(val_firme_service_abi_is_advertised(FIRME_SERVICE_ID_BASE, 0ULL,
                                              &advertised, &features, &response) == VAL_ERROR);
    assert(advertised == VAL_FALSE && features == 0ULL);
}

/** @brief Reject empty or malformed policy records rather than passing an empty comparison. */
static void test_policy_coverage(void)
{
    firme_rule_res_t result = {0};

    g_policy = (val_firme_spec_policy_evidence_t){0};
    assert(specification_version_policy_run(&result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_ERR);
    g_policy.previous = (val_firme_spec_policy_release_t){
        .major_version = 1U, .service_count = 1U,
        .services = {
            {.service_id = FIRME_SERVICE_ID_BASE, .major_version = 1U}
        }
    };
    g_policy.current = g_policy.previous;
    assert(specification_version_policy_run(&result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    g_policy.current.services[0].minor_version = 1U;
    assert(specification_version_policy_run(&result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    g_policy.current.minor_version = 1U;
    assert(specification_version_policy_run(&result) == VAL_SUCCESS);
    g_policy.current.services[0].minor_version = UINT32_MAX;
    assert(specification_version_policy_run(&result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_ERR);
}

/** @brief Validate Base caller coverage without claiming any native fixture is DUT evidence. */
int main(void)
{
    test_input_coverage();
    test_service_coverage();
    test_policy_coverage();
    puts("Base caller/discovery/policy regressions: PASS");
    return VAL_SUCCESS;
}
