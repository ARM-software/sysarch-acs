/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"

#include "aarch64/sysreg.h"
#include "val_common/val_status.h"
#include "val_firme_service.h"
#include "val_log.h"

#define VAL_FIRME_GPT_PGS_SIZE_4KB  UINT64_C(0x1000)
#define VAL_FIRME_GPT_PGS_SIZE_16KB UINT64_C(0x4000)
#define VAL_FIRME_GPT_PGS_SIZE_64KB UINT64_C(0x10000)

/**
 *   @brief    - Probe a 64-bit load from a given world and return fault details.
 *   @param    - world     : Target world (NS-EL2/Secure/Realm).
 *   @param    - addr      : Address to read.
 *   @param    - out_value : Optional value read (valid when no fault).
 *   @param    - out_fault : Optional fault info (faulted/esr/far).
 *   @return   - VAL_SUCCESS on dispatch, VAL_ERROR on transport failure.
**/
int val_firme_mem_read64_world(firme_acs_world_t world,
                               uint64_t addr,
                               uint64_t *out_value,
                               val_fault_info_t *out_fault)
{
    firme_acs_call_res_t res = {0};

    if (out_fault) {
        volatile uint64_t *dst = (volatile uint64_t *)(void *)out_fault;
        dst[0] = 0;
        dst[1] = 0;
        dst[2] = 0;
    }
    if (out_value) {
        *out_value = 0;
    }

    if (world == FIRME_ACS_WORLD_NSEL2) {
        val_probe_read64(addr, out_value, out_fault);
        return VAL_SUCCESS;
    }

    if (val_firme_world_call(world, FIRME_ACS_CMD_RUN_DIAG,
                            /* test_id */ 2,
                            /* a0 */ addr,
                            /* a1 */ 0,
                            /* a2 */ 0,
                            &res) != 0) {
        return VAL_ERROR;
    }
    if (res.x0 != (uint64_t)ACS_STATUS_PASS) {
        return VAL_ERROR;
    }

    if (out_fault) {
        /*
         * Use volatile stores to avoid paired stores (stp) to potentially
         * misaligned addresses when alignment checking is enabled.
         * Cross-world memory probes use a compact payload format:
         * x1=faulted, x2=ESR when faulted or read value when not faulted.
         * Realm transport only guarantees two payload words through RMMD.
         */
        volatile uint64_t *dst = (volatile uint64_t *)(void *)out_fault;
        dst[0] = res.x1; /* faulted */
        dst[1] = (res.x1 != 0ULL) ? res.x2 : 0; /* esr */
        dst[2] = 0; /* far is not part of the compact transport payload */
    }
    if (out_value) {
        *out_value = (res.x1 == 0ULL) ? res.x2 : 0; /* value */
    }
    return VAL_SUCCESS;
}

/**
 *   @brief    - Perform an NS write followed by a DSB to commit the store.
 *   @param    - addr  : Target address.
 *   @param    - value : Value to store.
 *   @return   - void
**/
void val_firme_ns_write64(uint64_t addr, uint64_t value)
{
    *(volatile uint64_t *)(uintptr_t)addr = value;
    dsb_sy();
}

/**
 *   @brief    - Convert an ABI return value into a signed W0 status.
 *   @param    - v : Raw x0 return.
 *   @return   - Signed FIRME status value.
**/
int64_t val_firme_as_i64(uint64_t v)
{
    return val_firme_status_to_i64(v);
}

/**
 *   @brief    - Check whether a FIRME return indicates success with an expected count.
 *   @param    - x0      : FIRME status.
 *   @param    - x1      : FIRME completion count.
 *   @param    - exp_cnt : Expected completion count.
 *   @return   - 1 if success and count matches, 0 otherwise.
**/
uint32_t val_firme_status_success(uint64_t x0, uint64_t x1, uint64_t exp_cnt)
{
    return (uint32_t)((val_firme_as_i64(x0) == (int64_t)FIRME_SUCCESS) && (x1 == exp_cnt));
}

/**
 *   @brief    - Check whether a FIRME return indicates a denied access.
 *   @param    - x0 : FIRME status.
 *   @return   - 1 if denied, 0 otherwise.
**/
uint32_t val_firme_status_deniedish(uint64_t x0)
{
    return (uint32_t)(val_firme_as_i64(x0) == (int64_t)FIRME_DENIED);
}

/**
 *   @brief    - Check whether a FIRME return indicates the ABI is not supported.
 *   @param    - x0 : FIRME status.
 *   @return   - 1 if not supported, 0 otherwise.
**/
uint32_t val_firme_status_not_supported(uint64_t x0)
{
    return (uint32_t)(val_firme_as_i64(x0) == (int64_t)FIRME_NOT_SUPPORTED);
}

/**
 *   @brief    - Check whether a FIRME return indicates invalid parameters.
 *   @param    - x0 : FIRME status.
 *   @return   - 1 if invalid parameters, 0 otherwise.
**/
uint32_t val_firme_status_invalid_parameters(uint64_t x0)
{
    return (uint32_t)(val_firme_as_i64(x0) == (int64_t)FIRME_INVALID_PARAMETERS);
}

/**
 *   @brief    - Check whether FIRME_GM_GPI_SET is advertised by the DUT.
 *   @param    - out       : Optional rule result updated on skip/failure.
 *   @param    - fail_step : Step ID to report on service-query failure.
 *   @return   - VAL_TRUE if advertised, otherwise VAL_FALSE.
**/
uint32_t val_firme_gpt_gpi_set_advertised(firme_rule_res_t *out, uint64_t fail_step)
{
    firme_res_t features_res = {0};
    uint64_t feature_reg0 = 0ULL;
    uint32_t advertised = VAL_FALSE;

    if (val_firme_service_abi_is_advertised(FIRME_SERVICE_ID_GRANULE_MGMT,
                                            FIRME_GM_FEATURE_REG0_GPI_SET_BIT,
                                            &advertised, &feature_reg0,
                                            &features_res) != VAL_SUCCESS) {
        LOG_ERROR(NULL, "gpt: failed to query GM_GPI_SET advertisement x0=%llx x1=%llx",
                  (unsigned long long)features_res.x0,
                  (unsigned long long)features_res.x1);
        if (out) {
            out->status = (uint64_t)ACS_STATUS_FAIL;
            out->fail_step = fail_step;
            out->last_x0 = features_res.x0;
            out->last_x1 = features_res.x1;
        }
        return VAL_FALSE;
    }

    if (advertised == VAL_FALSE) {
        if (val_firme_status_to_i64(features_res.x0) ==
            (int64_t)FIRME_NOT_SUPPORTED) {
            if (out) {
                out->status = (uint64_t)ACS_STATUS_SKIP;
                out->fail_step = 0;
                out->last_x0 = features_res.x0;
                out->last_x1 = 0;
            }
            return VAL_FALSE;
        }

        LOG_ERROR(NULL, "gpt: GM service omits mandatory GPI_SET bit reg0=%llx",
                  (unsigned long long)feature_reg0);
        if (out) {
            out->status = (uint64_t)ACS_STATUS_FAIL;
            out->fail_step = fail_step;
            out->last_x0 = features_res.x0;
            out->last_x1 = feature_reg0;
        }
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 *   @brief    - Read Granule Management feature register 1.
 *   @param    - feature_reg1 : Returned feature register value.
 *   @param    - out          : Optional rule result updated on skip/failure.
 *   @param    - fail_step    : Step ID to report on service-query failure.
 *   @return   - VAL_TRUE if feature register 1 is available, otherwise VAL_FALSE.
**/
uint32_t val_firme_gpt_feature_reg1(uint64_t *feature_reg1, firme_rule_res_t *out,
                                    uint64_t fail_step)
{
    firme_res_t res = {0};

    if (feature_reg1)
        *feature_reg1 = 0ULL;

    val_firme_service_features(FIRME_SERVICE_ID_GRANULE_MGMT,
                               FIRME_GM_FEATURE_REG1_INDEX, &res);

    if (val_firme_status_not_supported(res.x0)) {
        if (out) {
            out->status = (uint64_t)ACS_STATUS_SKIP;
            out->fail_step = 0;
            out->last_x0 = res.x0;
            out->last_x1 = res.x1;
        }
        return VAL_FALSE;
    }

    if (val_firme_as_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        LOG_ERROR(NULL, "gpt: failed to read GM feature register 1 x0=%llx x1=%llx",
                  (unsigned long long)res.x0, (unsigned long long)res.x1);
        if (out) {
            out->status = (uint64_t)ACS_STATUS_FAIL;
            out->fail_step = fail_step;
            out->last_x0 = res.x0;
            out->last_x1 = res.x1;
        }
        return VAL_FALSE;
    }

    if (feature_reg1)
        *feature_reg1 = res.x1;

    return VAL_TRUE;
}

/**
 *   @brief    - Decode the PGS field in Granule Management feature register 1.
 *   @param    - feature_reg1 : Raw feature register 1 value.
 *   @return   - Granule size in bytes, or 0 for an unknown encoding.
**/
uint64_t val_firme_gpt_pgs_size_from_feature_reg1(uint64_t feature_reg1)
{
    uint64_t pgs = (feature_reg1 >> FIRME_GM_FEATURE_REG1_PGS_SHIFT) &
                   FIRME_GM_FEATURE_REG1_PGS_MASK;

    if (pgs == FIRME_GM_PGS_4KB)
        return VAL_FIRME_GPT_PGS_SIZE_4KB;

    if (pgs == FIRME_GM_PGS_16KB)
        return VAL_FIRME_GPT_PGS_SIZE_16KB;

    if (pgs == FIRME_GM_PGS_64KB)
        return VAL_FIRME_GPT_PGS_SIZE_64KB;

    return 0ULL;
}

/**
 *   @brief    - Determine which world is expected to access memory in a given GPI.
 *   @param    - gpi   : GPI ID (FIRME_GPI_*).
 *   @param    - world : World being validated.
 *   @return   - 1 if access should be permitted, 0 otherwise.
**/
uint32_t val_firme_expect_access_world(uint64_t gpi, firme_acs_world_t world)
{
    switch ((uint32_t)gpi) {
    case (uint32_t)FIRME_GPI_SECURE:
        return (uint32_t)(world == FIRME_ACS_WORLD_SECURE);
    case (uint32_t)FIRME_GPI_REALM:
        return (uint32_t)(world == FIRME_ACS_WORLD_REALM);
    case (uint32_t)FIRME_GPI_ROOT:
        return 0U;
    case (uint32_t)FIRME_GPI_NONSECURE:
    case (uint32_t)FIRME_GPI_NSO:
    case (uint32_t)FIRME_GPI_NSP:
    case (uint32_t)FIRME_GPI_SA:
    default:
        return (uint32_t)(world == FIRME_ACS_WORLD_NSEL2);
    }
}

/**
 *   @brief    - Validate access enforcement from all worlds for a single address/GPI.
 *   @param    - addr           : Address to probe.
 *   @param    - gpi            : GPI the granule is expected to be in.
 *   @param    - expected_value : Expected 64-bit value when access is permitted.
 *   @param    - check_value    : If non-zero, validate the returned value as well.
 *   @param    - out            : Optional rule result (filled on failure).
 *   @param    - fail_step_base : Base step index to encode which world failed.
 *   @return   - 1 if matrix matches expectations, 0 otherwise.
**/
uint32_t val_firme_check_access_matrix(uint64_t addr,
                                       uint64_t gpi,
                                       uint64_t expected_value,
                                       uint32_t check_value,
                                       firme_rule_res_t *out,
                                       uint64_t fail_step_base)
{
    /* Transport failure tag for out->last_x0. */
    const uint64_t transport_fail_magic = UINT64_C(0xFFFF0000);

    const firme_acs_world_t worlds[3] = {
        FIRME_ACS_WORLD_NSEL2,
        FIRME_ACS_WORLD_SECURE,
        FIRME_ACS_WORLD_REALM,
    };

    for (uint32_t i = 0; i < 3U; i++) {
        const firme_acs_world_t world = worlds[i];
        val_fault_info_t fault_info = {0};
        uint64_t value = 0;
        uint32_t expected_ok = 0;

        if (val_firme_mem_read64_world(world, addr, &value, &fault_info) != VAL_SUCCESS) {
            if (out) {
                out->fail_step = fail_step_base + (uint64_t)i;
                out->last_x0 = transport_fail_magic | (uint64_t)i;
                out->last_x1 = 0;
            }
            return 0U;
        }

        expected_ok = val_firme_expect_access_world(gpi, world);
        if (expected_ok != 0U) {
            if (fault_info.faulted != 0U) {
                if (out) {
                    out->fail_step = fail_step_base + (uint64_t)i;
                    out->last_x0 = fault_info.esr;
                    out->last_x1 = fault_info.far;
                }
                return 0U;
            }
            if (check_value != 0U && value != expected_value) {
                if (out) {
                    out->fail_step = fail_step_base + (uint64_t)i;
                    out->last_x0 = value;
                    out->last_x1 = expected_value;
                }
                return 0U;
            }
        } else {
            if (fault_info.faulted == 0U) {
                if (out) {
                    out->fail_step = fail_step_base + (uint64_t)i;
                    out->last_x0 = value;
                    out->last_x1 = 0;
                }
                return 0U;
            }
            if (!val_fault_is_gpf(fault_info.esr)) {
                if (out) {
                    out->fail_step = fail_step_base + (uint64_t)i;
                    out->last_x0 = fault_info.esr;
                    out->last_x1 = fault_info.far;
                }
                return 0U;
            }
        }
    }

    return 1U;
}
