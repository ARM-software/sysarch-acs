# Arm VBSA Architecture Compliance Test Scenarios

**Specification**: [Arm VBSA 1.0 (DEN0150)](https://developer.arm.com/documentation/den0150/latest/)<br>
**Companion Docs**: [Arm BSA 1.2 (DEN0094)](https://developer.arm.com/documentation/den0094/latest/)<br>
**BSA ACS Scenarios**: [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf)<br>

---

## Table of contents
- [1. About this document](#1-about-this-document)
- [2. V-BSA Level 1 Rules](#2-v-bsa-level-1-rules)
  - [V_L1PE_01 — Check vPE architectural compliance](#v_l1pe_01--check-vpe-architectural-compliance)
  - [V_L1PE_02 — Check PMUv3 support in vPE](#v_l1pe_02--check-pmuv3-support-in-vpe)
  - [V_L1MM_01 — Check VE Mem architectural compliance](#v_l1mm_01--check-ve-mem-architectural-compliance)
  - [V_L1MM_02 — DMA requestors access to NS addr space](#v_l1mm_02--dma-requestors-access-to-ns-addr-space)
  - [V_L1GI_01 — Check vGIC architectural compliance](#v_l1gi_01--check-vgic-architectural-compliance)
  - [V_L1PP_00 — Check VE PPI assignment mapping](#v_l1pp_00--check-ve-ppi-assignment-mapping)
  - [V_L1SM_01 — Check vSMMU architectural compliance](#v_l1sm_01--check-vsmmu-architectural-compliance)
  - [V_L1SM_02 — Check SMMU stage 1 support for VE](#v_l1sm_02--check-smmu-stage-1-support-for-ve)
  - [V_L1SM_03 — Boot devices not blocked by SMMU](#v_l1sm_03--boot-devices-not-blocked-by-smmu)
  - [V_L1TM_01 — Check Virt and Phy counter presence](#v_l1tm_01--check-virt-and-phy-counter-presence)
  - [V_L1TM_02 — Check Virt and Phy counter min freq](#v_l1tm_02--check-virt-and-phy-counter-min-freq)
  - [V_L1TM_03 — Generic timer conformance](#v_l1tm_03--generic-timer-conformance)
  - [V_L1TM_04 — Check uniform passage of time in VE](#v_l1tm_04--check-uniform-passage-of-time-in-ve)
  - [V_L1WK_01 — Wakeup semantics](#v_l1wk_01--wakeup-semantics)
  - [V_L1WK_02 — Check power wakeup interrupts](#v_l1wk_02--check-power-wakeup-interrupts)
  - [V_L1WK_03 — Wakeup semantics](#v_l1wk_03--wakeup-semantics)
  - [V_L1WK_04 — Wakeup semantics](#v_l1wk_04--wakeup-semantics)
  - [V_L1WK_05 — Check vPE power state semantics](#v_l1wk_05--check-vpe-power-state-semantics)
  - [V_L1WK_06 — Power state semantics](#v_l1wk_06--power-state-semantics)
  - [V_L1WK_07 — Power state semantics](#v_l1wk_07--power-state-semantics)
  - [V_L1WK_08 — Power state semantics](#v_l1wk_08--power-state-semantics)
  - [V_L1WK_09 — Power state semantics](#v_l1wk_09--power-state-semantics)
  - [V_L1PR_01 — Check Peripheral arch compliance](#v_l1pr_01--check-peripheral-arch-compliance)
  - [V_L1PR_02 — Check Virt PCIe arch compliance](#v_l1pr_02--check-virt-pcie-arch-compliance)
- [3. Future Level Rules](#3-future-level-rules)
  - [V_L2PE_01 — Check num PMU counters for vPEs](#v_l2pe_01--check-num-pmu-counters-for-vpes)
  - [V_L2PE_02 — Check vPE PMU overflow signal](#v_l2pe_02--check-vpe-pmu-overflow-signal)
  - [V_L2WD_01 — Watchdog](#v_l2wd_01--watchdog)

---

## 1. About this document
Purpose: capture ACS scenario outlines for each VBSA rule. Each section:
- states assumptions & scope (guest/host/firmware),
- enumerates observable behaviors and pass/fail conditions, and
- links to the algorithm in the BSA/SBSA scenario docs where applicable.

> **Note**: Where a VBSA rule references BSA/SBSA rules, this document does not duplicate the scenario algorithm. Instead, it references the appropriate BSA/SBSA scenario document under the sub-headers.

---

## 2. V-BSA Level 1 Rules

### V_L1PE_01 — Check vPE architectural compliance
**Summary**: For PE rules that VBSA references directly from BSA without modification, the existing BSA ACS tests apply as-is.

#### Referenced BSA Rules
- #### B_PE_01 — see section 2.2 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page9&search=B_PE_01)
- #### B_PE_02 — see section 2.2 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page9&search=B_PE_02)
- #### B_PE_03 — see section 2.2 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page9&search=B_PE_03)
- #### B_PE_04 — see section 2.2 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page9&search=B_PE_04)
- #### B_PE_05 — Not implemented
- #### B_PE_06 — see section 2.2 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page9&search=B_PE_06)
- #### B_PE_07 — see section 2.2 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page9&search=B_PE_07)
- #### B_PE_08 — see section 2.2 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page9&search=B_PE_08)
- #### B_PE_10 — see section 2.2 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page9&search=B_PE_10)
- #### B_PE_13 — see section 2.2 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page10&search=B_PE_13)
- #### B_PE_14 — see section 2.2 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page10&search=B_PE_14)

---

### V_L1PE_02 — Check PMUv3 support in vPE
**Summary**: A vPE that implements FEAT_PMUv3 (B_PE_09) and supports AArch64 at EL1 and EL0 (B_PE_08) shall implement PMCCNTR_EL0.

#### Related BSA Rule
- #### B_PE_08 — see section 2.2 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page9&search=B_PE_08)
- #### B_PE_09 — see section 2.2 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page9&search=B_PE_09)

---

### V_L1MM_01 — Check VE Mem architectural compliance
**Summary**: For Memory Map rules that VBSA references directly from BSA without modification, the existing BSA ACS tests apply as-is.

#### Referenced BSA Rules
- #### B_MEM_01 — see section 2.3 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page12&search=B_MEM_01)
- #### B_MEM_02 — see section 2.3 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page12&search=B_MEM_02)
- #### B_MEM_04 — see section 2.3 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page12&search=B_MEM_04)
- #### B_MEM_05 — see section 2.3 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page12&search=B_MEM_04)
- #### B_MEM_07 — Not implemented

---

### V_L1MM_02 — DMA requestors access to NS addr space
**Summary**: Run the same payload that validates B_MEM_03

#### Related BSA Rules
- #### B_MEM_03 — see section 2.3 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page12&search=B_MEM_03)

---

### V_L1GI_01 — Check vGIC architectural compliance
**Summary**: For GIC rules that VBSA references directly from BSA without modification, the existing BSA ACS tests apply as-is.

#### Referenced BSA Rules
- #### B_GIC_01 — see section 2.4 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page13&search=B_GIC_01)
- #### B_GIC_02 — see section 2.4 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page13&search=B_GIC_02)
- #### B_GIC_03 — see section 2.4 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page13&search=B_GIC_03)
- #### B_GIC_05 — see section 2.4 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page14&search=B_GIC_05)

---

<a id="v_l1pp_00--check-ve-ppi-assignment-mapping"></a>
### V_L1PP_00 — Check VE PPI assignment mapping
**Summary**: This rule references B_PPI_01 in the BSA. Run those PPI assignment checks that are ACS-testable.

#### B_PPI_01 mentiones the following PPI assignments
- **Overflow interrupt — CNTP**
    1. Query EL1 physical timer INTID.
    2. Install `isr_phy`.
    3. Program/arm the physical timer to overflow.
    4. Busy-wait until the ISR flips the test status.
    5. Aggregate pass/fail.

- **Overflow interrupt — CNTV**
    1. Query EL1 virtual timer INTID.
    2. Install `isr_vir`.
    3. Program/arm the virtual timer to overflow.
    4. Loop on pending status until the ISR signals completion.
    5. Aggregate pass/fail.

- **Performance Monitors Interrupt**
  - Covered under B_PE_10; no separate scenario here.

- **CTIIRQ / COMMIRQ / PMBIRQ**
  - Not testable in this ACS environment. Requires cross-trigger, debug communication channels, and access to secure-monitor which are not exercised by ACS.

---

### V_L1SM_01 — Check vSMMU architectural compliance
**Summary**: For SMMU rules that VBSA references directly from BSA without modification, the existing BSA ACS tests apply as-is.

#### Referenced BSA Rules
- #### B_SMMU_01 — see section 2.5 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page15&search=B_SMMU_01)
- #### B_SMMU_02 — see section 2.5 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page15&search=B_SMMU_02)
- #### B_SMMU_06 — see section 2.5 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page15&search=B_SMMU_06)
- #### B_SMMU_07 — test not implemented
- #### B_SMMU_12 — test not implemented

---

### V_L1SM_02 — Check SMMU stage 1 support for VE
**Summary**: Invoke the test that verified B_SMMU_08, ensuring that S-EL2 check is disabled.

#### Related BSA Rule
- #### B_SMMU_08 — see section 2.5 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page16&search=B_SMMU_08)

---

### V_L1SM_03 — Boot devices not blocked by SMMU
**Summary**: Test to cover this rule is not implemented and requires manual verification by the VE owner

---

<a id="v_l1tm_01--check-virt-and-phy-counter-presence"></a>
### V_L1TM_01 — Check Virt and Phy counter presence
**Rationale**: Each PE’s Generic Timer implementation includes a physical counter, a virtual counter, and additional timers. Confirming that the Generic Timer extension is implemented is sufficient to demonstrate compliance with this rule.

**Scenario**: Execute the same test used to validate B_TIME_01, which verifies the presence of the Generic Timer.
- see section 2.6 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page17&search=B_TIME_01)

---


<a id="v_l1tm_02--check-virt-and-phy-counter-min-freq"></a>
### V_L1TM_02 — Check Virt and Phy counter min freq
**Rationale**: Virtual Counter (CNTVCT_EL0) = Physical counter (CNTPCT_EL0) - Virtual offset (CNTVOFF_EL2). As the offset is a constant provided by the hypervisor, the virtual counter advances at the same effective frequency as the physical counter.

**Scenario**: Execute the same test used to validate B_TIME_02, which verifies physical counter runs at atleast 10MHz.
- see section 2.6 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page18&search=B_TIME_02)

---


### V_L1TM_03 — Generic timer conformance
**Summary**: This rule defers to B_TIME_03 and B_TIME_04 and ACS cannot exercise these. Manual verification by the VE owner is required.

**Rationale for not implementing**: see section 2.6 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page18&search=B_TIME_03)

---

<a id="v_l1tm_04--check-uniform-passage-of-time-in-ve"></a>
### V_L1TM_04 — Check uniform passage of time in VE

**Scenario**:
- Detect whether FEAT_ECV is present by reading ID_AA64MMFR0_EL1.ECV and branch accordingly; the run covers 2,000,000 samples to stress the counter monotonicity
- If physical counter accesses are permitted (g_el1physkip unset), repeatedly read CNTPCTSS_EL0 when FEAT_ECV is implemented, or CNTPCT_EL0 with an ISB barrier otherwise, and fail immediately if any new value is less than the previous sample.
- Always reset the iteration counter and validate the matching virtual counter (CNTVCTSS_EL0 with FEAT_ECV, else CNTVCT_EL0 plus ISB), again flagging any regression.
- If any decrement is observed the payload records a failure; otherwise the rule passes once all iterations complete.

---

### V_L1WK_01 — Wakeup semantics
**Summary**: Test to cover this rule is not implemented and requires manual verification by the VE owner

---

<a id="v_l1wk_02--check-power-wakeup-interrupts"></a>
### V_L1WK_02 — Check power wakeup interrupts
**Summary**: This rule is derived from B_WAK_03 in BSA. Execute the applicable tests that provide coverage for B_WAK_03.

**Test Coverage Note**: Tests U001 and U002 are run as part of VBSA ACS

**Related BSA Rule**
-  B_WAK_03 — see section 2.7 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page19&search=B_WAKE_03)

---

### V_L1WK_03 — Wakeup semantics
**Summary**: Test to cover this rule is not implemented and requires manual verification by the VE owner

---

### V_L1WK_04 — Wakeup semantics
**Summary**: Test to cover this rule is not implemented and requires manual verification by the VE owner

---

<a id="v_l1wk_05--check-vpe-power-state-semantics"></a>
### V_L1WK_05 — Check vPE power state semantics
**Summary**: This rule is derived from B_WAK_07 in BSA. Execute the applicable tests that provide coverage for B_WAK_07.

**Test Coverage Note**: Tests U001 and U002 are run as part of VBSA ACS

**Related BSA Rule**
-  B_WAK_07 — see section 2.8 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page20&search=B_WAKE_07)

---
### V_L1WK_06 — Power state semantics
**Summary**: Test to cover this rule is not implemented and requires manual verification by the VE owner

---

### V_L1WK_07 — Power state semantics
**Summary**: Test to cover this rule cannot be implemented by ACS. Requires manual verification by the VE owner

**Rationale for not implementing**:
- The rule requires the GIC to be powered down. According to VBSA Table 3, the valid system states in which the GIC is powered off are E and G.
- State G corresponds to the entire system being powered down and is therefore not practical.
- In states E, the PE can only be woken by system events, not interrupts, since the GIC is powered off. However, ACS does not have control over generating system events, making such verification infeasible.

---

### V_L1WK_08 — Power state semantics
**Summary**: Test to cover this rule is not implemented and requires manual verification by the VE owner

---

### V_L1WK_09 — Power state semantics
**Summary**: Test to cover this rule is not implemented and requires manual verification by the VE owner

---

<a id="v_l1pr_01--check-peripheral-arch-compliance"></a>
### V_L1PR_01 — Check Peripheral arch compliance
**Summary**:  This rule references multiple peripheral requirements in BSA. Run all tests that cover these rules as-is, without any modifications.

#### Referenced BSA Rules
- #### B_PER_01 — see section 2.10 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page22&search=B_PER_01)
- #### B_PER_02 — see section 2.10 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page22&search=B_PER_02)
- #### B_PER_03 — see section 2.10 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page22&search=B_PER_03)
- #### B_PER_04 — Test not implemented
- #### B_PER_05 — see section 2.10 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page23&search=B_PER_05)
- #### B_PER_06 — see section 2.10 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page23&search=B_PER_06)
- #### B_PER_09 — see section 2.10 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page24&search=B_PER_09)
- #### B_PER_10 — see section 2.10 in [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page24&search=B_PER_10)
- #### B_PER_11 — Test not implemented
- #### B_PER_12 — Covered by PCIe tests as part of V_L1PR_02

---

<a id="v_l1pr_02--check-virt-pcie-arch-compliance"></a>
### V_L1PR_02 — Check Virt PCIe arch compliance
**Summary**: This rule references Section E in BSA (except PCI_MM_02). Run all tests that cover these rules as-is, without any modifications.

**Related BSA Rule**
-   E.1 - Configuration Space
    - PCI_IN_01
    - PCI_IN_02
    - PCI_IN_03
    - PCI_IN_04
    - PCI_IN_05
    - PCI_IN_06
    - PCI_IN_07
    - PCI_IN_08
    - PCI_IN_09
    - PCI_IN_10
    - PCI_IN_11
    - PCI_IN_12
    - PCI_IN_13
    - PCI_IN_14
    - PCI_IN_15
    - PCI_IN_16
    - PCI_IN_17
    - PCI_IN_18
    - PCI_IN_19
    - PCI_IN_20

- E.2 - PCI Express memory space (Except PCI_MM_02)
    - PCI_MM_01
    - PCI_MM_03
    - PCI_MM_04

- E.3 - PCI Express deivce view of memory

    - PCI_MM_05
    - PCI_MM_06
    - PCI_MM_07

- E.4 - Message Signaled Interrupts

    - PCI_MSI_1
    - PCI_MSI_2

- E.6 - Legacy Interrupts

    - PCI_LI_01
    - PCI_LI_02
    - PCI_LI_03
    - PCI_LI_04

- E.7 - System MMU and Device Assignment

    - PCI_SM_01
    - PCI_SM_02

- E.8 - I/O Coherency
    - PCI_IC_11
    - PCI_IC_12
    - PCI_IC_13
    - PCI_IC_14
    - PCI_IC_15
    - PCI_IC_16
    - PCI_IC_17
    - PCI_IC_18

- E.9 - Legacy I/O

    - PCI_IO_01

- E.10 - Integrated End Points

    - PCI_IEP_1

- E.11 - Peer-to-Peer

    - PCI_PP_01
    - PCI_PP_02
    - PCI_PP_03
    - PCI_PP_04
    - PCI_PP_05
    - PCI_PP_06

- E.12 - PASID Support

    - PCI_PAS_1

- E.13 - PCIe Precision Time Measurement

    - PCI_PTM_1

For all rules with implemented tests, refer to Section 2.11 of the [BSA ACS Scenario Document](../bsa/arm_bsa_architecture_compliance_test_scenario.pdf#page25)

>Note: Several PCIe rules require the Exerciser to achieve complete coverage. Exerciser-based tests cannot be executed in virtual environments. These rules are reported as PASSED*(PARTIAL) or SKIPPED depending on other tests that cover the rule.

---

## 3. Future Level Rules

<a id="v_l2pe_01--check-num-pmu-counters-for-vpes"></a>
### V_L2PE_01 — Check num PMU counters for vPEs
**Scenario**: If ID_AA64DFR0_EL1.PMUVer is supported, then PMCR_EL0.N should be greater than or equal to 2.

---

<a id="v_l2pe_02--check-vpe-pmu-overflow-signal"></a>
### V_L2PE_02 — Check vPE PMU overflow signal
**Scenario**: If ID_AA64DFR0_EL1.PMUVer is supported, install ISR and verify PMU overflow interrupt by programming System register.

---

### V_L2WD_01 — Watchdog
**Summary**: Test to cover this rule cannot be implemented by ACS. Requires manual verification by the VE owner

**Rationale for not implementing**: There’s no standardized interface to identify an IMPDEF watchdog

---
