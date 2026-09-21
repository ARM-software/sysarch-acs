/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>

#include "../service_feature_registers.c"
#include "../feature_register_result_encoding.c"
#include "../feature_register_abi_presence.c"

#define TEST_UNUSED (UINT64_C(1) << 63)
#define TEST_UNUSED_SERVICE (UINT64_C(1) << 31)

static uint64_t g_base_reg0;
static uint64_t g_base_reg1;
static uint64_t g_gm_reg0;

/** @brief Supply the allocated Table 2.8 service-list bit for the fixture. */
uint64_t val_firme_base_service_list_bit(uint32_t service)
{
    assert(service > FIRME_SERVICE_ID_BASE && service < FIRME_SERVICE_ID_MAX);
    return UINT64_C(1) << (FIRME_BASE_SERVICE_LIST_SHIFT + service - 1U);
}

/** @brief Decode the signed W0 status returned by the mock transport. */
int64_t val_firme_status_to_i64(uint64_t value)
{
    return (int64_t)(int32_t)(uint32_t)value;
}

/** @brief Advertise only Base and GM in the mock instance. */
void val_firme_service_version(uint64_t service, firme_res_t *res)
{
    *res = (firme_res_t){.x0 = (uint64_t)FIRME_NOT_SUPPORTED};
    if (service == FIRME_SERVICE_ID_BASE || service == FIRME_SERVICE_ID_GRANULE_MGMT)
        res->x0 = UINT64_C(0x10000);
}

/** @brief Supply feature registers, including arbitrary unused bits. */
void val_firme_service_features(uint64_t service, uint64_t index, firme_res_t *res)
{
    *res = (firme_res_t){.x0 = FIRME_SUCCESS};
    if (service == FIRME_SERVICE_ID_BASE) {
        assert(index <= FIRME_BASE_FEATURE_REG1_INDEX);
        res->x1 = index == FIRME_BASE_FEATURE_REG0_INDEX ? g_base_reg0 : g_base_reg1;
    } else {
        assert(service == FIRME_SERVICE_ID_GRANULE_MGMT && index == 0ULL);
        res->x1 = g_gm_reg0;
    }
}

/** @brief Suppress expected negative-test diagnostics. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *format, ...)
{
    (void)verbosity;
    (void)tag;
    (void)format;
    return VAL_SUCCESS;
}

/** @brief Configure compliant fields with nonzero unused register/list bits. */
static void test_reset(void)
{
    g_base_reg0 = TEST_UNUSED | FIRME_BASE_FEATURE_REG0_MANDATORY_BITS;
    g_base_reg1 = TEST_UNUSED | TEST_UNUSED_SERVICE |
                  val_firme_base_service_list_bit(FIRME_SERVICE_ID_GRANULE_MGMT);
    g_gm_reg0 = TEST_UNUSED | FIRME_GM_FEATURE_REG0_GPI_SET_BIT |
                FIRME_GM_FEATURE_REG0_OP_CONTINUE_BIT;
}

/** @brief Validate ignored outputs without relaxing mandatory/prohibited fields. */
int main(void)
{
    firme_rule_res_t result = {0};

    test_reset();
    assert(service_feature_registers_run(FIRME_ACS_WORLD_NSEL2, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    assert(feature_register_result_run(FIRME_ACS_WORLD_NSEL2, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    assert(feature_register_abi_run(FIRME_ACS_WORLD_NSEL2, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);

    g_base_reg1 |= val_firme_base_service_list_bit(FIRME_SERVICE_ID_MECID_MGMT);
    assert(service_feature_registers_run(FIRME_ACS_WORLD_NSEL2, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    assert(feature_register_result_run(FIRME_ACS_WORLD_NSEL2, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);

    test_reset();
    g_base_reg0 &= ~FIRME_BASE_FEATURE_REG0_MANDATORY_BITS;
    assert(service_feature_registers_run(FIRME_ACS_WORLD_NSEL2, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    assert(feature_register_result_run(FIRME_ACS_WORLD_NSEL2, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    assert(feature_register_abi_run(FIRME_ACS_WORLD_NSEL2, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);

    test_reset();
    g_gm_reg0 &= ~FIRME_GM_FEATURE_REG0_GPI_SET_BIT;
    assert(feature_register_abi_run(FIRME_ACS_WORLD_NSEL2, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    test_reset();
    g_gm_reg0 |= FIRME_GM_FEATURE_REG0_L1_CREATE_BIT;
    assert(feature_register_abi_run(FIRME_ACS_WORLD_NSEL2, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    puts("Base feature masks: PASS");
    return VAL_SUCCESS;
}
