## PC-BSA ACS Testcase checklist

The PC-BSA ACS test checklist is based on **PC BSA v2.0 specification** and **PC BSA ACS v2.0.0** tag.

The checklist provides information about:

1. The PC BSA level at which each test runs.
2. The PC BSA rules covered by each test.
3. The PC BSA rules that are not currently covered by ACS.
4. The runtime environment (UEFI, BareMetal, or Linux) in which each test executes.
5. In a UEFI or Linux based test is **not** part of the SystemReady images, it is highlighted with `#`.
6. Tests which are dependent on platform information which cannot be auto-determined and needs manual effort to provide information in PAL API's are marked with `&`
7. The Exerciser PAL APIs used by each exerciser-backed test.

> **Note:** To identify an exerciser, `pal_is_bdf_exerciser()` and `pal_exerciser_get_state()` must be implemented for all exerciser-backed tests. These common APIs are omitted from the individual API lists.

[Latest checklist changes](#latest-checklist-changes) summarizing the latest checklist changes relative to the latest released tag, is present at end of document.

<br>
<table border="1" class="dataframe dataframe">
  <thead>
    <tr style="text-align: right;">
      <th>Level</th>
      <th>Specification Checklist Rule</th>
      <th>Sub Rule</th>
      <th>Test ID</th>
      <th>Test Description</th>
      <th>UEFI</th>
      <th>BM</th>
      <th>Linux</th>
      <th>Additional Information</th>
      <th>Exerciser PAL APIs Used</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>L1</td>
      <td>P_L1_01</td>
      <td>P_L1_01</td>
      <td>-</td>
      <td>BSA Level 1 requirements</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1PE_02</td>
      <td>P_L1PE_02</td>
      <td>24</td>
      <td>Check for 16-bit ASID support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1PE_03</td>
      <td>P_L1PE_03</td>
      <td>25</td>
      <td>Check AARCH64 implementation</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1PE_04</td>
      <td>P_L1PE_04</td>
      <td>15</td>
      <td>Check for FEAT_LSE support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1PE_05</td>
      <td>P_L1PE_05</td>
      <td>26</td>
      <td>Check FEAT_LPA Requirements</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1PE_06</td>
      <td>P_L1PE_06</td>
      <td>28</td>
      <td>Check DC CVAP support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1PE_07</td>
      <td>P_L1PE_07</td>
      <td>29</td>
      <td>Check for 16-Bit VMID</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1PE_08</td>
      <td>P_L1PE_08</td>
      <td>30</td>
      <td>Check for Virtual host extensions</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td rowspan="5">L1</td>
      <td rowspan="5">GFRQR</td>
      <td>B_SEC_01</td>
      <td>43</td>
      <td>Check Speculation Restriction</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>B_SEC_02</td>
      <td>44</td>
      <td>Check Speculative Str Bypass Safe</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>B_SEC_03</td>
      <td>45</td>
      <td>Check PEs Impl CSDB,SSBB,PSSBB</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>B_SEC_04</td>
      <td>46</td>
      <td>Check PEs Implement SB Barrier</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>B_SEC_05</td>
      <td>47</td>
      <td>Check PE Impl CFP,DVP,CPP RCTX</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>MHCBW</td>
      <td>MHCBW</td>
      <td>32</td>
      <td>Check for addr authentication support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L2</td>
      <td>YKRHG</td>
      <td>YKRHG</td>
      <td>36</td>
      <td>Check for nested virtualization</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L2</td>
      <td>CNBRV</td>
      <td>CNBRV</td>
      <td>48</td>
      <td>Check Fine Grain Trap Support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L2</td>
      <td>PBCRQ</td>
      <td>PBCRQ</td>
      <td>49</td>
      <td>Check for ECV support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L2</td>
      <td>NWCYZ</td>
      <td>NWCYZ</td>
      <td>61</td>
      <td>Check for FEAT_BRBE support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L2</td>
      <td>HQVRJ</td>
      <td>HQVRJ</td>
      <td>31</td>
      <td>Support Page table map size change</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L2</td>
      <td>FNXTB</td>
      <td>FNXTB</td>
      <td>39</td>
      <td>Check Protect Against Timing Fault</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L2</td>
      <td>XRRTP</td>
      <td>XRRTP</td>
      <td>62</td>
      <td>Check for unsupported PBHA bits</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td rowspan="2">L1</td>
      <td rowspan="2">P_L1MM_01</td>
      <td>S_L3MM_01</td>
      <td>105</td>
      <td>NS-EL2 Stage-2 64KB Mapping Check</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>S_L3MM_02</td>
      <td>108</td>
      <td>Check peripherals addr 64Kb apart</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1GI_01</td>
      <td>P_L1GI_01</td>
      <td>212</td>
      <td>Check GIC version</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1GI_02</td>
      <td>P_L1GI_02</td>
      <td>846</td>
      <td>Check all MSI(X) vectors are LPIs</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1GI_03</td>
      <td>P_L1GI_03</td>
      <td>210</td>
      <td>Check GIC supports disabling LPIs</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1GI_04</td>
      <td>P_L1GI_04</td>
      <td>211</td>
      <td>Check GICR_PENDBASER when LPIs enabled</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L2</td>
      <td>TTLFJ</td>
      <td>TTLFJ</td>
      <td>215</td>
      <td>Check GICv4.1 or higher compliant</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L2</td>
      <td>PGKDP</td>
      <td>PGKDP</td>
      <td>216</td>
      <td>Check Non standard GICv3 implementation</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1PP_01</td>
      <td>P_L1PP_01</td>
      <td>214</td>
      <td>Check All PPI Interrupt IDs</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1SM_01</td>
      <td>P_L1SM_01</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1SM_02</td>
      <td>P_L1SM_02</td>
      <td>308</td>
      <td>Check Stage 1 SMMUv3 functionality</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1SM_03</td>
      <td>P_L1SM_03</td>
      <td>325</td>
      <td>Check Stage 2 SMMUv3 functionality</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td rowspan="3">L1</td>
      <td rowspan="3">P_L1SM_04</td>
      <td>SMMU_01</td>
      <td>307</td>
      <td>SMMUv3 Integration compliance</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>SMMU_02</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td>ACS detects advertised stall support through IDR0.STALL_MODEL only; rule compliance requires integration/DV evidence.</td>
      <td></td>
    </tr>
    <tr>
      <td>SMMU_03</td>
      <td>333</td>
      <td>Check SMMU 128-bit atomicity support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1SM_05</td>
      <td>P_L1SM_05</td>
      <td>322</td>
      <td>Check if all DMA reqs behind SMMU</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1PCI_1</td>
      <td>P_L1PCI_1</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1PCI_2</td>
      <td>P_L1PCI_2</td>
      <td>887</td>
      <td>Check EA Capability</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td rowspan="73">L2</td>
      <td rowspan="73">RVFLY - B_REP_1</td>
      <td>RI_CRS_1</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RI_BAR_1</td>
      <td>883</td>
      <td>Read and write to BAR reg - RCiEP, iEP Pair</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP, iEP RP, iEP EP</td>
      <td></td>
    </tr>
    <tr>
      <td>RI_BAR_2</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RI_BAR_3</td>
      <td>862</td>
      <td>Check BAR memory space &amp; type - RCiEP, iEP Pair</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP, iEP RP, iEP EP</td>
      <td></td>
    </tr>
    <tr>
      <td>RI_INT_1</td>
      <td>869</td>
      <td>Check MSI and MSI-X support - RCiEP, iEP Pair</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP, iEP; MSI/MSI-X support required</td>
      <td></td>
    </tr>
    <tr>
      <td>RI_ORD_1</td>
      <td>1521</td>
      <td>Arrival order &amp; Gathering Check - RCiEP, iEP EP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
      <td>pal_exerciser_get_param (TRANSACTION_TYPE, CLEAR_TXN)<br>pal_exerciser_ops (START_TXN_MONITOR, STOP_TXN_MONITOR)<br>pal_exerciser_get_data (EXERCISER_DATA_MMIO_SPACE)</td>
    </tr>
    <tr>
      <td>RI_ORD_2</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RI_ORD_3</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td rowspan="3">RI_SMU_1</td>
      <td>880</td>
      <td>Check ATS Support Rule: RCiEP/iEP/RP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>1519</td>
      <td>PCIe Address translation check</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required; ATC cache required</td>
      <td>pal_exerciser_set_param (DMA_ATTRIBUTES)<br>pal_exerciser_ops (START_DMA)</td>
    </tr>
    <tr>
      <td>1520</td>
      <td>ATS Functionality Check</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required; ATC cache required</td>
      <td>pal_exerciser_get_param (ATS_RES_ATTRIBUTES)<br>pal_exerciser_set_param (DMA_ATTRIBUTES, CFG_TXN_ATTRIBUTES)<br>pal_exerciser_ops (ATS_TXN_REQ, START_DMA, ATS_TXN_CLEAR)</td>
    </tr>
    <tr>
      <td>RI_SMU_2</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RI_PWR_1</td>
      <td>870</td>
      <td>Check Power Mgmt rules - RCiEP, iEP Pair</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP, iEP RP, iEP EP</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_01</td>
      <td>801</td>
      <td>Check ECAM Presence</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP, RCEC</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_02</td>
      <td>802</td>
      <td>Check ECAM Memory accessibility</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP, RCEC</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_03</td>
      <td>838</td>
      <td>Check all RP in HB is in same ECAM</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP RP</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_07</td>
      <td>828</td>
      <td>Check all 1's for out of range</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_14</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
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
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_16</td>
      <td>808</td>
      <td>Check all 1's for out of range</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP RP</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_20</td>
      <td>809</td>
      <td>Vendor specific data is PCIe compliant</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP RP</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_MM_01</td>
      <td></td>
      <td>PCIe Device Memory mapping support</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>RCiEP, RCEC, iEP RP, iEP EP</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_MM_02</td>
      <td>907</td>
      <td>PCIe Non-Cacheable Memory mapping support</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>RCiEP, RCEC, iEP RP, iEP EP</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_MM_03</td>
      <td></td>
      <td>PCIe Normal Memory mapping support</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>RCiEP, RCEC, iEP RP, iEP EP</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_MM_04</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_MM_05</td>
      <td>895</td>
      <td>PCIe devices and PEs common physical address view</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Exerciser VIP required</td>
      <td>pal_exerciser_set_param (DMA_ATTRIBUTES)<br>pal_exerciser_ops (START_DMA)</td>
    </tr>
    <tr>
      <td>PCI_MM_06</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_MM_07</td>
      <td>905</td>
      <td>No extra address translation</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Exerciser VIP required</td>
      <td>pal_exerciser_set_param (DMA_ATTRIBUTES)<br>pal_exerciser_ops (START_DMA)</td>
    </tr>
    <tr>
      <td>PCI_MSI_1</td>
      <td>839</td>
      <td>Check MSI support for PCIe dev</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>MSI/MSI-X support required</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_MSI_2</td>
      <td></td>
      <td>Check MSI(X) vectors uniqueness</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>MSI/MSI-X support required</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_LI_01</td>
      <td></td>
      <td>Check Legacy Interrupt is SPI</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP, RCEC</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_LI_03</td>
      <td></td>
      <td>Check Legacy Intr SPI level sensitive</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP, RCEC</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_LI_04</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_SM_01</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_SM_02</td>
      <td>835</td>
      <td>Check Function level reset</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RTDTC / ITS_DEV_5</td>
      <td>256</td>
      <td>MSI-capable devices have DeviceID</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RTDTC / ITS_DEV_7</td>
      <td>254</td>
      <td>Check Device's SID/RID/DID behind SMMU</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RTDTC / ITS_DEV_8</td>
      <td>255</td>
      <td>Check Device IDs not behind SMMU</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RTDTC / ITS_DEV_9</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_11</td>
      <td></td>
      <td>PCIe RC,PE - Same Inr Shareable Domain</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_12</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_13</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_14</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_15</td>
      <td>1503</td>
      <td>Arrival order Check</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
      <td>pal_exerciser_get_param (TRANSACTION_TYPE)<br>pal_exerciser_ops (START_TXN_MONITOR, STOP_TXN_MONITOR)<br>pal_exerciser_get_data (EXERCISER_DATA_MMIO_SPACE)</td>
    </tr>
    <tr>
      <td>PCI_IC_16</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_17</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_18</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
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
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_PP_02</td>
      <td>1514</td>
      <td>P2P transactions must not deadlock</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
      <td>pal_exerciser_set_param (DMA_ATTRIBUTES)<br>pal_exerciser_ops (START_DMA)</td>
    </tr>
    <tr>
      <td>PCI_PP_03</td>
      <td>819</td>
      <td>RP must suprt ACS if P2P Txn are allow</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>PCIe Hierarchy P2P support required</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_PP_04</td>
      <td></td>
      <td>Check P2P ACS Functionality</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>PCIe Hierarchy and Device P2P support required</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_PP_05</td>
      <td></td>
      <td>Check Direct Transl P2P Support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>PCIe Hierarchy and Device P2P support required</td>
      <td></td>
    </tr>
    <tr>
      <td rowspan="2">PCI_PAS_1</td>
      <td>842</td>
      <td>PASID support at least 16 bits</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>PASID support required</td>
      <td></td>
    </tr>
    <tr>
      <td>1536</td>
      <td>Check PASID use as SubStreamID</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>PASID support and Exerciser VIP required</td>
      <td>pal_exerciser_set_param (DMA_ATTRIBUTES)<br>pal_exerciser_ops (PASID_TLP_START, START_DMA, PASID_TLP_STOP)</td>
    </tr>
    <tr>
      <td>PCI_PTM_1</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RE_PCI_1</td>
      <td>885</td>
      <td>Check Hdr type &amp; link Cap - RCiEP, RCEC</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP; PCIe Hierarchy and Device P2P support required</td>
      <td></td>
    </tr>
    <tr>
      <td>RE_PCI_2</td>
      <td>884</td>
      <td>Check Class code and Ext Cap - RCEC</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCEC</td>
      <td></td>
    </tr>
    <tr>
      <td>RE_CFG_1</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RE_CFG_2</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RE_CFG_3</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RE_ORD_4</td>
      <td>1508</td>
      <td>Tx pending bit clear correctness - RCiEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
      <td>pal_exerciser_set_param (DMA_ATTRIBUTES)<br>pal_exerciser_ops (START_DMA, TXN_NO_SNOOP_ENABLE)</td>
    </tr>
    <tr>
      <td>RE_PWR_2</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RE_PWR_3</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RE_ACS_1</td>
      <td>815</td>
      <td>Check ACS Cap on p2p support - RCiEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP; PCIe Hierarchy and Device P2P support required</td>
      <td></td>
    </tr>
    <tr>
      <td>RE_ACS_2</td>
      <td>816</td>
      <td>Check AER Cap on ACS Cap support - RCiEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP; PCIe Hierarchy and Device P2P support required</td>
      <td></td>
    </tr>
    <tr>
      <td>RE_ACS_3</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RE_REG_1</td>
      <td>848</td>
      <td>Check config header rule - RCEC, RCiEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP, RCEC</td>
      <td></td>
    </tr>
    <tr>
      <td>RE_REG_2</td>
      <td>856</td>
      <td>Check PWR Mgmt Cap/Ctrl/Status - RCEC, RCiEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP, RCEC</td>
      <td></td>
    </tr>
    <tr>
      <td>RE_REG_3</td>
      <td>852</td>
      <td>Check Dev Cap &amp; Ctrl Reg rule - RCiEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP</td>
      <td></td>
    </tr>
    <tr>
      <td>RE_REC_1</td>
      <td>853</td>
      <td>Check Dev Cap &amp; Ctrl Reg rule - RCEC</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCEC</td>
      <td></td>
    </tr>
    <tr>
      <td>RE_REC_2</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td rowspan="88">L2</td>
      <td rowspan="88">RVFLY - B_IEP_1</td>
      <td>RI_CRS_1</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RI_BAR_1</td>
      <td>883</td>
      <td>Read and write to BAR reg - RCiEP, iEP Pair</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP, iEP EP, iEP RP</td>
      <td></td>
    </tr>
    <tr>
      <td>RI_BAR_2</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RI_BAR_3</td>
      <td>862</td>
      <td>Check BAR memory space &amp; type - RCiEP, iEP Pair</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP, iEP EP, iEP RP</td>
      <td></td>
    </tr>
    <tr>
      <td>RI_INT_1</td>
      <td>869</td>
      <td>Check MSI and MSI-X support - RCiEP, iEP Pair</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP, iEP; MSI/MSI-X support required</td>
      <td></td>
    </tr>
    <tr>
      <td>RI_ORD_1</td>
      <td>1521</td>
      <td>Arrival order &amp; Gathering Check - RCiEP, iEP EP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
      <td>pal_exerciser_get_param (TRANSACTION_TYPE, CLEAR_TXN)<br>pal_exerciser_ops (START_TXN_MONITOR, STOP_TXN_MONITOR)<br>pal_exerciser_get_data (EXERCISER_DATA_MMIO_SPACE)</td>
    </tr>
    <tr>
      <td>RI_ORD_2</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RI_ORD_3</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td rowspan="3">RI_SMU_1</td>
      <td>880</td>
      <td>Check ATS Support Rule: RCiEP/iEP/RP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>1519</td>
      <td>PCIe Address translation check</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required; ATC cache required</td>
      <td>pal_exerciser_set_param (DMA_ATTRIBUTES)<br>pal_exerciser_ops (START_DMA)</td>
    </tr>
    <tr>
      <td>1520</td>
      <td>ATS Functionality Check</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required; ATC cache required</td>
      <td>pal_exerciser_get_param (ATS_RES_ATTRIBUTES)<br>pal_exerciser_set_param (DMA_ATTRIBUTES, CFG_TXN_ATTRIBUTES)<br>pal_exerciser_ops (ATS_TXN_REQ, START_DMA, ATS_TXN_CLEAR)</td>
    </tr>
    <tr>
      <td>RI_SMU_2</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RI_PWR_1</td>
      <td>870</td>
      <td>Check Power Mgmt rules - RCiEP, iEP Pair</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP, iEP RP, iEP EP</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_01</td>
      <td>801</td>
      <td>Check ECAM Presence</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP, RCEC</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_02</td>
      <td>802</td>
      <td>Check ECAM Memory accessibility</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP, RCEC</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_03</td>
      <td>838</td>
      <td>Check all RP in HB is in same ECAM</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP RP</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_04</td>
      <td></td>
      <td>All EP/Sw under RP in same ECAM Region</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP RP</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_05</td>
      <td></td>
      <td>Type 0/1 common config rule</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP RP</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_06</td>
      <td>901</td>
      <td>Check config read for dev under iEP_RP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP RP</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_07</td>
      <td>828</td>
      <td>Check all 1's for out of range</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
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
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_10</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_11</td>
      <td>1510</td>
      <td>Check RP Sec Bus transaction are TYPE0</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
      <td>pal_exerciser_get_param (CFG_TXN_ATTRIBUTES, CLEAR_TXN)<br>pal_exerciser_ops (START_TXN_MONITOR, STOP_TXN_MONITOR)</td>
    </tr>
    <tr>
      <td>PCI_IN_12</td>
      <td>837</td>
      <td>Check Config Txn for RP in HB</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP RP</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_13</td>
      <td></td>
      <td>Check RootPort NP Memory Access</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP RP</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_14</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
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
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_16</td>
      <td>808</td>
      <td>Check all 1's for out of range</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP RP</td>
      <td></td>
    </tr>
    <tr>
      <td rowspan="3">PCI_IN_17</td>
      <td>836</td>
      <td>Check ARI forwarding enable rule</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>871</td>
      <td>Check ARI forwarding enable rule</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>1515</td>
      <td>Check ARI forwarding enable rule</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
      <td>pal_exerciser_get_param (CFG_TXN_ATTRIBUTES, CLEAR_TXN)<br>pal_exerciser_ops (START_TXN_MONITOR, STOP_TXN_MONITOR)</td>
    </tr>
    <tr>
      <td>PCI_IN_18</td>
      <td>811</td>
      <td>Check RP Byte Enable Rules</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP RP</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_19</td>
      <td></td>
      <td>Check Command,BIST,and Hdr Cap registers</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP RP</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IN_20</td>
      <td>809</td>
      <td>Vendor specific data is PCIe compliant</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP RP</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_MM_01</td>
      <td></td>
      <td>PCIe Device Memory mapping support</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>RCiEP, RCEC, iEP RP, iEP EP</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_MM_02</td>
      <td>907</td>
      <td>PCIe Non-Cacheable Memory mapping support</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>RCiEP, RCEC, iEP RP, iEP EP</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_MM_03</td>
      <td></td>
      <td>PCIe Normal Memory mapping support</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>RCiEP, RCEC, iEP RP, iEP EP</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_MM_04</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_MM_05</td>
      <td>895</td>
      <td>PCIe devices and PEs common physical address view</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Exerciser VIP required</td>
      <td>pal_exerciser_set_param (DMA_ATTRIBUTES)<br>pal_exerciser_ops (START_DMA)</td>
    </tr>
    <tr>
      <td>PCI_MM_06</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_MM_07</td>
      <td>905</td>
      <td>No extra address translation</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Exerciser VIP required</td>
      <td>pal_exerciser_set_param (DMA_ATTRIBUTES)<br>pal_exerciser_ops (START_DMA)</td>
    </tr>
    <tr>
      <td>PCI_MSI_1</td>
      <td>839</td>
      <td>Check MSI support for PCIe dev</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>MSI/MSI-X support required</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_MSI_2</td>
      <td></td>
      <td>Check MSI(X) vectors uniqueness</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>MSI/MSI-X support required</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_LI_01</td>
      <td></td>
      <td>Check Legacy Interrupt is SPI</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP, RCEC</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_LI_03</td>
      <td></td>
      <td>Check Legacy Intr SPI level sensitive</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>RCiEP, RCEC</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_LI_04</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_SM_01</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_SM_02</td>
      <td>835</td>
      <td>Check Function level reset</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RTDTC / ITS_DEV_5</td>
      <td>256</td>
      <td>MSI-capable devices have DeviceID</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RTDTC / ITS_DEV_7</td>
      <td>254</td>
      <td>Check Device's SID/RID/DID behind SMMU</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RTDTC / ITS_DEV_8</td>
      <td>255</td>
      <td>Check Device IDs not behind SMMU</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>RTDTC / ITS_DEV_9</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_11</td>
      <td></td>
      <td>PCIe RC,PE - Same Inr Shareable Domain</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_12</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_13</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_14</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_15</td>
      <td>1503</td>
      <td>Arrival order Check</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
      <td>pal_exerciser_get_param (TRANSACTION_TYPE)<br>pal_exerciser_ops (START_TXN_MONITOR, STOP_TXN_MONITOR)<br>pal_exerciser_get_data (EXERCISER_DATA_MMIO_SPACE)</td>
    </tr>
    <tr>
      <td>PCI_IC_16</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_17</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_IC_18</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
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
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_PP_02</td>
      <td>1514</td>
      <td>P2P transactions must not deadlock</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
      <td>pal_exerciser_set_param (DMA_ATTRIBUTES)<br>pal_exerciser_ops (START_DMA)</td>
    </tr>
    <tr>
      <td>PCI_PP_03</td>
      <td>819</td>
      <td>RP must suprt ACS if P2P Txn are allow</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>PCIe Hierarchy P2P support required</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_PP_04</td>
      <td></td>
      <td>Check P2P ACS Functionality</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>PCIe Hierarchy P2P support required</td>
      <td></td>
    </tr>
    <tr>
      <td>PCI_PP_05</td>
      <td></td>
      <td>Check Direct Transl P2P Support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>PCIe Hierarchy and Device P2P support required</td>
      <td></td>
    </tr>
    <tr>
      <td rowspan="2">PCI_PAS_1</td>
      <td>842</td>
      <td>PASID support at least 16 bits</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>PASID support required</td>
      <td></td>
    </tr>
    <tr>
      <td>1536</td>
      <td>Check PASID use as SubStreamID</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>PASID support and Exerciser VIP required</td>
      <td>pal_exerciser_set_param (DMA_ATTRIBUTES)<br>pal_exerciser_ops (PASID_TLP_START, START_DMA, PASID_TLP_STOP)</td>
    </tr>
    <tr>
      <td>PCI_PTM_1</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>IE_CFG_1</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>IE_CFG_2</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>IE_CFG_4</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>IE_ORD_4</td>
      <td>1538</td>
      <td>Check Sec Bus Reset - iEP EP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
      <td>pal_exerciser_set_param (DMA_ATTRIBUTES)<br>pal_exerciser_ops (START_DMA, TXN_NO_SNOOP_ENABLE)</td>
    </tr>
    <tr>
      <td>IE_RST_2</td>
      <td>879</td>
      <td>Check Sec Bus Reset - iEP EP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP_RP</td>
      <td></td>
    </tr>
    <tr>
      <td>IE_RST_3</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>IE_PWR_2</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>IE_PWR_3</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>IE_REG_1</td>
      <td></td>
      <td>Check config header rule - iEP EP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP_EP</td>
      <td></td>
    </tr>
    <tr>
      <td>IE_REG_2</td>
      <td></td>
      <td>Check Dev Cap &amp; Ctrl Reg rule - iEP EP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP_EP</td>
      <td></td>
    </tr>
    <tr>
      <td>IE_REG_3</td>
      <td></td>
      <td>Check config header rule - iEP RP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP_EP</td>
      <td></td>
    </tr>
    <tr>
      <td>IE_REG_4</td>
      <td></td>
      <td>Check PCIe capability rules - iEP RP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP_EP</td>
      <td></td>
    </tr>
    <tr>
      <td>IE_REG_5</td>
      <td>857</td>
      <td>Check Power Mgmt Cap/Ctrl/Status - iEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP_EP, IEP RP</td>
      <td></td>
    </tr>
    <tr>
      <td>IE_REG_6</td>
      <td>892</td>
      <td>Secondary PCIe ECap Check - iEP Pair</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP_EP, IEP RP</td>
      <td></td>
    </tr>
    <tr>
      <td>IE_REG_7</td>
      <td>812</td>
      <td>Datalink feature ECap Check - iEP Pair</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP_EP, IEP RP</td>
      <td></td>
    </tr>
    <tr>
      <td>IE_REG_8</td>
      <td>813</td>
      <td>Phy Layer 16GT/s ECap Check - iEP Pair</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP_EP, IEP RP</td>
      <td></td>
    </tr>
    <tr>
      <td>IE_REG_9</td>
      <td>814</td>
      <td>Phy Layer 16GT/s ECap Check - iEP Pair</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>iEP_EP, IEP RP</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1NV_01</td>
      <td>P_L1NV_01</td>
      <td>-</td>
      <td>RuntimeServices/VariableServicesTest</td>
      <td>SCT</td>
      <td>No</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1SE_01</td>
      <td>P_L1SE_01</td>
      <td>-</td>
      <td>RuntimeServices/VariableServicesTest</td>
      <td>SCT</td>
      <td>No</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1SE_02</td>
      <td>P_L1SE_02</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1SE_03</td>
      <td>P_L1SE_03</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1SE_04</td>
      <td>P_L1SE_04</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1SE_05</td>
      <td>P_L1SE_05</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1TP_01</td>
      <td>P_L1TP_01</td>
      <td>1601</td>
      <td>Check TPM Version</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1TP_02</td>
      <td>P_L1TP_02</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1TP_03</td>
      <td>P_L1TP_03</td>
      <td>1602</td>
      <td>Check TPM interface locality support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>P_L1TP_04</td>
      <td>P_L1TP_04</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td rowspan="5">FR</td>
      <td rowspan="5">P_L2WD_01</td>
      <td>B_WD_01</td>
      <td>701</td>
      <td>Non Secure Watchdog Access</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>B_WD_02</td>
      <td>701</td>
      <td>Non Secure Watchdog Access</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>B_WD_03</td>
      <td>702</td>
      <td>Check Watchdog WS0 interrupt</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>B_WD_04</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>B_WD_05</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
  </tbody>
</table>

## Latest checklist changes

### PC BSA ACS v1.0.0

- Initial PC-BSA ACS testcase checklist check-in.

### PC BSA ACS v2.0.0

- PC-BSA ACS 2.0.0 updates
- Changed the format to match other ACS checklist 
