/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_suite.h"

#include "pal_platform.h"
#include "val_firme_abi_generated.h"
#include "val_firme_abi_catalog.h"

/*
 * Per-rule world bodies live under test/<module>/.
 * Keep them local to the registry: callers should use the registry APIs
 * instead of including per-rule headers or declaring externs in many files.
 */
/* Chapter selection is controlled by build-time macros (from CMake/Make). */

#if FIRME_ENABLE_BASE
extern int firme_base_smccc_version_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_smccc_version_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_smccc_version_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_smccc_sve_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_smccc_sve_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_smccc_sve_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_smccc_sme_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_smccc_sme_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_smccc_sme_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_smc64_calling_convention_run_secure(uint64_t base_pa,
                                                          firme_rule_res_t *out);
extern int firme_base_smc64_calling_convention_run_realm(uint64_t base_pa,
                                                         firme_rule_res_t *out);
extern int firme_base_smc64_calling_convention_run_ns(uint64_t base_pa,
                                                      firme_rule_res_t *out);
extern int firme_base_status_codes_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_status_codes_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_status_codes_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_feature_register_result_encoding_run_secure(uint64_t base_pa,
                                                                  firme_rule_res_t *out);
extern int firme_base_feature_register_result_encoding_run_realm(uint64_t base_pa,
                                                                 firme_rule_res_t *out);
extern int firme_base_feature_register_result_encoding_run_ns(uint64_t base_pa,
                                                              firme_rule_res_t *out);
extern int firme_base_feature_register_index_run_secure(uint64_t base_pa,
                                                        firme_rule_res_t *out);
extern int firme_base_feature_register_index_run_realm(uint64_t base_pa,
                                                       firme_rule_res_t *out);
extern int firme_base_feature_register_index_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_feature_register_abi_presence_run_secure(uint64_t base_pa,
                                                               firme_rule_res_t *out);
extern int firme_base_feature_register_abi_presence_run_realm(uint64_t base_pa,
                                                              firme_rule_res_t *out);
extern int firme_base_feature_register_abi_presence_run_ns(uint64_t base_pa,
                                                           firme_rule_res_t *out);
extern int firme_base_service_abi_compliance_run_secure(uint64_t base_pa,
                                                        firme_rule_res_t *out);
extern int firme_base_service_abi_compliance_run_realm(uint64_t base_pa,
                                                       firme_rule_res_t *out);
extern int firme_base_service_abi_compliance_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_reserved_input_registers_run_secure(uint64_t base_pa,
                                                           firme_rule_res_t *out);
extern int firme_base_reserved_input_registers_run_realm(uint64_t base_pa,
                                                          firme_rule_res_t *out);
extern int firme_base_reserved_input_registers_run_ns(uint64_t base_pa,
                                                       firme_rule_res_t *out);
extern int firme_base_reserved_input_bits_run_secure(uint64_t base_pa,
                                                     firme_rule_res_t *out);
extern int firme_base_reserved_input_bits_run_realm(uint64_t base_pa,
                                                    firme_rule_res_t *out);
extern int firme_base_reserved_input_bits_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_reserved_output_registers_run_secure(uint64_t base_pa,
                                                            firme_rule_res_t *out);
extern int firme_base_reserved_output_registers_run_realm(uint64_t base_pa,
                                                           firme_rule_res_t *out);
extern int firme_base_reserved_output_registers_run_ns(uint64_t base_pa,
                                                        firme_rule_res_t *out);
extern int firme_base_reserved_output_bits_run_secure(uint64_t base_pa,
                                                      firme_rule_res_t *out);
extern int firme_base_reserved_output_bits_run_realm(uint64_t base_pa,
                                                     firme_rule_res_t *out);
extern int firme_base_reserved_output_bits_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_service_version_compatibility_run_secure(uint64_t base_pa,
                                                               firme_rule_res_t *out);
extern int firme_base_service_version_compatibility_run_realm(uint64_t base_pa,
                                                              firme_rule_res_t *out);
extern int firme_base_service_version_compatibility_run_ns(uint64_t base_pa,
                                                           firme_rule_res_t *out);
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

#if FIRME_ENABLE_IDE
extern int firme_ide_keyset_id_bitmap_encoding_run_realm(uint64_t base_pa,
                                                          firme_rule_res_t *out);
extern int firme_ide_cxl_key_set_unused_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_ide_cxl_stream_id_zero_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_ide_cxl_substream_id_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_ide_keyset_prog_status_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_ide_keyset_go_status_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_ide_keyset_stop_status_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_ide_keyset_prog_post(const firme_acs_call_res_t *res_ns,
                                     const firme_acs_call_res_t *res_secure,
                                     const firme_acs_call_res_t *res_realm,
                                     firme_acs_call_res_t *overall);
extern int firme_ide_keyset_go_post(const firme_acs_call_res_t *res_ns,
                                   const firme_acs_call_res_t *res_secure,
                                   const firme_acs_call_res_t *res_realm,
                                   firme_acs_call_res_t *overall);
extern int firme_ide_keyset_stop_post(const firme_acs_call_res_t *res_ns,
                                     const firme_acs_call_res_t *res_secure,
                                     const firme_acs_call_res_t *res_realm,
                                     firme_acs_call_res_t *overall);
extern int firme_ide_keyset_poll_status_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_ide_secure_caller_not_supported_run_secure(uint64_t base_pa,
                                                            firme_rule_res_t *out);
extern int firme_ide_feature_register_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_ide_feature_register_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_ide_abi_compliance_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_ide_abi_compliance_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_ide_abi_compliance_run_ns(uint64_t base_pa, firme_rule_res_t *out);
#endif

#if FIRME_ENABLE_MEC
extern int firme_mec_common_mecid_width_entry(firme_acs_world_t world, uint32_t num_pe,
                                             val_test_run_payload_t run_payload,
                                             firme_rule_res_t *out);
extern int firme_mec_common_mecid_width_payload(firme_acs_world_t world,
                                               uint32_t phase, uint32_t argument,
                                               firme_rule_res_t *out);
extern int firme_mec_refresh_abi_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_mec_refresh_abi_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_mec_refresh_abi_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_mec_feature_registers_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_mec_abi_compliance_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_mec_abi_compliance_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_mec_abi_compliance_run_realm(uint64_t base_pa, firme_rule_res_t *out);
#endif

#if FIRME_ENABLE_GPT
extern int firme_gpt_granule_management_feature_registers_run_secure(uint64_t base_pa,
                                                                     firme_rule_res_t *out);
extern int firme_gpt_granule_management_feature_registers_run_realm(uint64_t base_pa,
                                                                    firme_rule_res_t *out);
extern int firme_gpt_granule_management_feature_registers_run_ns(uint64_t base_pa,
                                                                 firme_rule_res_t *out);
extern int firme_gpt_granule_management_abi_compliance_run_secure(uint64_t base_pa,
                                                                  firme_rule_res_t *out);
extern int firme_gpt_granule_management_abi_compliance_run_realm(uint64_t base_pa,
                                                                 firme_rule_res_t *out);
extern int firme_gpt_granule_management_abi_compliance_run_ns(uint64_t base_pa,
                                                              firme_rule_res_t *out);
extern int firme_gpt_granule_size_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_base_alignment_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_base_alignment_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_same_gpi_transition_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_same_gpi_transition_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_same_gpi_transition_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_transition_feat_rme_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_transition_feat_rme_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_transition_feat_rme_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_transition_feat_gpc2_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_transition_feat_gdi_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_start_granule_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_start_granule_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_start_granule_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_success_granule_count_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_success_granule_count_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_gpi_set_progress_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_gpi_set_progress_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_success_count_nonzero_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_success_count_nonzero_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_invalid_parameters_status_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_denied_status_run_ns(uint64_t base_pa, firme_rule_res_t *out);
#endif

/* Global, self-describing registry (ordered by chapter). */
const firme_test_case_t g_firme_test_cases[] = {
#if FIRME_ENABLE_BASE
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0007,
        .rule_str = "R0007",
        .suite = "firme",
        .test_name = "smccc_version",
        .desc = "FIRME ABIs comply with SMCCC version 1.2 or later",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "SMCCC version from Secure world",
        .check_realm = "SMCCC version from Realm world",
        .check_ns = "SMCCC version from NS world",
        .run_ns = firme_base_smccc_version_run_ns,
        .run_secure = firme_base_smccc_version_run_secure,
        .run_realm = firme_base_smccc_version_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0008,
        .rule_str = "R0008",
        .suite = "firme",
        .test_name = "smccc_sve",
        .desc = "FEAT_SVE systems comply with SMCCC version 1.3 or later",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "SMCCC SVE rule from Secure world",
        .check_realm = "SMCCC SVE rule from Realm world",
        .check_ns = "SMCCC SVE rule from NS world",
        .run_ns = firme_base_smccc_sve_run_ns,
        .run_secure = firme_base_smccc_sve_run_secure,
        .run_realm = firme_base_smccc_sve_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0009,
        .rule_str = "R0009",
        .suite = "firme",
        .test_name = "smccc_sme",
        .desc = "FEAT_SME systems comply with SMCCC version 1.4 or later",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "SMCCC SME rule from Secure world",
        .check_realm = "SMCCC SME rule from Realm world",
        .check_ns = "SMCCC SME rule from NS world",
        .run_ns = firme_base_smccc_sme_run_ns,
        .run_secure = firme_base_smccc_sme_run_secure,
        .run_realm = firme_base_smccc_sme_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0010,
        .rule_str = "R0010",
        .suite = "firme",
        .test_name = "smc64_calling_convention",
        .desc = "FIRME ABIs use the SMC64 calling convention",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "SMC64 convention from Secure world",
        .check_realm = "SMC64 convention from Realm world",
        .check_ns = "SMC64 convention from NS world",
        .run_ns = firme_base_smc64_calling_convention_run_ns,
        .run_secure = firme_base_smc64_calling_convention_run_secure,
        .run_realm = firme_base_smc64_calling_convention_run_realm,
    },
    /*
     * R0011 validates statuses observed from every advertised ABI against the
     * status set permitted for that ABI. It remains unscheduled until all
     * service-specific test paths feed the shared validator, so one safe probe
     * per advertised ABI is not misreported as complete status-path coverage.
     */
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0012,
        .rule_str = "R0012",
        .suite = "firme",
        .test_name = "reserved_input_registers",
        .desc = "Unused FIRME input registers are reserved and SBZ",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "Reserved input registers from Secure world",
        .check_realm = "Reserved input registers from Realm world",
        .check_ns = "Reserved input registers from NS world",
        .run_ns = firme_base_reserved_input_registers_run_ns,
        .run_secure = firme_base_reserved_input_registers_run_secure,
        .run_realm = firme_base_reserved_input_registers_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0013,
        .rule_str = "R0013",
        .suite = "firme",
        .test_name = "reserved_input_bits",
        .desc = "Unused FIRME input bits are reserved and SBZ",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "Reserved input bits from Secure world",
        .check_realm = "Reserved input bits from Realm world",
        .check_ns = "Reserved input bits from NS world",
        .run_ns = firme_base_reserved_input_bits_run_ns,
        .run_secure = firme_base_reserved_input_bits_run_secure,
        .run_realm = firme_base_reserved_input_bits_run_realm,
    },
    /*
     * R0014 and R0015 have caller-side poisoning self-tests, but are not
     * scheduled as DUT compliance tests. Full coverage also requires every
     * service/status output shape and a callee non-disclosure security audit.
     */
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
    /*
     * The strict R0031 body is implemented in
     * test/base/specification_version_policy.c. Keep its registry entry gated
     * until consecutive specification-release metadata is supplied through
     * the policy probe. It validates publication metadata, not DUT behavior.
     */
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0033,
        .rule_str = "R0033",
        .suite = "firme",
        .test_name = "service_version_compatibility",
        .desc = "Service major and minor versions follow compatibility rules",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "Service version compatibility from Secure world",
        .check_realm = "Service version compatibility from Realm world",
        .check_ns = "Service version compatibility from NS world",
        .run_ns = firme_base_service_version_compatibility_run_ns,
        .run_secure = firme_base_service_version_compatibility_run_secure,
        .run_realm = firme_base_service_version_compatibility_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0034,
        .rule_str = "R0034",
        .suite = "firme",
        .test_name = "service_versions",
        .desc = "Implemented FIRME services report ALP4 service version 1.0",
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
    /*
     * R0041 is a static interface-table requirement with no executable test:
     * SERVICE_FEATURES exposes register contents and indices, not observable
     * register-size metadata.
     */
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0044,
        .rule_str = "R0044",
        .suite = "firme",
        .test_name = "feature_register_result_encoding",
        .desc = "SMC64 result registers encode feature register bit ranges",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "Feature register result encoding from Secure world",
        .check_realm = "Feature register result encoding from Realm world",
        .check_ns = "Feature register result encoding from NS world",
        .run_ns = firme_base_feature_register_result_encoding_run_ns,
        .run_secure = firme_base_feature_register_result_encoding_run_secure,
        .run_realm = firme_base_feature_register_result_encoding_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0047,
        .rule_str = "R0047",
        .suite = "firme",
        .test_name = "feature_register_index",
        .desc = "Feature register indices are 8-bit unsigned integers",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "Feature register index from Secure world",
        .check_realm = "Feature register index from Realm world",
        .check_ns = "Feature register index from NS world",
        .run_ns = firme_base_feature_register_index_run_ns,
        .run_secure = firme_base_feature_register_index_run_secure,
        .run_realm = firme_base_feature_register_index_run_realm,
    },
    /*
     * R0048 is implemented in test/base/feature_register_abi_presence.c,
     * but is intentionally not scheduled until the reference DUT advertises
     * all ALP4 mandatory ABI bits for each implemented service. Current TF-A
     * reports Granule Management feature register 0 as 0x1, missing the
     * mandatory OP_CONTINUE bit, so this rule currently fails by design.
     */
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
    /*
     * R0054 is implemented in test/base/service_abi_compliance.c, but is
     * intentionally not scheduled until the reference DUT advertises all
     * ALP4 mandatory ABI bits for each implemented service. Do not relax the
     * strict rule body to match older DUT behavior; re-enable the registry
     * entry when the DUT claims the applicable ALP4 semantics.
     */
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
    /*
     * Strict R0064 and R0065 bodies are implemented in
     * test/base/max_concurrent_operations.c. They remain registry-gated while
     * current TF-A advertises IDE key management without the ALP4 feature
     * register 1 required by R0064. R0065 additionally reports SKIP for a
     * non-zero advertised limit until retained operations can be driven
     * collectively across ABIs and FIRME instances.
     */
    /*
     * Strict R0070, R0071, R0076 and R0077 bodies are implemented in
     * test/base/shared_buffer_*.c. Keep their registry entries gated
     * until an Attestation or Integrated Device adapter overrides the shared-
     * buffer probe and supplies caller/EL3 mapping, PAS/GPI and ABI evidence.
     * Do not replace the strict checks with Base feature-register inference.
     */
    /*
     * Strict R0080, R0081 and R0082 bodies are implemented in
     * test/base/partial_retrieval_*.c. Keep their registry entries gated until
     * an Attestation or Integrated Device adapter drives last, intermediate
     * and no-data retrieval paths through a real shared-buffer ABI.
     */
#endif
#if FIRME_ENABLE_IDE
    /*
     * Strict R0211/R0212 bodies are implemented in
     * test/ide/ecam_keyset_namespace.c and keyset_id_uniqueness.c.
     * Keep them registry-gated until PAL provides two independently
     * observable Root Port/endpoint paths and protected-traffic evidence.
     * ABI SUCCESS alone cannot prove that keyset namespaces do not alias.
     */
    {
        .chapter = FIRME_CHAPTER_IDE,
        .rule_id = FIRME_RULE_R0205,
        .rule_str = "R0205",
        .suite = "firme",
        .test_name = "ide_keyset_id_bitmap_encoding",
        .desc = "IDE keyset ID fields use the DEN0149 bitmap encoding",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_realm = "Validate IDE Keyset ID field layout from Realm world",
        .run_ns = 0,
        .run_secure = 0,
        .run_realm = firme_ide_keyset_id_bitmap_encoding_run_realm,
    },
    /*
     * R0207, R0209 and R0210 remain implemented but temporarily registry-gated.
     * The CI TF-A rejects CXL.cachemem IDE type 3 with INVALID_PARAMETERS
     * during valid baseline setup; the paired CXL firmware/model is absent.
     * Restore these entries with final CI qualification and golden-log updates.
     * Keep the strict test expectations; this temporary gate is not coverage.
     */
    {
        .chapter = FIRME_CHAPTER_IDE,
        .rule_id = FIRME_RULE_R0214,
        .rule_str = "R0214",
        .suite = "firme",
        .test_name = "ide_keyset_prog_status",
        .desc = "FIRME_IDE_KEYSET_PROG status and observable key programming",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_realm = "Validate KEYSET_PROG success and negative statuses",
        .run_ns = 0,
        .run_secure = 0,
        .run_realm = firme_ide_keyset_prog_status_run_realm,
        .post_run = firme_ide_keyset_prog_post,
    },
    {
        .chapter = FIRME_CHAPTER_IDE,
        .rule_id = FIRME_RULE_R0216,
        .rule_str = "R0216",
        .suite = "firme",
        .test_name = "ide_keyset_go_status",
        .desc = "FIRME_IDE_KEYSET_GO status and observable traffic enablement",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_realm = "Validate KEYSET_GO success and negative statuses",
        .run_ns = 0,
        .run_secure = 0,
        .run_realm = firme_ide_keyset_go_status_run_realm,
        .post_run = firme_ide_keyset_go_post,
    },
    {
        .chapter = FIRME_CHAPTER_IDE,
        .rule_id = FIRME_RULE_R0218,
        .rule_str = "R0218",
        .suite = "firme",
        .test_name = "ide_keyset_stop_status",
        .desc = "FIRME_IDE_KEYSET_STOP status and observable traffic cessation",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_realm = "Validate KEYSET_STOP success and negative statuses",
        .run_ns = 0,
        .run_secure = 0,
        .run_realm = firme_ide_keyset_stop_status_run_realm,
        .post_run = firme_ide_keyset_stop_post,
    },
    {
        .chapter = FIRME_CHAPTER_IDE,
        .rule_id = FIRME_RULE_R0225,
        .rule_str = "R0225",
        .suite = "firme",
        .test_name = "ide_keyset_poll_status",
        .desc = "FIRME_IDE_KEYSET_POLL status handling",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_realm = "Validate KEYSET_POLL completion and negative statuses",
        .run_ns = 0,
        .run_secure = 0,
        .run_realm = firme_ide_keyset_poll_status_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_IDE,
        .rule_id = FIRME_RULE_R0227,
        .rule_str = "R0227",
        .suite = "firme",
        .test_name = "ide_secure_caller_not_supported",
        .desc = "Secure FIRME instance does not support IDE key management",
        .required_worlds_mask = FIRME_WORLD_MASK_SECURE,
        .optional_worlds_mask = 0u,
        .check_secure = "Invoke IDE key management ABIs from Secure world",
        .run_ns = 0,
        .run_secure = firme_ide_secure_caller_not_supported_run_secure,
        .run_realm = 0,
    },
    /* R0231: Re-enable after TF-A implements and qualifies ALP4 IDE register 1.
     * The strict NS/Realm bodies also check register 0 and unallocated indices.
     * Do not treat missing mandatory register 1 as an optional feature.
     */
    {
        .chapter = FIRME_CHAPTER_IDE,
        .rule_id = FIRME_RULE_R0232,
        .rule_str = "R0232",
        .suite = "firme",
        .test_name = "ide_abi_compliance",
        .desc = "IDE key management ABIs follow Table 4.2 compliance",
        .required_worlds_mask = FIRME_WORLD_MASK_SECURE | FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = FIRME_WORLD_MASK_NS,
        .check_secure = "Check IDE service absence from Secure world",
        .check_realm = "Check mandatory IDE ABIs from Realm world",
        .check_ns = "Check mandatory IDE ABIs from NS world",
        .run_ns = firme_ide_abi_compliance_run_ns,
        .run_secure = firme_ide_abi_compliance_run_secure,
        .run_realm = firme_ide_abi_compliance_run_realm,
    },
#endif
#if FIRME_ENABLE_MEC
    /*
     * Strict ALP4 bodies for R0236 and R0238 remain registry-gated because
     * the default FVP TF-A refresh hook does not replace the MPE context.
     * Local opt-in functional-model qualification does not qualify the
     * default DUT; re-enable only for a qualified deployment.
     */
    {
        .chapter = FIRME_CHAPTER_MEC,
        .rule_id = FIRME_RULE_R0239,
        .rule_str = "R0239",
        .suite = "firme",
        .test_name = "mec_common_mecid_width",
        .desc = "MECID management uses the common MECID width",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_realm = "Common MECID width enforcement from Realm world",
        .run_ns = 0,
        .run_secure = 0,
        .num_pe = VAL_TEST_ALL_PES,
        .mp_entry = firme_mec_common_mecid_width_entry,
        .mp_payload = firme_mec_common_mecid_width_payload,
    },
    {
        .chapter = FIRME_CHAPTER_MEC,
        .rule_id = FIRME_RULE_R0241,
        .rule_str = "R0241",
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
    {
        .chapter = FIRME_CHAPTER_MEC,
        .rule_id = FIRME_RULE_R0245,
        .rule_str = "R0245",
        .suite = "firme",
        .test_name = "mec_feature_registers",
        .desc = "MEC feature registers follow ALP4 Table 5.1",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_realm = "Validate MEC feature register indices and fields",
        .run_realm = firme_mec_feature_registers_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_MEC,
        .rule_id = FIRME_RULE_R0246,
        .rule_str = "R0246",
        .suite = "firme",
        .test_name = "mec_abi_compliance",
        .desc = "MEC_REFRESH follows the ALP4 Table 5.2 caller policy",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_ns = "MEC_REFRESH is unavailable from Normal world",
        .check_secure = "MEC_REFRESH is unavailable from Secure world",
        .check_realm = "MEC_REFRESH is mandatory in Realm world",
        .run_ns = firme_mec_abi_compliance_run_ns,
        .run_secure = firme_mec_abi_compliance_run_secure,
        .run_realm = firme_mec_abi_compliance_run_realm,
    },
#endif
#if FIRME_ENABLE_GPT
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0194,
        .rule_str = "R0194",
        .suite = "firme",
        .test_name = "granule_management_feature_registers",
        .desc = "Granule Management feature registers use specified indices and encodings",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "Validate GM feature registers from Secure world",
        .check_realm = "Validate GM feature registers from Realm world",
        .check_ns = "Validate GM feature registers from NS world",
        .run_ns = firme_gpt_granule_management_feature_registers_run_ns,
        .run_secure = firme_gpt_granule_management_feature_registers_run_secure,
        .run_realm = firme_gpt_granule_management_feature_registers_run_realm,
    },
    /*
     * R0195 is implemented in test/gpt/granule_management_abi_compliance.c,
     * but is intentionally not scheduled until the reference DUT advertises
     * all ALP4 mandatory Granule Management ABIs. Current TF-A reports GM
     * feature register 0 as 0x1, missing FIRME_GM_GPI_OP_CONTINUE (bit 1),
     * so this rule currently fails by design.
     */
    /*
     * R0102, R0103, R0134, R0138-R0141, R0356, and R0357 have strict
     * GPT_INFO bodies or generated interface plans. They remain outside the
     * runtime registry because current TF-A neither advertises nor implements
     * mandatory FIRME_GM_GPT_INFO. Re-enable the handwritten rule entries and
     * the two generated plans together when the DUT advertises feature
     * register 0 bit 4.
     */
    /*
     * R0114, R0115, R0118, R0122, R0124, R0126, R0129, and R0130 have
     * strict bodies for continuation and architected status behavior. They
     * remain registry-gated because current TF-A does not advertise
     * FIRME_GM_GPI_OP_CONTINUE and the default FVP PAL cannot create the
     * deterministic BUSY, allocation-failure, or conflict scenarios.
     * R0144 also needs GPT_INFO and a real, synchronized protected-range
     * state change; MP worker support alone is not sufficient.
     */
    /*
     * R0156, R0161, R0167, R0168, R0170, R0171, R0173, R0179, R0180,
     * R0184, R0185, R0186, R0188, R0189, and R0191 have strict one-rule
     * L1 GPT lifecycle bodies. They remain registry-gated because current
     * TF-A does not advertise the optional CREATE/DESTROY ABI pair and the
     * default FVP PAL cannot provide deterministic lifecycle, bridge, scrub,
     * complete-range GPI observation, or retained-operation scenarios.
     */
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0086,
        .rule_str = "R0086",
        .suite = "firme",
        .test_name = "gpt_granule_size",
        .desc = "GPI transition granule size matches advertised PGS",
        .required_worlds_mask = FIRME_WORLD_MASK_NS,
        .optional_worlds_mask = 0u,
        .check_ns = "Validate GM feature register 1 PGS against PAL granule size",
        .run_ns = firme_gpt_granule_size_run_ns,
        .run_secure = 0,
        .run_realm = 0,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0090,
        .rule_str = "R0090",
        .suite = "firme",
        .test_name = "gpt_base_alignment",
        .desc = "GM_GPI_SET Base address is aligned to advertised PGS",
        .required_worlds_mask = FIRME_WORLD_MASK_SECURE | FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "Secure GM_GPI_SET aligned and misaligned Base handling",
        .check_realm = "Realm GM_GPI_SET aligned and misaligned Base handling",
        .run_ns = 0,
        .run_secure = firme_gpt_base_alignment_run_secure,
        .run_realm = firme_gpt_base_alignment_run_realm,
    },
    /*
     * Strict ALP4 bodies for R0092 and R0123 remain
     * registry-gated while current TF-A returns non-conformant status values
     * for those scenarios. Re-enable them after the corresponding DUT fixes.
     */
    /*
     * R0094, R0095, R0098 and R0120 remain implemented but temporarily
     * registry-gated after strict setup, negative-case and cleanup checks
     * exposed current TF-A behavior. Same-GPI NS setup/restoration returns
     * DENIED instead of successful nop progress (R0092/I0121). With GPC2/GDI
     * absent, unallocated NSO/NSP targets return DENIED instead of
     * INVALID_PARAMETERS (D0091/R0123). Re-enable these entries only after
     * all required caller paths, absent/present feature cases and cleanup
     * pass unchanged strict expectations. Do not treat this gate as coverage.
     */
    /*
     * R0111, R0113 and R0117 retain strict bodies but are temporarily gated.
     * The CI TF-A returns DENIED during NS setup/restoration with the GDI
     * profile, so the requested progress checks cannot complete successfully.
     * Restore the entries with qualified DUT behavior and final CI/golden-log
     * updates; do not accept failed setup or cleanup to force a passing run.
     */
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0127,
        .rule_str = "R0127",
        .suite = "firme",
        .test_name = "gpt_denied_status",
        .desc = "GM_GPI_SET denied transition returns DENIED",
        .required_worlds_mask = FIRME_WORLD_MASK_NS,
        .optional_worlds_mask = 0u,
        .check_ns = "Invoke non-permitted NS->Root transition and expect DENIED",
        .run_ns = firme_gpt_denied_status_run_ns,
        .run_secure = 0,
        .run_realm = 0,
    },
#endif
#include "val_firme_abi_registry.inc"
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
