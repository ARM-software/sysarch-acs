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

.. list-table::
  :header-rows: 1
  :widths: 22 20 58

  * - **Test Name**
    - **Validated rule(s)**
    - **Algorithm**

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

      `base_service_mandatory <../test/base/base_service_mandatory.c>`_

    -

      R0055: Base service discovery ABIs are mandatory at every implemented FIRME instance.

    -

      1. From each caller world, invoke ``FIRME_SERVICE_VERSION`` for the Base service.
      2. Expect SUCCESS.
      3. From each caller world, invoke ``FIRME_SERVICE_FEATURES`` for Base feature register 0.
      4. Expect SUCCESS.

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
      6. Read the DUT-advertised ``COMMON_MECID_WIDTH`` only when the applicable specification revision defines its feature-register encoding.
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

      `gpt_transition_feat_rme <../test/gpt/gpt_transition_feat_rme.c>`_

    -

      R0085: FEAT_RME permitted transitions (Table 3.1) via ``FIRME_GM_GPI_SET``.

    -

      1. Check FEAT_RME presence.
      2. From the required caller worlds, attempt the Table 3.1 transitions:

         - Secure caller: NS <-> Secure
         - Realm caller:  NS <-> Realm

      3. For each transition:
         a) Ensure the test granule is at the **Current GPI**.
         b) Invoke ``FIRME_GM_GPI_SET`` from the specified world.
         c) Expect SUCCESS and verify enforcement using cross-world access probes.

  * -

      `gpt_transition_feat_gpc2 <../test/gpt/gpt_transition_feat_gpc2.c>`_

    -

      R0086: FEAT_RME_GPC2 additional NS<->NSO transitions (Table 3.2).

    -

      1. Check FEAT_RME_GPC2 presence.
      2. From NS-EL2, attempt:

         - NS -> NSO
         - NSO -> NS

      3. Expect SUCCESS only when FEAT_RME_GPC2 is present; otherwise expect DENIED/NOT_SUPPORTED.
      4. Verify enforcement by probing the transitioned granule from Secure and Realm payloads and confirming
         the expected fault/no-fault behavior.

  * -

      `gpt_transition_feat_gdi <../test/gpt/gpt_transition_feat_gdi.c>`_

    -

      R0089: FEAT_RME_GDI NSP/SA transition policy (Table 3.3).

    -

      1. Check FEAT_RME_GDI presence.
      2. From NS-EL2, attempt the Table 3.3 permitted transitions:

         - NS/NSO -> NSP
         - NS/NSO -> SA
         - NSP    -> NS/NSO
         - SA     -> NS/NSO

      3. Expect SUCCESS only when FEAT_RME_GDI is present; otherwise expect DENIED/NOT_SUPPORTED.
      4. Verify enforcement using cross-world probes and confirm no transitions beyond the union of Table 3.1–3.3
         are permitted.

Notes
=====

- The tests are designed to be deterministic and self-contained (no dependency on full RMM/SPMC runtimes).

License
=======

FIRME-ACS is distributed under the BSD-3-Clause License.

*Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.*
