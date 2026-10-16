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

FIRME-ACS validates an EL3 firmware implementation of FIRME by exercising
its ABIs from the caller security states used in real deployments:

- Normal world: NS-EL2 dispatcher payload
- Secure world: S-EL2 shim payload with optional S-EL1 caller bodies
- Realm world: R-EL2 shim payload, reached through the platform Realm dispatcher

Scope and exclusions
====================

These scenarios target ACS release ``v26.10_FIRME_1.0.1`` against
DEN0149 FIRME 1.0 ALP4.

The ALP4 Alpha1 inventory contains 123 rules across Concepts/Base, Granule
Management, MECID Management, IDE Key Management and their ABI interfaces.
The following two rules are excluded from DUT validation with Architect
agreement; neither is executed or counted as a DUT PASS:

- ``R0023``: Service ID occupies bits [7:0]. Legal calls must zero the unused
  upper bits, so runtime discovery cannot reveal the DUT's internal type.
  Allocated-ID behavior is checked separately under R0024.
- ``R0041``: A feature-register response carries values, not register-size
  metadata. Zero is a valid value and cannot reveal the declared size.
  R0044 checks the specified mapping of defined outputs to result registers.

Local catalog assertions for these declarations are ACS checks, not DUT
compliance evidence. Revisit the exclusions if an independent observation
method becomes available.

Execution and evidence
======================

Algorithms below describe required checks, not a claim that every scenario
has been qualified. A missing mandatory ABI, invalid status or absent required
effect is a failure, even when the reference TF-A or model is incomplete.
Do not alter expected results or omit a rule merely to make regression pass.

Optional architectural features may be absent. A test requiring an
independent observer, exclusive resource or a controllable intermediate state
must report that missing prerequisite explicitly; status-only evidence must
not substitute for state/effect validation. Runtime SKIP and unpublished
fixtures remain coverage gaps, not waivers or PASS results.

Important prerequisites
-----------------------

- Shared-buffer rules use an actual applicable service, such as Attestation
  PAT retrieval, and owned memory. R0070/R0071 require independent mapping and
  PAS evidence. A successful retrieval does not establish these properties.
  The Realm adapter leases the firmware-provided boot shared page; it does
  not assume adjacent memory is owned or already mapped by EL3. Boundary
  requests that exceed that backing remain explicit coverage gaps.
  Realm initiation also requires a valid workload binding for the externally
  selected attestation model: copy it at offset ``0x40`` and pass its size in
  ``x5``. Continuations use ``x5=0``. The NS ``x4``-based initiation sequence
  is not a substitute. Missing model-specific binding evidence is an ACS
  fixture gap, not absent firmware support.
  Partial retrieval must use defined output fields; PAT's unused final
  Remaining-size output cannot establish R0080.
- R0031 checks publication/version-policy metadata rather than DUT behavior.
  It requires reviewed consecutive specification and service versions.
- GPT tests own their memory and L1 storage. Retained operations use real
  continuation cookies; unknown outstanding state prevents competing cleanup.
  Failed restoration or unresolved operation state quarantines GPT scratch.
  The original failure remains; later GPT bodies report a blocked SKIP while
  independent service tests continue. Blocked tests provide no rule coverage.
  Feature-present and absent cases use matched firmware/model configurations,
  not altered expected statuses. Missing mandatory GPT_INFO or continuation
  support is a failure; optional L1 absence remains distinguishable.
- IDE tests require an ACS-owned Root Port/endpoint path. Namespace isolation
  requires independent paths; asynchronous rules require real pending work.
  The optional paired CXL.mem backend supplies functional traffic evidence,
  not CXL.cache, wire-protocol, flit/IV or replay-protection certification.
- MEC semantic tests evict old plaintext to the required observation point
  and verify stale-data behavior plus a fresh-context round trip. A no-op
  refresh returning SUCCESS fails. Unclassified integrity faults do not
  prove ciphertext decorrelation.

Check the built registry and a complete UART log with
``tools/scripts/audit_alpha1_coverage.py``. Registration, source presence,
native harness checks and live DUT qualification are separate evidence.

Testcase checklist
==================

All 121 non-excluded checks are registered, including 24 generated ABI tests.
There are no registry gates. Caller-only audits, publication metadata and
unavailable observations remain separately identified; registration is not
complete DUT qualification.

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
      6. Verify preserved ``x18-x30``, SIMD/FP state, ``FPCR`` and ``FPSR``.
         Do not demand preservation of unused ``x1-x17`` output registers.

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

      1. Verify every ALP4 FIRME ABI function ID carries the SMC64 convention bit.
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
      6. Record the status/ABI combinations actually reached. Report SKIP for
         incomplete status coverage after preserving any observed failure;
         local status constants are not DUT evidence.

  * -

      `reserved_input_registers <../test/base/reserved_input_registers.c>`_

    -

      R0012: An unused input parameter register in a FIRME ABI is Reserved and SBZ.

    -

      1. Enable capture-only mode before invoking each production FIRME wrapper.
      2. Capture the final x0-x17 input frame immediately before SMC dispatch.
      3. Bypass the SMC while capture is enabled so GM, MEC and IDE audit requests
         cannot change DUT state.
      4. Exercise all 12 Alpha1 wrappers, including GM continuation, L1
         create/destroy and GPT_INFO, using independent specification masks.
      5. Verify every wholly unused input register is zero; the generic helper
         clears x8-x17 immediately before the capture point.
      6. Report PASS only when every captured wrapper frame satisfies SBZ.
         This is an ACS caller check, not proof about another firmware caller.

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
         Cover all 12 Alpha1 wrappers with independently specified masks;
         this is an ACS caller check, not proof about another firmware caller.

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
      8. Report incomplete evidence as SKIP until both parts are qualified;
         a passing caller self-check alone must not produce a DUT PASS.

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
      7. Report incomplete evidence as SKIP until both parts are qualified;
         a passing caller self-check alone must not produce a DUT PASS.

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
         expose identical discovery data. Report incomplete identity evidence
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

      `specification_version_policy <../test/base/specification_version_policy.c>`_

    -

      R0031: FIRME specification version increments track service version changes and new services.

    -

      1. Obtain consecutive specification versions and their service-version
         maps from the release-metadata adapter.
      2. Detect added or removed services and major or minor service-version
         increments.
      3. Require a specification major-version increment when a service is
         removed or a service major version is incremented.
      4. Require a specification minor-version increment when a service is
         added or a service minor version is incremented and no major-version
         transition supersedes it.
      5. Report missing reviewed metadata as SKIP; do not report this
         publication-policy check as DUT behavior.

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

      1. Iterate over all ALP4 service IDs at each FIRME instance.
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
      3. Iterate over non-Base ALP4 service IDs.
      4. If a non-Base service is implemented, use the already checked Base version as the dependency anchor.

  * -

      `service_version <../test/base/service_version.c>`_

    -

      R0038: ``FIRME_SERVICE_VERSION`` returns the requested service version or ``NOT_SUPPORTED``.

    -

      1. Invoke ``FIRME_SERVICE_VERSION`` for the Base service.
      2. Require an implemented version response and validate its defined
         major/minor encoding; the version is not a SUCCESS status code.
      3. Iterate over all unallocated 8-bit service IDs.
      4. Expect ``NOT_SUPPORTED`` for every unallocated service ID.

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
      5. Report the field-declaration claim as incomplete: legal boundary calls
         exercise only the allocated eight-bit field. They do not independently
         validate its architectural definition. This rule is not waived.

  * -

      `feature_register_abi_presence <../test/base/feature_register_abi_presence.c>`_

    -

      R0048: Feature register 0 reports which ABIs of an implemented service are implemented.

    -

      1. Invoke Base feature register 0 and verify the mandatory Base ABI bits are set.
      2. Iterate over implemented ALP4 service IDs using ``FIRME_SERVICE_VERSION``.
      3. For each implemented service, read feature register 0.
      4. Verify mandatory ABI bits are set and prohibited ABI bits are clear.
         Ignore unused output bits; they are not required to be zero.

  * -

      `service_feature_registers <../test/base/service_feature_registers.c>`_

    -

      R0051: Base service feature registers report architected fields and encodings.

    -

      1. Invoke ``FIRME_SERVICE_FEATURES`` for Base feature register 0.
      2. Expect SUCCESS and verify the mandatory Base ABI bits are set.
      3. Invoke ``FIRME_SERVICE_FEATURES`` for Base feature register 1.
      4. Expect SUCCESS and verify the shared-buffer size encoding is allocated.
      5. Verify allocated service-list bits contain only services permitted by Table 2.3
         for the caller world; ignore unused service-list bits.
      6. Do not require every optional Table 2.3 service to be present.

  * -

      `service_features <../test/base/service_features.c>`_

    -

      R0053: ``FIRME_SERVICE_FEATURES`` returns the requested feature register or ``NOT_SUPPORTED``.

    -

      1. Read Base feature registers 0 and 1 and require ``SUCCESS``.
      2. Use the Base service list and caller-world compliance table to
         identify implemented services at this instance.
      3. Query every index 0 through 255 for each allocated service ID.
         Require ``SUCCESS`` for defined registers of an implemented service
         and exactly ``NOT_SUPPORTED`` for absent services or unallocated indices.
      4. Query every unallocated Base index and require ``NOT_SUPPORTED``.
      5. Query register 0 at every unallocated 8-bit service ID and require
         ``NOT_SUPPORTED``.

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
         be all present or all absent. RAK_GET is conditional in Realm;
         require it whenever RAT_SIGN is advertised. RAT_SIGN is independent
         of the extension-claims group.
      6. Report missing mandatory ABIs as FAIL, without altering the
         specification's compliance classification for the current DUT.

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

      `max_concurrent_operations (R0064) <../test/base/max_concurrent_operations.c>`_

    -

      R0064: Every implemented non-Base service exposes a 16-bit
      ``MAX_CONCURRENT_OP_CNT`` field in feature register 1.

    -

      1. Discover every implemented non-Base service at each caller instance
         with ``FIRME_SERVICE_VERSION``.
      2. Read feature register 1 for every discovered service and require
         ``SUCCESS``.
      3. Decode the 16-bit ``MAX_CONCURRENT_OP_CNT`` field at the
         service-specific position defined by its ALP4 feature table.
      4. Interpret zero as no advertised maximum and a non-zero value as the
         advertised maximum.
      5. Fail if an implemented service does not expose feature register 1.
      6. IDE register 1 is required by ALP4 Table 4.1; S0068 does not waive
         this requirement. Its absence in TF-A is a confirmed DUT mismatch.
         Run the strict check and retain the failure until the DUT is corrected.

  * -

      `max_concurrent_operations (R0065) <../test/base/max_concurrent_operations.c>`_

    -

      R0065: A service advertises the same ``MAX_CONCURRENT_OP_CNT`` at every
      implemented instance and applies it collectively across its ABIs and
      instances.

    -

      1. Collect the R0064 value for every implemented instance of each
         non-Base service.
      2. Compare values for the same service across all caller worlds and fail
         any mismatch.
      3. If the common value is zero, record that no maximum is advertised and
         do not infer a capacity limit.
      4. For a non-zero value, retain stateful or asynchronous operations across
         the service ABIs and caller worlds, counting each operation once.
      5. Verify the number of simultaneously retained operations never exceeds
         the advertised value; use MP execution only where synchronous
         invocations must overlap.
      6. If other permitted resource limits prevent saturation, report the
         capacity subcase as not demonstrable rather than claiming full
         behavioral coverage.
      7. Complete or terminate every retained operation during cleanup.

  * -

      `shared_buffer_mapping_attributes <../test/base/shared_buffer_mapping_attributes.c>`_

    -

      R0070: A shared buffer is mapped as Normal, Write-Back Cacheable, Inner-shareable memory.

    -

      1. Select an implemented Attestation or Integrated Device ABI that
         consumes a valid, owned shared buffer with otherwise-valid inputs.
      2. Check the caller page-table attributes and invoke the ABI using that
         buffer. Require Normal, Write-Back Cacheable, Inner-shareable memory.
      3. Obtain independent EL3 mapping evidence through a reviewed
         implementation audit or approved observer and require the same
         attributes. A successful invocation alone does not prove them.
      4. Clean up the service operation and buffer mappings.
      5. Report SKIP if the service adapter or either set of mapping evidence
         is unavailable. Missing EL3 evidence remains an
         explicit coverage gap, not an inference from Base feature metadata.
         A failed valid invocation must remain FAIL even without an observer.
         Validate each available observation before reporting missing evidence;
         an observed attribute mismatch remains FAIL even if another observer
         is unavailable. A later retrieval or cleanup error must not be hidden
         by an initially accepted invocation.

  * -

      `shared_buffer_pas_gpi <../test/base/shared_buffer_pas_gpi.c>`_

    -

      R0071: The shared-buffer PAS and GPT GPI correspond to the FIRME instance state.

    -

      1. Select an implemented Attestation or Integrated Device ABI that
         consumes a shared buffer. Prepare an owned buffer and independently
         verify its PAS and GPT GPI match the invoking FIRME instance.
      2. Invoke with otherwise-valid inputs and check caller-visible
         ownership/access behavior using correctly attributed faults.
      3. Correlate the independent PAS/GPI observations with the exact buffer
         used by the invocation. Neither success nor a generic fault proves
         the GPI encoding or a confidentiality property. EL3 mapping
         attributes are checked separately by R0070.
      4. Clean up the service operation and buffer state.
      5. Report SKIP if the real service adapter or required independent
         ownership/mapping evidence is unavailable. Do not hide a failed
         valid invocation behind missing observation evidence.
         Check each available PAS/GPI observation independently; missing
         complementary evidence must not suppress an observed mismatch.

  * -

      `shared_buffer_size_bounds <../test/base/shared_buffer_size_bounds.c>`_

    -

      R0076: The shared-buffer size is within the advertised minimum and maximum bounds.

    -

      1. Read ``MIN_SH_BUF_SZ`` and ``MAX_SH_BUF_PG_CNT`` from Base feature
         register 1. Decode the minimum size and calculate maximum bytes as
         ``(MAX_SH_BUF_PG_CNT + 1) * minimum_size``.
      2. Prepare owned minimum/maximum buffers within reserved memory limits;
         record unavailable boundary fixtures as coverage gaps. Under RWXTJX,
         actual pages equal the encoded Page count plus one.
      3. Invoke a real buffer-consuming ABI with Page count zero and then
         ``MAX_SH_BUF_PG_CNT``. Require each valid boundary request to follow
         the selected ABI's normal path with otherwise-valid inputs.
      4. Exercise invalid-size cases only when representable and specified by
         that ABI, checking its defined error. A zero-based page count cannot
         encode a below-minimum buffer; do not fabricate unused-bit violations
         or require an unencodable above-maximum request. Record unencodable
         cases as not applicable.
      5. Clean up each operation; report missing service-specific evidence
         as SKIP, not PASS.

  * -

      `shared_buffer_alignment <../test/base/shared_buffer_alignment.c>`_

    -

      R0077: The shared-buffer physical base address is aligned to the minimum shared-buffer size.

    -

      1. Read the minimum shared-buffer size and alignment from Base feature
         register 1.
      2. Invoke a real buffer-consuming ABI with a base address aligned to the
         minimum size and establish a successful baseline using owned memory.
      3. Where the selected interface permits encoding a misaligned address,
         vary only that address and check the ABI's defined rejection. Audit
         the actual caller encoding; do not invent a rejection requirement for
         unused bits. Record unencodable misalignment cases as not applicable.
      4. Clean up each operation; report missing service-specific evidence
         as SKIP, not PASS.

  * -

      `partial_retrieval_last_chunk <../test/base/partial_retrieval_last_chunk.c>`_

    -

      R0080: Last-chunk partial retrieval reports non-zero written size and zero remaining size.

    -

      1. Use the service adapter to drive a real Attestation or Integrated
         Device retrieval through valid continuation calls to its final
         non-empty chunk.
      2. Require non-zero Written size and zero Remaining size; validate
         buffer bounds and the assembled data.
      3. Check return status against the selected ABI, not a universal
         ``SUCCESS`` requirement inferred from this Concepts rule.
      4. Clean up the retrieval operation and shared buffer.
      5. Report unavailable service or chunk evidence as SKIP, not PASS.

  * -

      `partial_retrieval_intermediate_chunk <../test/base/partial_retrieval_intermediate_chunk.c>`_

    -

      R0081: First or intermediate partial-retrieval chunks report written data and remaining data.

    -

      1. Use a supported, controlled buffer size to force a real retrieval to
         return a first or intermediate non-empty chunk.
      2. Require non-zero Written size and non-zero Remaining size, with
         bounded writes and a valid continuation.
      3. Check return status against the selected ABI; this Concepts rule
         does not impose a universal ``INCOMPLETE`` requirement.
      4. Finish the retrieval and clean up the operation and buffer.
         A later continuation error remains FAIL even after a valid first
         chunk; uncertain completion retains ownership of the buffer.
      5. Report unavailable service or chunk evidence as SKIP, not PASS.

  * -

      `partial_retrieval_no_chunk <../test/base/partial_retrieval_no_chunk.c>`_

    -

      R0082: No-data partial retrieval reports zero written size and zero remaining size.

    -

      1. Drive a real partial-retrieval ABI through a defined no-data path on
         which Written size and Remaining size are valid outputs.
      2. Require both sizes to be zero and verify that the caller consumes
         neither data nor undefined error-path outputs.
      3. Check return status against the selected ABI; do not impose a
         universal ``SUCCESS`` or ``INCOMPLETE`` requirement.
      4. Finish or recover the operation through the selected ABI's defined
         paths and clean up the buffer; do not assume a generic abort exists.
      5. Report unavailable service or chunk evidence as SKIP, not PASS.

  * -

      `ide_keyset_id_bitmap_encoding <../test/ide/keyset_id_bitmap_encoding.c>`_

    -

      R0205: IDE keyset ID fields follow the DEN0149 bitmap encoding.

    -

      1. Encode a Keyset ID using representative Segment, Root Port, Stream, Substream,
         Direction, and Key Set fields.
      2. Decode each field and verify the field occupies the DEN0149 bit position.
      3. Discover an unused Root Port/endpoint Selective IDE pair and encode
         its selected Stream ID and Root Port identity into the request.
      4. Invoke ``FIRME_IDE_KEYSET_PROG`` and require successful completion,
         polling the same keyset and checking its handle when asynchronous.
      5. Observe the exact selected stream, keyset, key and completed operation
         before cleanup. A separate CXL operation is not evidence for this
         Selective request; missing independent evidence remains SKIP.
      6. Stop the programmed keyset and restore the pair's configuration;
         retain a failure if cleanup cannot complete.

  * -

      `ide_cxl_key_set_unused <../test/ide/cxl_key_set_unused.c>`_

    -

      R0207: CXL.cachemem IDE treats the Key Set field as unused.

    -

      1. Keep the unused Key Set field zero in the production request.
      2. Invoke a valid CXL.cachemem ``FIRME_IDE_KEYSET_PROG`` with Key Set 0.
      3. Report ``SKIP`` only when the initial request returns ``NOT_SUPPORTED``
         without accepting an operation; otherwise require completion.
      4. GO and STOP the same keyset and require successful completion of each.
      5. On every exit, drain accepted work and STOP the known keyset. Cleanup
         failure remains FAIL; uncertain ownership blocks reuse until recovery/reset.

  * -

      `ide_cxl_stream_id_zero <../test/ide/cxl_stream_id_zero.c>`_

    -

      R0209: CXL.cachemem IDE Stream ID is zero.

    -

      1. Invoke a baseline CXL.cachemem ``FIRME_IDE_KEYSET_PROG`` with Stream ID 0.
      2. Skip only an initially unsupported path; otherwise require successful
         synchronous completion or exact-keyset polling with the correct handle.
      3. Repeat CXL.cachemem ``FIRME_IDE_KEYSET_PROG`` for Stream IDs 1 through 255.
      4. Expect ``INVALID_PARAMETERS``.
      5. Drain and STOP the valid baseline on every safe exit. Do not poll an
         unexpectedly accepted malformed keyset; report FAIL and retain ownership.

  * -

      `ide_cxl_substream_id <../test/ide/cxl_substream_id.c>`_

    -

      R0210: CXL.cachemem IDE Substream ID is 0b1000.

    -

      1. Invoke a baseline CXL.cachemem ``FIRME_IDE_KEYSET_PROG`` with Substream ID 0b1000.
      2. Skip only an initially unsupported path; otherwise require successful
         synchronous completion or exact-keyset polling with the correct handle.
      3. Repeat CXL.cachemem ``FIRME_IDE_KEYSET_PROG`` for every other four-bit
         Substream ID encoding, keeping all remaining fields valid.
      4. Expect ``INVALID_PARAMETERS``.
      5. Drain and STOP the valid baseline on every safe exit. Do not poll an
         unexpectedly accepted malformed keyset; report FAIL and retain ownership.

  * -

      `ide_ecam_keyset_namespace <../test/ide/ecam_keyset_namespace.c>`_

    -

      R0211: A Root Port ECAM address space defines the keyset-ID namespace.

    -

      1. Prepare two independently observable Root Port/endpoint paths with
         different ECAM bases but the same encoded keyset ID.
      2. Program distinct keys, start both streams, and require protected
         traffic through each endpoint path.
      3. Stop the first stream and require the second to remain active; then
         stop the second and require both paths to be idle.
      4. Report SKIP if two owned paths and independent traffic evidence
         are unavailable; ABI success alone is insufficient.

  * -

      `ide_keyset_id_uniqueness <../test/ide/keyset_id_uniqueness.c>`_

    -

      R0212: Each keyset ID uniquely selects a context within one ECAM space.

    -

      1. Own a paired endpoint session in one ECAM space. Program distinct
         Root Port RX/TX keyset IDs and keys through the selected FIRME
         instance, with matching endpoint keys. Verify protected traffic.
      2. Stop only Root Port RX. Require a fresh TX write to complete with
         its cipher counter advancing and no RX or failure counter change.
      3. Read the address and require the expected external abort, not a
         translation or granule protection fault, with failure evidence.
      4. Restart RX without reprogramming either key. Read the earlier
         pattern without rewriting it and require both cipher counters to
         advance. Include stopping both ends and session cleanup in the result.
      5. Report SKIP without the paired observer; ABI statuses alone do not
         prove keyset independence. This does not cover cross-ECAM isolation.

  * -

      `ide_keyset_prog_status <../test/ide/keyset_prog_status.c>`_

    -

      R0214: ``FIRME_IDE_KEYSET_PROG`` returns architected status codes.

    -

      1. Check that IDE key management and ``KEYSET_PROG`` are advertised.
         Discover an ACS-owned Root Port/endpoint pair and prepare an unused
         Selective stream in VAL. Use Realm access for its RMEDA lock when present.
      2. Invoke with an invalid ECAM base and expect ``INVALID_PARAMETERS``.
      3. Invoke with an unused IDE type and expect ``INVALID_PARAMETERS``.
      4. Invoke with the discovered keyset and expect ``SUCCESS`` or ``INCOMPLETE``.
      5. If non-blocking, verify same-keyset conflict and poll completion.
      6. Observe the exact completed request while retaining its Selective
         stream, keyset, key and handle. Require matching-key traffic and
         controlled mismatch/recovery evidence on that same path, not a
         newly created CXL operation.
      7. Include session and resource cleanup in the rule result. Without an
         observation backend, report SKIP rather than a status-only PASS.

  * -

      `ide_keyset_go_status <../test/ide/keyset_go_status.c>`_

    -

      R0216: ``FIRME_IDE_KEYSET_GO`` returns architected status codes.

    -

      1. Check that IDE key management and ``KEYSET_GO`` are advertised.
         Discover and prepare an unused Selective pair rather than assuming a
         platform-configured Stream ID; report unavailable resources explicitly.
      2. Invoke with an invalid ECAM base and expect ``INVALID_PARAMETERS``.
      3. Invoke with an unused IDE type and expect ``INVALID_PARAMETERS``.
      4. Prepare matching endpoint and Root Port keys on the owned Selective
         pair, invoke ``KEYSET_GO``, and expect ``SUCCESS`` or ``INCOMPLETE``.
      5. If non-blocking, poll completion and expect ``SUCCESS``.
      6. Retain the exact selected Selective stream and completed GO request;
         verify protected traffic with those same endpoint/Root Port keys.
         Stop only one Root Port direction through the same FIRME instance;
         leave the endpoint active and require link-level rejection.
      7. Invoke Root Port GO and require protected traffic to recover. Repeat
         for the other direction and include cleanup in the rule result.
         Without a same-operation observation backend, report SKIP. A separate
         CXL operation cannot qualify this Selective request.

  * -

      `ide_keyset_stop_status <../test/ide/keyset_stop_status.c>`_

    -

      R0218: ``FIRME_IDE_KEYSET_STOP`` returns architected status codes.

    -

      1. Check that IDE key management and ``KEYSET_STOP`` are advertised.
         Discover and prepare an unused Selective pair rather than assuming a
         platform-configured Stream ID; report unavailable resources explicitly.
      2. Invoke with an invalid ECAM base and expect ``INVALID_PARAMETERS``.
      3. Invoke with an unused IDE type and expect ``INVALID_PARAMETERS``.
      4. Program matching keys and establish protected traffic on the owned
         Selective pair. Invoke ``KEYSET_STOP`` on the selected Root Port
         direction and expect ``SUCCESS`` or ``INCOMPLETE``.
      5. If non-blocking, poll completion and expect ``SUCCESS``.
      6. Retain the exact selected Selective stream and completed STOP request;
         leave the endpoint active and require link-level rejection.
      7. Restart only that Root Port direction and require traffic to recover,
         proving that the failed access was caused by STOP, not a broken setup.
         Repeat for the other direction and include cleanup in the rule result.
         Without a same-operation observation backend, report SKIP. A separate
         CXL operation cannot qualify this Selective request.

  * -

      `ide_keyset_poll_status <../test/ide/keyset_poll_status.c>`_

    -

      R0225: ``FIRME_IDE_KEYSET_POLL`` returns architected status codes.

    -

      1. Check that IDE key management and ``KEYSET_POLL`` are advertised.
         Discover and prepare one unused Selective pair for sequential reuse.
      2. Invoke with an invalid ECAM base and expect ``INVALID_PARAMETERS``.
      3. Poll a keyset with no pending response and expect ``DENIED``.
      4. Use the VAL-prepared stream for ``KEYSET_PROG``. If it completes
         synchronously, report the pending-response scenario as unavailable,
         not a DUT failure. Otherwise poll the exact keyset to ``SUCCESS``.
      5. After consuming that response, reuse the same keyset for poll-any.
         Verify the Handle and defined Keyset ID fields, ignoring unused bits.
      6. Drain owned requests and STOP the stream on success, SKIP, or failure.
         Release only ACS-owned stream configuration and RMEDA locks after
         cleanup succeeds. Cleanup errors fail and quarantine the reservation.

  * -

      `ide_secure_caller_not_supported <../test/ide/secure_caller_not_supported.c>`_

    -

      R0227: IDE key management ABIs return ``NOT_SUPPORTED`` from Secure state.

    -

      1. From Secure world, invoke ``KEYSET_PROG``, ``KEYSET_GO``,
         ``KEYSET_STOP``, and ``KEYSET_POLL``.
      2. Expect ``NOT_SUPPORTED`` for each invocation.

  * -

      `ide_feature_register <../test/ide/feature_register.c>`_

    -

      R0231: IDE feature registers follow ALP4 Table 4.1.

    -

      1. Check whether IDE key management is implemented at the current instance.
      2. If absent, report ``SKIP`` for that instance.
      3. Read IDE feature register 0 and ignore unused bits[63:4].
      4. Verify all mandatory IDE ABI presence bits are advertised where the service exists.
      5. Require ``SUCCESS`` for feature register 1. Decode bits[15:0] as
         ``MAX_CONCURRENT_OP_CNT`` and ignore unused bits[63:16].
      6. Query unallocated indices beginning at 2 and require ``NOT_SUPPORTED``.

      Missing mandatory register 1 remains a failure, not an optional-feature skip.

  * -

      `ide_abi_compliance <../test/ide/abi_compliance.c>`_

    -

      R0232: IDE key management ABIs follow the Table 4.2 compliance matrix.

    -

      1. From Secure world, expect IDE service version to return ``NOT_SUPPORTED``.
      2. From Realm and optional NS worlds, skip only if IDE key management is absent.
      3. Where the service is present, read feature register 0.
      4. Verify all mandatory IDE ABIs for that instance are advertised,
         ignoring unused output bits.

  * -

      `mec_context_refresh <../test/mec/mec_context_refresh.c>`_

    -

      R0236: MEC refresh invalidates the previous encryption context and generates a new context.

    -

      1. Reserve a Realm page and a platform-owned MECID, then establish its
         initial encryption context.
      2. Map the page with that alternate MECID, write controlled plaintext
         samples at fixed physical addresses and verify the old-context reads.
      3. Clean and invalidate every sample to the Point of Encryption so the
         old protected representation is no longer hidden in a PE cache.
      4. Refresh the same MECID, completing any architected ``BUSY`` retries.
      5. Read the old protected representation through the replacement context.
         Require completed reads to differ from the original plaintext.
         Unrelated address, translation, permission or instruction faults fail;
         an unclassified data error or SError is inconclusive, not proof that
         the encryption context changed.
      6. Rewrite the plaintext through the replacement context, clean and
         invalidate it, then require a successful plaintext round trip. This
         excludes a broken memory path from producing a false pass.
      7. Retire the controlled context and restore the temporary mapping and
         MEC configuration. Report inconclusive fault evidence only after
         checking every sample, the new-context round trip and cleanup, so it
         cannot hide a known failure.

  * -

      `mec_ciphertext_decorrelation <../test/mec/mec_ciphertext_decorrelation.c>`_

    -

      R0238: Identical plaintext encrypted after MEC refresh does not match pre-refresh ciphertext.

    -

      1. Reserve a Realm page and platform-owned MECID, then write identical
         plaintext samples through its initial context at fixed physical
         addresses and verify them before refresh.
      2. Clean and invalidate every sample to the Point of Encryption and
         refresh the same MECID.
      3. Read the old protected representations through the replacement
         context. Every read must complete and return data different from the
         original plaintext.
      4. Rewrite the identical plaintext through the replacement context and
         require a successful post-refresh plaintext round trip.
      5. Infer decorrelation from the same-address stale-read mismatch: an old
         protected representation that were identical to the new one would
         decrypt to the original plaintext under the replacement context.
      6. Fail unrelated stale-read faults, including translation, alignment
         and GPC faults. Classify unconfirmed data-error syndromes as
         inconclusive, not as proof of ciphertext difference.
         Even a separately verified integrity abort would not
         prove that ciphertext bits, rather than integrity metadata, differ.
         Do not let an inconclusive sample hide another sample's mismatch,
         round-trip or cleanup failure.
      7. Retire the controlled context and restore all temporary state. The
         observed plaintext values are evidence and are not reported as raw
         ciphertext samples.

  * -

      `mec_common_mecid_width <../test/mec/mec_common_mecid_width.c>`_

    -

      R0239: MECID management uses the common MECID width.

    -

      1. Invoke ``FIRME_SERVICE_VERSION`` for the MECID management service.
      2. If the service is not implemented on the platform, report SKIP.
      3. Select all platform PEs through the normal MP test entry. On each PE,
         check MEC support and read its own ``MECIDR_EL2`` from Realm.
      4. Enumerate all SMMUs from the independent platform inventory. Read each
         SMMU's Realm MEC capability and ``SMMU_R_MECIDR`` without programming it.
      5. Compute the minimum width across all PEs and SMMUs. Missing capability
         leaves coverage incomplete; invalid inventory or failed register access
         must not silently exclude a requester or produce PASS.
      6. On every PE, read DUT-advertised ``COMMON_MECID_WIDTH`` from feature register 1.
         Decode bits[3:0] as width minus one, as specified by ALP4 Table 5.1.
      7. Compare the DUT-advertised width against the independently derived
         expected width and fail on mismatch.
      8. On every PE, invoke ``FIRME_MEC_REFRESH`` with the largest MECID inside
         the independently calculated common width and expect ``SUCCESS`` after
         bounded BUSY retries.
      9. If encodable, invoke ``FIRME_MEC_REFRESH`` with the first MECID outside the common width.
      10. Expect ``INVALID_PARAMETERS``. Preserve per-PE failures and confirm
          secondary shutdown before proceeding; report one aggregated rule result.

  * -

      `mec_refresh_abi <../test/mec/mec_refresh_abi.c>`_

    -

      R0241: ``FIRME_MEC_REFRESH`` refreshes the selected context and returns architected statuses.

    -

      1. Invoke ``FIRME_MEC_REFRESH`` from NS and Secure callers.
      2. Expect ``NOT_SUPPORTED`` because MECID management is Realm-only.
      3. Read the advertised common MECID width from MEC feature register 1.
      4. When width is less than 16, require ``INVALID_PARAMETERS`` for the
         first encodable out-of-width MECID. At width 16 this case is inapplicable.
      5. On the reserved MECID, store plaintext, evict it, refresh with the
         CREATE hint and require ``SUCCESS`` after bounded BUSY retries.
      6. Require completed stale-data reads to differ and a fresh-context
         rewrite/read to match. Unclassified integrity faults remain inconclusive.
      7. Repeat independently with the DESTROY hint. Observe old data before
         re-enabling with CREATE, then verify the fresh-context round trip.
         A disabled MECID can fault: unclassified faults remain incomplete evidence,
         while a completed stale read returning the original plaintext fails.
      8. Evict test data, require successful retirement and restore mappings.

  * -

      `mec_feature_registers <../test/mec/mec_feature_registers.c>`_

    -

      R0245: MECID management feature registers follow ALP4 Table 5.1.

    -

      1. In Realm, require an implemented MECID management service version.
      2. Read feature register 0 and require the mandatory
         ``FIRME_MEC_REFRESH`` bit; ignore unused output bits.
      3. Read feature register 1 and ignore unused bits[63:20]. Decode
         bits[19:4] as ``MAX_CONCURRENT_OP_CNT`` and bits[3:0] as common
         MECID width minus one. Independent width comparison belongs to R0239.
      4. Query every unallocated register index from 2 through 255 and require
         exactly ``NOT_SUPPORTED``.

  * -

      `mec_abi_compliance <../test/mec/mec_abi_compliance.c>`_

    -

      R0246: ``FIRME_MEC_REFRESH`` follows ALP4 Table 5.2 compliance.

    -

      1. From Normal and Secure worlds, require service absence and
         ``NOT_SUPPORTED`` from a direct refresh call.
      2. From Realm, require an implemented service version and the
         mandatory refresh ABI advertisement.
      3. Invoke a valid refresh and require ``SUCCESS`` after bounded
         ``BUSY`` retries; retire its test MECID before completion.

  * -

      `granule_management_feature_registers <../test/gpt/granule_management_feature_registers.c>`_

    -

      R0194: Granule Management feature-register size, index, and encoding follow Table 3.9.

    -

      1. Invoke ``FIRME_SERVICE_VERSION`` for the Granule Management service.
      2. If the service is not implemented at the current FIRME instance, report SKIP.
      3. Read feature register 0; require mandatory ABI bits and reject allocated
         ABI bits forbidden for this caller. Ignore unused output bits.
      4. Read feature register 1, ignoring unused output bits.
      5. Decode ``PGS``, ``L0GPTSZ``, and ``PPS`` and verify each uses an
         allocated architectural encoding.
      6. Query every unallocated GM register index from 2 through 255 and
         require exactly ``NOT_SUPPORTED``.

  * -

      `granule_management_abi_compliance <../test/gpt/granule_management_abi_compliance.c>`_

    -

      R0195: Granule Management ABIs follow the Table 3.10 compliance matrix.

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

      R0086: The GPI transition granule size is equal to the PGS advertised by the Granule Management service.

    -

      1. Check FEAT_RME and ``FIRME_GM_GPI_SET`` advertisement.
      2. Read Granule Management feature register 1 using ``FIRME_SERVICE_FEATURES``.
      3. Decode the PGS field and compare it with the PAL-configured GPT test granule size.

  * -

      `gpt_base_alignment <../test/gpt/gpt_base_alignment.c>`_

    -

      R0090: ``FIRME_GM_GPI_SET`` base address is aligned to PGS.

    -

      1. Read PGS from Granule Management feature register 1.
      2. From Secure and Realm, invoke ``FIRME_GM_GPI_SET`` with a deliberately
         misaligned Base address and expect ``INVALID_PARAMETERS``.
      3. Repeat the same changing transition with the PGS-aligned Base address.
      4. Expect ``SUCCESS`` with a one-granule count, then restore the granule
         to Non-secure state.

  * -

      `gpt_same_gpi_transition <../test/gpt/gpt_same_gpi_transition.c>`_

    -

      R0092: Same-GPI transitions are always permitted and complete as nops.

    -

      1. Check FEAT_RME and ``FIRME_GM_GPI_SET`` advertisement.
      2. Place the test granule at the target GPI for the caller world.
      3. Invoke ``FIRME_GM_GPI_SET`` again with Current GPI == Target GPI.
      4. Expect ``SUCCESS`` and a granule count of 1.

  * -

      `gpt_transition_feat_rme <../test/gpt/gpt_transition_feat_rme.c>`_

    -

      R0094: FEAT_RME permitted transitions (Table 3.1) via ``FIRME_GM_GPI_SET``.

    -

      1. Check FEAT_RME and ``FIRME_GM_GPI_SET`` advertisement.
      2. From the required caller worlds, attempt the Table 3.1 transitions:

         - Secure caller: NS <-> Secure
         - Realm caller:  NS <-> Realm

      3. For each transition:
         a) Ensure the test granule is at the **Current GPI**.
         b) Invoke ``FIRME_GM_GPI_SET`` from the specified world.
         c) Expect ``SUCCESS`` and verify enforcement using cross-world access probes.
      4. Preserve a failed initialization or transition as FAIL. Restore only
         changed storage after terminal completion. Host verifies the entire
         scratch range is NS before and after remote execution; protected
         callers must not nest cross-world transport during their rule body.
         Retained operations or unconfirmed ownership remain quarantined.

  * -

      `gpt_transition_feat_gpc2 <../test/gpt/gpt_transition_feat_gpc2.c>`_

    -

      R0095: FEAT_RME_GPC2 additional NS<->NSO transitions (Table 3.2).

    -

      1. Check FEAT_RME_GPC2 and ``FIRME_GM_GPI_SET`` advertisement.
      2. From NS-EL2, attempt:

         - NS -> NSO
         - NSO -> NS

      3. With GPC2, require ``SUCCESS`` and Granule count 1 for both transitions.
         Without GPC2, require ``INVALID_PARAMETERS`` for the unallocated NSO target.
      4. Verify NS access and Secure/Realm own-PAS granule protection faults.
      5. Before NSO -> NS, scrub the entire owned granule and clean to PoC.
         Verify the scrubbed data after release and check cleanup results.
      6. From Secure state, probe the same address through a Non-secure-output
         mapping: require access for NS and a granule protection fault for
         NSO. Restore translation state after each probe. An unavailable
         observer is incomplete coverage, not PASS; wrong ABI results remain
         FAIL and unknown retained state remains ERR.

  * -

      `gpt_transition_feat_gdi <../test/gpt/gpt_transition_feat_gdi.c>`_

    -

      R0098: FEAT_RME_GDI NSP/SA transition policy (Table 3.3).

    -

      1. Check FEAT_RME_GDI and ``FIRME_GM_GPI_SET`` advertisement.
      2. From NS-EL2, attempt the Table 3.3 permitted transitions:

         - NS/NSO -> NSP
         - NS/NSO -> SA
         - NSP    -> NS/NSO
         - SA     -> NS/NSO

      3. With GDI, require ``SUCCESS`` and Granule count 1 for every listed edge,
         including NSO paths. A failed NS -> NSO setup is FAIL, not omitted coverage.
         Without GDI, require ``INVALID_PARAMETERS`` for NSP and SA targets.
      4. Require NS, Secure and Realm PE probes to fault for NSP/SA; these PASs
         are not PE-accessible. Check NS access after restoration without assuming
         that data survives a change of PAS. Non-PE requester access is not claimed.
      5. Scrub and clean an NSO granule to PoC before cleanup releases it to NS.
         Restore NS and scrub the test granule, preserving any original failure.

  * -

      `gpt_start_granule <../test/gpt/gpt_start_granule.c>`_

    -

      R0111: ``FIRME_GM_GPI_SET`` starts at the granule at the Base address.

    -

      1. Check FEAT_RME and ``FIRME_GM_GPI_SET`` advertisement.
      2. Seed two adjacent granules as NS; require ``SUCCESS`` and count 1
         for each initialization, rather than accepting any terminal status.
      3. Request a one-granule transition at Base to the caller world's GPI.
      4. Verify the first granule is accessible from the caller world.
      5. Verify the adjacent granule remains NS by expecting a GPF from the caller world.
      6. When GDI is present, also run from NS with target NSP: the first
         granule must fault while the adjacent NS granule remains accessible.
         Without GDI, only this NS subcheck is skipped.
      7. Restore the changed granule with checked results, then independently
         verify both granules are NS from Host before reuse. Preserve the
         original failure; quarantine unresolved work or an unconfirmed range.

  * -

      `gpt_success_granule_count <../test/gpt/gpt_success_granule_count.c>`_

    -

      R0113: A valid granule count output reports the successful contiguous prefix.

    -

      1. Check FEAT_RME and ``FIRME_GM_GPI_SET`` advertisement.
      2. From Secure and Realm worlds, establish two adjacent granules as Non-secure.
      3. Invoke raw ``FIRME_GM_GPI_SET`` requests for the remaining range until
         both granules have transitioned; permit a partial ``SUCCESS`` count.
      4. After each ``SUCCESS``, verify that exactly the reported contiguous
         prefix changed to the caller GPI and that the first unreported granule
         remains Non-secure.
      5. Reissue the operation at the updated Base address for any remaining
         granules. Restore the known changed prefix and independently verify
         the whole range is NS from Host; do not issue redundant transitions
         to an untouched suffix. Unresolved operations prohibit cleanup.

      This body qualifies the ``SUCCESS`` path. Full rule coverage also needs
      the other statuses for which Table 3.4 defines a valid output count.

  * -

      `gpt_op_continue_start_granule <../test/gpt/gpt_op_continue_start_granule.c>`_

    -

      R0114: ``FIRME_GM_GPI_OP_CONTINUE`` starts after all granules reported
      by the original and preceding incomplete invocations.

    -

      1. Require advertised ``FIRME_GM_GPI_SET`` and
         ``FIRME_GM_GPI_OP_CONTINUE``.
      2. Lease an owned NS range through the Host fixture and issue a changing
         ``FIRME_GM_GPI_SET`` in the selected caller world. Require a real
         ``INCOMPLETE`` response; synchronous completion leaves this case SKIP.
      3. Continue using each returned cookie and accumulate only counts that
         are architecturally valid for the returned status.
      4. Require each continuation to begin at the accumulated count from the
         original operation base.
      5. Independently observe the changed prefix and unchanged suffix after
         every phase, including BUSY. Drain retained work, scrub NSO before
         release and restore the range; quarantine uncertain cleanup.

  * -

      `gpt_op_continue_granule_count <../test/gpt/gpt_op_continue_granule_count.c>`_

    -

      R0115: A valid ``FIRME_GM_GPI_OP_CONTINUE`` Granule count describes the
      successful prefix beginning at that invocation's start granule.

    -

      1. Use the owned Host fixture to obtain a real retained changing
         operation in the selected caller world and trace every continuation.
      2. For each status where the count is valid, require the count to fit
         within the suffix that existed at that invocation.
      3. Reject unsupported or unallocated status values for the valid setup.
      4. Use cross-PAS observations to verify the changed prefix and unchanged
         suffix. For NS, use NS-to-NSO with GPC2; NS-to-NS cannot prove progress.
      5. Drain retained state, scrub NSO before release and restore the range.
         Missing mandatory continuation fails; a wholly synchronous operation
         does not cover this rule and remains SKIP.

  * -

      `gpt_gpi_set_progress <../test/gpt/gpt_gpi_set_progress.c>`_

    -

      R0117: ``FIRME_GM_GPI_SET`` progress follows the status, Granule count,
      and operation-state relationships in Table 3.4.

    -

      1. Check FEAT_RME and ``FIRME_GM_GPI_SET`` advertisement.
      2. From Secure and Realm, establish two adjacent granules as Non-secure.
      3. Invoke a two-granule changing transition and inspect each raw response.
      4. Treat ``SUCCESS`` with a count equal to the request as complete; treat
         a smaller non-zero count as stateless progress and reissue
         ``FIRME_GM_GPI_SET`` for the remaining range.
      5. Retry ``BUSY`` with a bounded budget. Follow ``INCOMPLETE`` using
         the returned continuation cookie, including replacement cookies.
         Validate counts and the accessible prefix after each response;
         a probe infrastructure error is not a successful access.
      6. Invoke a non-permitted Root transition and require ``DENIED`` with a
         zero count and no observed access change.
      7. Restore the changed prefix only after a known terminal outcome, then
         independently verify the whole range is NS from Host. Unknown or
         retained state is an error and must prevent competing cleanup.
      8. Record which status rows were reached. Caller access probes establish
         prefix accessibility, not the exact GPI; cross-PAS observations are
         needed to distinguish an incorrect ANY transition.

  * -

      `gpt_op_continue_progress <../test/gpt/gpt_op_continue_progress.c>`_

    -

      R0118: ``FIRME_GM_GPI_OP_CONTINUE`` outputs follow the status, Granule
      count, and operation-cookie relationships in Table 3.5.

    -

      1. Require advertised ``FIRME_GM_GPI_OP_CONTINUE``.
      2. Use deterministic PAL scenarios for ``SUCCESS``, ``DENIED``,
         ``OP_CONFLICT``, ``NOT_FOUND``, ``INCOMPLETE``, and ``BUSY``.
      3. Require the exact expected status and consume the Granule count only
         for rows where Table 3.5 makes it valid.
      4. Require every valid count to fit the retained operation suffix.
      5. For ``INCOMPLETE`` and ``BUSY``, retry the cookie returned by that
         invocation and require another architected operation status.
      6. Clean up each deterministic scenario.

  * -

      `gpt_success_count_nonzero <../test/gpt/gpt_success_count_nonzero.c>`_

    -

      R0120: On ``SUCCESS``, the granule count output is greater than or equal to 1.

    -

      1. Check FEAT_RME and ``FIRME_GM_GPI_SET`` advertisement.
      2. From Secure and Realm, establish the test granule as Non-secure.
      3. Invoke a one-granule changing transition using ``FIRME_GM_GPI_SET``.
      4. Require every observed ``SUCCESS``, including successful setup and
         cleanup responses, to report a non-zero Granule count.
      5. Restore the granule to Non-secure state.
      6. Extend this rule test to ``FIRME_GM_GPI_OP_CONTINUE`` when that ABI is
         advertised by the DUT.

  * -

      `gpt_not_supported_status <../test/gpt/gpt_not_supported_status.c>`_

    -

      R0122: An unavailable GPI-management ABI returns ``NOT_SUPPORTED``.

    -

      1. Discover the advertised GPI-management ABIs without invoking an
         unallocated function ID.
      2. For a defined ABI reported absent, issue a bounded invocation and
         require exactly ``NOT_SUPPORTED``. Use owned scratch for GPI_SET;
         the OP_CONTINUE absence probe does not claim a live-cookie scenario.
      3. For an advertised ABI, require a genuine caller-disallowed fixture
         rather than inventing a restriction or continuation cookie.
      4. Clean up prepared fixtures. Unexpected acceptance retains uncertain
         state and quarantines storage; it must not trigger guessed cleanup.
         Skip only when neither absence nor a caller-disallowed case applies.

  * -

      `gpt_invalid_parameters_status <../test/gpt/gpt_invalid_parameters_status.c>`_

    -

      R0123: Invalid ``FIRME_GM_GPI_SET`` parameters return ``INVALID_PARAMETERS``.

    -

      1. Check FEAT_RME and ``FIRME_GM_GPI_SET`` advertisement.
      2. Invoke with Granule count 0 and expect ``INVALID_PARAMETERS``.
      3. Invoke with an unallocated Target GPI in Attributes and expect ``INVALID_PARAMETERS``.
      4. Invoke with a Base address not aligned to PGS and expect ``INVALID_PARAMETERS``.
      5. Fail on an incorrect status; missing TF-A behavior does not change
         these expectations.

  * -

      `gpt_busy_retry <../test/gpt/gpt_busy_retry.c>`_

    -

      R0124: ``BUSY`` reports temporary inability and permits the operation to
      be retried.

    -

      1. Require both mandatory GPI-management ABIs.
      2. Induce ``BUSY`` independently for ``GPI_SET`` and ``OP_CONTINUE``.
      3. Require exactly ``BUSY`` and verify that no additional GPI progress
         occurred.
      4. Retry ``GPI_SET`` with its original inputs and ``OP_CONTINUE`` with
         the cookie returned by ``BUSY``.
      5. Require an architected operation status and clean up each scenario.

  * -

      `gpt_l1_allocation_not_found <../test/gpt/gpt_l1_allocation_not_found.c>`_

    -

      R0126: An L1 GPT allocation failure returns ``NOT_FOUND``.

    -

      1. Use PAL to prepare a range containing an L0 Block that requires an L1
         object and to induce allocation failure.
      2. Invoke ``FIRME_GM_GPI_SET`` and require exactly ``NOT_FOUND``.
      3. Require the valid count to be strictly smaller than the requested
         count: allocation failure leaves at least one granule uncompleted.
      4. Where locally observable, verify that the count describes the exact
         successful prefix before the allocation failure.
      5. Restore the range and release all scenario resources.

  * -

      `gpt_denied_status <../test/gpt/gpt_denied_status.c>`_

    -

      R0127: Non-permitted ``FIRME_GM_GPI_SET`` transitions return ``DENIED``.

    -

      1. Check FEAT_RME and ``FIRME_GM_GPI_SET`` advertisement.
      2. Establish NS state with ``SUCCESS`` and count 1; independently probe
         caller access before the negative invocation.
      3. Request NS -> Root and require terminal ``DENIED`` with count 0.
      4. Verify the access matrix remains unchanged, then restore NS safely.

  * -

      `gpt_operation_conflict <../test/gpt/gpt_operation_conflict.c>`_

    -

      R0129: An operation overlapping a retained GPI operation returns
      ``OP_CONFLICT``.

    -

      1. Require both mandatory GPI-management ABIs.
      2. Use the owned Host fixture to retain a changing operation in the
         selected world. Target its first unfinished granule with a second
         one-granule ``GPI_SET`` request.
      3. Require exactly ``OP_CONFLICT``, count zero and no observed side
         effects; bound BUSY retries without consuming the original cookie.
      4. Continue the original operation to completion and independently
         verify its changed prefix and unchanged suffix.
      5. Restore the range, scrubbing NSO before release; quarantine uncertainty.
         This covers GPI-vs-GPI overlap. L1 conflicts require an additional
         L1 fixture and are not claimed by this path.

  * -

      `gpt_stateful_incomplete <../test/gpt/gpt_stateful_incomplete.c>`_

    -

      R0130: ``INCOMPLETE`` identifies a retained, stateful GPI operation.

    -

      1. Use the owned Host fixture to seek a real ``INCOMPLETE`` response
         from a changing ``GPI_SET`` in the selected caller world.
      2. Require a valid partial count and retain the returned cookie.
      3. Continue that cookie and reject ``INVALID_PARAMETERS``,
         ``NOT_SUPPORTED``, or any unallocated status.
      4. Continue through ``INCOMPLETE`` or ``BUSY`` to successful completion,
         independently checking the changed prefix and unchanged suffix.
      5. Fail on retry exhaustion, drain retained state and restore the range.
         Scrub NSO before release; quarantine uncertain cleanup. A wholly
         synchronous operation remains SKIP, not evidence of retained state.

  * -

      `gpt_protected_range_base_alignment <../test/gpt/gpt_protected_range_base_alignment.c>`_

    -

      R0102: A protected-range base is aligned to the L0GPTSZ coverage size.

    -

      1. Require advertised ``FIRME_GM_GPT_INFO`` and read GM feature register 1.
      2. Cross-check PGS, L0GPTSZ and PPS against independent PAL geometry.
      3. Derive the range containing the test PA and require ``SUCCESS`` for it.
      4. Move the base by one PGS while retaining an aligned top.
      5. Require ``INVALID_PARAMETERS`` for the misaligned base.

  * -

      `gpt_protected_range_size <../test/gpt/gpt_protected_range_size.c>`_

    -

      R0103: A protected-range size equals the L0GPTSZ coverage size.

    -

      1. Cross-check feature register 1 against independent PAL geometry.
      2. Derive and query one complete protected range; require ``SUCCESS``.
      3. Move the exclusive top inside the range by one PGS.
      4. Require ``INVALID_PARAMETERS`` for the partial protected range.

  * -

      `gpt_management_owner_stability <../test/gpt/gpt_management_owner_stability.c>`_

    -

      R0134: A protected range does not change between EL3-managed and Host-managed.

    -

      1. Query the protected range and record the owner bit in its PAR state.
      2. Transition one NS granule to the Secure or Realm GPI.
      3. Query the same range and require the owner bit to remain unchanged.
      4. Restore the granule to NS, query again, and require the original owner.
         Attempt restoration even after a malformed transition response;
         retain the original failure if cleanup also fails.

  * -

      `gpt_par_state_encoding <../test/gpt/gpt_par_state_encoding.c>`_

    -

      R0138: GPT_INFO uses the architected PAR-state encodings.

    -

      1. Query one independently described protected range.
      2. Require an allocated PAR state in the range 0-3.
      3. Compare the state with platform metadata that is independent of the
         GPT_INFO result.

  * -

      `gpt_info_status <../test/gpt/gpt_info_status.c>`_

    -

      R0139: GPT_INFO returns the status selected by its input conditions.

    -

      1. Require ``SUCCESS`` for an aligned range fully inside PPS.
      2. Require ``INVALID_PARAMETERS`` for a misaligned base.
      3. Require ``INVALID_PARAMETERS`` for a misaligned top.
      4. Require ``INVALID_PARAMETERS`` when top is equal to base.
      5. Require ``INVALID_PARAMETERS`` when top is less than base.
      6. Require ``INVALID_PARAMETERS`` for an aligned range outside PPS.

  * -

      `gpt_info_success_outputs <../test/gpt/gpt_info_success_outputs.c>`_

    -

      R0140: GPT_INFO SUCCESS returns a valid non-empty prefix and observed state.

    -

      1. Query exactly one protected range and require ``SUCCESS``.
      2. Require returned top to be greater than base, no greater than input top,
         and aligned to the protected-range size.
      3. Require the PAR state to use an allocated encoding.
      4. Compare the state with independent platform GPT metadata.

  * -

      `gpt_info_output_validity <../test/gpt/gpt_info_output_validity.c>`_

    -

      R0141: GPT_INFO range-top and PAR-state outputs are valid only on SUCCESS.

    -

      1. Consume and validate x1 and x2 after a successful query.
      2. Enable controlled caller-side poisoning for x1 and x2.
      3. Invoke GPT_INFO with a misaligned base and require
         ``INVALID_PARAMETERS`` using x0 only.
      4. Confirm the failing path does not consume the poisoned outputs.

  * -

      `gpt_info_concurrent_state <../test/gpt/gpt_info_concurrent_state.c>`_

    -

      R0144: A concurrent GPT change does not require ``GPT_INFO`` to return a
      single atomic snapshot, but its state must have been observed during the
      invocation.

    -

      1. Require advertised ``FIRME_GM_GPT_INFO`` and an MP-capable PAL path.
      2. Arrange a real protected-range state change on another PE, such as an
         L1 GPT lifecycle operation, and overlap it with ``GPT_INFO``. Record
         the state before and after the call; a GPI change alone is not proof
         that the protected-range state changed.
      3. Verify the queried range geometry against independent PAL metadata.
      4. Require the returned PAR state to be an allocated encoding equal to
         either the state observed before or after the concurrent transition.
      5. Reject an unrelated state; do not require an atomic before/after
         snapshot that the rule does not mandate.
      6. Fail if mandatory ``GPT_INFO`` is missing. Report SKIP if the
         independent, state-changing concurrent fixture is unavailable;
         MP worker startup alone does not provide this evidence.

  * -

      `l1_bridge_object_properties <../test/gpt/l1_bridge_object_properties.c>`_

    -

      R0156: A bridge L1 GPT object has valid geometry, Root ownership, and
      does not overlap the managed range or self-describing object.

    -

      1. Require the optional L1 CREATE/DESTROY pair and prepare a
         self-describing Host-managed range.
      2. Observe the temporary bridge independently while CREATE executes.
      3. Require EL3 ownership, Root GPI, advertised L1 size/alignment, and no
         overlap with the managed range or final object.
      4. Require the bridge to be reclaimed and restore all platform state.

  * -

      `l1_create_transition_policy <../test/gpt/l1_create_transition_policy.c>`_

    -

      R0161: L1 CREATE permits only the Realm Non-secure-to-Root transition.

    -

      1. Require ``NOT_SUPPORTED`` from Normal and Secure FIRME instances.
      2. Prepare a Realm non-self-describing object whose complete object
         range is independently observed as Non-secure.
      3. Complete CREATE and require every object granule to be Root.
      4. Restore the original range and object state.

  * -

      `l1_create_status <../test/gpt/l1_create_status.c>`_

    -

      R0167: L1 CREATE returns the exact status assigned to every condition.

    -

      1. Require ``NOT_SUPPORTED`` for callers outside the Realm instance.
      2. Exercise valid self-describing and non-self-describing requests.
      3. Isolate every invalid range/object, absent auxiliary L1, existing
         object, conflict, BUSY, allocation, GPI, and interrupted condition.
      4. Compare each invocation with its exact architected status and reject
         unknown or cross-ABI statuses.
      5. Clean up after every condition, including failure paths.

  * -

      `l1_create_range_gpi_stability <../test/gpt/l1_create_range_gpi_stability.c>`_

    -

      R0168: CREATE preserves non-object GPIs on ``SUCCESS`` and
      ``INCOMPLETE``.

    -

      1. Prepare self-describing and non-self-describing success and
         interruption scenarios.
      2. Independently check every protected-range granule not occupied by
         the self-describing object, or the complete non-self range.
      3. Invoke CREATE and require exact ``SUCCESS`` or ``INCOMPLETE``.
      4. Recheck every covered granule against its pre-operation GPI.

  * -

      `l1_create_success_object_gpi <../test/gpt/l1_create_success_object_gpi.c>`_

    -

      R0170: Successful CREATE assigns Root GPI to every object granule.

    -

      1. Prepare both L1 object layouts and verify every object granule starts
         Non-secure.
      2. Retry ``BUSY`` and continue ``INCOMPLETE`` requests until
         ``SUCCESS``.
      3. Independently require Root GPI for the complete object range.

  * -

      `l1_create_incomplete_object_gpi <../test/gpt/l1_create_incomplete_object_gpi.c>`_

    -

      R0171: Interrupted CREATE leaves every object granule Root or unchanged.

    -

      1. Prepare deterministic interrupted requests for both object layouts.
      2. Verify every object granule starts Non-secure.
      3. Require ``INCOMPLETE`` from the initial CREATE invocation.
      4. Require every object granule to be either Root or Non-secure, then
         complete or roll back the retained operation during cleanup.

  * -

      `l1_create_denied_no_effect <../test/gpt/l1_create_denied_no_effect.c>`_

    -

      R0173: A denied CREATE restores every object granule to its initial GPI.

    -

      1. Prepare an object with one known non-Non-secure granule and all other
         granules Non-secure.
      2. Record every object's GPI through independent PAL observation.
      3. Resume ``INCOMPLETE`` rollback and retry ``BUSY`` within a bound,
         requiring terminal ``DENIED`` rather than first-call ``DENIED``.
      4. Recheck the prefix, offending granule, and suffix against their exact
         pre-operation GPIs.

  * -

      `l1_destroy_transition_policy <../test/gpt/l1_destroy_transition_policy.c>`_

    -

      R0179: L1 DESTROY permits only the Realm Root-to-Non-secure transition.

    -

      1. Require ``NOT_SUPPORTED`` from Normal and Secure FIRME instances.
      2. Prepare a Realm object and verify every object granule is Root.
      3. Complete DESTROY and require every object granule to be Non-secure.

  * -

      `l1_destroy_scrub <../test/gpt/l1_destroy_scrub.c>`_

    -

      R0180: DESTROY scrubs the complete object before any Root-to-NS release.

    -

      1. Prepare existing self-describing and non-self-describing objects.
      2. Enable independent write and GPI-transition observation.
      3. Complete DESTROY.
      4. Require overwrite coverage for every object byte, a scrub value
         independent of the original contents, and scrub completion before
         any object granule becomes Non-secure.

  * -

      `l1_destroy_status <../test/gpt/l1_destroy_status.c>`_

    -

      R0184: L1 DESTROY returns the exact status assigned to every condition.

    -

      1. Require ``NOT_SUPPORTED`` for callers outside the Realm instance.
      2. Exercise valid objects and isolate invalid range, missing object,
         conflict, three DENIED layouts, bridge-allocation, BUSY, and
         interrupted conditions.
      3. Compare each invocation with its exact architected status.
      4. Clean up every case without retaining GPT state.

  * -

      `l1_destroy_success_output <../test/gpt/l1_destroy_success_output.c>`_

    -

      R0185: Successful DESTROY returns the exact physical L1 object base.

    -

      1. Prepare self-describing and non-self-describing objects with
         independently known bases.
      2. Complete DESTROY through all ``BUSY`` and ``INCOMPLETE`` phases.
      3. Consume x2 only after ``SUCCESS`` and compare it with the prepared
         object base.

  * -

      `l1_destroy_range_gpi_stability <../test/gpt/l1_destroy_range_gpi_stability.c>`_

    -

      R0186: DESTROY preserves non-object GPIs on ``SUCCESS`` and
      ``INCOMPLETE``.

    -

      1. Prepare both layouts for terminal and interrupted destruction.
      2. Check every non-object range granule before the invocation.
      3. Require exact ``SUCCESS`` or ``INCOMPLETE``.
      4. Recheck the complete covered range against its initial GPI.

  * -

      `l1_destroy_success_object_gpi <../test/gpt/l1_destroy_success_object_gpi.c>`_

    -

      R0188: Successful DESTROY assigns Non-secure GPI to every object granule.

    -

      1. Prepare both object layouts and verify every object granule is Root.
      2. Complete DESTROY.
      3. Independently require Non-secure GPI for the complete object range.

  * -

      `l1_destroy_incomplete_object_gpi <../test/gpt/l1_destroy_incomplete_object_gpi.c>`_

    -

      R0189: Interrupted DESTROY leaves every object granule Root or
      Non-secure.

    -

      1. Prepare deterministic interrupted destruction for both layouts.
      2. Verify every object granule starts Root.
      3. Require ``INCOMPLETE``.
      4. Require every object granule to be either Root or Non-secure before
         cleanup resumes or rolls back the operation.

  * -

      `l1_single_in_progress_operation <../test/gpt/l1_single_in_progress_operation.c>`_

    -

      R0191: At most one L1 CREATE or DESTROY operation owns a range.

    -

      1. Retain an interrupted CREATE and require a new CREATE and DESTROY for
         the same range to return ``OP_CONFLICT``.
      2. Resume the original CREATE with a zero L1 base to ``SUCCESS``.
      3. Retain an interrupted DESTROY and require CREATE for the same range
         to return ``OP_CONFLICT``.
      4. Resume the original DESTROY and restore the initial platform state.

  * -

      `generated_abi_gm_gpt_info_inputs <../test/gpt/generated_abi_gm_gpt_info_inputs.c>`_

    -

      R0356: GPT_INFO inputs occupy x1 and x2 with all other input registers SBZ.

    -

      1. Build an aligned inclusive base and exclusive top from feature register 1.
      2. Invoke GPT_INFO through the common x0-x17 ABI frame.
      3. Verify the generated input masks and production wrapper keep all
         unallocated input bits and registers zero.

  * -

      `generated_abi_gm_gpt_info_outputs <../test/gpt/generated_abi_gm_gpt_info_outputs.c>`_

    -

      R0357: GPT_INFO returns status in w0, range top in x1 and PAR state in x2.

    -

      1. Invoke a valid query through the common ABI framework.
      2. Require ``SUCCESS`` in w0.
      3. Validate x1 as a non-empty aligned prefix top.
      4. Validate x2 as an allocated PAR-state encoding.

  * -

      `generated_abi_ide_keyset_prog_inputs <../test/ide/generated_abi_ide_keyset_prog_inputs.c>`_

    -

      R0358: PROG inputs carry ECAM, IDE type, keyset, four key words and handle.

    -

      1. Reserve a real IDE path and construct x0-x8 with the ALP4 fields.
      2. Check unused input bits/registers are zero; invoke a valid request.
      3. Require exact ``INVALID_PARAMETERS`` for invalid ECAM or IDE type.

  * -

      `generated_abi_ide_keyset_prog_outputs <../test/ide/generated_abi_ide_keyset_prog_outputs.c>`_

    -

      R0359: PROG returns one of the Table 8.16 status codes in w0.

    -

      1. Require a valid request to complete with ``SUCCESS`` or ``INCOMPLETE``.
      2. Poll an incomplete request to terminal ``SUCCESS`` with its handle.
      3. Check exact invalid-input statuses and restore the reserved path.

  * -

      `generated_abi_ide_keyset_go_inputs <../test/ide/generated_abi_ide_keyset_go_inputs.c>`_

    -

      R0360: GO inputs carry ECAM, IDE type, keyset and handle.

    -

      1. Program a valid keyset first, completing any asynchronous PROG.
      2. Build x0-x4 and leave every other input register zero.
      3. Invoke valid, invalid-ECAM and invalid-type cases and clean up.

  * -

      `generated_abi_ide_keyset_go_outputs <../test/ide/generated_abi_ide_keyset_go_outputs.c>`_

    -

      R0361: GO returns one of the Table 8.18 status codes in w0.

    -

      1. Require ``SUCCESS`` or ``INCOMPLETE`` for a programmed keyset.
      2. Complete an asynchronous GO using POLL and verify its handle.
      3. Require ``INVALID_PARAMETERS`` for invalid ECAM or IDE type, then STOP.

  * -

      `generated_abi_ide_keyset_stop_inputs <../test/ide/generated_abi_ide_keyset_stop_inputs.c>`_

    -

      R0362: STOP inputs carry ECAM, IDE type, keyset and handle.

    -

      1. Program and start a valid keyset before the STOP request.
      2. Build x0-x4 with all other input registers zero.
      3. Invoke valid, invalid-ECAM and invalid-type cases and clean up.

  * -

      `generated_abi_ide_keyset_stop_outputs <../test/ide/generated_abi_ide_keyset_stop_outputs.c>`_

    -

      R0363: STOP returns one of the Table 8.20 status codes in w0.

    -

      1. Require ``SUCCESS`` or ``INCOMPLETE`` for a running keyset.
      2. Complete an asynchronous STOP using POLL and verify its handle.
      3. Require ``INVALID_PARAMETERS`` for invalid ECAM or IDE type.

  * -

      `generated_abi_ide_keyset_poll_inputs <../test/ide/generated_abi_ide_keyset_poll_inputs.c>`_

    -

      R0364: POLL inputs carry ECAM, pending selector and keyset ID.

    -

      1. Start a pending PROG with a unique caller handle.
      2. Test exact-keyset and any-pending selector inputs in x0-x3.
      3. Test invalid ECAM and no-pending conditions with exact statuses.

  * -

      `generated_abi_ide_keyset_poll_outputs <../test/ide/generated_abi_ide_keyset_poll_outputs.c>`_

    -

      R0365: POLL returns status and conditional handle/keyset outputs.

    -

      1. Require terminal ``SUCCESS`` after bounded ``INCOMPLETE`` retries.
      2. Compare x1 with the initiating handle, and x2 with the keyset ID
         only for a successful any-pending response, ignoring unused Keyset ID bits.
      3. Require ``DENIED`` when no operation is pending; restore all state.

Notes
=====

- The tests are designed to be deterministic and self-contained (no dependency on full RMM/SPMC runtimes).

License
=======

FIRME-ACS is distributed under the BSD-3-Clause License.

*Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.*
