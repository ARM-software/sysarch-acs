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
#include "val_common/val_smccc.h"
#include "val_common/val_status.h"

static uint64_t g_frame[8];
static uint64_t g_version = 0x10000;
static uint64_t g_poison;

/** @brief Return a stable PE identity for host-side output-poison checks. */
uint64_t read_mpidr_el1(void)
{
    return 0;
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
    puts("PASS: wrapper frames and poisoned-output discovery checks");
    return 0;
}
"""


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
                        str(root / "val/src/val_firme_service.c"), "-o", str(binary)])
        subprocess.run(command, check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
