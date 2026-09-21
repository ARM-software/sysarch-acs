..
.. Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
..
.. SPDX-License-Identifier: BSD-3-Clause
..

********************************************************************************
FIRME Architecture Compliance Suite - Scenario Document (FIRME-ACS)
********************************************************************************

.. contents:: Contents
  :depth: 2

Overview
========

FIRME-ACS validates the TF-A BL31 (EL3) implementation of the FIRME service by exercising FIRME ABIs from the
caller security states BL31 expects in real deployments:

- Normal world: NS-EL2 dispatcher payload (BL33)
- Secure world: S-EL2 shim payload (BL32) with optional S-EL1 caller bodies
- Realm world: R-EL2 shim payload (Realm payload image), reached via RMMD forwarding

Testcase checklist
==================

Deferred coverage
-----------------

The following rules have documented algorithms but no executable body or
registry entry because the ACS cannot yet drive the required observable ABI
flow:

- R0063, R0064, R0069, and R0070 need a service-specific ABI that
  consumes a shared buffer. Until such a service driver is added, the ACS
  cannot validate mapping attributes, PAS/GPI ownership, size bounds, or
  base alignment from an actual service invocation.
- R0073, R0074, and R0075 need a service-specific ABI that supports
  partial retrieval. Until such a service driver is added, the ACS cannot
  observe written-size and remaining-size semantics for last, intermediate,
  or no-data chunks.

Expected runtime SKIP coverage
------------------------------

The active IDE tests report an intentional ``SKIP`` when the required
CXL.cachemem path is not exposed by the DUT:

- R0175, R0177, and R0178 require CXL.cachemem IDE handling. A valid baseline
  request returning ``NOT_SUPPORTED`` reports ``SKIP``. Any other unexpected
  status, including ``INVALID_PARAMETERS`` or unfinished completion, fails.

Implemented but not scheduled by default
----------------------------------------

The following strict ALP3 rule bodies are implemented in source but not
registered in the default regression until the reference DUT implements
the corresponding ALP3 ``FIRME_GM_GPI_SET`` semantics:

- R0085: same-GPI transitions are nops that complete with ``SUCCESS``.
- R0106: multi-granule ``SUCCESS`` reports the number of transitioned granules.
- R0116: malformed Attributes fields complete with ``INVALID_PARAMETERS``.

The rule algorithms must remain spec-driven. If a DUT claims the matching
ALP3 behavior, these rules should be scheduled and any failure should be
treated as a DUT issue.

.. list-table::
  :header-rows: 1
  :widths: 22 20 58

  * - **Test Name**
    - **Validated rule(s)**
    - **Algorithm**

  * -

      `smccc_version <../test/base/smccc_version.c>`_

    -

      R0007: FIRME ABIs are compliant with SMCCC version 1.2 or later.

    -

      1. Invoke the SMCCC ``VERSION`` call from each caller world.
      2. Decode the returned SMCCC major and minor version fields.
      3. Expect a non-error response and verify the version is at least 1.2.
      4. Discover every service and ABI advertised at the current FIRME instance.
      5. Invoke each advertised ABI with side-effect-safe, ABI-specific inputs.
      6. Verify unused ``x4-x17``, preserved ``x18-x30``, SIMD/FP state, ``FPCR`` and ``FPSR``.

  * -

      `smccc_sve <../test/base/smccc_sve.c>`_

    -

      R0008: If FEAT_SVE is implemented, FIRME ABIs are compliant with SMCCC version 1.3 or later.

    -

      1. Read ``ID_AA64PFR0_EL1`` and check the SVE field.
      2. If FEAT_SVE is absent, report SKIP because the conditional rule is not applicable.
      3. If FEAT_SVE is present, invoke SMCCC ``VERSION``.
      4. Expect the reported SMCCC version to be at least 1.3.
      5. Seed live ``Z``, ``P`` and ``FFR`` state and invoke every advertised ABI.
      6. Repeat each ABI with the SVE hint and verify discardable state is preserved or zeroed.
      7. Verify the lower 128 bits of every ``Z`` register and SIMD/FP controls in both cases.

  * -

      `smccc_sme <../test/base/smccc_sme.c>`_

    -

      R0009: If FEAT_SME is implemented, FIRME ABIs are compliant with SMCCC version 1.4 or later.

    -

      1. Read ``ID_AA64PFR1_EL1`` and check the SME field.
      2. If FEAT_SME is absent, report SKIP because the conditional rule is not applicable.
      3. If FEAT_SME is present, invoke SMCCC ``VERSION``.
      4. Expect the reported SMCCC version to be at least 1.4.
      5. Seed live ``ZA`` and vector state in non-streaming and streaming modes.
      6. Invoke every advertised ABI with and without the SVE hint.
      7. Verify live ``ZA``, applicable vector state, streaming state and SIMD/FP controls.

  * -

      `smc64_calling_convention <../test/base/smc64_calling_convention.c>`_

    -

      R0010: FIRME ABIs use the SMC64 calling convention.

    -

      1. Verify every ALP3 FIRME ABI function ID carries the SMC64 convention bit.
      2. Discover each ABI advertised at the current FIRME instance.
      3. Invoke every advertised ABI using its SMC64 function ID and require an ABI-defined response.

  * -

      `status_codes <../test/base/status_codes.c>`_

    -

      R0011: FIRME ABIs use the status codes defined by the FIRME specification.

    -

      1. Invoke a valid Base feature-register request and expect ``SUCCESS``.
      2. Exercise allocated and unallocated service discovery requests.
      3. Discover every ABI advertised at the current FIRME instance.
      4. Invoke each advertised ABI with side-effect-safe inputs.
      5. Require each observed return value to be permitted by that ABI definition.

  * -

      `reserved_input_registers <../test/base/reserved_input_registers.c>`_

    -

      R0012: An unused input parameter register in a FIRME ABI is Reserved and SBZ.

    -

      1. Enable capture-only mode before invoking each production FIRME wrapper.
      2. Capture the final x0-x17 input frame immediately before SMC dispatch.
      3. Bypass the SMC while capture is enabled so GM, MEC and IDE audit requests
         cannot change DUT state.
      4. Exercise the Base, GM, MEC and IDE wrappers present in this revision.
      5. Verify every wholly unused input register is zero; the generic helper
         clears x8-x17 immediately before the capture point.
      6. Report PASS only when every captured wrapper frame satisfies SBZ.

  * -

      `reserved_input_bits <../test/base/reserved_input_bits.c>`_

    -

      R0013: An unused bit in an input parameter register in a FIRME ABI is Reserved and SBZ.

    -

      1. Enable capture-only mode and invoke every production FIRME wrapper
         present in this revision with all-one values in parameter fields.
      2. Capture the final x0-x17 input frame immediately before SMC dispatch.
      3. Verify only allocated Base service-ID, feature-index, GM attribute, MEC
         parameter, and IDE flag/keyset-ID bits remain; all others are zero.
      4. Bypass the SMC while capture is enabled so malformed audit inputs cannot
         change DUT state.
      5. Use the raw SMCCC path for intentional DUT parameter-validation tests,
         so production-wrapper sanitization cannot weaken negative coverage.
      6. Report PASS only when every captured wrapper frame satisfies SBZ.

  * -

      `reserved_output_registers <../test/base/reserved_output_registers.c>`_

    -

      R0014: An unused output parameter register is Reserved and ignored by the caller.

    -

      1. Invoke ``FIRME_SERVICE_VERSION`` and consume only the architected version/status output.
      2. Invoke ``FIRME_SERVICE_FEATURES`` and consume only architected status and feature-register outputs.
      3. Configure the ACS caller-side response hook to replace unused output
         registers with zero, all-one and mixed patterns after the real SMC.
      4. Exercise the production service-discovery consumers with each pattern.
      5. Confirm service availability and decoded fields are unchanged.
         Registers not exposed by the VAL response type are ignored by
         construction.
      6. Scope poisoning to the current PE and requested FID so unrelated calls
         retain the genuine DUT response.
      7. This validates Base caller behavior without modifying the DUT. The
         remaining service/status output shapes and callee non-disclosure
         property require caller and implementation security review.
      8. Keep R0014 registry-gated until both parts have complete evidence.

  * -

      `reserved_output_bits <../test/base/reserved_output_bits.c>`_

    -

      R0015: An unused output bit is Reserved and ignored by the caller.

    -

      1. Invoke Base feature registers 0 and 1 using ``FIRME_SERVICE_FEATURES``.
      2. Configure the ACS caller-side response hook to replace upper X0 bits
         and unused feature-register fields after the real SMC.
      3. Repeat with zero, all-one and mixed patterns through the production
         service-discovery and signed-W0 decoding paths.
      4. Confirm service state, version and every allocated feature field
         remain identical for every poison pattern.
      5. Scope poisoning to the current PE and requested FID.
      6. This validates Base caller behavior without modifying the DUT. The
         remaining service/status output fields and callee non-disclosure
         property require caller and implementation security review.
      7. Keep R0015 registry-gated until both parts have complete evidence.

  * -

      Static service ID width review

    -

      R0023: FIRME service IDs are 8-bit unsigned integers.

    -

      1. Review the service-ID field definition against the specification.
      2. Statically check that ACS allocations fit the eight-bit field.
      3. Do not add a runtime body or registry entry: upper input bits are SBZ,
         so legal calls cannot distinguish an eight-bit decoder from a wider one.
      4. Unallocated-ID behavior is handled separately by R0024.

  * -

      `service_id_allocation <../test/base/service_id_allocation.c>`_

    -

      R0024: FIRME services use the service IDs allocated by Table 2.2; unallocated values are unused.

    -

      1. Invoke ``FIRME_SERVICE_VERSION`` for the Base service ID ``0`` and expect a version.
      2. Query VERSION and feature register 0 at every allocated service ID.
      3. Require a version and successful feature discovery for implemented
         services; an absent service must return exactly NOT_SUPPORTED for both.
      4. Query both ABIs for every unallocated eight-bit service ID and require
         NOT_SUPPORTED.
      5. This validates discovery allocation but cannot identify services that
         expose identical discovery data. Keep R0024 out of the runtime registry
         until service-specific ABI checks identify every implemented ID 1-5.

  * -

      `service_instance_presence <../test/base/service_instance_presence.c>`_

    -

      R0025: FIRME service presence follows the per-instance applicability table.

    -

      1. Invoke Base service discovery from each caller world and expect the Base service to be present.
      2. For services marked ``No`` for the current FIRME instance in Table 2.3, invoke ``FIRME_SERVICE_VERSION``.
      3. Expect ``NOT_SUPPORTED`` for every forbidden service at that instance.
      4. Invoke ``FIRME_SERVICE_FEATURES`` register 0 for each forbidden service and expect ``NOT_SUPPORTED``.
      5. Services marked ``Yes`` are permitted but not required by this test unless covered by a service-specific rule.

  * -

      `base_service_presence <../test/base/base_service_presence.c>`_

    -

      R0026: The FIRME Base service is implemented at all FIRME instances.

    -

      1. From each caller world, invoke ``FIRME_SERVICE_VERSION`` for the Base service.
      2. Expect the Base service version to be reported.
      3. Invoke ``FIRME_SERVICE_FEATURES`` for Base feature registers 0 and 1.
      4. Expect SUCCESS for both Base feature registers.

  * -

      Static specification version policy review

    -

      R0031: FIRME specification version increments track service version changes and new services.

    -

      1. Compare specification revisions and their service-version/allocation tables.
      2. Do not require DUT service versions to equal the specification version.
      3. Do not add a runtime body or registry entry: R0031 is a specification
         release-policy requirement, not an observable DUT constraint.

  * -

      `service_version_compatibility <../test/base/service_version_compatibility.c>`_

    -

      R0033: FIRME service major/minor version values follow compatibility rules.

    -

      1. Discover every allocated service and reject unexpected version errors.
      2. For a newer Base 1.x minor revision, execute the Base 1.0 ABI algorithms
         for R0038, R0051, R0053 and R0055 in the current caller world.
      3. Preserve failures from those baseline tests; missing or skipped baseline
         bodies mean incomplete coverage, not PASS.
      4. For a non-Base service with a newer minor revision, require a complete
         service-specific 1.0 baseline driver; otherwise report incomplete SKIP.
      5. Treat a baseline-only 1.0 implementation as PASS. Report SKIP for a
         newer revision whose baseline cannot be exercised; never infer
         compatibility from its version value or Base presence bits alone.

  * -

      `service_versions <../test/base/service_versions.c>`_

    -

      R0034: Implemented FIRME services report the versions listed for this FIRME revision.

    -

      1. Iterate over all ALP3 service IDs at each FIRME instance.
      2. Invoke ``FIRME_SERVICE_VERSION`` for each service ID.
      3. Ignore ``NOT_SUPPORTED`` services because they are not implemented at that instance.
      4. For implemented services, verify the reported service version is 1.0.

  * -

      `service_base_dependency <../test/base/service_base_dependency.c>`_

    -

      R0035: Implemented FIRME services depend on FIRME Base service version 1.0.

    -

      1. Invoke ``FIRME_SERVICE_VERSION`` for the Base service at the current FIRME instance.
      2. Verify the Base major version is 1; newer minor revisions are compatible.
      3. Iterate over non-Base ALP3 service IDs.
      4. If a non-Base service is implemented, use the already checked Base version as the dependency anchor.

  * -

      `service_version <../test/base/service_version.c>`_

    -

      R0038: ``FIRME_SERVICE_VERSION`` returns the requested service version or ``NOT_SUPPORTED``.

    -

      1. Invoke ``FIRME_SERVICE_VERSION`` for the Base service.
      2. Expect SUCCESS and verify the returned major version matches the supported FIRME major version.
      3. Iterate over all unallocated 8-bit service IDs.
      4. Expect ``NOT_SUPPORTED`` for every unallocated service ID.

  * -

      Static feature register size review

    -

      R0041: The size of a feature register is a non-zero multiple of 64 bits.

    -

      1. Discover every allocated service and read all registers defined in its
         ALP3 feature table, using the complete 64-bit result.
      2. Fail unexpected discovery responses or a missing defined register.
      3. Review the non-zero, 64-bit-multiple sizes against the interface tables.
         ``FIRME_SERVICE_FEATURES`` does not return a size field; neither the
         register count nor a local ``sizeof`` check proves DUT width.
      4. Do not add a runtime body or registry entry; classify R0041 as a
         static interface-table requirement until size metadata is observable.

  * -

      `feature_register_result_encoding <../test/base/feature_register_result_encoding.c>`_

    -

      R0044: SMC64 result register N encodes the corresponding 64-bit feature register range.

    -

      1. Invoke ``FIRME_SERVICE_FEATURES`` for Base feature register 0.
      2. Verify the mandatory Base ABI bits are visible in ``x1``.
      3. Invoke ``FIRME_SERVICE_FEATURES`` for Base feature register 1.
      4. Verify the Base feature fields are decoded from ``x1``.

  * -

      `feature_register_index <../test/base/feature_register_index.c>`_

    -

      R0047: A feature register index is an 8-bit unsigned integer.

    -

      1. Invoke ``FIRME_SERVICE_FEATURES`` for allocated Base indices 0 and 1.
      2. Expect ``SUCCESS`` for the allocated indices.
      3. Invoke ``FIRME_SERVICE_FEATURES`` for Base index ``0xff``.
      4. Expect ``NOT_SUPPORTED`` because the maximum 8-bit index is unallocated.

  * -

      `feature_register_abi_presence <../test/base/feature_register_abi_presence.c>`_

    -

      R0048: Feature register 0 reports which ABIs of an implemented service are implemented.

    -

      1. Invoke Base feature register 0 and verify the mandatory Base ABI bits are set.
      2. Iterate over implemented ALP3 service IDs using ``FIRME_SERVICE_VERSION``.
      3. For each implemented service, read feature register 0.
      4. Verify only allocated ABI-bit positions are used for that service.

  * -

      `service_feature_registers <../test/base/service_feature_registers.c>`_

    -

      R0051: Base service feature registers report architected fields and encodings.

    -

      1. Invoke ``FIRME_SERVICE_FEATURES`` for Base feature register 0.
      2. Expect SUCCESS and verify the mandatory Base ABI bits are set.
      3. Invoke ``FIRME_SERVICE_FEATURES`` for Base feature register 1.
      4. Expect SUCCESS and verify the shared-buffer size encoding is allocated.
      5. Verify the service-list bits contain only services permitted by Table 2.3 for the caller world.
      6. Do not require every optional Table 2.3 service to be present.

  * -

      `service_features <../test/base/service_features.c>`_

    -

      R0053: ``FIRME_SERVICE_FEATURES`` returns the requested feature register or ``NOT_SUPPORTED``.

    -

      1. Invoke ``FIRME_SERVICE_FEATURES`` for valid Base feature register indices 0 and 1.
      2. Expect SUCCESS for each implemented Base feature register.
      3. Iterate over all unallocated Base feature register indices.
      4. Expect ``NOT_SUPPORTED`` for every unallocated Base feature register index.
      5. Iterate over all unallocated 8-bit service IDs and expect ``NOT_SUPPORTED``.

  * -

      `service_abi_compliance <../test/base/service_abi_compliance.c>`_

    -

      R0054: Implemented service ABIs follow their mandatory, optional, or not-supported classification.

    -

      1. Discover each service at each caller instance; allow clean service absence.
      2. For each implemented service, require successful feature register 0 discovery.
      3. Check mandatory, optional and prohibited bits against its compliance table.
      4. Require the GM L1 CREATE/DESTROY pair to be both present or both absent.
      5. Require the attestation extension-claims STAGE/FINALISE/CLEAR group to
         be all present or all absent; RAT_SIGN is independent of that group.
      6. Keep the rule registry-gated while the reference DUT lacks ALP3 mandatory
         ABIs. The strict body remains available and must be re-enabled for release.

  * -

      `base_service_mandatory <../test/base/base_service_mandatory.c>`_

    -

      R0055: Base service discovery ABIs are mandatory at every implemented FIRME instance.

    -

      1. From each caller world, invoke ``FIRME_SERVICE_VERSION`` for the Base service.
      2. Expect SUCCESS.
      3. From each caller world, invoke ``FIRME_SERVICE_FEATURES`` for Base feature register 0.
      4. Expect SUCCESS.

  * -

      Deferred shared-buffer mapping algorithm

    -

      R0063: A shared buffer is mapped as Normal, Write-Back Cacheable, Outer-shareable memory.

    -

      1. Add an executable body and registry entry when a service-specific ABI
         consumes a shared buffer.
      2. Validate the EL3 mapping as Normal, Write-Back Cacheable and Outer-shareable.
      3. Do not infer mapping behavior from Base feature-register metadata.

  * -

      Deferred shared-buffer PAS/GPI algorithm

    -

      R0064: The shared-buffer PAS and GPT GPI correspond to the FIRME instance state.

    -

      1. Add an executable body and registry entry when a service-specific ABI
         consumes a shared buffer.
      2. Validate the buffer PAS and GPT GPI for the invoking FIRME instance.
      3. Do not infer ownership from the caller world without an actual service invocation.

  * -

      Deferred shared-buffer size algorithm

    -

      R0069: The shared-buffer size is within the advertised minimum and maximum bounds.

    -

      1. Add an executable body and registry entry when a service-specific ABI
         consumes a shared buffer.
      2. Invoke that ABI with buffers below, within and above the advertised bounds.
      3. Validate acceptance and rejection from observable DUT responses; decoding the
         related fields alone does not validate the size rule.

  * -

      Deferred shared-buffer alignment algorithm

    -

      R0070: The shared-buffer physical base address is aligned to the minimum shared-buffer size.

    -

      1. Add an executable body and registry entry when a service-specific ABI
         consumes a shared buffer.
      2. Invoke that ABI with aligned and deliberately misaligned buffer addresses.
      3. Validate alignment handling from observable DUT responses.

  * -

      Deferred last-chunk retrieval algorithm

    -

      R0073: Last-chunk partial retrieval reports non-zero written size and zero remaining size.

    -

      1. Add an executable body and registry entry only when an attestation retrieval ABI driver is available.
      2. Exercise a final chunk that writes data and completes retrieval.
      3. Validate non-zero written size and zero remaining size from the real ABI response.

  * -

      Deferred intermediate-chunk retrieval algorithm

    -

      R0074: First or intermediate partial-retrieval chunks report written data and remaining data.

    -

      1. Add an executable body and registry entry only when an attestation retrieval ABI driver is available.
      2. Exercise a first or intermediate chunk with data still remaining.
      3. Validate non-zero written and remaining sizes from the real ABI response.

  * -

      Deferred no-chunk retrieval algorithm

    -

      R0075: No-data partial retrieval reports zero written size and zero remaining size.

    -

      1. Add an executable body and registry entry only when an attestation retrieval ABI driver is available.
      2. Exercise completion where no data chunk is returned.
      3. Validate zero written size and zero remaining size from the real ABI response.

  * -

      `ide_keyset_id_bitmap_encoding <../test/ide/keyset_id_bitmap_encoding.c>`_

    -

      R0173: IDE keyset ID fields follow the DEN0149 bitmap encoding.

    -

      1. Encode a Keyset ID using representative Segment, Root Port, Stream, Substream,
         Direction, and Key Set fields.
      2. Decode each field and verify the field occupies the DEN0149 bit position.
      3. Invoke ``FIRME_IDE_KEYSET_PROG`` with a PAL-provided platform keyset.
      4. Expect the DUT not to reject the valid platform keyset as ``INVALID_PARAMETERS``.

  * -

      `ide_cxl_key_set_unused <../test/ide/cxl_key_set_unused.c>`_

    -

      R0175: CXL.cachemem IDE treats the Key Set field as unused.

    -

      1. Keep the unused Key Set field zero in the production request.
      2. Invoke a valid CXL.cachemem ``FIRME_IDE_KEYSET_PROG`` with Key Set 0.
      3. Report ``SKIP`` only for ``NOT_SUPPORTED``; otherwise require completion.
      4. GO and STOP the same keyset and require successful completion of each.

  * -

      `ide_cxl_stream_id_zero <../test/ide/cxl_stream_id_zero.c>`_

    -

      R0177: CXL.cachemem IDE Stream ID is zero.

    -

      1. Invoke a baseline CXL.cachemem ``FIRME_IDE_KEYSET_PROG`` with Stream ID 0.
      2. Report ``SKIP`` only for ``NOT_SUPPORTED``; otherwise require ``SUCCESS``.
      3. Invoke CXL.cachemem ``FIRME_IDE_KEYSET_PROG`` with a non-zero Stream ID.
      4. Expect ``INVALID_PARAMETERS``.

  * -

      `ide_cxl_substream_id <../test/ide/cxl_substream_id.c>`_

    -

      R0178: CXL.cachemem IDE Substream ID is 0b1000.

    -

      1. Invoke a baseline CXL.cachemem ``FIRME_IDE_KEYSET_PROG`` with Substream ID 0b1000.
      2. Report ``SKIP`` only for ``NOT_SUPPORTED``; otherwise require ``SUCCESS``.
      3. Invoke CXL.cachemem ``FIRME_IDE_KEYSET_PROG`` with a different Substream ID.
      4. Expect ``INVALID_PARAMETERS``.

  * -

      `ide_keyset_prog_status <../test/ide/keyset_prog_status.c>`_

    -

      R0182: ``FIRME_IDE_KEYSET_PROG`` returns architected status codes.

    -

      1. Check that IDE key management and ``KEYSET_PROG`` are advertised.
      2. Invoke with an invalid ECAM base and expect ``INVALID_PARAMETERS``.
      3. Invoke with an unused IDE type and expect ``INVALID_PARAMETERS``.
      4. Invoke with a valid platform keyset and expect ``SUCCESS`` or ``INCOMPLETE``.
      5. If non-blocking, verify same-keyset conflict and poll completion.
      6. In the same FIRME caller instance, program independent random keys for
         a test-owned Root Port and its authenticated endpoint. Require protected
         traffic with matching keys, rejection after mismatching each endpoint
         direction, and recovery after restoring it.
      7. Include session and resource cleanup in the rule result. Without an
         observation backend, report SKIP rather than a status-only PASS.

  * -

      `ide_keyset_go_status <../test/ide/keyset_go_status.c>`_

    -

      R0184: ``FIRME_IDE_KEYSET_GO`` returns architected status codes.

    -

      1. Check that IDE key management and ``KEYSET_GO`` are advertised.
      2. Invoke with an invalid ECAM base and expect ``INVALID_PARAMETERS``.
      3. Invoke with an unused IDE type and expect ``INVALID_PARAMETERS``.
      4. Program a valid keyset, invoke ``KEYSET_GO``, and expect ``SUCCESS`` or ``INCOMPLETE``.
      5. If non-blocking, poll completion and expect ``SUCCESS``.
      6. Establish paired endpoint/Root Port keys and working protected traffic.
         Stop only one Root Port direction through the same FIRME instance;
         leave the endpoint active and require link-level rejection.
      7. Invoke Root Port GO and require protected traffic to recover. Repeat
         for the other direction and include cleanup in the rule result.
         Without an observation backend, report SKIP.

  * -

      `ide_keyset_stop_status <../test/ide/keyset_stop_status.c>`_

    -

      R0186: ``FIRME_IDE_KEYSET_STOP`` returns architected status codes.

    -

      1. Check that IDE key management and ``KEYSET_STOP`` are advertised.
      2. Invoke with an invalid ECAM base and expect ``INVALID_PARAMETERS``.
      3. Invoke with an unused IDE type and expect ``INVALID_PARAMETERS``.
      4. Program a valid keyset, invoke ``KEYSET_STOP``, and expect ``SUCCESS`` or ``INCOMPLETE``.
      5. If non-blocking, poll completion and expect ``SUCCESS``.
      6. Establish paired endpoint/Root Port keys and working protected traffic.
         Stop only one Root Port direction through the same FIRME instance;
         leave the endpoint active and require link-level rejection.
      7. Restart only that Root Port direction and require traffic to recover,
         proving that the failed access was caused by STOP, not a broken setup.
         Repeat for the other direction and include cleanup in the rule result.
         Without an observation backend, report SKIP.

  * -

      `ide_keyset_poll_status <../test/ide/keyset_poll_status.c>`_

    -

      R0192: ``FIRME_IDE_KEYSET_POLL`` returns architected status codes.

    -

      1. Check that IDE key management and ``KEYSET_POLL`` are advertised.
      2. Invoke with an invalid ECAM base and expect ``INVALID_PARAMETERS``.
      3. Poll a keyset with no pending response and expect ``DENIED``.
      4. Use the PAL-reserved stream for ``KEYSET_PROG``. If it completes
         synchronously, report the pending-response scenario as unavailable,
         not a DUT failure. Otherwise poll the exact keyset to ``SUCCESS``.
      5. After consuming that response, reuse the same keyset for poll-any.
         Verify the Handle and defined Keyset ID fields, ignoring unused bits.
      6. Drain owned requests and STOP the stream on success, SKIP, or failure.

  * -

      `ide_secure_caller_not_supported <../test/ide/secure_caller_not_supported.c>`_

    -

      R0194: IDE key management ABIs return ``NOT_SUPPORTED`` from Secure state.

    -

      1. From Secure world, invoke ``KEYSET_PROG``, ``KEYSET_GO``,
         ``KEYSET_STOP``, and ``KEYSET_POLL``.
      2. Expect ``NOT_SUPPORTED`` for each invocation.

  * -

      `ide_feature_register <../test/ide/feature_register.c>`_

    -

      R0198: IDE feature register 0 reports the defined ABI presence bits.

    -

      1. Check whether IDE key management is implemented at the current instance.
      2. If absent, report ``SKIP`` for that instance.
      3. Read IDE feature register 0 and ignore unused bits[63:4].
      4. Verify all mandatory IDE ABI presence bits are advertised where the service exists.

  * -

      `ide_abi_compliance <../test/ide/abi_compliance.c>`_

    -

      R0199: IDE key management ABIs follow the Table 4.2 compliance matrix.

    -

      1. From Secure world, expect IDE service version to return ``NOT_SUPPORTED``.
      2. From Realm and optional NS worlds, skip only if IDE key management is absent.
      3. Where the service is present, read feature register 0.
      4. Verify all mandatory IDE ABIs for that instance are advertised,
         ignoring unused output bits.

  * -

      `mec_common_mecid_width <../test/mec/mec_common_mecid_width.c>`_

    -

      R0206: MECID management uses the common MECID width.

    -

      1. Invoke ``FIRME_SERVICE_VERSION`` for the MECID management service.
      2. If the service is not implemented on the platform, report SKIP.
      3. Derive the ACS expected common MECID width from independently discovered requesters.
      4. The complete check requires the minimum width across all PEs and MEC-capable requesters such as SMMUs.
      5. If full requester discovery is unavailable, do not claim system common-width coverage.
      6. Read feature register 1 and decode ``COMMON_MECID_WIDTH`` from
         bits[3:0] as a width-minus-one value, as defined by ALP4 Table 5.1.
      7. Compare the DUT-advertised width against the ACS-derived expected width and fail on mismatch.
      8. Invoke ``FIRME_MEC_REFRESH`` with the largest MECID inside the common width and expect ``SUCCESS``.
      9. If encodable, invoke ``FIRME_MEC_REFRESH`` with the first MECID outside the common width.
      10. Expect ``INVALID_PARAMETERS``.

  * -

      `mec_refresh_abi <../test/mec/mec_refresh_abi.c>`_

    -

      R0208: ``FIRME_MEC_REFRESH`` returns architected status codes for caller and MECID-bound checks.

    -

      1. Invoke ``FIRME_MEC_REFRESH`` from NS and Secure callers.
      2. Expect ``NOT_SUPPORTED`` because MECID management is Realm-only.
      3. From Realm, derive the effective common MECID width only when the applicable specification revision defines the discovery encoding.
      4. Invoke valid create and destroy refresh requests and expect ``SUCCESS`` after BUSY retry.
      5. If the common MECID width is less than 16, invoke with the first encodable MECID outside that width.
      6. Expect ``INVALID_PARAMETERS`` for the out-of-range MECID; skip this negative check when width is 16.

  * -

      `granule_management_feature_registers <../test/gpt/granule_management_feature_registers.c>`_

    -

      R0162: Granule Management feature-register size, index, and encoding follow Table 3.9.

    -

      1. Invoke ``FIRME_SERVICE_VERSION`` for the Granule Management service.
      2. If the service is not implemented at the current FIRME instance, report SKIP.
      3. Read feature register 0, ignoring unused output bits.
      4. Read feature register 1, ignoring unused output bits.
      5. Decode the ``PGS`` field and verify it uses an allocated ``GPCCR_EL3.PGS`` encoding.
      6. Invoke ``FIRME_SERVICE_FEATURES`` for the first unallocated GM register index and expect
         ``NOT_SUPPORTED``.

  * -

      `granule_management_abi_compliance <../test/gpt/granule_management_abi_compliance.c>`_

    -

      R0163: Granule Management ABIs follow the Table 3.10 compliance matrix.

    -

      1. Invoke ``FIRME_SERVICE_VERSION`` for the Granule Management service.
      2. If the service is not implemented at the current FIRME instance, report SKIP.
      3. Read feature register 0, ignoring unused ABI-presence bits.
      4. Verify mandatory ABIs for the current FIRME instance are advertised.
      5. Verify not-supported ABIs for the current FIRME instance are not advertised.
      6. For the Realm FIRME instance, verify ``FIRME_GM_L1_GPT_CREATE`` and
         ``FIRME_GM_L1_GPT_DESTROY`` are either both advertised or both absent.

  * -

      `gpt_granule_size <../test/gpt/gpt_granule_size.c>`_

    -

      R0079: The GPI transition granule size is equal to the PGS advertised by the Granule Management service.

    -

      1. Check FEAT_RME and ``FIRME_GM_GPI_SET`` advertisement.
      2. Read Granule Management feature register 1 using ``FIRME_SERVICE_FEATURES``.
      3. Decode the PGS field and compare it with the PAL-configured GPT test granule size.

  * -

      Caller-side aligned-input requirement

    -

      R0083: ``FIRME_GM_GPI_SET`` base address is aligned to PGS.

    -

      1. Treat alignment as a caller precondition rather than observable DUT behaviour.
      2. Supply a PGS-aligned Base address in every valid ``FIRME_GM_GPI_SET`` scenario.
      3. Cover rejection of an unaligned Base address under R0116.
      4. Do not register a separate runtime test that misclassifies the R0116 negative case as R0083.

  * -

      `gpt_same_gpi_transition <../test/gpt/gpt_same_gpi_transition.c>`_

    -

      R0085: Same-GPI transitions are always permitted and complete as nops.

    -

      1. Check FEAT_RME and ``FIRME_GM_GPI_SET`` advertisement.
      2. Place the test granule at the target GPI for the caller world.
      3. Invoke ``FIRME_GM_GPI_SET`` again with Current GPI == Target GPI.
      4. Expect ``SUCCESS`` and a granule count of 1.

  * -

      `gpt_transition_feat_rme <../test/gpt/gpt_transition_feat_rme.c>`_

    -

      R0087: FEAT_RME permitted transitions (Table 3.1) via ``FIRME_GM_GPI_SET``.

    -

      1. Check FEAT_RME and ``FIRME_GM_GPI_SET`` advertisement.
      2. From the required caller worlds, attempt the Table 3.1 transitions:

         - Secure caller: NS <-> Secure
         - Realm caller:  NS <-> Realm

      3. For each transition:
         a) Ensure the test granule is at the **Current GPI**.
         b) Invoke ``FIRME_GM_GPI_SET`` from the specified world.
         c) Expect ``SUCCESS`` and verify enforcement using cross-world access probes.

  * -

      `gpt_transition_feat_gpc2 <../test/gpt/gpt_transition_feat_gpc2.c>`_

    -

      R0088: FEAT_RME_GPC2 additional NS<->NSO transitions (Table 3.2).

    -

      1. Check FEAT_RME_GPC2 and ``FIRME_GM_GPI_SET`` advertisement.
      2. From NS-EL2, attempt:

         - NS -> NSO
         - NSO -> NS

      3. Expect ``SUCCESS`` only when FEAT_RME_GPC2 is present; otherwise expect DENIED/NOT_SUPPORTED.
      4. Verify enforcement by probing the transitioned granule from Secure and Realm payloads and confirming
         the expected fault/no-fault behavior.

  * -

      `gpt_transition_feat_gdi <../test/gpt/gpt_transition_feat_gdi.c>`_

    -

      R0091: FEAT_RME_GDI NSP/SA transition policy (Table 3.3).

    -

      1. Check FEAT_RME_GDI and ``FIRME_GM_GPI_SET`` advertisement.
      2. From NS-EL2, attempt the Table 3.3 permitted transitions:

         - NS/NSO -> NSP
         - NS/NSO -> SA
         - NSP    -> NS/NSO
         - SA     -> NS/NSO

      3. Expect ``SUCCESS`` only when FEAT_RME_GDI is present; otherwise expect DENIED/NOT_SUPPORTED.
      4. Verify enforcement using cross-world probes and confirm no transitions beyond the union of Table 3.1–3.3
         are permitted.

  * -

      `gpt_start_granule <../test/gpt/gpt_start_granule.c>`_

    -

      R0104: ``FIRME_GM_GPI_SET`` starts at the granule at the Base address.

    -

      1. Check FEAT_RME and ``FIRME_GM_GPI_SET`` advertisement.
      2. From Secure and Realm worlds, seed two adjacent granules as NS.
      3. Request a one-granule transition at Base to the caller world's GPI.
      4. Verify the first granule is accessible from the caller world.
      5. Verify the adjacent granule remains NS by expecting a GPF from the caller world.
      6. Report NS coverage as skipped when no NS changing transition is available.

  * -

      `gpt_success_granule_count <../test/gpt/gpt_success_granule_count.c>`_

    -

      R0106: On ``SUCCESS``, the granule count output reports the number of successful transitions.

    -

      1. Check FEAT_RME and ``FIRME_GM_GPI_SET`` advertisement.
      2. From Secure and Realm worlds, seed two adjacent granules at the Current GPI.
      3. Invoke ``FIRME_GM_GPI_SET`` for a two-granule permitted transition.
      4. Expect ``SUCCESS`` and output granule count of 2.
      5. Report NS coverage as skipped when no NS changing transition is available.

  * -

      `gpt_success_count_nonzero <../test/gpt/gpt_success_count_nonzero.c>`_

    -

      R0113: On ``SUCCESS``, the granule count output is greater than or equal to 1.

    -

      1. Check FEAT_RME and ``FIRME_GM_GPI_SET`` advertisement.
      2. From NS, invoke the architecturally permitted NS-to-NS no-op.
      3. From Secure and Realm, invoke successful one-granule changing transitions.
      4. For ``INCOMPLETE``, resume with ``FIRME_GM_GPI_OP_CONTINUE`` and follow
         subsequent stateful or stateless phases until a terminal result.
      5. Require every observed ``SUCCESS`` to report a non-zero granule count.
      6. Keep this test body registry-gated until TF-A implements the ALP3 same-GPI
         success behaviour needed by the NS path.

  * -

      `gpt_invalid_parameters_status <../test/gpt/gpt_invalid_parameters_status.c>`_

    -

      R0116: Invalid ``FIRME_GM_GPI_SET`` parameters return ``INVALID_PARAMETERS``.

    -

      1. Check FEAT_RME and ``FIRME_GM_GPI_SET`` advertisement.
      2. Invoke with Granule count 0 and expect ``INVALID_PARAMETERS``.
      3. Invoke with an unallocated Target GPI in Attributes and expect ``INVALID_PARAMETERS``.
      4. Invoke with a Base address not aligned to PGS and expect ``INVALID_PARAMETERS``.
      5. Keep this strict ALP3 body registry-gated until TF-A returns the required
         status for all three malformed-input conditions.

  * -

      `gpt_denied_status <../test/gpt/gpt_denied_status.c>`_

    -

      R0120: Non-permitted ``FIRME_GM_GPI_SET`` transitions return ``DENIED``.

    -

      1. Check FEAT_RME and ``FIRME_GM_GPI_SET`` advertisement.
      2. From NS, request NS -> Root.
      3. Expect ``DENIED``.

Notes
=====

- The tests are designed to be deterministic and self-contained (no dependency on full RMM/SPMC runtimes).

License
=======

FIRME-ACS is distributed under the BSD-3-Clause License.

*Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.*
