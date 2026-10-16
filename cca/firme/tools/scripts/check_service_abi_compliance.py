#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

"""Native ALP4 service-catalog regressions, not DUT compliance qualification."""

import os
from pathlib import Path
import subprocess
import tempfile


SOURCE = r"""
#include <assert.h>
#include <stdio.h>

#include "val_common/val_smccc.h"
#include "service_abi_compliance.c"
#include "feature_register_abi_presence.c"

#define TEST_VERSION_1_0 (UINT64_C(1) << FIRME_VERSION_MAJOR_SHIFT)
#define TEST_NO_FAILURE UINT64_C(0)

static uint32_t attestation_implemented = VAL_TRUE;
static uint64_t attestation_features;

/** @brief Supply the primary PE identity to the production ABI result wrapper. */
uint64_t read_mpidr_el1(void)
{
    return UINT64_C(0);
}

/** @brief Supply service discovery responses independently of the production catalog. */
void val_smccc_smc_call(uint64_t fid, uint64_t service_id, uint64_t index,
                        uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6,
                        uint64_t arg7, smccc_res_t *res)
{
    assert(res != NULL);
    assert((fid == FIRME_FID_SERVICE_VERSION) || (fid == FIRME_FID_SERVICE_FEATURES));
    assert(index == FIRME_BASE_FEATURE_REG0_INDEX);
    assert((arg3 | arg4 | arg5 | arg6 | arg7) == 0U);
    *res = (smccc_res_t){0};
    res->x0 = (uint64_t)FIRME_NOT_SUPPORTED;
    if ((service_id != FIRME_SERVICE_ID_BASE) &&
        ((service_id != FIRME_SERVICE_ID_ATTESTATION) ||
         (attestation_implemented == VAL_FALSE)))
        return;

    if (fid == FIRME_FID_SERVICE_VERSION) {
        res->x0 = TEST_VERSION_1_0;
        return;
    }

    res->x0 = FIRME_SUCCESS;
    res->x1 = (service_id == FIRME_SERVICE_ID_BASE) ?
              FIRME_BASE_FEATURE_REG0_MANDATORY_BITS : attestation_features;
}

/** @brief Suppress diagnostics from intentionally noncompliant fixture responses. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *format, ...)
{
    (void)verbosity;
    (void)tag;
    (void)format;
    return VAL_SUCCESS;
}

/** @brief Check Table 6.7 classifications without requiring RAK_GET for Base attestation. */
static void check_attestation_catalog(void)
{
    const uint64_t optional = FIRME_ATTEST_FEATURE_REG0_RAK_GET_BIT |
                              FIRME_ATTEST_FEATURE_REG0_RAT_SIGN_BIT |
                              FIRME_ATTEST_FEATURE_REG0_EXT_CLAIMS_STAGE_BIT |
                              FIRME_ATTEST_FEATURE_REG0_EXT_CLAIMS_FINAL_BIT |
                              FIRME_ATTEST_FEATURE_REG0_EXT_CLAIMS_CLEAR_BIT;

    assert(val_firme_service_mandatory_abi_mask(FIRME_ACS_WORLD_REALM,
               FIRME_SERVICE_ID_ATTESTATION) == FIRME_ATTEST_FEATURE_REG0_PAT_GET_BIT);
    assert(val_firme_service_optional_abi_mask(FIRME_ACS_WORLD_REALM,
               FIRME_SERVICE_ID_ATTESTATION) == optional);
    assert(val_firme_service_supported_abi_mask(FIRME_ACS_WORLD_REALM,
               FIRME_SERVICE_ID_ATTESTATION) == (FIRME_ATTEST_FEATURE_REG0_PAT_GET_BIT | optional));
    assert(val_firme_service_mandatory_abi_mask(FIRME_ACS_WORLD_NSEL2,
               FIRME_SERVICE_ID_ATTESTATION) == FIRME_ATTEST_FEATURE_REG0_PAT_GET_BIT);
    assert(val_firme_service_optional_abi_mask(FIRME_ACS_WORLD_NSEL2,
               FIRME_SERVICE_ID_ATTESTATION) == 0U);
    assert(val_firme_service_supported_abi_mask(FIRME_ACS_WORLD_NSEL2,
               FIRME_SERVICE_ID_ATTESTATION) == FIRME_ATTEST_FEATURE_REG0_PAT_GET_BIT);
    assert(val_firme_service_is_allowed_at_world(FIRME_ACS_WORLD_SECURE,
               FIRME_SERVICE_ID_ATTESTATION) == VAL_FALSE);
    assert(val_firme_service_supported_abi_mask(FIRME_ACS_WORLD_SECURE,
               FIRME_SERVICE_ID_ATTESTATION) == 0U);
}

/** @brief Check every Realm attestation bitmap against mandatory, dependency and group rules. */
static void check_realm_compliance(void)
{
    const uint64_t extension_group = FIRME_ATTEST_FEATURE_REG0_EXT_CLAIMS_STAGE_BIT |
                                     FIRME_ATTEST_FEATURE_REG0_EXT_CLAIMS_FINAL_BIT |
                                     FIRME_ATTEST_FEATURE_REG0_EXT_CLAIMS_CLEAR_BIT;
    firme_rule_res_t result;
    uint64_t bitmap;
    uint64_t expected_step;
    uint64_t extensions;
    int status;

    for (bitmap = 0U; bitmap <= FIRME_ATTEST_FEATURE_REG0_ALL_ABI_BITS; bitmap++) {
        attestation_features = bitmap;
        extensions = bitmap & extension_group;
        expected_step = TEST_NO_FAILURE;
        if ((bitmap & FIRME_ATTEST_FEATURE_REG0_PAT_GET_BIT) == 0U)
            expected_step = SERVICE_ABI_COMPLIANCE_STEP_MANDATORY_BITS;
        else if (((bitmap & FIRME_ATTEST_FEATURE_REG0_RAT_SIGN_BIT) != 0U) &&
                 ((bitmap & FIRME_ATTEST_FEATURE_REG0_RAK_GET_BIT) == 0U))
            expected_step = SERVICE_ABI_COMPLIANCE_STEP_ABI_DEPENDENCY;
        else if ((extensions != 0U) && (extensions != extension_group))
            expected_step = SERVICE_ABI_COMPLIANCE_STEP_OPTIONAL_GROUP;

        status = firme_base_service_abi_compliance_run_realm(0U, &result);
        if (expected_step == TEST_NO_FAILURE) {
            assert(status == VAL_SUCCESS && result.status == ACS_STATUS_PASS);
        } else {
            assert(status == VAL_ERROR && result.status == ACS_STATUS_FAIL);
            assert(result.last_x0 == bitmap);
            assert(result.last_x1 == FIRME_SERVICE_ID_ATTESTATION);
        }
        assert(result.fail_step == expected_step);

        status = firme_base_feature_register_abi_presence_run_realm(0U, &result);
        if ((bitmap & FIRME_ATTEST_FEATURE_REG0_PAT_GET_BIT) != 0U) {
            assert(status == VAL_SUCCESS && result.status == ACS_STATUS_PASS);
        } else {
            assert(status == VAL_ERROR && result.status == ACS_STATUS_FAIL);
            assert(result.fail_step == FEATURE_REGISTER_ABI_STEP_MANDATORY_BITS);
        }
    }
}

/** @brief Preserve NS/Secure restrictions and clean absence of an optional service. */
static void check_other_instances(void)
{
    firme_rule_res_t result;
    uint64_t bitmap;
    uint64_t expected_step;
    int status;

    for (bitmap = 0U; bitmap <= FIRME_ATTEST_FEATURE_REG0_ALL_ABI_BITS; bitmap++) {
        attestation_features = bitmap;
        expected_step = TEST_NO_FAILURE;
        if ((bitmap & FIRME_ATTEST_FEATURE_REG0_PAT_GET_BIT) == 0U)
            expected_step = SERVICE_ABI_COMPLIANCE_STEP_MANDATORY_BITS;
        else if (bitmap != FIRME_ATTEST_FEATURE_REG0_PAT_GET_BIT)
            expected_step = SERVICE_ABI_COMPLIANCE_STEP_PROHIBITED_BITS;

        status = firme_base_service_abi_compliance_run_ns(0U, &result);
        if (expected_step == TEST_NO_FAILURE)
            assert(status == VAL_SUCCESS && result.status == ACS_STATUS_PASS);
        else
            assert(status == VAL_ERROR && result.status == ACS_STATUS_FAIL);
        assert(result.fail_step == expected_step);
    }
    assert(firme_base_service_abi_compliance_run_secure(0U, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL);
    assert(result.fail_step == SERVICE_ABI_COMPLIANCE_STEP_SERVICE_FORBIDDEN);

    attestation_implemented = VAL_FALSE;
    assert(firme_base_service_abi_compliance_run_ns(0U, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    assert(firme_base_service_abi_compliance_run_secure(0U, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
    assert(firme_base_service_abi_compliance_run_realm(0U, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS);
}

/** @brief Exercise the production catalog and Base rule bodies using ABI-visible fixtures. */
int main(void)
{
    check_attestation_catalog();
    check_realm_compliance();
    check_other_instances();
    puts("PASS: ALP4 attestation catalog, RAT_SIGN dependency, extension group and world checks");
    return VAL_SUCCESS;
}
"""


def main():
    """Compile and run the catalog and its rule consumers with undefined-behavior checks."""
    root = Path(__file__).resolve().parents[2]
    command = [os.environ.get("HOST_CC", "cc"), "-std=c11", "-Wall", "-Wextra", "-Werror",
               "-Wdeclaration-after-statement", "-ffunction-sections", "-fdata-sections",
               "-fsanitize=undefined", "-fno-sanitize-recover=all"]
    for include in ["val/include", "val_common/include", "test/base",
                    "platform/pal_baremetal/targets/fvp/include"]:
        command.extend(["-I", str(root / include)])
    with tempfile.TemporaryDirectory(prefix="build-service-native-", dir=root) as directory:
        temporary = Path(directory)
        source = temporary / "service_checks.c"
        binary = temporary / "service_checks"
        source.write_text(SOURCE)
        subprocess.run(command + [str(source), str(root / "val/src/val_firme_service.c"),
                                  str(root / "val/src/val_firme_abi.c"),
                                  "-Wl,--gc-sections", "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
