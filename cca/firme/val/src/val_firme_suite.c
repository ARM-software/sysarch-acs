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
extern int firme_base_specification_version_policy_run_ns(uint64_t base_pa,
    firme_rule_res_t *out);
extern int firme_base_shared_buffer_mapping_attributes_run_ns(uint64_t base_pa,
    firme_rule_res_t *out);
extern int firme_base_shared_buffer_mapping_attributes_run_realm(uint64_t base_pa,
    firme_rule_res_t *out);
extern int firme_base_shared_buffer_pas_gpi_run_ns(uint64_t base_pa,
    firme_rule_res_t *out);
extern int firme_base_shared_buffer_pas_gpi_run_realm(uint64_t base_pa,
    firme_rule_res_t *out);
extern int firme_base_shared_buffer_size_bounds_run_ns(uint64_t base_pa,
    firme_rule_res_t *out);
extern int firme_base_shared_buffer_size_bounds_run_realm(uint64_t base_pa,
    firme_rule_res_t *out);
extern int firme_base_shared_buffer_alignment_run_ns(uint64_t base_pa,
    firme_rule_res_t *out);
extern int firme_base_shared_buffer_alignment_run_realm(uint64_t base_pa,
    firme_rule_res_t *out);
extern int firme_base_partial_retrieval_last_chunk_run_ns(uint64_t base_pa,
    firme_rule_res_t *out);
extern int firme_base_partial_retrieval_last_chunk_run_realm(uint64_t base_pa,
    firme_rule_res_t *out);
extern int firme_base_partial_retrieval_intermediate_chunk_run_ns(uint64_t base_pa,
    firme_rule_res_t *out);
extern int firme_base_partial_retrieval_intermediate_chunk_run_realm(uint64_t base_pa,
    firme_rule_res_t *out);
extern int firme_base_partial_retrieval_no_chunk_run_ns(uint64_t base_pa,
    firme_rule_res_t *out);
extern int firme_base_partial_retrieval_no_chunk_run_realm(uint64_t base_pa,
    firme_rule_res_t *out);
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
extern int firme_base_max_concurrent_r0064_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_max_concurrent_r0064_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_max_concurrent_r0064_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_max_concurrent_r0065_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_max_concurrent_r0065_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_max_concurrent_r0065_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_base_max_concurrent_r0065_post(const firme_acs_call_res_t *res_ns,
                                                 const firme_acs_call_res_t *res_secure,
                                                 const firme_acs_call_res_t *res_realm,
                                                 firme_acs_call_res_t *overall);
#endif

#if FIRME_ENABLE_IDE
extern int firme_ide_ecam_keyset_namespace_run_ns(uint64_t base_pa,
    firme_rule_res_t *out);
extern int firme_ide_ecam_keyset_namespace_run_realm(uint64_t base_pa,
    firme_rule_res_t *out);
extern int firme_ide_keyset_id_uniqueness_run_ns(uint64_t base_pa,
    firme_rule_res_t *out);
extern int firme_ide_keyset_id_uniqueness_run_realm(uint64_t base_pa,
    firme_rule_res_t *out);
extern int firme_ide_keyset_id_uniqueness_run_host_realm(uint64_t base_pa,
    firme_rule_res_t *out);
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
extern int firme_mec_context_refresh_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_mec_ciphertext_decorrelation_run_realm(uint64_t base_pa,
                                                        firme_rule_res_t *out);
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
extern int firme_gpt_protected_range_base_alignment_run_ns(uint64_t base_pa,
                                                           firme_rule_res_t *out);
extern int firme_gpt_protected_range_base_alignment_run_secure(uint64_t base_pa,
                                                               firme_rule_res_t *out);
extern int firme_gpt_protected_range_base_alignment_run_realm(uint64_t base_pa,
                                                              firme_rule_res_t *out);
extern int firme_gpt_protected_range_size_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_protected_range_size_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_protected_range_size_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_management_owner_stability_run_secure(uint64_t base_pa,
                                                           firme_rule_res_t *out);
extern int firme_gpt_management_owner_stability_run_realm(uint64_t base_pa,
                                                          firme_rule_res_t *out);
extern int firme_gpt_par_state_encoding_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_par_state_encoding_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_par_state_encoding_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_info_status_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_info_status_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_info_status_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_info_success_outputs_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_info_success_outputs_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_info_success_outputs_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_info_output_validity_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_info_output_validity_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_info_output_validity_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_op_continue_start_granule_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_op_continue_start_granule_run_secure(uint64_t base_pa,
                                                          firme_rule_res_t *out);
extern int firme_gpt_op_continue_start_granule_run_realm(uint64_t base_pa,
                                                         firme_rule_res_t *out);
extern int firme_gpt_op_continue_granule_count_run_ns(uint64_t base_pa,
                                                      firme_rule_res_t *out);
extern int firme_gpt_op_continue_granule_count_run_secure(uint64_t base_pa,
                                                          firme_rule_res_t *out);
extern int firme_gpt_op_continue_granule_count_run_realm(uint64_t base_pa,
                                                         firme_rule_res_t *out);
extern int firme_gpt_op_continue_progress_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_op_continue_progress_run_secure(uint64_t base_pa,
                                                     firme_rule_res_t *out);
extern int firme_gpt_op_continue_progress_run_realm(uint64_t base_pa,
                                                    firme_rule_res_t *out);
extern int firme_gpt_not_supported_status_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_not_supported_status_run_secure(uint64_t base_pa,
                                                    firme_rule_res_t *out);
extern int firme_gpt_not_supported_status_run_realm(uint64_t base_pa,
                                                   firme_rule_res_t *out);
extern int firme_gpt_busy_retry_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_busy_retry_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_busy_retry_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_l1_allocation_not_found_run_ns(uint64_t base_pa,
                                                    firme_rule_res_t *out);
extern int firme_gpt_l1_allocation_not_found_run_secure(uint64_t base_pa,
                                                        firme_rule_res_t *out);
extern int firme_gpt_l1_allocation_not_found_run_realm(uint64_t base_pa,
                                                       firme_rule_res_t *out);
extern int firme_gpt_operation_conflict_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_operation_conflict_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_operation_conflict_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_stateful_incomplete_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_stateful_incomplete_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_stateful_incomplete_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_info_concurrent_state_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_info_concurrent_state_run_secure(uint64_t base_pa,
                                                     firme_rule_res_t *out);
extern int firme_gpt_info_concurrent_state_run_realm(uint64_t base_pa,
                                                    firme_rule_res_t *out);
extern int firme_gpt_l1_bridge_object_properties_run_realm(uint64_t base_pa,
                                                            firme_rule_res_t *out);
extern int firme_gpt_l1_create_transition_policy_run_ns(uint64_t base_pa,
                                                         firme_rule_res_t *out);
extern int firme_gpt_l1_create_transition_policy_run_secure(uint64_t base_pa,
                                                             firme_rule_res_t *out);
extern int firme_gpt_l1_create_transition_policy_run_realm(uint64_t base_pa,
                                                            firme_rule_res_t *out);
extern int firme_gpt_l1_create_status_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_l1_create_status_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_l1_create_status_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_l1_create_range_gpi_stability_run_realm(uint64_t base_pa,
                                                              firme_rule_res_t *out);
extern int firme_gpt_l1_create_success_object_gpi_run_realm(uint64_t base_pa,
                                                             firme_rule_res_t *out);
extern int firme_gpt_l1_create_incomplete_object_gpi_run_realm(uint64_t base_pa,
                                                                firme_rule_res_t *out);
extern int firme_gpt_l1_create_denied_no_effect_run_realm(uint64_t base_pa,
                                                           firme_rule_res_t *out);
extern int firme_gpt_l1_destroy_transition_policy_run_ns(uint64_t base_pa,
                                                          firme_rule_res_t *out);
extern int firme_gpt_l1_destroy_transition_policy_run_secure(uint64_t base_pa,
                                                              firme_rule_res_t *out);
extern int firme_gpt_l1_destroy_transition_policy_run_realm(uint64_t base_pa,
                                                             firme_rule_res_t *out);
extern int firme_gpt_l1_destroy_scrub_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_l1_destroy_status_run_ns(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_l1_destroy_status_run_secure(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_l1_destroy_status_run_realm(uint64_t base_pa, firme_rule_res_t *out);
extern int firme_gpt_l1_destroy_success_output_run_realm(uint64_t base_pa,
                                                          firme_rule_res_t *out);
extern int firme_gpt_l1_destroy_range_gpi_stability_run_realm(uint64_t base_pa,
                                                               firme_rule_res_t *out);
extern int firme_gpt_l1_destroy_success_object_gpi_run_realm(uint64_t base_pa,
                                                              firme_rule_res_t *out);
extern int firme_gpt_l1_destroy_incomplete_object_gpi_run_realm(uint64_t base_pa,
                                                                 firme_rule_res_t *out);
extern int firme_gpt_l1_single_in_progress_operation_run_realm(uint64_t base_pa,
                                                                firme_rule_res_t *out);
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
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0011,
        .rule_str = "R0011",
        .suite = "firme",
        .test_name = "status_codes",
        .desc = "Architected statuses from implemented service paths",
        .required_worlds_mask = FIRME_WORLD_MASK_NS |
                                FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_ns = "Architected statuses from implemented service paths (ns)",
        .run_ns = firme_base_status_codes_run_ns,
        .check_secure = "Architected statuses from implemented service paths (secure)",
        .run_secure = firme_base_status_codes_run_secure,
        .check_realm = "Architected statuses from implemented service paths (realm)",
        .run_realm = firme_base_status_codes_run_realm,
    },
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
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0014,
        .rule_str = "R0014",
        .suite = "firme",
        .test_name = "reserved_output_registers",
        .desc = "Unused output-register caller and callee evidence",
        .required_worlds_mask = FIRME_WORLD_MASK_NS |
                                FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_ns = "Unused output-register caller and callee evidence (ns)",
        .run_ns = firme_base_reserved_output_registers_run_ns,
        .check_secure = "Unused output-register caller and callee evidence (secure)",
        .run_secure = firme_base_reserved_output_registers_run_secure,
        .check_realm = "Unused output-register caller and callee evidence (realm)",
        .run_realm = firme_base_reserved_output_registers_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0015,
        .rule_str = "R0015",
        .suite = "firme",
        .test_name = "reserved_output_bits",
        .desc = "Unused output-bit caller and callee evidence",
        .required_worlds_mask = FIRME_WORLD_MASK_NS |
                                FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_ns = "Unused output-bit caller and callee evidence (ns)",
        .run_ns = firme_base_reserved_output_bits_run_ns,
        .check_secure = "Unused output-bit caller and callee evidence (secure)",
        .run_secure = firme_base_reserved_output_bits_run_secure,
        .check_realm = "Unused output-bit caller and callee evidence (realm)",
        .run_realm = firme_base_reserved_output_bits_run_realm,
    },
    /* R0023 is Architect-excluded: legal inputs cannot establish internal type width. */
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0024,
        .rule_str = "R0024",
        .suite = "firme",
        .test_name = "service_id_allocation",
        .desc = "Allocated service IDs identify prescribed services",
        .required_worlds_mask = FIRME_WORLD_MASK_NS |
                                FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_ns = "Allocated service IDs identify prescribed services (ns)",
        .run_ns = firme_base_service_id_allocation_run_ns,
        .check_secure = "Allocated service IDs identify prescribed services (secure)",
        .run_secure = firme_base_service_id_allocation_run_secure,
        .check_realm = "Allocated service IDs identify prescribed services (realm)",
        .run_realm = firme_base_service_id_allocation_run_realm,
    },
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
    /* Publication metadata is reported separately from DUT observations. */
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0031,
        .rule_str = "R0031",
        .suite = "firme",
        .test_name = "specification_version_policy",
        .desc = "Publication policy for specification and service versions",
        .required_worlds_mask = FIRME_WORLD_MASK_NS,
        .optional_worlds_mask = 0u,
        .check_ns = "Publication policy for specification and service versions (ns)",
        .run_ns = firme_base_specification_version_policy_run_ns,
    },
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
     * R0041 is an Architect-agreed DUT-validation exclusion:
     * SERVICE_FEATURES exposes register contents and indices, not observable
     * register-size metadata. Catalog checks do not prove DUT compliance.
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
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0048,
        .rule_str = "R0048",
        .suite = "firme",
        .test_name = "feature_register_abi_presence",
        .desc = "Implemented services advertise mandatory and permitted ABIs",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .check_ns = "Check mandatory ABI bits from NS world",
        .check_secure = "Check mandatory ABI bits from Secure world",
        .check_realm = "Check mandatory ABI bits from Realm world",
        .run_ns = firme_base_feature_register_abi_presence_run_ns,
        .run_secure = firme_base_feature_register_abi_presence_run_secure,
        .run_realm = firme_base_feature_register_abi_presence_run_realm,
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
        .rule_id = FIRME_RULE_R0054,
        .rule_str = "R0054",
        .suite = "firme",
        .test_name = "service_abi_compliance",
        .desc = "Each implemented service follows its ABI compliance table",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .check_ns = "Check service ABI classification from NS world",
        .check_secure = "Check service ABI classification from Secure world",
        .check_realm = "Check service ABI classification from Realm world",
        .run_ns = firme_base_service_abi_compliance_run_ns,
        .run_secure = firme_base_service_abi_compliance_run_secure,
        .run_realm = firme_base_service_abi_compliance_run_realm,
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
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0064,
        .rule_str = "R0064",
        .suite = "firme",
        .test_name = "max_concurrent_feature_fields",
        .desc = "Non-Base services expose MAX_CONCURRENT_OP_CNT in register 1",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .check_ns = "Check register 1 from NS world",
        .check_secure = "Check register 1 from Secure world",
        .check_realm = "Check register 1 from Realm world",
        .run_ns = firme_base_max_concurrent_r0064_run_ns,
        .run_secure = firme_base_max_concurrent_r0064_run_secure,
        .run_realm = firme_base_max_concurrent_r0064_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0065,
        .rule_str = "R0065",
        .suite = "firme",
        .test_name = "max_concurrent_consistency",
        .desc = "Compare concurrent-operation limits across FIRME instances",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .check_ns = "Collect limits from NS world",
        .check_secure = "Collect limits from Secure world",
        .check_realm = "Collect limits from Realm world",
        .run_ns = firme_base_max_concurrent_r0065_run_ns,
        .run_secure = firme_base_max_concurrent_r0065_run_secure,
        .run_realm = firme_base_max_concurrent_r0065_run_realm,
        .post_run = firme_base_max_concurrent_r0065_post,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0070,
        .rule_str = "R0070",
        .suite = "firme",
        .test_name = "shared_buffer_mapping_attributes",
        .desc = "Shared-buffer mapping attributes",
        .required_worlds_mask = FIRME_WORLD_MASK_NS |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_ns = "Shared-buffer mapping attributes (ns)",
        .run_ns = firme_base_shared_buffer_mapping_attributes_run_ns,
        .check_realm = "Shared-buffer mapping attributes (realm)",
        .run_realm = firme_base_shared_buffer_mapping_attributes_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0071,
        .rule_str = "R0071",
        .suite = "firme",
        .test_name = "shared_buffer_pas_gpi",
        .desc = "Shared-buffer PAS and GPI ownership",
        .required_worlds_mask = FIRME_WORLD_MASK_NS |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_ns = "Shared-buffer PAS and GPI ownership (ns)",
        .run_ns = firme_base_shared_buffer_pas_gpi_run_ns,
        .check_realm = "Shared-buffer PAS and GPI ownership (realm)",
        .run_realm = firme_base_shared_buffer_pas_gpi_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0076,
        .rule_str = "R0076",
        .suite = "firme",
        .test_name = "shared_buffer_size_bounds",
        .desc = "Shared-buffer size bounds",
        .required_worlds_mask = FIRME_WORLD_MASK_NS |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_ns = "Shared-buffer size bounds (ns)",
        .run_ns = firme_base_shared_buffer_size_bounds_run_ns,
        .check_realm = "Shared-buffer size bounds (realm)",
        .run_realm = firme_base_shared_buffer_size_bounds_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0077,
        .rule_str = "R0077",
        .suite = "firme",
        .test_name = "shared_buffer_alignment",
        .desc = "Shared-buffer base alignment",
        .required_worlds_mask = FIRME_WORLD_MASK_NS |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_ns = "Shared-buffer base alignment (ns)",
        .run_ns = firme_base_shared_buffer_alignment_run_ns,
        .check_realm = "Shared-buffer base alignment (realm)",
        .run_realm = firme_base_shared_buffer_alignment_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0080,
        .rule_str = "R0080",
        .suite = "firme",
        .test_name = "partial_retrieval_last_chunk",
        .desc = "Partial retrieval final-chunk outputs",
        .required_worlds_mask = FIRME_WORLD_MASK_NS |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_ns = "Partial retrieval final-chunk outputs (ns)",
        .run_ns = firme_base_partial_retrieval_last_chunk_run_ns,
        .check_realm = "Partial retrieval final-chunk outputs (realm)",
        .run_realm = firme_base_partial_retrieval_last_chunk_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0081,
        .rule_str = "R0081",
        .suite = "firme",
        .test_name = "partial_retrieval_intermediate_chunk",
        .desc = "Partial retrieval intermediate outputs",
        .required_worlds_mask = FIRME_WORLD_MASK_NS |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_ns = "Partial retrieval intermediate outputs (ns)",
        .run_ns = firme_base_partial_retrieval_intermediate_chunk_run_ns,
        .check_realm = "Partial retrieval intermediate outputs (realm)",
        .run_realm = firme_base_partial_retrieval_intermediate_chunk_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_BASE,
        .rule_id = FIRME_RULE_R0082,
        .rule_str = "R0082",
        .suite = "firme",
        .test_name = "partial_retrieval_no_chunk",
        .desc = "Partial retrieval without a returned chunk",
        .required_worlds_mask = FIRME_WORLD_MASK_NS |
                                FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_ns = "Partial retrieval without a returned chunk (ns)",
        .run_ns = firme_base_partial_retrieval_no_chunk_run_ns,
        .check_realm = "Partial retrieval without a returned chunk (realm)",
        .run_realm = firme_base_partial_retrieval_no_chunk_run_realm,
    },
#endif
#if FIRME_ENABLE_IDE
    {
        .chapter = FIRME_CHAPTER_IDE,
        .rule_id = FIRME_RULE_R0211,
        .rule_str = "R0211",
        .suite = "firme",
        .test_name = "ecam_keyset_namespace",
        .desc = "Independent ECAM keyset namespaces",
        .required_worlds_mask = 0u,
        .optional_worlds_mask = FIRME_WORLD_MASK_NS |
                                FIRME_WORLD_MASK_REALM,
        .check_ns = "Independent ECAM keyset namespaces (ns)",
        .run_ns = firme_ide_ecam_keyset_namespace_run_ns,
        .check_realm = "Independent ECAM keyset namespaces (realm)",
        .run_realm = firme_ide_ecam_keyset_namespace_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_IDE,
        .rule_id = FIRME_RULE_R0212,
        .rule_str = "R0212",
        .suite = "firme",
        .test_name = "keyset_id_uniqueness",
        .desc = "Independent keyset selection within an ECAM space",
        .required_worlds_mask = 0u,
        .optional_worlds_mask = FIRME_WORLD_MASK_NS |
                                FIRME_WORLD_MASK_REALM,
        .check_ns = "Independent keyset selection within an ECAM space (ns)",
        .run_ns = firme_ide_keyset_id_uniqueness_run_ns,
        .check_realm = "Independent keyset selection within an ECAM space (realm)",
        .run_realm = firme_ide_keyset_id_uniqueness_run_realm,
        .run_host_realm = firme_ide_keyset_id_uniqueness_run_host_realm,
    },
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
    {
        .chapter = FIRME_CHAPTER_IDE,
        .rule_id = FIRME_RULE_R0207,
        .rule_str = "R0207",
        .suite = "firme",
        .test_name = "ide_cxl_key_set_unused",
        .desc = "CXL.cachemem requests keep the unused Key Set field zero",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_realm = "Invoke CXL.cachemem PROG with unused Key Set zero",
        .run_ns = 0,
        .run_secure = 0,
        .run_realm = firme_ide_cxl_key_set_unused_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_IDE,
        .rule_id = FIRME_RULE_R0209,
        .rule_str = "R0209",
        .suite = "firme",
        .test_name = "ide_cxl_stream_id_zero",
        .desc = "CXL.cachemem IDE Stream ID is zero",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_realm = "Invoke CXL.cachemem PROG with non-zero Stream ID",
        .run_ns = 0,
        .run_secure = 0,
        .run_realm = firme_ide_cxl_stream_id_zero_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_IDE,
        .rule_id = FIRME_RULE_R0210,
        .rule_str = "R0210",
        .suite = "firme",
        .test_name = "ide_cxl_substream_id",
        .desc = "CXL.cachemem IDE Substream ID is 0b1000",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_realm = "Invoke CXL.cachemem PROG with invalid Substream ID",
        .run_ns = 0,
        .run_secure = 0,
        .run_realm = firme_ide_cxl_substream_id_run_realm,
    },
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
    {
        .chapter = FIRME_CHAPTER_IDE,
        .rule_id = FIRME_RULE_R0231,
        .rule_str = "R0231",
        .suite = "firme",
        .test_name = "ide_feature_register",
        .desc = "IDE feature registers follow ALP4 Table 4.3",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = FIRME_WORLD_MASK_NS,
        .check_ns = "Check IDE feature registers from NS world",
        .check_realm = "Check IDE feature registers from Realm world",
        .run_ns = firme_ide_feature_register_run_ns,
        .run_realm = firme_ide_feature_register_run_realm,
    },
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
    {
        .chapter = FIRME_CHAPTER_MEC,
        .rule_id = FIRME_RULE_R0236,
        .rule_str = "R0236",
        .suite = "firme",
        .test_name = "mec_context_refresh",
        .desc = "MEC_REFRESH invalidates the previous encryption context",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .check_realm = "Observe stale data and the new MEC context",
        .run_realm = firme_mec_context_refresh_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_MEC,
        .rule_id = FIRME_RULE_R0238,
        .rule_str = "R0238",
        .suite = "firme",
        .test_name = "mec_ciphertext_decorrelation",
        .desc = "MEC_REFRESH decorrelates old and new protected representations",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .check_realm = "Compare old-context and new-context memory behavior",
        .run_realm = firme_mec_ciphertext_decorrelation_run_realm,
    },
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
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0195,
        .rule_str = "R0195",
        .suite = "firme",
        .test_name = "granule_management_abi_compliance",
        .desc = "Granule Management ABIs follow the ALP4 compliance table",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .check_ns = "Check GM ABI classification from NS world",
        .check_secure = "Check GM ABI classification from Secure world",
        .check_realm = "Check GM ABI classification from Realm world",
        .run_ns = firme_gpt_granule_management_abi_compliance_run_ns,
        .run_secure = firme_gpt_granule_management_abi_compliance_run_secure,
        .run_realm = firme_gpt_granule_management_abi_compliance_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0102,
        .rule_str = "R0102",
        .suite = "firme",
        .test_name = "gpt_protected_range_base_alignment",
        .desc = "GPT protected range base aligns to advertised geometry",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .run_ns = firme_gpt_protected_range_base_alignment_run_ns,
        .run_secure = firme_gpt_protected_range_base_alignment_run_secure,
        .run_realm = firme_gpt_protected_range_base_alignment_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0103,
        .rule_str = "R0103",
        .suite = "firme",
        .test_name = "gpt_protected_range_size",
        .desc = "GPT protected range size matches advertised geometry",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .run_ns = firme_gpt_protected_range_size_run_ns,
        .run_secure = firme_gpt_protected_range_size_run_secure,
        .run_realm = firme_gpt_protected_range_size_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0134,
        .rule_str = "R0134",
        .suite = "firme",
        .test_name = "gpt_management_owner_stability",
        .desc = "GPI transitions preserve protected-range management ownership",
        .required_worlds_mask = FIRME_WORLD_MASK_SECURE | FIRME_WORLD_MASK_REALM,
        .run_secure = firme_gpt_management_owner_stability_run_secure,
        .run_realm = firme_gpt_management_owner_stability_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0138,
        .rule_str = "R0138",
        .suite = "firme",
        .test_name = "gpt_par_state_encoding",
        .desc = "GPT_INFO reports an architected protected-range state",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .run_ns = firme_gpt_par_state_encoding_run_ns,
        .run_secure = firme_gpt_par_state_encoding_run_secure,
        .run_realm = firme_gpt_par_state_encoding_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0139,
        .rule_str = "R0139",
        .suite = "firme",
        .test_name = "gpt_info_status",
        .desc = "GPT_INFO returns the specified status for each input class",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .run_ns = firme_gpt_info_status_run_ns,
        .run_secure = firme_gpt_info_status_run_secure,
        .run_realm = firme_gpt_info_status_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0140,
        .rule_str = "R0140",
        .suite = "firme",
        .test_name = "gpt_info_success_outputs",
        .desc = "GPT_INFO success outputs describe the queried protected range",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .run_ns = firme_gpt_info_success_outputs_run_ns,
        .run_secure = firme_gpt_info_success_outputs_run_secure,
        .run_realm = firme_gpt_info_success_outputs_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0141,
        .rule_str = "R0141",
        .suite = "firme",
        .test_name = "gpt_info_output_validity",
        .desc = "GPT_INFO outputs are meaningful only on success",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .run_ns = firme_gpt_info_output_validity_run_ns,
        .run_secure = firme_gpt_info_output_validity_run_secure,
        .run_realm = firme_gpt_info_output_validity_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0114,
        .rule_str = "R0114",
        .suite = "firme",
        .test_name = "gpt_op_continue_start_granule",
        .desc = "OP_CONTINUE resumes at the first untransitioned granule",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .run_ns = firme_gpt_op_continue_start_granule_run_ns,
        .run_secure = firme_gpt_op_continue_start_granule_run_secure,
        .run_realm = firme_gpt_op_continue_start_granule_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0115,
        .rule_str = "R0115",
        .suite = "firme",
        .test_name = "gpt_op_continue_granule_count",
        .desc = "OP_CONTINUE reports progress for the resumed granule prefix",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .run_ns = firme_gpt_op_continue_granule_count_run_ns,
        .run_secure = firme_gpt_op_continue_granule_count_run_secure,
        .run_realm = firme_gpt_op_continue_granule_count_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0118,
        .rule_str = "R0118",
        .suite = "firme",
        .test_name = "gpt_op_continue_progress",
        .desc = "OP_CONTINUE status, count and cookie describe actual progress",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .run_ns = firme_gpt_op_continue_progress_run_ns,
        .run_secure = firme_gpt_op_continue_progress_run_secure,
        .run_realm = firme_gpt_op_continue_progress_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0122,
        .rule_str = "R0122",
        .suite = "firme",
        .test_name = "gpt_not_supported_status",
        .desc = "Disallowed Granule Management calls return NOT_SUPPORTED",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .run_ns = firme_gpt_not_supported_status_run_ns,
        .run_secure = firme_gpt_not_supported_status_run_secure,
        .run_realm = firme_gpt_not_supported_status_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0124,
        .rule_str = "R0124",
        .suite = "firme",
        .test_name = "gpt_busy_retry",
        .desc = "BUSY reports no progress and permits a bounded retry",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .run_ns = firme_gpt_busy_retry_run_ns,
        .run_secure = firme_gpt_busy_retry_run_secure,
        .run_realm = firme_gpt_busy_retry_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0126,
        .rule_str = "R0126",
        .suite = "firme",
        .test_name = "gpt_l1_allocation_not_found",
        .desc = "Unavailable L1 allocation reports NOT_FOUND and valid progress",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .run_ns = firme_gpt_l1_allocation_not_found_run_ns,
        .run_secure = firme_gpt_l1_allocation_not_found_run_secure,
        .run_realm = firme_gpt_l1_allocation_not_found_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0129,
        .rule_str = "R0129",
        .suite = "firme",
        .test_name = "gpt_operation_conflict",
        .desc = "Overlapping retained operations report OP_CONFLICT",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .run_ns = firme_gpt_operation_conflict_run_ns,
        .run_secure = firme_gpt_operation_conflict_run_secure,
        .run_realm = firme_gpt_operation_conflict_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0130,
        .rule_str = "R0130",
        .suite = "firme",
        .test_name = "gpt_stateful_incomplete",
        .desc = "INCOMPLETE retains operation state until its terminal result",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .run_ns = firme_gpt_stateful_incomplete_run_ns,
        .run_secure = firme_gpt_stateful_incomplete_run_secure,
        .run_realm = firme_gpt_stateful_incomplete_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0144,
        .rule_str = "R0144",
        .suite = "firme",
        .test_name = "gpt_info_concurrent_state",
        .desc = "Concurrent GPT_INFO returns a state observed during the call",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .run_ns = firme_gpt_info_concurrent_state_run_ns,
        .run_secure = firme_gpt_info_concurrent_state_run_secure,
        .run_realm = firme_gpt_info_concurrent_state_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0156,
        .rule_str = "R0156",
        .suite = "firme",
        .test_name = "l1_bridge_object_properties",
        .desc = "Temporary L1 bridge preserves object ownership and geometry",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .run_realm = firme_gpt_l1_bridge_object_properties_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0161,
        .rule_str = "R0161",
        .suite = "firme",
        .test_name = "l1_create_transition_policy",
        .desc = "L1 CREATE follows Realm-only NS-to-Root transition policy",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE,
        .run_ns = firme_gpt_l1_create_transition_policy_run_ns,
        .run_secure = firme_gpt_l1_create_transition_policy_run_secure,
        .run_realm = firme_gpt_l1_create_transition_policy_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0167,
        .rule_str = "R0167",
        .suite = "firme",
        .test_name = "l1_create_status",
        .desc = "L1 CREATE returns the status for each architected condition",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE,
        .run_ns = firme_gpt_l1_create_status_run_ns,
        .run_secure = firme_gpt_l1_create_status_run_secure,
        .run_realm = firme_gpt_l1_create_status_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0168,
        .rule_str = "R0168",
        .suite = "firme",
        .test_name = "l1_create_range_gpi_stability",
        .desc = "L1 CREATE preserves non-object range GPIs",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .run_realm = firme_gpt_l1_create_range_gpi_stability_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0170,
        .rule_str = "R0170",
        .suite = "firme",
        .test_name = "l1_create_success_object_gpi",
        .desc = "Successful L1 CREATE moves object granules to Root",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .run_realm = firme_gpt_l1_create_success_object_gpi_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0171,
        .rule_str = "R0171",
        .suite = "firme",
        .test_name = "l1_create_incomplete_object_gpi",
        .desc = "Interrupted L1 CREATE keeps each object granule in a valid GPI",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .run_realm = firme_gpt_l1_create_incomplete_object_gpi_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0173,
        .rule_str = "R0173",
        .suite = "firme",
        .test_name = "l1_create_denied_no_effect",
        .desc = "Denied L1 CREATE leaves the remaining object range unchanged",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .run_realm = firme_gpt_l1_create_denied_no_effect_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0179,
        .rule_str = "R0179",
        .suite = "firme",
        .test_name = "l1_destroy_transition_policy",
        .desc = "L1 DESTROY follows Realm-only Root-to-NS transition policy",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE,
        .run_ns = firme_gpt_l1_destroy_transition_policy_run_ns,
        .run_secure = firme_gpt_l1_destroy_transition_policy_run_secure,
        .run_realm = firme_gpt_l1_destroy_transition_policy_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0180,
        .rule_str = "R0180",
        .suite = "firme",
        .test_name = "l1_destroy_scrub",
        .desc = "L1 DESTROY scrubs object storage before releasing it",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .run_realm = firme_gpt_l1_destroy_scrub_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0184,
        .rule_str = "R0184",
        .suite = "firme",
        .test_name = "l1_destroy_status",
        .desc = "L1 DESTROY returns the status for each architected condition",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE,
        .run_ns = firme_gpt_l1_destroy_status_run_ns,
        .run_secure = firme_gpt_l1_destroy_status_run_secure,
        .run_realm = firme_gpt_l1_destroy_status_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0185,
        .rule_str = "R0185",
        .suite = "firme",
        .test_name = "l1_destroy_success_output",
        .desc = "Successful L1 DESTROY returns the correct object base",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .run_realm = firme_gpt_l1_destroy_success_output_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0186,
        .rule_str = "R0186",
        .suite = "firme",
        .test_name = "l1_destroy_range_gpi_stability",
        .desc = "L1 DESTROY preserves non-object range GPIs",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .run_realm = firme_gpt_l1_destroy_range_gpi_stability_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0188,
        .rule_str = "R0188",
        .suite = "firme",
        .test_name = "l1_destroy_success_object_gpi",
        .desc = "Successful L1 DESTROY returns object granules to NS",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .run_realm = firme_gpt_l1_destroy_success_object_gpi_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0189,
        .rule_str = "R0189",
        .suite = "firme",
        .test_name = "l1_destroy_incomplete_object_gpi",
        .desc = "Interrupted L1 DESTROY keeps each object granule in a valid GPI",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .run_realm = firme_gpt_l1_destroy_incomplete_object_gpi_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0191,
        .rule_str = "R0191",
        .suite = "firme",
        .test_name = "l1_single_in_progress_operation",
        .desc = "Conflicting L1 CREATE and DESTROY requests cannot overlap",
        .required_worlds_mask = FIRME_WORLD_MASK_REALM,
        .run_realm = firme_gpt_l1_single_in_progress_operation_run_realm,
    },
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
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0092,
        .rule_str = "R0092",
        .suite = "firme",
        .test_name = "gpt_same_gpi_transition",
        .desc = "A same-GPI transition succeeds without changing ownership",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .run_ns = firme_gpt_same_gpi_transition_run_ns,
        .run_secure = firme_gpt_same_gpi_transition_run_secure,
        .run_realm = firme_gpt_same_gpi_transition_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0094,
        .rule_str = "R0094",
        .suite = "firme",
        .test_name = "gpt_transition_feat_rme",
        .desc = "GPI transitions follow FEAT_RME caller policy",
        .required_worlds_mask = FIRME_WORLD_MASK_NS | FIRME_WORLD_MASK_SECURE |
                                FIRME_WORLD_MASK_REALM,
        .run_ns = firme_gpt_transition_feat_rme_run_ns,
        .run_secure = firme_gpt_transition_feat_rme_run_secure,
        .run_realm = firme_gpt_transition_feat_rme_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0095,
        .rule_str = "R0095",
        .suite = "firme",
        .test_name = "gpt_transition_feat_gpc2",
        .desc = "NSO transition policy follows FEAT_GPC2 availability",
        .required_worlds_mask = FIRME_WORLD_MASK_NS,
        .run_ns = firme_gpt_transition_feat_gpc2_run_ns,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0098,
        .rule_str = "R0098",
        .suite = "firme",
        .test_name = "gpt_transition_feat_gdi",
        .desc = "NSP transition policy follows FEAT_GDI availability",
        .required_worlds_mask = FIRME_WORLD_MASK_NS,
        .run_ns = firme_gpt_transition_feat_gdi_run_ns,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0120,
        .rule_str = "R0120",
        .suite = "firme",
        .test_name = "gpt_success_count_nonzero",
        .desc = "Successful GPI operations report nonzero progress",
        .required_worlds_mask = FIRME_WORLD_MASK_SECURE | FIRME_WORLD_MASK_REALM,
        .run_secure = firme_gpt_success_count_nonzero_run_secure,
        .run_realm = firme_gpt_success_count_nonzero_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0123,
        .rule_str = "R0123",
        .suite = "firme",
        .test_name = "gpt_invalid_parameters_status",
        .desc = "Malformed GM_GPI_SET inputs return INVALID_PARAMETERS",
        .required_worlds_mask = FIRME_WORLD_MASK_NS,
        .run_ns = firme_gpt_invalid_parameters_status_run_ns,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0111,
        .rule_str = "R0111",
        .suite = "firme",
        .test_name = "gpt_start_granule",
        .desc = "GM_GPI_SET start granule is the granule at Base address",
        .required_worlds_mask = FIRME_WORLD_MASK_SECURE | FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = FIRME_WORLD_MASK_NS,
        .check_secure = "Secure GM_GPI_SET starts at Base granule",
        .check_realm = "Realm GM_GPI_SET starts at Base granule",
        .check_ns = "NS GM_GPI_SET starts at Base granule (GDI)",
        .run_ns = firme_gpt_start_granule_run_ns,
        .run_secure = firme_gpt_start_granule_run_secure,
        .run_realm = firme_gpt_start_granule_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0113,
        .rule_str = "R0113",
        .suite = "firme",
        .test_name = "gpt_success_granule_count",
        .desc = "GM_GPI_SET reports the transitioned granule prefix",
        .required_worlds_mask = FIRME_WORLD_MASK_SECURE | FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "Secure GM_GPI_SET successful prefix and partial progress",
        .check_realm = "Realm GM_GPI_SET successful prefix and partial progress",
        .run_ns = 0,
        .run_secure = firme_gpt_success_granule_count_run_secure,
        .run_realm = firme_gpt_success_granule_count_run_realm,
    },
    {
        .chapter = FIRME_CHAPTER_GPT,
        .rule_id = FIRME_RULE_R0117,
        .rule_str = "R0117",
        .suite = "firme",
        .test_name = "gpt_gpi_set_progress",
        .desc = "GM_GPI_SET status and count describe operation progress",
        .required_worlds_mask = FIRME_WORLD_MASK_SECURE | FIRME_WORLD_MASK_REALM,
        .optional_worlds_mask = 0u,
        .check_secure = "Secure GM_GPI_SET success and denied progress",
        .check_realm = "Realm GM_GPI_SET success and denied progress",
        .run_ns = 0,
        .run_secure = firme_gpt_gpi_set_progress_run_secure,
        .run_realm = firme_gpt_gpi_set_progress_run_realm,
    },
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
