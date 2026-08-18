# Arm DRTM Architecture Compliance Test Scenarios

**Specification**: DRTM Architecture For Arm [(DEN0113)
](https://developer.arm.com/documentation/den0113/latest)

---

## Table of contents

- [Arm DRTM Architecture](#arm-drtm-architecture)
  - [DRTM ACS](#drtm-acs)
  - [Interface](#interface)
    - [Test 1 Check DRTM implementation version](#test-1-check-drtm-implementation-version)
    - [Test 2 Check unsupported and reserved function IDs](#test-2-check-unsupported-and-reserved-function-ids)
    - [Test 3 Check mandatory function implementation](#test-3-check-mandatory-function-implementation)
    - [Test 4 Check TCB hash feature reserved bits](#test-4-check-tcb-hash-feature-reserved-bits)
    - [Test 5 Check DMA protection feature reserved bits](#test-5-check-dma-protection-feature-reserved-bits)
    - [Test 6 Check TPM feature reserved bits](#test-6-check-tpm-feature-reserved-bits)
    - [Test 7 Check minimum memory requirement reserved bits](#test-7-check-minimum-memory-requirement-reserved-bits)
    - [Test 8 Check PSCI version](#test-8-check-psci-version)
    - [Test 9 Check SMCCC version](#test-9-check-smccc-version)
    - [Test 10 Check GIC LPI clearing support](#test-10-check-gic-lpi-clearing-support)
    - [Test 11 Check GICR_PENDBASER modification restriction](#test-11-check-gicr_pendbaser-modification-restriction)
    - [Test 12 Check TCB hash lock function dependency](#test-12-check-tcb-hash-lock-function-dependency)
    - [Test 13 Check TCB hash table out-of-resource behavior](#test-13-check-tcb-hash-table-out-of-resource-behavior)
    - [Test 14 Check TCB hash invalid parameters and denied behavior](#test-14-check-tcb-hash-invalid-parameters-and-denied-behavior)
    - [Test 15 Check DLME image authentication feature reserved bits](#test-15-check-dlme-image-authentication-feature-reserved-bits)
  - [Dynamic Launch](#dynamic-launch)
    - [Test 101 Check DRTM and DLME alignment rules](#test-101-check-drtm-and-dlme-alignment-rules)
    - [Test 102 Check successful dynamic launch](#test-102-check-successful-dynamic-launch)
    - [Test 103 Check successive dynamic launch denied behavior](#test-103-check-successive-dynamic-launch-denied-behavior)
    - [Test 104 Check DRTM close locality](#test-104-check-drtm-close-locality)
    - [Test 105 Check DLME data rules and DCE populated data](#test-105-check-dlme-data-rules-and-dce-populated-data)
    - [Test 106 Check DRTM event log format and hash algorithm](#test-106-check-drtm-event-log-format-and-hash-algorithm)
    - [Test 107 Check secondary PEs are off during dynamic launch](#test-107-check-secondary-pes-are-off-during-dynamic-launch)
    - [Test 108 Check dynamic launch with invalid DRTM_PARAMETERS features](#test-108-check-dynamic-launch-with-invalid-drtm_parameters-features)
    - [Test 109 Check DLME Authorities Schema without image authentication](#test-109-check-dlme-authorities-schema-without-image-authentication)
    - [Test 110 Check dynamic launch only on boot PE](#test-110-check-dynamic-launch-only-on-boot-pe)
    - [Test 111 Check memory region descriptor requirements](#test-111-check-memory-region-descriptor-requirements)
    - [Test 112 Check DLME image authentication event log](#test-112-check-dlme-image-authentication-event-log)
    - [Test 113 Validate default PCR schema event ordering](#test-113-validate-default-pcr-schema-event-ordering)
    - [Test 114 Check NS asynchronous exceptions masked on boot PE](#test-114-check-ns-asynchronous-exceptions-masked-on-boot-pe)
    - [Test 115 Validate DLME Authorities event ordering](#test-115-validate-dlme-authorities-event-ordering)
    - [Test 116 Validate zero digest for non-distinct DCE](#test-116-validate-zero-digest-for-non-distinct-dce)
    - [Test 117 Check trustworthy ACPI tables](#test-117-check-trustworthy-acpi-tables)
    - [Test 118 Check debug exceptions masked on boot PE](#test-118-check-debug-exceptions-masked-on-boot-pe)
    - [Test 119 Check Enable Secure Interrupts command](#test-119-check-enable-secure-interrupts-command)
- [Appendix A. Revisions](#appendix-a-revisions)

---

## Arm DRTM Architecture

The objective of DRTM is to begin a new chain of trust and instantiate a smaller TCB that excludes untrusted and arbitrarily extensible components. DRTM does this by measuring and launching a protected payload.

Establishing an attestable TCB becomes difficult when the number of components in the boot chain grows or when firmware is dynamically extensible, for example by loading drivers from add-in peripherals. The larger and more complex the TCB, the greater the attack surface and the risk of untrusted code executing which can compromise security.

Dynamic Root of Trust for Measurement begins a new chain of trust by measuring and executing a protected payload. DRTM is implemented by a trusted agent that ensures:

- All cores are placed in a known state.
- The target payload is protected against modification.
- A single core measures and begins running the payload.
- Execution is confined to the payload.
- The payload is provided with data that can be used to validate key properties of the system.

### DRTM ACS

The tests are divided into interface and Dynamic Launch tests. Interface tests check compliance for DRTM functions and supported features. Dynamic Launch tests check compliance for successful dynamic launch with an example DLME image.

DRTM compliance also requires the system to be compliant with the BSA specification. Refer to the [BSA test scenario document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf) for BSA tests.

The tests are classified as:

- Interface
- Dynamic Launch

---

### Interface

#### Test 1 Check DRTM version

**Rule ID**: NA

**Scenario**:

- Major version of the DRTM implementation should be 1.
- Minor version of the DRTM implementation should be 1.

**Algorithm**:

- Invoke the `DRTM_VERSION` function.
- Check that the reported DRTM major version is 1.
- Check that the reported DRTM minor version is 1.
- Fail the test if either version field does not match the expected value.

---

#### Test 2 DRTM Invalid Function ID Test

**Rule ID**: NA

**Scenario**:

- Unimplemented functions return `NOT_SUPPORTED`.
- If reserved bits are not zero, the implementation returns `NOT_SUPPORTED`.

**Algorithm**:

- Pass an invalid function ID and check that the returned status is `NOT_SUPPORTED`.
- Pass invalid reserved bits and check that the returned status is `NOT_SUPPORTED`.
- Fail the test if any invalid request is accepted or returns an unexpected status.

---

#### Test 3 Check mandatory function

**Rule ID**: R31000

**Scenario**: DRTM instance must implement mandatory functions.

**Algorithm**:

- Invoke each mandatory DRTM function.
- Check that each mandatory function returns success status.
- Fail the test if any mandatory function is missing or returns an unexpected status.

---

#### Test 4 TCB hash invalid reserved bit check

**Rule ID**: NA

**Scenario**: TCB hash feature reserved bits must be zero.

**Algorithm**:

- Send the `DRTM_FEATURES` function with feature ID `0x5`.
- Check that the reserved bits in the returned TCB hash feature value are zero.
- Fail the test if any reserved bit is set.

---

#### Test 5 DMA Protection Support/Reserved check

**Rule ID**: R58000

**Scenario**: DMA protection feature reserved bits must be zero.

**Algorithm**:

- Send the `DRTM_FEATURES` function with feature ID `0x3`.
- Check that the reserved bits in the returned DMA protection feature value are zero.
- Fail the test if any reserved bit is set.

---

#### Test 6 TPM features invalid reserved check

**Rule ID**: NA

**Scenario**: TPM feature reserved bits must be zero.

**Algorithm**:

- Send the `DRTM_FEATURES` function with feature ID `0x1`.
- Check that the reserved bits in the returned TPM feature value are zero.
- Fail the test if any reserved bit is set.

---

#### Test 7 Min memory requirement features check

**Rule ID**: NA

**Scenario**: Minimum memory requirement reserved bits must be zero.

**Algorithm**:

- Send the `DRTM_FEATURES` function with feature ID `0x2`.
- Check that the reserved bits in the returned minimum memory requirement feature value are zero.
- Fail the test if any reserved bit is set.

---

#### Test 8 Check PSCI implemented version

**Rule ID**: NA

**Scenario**: The version of PSCI should be 1.0 or later.

**Algorithm**:

- Get the PSCI version.
- Check that the PSCI version is 1.0 or later.
- Fail the test if the PSCI version is older than 1.0.

---

#### Test 9 Check SMCCC implemented version

**Rule ID**: NA

**Scenario**: The version of SMCCC should be 1.0 or later.

**Algorithm**:

- Get the SMCCC version.
- Check that the SMCCC version is 1.0 or later.
- Fail the test if the SMCCC version is older than 1.0.

---

#### Test 10 Check GIC supports disabling LPIs

**Rule ID**: NA

**Scenario**: GIC that implements LPIs should support clearing `GICR_CTLR.EnableLPIs`.

**Algorithm**:

- Get the ITS address for the current ITS.
- Check that `GITS_CTLR.Enabled` is 0.
- Check that `GITS_CTLR.Quiescent` is 1.
- Fail the test if LPIs cannot be cleared as required.

---

#### Test 11 Check GICR_PENDBASER when LPIs enabled

**Rule ID**: NA

**Scenario**: If GIC does not support clearing `GICR_CTLR.EnableLPIs` after it is set, modification of `GICR_PENDBASER` when `GICR_CTLR.EnableLPIs == 1` must not be permitted.

**Algorithm**:

- Get the Redistributor base address for the current PE.
- Check that `GICR_CTLR.EnableLPIs` is 0.
- Check that `GICR_CTLR.RWP` is 0.
- Fail the test if `GICR_PENDBASER` modification is permitted when it must be blocked.

---

#### Test 12 Check DRTM_LOCK_TCB_HASHES function

**Rule ID**: R31010

**Scenario**: If the `DRTM_SET_TCB_HASH` function is implemented, the `DRTM_LOCK_TCB_HASHES` function must be implemented.

**Algorithm**:

- Check whether `DRTM_SET_TCB_HASH` is implemented.
- If `DRTM_SET_TCB_HASH` returns success, invoke `DRTM_FEATURES` using `DRTM_LOCK_TCB_HASHES` as the function ID.
- Check that `DRTM_LOCK_TCB_HASHES` returns success.
- Fail the test if `DRTM_SET_TCB_HASH` is implemented but `DRTM_LOCK_TCB_HASHES` is not implemented.

---

#### Test 13 Check SET_TCB_HASHES max hashes case

**Rule ID**: NA

**Scenario**: If the maximum number of entries in the hash table is exceeded, the implementation must return `OUT_OF_RESOURCE`.

**Algorithm**:

- Check whether `DRTM_SET_TCB_HASH` is implemented.
- Get the maximum number of available hash entries from `DRTM_FEATURES`.
- Fill the hash table with more hashes than the maximum supported entry count.
- Send `DRTM_SET_TCB_HASH`.
- Check that the returned status is `OUT_OF_RESOURCE`.
- Fail the test if the implementation accepts the oversized hash table or returns an unexpected status.

---

#### Test 14 Check TCB Hashes invalid and denied

**Rule ID**: NA

**Scenario**:

- Error in the hash table should result in `INVALID_PARAMETERS` status.
- Invoking `DRTM_SET_TCB_HASH` after `DRTM_LOCK_TCB_HASHES` should result in `DENIED` status.

**Algorithm**:

- Check whether `DRTM_SET_TCB_HASH` is implemented.
- Fill the hash table with an incorrect revision and set hashes using `DRTM_SET_TCB_HASH`.
- Check that `INVALID_PARAMETERS` is returned.
- Lock the hashes using `DRTM_LOCK_TCB_HASHES`.
- Fill the hash table again and send `DRTM_SET_TCB_HASH`.
- Check that `DENIED` is returned.
- Fail the test if either invalid-parameter or locked-hash behavior is not observed.

---

#### Test 15 DLME Img Auth invalid res bit check

**Rule ID**: NA

**Scenario**: DLME image authentication feature reserved bits must be zero.

**Algorithm**:

- Send the `DRTM_FEATURES` function with feature ID `0x6`.
- Check that the reserved bits in the returned DLME image authentication feature value are zero.
- Fail the test if any reserved bit is set.

---

### Dynamic Launch

#### Test 101 Check Dynamic Launch Invalid Parameter

**Rule ID**: R312010, R314010, R314020, R314030, R314040

**Scenario**:

- The DRTM parameters must start at a 4KB aligned address.
- The DLME region, image, and data must start at 4KB aligned addresses.
- The DLME image must come before the DLME data and must not overlap with it.

**Algorithm**:

- Send the `DRTM_FEATURES` function with feature ID `0x2` to get the minimum size of DLME data and set DRTM parameters.
- Send `DRTM_DYNAMIC_LAUNCH` with an unaligned DRTM parameters address and check that the returned status is `INVALID_PARAMETERS`.
- Send `DRTM_DYNAMIC_LAUNCH` with unaligned DLME region, image, and data start addresses in DRTM parameters and check that the returned status is `INVALID_PARAMETERS`.
- Swap the DLME image address and DLME data address so the DLME data is before the DLME image in DRTM parameters.
- Send `DRTM_DYNAMIC_LAUNCH` and check that the returned status is `INVALID_PARAMETERS`.

---

#### Test 102 Check DRTM Dynamic Launch Success

**Rule ID**: NA

**Scenario**: Dynamic Launch Event should succeed.

**Algorithm**:

- Send the `DRTM_FEATURES` function with feature ID `0x2` to get the minimum size of DLME data and set DRTM parameters.
- Send `DRTM_DYNAMIC_LAUNCH` with DRTM parameters.
- Check that Dynamic Launch is performed.
- Call DRTM Unprotect Memory.
- After Dynamic Launch, check the `x0` and `x1` values saved during DLME image execution against expected values.
- Check the `DRTM_GET_ERROR` success case when there are no errors in the previous Dynamic Launch.
- Call Unprotect Memory again and check that it returns `DENIED` because no memory is protected.

---

#### Test 103 Check Successive DL, DENIED Error Case

**Rule ID**: R43010

**Scenario**: Successive Dynamic Launch and Dynamic Launch denied error case.

**Algorithm**:

- Send the `DRTM_FEATURES` function with feature ID `0x2` to get the minimum size of DLME data and set DRTM parameters.
- Send `DRTM_DYNAMIC_LAUNCH` with DRTM parameters.
- Check that Dynamic Launch is performed.
- Do not call Unprotect Memory.
- After Dynamic Launch, check the `x0` and `x1` values saved during DLME image execution against expected values.
- Check the `DRTM_GET_ERROR` success case when there are no errors in the previous Dynamic Launch.
- Invoke Dynamic Launch again and check that it fails because Unprotect Memory was not done.
- Call Unprotect Memory and check that it returns success.
- Invoke a second Dynamic Launch.
- After Dynamic Launch, check the `x0` and `x1` values saved during DLME image execution against expected values.

---

#### Test 104 Check DRTM Close Locality

**Rule ID**: NA

**Scenario**: Check DRTM Close Locality.

**Algorithm**:

- Send the `DRTM_FEATURES` function with feature ID `0x2` to get the minimum size of DLME data and set DRTM parameters.
- Send `DRTM_DYNAMIC_LAUNCH` with DRTM parameters.
- Check that Dynamic Launch is performed.
- Check Close Locality for Locality 1 and confirm that it returns `INVALID_PARAMETERS`.
- Check Close Locality for Locality 2 and confirm that it returns success.
- Check Close Locality for Locality 2 again and confirm that it returns an already-closed status.

---

#### Test 105 Check DLME Data Rules

**Rule ID**: R45250, R45260, R45270, R314050, R314060, R314150

**Scenario**: Check DLME data rules and validate the DLME data populated by the DCE.

**Algorithm**:

- Send the `DRTM_FEATURES` function with feature ID `0x2` to get the minimum size of DLME data and set DRTM parameters.
- Send `DRTM_DYNAMIC_LAUNCH` with DRTM parameters.
- Check that Dynamic Launch is performed.
- Call DRTM Unprotect Memory.
- Read DLME data from the DLME data region.
- Validate that the DLME data header and all referenced subregions are within the DLME region.
- Validate that protected regions, the address map, and the event log are present and populated.
- Validate that protected regions and address map entries are 4KB aligned and sorted.
- If complete DMA protection is used, validate that a single protected region starts at address 0.
- If present, validate that the TCB hash table is valid and within the supported hash count.
- If present, validate that the ACPI table region starts with XSDT.
- Validate that only one of the TCB hash table or ACPI table region is present.

---

#### Test 106 Check DRTM event log

**Rule ID**: R45300, R48000

**Scenario**: Check DRTM event log format and measurement hash algorithm.

**Algorithm**:

- Send the `DRTM_FEATURES` function with feature ID `0x2` to get the minimum size of DLME data and set DRTM parameters.
- Send `DRTM_DYNAMIC_LAUNCH` with DRTM parameters.
- Check that Dynamic Launch is performed.
- Call DRTM Unprotect Memory.
- Read the event log from the DLME data region.
- Validate that the event log header has PCRIndex 0 and EventType `EV_NO_ACTION`.
- Validate that the TCG Event Spec signature is `Spec ID Event03`.
- Validate that event log entries can be parsed correctly.
- Validate that `EVTYPE_ARM_SEPARATOR` is present.
- Validate that DCE measurements use a hash algorithm valid for the TPM capabilities.

---

#### Test 107 Check Dynamic Launch Sec PE on

**Rule ID**: NA

**Scenario**: When the DLME image is launched on the boot PE, all other PEs should be off.

**Algorithm**:

- Send the `DRTM_FEATURES` function with feature ID `0x2` to get the minimum size of DLME data and set DRTM parameters.
- Switch on the secondary PE.
- Send `DRTM_DYNAMIC_LAUNCH` with DRTM parameters.
- Check that the returned status is `SECONDARY_PE_NOT_OFF`.

---

#### Test 108 Check Dynamic Launch Invalid Features

**Rule ID**: R44065

**Scenario**: Check Dynamic Launch with invalid features in `DRTM_PARAMETERS`.

**Algorithm**:

- Send the `DRTM_FEATURES` function with feature ID `0x2` to get the minimum size of DLME data and set DRTM parameters.
- If region memory protection is supported, set launch features of `DRTM_PARAMETERS` for all memory protection. Otherwise, set the opposite unsupported memory protection feature.
- Check that Dynamic Launch returns `INVALID_PARAMETERS`.
- If Default PCR Usage Schema is supported, set launch features of `DRTM_PARAMETERS` for DLME Authorities Schema. Otherwise, set the opposite unsupported PCR schema feature.
- Check that Dynamic Launch returns `INVALID_PARAMETERS`.
- If DLME image authentication is not supported, set launch features of `DRTM_PARAMETERS` for DLME image authentication.
- Check that Dynamic Launch returns `INVALID_PARAMETERS`.
- If TPM-based hashing is not supported, set launch features of `DRTM_PARAMETERS` for TPM-based hashing.
- Check that Dynamic Launch returns `INVALID_PARAMETERS`.

---

#### Test 109 Request DLME Schema without Image Auth

**Rule ID**: R312090

**Scenario**: Request for DLME Authorities Schema without image authentication error case.

**Algorithm**:

- Send the `DRTM_FEATURES` function with feature ID `0x2` to get the minimum size of DLME data and set DRTM parameters.
- Set the `DRTM_PARAMETERS` launch features to DLME Authorities Schema.
- Send `DRTM_DYNAMIC_LAUNCH`.
- Check that Dynamic Launch returns `INVALID_PARAMETERS`.

---

#### Test 110 Check DL on PE other than BOOT PE

**Rule ID**: R44030

**Scenario**: Dynamic Launch should be launched only on the boot PE.

**Algorithm**:

- Send the `DRTM_FEATURES` function with feature ID `0x2` to get the minimum size of DLME data and set DRTM parameters.
- If region memory protection is supported, set launch features of `DRTM_PARAMETERS` for all memory protection. Otherwise, set the opposite unsupported memory protection feature.
- Switch on the secondary PE.
- Send `DRTM_DYNAMIC_LAUNCH` with DRTM parameters to launch on a PE other than the boot PE.
- Check that the returned status is `DENIED`.

---

#### Test 111 Check Memory Region Desc Requirements

**Rule ID**: R313010, R313020

**Scenario**: Check Memory Region Descriptor requirements.

**Algorithm**:

- Send the `DRTM_FEATURES` function with feature ID `0x2` to get the minimum size of DLME data and set DRTM parameters.
- If complete memory region protection is not supported, or the maximum number of memory regions is zero, skip the test.
- Set a memory region address as 4KB unaligned.
- Pass the memory region table address to `DRTM_PARAMETERS` and send `DRTM_DYNAMIC_LAUNCH`.
- Check that Dynamic Launch returns `MEM_PROTECT_INVALID`.
- Set a memory region address as overlapping another region.
- Pass the memory region table address to `DRTM_PARAMETERS` and send `DRTM_DYNAMIC_LAUNCH`.
- Check that Dynamic Launch returns `MEM_PROTECT_INVALID`.

---

#### Test 112 Check DLME IMG AUTH when requested

**Rule ID**: R45242

**Scenario**: When DLME image authentication is requested, the DLME image should be authenticated.

**Algorithm**:

- Send the `DRTM_FEATURES` function with feature ID `0x2` to get the minimum size of DLME data and set DRTM parameters.
- Send `DRTM_DYNAMIC_LAUNCH` with DLME image authentication set in the launch features of `DRTM_PARAMETERS`.
- Check that Dynamic Launch is performed.
- Check whether the DLME image is authenticated in the event log.

---

#### Test 113 Validate default PCR schema event ordering

**Rule ID**: R48020

**Scenario**: Validate default PCR schema event ordering.

**Algorithm**:

- If Default PCR schema is not supported, skip the test.
- Initialize DRTM parameters.
- If `DRTM_ENABLE_SECURE_INTERRUPTS` is supported, set the Secure Interrupt Disable launch feature.
- Invoke `DRTM_DYNAMIC_LAUNCH`.
- If secure interrupts were disabled, call `DRTM_ENABLE_SECURE_INTERRUPTS` and expect success.
- Parse the DRTM event log.
- Validate PCR[17] ordering: DCE, PCR_SCHEMA, optional TZFW entries, optional DCE_SECONDARY entries, SECURE_INT_DISABLE if requested, SEPARATOR, and no extra events.
- Validate PCR[18] ordering: PCR_SCHEMA, optional DCE_PUBKEY entries, DLME, DLME_ENTRY_POINT, DEBUG_CONFIG, NONSECURE_CONFIG, SEPARATOR, and no extra events.

---

#### Test 114 Check NS async excp masked on boot PE

**Rule ID**: R45460

**Scenario**: Check NS asynchronous exceptions masked on boot PE.

**Algorithm**:

- Initialize DRTM parameters and invoke `DRTM_DYNAMIC_LAUNCH`.
- Call `DRTM_UNPROTECT_MEMORY`.
- Read DAIF on the boot PE.
- Verify that non-secure asynchronous exceptions, SError, IRQ, and FIQ are masked.

---

#### Test 115 Validate DLME Authorities event ordering

**Rule ID**: R48030

**Scenario**: Validate DLME Authorities event ordering.

**Algorithm**:

- If DLME image authentication or DLME Authorities PCR schema is not supported, skip the test.
- Initialize DRTM parameters.
- Request DLME image authentication and DLME Authorities PCR schema.
- If `DRTM_ENABLE_SECURE_INTERRUPTS` is supported, set the Secure Interrupt Disable launch feature.
- Invoke `DRTM_DYNAMIC_LAUNCH`.
- If secure interrupts were disabled, call `DRTM_ENABLE_SECURE_INTERRUPTS` and expect success.
- Parse the DRTM event log.
- Validate PCR[17] ordering: DCE, PCR_SCHEMA, optional TZFW entries, optional DCE_SECONDARY entries, SECURE_INT_DISABLE if requested, SEPARATOR, and no extra events.
- Validate PCR[18] ordering: PCR_SCHEMA, optional DCE_PUBKEY entries, DLME_PUBKEY, optional DLME_SVN, DLME_ENTRY_POINT, DEBUG_CONFIG, NONSECURE_CONFIG, SEPARATOR, and no extra events.

---

#### Test 116 Validate zero digest for non-distinct DCE

**Rule ID**: R44160

**Scenario**: Validate zero digest for non-distinct DCE.

**Algorithm**:

- If the D-CRTM and DCE images are distinct, skip the test.
- Get the firmware hash algorithm from `DRTM_FEATURES`.
- Invoke `DRTM_DYNAMIC_LAUNCH`.
- Call `DRTM_UNPROTECT_MEMORY`.
- Parse the DRTM event log.
- Verify that the DCE event is present in PCR[17] and its digest matches the digest of a one-byte zero value.
- Verify that the DCE_PUBKEY event is present in PCR[18] and its digest matches the digest of a one-byte zero value.

---

#### Test 117 Check Trustworthy ACPI Tables

**Rule ID**: R45440

**Scenario**: Check Trustworthy ACPI Tables.

**Algorithm**:

- Initialize DRTM parameters and invoke `DRTM_DYNAMIC_LAUNCH`.
- Call `DRTM_UNPROTECT_MEMORY` and validate the DLME result.
- Read DLME data and locate the TCB hash table and trustworthy ACPI table region.
- Verify that MADT, MCFG, GTDT, IORT, and TPM2 are present either as hashes in the TCB hash table or as tables in the trustworthy ACPI XSDT region.
- Fail the test if any required table is missing.

---

#### Test 118 Check Debug excp masked on boot PE

**Rule ID**: R45470

**Scenario**: Check debug exceptions masked on boot PE.

**Algorithm**:

- Initialize DRTM parameters and invoke `DRTM_DYNAMIC_LAUNCH`.
- Call `DRTM_UNPROTECT_MEMORY`.
- Read DAIF on the boot PE.
- Verify that debug exceptions are masked.

---

#### Test 119 Check Enable Secure Interrupts cmd

**Rule ID**: NA

**Scenario**: Check Enable Secure Interrupts command.

**Algorithm**:

- If `DRTM_ENABLE_SECURE_INTERRUPTS` is not implemented, skip the test.
- Initialize DRTM parameters and set the Secure Interrupt Disable launch feature.
- Invoke `DRTM_DYNAMIC_LAUNCH`.
- Call `DRTM_ENABLE_SECURE_INTERRUPTS` and expect success.
- Call `DRTM_ENABLE_SECURE_INTERRUPTS` again and expect `DENIED`.
- Call `DRTM_UNPROTECT_MEMORY`.

---

## Appendix A. Revisions

This appendix describes the technical changes between released issues of this document.

Issue 01

- First release.

Issue 02

- Updated Interface tests and Dynamic Launch tests.

Issue 03

- Added Dynamic Launch test scenarios 113-119.
- Updated Dynamic Launch test 105 and 106 algorithms.

---

*Copyright (c) 2026, Arm Limited and Contributors. All rights reserved.*
