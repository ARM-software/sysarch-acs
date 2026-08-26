/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>

#include "../granule_management_feature_registers.c"
#include "../granule_management_abi_compliance.c"

#define TEST_UNUSED (UINT64_C(1) << 63)

static uint64_t g_reg0;
static uint64_t g_reg1;
static int64_t g_version;
static int64_t g_feature_status;

/** @brief Decode the signed W0 status from the mock transport. */
int64_t val_firme_status_to_i64(uint64_t value)
{
    return (int64_t)(int32_t)(uint32_t)value;
}

/** @brief Advertise the configured GM service version. */
void val_firme_service_version(uint64_t service, firme_res_t *res)
{
    assert(service == FIRME_SERVICE_ID_GRANULE_MGMT);
    *res = (firme_res_t){.x0 = (uint64_t)g_version};
}

/** @brief Supply feature registers with independently poisoned unused bits. */
void val_firme_service_features(uint64_t service, uint64_t index, firme_res_t *res)
{
    assert(service == FIRME_SERVICE_ID_GRANULE_MGMT);
    *res = (firme_res_t){.x0 = (uint64_t)g_feature_status};
    if (index == FIRME_GM_FEATURE_REG0_INDEX)
        res->x1 = g_reg0;
    else if (index == FIRME_GM_FEATURE_REG1_INDEX)
        res->x1 = g_reg1;
    else
        res->x0 = (uint64_t)FIRME_NOT_SUPPORTED;
}

/** @brief Suppress expected diagnostics from negative test cases. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *format, ...)
{
    (void)verbosity;
    (void)tag;
    (void)format;
    return VAL_SUCCESS;
}

/** @brief Initialize valid discovery data with nonzero unused output bits. */
static void test_reset(void)
{
    g_version = INT64_C(0x10000);
    g_feature_status = FIRME_SUCCESS;
    g_reg0 = TEST_UNUSED | FIRME_GM_FEATURE_REG0_GPI_SET_BIT |
             FIRME_GM_FEATURE_REG0_OP_CONTINUE_BIT;
    g_reg1 = TEST_UNUSED | FIRME_GM_PGS_4KB;
}

/** @brief Keep ignored bits harmless and architected failures observable. */
int main(void)
{
    firme_rule_res_t result = {0};

    test_reset();
    assert(gm_feature_registers_run(&result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    assert(gm_abi_compliance_run(FIRME_ACS_WORLD_NSEL2, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    assert(gm_abi_compliance_run(FIRME_ACS_WORLD_SECURE, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    assert(gm_abi_compliance_run(FIRME_ACS_WORLD_REALM, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);

    g_reg0 &= ~FIRME_GM_FEATURE_REG0_GPI_SET_BIT;
    assert(gm_abi_compliance_run(FIRME_ACS_WORLD_REALM, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);

    test_reset();
    g_reg0 |= FIRME_GM_FEATURE_REG0_L1_CREATE_BIT;
    assert(gm_abi_compliance_run(FIRME_ACS_WORLD_NSEL2, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    assert(gm_abi_compliance_run(FIRME_ACS_WORLD_REALM, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    g_reg0 |= FIRME_GM_FEATURE_REG0_L1_DESTROY_BIT;
    assert(gm_abi_compliance_run(FIRME_ACS_WORLD_REALM, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);

    test_reset();
    g_reg1 |= FIRME_GM_FEATURE_REG1_PGS_MASK;
    assert(gm_feature_registers_run(&result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    test_reset();
    g_feature_status = FIRME_INVALID_PARAMETERS;
    assert(gm_feature_registers_run(&result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    assert(gm_abi_compliance_run(FIRME_ACS_WORLD_REALM, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);

    test_reset();
    g_version = FIRME_NOT_SUPPORTED;
    assert(gm_feature_registers_run(&result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_SKIP);
    assert(gm_abi_compliance_run(FIRME_ACS_WORLD_REALM, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_SKIP);
    g_version = FIRME_INVALID_PARAMETERS;
    assert(gm_feature_registers_run(&result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    assert(gm_abi_compliance_run(FIRME_ACS_WORLD_REALM, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    assert(gm_feature_registers_run(NULL) == VAL_ERROR);
    assert(gm_abi_compliance_run(FIRME_ACS_WORLD_REALM, NULL) == VAL_ERROR);
    puts("GM discovery rules: PASS");
    return VAL_SUCCESS;
}
