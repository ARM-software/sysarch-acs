# Arm MPAM MSC Architecture Compliance Test Scenarios

**Specification**: MPAM Memory System Component Specification [(ARM IHI 0099B.c)](https://developer.arm.com/documentation/ihi0099/bc/)

---

## Table of contents
- [Arm MPAM MSC Architecture](#arm-mpam-msc-architecture)
  - [SYS-MPAM ACS](#sys-mpam-acs)
  - [Register Tests](#register-tests)
    - [Test 1 Check MPAM Version EXT Bit Check](#test-1-check-mpam-version-ext-bit-check)
    - [Test 4 Check MPAM for RME Feature Test](#test-4-check-mpam-for-rme-feature-test)
    - [Test 5 Check MSC Four Space Region Test](#test-5-check-msc-four-space-region-test)
    - [Test 6 Check NFU bit when PARTID NRW is impl](#test-6-check-nfu-bit-when-partid-nrw-is-impl)
  - [Cache Tests](#cache-tests)
    - [Test 101 Check CPOR Partitioning](#test-101-check-cpor-partitioning)
    - [Test 102 Check CCAP Partitioning](#test-102-check-ccap-partitioning)
    - [Test 104 Check CASSOC Partitioning](#test-104-check-cassoc-partitioning)
    - [Test 105 Check CMAX Partitioning with softlimit](#test-105-check-cmax-partitioning-with-softlimit)
    - [Test 106 Check CMIN resource controls](#test-106-check-cmin-resource-controls)
    - [Test 107 Check PARTID narrowing and monitoring](#test-107-check-partid-narrowing-and-monitoring)
    - [Test 108 Check narrowed request PARTID disable](#test-108-check-narrowed-request-partid-disable)
    - [Test 125 Check PARTID Disable functionality](#test-125-check-partid-disable-functionality)
  - [Monitor Tests](#monitor-tests)
    - [Test 151 Check PMG Storage by CPOR Nodes](#test-151-check-pmg-storage-by-cpor-nodes)
    - [Test 155 Check MSMON CTL Disable Behavior](#test-155-check-msmon-ctl-disable-behavior)
  - [Error Tests](#error-tests)
    - [Test 201 Check MSC PARTID selection range error](#test-201-check-msc-partid-selection-range-error)
    - [Test 203 Check MPAMCFG write with out-of-range PARTID](#test-203-check-mpamcfg-write-with-out-of-range-partid)
    - [Test 205 Check MSC MONITOR selection range error](#test-205-check-msc-monitor-selection-range-error)
    - [Test 207 Check Unexpected internal error](#test-207-check-unexpected-internal-error)
    - [Test 208 Check undefined RIS in MPAMCFG_PART_SEL error](#test-208-check-undefined-ris-in-mpamcfg_part_sel-error)
    - [Test 209 Check RIS no control error](#test-209-check-ris-no-control-error)
    - [Test 210 Check undefined RIS in MSMON_CFG_MON_SEL error](#test-210-check-undefined-ris-in-msmon_cfg_mon_sel-error)
    - [Test 212 Check MSC Level-Sensitive Error Interrupt Behaviour](#test-212-check-msc-level-sensitive-error-interrupt-behaviour)
    - [Test 213 Check MSC Edge-Trigger Error Interrupt Behaviour](#test-213-check-msc-edge-trigger-error-interrupt-behaviour)
    - [Test 214 Check MBWU monitor overflow interrupt functionality](#test-214-check-mbwu-monitor-overflow-interrupt-functionality)
    - [Test 215 Check MPAM IDR Zero for Undefined RIS](#test-215-check-mpam-idr-zero-for-undefined-ris)
    - [Test 216 Check MPAMF_ESR.RIS field correctness](#test-216-check-mpamf_esr-ris-field-correctness)
    - [Test 217 Check MPAM error overwrite behavior](#test-217-check-mpam-error-overwrite-behavior)
    - [Test 218 Check MBWU_L overflow interrupt status](#test-218-check-mbwu_l-overflow-interrupt-status)
    - [Test 219 Check MBWU Oflow MSI interrupt status](#test-219-check-mbwu-oflow-msi-interrupt-status)
  - [Memory Bandwidth Tests](#memory-bandwidth-tests)
    - [Test 301 Check MBWPBM Partitioning](#test-301-check-mbwpbm-partitioning)
    - [Test 302 Check MBWMIN Partitioning](#test-302-check-mbwmin-partitioning)
    - [Test 303 Check MBWMAX Partitioning](#test-303-check-mbwmax-partitioning)
    - [Test 305 Check MBWU OFLOW_FRZ overflow behavior](#test-305-check-mbwu-oflow_frz-overflow-behavior)
    - [Test 306 Check MBWU MSMON OFLOW Reset behavior](#test-306-check-mbwu-msmon-oflow-reset-behavior)
- [Appendix A. Revisions](#appendix-a-revisions)

---

## Arm MPAM MSC Architecture
The MPAM (Memory System Resource Partitioning and Monitoring) architecture is designed for systems running multiple applications or virtual machines on shared memory, particularly in enterprise networking and server environments. It addresses key requirements like controlling the performance impact of non-conforming software, bounding resource usage, and minimizing interference between software components.

MPAM achieves this through two main mechanisms: memory-system resource partitioning and resource usage monitoring. It propagates Partition IDs (PARTID) and Performance Monitoring Groups (PMG) across the memory system, enabling fine-grained control and monitoring of memory resources.

MPAM defines a framework for memory-system components (MSCs) to manage and monitor resource usage, providing standardized, implementation-independent memory-mapped interfaces. These features help align system performance with higher-level software goals and ensure consistent behaviour in complex, multi-tenant environments.

The goal of SYS-MPAM ACS is to test the errors in MSCs, partitioning, and monitoring related features for compliance against the MPAM MSC specification.

### SYS-MPAM ACS
The tests are classified as:
- Register Tests
- Cache Tests
- Monitor Tests
- Error Tests
- Memory Bandwidth Tests

---

### Register Tests

#### Test 1 Check MPAM Version EXT Bit Check

**Rules**: CFXGZ, DYDFT, GTKCX, JGWSM, JLYJJ, KNNBY, MMXWZ, WZXPM, ZPWLC, ZTFBQ

**Source**: [`test_pool/mpam/reg001.c`](../../test_pool/mpam/reg001.c)

**Scenario**: Validate the EXT field for supported MPAM versions.

**Algorithm**:

1. Get the total number of MSC nodes.
2. Loop through each MSC node by index.
3. If version is MPAM v1.0, check if EXT field is 0.
4. If version is MPAM v1.1, check if EXT field is 1.
5. If EXT field doesn’t match the expected value, print error and fail the test.
6. If version is not 1.0 or 1.1, print error and fail the test.
7. If all MSCs pass the check, mark the test as passed.

---



#### Test 4 Check MPAM for RME Feature Test

**Rules**: FXVBB, KXMHB, VBHPB

**Source**: [`test_pool/mpam/reg004.c`](../../test_pool/mpam/reg004.c)

**Scenario**: Verify MPAM behavior when RME is present.

**Algorithm**:

1. For each PE, read the ID_AA64PFR0_EL1 system register.
2. Check if ID_AA64PFR0_EL1.RME field is set (indicates RME support) and ID_AA64PFR0_EL1.MPAM is set.
3. Check if MPAM support is at least v1.1 (ID_AA64PFR0_EL1.MPAM == 0b0001 && ID_AA64PFR1_EL1.MPAM_frac == 0b0001).
4. Confirm the PE supports four MPAM PARTID spaces (MPAMIDR_EL1.SP4 == 1).
5. Verify the ALTSP feature is implemented ( MPAMIDR_EL1.HAS_ALTSP == 1).
6. If any condition fails, log an error.
7. If all PEs with RME meet the above criteria, mark the test as passed.

---

#### Test 5 Check MSC Four Space Region Test

**Rules**: GDMQZ, LRNPR

**Source**: [`test_pool/mpam/reg005.c`](../../test_pool/mpam/reg005.c)

**Scenario**: Verify at least one MSC advertises four-space regions when MPAM for RME is supported.

**Algorithm**:

1. Identify all MSC components in the system.
2. For each MSC, read the MPAMF_IDR.SP4 bit.
3. Confirm that at least one MSC has SP4 == 1, indicating it supports four MPAM PARTID spaces.
4. If no MSC has SP4 == 1, log an error and fail the test.
5. Otherwise, confirm that the system includes at least one four-space MPAM region and mark the test as passed.

---

#### Test 6 Check NFU bit when PARTID NRW is impl

**Rules**: RNSLZ

**Source**: [`test_pool/mpam/reg006.c`](../../test_pool/mpam/reg006.c)

**Scenario**: Ensure NFU is not implemented when PARTID narrowing is supported.

**Algorithm**:

1. Iterate MSC nodes and detect PARTID narrowing support.
2. For nodes with PARTID narrowing, read MPAMF_IDR and ensure HAS_NFU is 0.
3. Fail if any node reports HAS_NFU when PARTID narrowing is supported.

---

### Cache Tests

#### Test 101 Check CPOR Partitioning

**Rules**: CNGYT, CWTSC, GVCBB, KWGHQ, RLHRV, WMPLL, WSXSL, XPPBS, XPPGG, ZRSRZ, ZVTKX

**Source**: [`test_pool/mpam/partition001.c`](../../test_pool/mpam/partition001.c)

**Scenario**: Verify CPOR partitioning behaviour using CSU monitor counts.

**Algorithm**:

1. Get the index of the LLC and its cache identifier; skip the test if LLC info is invalid or not found.
2. For each MSC node:
  - Find matching cache resource nodes of type PE_CACHE.
  - Check if CPOR and CSU monitor are supported. Track number of CPOR-capable nodes and CSU monitor count.
  - Determine max cache size and min/max PARTID.
3. Skip the test if CPOR nodes or CSU monitors are not available.
4. Allocate memory for storing CSU monitor counters for each CPOR scenario. Read and modify MPAM2_EL2 to set PARTID and PMG.
5. For each MSC node with CPOR support:
  - Configure CPOR settings and disable CCAP partition settings.
  - Allocate source and destination buffers.
  - Configure and enable CSU monitor.
  - Wait for NRDY timeout to stabilize configuration.
  - Capture CSU monitor count before and after memory copy.
  - Store the count difference for analysis. Free allocated buffers.
6. Restore original MPAM2_EL2 value.
7. Compare CSU monitor counters across scenarios. If counter increases when it should decrease, mark test as failed.
8. Set test result to pass if all comparisons are valid, else fail.

---

#### Test 102 Check CCAP Partitioning

**Rules**: ZHBXH

**Source**: [`test_pool/mpam/partition002.c`](../../test_pool/mpam/partition002.c)

**Scenario**: Verify CCAP partitioning behaviour using CSU monitor counts.

**Algorithm**:

1. Get the index of the LLC and its cache identifier; skip the test if LLC info is invalid or not found.
2. For each MSC node:
  - Find matching cache resource nodes of type PE_CACHE.
  - Check if CCAP and CSU monitor are supported.
  - Track number of CCAP-capable nodes and CSU monitor count.
3. Determine max cache size and min/max PARTID. Skip the test if CCAP nodes or CSU monitors are not available.
4. Allocate memory for storing CSU monitor counters for each CCAP scenario. Read and modify MPAM2_EL2 to set PARTID and PMG.
5. For each MSC node with CCAP support: configure CCAP settings and disable CPOR partition settings.
6. Allocate source and destination buffers.
7. Configure and enable CSU monitor. Wait for NRDY timeout to stabilize configuration.
8. Capture CSU monitor count before and after memory copy. Store the count difference for analysis.
9. Free allocated buffers. Restore original MPAM2_EL2 value.
10. Compare CSU monitor counters across scenarios. If counter increases when it should decrease, mark test as failed.
11. Set test result to pass if all comparisons are valid, else fail.

---


#### Test 104 Check CASSOC Partitioning

**Rules**: BDRCM, YCBKB

**Source**: [`test_pool/mpam/partition004.c`](../../test_pool/mpam/partition004.c)

**Scenario**: Validate cache associativity partitioning enforcement.

**Algorithm**:

1. Identify an MSC with cache resources that support associativity partitioning by checking that MPAMF_IDR.HAS_CCAP_PART is set and MPAMF_CCAP_IDR.HAS_CASSOC is also set.
2. Select a valid PARTID, for example PARTID_X.
3. Configure MPAMCFG_CASSOC for PARTID_X to 100% cache associativity, which is typically represented by the value 0xFFFF.
4. Assign PARTID_X to the running PE using MPAM2_EL2.
5. Generate a memory workload from the PE to populate the cache and observe full usage of available ways.
6. Measure the cache usage using the CSU monitor and record the value.
7. Reconfigure MPAMCFG_CASSOC for PARTID_Y to restrict associativity to 50%, represented by a value like 0x7FFF.
8. Repeat the same memory workload under the same conditions.
9. Observe whether cache lines allocated to PARTID_X are now limited to fewer ways or if evictions occur earlier.
10. Use the CSU monitor to confirm the reduced cache allocation compared to the full associativity setting.
11. If the observed cache usage aligns with the restricted CASSOC setting, confirming enforcement of associativity limits, mark the test passed.
12. If the allocation behavior remains unchanged despite the restriction, indicating that the control is not being applied, mark the test failed.

---

#### Test 105 Check CMAX Partitioning with softlimit

**Rules**: RJFKG, SWNLN, VZBFH

**Source**: [`test_pool/mpam/partition005.c`](../../test_pool/mpam/partition005.c)

**Scenario**: Validate CMAX soft limit behavior for cache partitioning.

**Algorithm**:

1. Identify an MSC that supports cache maximum-capacity control and soft limit functionality by verifying that MPAMF_IDR.HAS_CCAP_PART, MPAMF_CCAP_IDR.HAS_CMAX, and MPAMF_CCAP_IDR.HAS_SOFTLIM are all set.
2. Select two PARTIDs, PARTID_X and PARTID_Y.
3. Configure MPAMCFG_CMAX such that PARTID_X is assigned 75% of the cache and PARTID_Y is assigned 25%, both with SOFTLIM = 0.
4. Program the PE with PARTID_X using MPAM2_EL2.
5. Generate a memory workload under PARTID_X to fully utilize its 75% cache share.
6. Measure cache usage using the CSU monitor and record it as Value A.
7. Program the PE with PARTID_Y using MPAM2_EL2 and disable it by writing to MPAMCFG_DIS.PARTID (do not set NFU).
8. Re-program the PE with PARTID_X again and set SOFTLIM = 1 for PARTID_X.
9. Perform the same memory workload from the PE.
10. Measure cache usage again with the CSU monitor and record it as Value B.
11. Compare Value B with Value A.
12. If Value B > Value A, it confirms that PARTID_X was able to use the additional 25% cache that was previously assigned to PARTID_Y, indicating correct SOFTLIM behavior.
13. If Value B is not significantly greater than Value A, the test fails.

---

#### Test 106 Check CMIN resource controls

**Rules**: CXZDR, DPZKC, FKNKB, FRTVY

**Source**: [`test_pool/mpam/partition006.c`](../../test_pool/mpam/partition006.c)

**Scenario**: Validate cache minimum-capacity resource control behavior.

**Algorithm**:

1. Identify an MSC that supports Cache Minimum-Capacity resource control by checking both MPAMF_IDR.HAS_CCAP_PART and MPAMF_CCAP_IDR.HAS_CMIN are set.
2. Select a valid PARTID (for example, PARTID_X) and configure MPAMCFG_CMAX to allocate a max of 100% of the cache capacity to this PARTID.
3. Perform a memory workload that utilizes the cache to ensure that PARTID_X is actively using its allocated cache capacity.
4. Select another PARTID (PARTID_Y) and configure MPAMCFG_CMAX to allocate a max of 50% of the cache capacity to it.
5. Perform a memory workload that utilizes the cache to ensure that PARTID_Y is actively using its allocated cache capacity.
6. Disable PARTID_Y by writing its ID to MPAMCFG_DIS.PARTID without setting the NFU bit, effectively marking its cache lines as highest priority for eviction.
7. Select a third PARTID (PARTID_Z) and configure MPAMCFG_CMIN to request 75% of the cache capacity.
8. Perform a memory workload that utilizes the cache to ensure that PARTID_Z is actively trying to use the cache.
9. Since cache usage by PARTID_Z is below its CMIN threshold, it should be given elevated allocation priority.
10. During cache pressure, the system is expected to evict cache lines from the disabled PARTID_Y first, followed by evictions from PARTID_X.
11. Use the CSU monitor to observe and confirm that cache usage increases for PARTID_Z while eviction or reduction in cache usage occurs for PARTID_X and disabled PARTID_Y.
12. If this behavior is observed, the test passes as it confirms that minimum-capacity rules and priority adjustments are correctly enforced.
13. If PARTID_Z does not receive allocation priority or if evictions do not follow the expected order, the test fails.

---


#### Test 107 Check PARTID narrowing and monitoring

**Rules**: GFYDF, ZGGMX

**Source**: [`test_pool/mpam/partition007.c`](../../test_pool/mpam/partition007.c)

**Scenario**: Verify request-to-internal PARTID mapping and monitor filtering when PARTID narrowing is enabled.

**Algorithm**:

1. Configure two request PARTIDs with distinct internal mappings and cache allocations.
2. Generate traffic for each request PARTID and measure it with CSU filters for the mapped internal PARTIDs.
3. Verify allocation and monitoring follow the mapping, then restore selectors, mappings, and controls.

---

#### Test 108 Check narrowed request PARTID disable

**Rules**: CSLHH

**Source**: [`test_pool/mpam/partition008.c`](../../test_pool/mpam/partition008.c)

**Scenario**: Verify that disabling an internal PARTID affects every request PARTID narrowed to it.

**Algorithm**:

1. Map request PARTIDs to one internal PARTID and populate its cache allocation.
2. Disable the internal PARTID, create competing traffic, and verify its CSU occupancy falls.
3. Restore the mapping, selectors, and partition controls.

---

#### Test 125 Check PARTID Disable functionality

**Rules**: TZWJG, VFJMZ

**Source**: [`test_pool/mpam/feat001.c`](../../test_pool/mpam/feat001.c)

**Scenario**: Validate PARTID disable/enable behavior.

**Algorithm**:

1. Check if the MSC supports PARTID enable/disable control by confirming MPAMF_IDR.HAS_ENDIS == 1.
2. Select a valid PARTID (for example, PARTID_X) and identify a cache resource that supports portion-based partitioning.
3. Program PARTID_X with a defined partition share (for example, 100%) using the cache’s CPOR configuration.
4. Set MPAM2_EL2 (or equivalent register) to assign PARTID_X to the active PE or thread.
5. Perform a memory workload (for example, memcpy or streaming load/store) large enough to utilize a measurable portion of the cache.
6. Use the CSU monitor to measure the cache usage by PARTID_X. Record the CSU counter as Value A.
7. Disable PARTID_X by writing its ID to MPAMCFG_DIS.PARTID. Do not set the NFU bit.
8. Re-run the same memory workload from the same PE.
9. Generate traffic for PARTID_Y with CPOR (100%).
10. Use the CSU monitor to measure the cache usage by PARTID_X. Record the CSU counter as Value B. Value B should be less than Value A and gradually decreasing.
11. Now, re-enable PARTID_X by writing it to MPAMCFG_EN.PARTID and disable PARTID_Y.
12. Re-run the same memory workload once more from the same PE.
13. Measure cache usage again using CSU. Record this as Value C.
14. Validate the results:
  - Value A ≈ Value C confirms that the original partition settings were preserved after re-enabling.
  - Value B < Value A confirms that the disabled PARTID was deprioritized or blocked from allocation.
  - If both conditions hold, the test is passed.

---

### Monitor Tests

#### Test 151 Check PMG Storage by CPOR Nodes

**Rules**: BDJXQ, GKWNB, JXRZM, RHYKB, VJDKF

**Source**: [`test_pool/mpam/monitor001.c`](../../test_pool/mpam/monitor001.c)

**Scenario**: Validate PMG storage behaviour for CPOR-capable nodes.

**Algorithm**:

1. Get the LLC index and cache identifier; skip the test if either is missing or invalid.
2. Loop through all MSC nodes and their resources to find PE_CACHE resources matching the LLC identifier that support CPOR; for each valid node:
  - Configure RIS if supported.
  - Update the max cache size.
  - Get the max PMG value.
  - Count CSU monitors if supported.
  - Track the minimum max PARTID value.
3. Skip the test if no CPOR-capable nodes or CSU monitors are available.
4. Loop through MSC nodes again and for each valid CPOR-capable PE_CACHE resource, configure CPOR partitioning to 75% using the selected PARTID.
5. Set up two PMG groups: PMG1 as default and PMG2 as the highest supported PMG. Read and store the current MPAM2_EL2 value, clear PARTID_D and PMG_D fields, set PARTID and PMG1, and write back the modified MPAM2_EL2.
6. For each matching PE_CACHE resource, allocate source and destination memory buffers sized to 50% of the cache size, log the buffer size, and skip if allocation fails.
7. Configure CSU monitor with PMG1, enable monitoring, wait for NRDY timeout, perform a memory copy transaction, and record the updated CSU monitor count (storage_value1).
8. Reconfigure CSU monitor with PMG2, enable monitoring, wait for NRDY timeout again, perform a second memory copy transaction, and record the updated CSU monitor count (storage_value2).
9. Disable CSU monitor, restore original MPAM2_EL2 value, and free the memory buffers.
10. Fail the test if storage_value1 is zero (no activity recorded for PMG1) or if storage_value2 is non-zero (unexpected activity for PMG2); otherwise, mark the test as passed.

---




#### Test 155 Check MSMON CTL Disable Behavior

**Rules**: DSCQB, VBBTL

**Source**: [`test_pool/mpam/monitor005.c`](../../test_pool/mpam/monitor005.c)

**Scenario**: Validate monitor control disable behavior for CSU monitors.

**Algorithm**:

1. Identify an MSC that supports monitoring (for example, CSU) by checking that MPAMF_IDR.HAS_MSMON is set.
2. Select a monitor instance using MSMON_CFG_MON_SEL.MON_SEL and, if applicable, set MSMON_CFG_MON_SEL.RIS.
3. Ensure the monitor is disabled by setting MSMON_CFG_CSU_CTL.EN = 0.
4. Record the current value of the monitor’s counter register (MSMON_CSU.VALUE).
5. Generate traffic that would normally increment the counter if the monitor were enabled.
6. Re-read the counter value and confirm that it remains unchanged.
7. Write a new value manually to the counter register (if writable) and confirm that the new value is accepted.
8. If the monitor does not count automatically while disabled and accepts software writes, mark the test as passed.
9. If the counter increments without being enabled or refuses valid writes, mark the test as failed.

---

### Error Tests

#### Test 201 Check MSC PARTID selection range error

**Rules**: DLSCZ, PFNBV, PNPQQ, WHSLQ

**Source**: [`test_pool/mpam/error001.c`](../../test_pool/mpam/error001.c)

**Scenario**: Validate error reporting for out-of-range PARTID selections.

**Algorithm**:

1. Get PE index and total MSC node count.
2. Loop through each MSC; skip if MPAMF_ESR is not supported.
3. Save current MPAMF_ECR value for later restore. Reset error code; fail test if reset fails.
4. Program MPAMCFG_PART_SEL with out-of-range PARTID. Wait briefly for error to propagate.
5. Read MPAMF_ESR and check for expected error code. If error code mismatch, log and mark test as failed.
6. Restore original MPAMF_ECR value.
7. After loop, skip test if no ESR-capable nodes were found; fail if any mismatch; else pass.

---


#### Test 203 Check MPAMCFG write with out-of-range PARTID

**Rules**: ZWRQW

**Source**: [`test_pool/mpam/error003.c`](../../test_pool/mpam/error003.c)

**Scenario**: Verify permitted error behavior for an MPAMCFG write made with an out-of-range PARTID selected.

**Algorithm**:

1. Select PARTID_MAX + 1 and continue when the out-of-range selector is retained.
2. Write a supported MPAMCFG register and accept either no error or the out-of-range PARTID error.
3. Restore the configuration register, selector, and error state.

---


#### Test 205 Check MSC MONITOR selection range error

**Rules**: VMTBP

**Source**: [`test_pool/mpam/error005.c`](../../test_pool/mpam/error005.c)

**Scenario**: Validate error reporting for out-of-range monitor selection.

**Algorithm**:

1. Get PE index and total MSC count. Loop through MSCs; skip if ESR not supported or no monitors are implemented.
2. Count CSU and MBWU monitors; skip MSC if none found.
3. Save MPAMF_ECR and reset error code; fail if reset fails. Mark test as executed, generate monitor selection range error using total monitor count.
4. Wait, read MPAMF_ESR, and check for expected error code; fail if mismatched.
5. Restore MPAMF_ECR and update final test status as skipped, failed, or passed.

---


#### Test 207 Check Unexpected internal error

**Rules**: CZZZK

**Source**: [`test_pool/mpam/error007.c`](../../test_pool/mpam/error007.c)

**Scenario**: Validate error reporting for unexpected internal errors.

**Algorithm**:

1. Get PE index and total number of MSCs.
2. Loop through MSCs; skip if MPAMF_ESR or PARTID narrowing is not supported. Save MPAMF_ECR and reset error code; fail if reset fails.
3. Mark test as executed, get max PARTID and max internal PARTID.
4. Configure MPAMCFG_PART_SEL to select internal mapping using a valid PARTID.
5. Configure MPAMCFG_INTPARTID with the max internal PARTID to trigger an unexpected internal error.
6. Wait, read MPAMF_ESR, and check for expected error code; fail if mismatched. Restore MPAMF_ECR and set final status as skipped, failed, or passed.

---

#### Test 208 Check undefined RIS in MPAMCFG_PART_SEL error

**Rules**: DQNFB, HHQWY, MRZLG, NPVVN, VKKDP, XDCDL, YQBMW, YXZFG

**Source**: [`test_pool/mpam/error008.c`](../../test_pool/mpam/error008.c)

**Scenario**: Validate error reporting for undefined RIS in MPAMCFG_PART_SEL.

**Algorithm**:

1. Get PE index and total number of MSCs.
2. Loop through MSCs; skip if MPAMF_ESR or RIS is not supported. Save MPAMF_ECR and reset error code; fail if reset fails.
3. Get max PARTID and max RIS index, then write an out-of-range RIS to MPAMCFG_PART_SEL.
4. Trigger error by accessing the same register; wait, then read MPAMF_ESR.
5. If expected RIS error code is not seen, check if MPAMCFG_PART_SEL behaves as RAZ/WI; fail if not.
6. Restore MPAMF_ECR, and mark the test as skipped, failed, or passed based on result.

---

#### Test 209 Check RIS no control error

**Rules**: PLZWM

**Source**: [`test_pool/mpam/error009.c`](../../test_pool/mpam/error009.c)

**Scenario**: Validate RIS_NO_CTRL error reporting when control registers are unimplemented.

**Algorithm**:

1. Get PE index and total MSC node count. Loop through MSCs; skip if MPAMF_ESR or RIS is not supported.
2. For each resource in the MSC, save MPAMF_ECR and reset error code; fail if reset fails. Select the current RIS index using MPAMCFG_PART_SEL.RIS.
3. Check if CCAP, CPOR, or MBW partitioning is unimplemented; if so, write to the corresponding partition register to trigger an error.
4. Read MPAMF_ESR and verify if the RIS_NO_CTRL error code is reported.
5. If error code mismatches, check if the register behaves as RAZ/WI; fail if not.
6. Restore MPAMF_ECR and update final test status as skipped, failed, or passed.

---

#### Test 210 Check undefined RIS in MSMON_CFG_MON_SEL error

**Rules**: DNNTB, HJRZC, LXXDB, SQTSJ

**Source**: [`test_pool/mpam/error010.c`](../../test_pool/mpam/error010.c)

**Scenario**: Validate error reporting for undefined RIS in MSMON_CFG_MON_SEL.

**Algorithm**:

1. Get PE index and total MSC count. Loop through MSCs; skip if MPAMF_ESR, RIS, or MSMON is not supported.
2. Save MPAMF_ECR and reset error code; fail if reset fails. Get max RIS index and write out-of-range RIS to MSMON_CFG_MON_SEL.
3. Access the same register to trigger the error; wait and read MPAMF_ESR.
4. If expected error code is not reported, verify if register behaves as RAZ/WI; fail if not.
5. Restore MPAMF_ECR and mark test as skipped, failed, or passed.

---


#### Test 212 Check MSC Level-Sensitive Error Interrupt Behaviour

**Rules**: GLPPK

**Source**: [`test_pool/mpam/intr001.c`](../../test_pool/mpam/intr001.c)

**Scenario**: Validate behaviour of level-sensitive MSC error interrupts.

**Algorithm**:

1. Get PE index and total MSC count. Loop through MSCs; skip if MPAMF_ESR is not supported.
2. Get error interrupt number and type; skip if interrupt is not level-triggered or not implemented.
3. Save MPAMF_ECR and reset error code; fail if reset fails. Register the interrupt handler and route the interrupt to the PE.
4. Trigger an MSC error interrupt by writing nonzero value to MPAMF_ESR. Poll until interrupt is received or timeout occurs.
5. Restore MPAMF_ECR; fail if interrupt not received in time.
6. Mark test as skipped if no MSC supports error interrupt; otherwise, pass or fail based on result.

---

#### Test 213 Check MSC Edge-Trigger Error Interrupt Behaviour

**Rules**: KVGWS

**Source**: [`test_pool/mpam/intr002.c`](../../test_pool/mpam/intr002.c)

**Scenario**: Validate behaviour of edge-triggered MSC error interrupts.

**Algorithm**:

1. Get PE index and total MSC count. Loop through MSCs; skip if MPAMF_ESR not supported.
2. Get error interrupt number and flags; skip if not edge-triggered or not implemented.
3. Save MPAMF_ECR and reset error code; fail if reset fails. Register interrupt handler and route the interrupt to the PE.
4. Try to trigger an MSC error interrupt via MPAMF_ESR. Poll for interrupt completion; if handler catches interrupt in time, fail the test.
5. Restore MPAMF_ECR and mark test as passed, failed, or skipped based on result.

---

#### Test 214 Check MBWU monitor overflow interrupt functionality

**Rules**: YLCVK

**Source**: [`test_pool/mpam/intr003.c`](../../test_pool/mpam/intr003.c)

**Scenario**: Validate MBWU monitor overflow interrupt handling.

**Algorithm**:

1. Get PE index and total MSC count, save and update MPAM2_EL2 with default PARTID and PMG.
2. Loop through MSCs and their resources; check for RIS, MBWU monitor, and overflow interrupt support.
3. Skip if monitor or interrupt not supported; otherwise, register interrupt handler and route it to PE. Trigger MBWU overflow by enabling monitor, writing the maximum supported counter value to MBWU monitor and performing large memory copy using 10MB buffers.
4. Wait for NRDY timeout, perform memory copy, and poll for interrupt completion.
5. If interrupt not received in time, fail the test; otherwise, reset status and free buffers. Restore MPAM2_EL2, disable and reset monitor after test.
6. Mark test as skipped if no MSC supports overflow interrupt; else pass or fail based on result.

---

#### Test 215 Check MPAM IDR Zero for Undefined RIS

**Rules**: HSFKH, XTQZR

**Source**: [`test_pool/mpam/error012.c`](../../test_pool/mpam/error012.c)

**Scenario**: Validate MPAM IDR registers read as zero for undefined RIS values.

**Algorithm**:

1. Identify all MSC components that implement RIS (MPAMF_IDR.HAS_RIS == 1).
2. For each such MSC, determine MPAMF_IDR.RIS_MAX (maximum defined RIS value).
3. For testing, set MPAMCFG_PART_SEL.RIS to a value greater than RIS_MAX (an undefined RIS).
4. Attempt to read an MPAMF ID register (for example, MPAMF_CCAP_IDR or MPAMF_MBW_IDR).
5. Verify the following behavior:
  - All HAS_* fields in the register must read as 0.
  - MPAMF_ESR must not log an error (ERRCODE == 0).
  - No error interrupt should be raised.
6. If any HAS_* field is non-zero or an error is logged or interrupt is generated, fail the test.
7. Otherwise, mark the test as passed.

---

#### Test 216 Check MPAMF_ESR.RIS field correctness

**Rules**: QBKDG

**Source**: [`test_pool/mpam/error013.c`](../../test_pool/mpam/error013.c)

**Scenario**: Validate MPAMF_ESR.RIS reports the programmed RIS on error.

**Algorithm**:

1. Identify MSC components that implement RIS (MPAMF_IDR.HAS_RIS == 1).
2. For each MSC, select a valid RIS value and configure it via MPAMCFG_PART_SEL.RIS.
3. Intentionally trigger an error involving a MPAMCFG_* register access (for example, access a non-implemented control).
4. Read MPAMF_ESR and verify that MPAMF_ESR.RIS == MPAMCFG_PART_SEL.RIS.
5. Next, select a valid RIS via MSMON_CFG_MON_SEL.RIS.
6. Trigger an error involving a MSMON_* register access (for example, read from an unimplemented monitor index).
7. Read MPAMF_ESR again and verify that MPAMF_ESR.RIS == MSMON_CFG_MON_SEL.RIS.
8. If any RIS value in MPAMF_ESR does not match the expected source selector, log an error and fail the test.
9. If all comparisons match expected RIS values, mark the test as passed.

---

#### Test 217 Check MPAM error overwrite behavior

**Rules**: DYNFP, GVBGS

**Source**: [`test_pool/mpam/error014.c`](../../test_pool/mpam/error014.c)

**Scenario**: Validate MPAM error overwrite behavior when multiple errors occur.

**Algorithm**:

1. Identify an MSC that supports error reporting by checking that MPAMF_IDR.HAS_ESR is set, indicating that the MPAMF_ESR register is implemented.
2. Trigger an Invalid PARTID error, such as accessing an undefined or misconfigured MPAM register, to force the hardware to write to MPAMF_ESR.
3. Immediately read the MPAMF_ESR register and record the current ERRCODE value. It should be Invalid PARTID Error.
4. Verify that the OVRWR bit is 0, as this is the first error occurrence and no previous error was overwritten.
5. Without clearing the ERRCODE field, trigger an Invalid PMG error.
6. Read MPAMF_ESR again and confirm that ERRCODE has updated to the new error code and OVRWR is now set to 1, indicating that the second error overwrote the first.
7. Clear the error status by writing 0 to both the ERRCODE and OVRWR fields in MPAMF_ESR.
8. Trigger a new MPAM error after clearing.
9. Read MPAMF_ESR once more and verify that ERRCODE reflects the new error and OVRWR remains 0, as there was no overwrite condition this time.
10. If all expected bit transitions and error logging behaviors occur as described, the test passes.
11. If ERRCODE fails to update or OVRWR does not reflect the overwrite state accurately, the test fails.

---

#### Test 218 Check MBWU_L overflow interrupt status

**Rules**: RYRQJ

**Source**: [`test_pool/mpam/intr004.c`](../../test_pool/mpam/intr004.c)

**Scenario**: Validate MBWU_L overflow interrupt status behavior.

**Algorithm**:

1. Identify MSCs that support MBWU long counters by confirming MPAMF_MBWUMON_IDR.HAS_LONG is set.
2. Select a valid MBWU monitor instance and configure it using MSMON_CFG_MON_SEL.MON_SEL and, if applicable, MSMON_CFG_MON_SEL.RIS.
3. Set MSMON_CFG_MBWU_CTL.OFLOW_INTR_L = 1 to enable interrupt generation on overflow.
4. Enable the monitor by setting MSMON_CFG_MBWU_CTL.EN = 1.
5. If the counter is writable, initialize the counter close to its maximum value to induce overflow quickly.
6. Generate memory traffic matching the monitor’s filter criteria to increment the MBWU_L counter and trigger overflow.
7. Monitor MSMON_CFG_MBWU_CTL.OFLOW_STATUS_L and confirm it is set to 1 once the counter overflows.
8. Confirm that an MPAM overflow interrupt is delivered upon overflow (via IRQ or MSI if configured).
9. Clear the overflow status by writing 0 to OFLOW_STATUS_L and disable the monitor.
10. If the overflow and corresponding interrupt are observed as expected, mark the test as passed; otherwise, mark it as failed.

---

#### Test 219 Check MBWU Oflow MSI interrupt status

**Rules**: YJCDR

**Source**: [`test_pool/mpam/intr005.c`](../../test_pool/mpam/intr005.c)

**Scenario**: Validate MBWU overflow MSI interrupt behavior.

**Algorithm**:

1. Program MPAM2_EL2 with DEFAULT_PARTID and default PMG; save the original value for restore.
2. For each MSC:
  - Skip if no MBWU monitors are present.
  - Read MPAMF_MSMON_IDR and skip if overflow MSI is not supported.
  - Fetch the MSC device ID and ITS ID for MSI routing.
  - Iterate memory resources; if RIS is supported, select the resource instance.
  - Validate the memory range is large enough for two buffers; allocate source and destination buffers.
  - Request OFLOW_MSI assignment and install the GIC ISR once; route the MSI to the current PE.
  - Enable OFLOW_MSI for the MSC.
  - Configure MBWU monitor:
    - Clear any existing overflow status.
    - Set OFLOW_INTR = 1, OFLOW_INTR_L = 0, and enable PARTID/PMG matching.
    - Pre-fill the MBWU counter near overflow and enable the monitor.
    - Generate memory traffic (memcpy) to trigger overflow.
  - Wait for the MSI handler to run and confirm OFLOW_STATUS is observed set in the handler.
  - The handler clears overflow status, disables OFLOW_MSI, disables the monitor, resets MPAM error status, and signals end-of-interrupt.
  - Fail if the MSI times out, the handler is not invoked, or OFLOW_STATUS was not observed set.
3. Cleanup per resource: free buffers, disable and reset MBWU monitor, clear overflow status, and disable OFLOW_MSI.
4. Restore MPAM2_EL2; pass if at least one MSI path succeeded and no failures occurred, otherwise fail or skip.

---


### Memory Bandwidth Tests

#### Test 301 Check MBWPBM Partitioning

**Rules**: CFHQL, DKYDB, GYMDB, YFMQP

**Source**: [`test_pool/mpam/mem001.c`](../../test_pool/mpam/mem001.c)

**Scenario**: Validate MBWPBM partitioning behaviour using MBWU monitor counts.

**Algorithm**:

1. Get PE index and total MSC count; identify memory resources supporting MBWPBM and track minmax PARTID.
2. Skip test if no MBWPBM-supported memory nodes exist. Disable CPOR and CCAP partitioning across all MSCs for minmax PARTID.
3. Update MPAM2_EL2 with minmax PARTID and default PMG and prepare a counter array to log MBWU activity.
4. For each test scenario (for example, 20% and 5% partition): loop through memory MSC nodes, if MBWPBM is supported:
  - Configure MBWPBM for the scenario and disable MBWMIN/MBWMAX if present. Allocate source and destination buffers based on available address range and required size.
  - Skip if SRAT info is missing or memory cannot be allocated. If MBWU monitor is not found, skip the test.
  - Configure and enable the MBWU monitor, then perform memory copy. Record start and end MBWU monitor counts to measure bandwidth usage.
  - Reset and disable MBWU monitor and free allocated memory. Restore original MPAM2_EL2 settings.
5. Compare recorded monitor counts across scenarios; if higher bandwidth usage is recorded in more restricted settings, mark test as failed. Otherwise, mark test as passed.

---

#### Test 302 Check MBWMIN Partitioning

**Rules**: WKZBJ, WRXBV

**Source**: [`test_pool/mpam/mem002.c`](../../test_pool/mpam/mem002.c)

**Scenario**: Validate MBWMIN partitioning behaviour using MBWU monitor counts under contention.

**Algorithm**:

1. Get PE index and total MSC count; identify memory nodes supporting MBWMIN and find the minimum valid PARTID. Skip the test if no MBWMIN-capable memory nodes exist.
2. Set up MPAM2_EL2 with default PMG and minmax PARTID, and configure all cache nodes to disable CPOR, CCAP, MBWPBM, and MBWMAX. Install exception handlers for sync and async faults.
3. For each MBWMIN-capable memory node: configure RIS, disable MBWPBM and MBWMAX.
4. Allocate shared buffers and determine buffer size. Launch contention by spawning memory-copy threads on secondary PEs.
5. Scenario 1: Configure MBWMIN with BW1 percentage and record MBWU monitor count. Wait for all secondary PEs to complete; if timeout, fail the test.
6. Scenario 2: Reconfigure MBWMIN with BW2 percentage and repeat measurement. Compare bandwidth usage across scenarios; fail if BW usage increases when a stricter limit is set.
7. Restore MPAM2_EL2 and clean up memory allocations. Mark test as passed if all conditions are met.

---

#### Test 303 Check MBWMAX Partitioning

**Rules**: NWVFK, VWHYW

**Source**: [`test_pool/mpam/mem003.c`](../../test_pool/mpam/mem003.c)

**Scenario**: Validate MBWMAX partitioning behaviour using MBWU monitor counts.

**Algorithm**:

1. Get PE index and total MSC count; identify memory resources supporting MBWMAX and track minmax PARTID.
2. Skip test if no MBWMAX-supported memory nodes exist. Disable CPOR and CCAP partitioning across all MSCs for minmax PARTID.
3. Update MPAM2_EL2 with minmax PARTID and default PMG and prepare a counter array to log MBWU activity.
4. For each test scenario (for example, 20% and 5% partition): loop through memory MSC nodes; if MBWMAX is supported:
  - Configure MBWMAX for the scenario and disable MBWMIN/MBWPBM if present.
  - Allocate source and destination buffers based on available memory and required size. Skip if SRAT info is invalid or buffer allocation fails.
  - If MBWU monitor is not found, skip the test. Configure and enable the MBWU monitor, then perform memory copy.
  - Record start and end MBWU monitor counts to measure bandwidth usage. Reset and disable MBWU monitor and free allocated memory. Restore original MPAM2_EL2 settings.
5. Compare recorded monitor counts across scenarios; if usage is higher at a more restrictive setting, mark test as failed. Otherwise, mark test as passed.

---


#### Test 305 Check MBWU OFLOW_FRZ overflow behavior

**Rules**: NXKSF, PVNHR, VBZTW, WHJQG, WQJKV, XSTRM, XZXVK

**Source**: [`test_pool/mpam/monitor007.c`](../../test_pool/mpam/monitor007.c)

**Scenario**: Validate MBWU overflow freeze behavior.

**Algorithm**:

1. Identify an MSC with monitoring support and verify the monitor type (MBWU) includes the OFLOW_FRZ in the MSMON_CFG_<type>_CTL register.
2. Select a monitor instance using MSMON_CFG_MON_SEL.MON_SEL, and configure MSMON_CFG_MON_SEL.RIS if applicable.
3. Set MSMON_CFG_<type>_CTL.OFLOW_INTR = 0 to avoid interrupt interference during testing.
4. Enable overflow freeze behavior by setting MSMON_CFG_<type>_CTL.OFLOW_FRZ = 1.
5. Enable the monitor (EN = 1) and, if the counter is writable, initialize it near its maximum value to easily induce overflow.
6. Generate traffic to trigger an overflow and read the counter value.
7. Confirm that after overflow, the counter value remains frozen (unchanged) despite ongoing traffic.
8. Reset OFLOW_FRZ = 0 and check OFLOW_STATUS must also be reset. Generate more traffic.
9. Verify that the counter resumes incrementing beyond the frozen value.
10. If the counter correctly freezes and resumes based on the OFLOW_FRZ bit setting, mark the test as passed.
11. If freezing or resumption fails to behave as expected, mark the test as failed.

This validates that OFLOW_FRZ properly controls whether the monitor counter halts or continues after an overflow.

---

#### Test 306 Check MBWU MSMON OFLOW Reset behavior

**Rules**: FWCFZ, GNQFY, NDMMK

**Source**: [`test_pool/mpam/monitor008.c`](../../test_pool/mpam/monitor008.c)

**Scenario**: Validate MBWU overflow reset behavior.

**Algorithm**:

1. Identify an MSC with monitoring support and ensure the monitor type (MBWU) implements the OFLOW_STATUS field in the MSMON_CFG_CTL register.
2. Select a monitor instance via MSMON_CFG_MON_SEL.MON_SEL and, if RIS is implemented, set the appropriate RIS using MSMON_CFG_MON_SEL.RIS.
3. Enable the monitor by setting EN = 1 in the control register.
4. If the monitor counter is writable, preset it to a value near its maximum, such as 0xFFFFFFF0, to easily cause an overflow.
5. Generate memory activity that will trigger the counter to overflow.
6. After overflow occurs, verify that OFLOW_STATUS is set to 1.
7. To clear the overflow state, write a new value directly to the monitor counter register.
8. Resume the memory traffic after resetting the overflow condition.
9. Monitor the counter to ensure it resumes counting from the cleared or new value.
10. If the counter begins counting again correctly after overflow reset, the test is passed.
11. If the counter does not resume or remains stuck despite clearing OFLOW_STATUS or updating the counter, the test fails.

---

## Appendix A. Revisions
This appendix describes the technical changes between released issues of this document.

v26.10_MPAM_b.c.0
- Updated the test scenarios for rule-based execution.
- Added all covered rule IDs and source links for each active test.
- Updated the scenario algorithms to match the current tests.

v26.03_MPAM_0.7.0
- Added BETA scenario algorithms.
- Moved from PDF format to Markdown format.
