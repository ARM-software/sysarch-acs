/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_mp.h"
#include "aarch64/sysreg.h"
#include "val_common/val_fault.h"
#include "val_common/val_status.h"
#include "val_common/val_smccc.h"
#include "val_firme_dispatch.h"
#include "val_log.h"

#define VAL_MP_SELF_TEST_WORDS 512U
#define VAL_MP_SELF_TEST_ROUNDS 16U
#define VAL_MP_SELF_TEST_CYCLES 2U
#define VAL_MP_SELF_TEST_UNMAPPED UINT64_C(0x400000000)
#define VAL_MP_SELF_TEST_PONG UINT64_C(0x504f4e47)
#define VAL_MP_SELF_TEST_DATA_ABORT UINT64_C(0x25)
#define VAL_MP_SELF_TEST_PSCI_VERSION UINT64_C(0x84000000)
#define VAL_MP_SELF_TEST_PSCI_MIN_VERSION UINT64_C(2)

typedef struct __attribute__((aligned(256))) {
    uint64_t words[VAL_MP_SELF_TEST_WORDS];
    uint64_t seed;
    uint64_t mpidr;
    uint64_t frequency;
    uint32_t epoch;
    uint32_t ready;
    _Atomic uint32_t fail_step;
} val_mp_self_test_data_t;

static val_mp_self_test_data_t g_mp_self_test_data[PAL_PE_COUNT];
static uint32_t g_mp_self_test_secure;
static uint32_t g_mp_self_test_realm;
static uint32_t g_mp_self_test_epoch;

/** @brief Exercise coherent data exchange and independent fault recovery. */
static int val_host_mp_memory_check(uint32_t pe_index, void *context)
{
    val_mp_self_test_data_t *data = context;
    val_fault_info_t fault = {0};
    uint64_t fault_address = VAL_MP_SELF_TEST_UNMAPPED + ((uint64_t)pe_index << 12);
    uint64_t value = 0U;
    uint64_t start = val_read_counter();
    uint32_t word_index = 0U;

    if (data == NULL)
        return VAL_ERROR;

    data->fail_step = 1U;
    if (((read_mpidr_el1() & (uint64_t)PAL_PE_AFFINITY_MASK) != data->mpidr) ||
        (val_read_counter_frequency() != data->frequency) ||
        ((read_sctlr_el2() & UINT64_C(5)) != UINT64_C(5)))
        return VAL_ERROR;

    /* All workers must reach the same phase before touching their test page. */
    __atomic_store_n(&data->ready, data->epoch, __ATOMIC_RELEASE);
    while (__atomic_load_n(&g_mp_self_test_epoch, __ATOMIC_ACQUIRE) != data->epoch) {
        if ((val_read_counter() - start) >= data->frequency * UINT64_C(5))
            return VAL_ERROR;

        val_mp_relax();
    }

    data->fail_step = 2U;
    for (word_index = 0U; word_index < VAL_MP_SELF_TEST_WORDS; word_index++) {
        if (data->words[word_index] != (data->seed + word_index))
            return VAL_ERROR;

        data->words[word_index] = ~(data->seed + word_index);
    }

    data->fail_step = 3U;
    if ((val_probe_read64(fault_address, &value, &fault) != VAL_ERROR) ||
        (fault.faulted == 0U) || (fault.far != fault_address) ||
        ((fault.esr >> 26) != VAL_MP_SELF_TEST_DATA_ABORT))
        return VAL_ERROR;

    data->fail_step = 4U;
    if ((val_probe_read64((uintptr_t)&data->words[0], &value, &fault) != VAL_SUCCESS) ||
        (value != ~data->seed))
        return VAL_ERROR;

    data->fail_step = 0U;
    return VAL_SUCCESS;
}

/** @brief Verify secondary Secure/Realm entry and return without running tests. */
static int val_host_mp_world_check(uint32_t pe_index, void *context)
{
    firme_acs_call_res_t result = {0};
    smccc_res_t psci_result = {0};
    val_mp_self_test_data_t *data = context;

    if ((data == NULL) || (data->mpidr != g_pal_pe_mpidrs[pe_index]))
        return VAL_ERROR;

    /* The primary's caller-audit scope must not intercept this PE's SMC. */
    data->fail_step = 5U;
    val_smccc_smc_call(VAL_MP_SELF_TEST_PSCI_VERSION, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
                       &psci_result);
    if (((int32_t)psci_result.x0 < 0) ||
        (psci_result.x0 < VAL_MP_SELF_TEST_PSCI_MIN_VERSION))
        return VAL_ERROR;

    if (g_mp_self_test_secure != 0U) {
        data->fail_step = 6U;
        if ((val_firme_world_call(FIRME_ACS_WORLD_SECURE, FIRME_ACS_CMD_PING,
                                 0U, 0U, 0U, 0U, &result) != VAL_SUCCESS) ||
            (result.x0 != ACS_STATUS_PASS) || (result.x1 != VAL_MP_SELF_TEST_PONG))
            return VAL_ERROR;
    }

    if (g_mp_self_test_realm != 0U) {
        data->fail_step = 7U;
        if ((val_firme_world_call(FIRME_ACS_WORLD_REALM, FIRME_ACS_CMD_PING,
                                 0U, 0U, 0U, 0U, &result) != VAL_SUCCESS) ||
            (result.x0 != ACS_STATUS_PASS) || (result.x1 != VAL_MP_SELF_TEST_PONG))
            return VAL_ERROR;
    }

    data->fail_step = 0U;
    return VAL_SUCCESS;
}

/** @brief Return an intentional callback error to validate result propagation. */
static int val_host_mp_error_check(uint32_t pe_index, void *context)
{
    (void)pe_index;
    (void)context;
    return VAL_ERROR;
}

/** @brief Check MP infrastructure separately from specification-rule results. */
int val_host_mp_self_test(void)
{
    uint32_t primary = val_pe_index();
    uint32_t pe_index = 0U;
    uint32_t cycle = 0U;
    uint32_t round = 0U;
    uint32_t word_index = 0U;
    uint64_t start = 0U;
    int result = VAL_ERROR;
    int dispatch_status = VAL_ERROR;
    firme_acs_call_res_t world_result = {0};
    val_mp_self_test_data_t *data = NULL;

    if (PAL_PE_COUNT == 1U) {
        LOG_TEST(NULL, "MP self-test: single-PE platform, no secondary coverage");
        return VAL_SUCCESS;
    }

    g_mp_self_test_secure =
        (val_firme_world_call(FIRME_ACS_WORLD_SECURE, FIRME_ACS_CMD_PING,
                              0U, 0U, 0U, 0U, &world_result) == VAL_SUCCESS) &&
        (world_result.x0 == ACS_STATUS_PASS) &&
        (world_result.x1 == VAL_MP_SELF_TEST_PONG);
    g_mp_self_test_realm =
        (val_firme_world_call(FIRME_ACS_WORLD_REALM, FIRME_ACS_CMD_PING,
                              0U, 0U, 0U, 0U, &world_result) == VAL_SUCCESS) &&
        (world_result.x0 == ACS_STATUS_PASS) &&
        (world_result.x1 == VAL_MP_SELF_TEST_PONG);
    LOG_TEST(NULL, "MP self-test: %u PEs, Secure=%u Realm=%u",
             PAL_PE_COUNT, g_mp_self_test_secure, g_mp_self_test_realm);

    for (cycle = 0U; cycle < VAL_MP_SELF_TEST_CYCLES; cycle++) {
        /* Step 1: Bring up all secondary PEs, including the other cluster. */
        for (pe_index = 0U; pe_index < PAL_PE_COUNT; pe_index++) {
            if (pe_index == primary)
                continue;

            if (val_host_mp_start(pe_index) != VAL_SUCCESS)
                return VAL_ERROR;
        }

        /* Step 2: Publish distinct cacheable pages and fault concurrently. */
        for (round = 0U; round < VAL_MP_SELF_TEST_ROUNDS; round++) {
            for (pe_index = 0U; pe_index < PAL_PE_COUNT; pe_index++) {
                if (pe_index == primary)
                    continue;

                data = &g_mp_self_test_data[pe_index];
                data->seed = ((uint64_t)cycle << 48) | ((uint64_t)round << 32) | pe_index;
                data->mpidr = g_pal_pe_mpidrs[pe_index];
                data->frequency = val_read_counter_frequency();
                data->epoch = cycle * VAL_MP_SELF_TEST_ROUNDS + round + 1U;
                data->ready = 0U;
                for (word_index = 0U; word_index < VAL_MP_SELF_TEST_WORDS; word_index++)
                    data->words[word_index] = data->seed + word_index;

                if (val_host_mp_dispatch(pe_index, val_host_mp_memory_check, data) !=
                    VAL_SUCCESS)
                    return VAL_ERROR;
            }

            start = val_read_counter();
            for (pe_index = 0U; pe_index < PAL_PE_COUNT; pe_index++) {
                if (pe_index == primary)
                    continue;

                data = &g_mp_self_test_data[pe_index];
                while (__atomic_load_n(&data->ready, __ATOMIC_ACQUIRE) != data->epoch) {
                    if ((val_read_counter() - start) >= data->frequency * UINT64_C(5)) {
                        LOG_ERROR(NULL, "MP rendezvous failed on PE=%u", pe_index);
                        return VAL_ERROR;
                    }

                    val_mp_relax();
                }

                /* A pending job must not be replaced or powered down. */
                if ((val_host_mp_dispatch(pe_index, val_host_mp_memory_check, data) !=
                     VAL_ERROR) || (val_host_mp_stop(pe_index) != VAL_ERROR))
                    return VAL_ERROR;
            }

            __atomic_store_n(&g_mp_self_test_epoch,
                             cycle * VAL_MP_SELF_TEST_ROUNDS + round + 1U,
                             __ATOMIC_RELEASE);

            for (pe_index = 0U; pe_index < PAL_PE_COUNT; pe_index++) {
                if (pe_index == primary)
                    continue;

                if (val_host_mp_wait(pe_index, &result) != VAL_SUCCESS) {
                    LOG_ERROR(NULL, "MP memory/fault check PE=%u round=%u step=%u failed",
                              pe_index, round, g_mp_self_test_data[pe_index].fail_step);
                    return VAL_ERROR;
                }

                data = &g_mp_self_test_data[pe_index];
                for (word_index = 0U; word_index < VAL_MP_SELF_TEST_WORDS; word_index++) {
                    if (data->words[word_index] != ~(data->seed + word_index)) {
                        LOG_ERROR(NULL, "MP result mismatch PE=%u word=%u",
                                  pe_index, word_index);
                        return VAL_ERROR;
                    }
                }
            }
        }

        /* Step 3: Check failed jobs, world entry, CPU_OFF and later warm restart. */
        for (pe_index = 0U; pe_index < PAL_PE_COUNT; pe_index++) {
            if (pe_index == primary)
                continue;

            if ((val_host_mp_dispatch(pe_index, val_host_mp_error_check, NULL) !=
                 VAL_SUCCESS) || (val_host_mp_wait(pe_index, &result) != VAL_ERROR) ||
                (result != VAL_ERROR))
                return VAL_ERROR;

            if (val_smccc_test_input_capture_enable() != VAL_SUCCESS)
                return VAL_ERROR;

            dispatch_status = val_host_mp_dispatch(pe_index, val_host_mp_world_check,
                                                   &g_mp_self_test_data[pe_index]);
            if (dispatch_status == VAL_SUCCESS)
                dispatch_status = val_host_mp_wait(pe_index, &result);

            val_smccc_test_input_capture_disable();
            if (dispatch_status != VAL_SUCCESS) {
                LOG_ERROR(NULL, "MP world check PE=%u step=%u failed",
                          pe_index, g_mp_self_test_data[pe_index].fail_step);
                return VAL_ERROR;
            }

            if (val_host_mp_stop(pe_index) != VAL_SUCCESS)
                return VAL_ERROR;
        }
    }

    LOG_TEST(NULL, "MP self-test PASS: %u workers, %u rounds, %u boot/off cycles",
             PAL_PE_COUNT - 1U, VAL_MP_SELF_TEST_ROUNDS, VAL_MP_SELF_TEST_CYCLES);
    return VAL_SUCCESS;
}
