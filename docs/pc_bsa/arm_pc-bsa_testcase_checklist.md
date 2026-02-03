## PC-BSA ACS Testcase checklist

The PC-BSA ACS test checklist is based on the **PC BSA v1.0 specification** and the **PC BSA ACS v1.0.0** release.

The checklist provides information about:

1. The PC BSA level at which each test runs.
2. The PC BSA rules covered by ACS.
3. The runtime environment (UEFI, BareMetal, Linux) in which each test executes.

> **Note:** Rules that cannot be validated by ACS require a **manual declaration of compliance** from the DUT owner.

<br>
<table border="1" class="dataframe dataframe">
  <thead>
    <tr>
      <th>Test No</th>
      <th>Test Description</th>
      <th>Level</th>
      <th>Rule ID</th>
      <th>Coverage by ACS</th>
      <th>UEFI</th>
      <th>BareMetal</th>
      <th>Linux</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>-</td>
      <td>Covered by BSA Rules</td>
      <td>1</td>
      <td>P_L1_01</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>4</td>
      <td>Check PE 4KB Granule Support</td>
      <td>1</td>
      <td>P_L1PE_01</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>18</td>
      <td>Check PE Stage 2 4KB Granule Support</td>
      <td>1</td>
      <td>P_L1PE_01</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>24</td>
      <td>Check for 16-bit ASID support</td>
      <td>1</td>
      <td>P_L1PE_02</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>25</td>
      <td>Check AARCH64 implementation</td>
      <td>1</td>
      <td>P_L1PE_03</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>15</td>
      <td>Check for FEAT_LSE support</td>
      <td>1</td>
      <td>P_L1PE_04</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>26</td>
      <td>Check FEAT_LPA Requirements</td>
      <td>1</td>
      <td>P_L1PE_05</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>28</td>
      <td>Check DC CVAP support</td>
      <td>1</td>
      <td>P_L1PE_06</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>29</td>
      <td>Check for 16-Bit VMID</td>
      <td>1</td>
      <td>P_L1PE_07</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>30</td>
      <td>Check for Virtual host extensions</td>
      <td>1</td>
      <td>P_L1PE_08</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>105</td>
      <td>NS-EL2 Stage-2 64KB Mapping Check</td>
      <td>1</td>
      <td>P_L1MM_01</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>108</td>
      <td>Check peripherals addr 64Kb apart</td>
      <td>1</td>
      <td>P_L1MM_01</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>212</td>
      <td>Check GIC version</td>
      <td>1</td>
      <td>P_L1GI_01</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>846</td>
      <td>Check all MSI(X) vectors are LPIs</td>
      <td>1</td>
      <td>P_L1GI_02</td>
      <td>Yes</td>
      <td>No</td>
      <td>No</td>
      <td>Yes</td>
    </tr>
    <tr>
      <td>210</td>
      <td>Check GIC supports disabling LPIs</td>
      <td>1</td>
      <td>P_L1GI_03</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>211</td>
      <td>Check GICR_PENDBASER when LPIs enabled</td>
      <td>1</td>
      <td>P_L1GI_04</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>214</td>
      <td>Check All PPI Interrupt IDs</td>
      <td>1</td>
      <td>P_L1PP_01</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>-</td>
      <td>-</td>
      <td>1</td>
      <td>P_L1SM_01</td>
      <td>No</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
    </tr>
    <tr>
      <td>308</td>
      <td>Check Stage 1 SMMUv3 functionality</td>
      <td>1</td>
      <td>P_L1SM_02</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>325</td>
      <td>Check Stage 2 SMMUv3 functionality</td>
      <td>1</td>
      <td>P_L1SM_03</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>320</td>
      <td>Check SMMU Coherent Access Support</td>
      <td>1</td>
      <td>P_L1SM_04</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>322</td>
      <td>Check if all DMA reqs behind SMMU</td>
      <td>1</td>
      <td>P_L1SM_05</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>-</td>
      <td>-</td>
      <td>1</td>
      <td>P_L1PCI_1</td>
      <td>No</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
    </tr>
    <tr>
      <td>887</td>
      <td>Check EA Capability</td>
      <td>1</td>
      <td>P_L1PCI_2</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>-</td>
      <td>RuntimeServices/VariableServicesTest</td>
      <td>1</td>
      <td>P_L1NV_01</td>
      <td>Yes</td>
      <td>SCT</td>
      <td>No</td>
      <td>No</td>
    </tr>
    <tr>
      <td>-</td>
      <td>RuntimeServices/VariableServicesTest</td>
      <td>1</td>
      <td>P_L1SE_01</td>
      <td>Yes</td>
      <td>SCT</td>
      <td>No</td>
      <td>No</td>
    </tr>
    <tr>
      <td>-</td>
      <td>-</td>
      <td>1</td>
      <td>P_L1SE_02</td>
      <td>No</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
    </tr>
    <tr>
      <td>-</td>
      <td>-</td>
      <td>1</td>
      <td>P_L1SE_03</td>
      <td>No</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
    </tr>
    <tr>
      <td>-</td>
      <td>-</td>
      <td>1</td>
      <td>P_L1SE_04</td>
      <td>No</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
    </tr>
    <tr>
      <td>-</td>
      <td>-</td>
      <td>1</td>
      <td>P_L1SE_05</td>
      <td>No</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
    </tr>
    <tr>
      <td>1601</td>
      <td>Check TPM Version</td>
      <td>1</td>
      <td>P_L1TP_01</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>-</td>
      <td>-</td>
      <td>1</td>
      <td>P_L1TP_02</td>
      <td>No</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
    </tr>
    <tr>
      <td>1602</td>
      <td>Check TPM interface locality support</td>
      <td>1</td>
      <td>P_L1TP_03</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>-</td>
      <td>-</td>
      <td>1</td>
      <td>P_L1TP_04</td>
      <td>No</td>
      <td>-</td>
      <td>-</td>
      <td>-</td>
    </tr>
    <tr>
      <td>701</td>
      <td>Non Secure Watchdog Access</td>
      <td>FR</td>
      <td>P_L2WD_01</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
    <tr>
      <td>702</td>
      <td>Check Watchdog WS0 interrupt</td>
      <td>FR</td>
      <td>P_L2WD_01</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
    </tr>
  </tbody>
</table>
