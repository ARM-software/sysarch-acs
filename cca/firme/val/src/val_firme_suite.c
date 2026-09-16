/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_suite.h"

#include "pal_platform.h"

/*
 * Per-rule world bodies live under test/<module>/.
 * Keep them local to the registry: callers should use the registry APIs
 * instead of including per-rule headers or declaring externs in many files.
 */
/* Chapter selection is controlled by build-time macros (from CMake/Make). */

#if FIRME_ENABLE_BASE
extern int firme_base_service_id_allocation_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_service_id_allocation_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_service_id_allocation_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_service_instance_presence_run_secure(uint64_t base_pa,
                                                           firme_rule_res_t *out);
extern int firme_base_service_instance_presence_run_realm(uint64_t base_pa,
                                                          firme_rule_res_t *out);
extern int firme_base_service_instance_presence_run_ns(uint64_t base_pa,
                                                       firme_rule_res_t *out);
extern int firme_base_service_presence_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_service_presence_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_service_presence_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_service_versions_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_service_versions_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_service_versions_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_service_base_dependency_run_secure(uint64_t base_pa,
                                                         firme_rule_res_t *out);
extern int firme_base_service_base_dependency_run_realm(uint64_t base_pa,
                                                        firme_rule_res_t *out);
extern int firme_base_service_base_dependency_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_service_version_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_service_version_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_service_version_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_service_feature_registers_run_secure(uint64_t base_pa,
                                                           firme_rule_res_t *out);
extern int firme_base_service_feature_registers_run_realm(uint64_t base_pa,
                                                          firme_rule_res_t *out);
extern int firme_base_service_feature_registers_run_ns(uint64_t base_pa,
                                                       firme_rule_res_t *out);
extern int firme_base_service_features_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_service_features_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_service_features_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_service_mandatory_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_service_mandatory_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_service_mandatory_run_ns(uint64_t base_pa, firme_rule_res_t *out);
#endif

#if FIRME_ENABLE_MEC
extern int firme_mec_common_mecid_width_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_mec_refresh_abi_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_mec_refresh_abi_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_mec_refresh_abi_run_ns(uint64_t base_pa, firme_rule_res_t *out);
#endif

#if FIRME_ENABLE_GPT
extern int firme_gpt_transition_feat_rme_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_transition_feat_rme_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_transition_feat_rme_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_transition_feat_gpc2_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_transition_feat_gdi_run_ns(uint64_t base_pa, firme_rule_res_t *out);
#endif

/* Global, self-describing registry (ordered by chapter). */
const firme_test_case_t g_firme_test_cases[] = {
#if FIRME_ENABLE_BASE
    /*
     * R0023 is a static interface-definition check with no executable test:
     * legal calls keep bits [63:8] SBZ and cannot distinguish a wider decoder.
     * R0024 validates the allocated/unallocated discovery ranges, but remains
     * unscheduled until service-specific behavior identifies every implemented
     * ID 1 through 5.
     */
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0025,
        .rule_str = "R0025",
        .suite = "firme",
        .test_name = "service_instance_presence",
        .desc = "FIRME service presence respects instance applicability",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "Service instance presence from Secure world",
        .check_realm = "Service instance presence from Realm world",
        .check_ns = "Service instance presence from NS world",
        .run_ns = firme_base_service_instance_presence_run_ns,
        .run_secure = firme_base_service_instance_presence_run_secure,
        .run_realm = firme_base_service_instance_presence_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0026,
        .rule_str = "R0026",
        .suite = "firme",
        .test_name = "base_service_presence",
        .desc = "FIRME Base service is implemented at every FIRME instance",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "Base service presence from Secure world",
        .check_realm = "Base service presence from Realm world",
        .check_ns = "Base service presence from NS world",
        .run_ns = firme_base_service_presence_run_ns,
        .run_secure = firme_base_service_presence_run_secure,
        .run_realm = firme_base_service_presence_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0034,
        .rule_str = "R0034",
        .suite = "firme",
        .test_name = "service_versions",
        .desc = "Implemented FIRME services report ALP3 service version 1.0",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "Service versions from Secure world",
        .check_realm = "Service versions from Realm world",
        .check_ns = "Service versions from NS world",
        .run_ns = firme_base_service_versions_run_ns,
        .run_secure = firme_base_service_versions_run_secure,
        .run_realm = firme_base_service_versions_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0035,
        .rule_str = "R0035",
        .suite = "firme",
        .test_name = "service_base_dependency",
        .desc = "Implemented FIRME services depend on Base service version 1.0",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "Base service dependency from Secure world",
        .check_realm = "Base service dependency from Realm world",
        .check_ns = "Base service dependency from NS world",
        .run_ns = firme_base_service_base_dependency_run_ns,
        .run_secure = firme_base_service_base_dependency_run_secure,
        .run_realm = firme_base_service_base_dependency_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0038,
        .rule_str = "R0038",
        .suite = "firme",
        .test_name = "service_version",
        .desc = "FIRME_SERVICE_VERSION returns service version or NOT_SUPPORTED",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "FIRME_SERVICE_VERSION from Secure world",
        .check_realm = "FIRME_SERVICE_VERSION from Realm world",
        .check_ns = "FIRME_SERVICE_VERSION from NS world",
        .run_ns = firme_base_service_version_run_ns,
        .run_secure = firme_base_service_version_run_secure,
        .run_realm = firme_base_service_version_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0051,
        .rule_str = "R0051",
        .suite = "firme",
        .test_name = "service_feature_registers",
        .desc = "Base service feature registers match Table 2.8 encodings",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "Base feature-register encoding from Secure world",
        .check_realm = "Base feature-register encoding from Realm world",
        .check_ns = "Base feature-register encoding from NS world",
        .run_ns = firme_base_service_feature_registers_run_ns,
        .run_secure = firme_base_service_feature_registers_run_secure,
        .run_realm = firme_base_service_feature_registers_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0053,
        .rule_str = "R0053",
        .suite = "firme",
        .test_name = "service_features",
        .desc = "FIRME_SERVICE_FEATURES returns requested register or NOT_SUPPORTED",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "FIRME_SERVICE_FEATURES from Secure world",
        .check_realm = "FIRME_SERVICE_FEATURES from Realm world",
        .check_ns = "FIRME_SERVICE_FEATURES from NS world",
        .run_ns = firme_base_service_features_run_ns,
        .run_secure = firme_base_service_features_run_secure,
        .run_realm = firme_base_service_features_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0055,
        .rule_str = "R0055",
        .suite = "firme",
        .test_name = "base_service_mandatory",
        .desc = "Base service discovery ABIs are mandatory at every FIRME instance",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "Mandatory Base service ABIs from Secure world",
        .check_realm = "Mandatory Base service ABIs from Realm world",
        .check_ns = "Mandatory Base service ABIs from NS world",
        .run_ns = firme_base_service_mandatory_run_ns,
        .run_secure = firme_base_service_mandatory_run_secure,
        .run_realm = firme_base_service_mandatory_run_realm,
    },
#endif
#if FIRME_ENABLE_MEC
    {
        .chapter = FIRME_CHAPTER_MEC,
        .rule_id = FIRME_RULE_R0206,
        .rule_str = "R0206",
        .suite = "firme",
        .test_name = "mec_common_mecid_width",
        .desc = "MECID management uses the common MECID width",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_realm = "Common MECID width enforcement from Realm world",
        .run_ns = 0,
        .run_secure = 0,
        .run_realm = firme_mec_common_mecid_width_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_MEC,
        .rule_id = FIRME_RULE_R0208,
        .rule_str = "R0208",
        .suite = "firme",
        .test_name = "mec_refresh_abi",
        .desc = "FIRME_MEC_REFRESH return status and parameter checks",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "FIRME_MEC_REFRESH from Secure world",
        .check_realm = "FIRME_MEC_REFRESH from Realm world",
        .check_ns = "FIRME_MEC_REFRESH from NS world",
        .run_ns = firme_mec_refresh_abi_run_ns,
        .run_secure = firme_mec_refresh_abi_run_secure,
        .run_realm = firme_mec_refresh_abi_run_realm,
    },
#endif
#if FIRME_ENABLE_GPT
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0085,
        .rule_str = "R0085",
        .suite = "firme",
        .test_name = "gpt_transition_feat_rme",
        .desc = "FEAT_RME permitted GM_GPI_SET transitions",
        .required_worlds_mask = FIRME_WORLD_MASK_NS,
        .optional_worlds_mask = FIRME_WORLD_MASK_SECURE | FIRME_WORLD_MASK_REALM,
        .check_secure = "Testing FIRME ABI: GM_GPI_SET from Secure world",
        .check_realm = "Testing FIRME ABI: GM_GPI_SET from Realm world",
        .check_ns = "Denied transitions + baseline enforcement (NS)",
        .run_ns = firme_gpt_transition_feat_rme_run_ns,
        .run_secure = firme_gpt_transition_feat_rme_run_secure,
        .run_realm = firme_gpt_transition_feat_rme_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0086,
        .rule_str = "R0086",
        .suite = "firme",
        .test_name = "gpt_transition_feat_gpc2",
        .desc = "FEAT_RME_GPC2 additional NS<->NSO transitions",
        .required_worlds_mask = FIRME_WORLD_MASK_NS,
        .optional_worlds_mask = 0u,
        .check_ns = "Testing FIRME ABI: GM_GPI_SET from NS world (NS<->NSO)",
        .run_ns = firme_gpt_transition_feat_gpc2_run_ns,
        .run_secure = 0,
        .run_realm = 0,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0089,
        .rule_str = "R0089",
        .suite = "firme",
        .test_name = "gpt_transition_feat_gdi",
        .desc = "FEAT_RME_GDI additional NSP/SA transitions",
        .required_worlds_mask = FIRME_WORLD_MASK_NS,
        .optional_worlds_mask = 0u,
        .check_ns = "Testing FIRME ABI: GM_GPI_SET from NS world (NSP/SA)",
        .run_ns = firme_gpt_transition_feat_gdi_run_ns,
        .run_secure = 0,
        .run_realm = 0,
    },
#endif
    /* Sentinel (keeps the array non-empty even if all chapters are disabled). */
    {0},
};

const uint32_t g_firme_test_case_count =
    (uint32_t)(sizeof(g_firme_test_cases) / sizeof(g_firme_test_cases[0])) - 1u;

/**
 *   @brief    - Return the FIRME-ACS test registry (ordered by chapter).
 *   @param    - out_count : Returned number of test cases.
 *   @return   - Pointer to a read-only array of test cases.
**/
const firme_test_case_t *val_firme_suite_cases(uint32_t *out_count)
{
    if (out_count)
        *out_count = g_firme_test_case_count;
    return &g_firme_test_cases[0];
}

/**
 *   @brief    - Lookup a test case by rule ID.
 *   @param    - rule_id : Rule ID.
 *   @return   - Pointer to the test case, or NULL if not found.
**/
const firme_test_case_t *val_firme_suite_find(firme_rule_id_t rule_id)
{
    for (uint32_t i = 0; i < g_firme_test_case_count; i++) {
        if (g_firme_test_cases[i].rule_id == rule_id)
            return &g_firme_test_cases[i];
    }
    return 0;
}

/**
 *   @brief    - Map a chapter ID to a printable string.
 *   @param    - chapter : Chapter identifier.
 *   @return   - Chapter name string.
**/
const char *val_firme_chapter_str(firme_chapter_id_t chapter)
{
    if (chapter == FIRME_CHAPTER_BASE)
        return "Base service";
    if (chapter == FIRME_CHAPTER_GPT)
        return "GPT management";
    if (chapter == FIRME_CHAPTER_MEC)
        return "MEC management";
    if (chapter == FIRME_CHAPTER_IDE)
        return "IDE management";
    if (chapter == FIRME_CHAPTER_ATTEST)
        return "Attestation";
    return "Other";
}

/**
 *   @brief    - Return the default GPT base PA for the given caller world.
 *   @param    - world : Target world selector.
 *   @return   - Platform base PA for GPT tests (0 if not configured).
**/
uint64_t val_firme_gpt_base_for_world(firme_acs_world_t world)
{
    if (world == FIRME_ACS_WORLD_SECURE)
        return (uint64_t)PLAT_FIRME_GPT_BASE_SECURE;
    if (world == FIRME_ACS_WORLD_REALM)
        return (uint64_t)PLAT_FIRME_GPT_BASE_REALM;
    return (uint64_t)PLAT_FIRME_GPT_BASE_NS;
}
