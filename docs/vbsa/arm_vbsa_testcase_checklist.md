## VBSA ACS Testcase checklist

The VBSA ACS test checklist is based on **VBSA 1.0 specification** and **VBSA ACS 0.7.0** tag.

The checklist provides information about:

1. The VBSA level at which each test runs.
2. The VBSA rules covered by each test.
3. The VBSA rules that are not currently covered by ACS.
4. The runtime environment (UEFI, BareMetal, or Linux) in which each test executes.
5. The base vBSA rules reference the vBSA ACS scenario document.


[Latest checklist changes](#latest-checklist-changes) summarizing the latest checklist changes relative to the latest released tag, is present at end of document.

<table border="1">
  <thead>
    <tr>
      <th>Level</th>
      <th>Spec checklist rule</th>
      <th>Subrules</th>
      <th>Test number</th>
      <th>Test description</th>
      <th>UEFI</th>
      <th>Linux</th>
      <th>Additional Comments</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td rowspan="12">L1</td>
      <td rowspan="12"><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1pe_01--check-vpe-architectural-compliance">V_L1PE_01</td>
      <td></td>
      <td></td>
      <td>Check vPE architectural compliance</td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>B_PE_01</td>
      <td>1</td>
      <td>Check Arch symmetry across PE</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_PE_02</td>
      <td>2</td>
      <td>Check for number of PE</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_PE_03</td>
      <td>3</td>
      <td>Check for AdvSIMD and FP support</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_PE_04</td>
      <td>4</td>
      <td>Check PE 4KB Granule Support</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_PE_05</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>B_PE_06</td>
      <td>6</td>
      <td>Check Cryptographic extensions</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_PE_07</td>
      <td>7</td>
      <td>Check Little Endian support</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_PE_08</td>
      <td>8</td>
      <td>Check EL1 and EL0 implementation</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_PE_10</td>
      <td>10</td>
      <td>Check PMU Overflow signal</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_PE_13</td>
      <td>13</td>
      <td>Check CRC32 instruction support</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_PE_14</td>
      <td>16</td>
      <td>Check SVE2 for v9 PE</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td><a href="./arm_vbsa_architecture_compliance_test_scenario.md#v_l1pe_02--check-pmuv3-support-in-vpe">V_L1PE_02</td>
      <td></td>
      <td>67</td>
      <td>Check PMUv3 support in vPE</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td rowspan="6">L1</td>
      <td rowspan="6"><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1mm_01--check-ve-mem-architectural-compliance">V_L1MM_01</td>
      <td></td>
      <td></td>
      <td>Check VE Mem architectural compliance</td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>B_MEM_01</td>
      <td>102</td>
      <td>Mem Access Response in finite time</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_MEM_02</td>
      <td>101</td>
      <td>Memory Access to Un-Populated addr</td>
      <td>❌</td>
      <td>❌</td>
      <td>This test is run only on Baremetal environment in BSA ACS</td>
    </tr>
    <tr>
      <td>B_MEM_04</td>
      <td>106</td>
      <td>Check Addressability when SMMU's off</td>
      <td>❌</td>
      <td>✅</td>
      <td></td>
    </tr>
    <tr>
      <td>B_MEM_05</td>
      <td>103</td>
      <td>PE must access all NS addr space</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_MEM_07</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1mm_02--dma-requestors-access-to-ns-addr-space">V_L1MM_02</td>
      <td></td>
      <td>104</td>
      <td>DMA requestors access to NS addr space</td>
      <td>❌</td>
      <td>✅</td>
      <td></td>
    </tr>
    <tr>
      <td rowspan="5">L1</td>
      <td rowspan="5"><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1gi_01--check-vgic-architectural-compliance">V_L1GI_01</td>
      <td></td>
      <td></td>
      <td>Check vGIC architectural compliance</td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>B_GIC_01</td>
      <td>201</td>
      <td>Check GIC version</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_GIC_02</td>
      <td>202</td>
      <td>Check GICv2 Valid Configuration</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_GIC_03</td>
      <td>203</td>
      <td>If PCIe, GICv3 then ITS, LPI</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_GIC_05</td>
      <td>205</td>
      <td>Non-secure SGIs are implemented</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td rowspan="3">L1</td>
      <td rowspan="3"><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1pp_00--check-ve-ppi-assignment-mapping">V_L1PP_00</td>
      <td></td>
      <td></td>
      <td>Check VE PPI assignment mapping</td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>B_PPI_01</td>
      <td>206</td>
      <td>Check EL1-Phy timer PPI assignment</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td>207</td>
      <td>Check EL1-Virt timer PPI assignment</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td rowspan="6">L1</td>
      <td rowspan="6"><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1sm_01--check-vsmmu-architectural-compliance">V_L1SM_01</td>
      <td></td>
      <td></td>
      <td>Check vSMMU architectural compliance</td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>B_SMMU_01</td>
      <td>301</td>
      <td>All SMMUs have same Arch Revision</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_SMMU_02</td>
      <td>302</td>
      <td>Check SMMU Granule Support</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_SMMU_06</td>
      <td>303</td>
      <td>Check SMMU Large Physical Addr Support</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_SMMU_07</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>B_SMMU_12</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1sm_02--check-smmu-stage-1-support-for-ve">V_L1SM_02</td>
      <td></td>
      <td>331</td>
      <td>Check SMMU stage 1 support for VE</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1sm_03--boot-devices-not-blocked-by-smmu">V_L1SM_03</td>
      <td></td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1tm_01--check-virt-and-phy-counter-presence">V_L1TM_01</td>
      <td></td>
      <td>401</td>
      <td>Check Virt and Phy counter presence</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1tm_02--check-virt-and-phy-counter-min-freq">V_L1TM_02</td>
      <td></td>
      <td>402</td>
      <td>Check Virt and Phy counter min freq</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1tm_03--generic-timer-conformance">V_L1TM_03</td>
      <td></td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1tm_04--check-uniform-passage-of-time-in-ve">V_L1TM_04</td>
      <td></td>
      <td>408</td>
      <td>Check uniform passage of time in VE</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1wk_01--wakeup-semantics">V_L1WK_01</td>
      <td></td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td rowspan="3">L1</td>
      <td rowspan="3"><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1wk_02--check-power-wakeup-interrupts">V_L1WK_02</td>
      <td></td>
      <td></td>
      <td>Check power wakeup interrupts</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td>501</td>
      <td>Wake from EL1 PHY Timer Int</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td>502</td>
      <td>Wake from EL1 VIR Timer Int</td>
      <td>✅</td>
      <td>❌</td>
    </tr>
    <tr>
      <td>L1</td>
      <td><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1wk_03--wakeup-semantics">V_L1WK_03</td>
      <td></td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1wk_04--wakeup-semantics">V_L1WK_04</td>
      <td></td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td rowspan="3">L1</td>
      <td rowspan="3"><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1wk_05--check-vpe-power-state-semantics">V_L1WK_05</td>
      <td></td>
      <td></td>
      <td>Check vPE power state semantics</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td>501</td>
      <td>Wake from EL1 PHY Timer Int</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td>502</td>
      <td>Wake from EL1 VIR Timer Int</td>
      <td>✅</td>
      <td>❌</td>
    </tr>
    <tr>
      <td>L1</td>
      <td><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1wk_06--power-state-semantics">V_L1WK_06</td>
      <td></td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1wk_07--power-state-semantics">V_L1WK_07</td>
      <td></td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1wk_08--power-state-semantics">V_L1WK_08</td>
      <td></td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1wk_09--power-state-semantics">V_L1WK_09</td>
      <td></td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td rowspan="11">L1</td>
      <td rowspan="11"><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1pr_01--check-peripheral-arch-compliance">V_L1PR_01</td>
      <td></td>
      <td></td>
      <td>Check Peripheral arch compliance</td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>B_PER_01</td>
      <td>601</td>
      <td>USB CTRL Interface EHCI check</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_PER_02</td>
      <td>608</td>
      <td>USB CTRL Interface XHCI check</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_PER_03</td>
      <td>602</td>
      <td>Check SATA CTRL Interface</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_PER_04</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>B_PER_05</td>
      <td>603</td>
      <td>Check UART type Arm Generic or 16550</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_PER_06</td>
      <td>606</td>
      <td>Check Arm GENERIC UART Interrupt</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>B_PER_09</td>
      <td>604</td>
      <td>Check Memory Attributes of DMA</td>
      <td>❌</td>
      <td>✅</td>
      <td></td>
    </tr>
    <tr>
      <td>B_PER_10</td>
      <td>607</td>
      <td>Memory Attribute of I/O coherent DMA</td>
      <td>❌</td>
      <td>✅</td>
      <td></td>
    </tr>
    <tr>
      <td>B_PER_11</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>B_PER_12</td>
      <td>821</td>
      <td>Type 0 config header rules</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td rowspan="70">L1</td>
      <td rowspan="70"><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l1pr_02--check-virt-pcie-arch-compliance">V_L1PR_02</td>
      <td></td>
      <td></td>
      <td>Check Virt PCIe arch compliance</td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_01</td>
      <td>801</td>
      <td>Check ECAM Presence</td>
      <td>✅</td>
      <td>✅</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_02</td>
      <td>802</td>
      <td>Check ECAM Memory accessibility</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_03</td>
      <td>838</td>
      <td>Check all RP in HB is in same ECAM</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_04</td>
      <td>803</td>
      <td>All EP/Sw under RP in same ECAM Region</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_05</td>
      <td>820</td>
      <td>Type 0/1 common config rule</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td>822</td>
      <td>Check Type 1 config header rules</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td>824</td>
      <td>Device capabilities reg rule</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td>825</td>
      <td>Device Control register rule</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td>826</td>
      <td>Device cap 2 register rules</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td>1517</td>
      <td>Check BME functionality of RP</td>
      <td>✅</td>
      <td>❌</td>
      <td>This test requires Exerciser VIP on BSA ACS and will be skipped in virtual environments.</td>
    </tr>
    <tr>
      <td></td>
      <td>833</td>
      <td>Check Max payload size supported</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_06</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_07</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_08</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_09</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_10</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_11</td>
      <td>1510</td>
      <td>Check RP Sec Bus transaction are TYPE0</td>
      <td>✅</td>
      <td>❌</td>
      <td>This test requires Exerciser VIP on BSA ACS and will be skipped in virtual environments.</td>
    </tr>
    <tr>
      <td>PCI_IN_12</td>
      <td>837</td>
      <td>Check Config Txn for RP in HB</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_13</td>
      <td>804</td>
      <td>Check RootPort NP Memory Access</td>
      <td>❌</td>
      <td>❌</td>
      <td>This test is run only on Baremetal environment in BSA ACS</td>
    </tr>
    <tr>
      <td></td>
      <td>805</td>
      <td>Check RootPort P Memory Access</td>
      <td>❌</td>
      <td>❌</td>
      <td>This test is run only on Baremetal environment in BSA ACS</td>
    </tr>
    <tr>
      <td>PCI_IN_14</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_15</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_16</td>
      <td>808</td>
      <td>Check all 1's for out of range</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
    <tr>
      <td>PCI_IN_17</td>
      <td>836</td>
      <td>Check ARI forwarding enable rule</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
      <td></td>
      <td>1515</td>
      <td>Check ARI forwarding enable rule</td>
      <td>✅</td>
      <td>❌</td>
      <td>This test requires Exerciser VIP on BSA ACS and will be skipped in virtual environments.</td>
    </tr>
    <tr>
      <td>PCI_IN_18</td>
      <td>811</td>
      <td>Check RP Byte Enable Rules</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_19</td>
      <td>830</td>
      <td>Check Cmd Reg memory space enable</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td>831</td>
      <td>Check Type0/1 BIST Register rule</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td>832</td>
      <td>Check HDR CapPtr Register rule</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_20</td>
      <td>809</td>
      <td>Vendor specific data is PCIe compliant</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_MM_01</td>
      <td>845</td>
      <td>PCIe Device Memory mapping support</td>
      <td>❌</td>
      <td>✅</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td>1516</td>
      <td>PCIe Device Memory access check</td>
      <td>✅</td>
      <td>❌</td>
      <td>This test requires Exerciser VIP on BSA ACS and will be skipped in virtual environments.</td>
    </tr>
    <tr>
      <td>PCI_MM_03</td>
      <td>894</td>
      <td>PCIe Normal Memory mapping support</td>
      <td>❌</td>
      <td>✅</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td>1539</td>
      <td>PCIe Normal Memory access check</td>
      <td>✅</td>
      <td>❌</td>
      <td>This test requires Exerciser VIP on BSA ACS and will be skipped in virtual environments.</td>
    </tr>
    <tr>
      <td>PCI_MM_04</td>
      <td>847</td>
      <td>NP type-1 pcie only support 32-bit</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_MM_05</td>
      <td>895</td>
      <td>PCIe &amp; PE common physical memory view</td>
      <td>❌</td>
      <td>✅</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_MM_06</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_MM_07</td>
      <td>905</td>
      <td>No extra address translation</td>
      <td>❌</td>
      <td>✅</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_MSI_1</td>
      <td>839</td>
      <td>Check MSI support for PCIe dev</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_MSI_2</td>
      <td>897</td>
      <td>Check MSI=X vectors uniqueness</td>
      <td>❌</td>
      <td>✅</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td>1533</td>
      <td>MSI(-X) triggers intr with unique ID</td>
      <td>✅</td>
      <td>❌</td>
      <td>This test requires Exerciser VIP on BSA ACS and will be skipped in virtual environments.</td>
    </tr>
    <tr>
      <td>PCI_LI_01</td>
      <td>806</td>
      <td>Check Legacy Interrupt is SPI</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_LI_02</td>
      <td>896</td>
      <td>PCI legacy intr SPI ID unique</td>
      <td>❌</td>
      <td>✅</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td>1506</td>
      <td>Generate PCIe legacy interrupt</td>
      <td>✅</td>
      <td>❌</td>
      <td>This test requires Exerciser VIP on BSA ACS and will be skipped in virtual environments.</td>
    </tr>
    <tr>
      <td>PCI_LI_03</td>
      <td>823</td>
      <td>Check Legacy Intr SPI level sensitive</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_LI_04</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_SM_01</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_SM_02</td>
      <td>835</td>
      <td>Check Function level reset</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_11</td>
      <td>868</td>
      <td>PCIe RC,PE - Same Inr Shareable Domain</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_12</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_13</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_14</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_15</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_16</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_17</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_18</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IO_01</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IEP_1</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_PP_01</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_PP_02</td>
      <td>1514</td>
      <td>P2P transactions must not deadlock</td>
      <td>✅</td>
      <td>❌</td>
      <td>This test requires Exerciser VIP on BSA ACS and will be skipped in virtual environments.</td>
    </tr>
    <tr>
      <td>PCI_PP_03</td>
      <td>819</td>
      <td>RP must suprt ACS if P2P Txn are allow</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_PP_04</td>
      <td>818</td>
      <td>Check RP Adv Error Report</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td>1501</td>
      <td>Check P2P ACS Functionality</td>
      <td>✅</td>
      <td>❌</td>
      <td>This test requires Exerciser VIP on BSA ACS and will be skipped in virtual environments.</td>
    </tr>
    <tr>
      <td></td>
      <td>1502</td>
      <td>Check ACS Redirect Req Valid</td>
      <td>✅</td>
      <td>❌</td>
      <td>This test requires Exerciser VIP on BSA ACS and will be skipped in virtual environments.</td>
    </tr>
    <tr>
      <td>PCI_PP_05</td>
      <td>817</td>
      <td>Check Direct Transl P2P Support</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_PP_06</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_PAS_1</td>
      <td>842</td>
      <td>PASID support atleast 16 bits</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_PTM_1</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L2</td>
      <td><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l2pe_01--check-num-pmu-counters-for-vpes">V_L2PE_01</td>
      <td></td>
      <td>68</td>
      <td>Check num PMU counters for vPEs</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>L2</td>
      <td><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l2pe_02--check-vpe-pmu-overflow-signal">V_L2PE_02</td>
      <td></td>
      <td>10</td>
      <td>Check vPE PMU overflow signal</td>
      <td>✅</td>
      <td>❌</td>
      <td></td>
    </tr>
    <tr>
      <td>L2</td>
      <td><a href="arm_vbsa_architecture_compliance_test_scenario.md#v_l2wd_01--watchdog">V_L2WD_01</td>
      <td></td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
  </tbody>
</table>

## Latest Checklist Changes
#### v25.12_VBSA_0.7.0 - *Dec, 2025*
- Initial VBSA ACS testcase checklist check-in.