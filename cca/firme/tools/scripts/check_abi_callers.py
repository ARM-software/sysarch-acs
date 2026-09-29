#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

"""Host-side caller checks; these are not DUT compliance or non-disclosure tests."""

import os
from pathlib import Path
import subprocess
import tempfile


SOURCE = r"""
#include <assert.h>
#include <stdio.h>
#include "val_firme_abi.h"
#include "val_firme_service.h"
#include "val_firme_rules.h"
#include "val_firme_shared_buffer.h"
#include "val_common/val_smccc.h"
#include "val_common/val_status.h"
#include "val_log.h"

int firme_base_reserved_output_registers_run_ns(uint64_t base_pa, firme_rule_res_t *out);
int firme_base_reserved_output_bits_run_ns(uint64_t base_pa, firme_rule_res_t *out);

static uint64_t g_frame[8];
static uint64_t g_version = 0x10000;
static uint64_t g_poison;
static uint64_t g_mpidr;

/** @brief Replace the architectural barrier in native-only caller checks. */
void dsb_sy(void)
{
}

/** @brief Reject unplanned PAT calls in the discovery-only caller fixture. */
void val_smccc_smc_call_frame(const smccc_call_frame_t *input, smccc_call_frame_t *output)
{
    (void)input;
    (void)output;
    assert(VAL_FALSE);
}

/** @brief Keep native caller checks independent of the UART logger. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *fmt, ...)
{
    (void)verbosity;
    (void)tag;
    (void)fmt;
    return VAL_SUCCESS;
}

/** @brief Return a stable PE identity for host-side output-poison checks. */
uint64_t read_mpidr_el1(void)
{
    return g_mpidr;
}

/** @brief Capture the production wrapper's inputs and poison unused outputs. */
void val_smccc_smc_call(uint64_t fid, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                       uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7,
                       smccc_res_t *res)
{
    const uint64_t frame[] = {fid, arg1, arg2, arg3, arg4, arg5, arg6, arg7};
    unsigned int index;

    for (index = 0; index < 8; index++)
        g_frame[index] = frame[index];
    res->x0 = (g_poison & UINT64_C(0xffffffff00000000)) |
              (fid == FIRME_FID_SERVICE_VERSION ? (uint32_t)g_version : FIRME_SUCCESS);
    res->x1 = fid == FIRME_FID_SERVICE_FEATURES ?
              FIRME_BASE_FEATURE_REG0_MANDATORY_BITS | (g_poison & ~UINT64_C(3)) : g_poison;
    if ((fid == FIRME_FID_SERVICE_FEATURES) &&
        (arg2 == FIRME_BASE_FEATURE_REG1_INDEX))
        res->x1 = val_firme_base_service_list_bit(FIRME_SERVICE_ID_GRANULE_MGMT) |
                  (UINT64_C(7) << FIRME_BASE_MAX_SH_BUF_PG_CNT_SHIFT) |
                  UINT64_C(2) | (g_poison & UINT64_C(0xffffffff00000000));
    res->x2 = g_poison;
    res->x3 = g_poison;
    res->x4 = g_poison;
    res->x5 = g_poison;
    res->x6 = g_poison;
    res->x7 = g_poison;
}

/** @brief Require zero in every unused input accepted by the C helper signature. */
static void check_unused(unsigned int first)
{
    unsigned int index;

    for (index = first; index < 8; index++)
        assert(g_frame[index] == 0);
}

/** @brief Check real wrappers and discovery consumers, not duplicated test decoders. */
int main(void)
{
    const uint64_t poisons[] = {0, UINT64_MAX, UINT64_C(0xa5a5a5a55a5a5a5a)};
    const uint64_t base_address = UINT64_C(0x90000000);
    const uint64_t granule_count = 1;
    const uint64_t target_gpi = 9;
    const uint64_t mec_params = (UINT64_C(0x1234) << FIRME_MEC_REFRESH_MECID_SHIFT) |
                                FIRME_MEC_REFRESH_REASON_DESTROY;
    firme_res_t result;
    firme_res_t mask = {.x0 = UINT64_C(0xffffffff00000000), .x1 = UINT64_MAX};
    firme_res_t replacement = {.x0 = UINT64_MAX, .x1 = UINT64_MAX};
    firme_rule_res_t rule_result;
    uint32_t available;
    uint64_t features;
    unsigned int index;

    for (index = 0; index < sizeof(poisons) / sizeof(poisons[0]); index++) {
        g_poison = poisons[index];
        g_version = 0x10000;
        val_firme_service_version(FIRME_SERVICE_ID_BASE, &result);
        assert(g_frame[1] == FIRME_SERVICE_ID_BASE);
        check_unused(2);
        assert(val_firme_status_to_i64(result.x0) == 0x10000);
        assert(val_firme_service_get_version(FIRME_SERVICE_ID_BASE, &available, &result) ==
               VAL_SUCCESS && available == VAL_TRUE);
        assert(val_firme_service_abi_is_advertised(FIRME_SERVICE_ID_BASE,
               FIRME_BASE_FEATURE_REG_VERSION_BIT, &available, &features, &result) ==
               VAL_SUCCESS && available == VAL_TRUE);
        check_unused(3);
        assert(g_frame[2] == FIRME_BASE_FEATURE_REG0_INDEX);
        g_version = (uint32_t)FIRME_NOT_SUPPORTED;
        assert(val_firme_service_get_version(FIRME_SERVICE_ID_BASE, &available, &result) ==
               VAL_SUCCESS && available == VAL_FALSE);
        g_version = (uint32_t)FIRME_INVALID_PARAMETERS;
        assert(val_firme_service_get_version(FIRME_SERVICE_ID_BASE, &available, &result) ==
               VAL_ERROR);
    }

    val_firme_gm_gpi_set(base_address, granule_count, target_gpi, &result);
    assert(g_frame[1] == base_address && g_frame[2] == granule_count);
    assert(g_frame[3] == target_gpi);
    check_unused(4);
    val_firme_mec_refresh(mec_params, &result);
    assert(g_frame[1] == mec_params);
    check_unused(2);

    val_firme_gm_gpi_op_continue(UINT64_MAX, &result);
    assert(g_frame[0] == FIRME_FID_GM_GPI_OP_CONTINUE && g_frame[1] == UINT64_MAX);
    check_unused(2);
    val_firme_gm_l1_gpt_create(base_address, base_address + 4096, &result);
    assert(g_frame[0] == FIRME_FID_GM_L1_GPT_CREATE && g_frame[1] == base_address);
    assert(g_frame[2] == base_address + 4096);
    check_unused(3);
    val_firme_gm_l1_gpt_destroy(base_address, &result);
    assert(g_frame[0] == FIRME_FID_GM_L1_GPT_DESTROY && g_frame[1] == base_address);
    check_unused(2);
    val_firme_gm_gpt_info(base_address, base_address + 4096, &result);
    assert(g_frame[0] == FIRME_FID_GM_GPT_INFO && g_frame[1] == base_address);
    assert(g_frame[2] == base_address + 4096);
    check_unused(3);

    val_firme_service_version(UINT64_MAX, &result);
    assert(g_frame[1] == FIRME_SERVICE_ID_INPUT_MASK);
    check_unused(2);
    val_firme_service_features(UINT64_MAX, UINT64_MAX, &result);
    assert(g_frame[1] == FIRME_SERVICE_ID_INPUT_MASK);
    assert(g_frame[2] == FIRME_FEATURE_REG_INDEX_INPUT_MASK);
    check_unused(3);
    val_firme_gm_gpi_set(base_address, UINT64_MAX, UINT64_MAX, &result);
    assert(g_frame[2] == UINT64_MAX && g_frame[3] == FIRME_GM_GPI_SET_ATTRIBUTES_MASK);
    check_unused(4);
    val_firme_mec_refresh(UINT64_MAX, &result);
    assert(g_frame[1] == FIRME_MEC_REFRESH_PARAMS_MASK);
    check_unused(2);

    g_poison = 0;
    g_version = 0x10000;
    assert(val_firme_test_output_poison_enable(FIRME_FID_SERVICE_VERSION,
                                              &mask, &replacement) == VAL_SUCCESS);
    val_firme_service_version(FIRME_SERVICE_ID_BASE, &result);
    assert(result.x0 == UINT64_C(0xffffffff00010000) && result.x1 == UINT64_MAX);
    g_mpidr = 1;
    val_firme_service_version(FIRME_SERVICE_ID_BASE, &result);
    assert(result.x0 == g_version && result.x1 == 0);
    g_mpidr = 0;
    val_firme_service_features(FIRME_SERVICE_ID_BASE, 0, &result);
    assert(result.x0 == FIRME_SUCCESS && result.x1 == FIRME_BASE_FEATURE_REG0_MANDATORY_BITS);
    val_firme_test_output_poison_disable();
    val_firme_service_version(FIRME_SERVICE_ID_BASE, &result);
    assert(result.x0 == g_version && result.x1 == 0);
    assert(val_firme_test_output_poison_enable(FIRME_FID_SERVICE_VERSION,
                                              NULL, &replacement) == VAL_ERROR);

    assert(firme_base_reserved_output_registers_run_ns(0, &rule_result) == VAL_SUCCESS);
    assert(rule_result.status == ACS_STATUS_PASS);
    assert(firme_base_reserved_output_bits_run_ns(0, &rule_result) == VAL_SUCCESS);
    assert(rule_result.status == ACS_STATUS_PASS);
    puts("PASS: wrapper frames and poisoned-output discovery checks");
    return VAL_SUCCESS;
}
"""


PAT_SOURCE = Path(__file__).with_name("check_abi_pat.c").read_text()
PAT_FILES = [
    "val/src/val_firme_abi.c", "val/src/val_firme_service.c",
    "val/src/val_firme_shared_buffer.c", "val/src/val_firme_partial_retrieval.c",
    "test/base/shared_buffer_test_common.c", "test/base/shared_buffer_alignment.c",
    "test/base/shared_buffer_size_bounds.c", "test/base/shared_buffer_mapping_attributes.c",
    "test/base/shared_buffer_pas_gpi.c", "test/base/partial_retrieval_test_common.c",
    "test/base/partial_retrieval_last_chunk.c", "test/base/partial_retrieval_intermediate_chunk.c",
    "test/base/partial_retrieval_no_chunk.c",
]


def main():
    """Compile the current production decoders against an instrumented SMC stub."""
    root = Path(__file__).resolve().parents[2]
    with tempfile.TemporaryDirectory(prefix="firme-callers-") as directory:
        temporary = Path(directory)
        source = temporary / "caller_checks.c"
        binary = temporary / "caller_checks"
        source.write_text(SOURCE)
        command = [os.environ.get("HOST_CC", "cc"), "-std=c11", "-Wall", "-Wextra", "-Werror"]
        for include in [root / "val/include", root / "val_common/include",
                        root / "platform/pal_baremetal/targets/fvp/include"]:
            command.extend(["-I", str(include)])
        command.extend([str(source), str(root / "val/src/val_firme_abi.c"),
                        str(root / "val/src/val_firme_service.c"),
                        str(root / "val/src/val_firme_shared_buffer.c"),
                        str(root / "test/base/reserved_output_bits.c"),
                        str(root / "test/base/reserved_output_registers.c"),
                        "-o", str(binary)])
        subprocess.run(command, check=True)
        subprocess.run([str(binary)], check=True)
        source.write_text(PAT_SOURCE)
        command = [os.environ.get("HOST_CC", "cc"), "-std=c11", "-Wall", "-Wextra", "-Werror"]
        for include in [root / "val/include", root / "val_common/include",
                        root / "platform/pal_baremetal/targets/fvp/include"]:
            command.extend(["-I", str(include)])
        command.extend([str(source), *(str(root / filename) for filename in PAT_FILES),
                        "-o", str(binary)])
        subprocess.run(command, check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
