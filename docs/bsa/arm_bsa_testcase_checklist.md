## BSA ACS Testcase checklist

The BSA ACS test checklist is based on **BSA 1.1 specification** and **BSA ACS 1.1.2** tag.

The checklist provides information about:

1. The BSA level at which each test runs.
2. The BSA rules covered by each test.
3. The BSA rules that are not currently covered by ACS.
4. The runtime environment (UEFI, BareMetal, or Linux) in which each test executes.
5. In a UEFI or Linux based test is **not** part of the SystemReady images, it is highlighted with `#`.
6. Tests which are dependent on platform information which cannot be auto-determined and needs manual effort to provide information in PAL API's are marked with `&`

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
      <th>Additonal Information</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>L1</td>
      <td>B_PE_01</td>
      <td>B_PE_01</td>
      <td>1</td>
      <td>Check Arch symmetry across PE</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PE_02</td>
      <td>B_PE_02</td>
      <td>2</td>
      <td>Check for number of PE</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PE_03</td>
      <td>B_PE_03</td>
      <td>3</td>
      <td>Check for AdvSIMD and FP support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PE_04</td>
      <td>B_PE_04</td>
      <td>4</td>
      <td>Check PE 4KB Granule Support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PE_05</td>
      <td>B_PE_05</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PE_06</td>
      <td>B_PE_06</td>
      <td>6</td>
      <td>Check Cryptographic extensions</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PE_07</td>
      <td>B_PE_07</td>
      <td>7</td>
      <td>Check Little Endian support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PE_08</td>
      <td>B_PE_08</td>
      <td>8</td>
      <td>Check EL1 and EL0 implementation</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PE_09</td>
      <td>B_PE_09</td>
      <td>9</td>
      <td>Check for PMU and PMU counters</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PE_10</td>
      <td>B_PE_10</td>
      <td>10</td>
      <td>Check PMU Overflow signal</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PE_11</td>
      <td>B_PE_11</td>
      <td>11</td>
      <td>Check num of Breakpoints and type</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PE_12</td>
      <td>B_PE_12</td>
      <td>12</td>
      <td>Check Synchronous Watchpoints</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PE_13</td>
      <td>B_PE_13</td>
      <td>13</td>
      <td>Check CRC32 instruction support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PE_14</td>
      <td>B_PE_14</td>
      <td>16</td>
      <td>Check SVE2 for v9 PE</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td>B_PE_15</td>
      <td>B_PE_15</td>
      <td>14</td>
      <td>Check PAuth if implementation</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>The rule is no longer present in BSA 1.1 checklist</td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PE_18</td>
      <td>B_PE_18</td>
      <td>17</td>
      <td>Check EL2 implementation</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PE_19</td>
      <td>B_PE_19</td>
      <td>18</td>
      <td>Check Stage 2 4KB Granule Support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PE_20</td>
      <td>B_PE_20</td>
      <td>19</td>
      <td>Check Stage2 and Stage1 Granule match</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PE_21</td>
      <td>B_PE_21</td>
      <td>20</td>
      <td>Check for PMU counters</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PE_22</td>
      <td>B_PE_22</td>
      <td>21</td>
      <td>Check VMID breakpoint number</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PE_23</td>
      <td>B_PE_23</td>
      <td>22</td>
      <td>Check for EL3 AArch64 support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PE_24</td>
      <td>B_PE_24</td>
      <td>63</td>
      <td>Check for Secure state implementation</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_MEM_01</td>
      <td>B_MEM_01</td>
      <td>102</td>
      <td>Mem Access Response in finite time</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_MEM_02</td>
      <td>B_MEM_02</td>
      <td>101</td>
      <td>Memory Access to Un-Populated addr</td>
      <td>No</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_MEM_03</td>
      <td>B_MEM_03</td>
      <td>104</td>
      <td>Check Addressability</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_MEM_04</td>
      <td>B_MEM_04</td>
      <td>106</td>
      <td>Check Addressability when SMMU's off</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_MEM_05</td>
      <td>B_MEM_05</td>
      <td>103</td>
      <td>PE must access all NS addr space</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_MEM_06</td>
      <td>B_MEM_06</td>
      <td>107</td>
      <td>Check Addressability for non-DMA dev</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_MEM_07</td>
      <td>B_MEM_07</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_MEM_08</td>
      <td>B_MEM_08</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_MEM_09</td>
      <td>B_MEM_09</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_GIC_01</td>
      <td>B_GIC_01</td>
      <td>201</td>
      <td>Check GIC version</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_GIC_02</td>
      <td>B_GIC_02</td>
      <td>202</td>
      <td>Check GICv2 Valid Configuration</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_GIC_03</td>
      <td>B_GIC_03</td>
      <td>203</td>
      <td>If PCIe, GICv3 then ITS, LPI</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_GIC_04</td>
      <td>B_GIC_04</td>
      <td>204</td>
      <td>Check GICv3 Security States</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_GIC_05</td>
      <td>B_GIC_05</td>
      <td>205</td>
      <td>Non-secure SGIs are implemented</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PPI_00</td>
      <td>B_PPI_01</td>
      <td>206</td>
      <td>Check EL1-Phy timer PPI assignment</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>207</td>
      <td>Check EL1-Virt timer PPI assignment</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>B_PPI_02</td>
      <td>209</td>
      <td>Check NS EL2-Virt timer PPI Assignment</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>210</td>
      <td>Check NS EL2-Phy timer PPI Assignment</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>211</td>
      <td>Check GIC Maintenance PPI Assignment</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>B_PPI_03</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_GIC_02</td>
      <td>Appendix I.5</td>
      <td>229</td>
      <td>Check GICv2m SPI allocated to MSI Ctrl</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>Appendix I.6</td>
      <td>226</td>
      <td>Check MSI SPI are Edge Triggered</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>Appendix I.6</td>
      <td>228</td>
      <td>Check GICv2m MSI to SPI Generation</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>Appendix I.9</td>
      <td>227</td>
      <td>Check GICv2m MSI Frame Register</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td></td>
      <td>ITS_01</td>
      <td>251</td>
      <td>Check number of ITS blocks in a group</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ITS_02</td>
      <td>252</td>
      <td>Check ITS block association with group</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ITS_03</td>
      <td>1511</td>
      <td>MSI-capable device linked to ITS group</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ITS_04</td>
      <td>1535</td>
      <td>MSI-cap device can target any ITS blk</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ITS_05</td>
      <td>1512</td>
      <td>MSI to ITS Blk outside assigned group</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ITS_06</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ITS_07</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ITS_08</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ITS_DEV_1</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ITS_DEV_2</td>
      <td>253</td>
      <td>Check uniqueness of StreamID</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ITS_DEV_3</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ITS_DEV_4</td>
      <td>1513</td>
      <td>MSI originating from different master</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ITS_DEV_5</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ITS_DEV_6</td>
      <td>1504</td>
      <td>MSI-X triggers intr with unique ID</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ITS_DEV_7</td>
      <td>254</td>
      <td>Check Device's SID/RID/DID behind SMMU</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ITS_DEV_8</td>
      <td>255</td>
      <td>Check Device IDs not behind SMMU</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ITS_DEV_9</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_SMMU_01</td>
      <td>B_SMMU_01</td>
      <td>301</td>
      <td>All SMMUs have same Arch Revision</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_SMMU_02</td>
      <td>B_SMMU_02</td>
      <td>302</td>
      <td>Check SMMU Granule Support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_SMMU_06</td>
      <td>B_SMMU_06</td>
      <td>303</td>
      <td>Check SMMU Large Physical Addr Support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_SMMU_07</td>
      <td>B_SMMU_07</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_SMMU_08</td>
      <td>B_SMMU_08</td>
      <td>304</td>
      <td>Check SMMU S-EL2 &amp; stage1 support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_SMMU_12</td>
      <td>B_SMMU_12</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_SMMU_16</td>
      <td>B_SMMU_16</td>
      <td>305</td>
      <td>Check SMMUs stage2 support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_SMMU_17</td>
      <td>B_SMMU_17</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_SMMU_18</td>
      <td>B_SMMU_18</td>
      <td>329</td>
      <td>Check SMMU S-EL2 &amp; stage2 support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_SMMU_19</td>
      <td>B_SMMU_19</td>
      <td>306</td>
      <td>SMMUv2 unique intr per ctxt bank</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_SMMU_21</td>
      <td>B_SMMU_21</td>
      <td>307</td>
      <td>SMMUv3 Integration compliance</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_TIME_01</td>
      <td>B_TIME_01</td>
      <td>401</td>
      <td>Check for Generic System Counter</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_TIME_02</td>
      <td>B_TIME_02</td>
      <td>407</td>
      <td>Check System Counter Frequency</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_TIME_03</td>
      <td>B_TIME_03</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_TIME_04</td>
      <td>B_TIME_04</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_TIME_05</td>
      <td>B_TIME_05</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_TIME_06</td>
      <td>B_TIME_06</td>
      <td>402</td>
      <td>SYS Timer if PE Timer not ON</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_TIME_07</td>
      <td>B_TIME_07</td>
      <td>403</td>
      <td>Memory mapped timer check</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_TIME_08</td>
      <td>B_TIME_08</td>
      <td>404</td>
      <td>Generate Mem Mapped SYS Timer Intr</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_TIME_09</td>
      <td>B_TIME_09</td>
      <td>405</td>
      <td>Restore PE timer on PE wake up</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_TIME_10</td>
      <td>B_TIME_10</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_WAK_01</td>
      <td>B_WAK_01</td>
      <td>501</td>
      <td>Wake from EL1 PHY Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>502</td>
      <td>Wake from EL1 VIR Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>503</td>
      <td>Wake from EL2 PHY Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>504</td>
      <td>Wake from Watchdog WS0 Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>505</td>
      <td>Wake from System Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_WAK_02</td>
      <td>B_WAK_02</td>
      <td>501</td>
      <td>Wake from EL1 PHY Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>502</td>
      <td>Wake from EL1 VIR Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>503</td>
      <td>Wake from EL2 PHY Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>504</td>
      <td>Wake from Watchdog WS0 Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>505</td>
      <td>Wake from System Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_WAK_03</td>
      <td>B_WAK_03</td>
      <td>501</td>
      <td>Wake from EL1 PHY Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>502</td>
      <td>Wake from EL1 VIR Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>503</td>
      <td>Wake from EL2 PHY Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>504</td>
      <td>Wake from Watchdog WS0 Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>505</td>
      <td>Wake from System Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_WAK_04</td>
      <td>B_WAK_04</td>
      <td>501</td>
      <td>Wake from EL1 PHY Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>502</td>
      <td>Wake from EL1 VIR Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>503</td>
      <td>Wake from EL2 PHY Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>504</td>
      <td>Wake from Watchdog WS0 Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>505</td>
      <td>Wake from System Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_WAK_05</td>
      <td>B_WAK_05</td>
      <td>501</td>
      <td>Wake from EL1 PHY Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>502</td>
      <td>Wake from EL1 VIR Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>503</td>
      <td>Wake from EL2 PHY Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>504</td>
      <td>Wake from Watchdog WS0 Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>505</td>
      <td>Wake from System Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_WAK_06</td>
      <td>B_WAK_06</td>
      <td>501</td>
      <td>Wake from EL1 PHY Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>502</td>
      <td>Wake from EL1 VIR Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>503</td>
      <td>Wake from EL2 PHY Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>504</td>
      <td>Wake from Watchdog WS0 Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>505</td>
      <td>Wake from System Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_WAK_07</td>
      <td>B_WAK_07</td>
      <td>501</td>
      <td>Wake from EL1 PHY Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>502</td>
      <td>Wake from EL1 VIR Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>503</td>
      <td>Wake from EL2 PHY Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>504</td>
      <td>Wake from Watchdog WS0 Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>505</td>
      <td>Wake from System Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_WAK_08</td>
      <td>B_WAK_08</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_WAK_10</td>
      <td>B_WAK_10</td>
      <td>501</td>
      <td>Wake from EL1 PHY Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>502</td>
      <td>Wake from EL1 VIR Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>503</td>
      <td>Wake from EL2 PHY Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>504</td>
      <td>Wake from Watchdog WS0 Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>505</td>
      <td>Wake from System Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_WAK_11</td>
      <td>B_WAK_11</td>
      <td>501</td>
      <td>Wake from EL1 PHY Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>502</td>
      <td>Wake from EL1 VIR Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>503</td>
      <td>Wake from EL2 PHY Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>504</td>
      <td>Wake from Watchdog WS0 Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>505</td>
      <td>Wake from System Timer Int</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_WAK_09</td>
      <td>B_WAK_09</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_WD_00</td>
      <td>B_WD_01</td>
      <td>701</td>
      <td>Non Secure Watchdog Access</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>B_WD_02</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>B_WD_03</td>
      <td>702</td>
      <td>Check Watchdog WS0 interrupt</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>B_WD_04</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>B_WD_05</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PER_01</td>
      <td>B_PER_01</td>
      <td>601</td>
      <td>USB CTRL Interface EHCI check</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PER_02</td>
      <td>B_PER_02</td>
      <td>608</td>
      <td>USB CTRL Interface XHCI check</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PER_03</td>
      <td>B_PER_03</td>
      <td>602</td>
      <td>Check SATA CTRL Interface</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PER_04</td>
      <td>B_PER_04</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PER_05</td>
      <td>B_PER_05</td>
      <td>603</td>
      <td>Check UART type Arm Generic or 16550</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PER_06</td>
      <td>B_PER_06</td>
      <td>606</td>
      <td>Check Arm GENERIC UART Interrupt</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PER_07</td>
      <td>B_PER_07</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PER_08</td>
      <td>PCI_IN_01</td>
      <td>801</td>
      <td>Check ECAM Presence</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IN_02</td>
      <td>802</td>
      <td>PE - ECAM Region accessibility check</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IN_03</td>
      <td>838</td>
      <td>Check all RP in HB is in same ECAM</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IN_04</td>
      <td>803</td>
      <td>All EP/Sw under RP in same ECAM Region</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IN_05</td>
      <td>820</td>
      <td>Type 0/1 common config rule</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>822</td>
      <td>Check Type 1 config header rules</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>824</td>
      <td>Device capabilities reg rule</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>825</td>
      <td>Device Control register rule</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>826</td>
      <td>Device cap 2 register rules</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>1517</td>
      <td>Check BME functionality of RP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>833</td>
      <td>Check Max payload size supported</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IN_06</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IN_07</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IN_08</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IN_09</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IN_10</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IN_11</td>
      <td>1510</td>
      <td>Check RP Sec Bus transaction are TYPE0</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IN_12</td>
      <td>837</td>
      <td>Check Config Txn for RP in HB</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IN_13</td>
      <td>804</td>
      <td>Check RootPort NP Memory Access</td>
      <td>No</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>805</td>
      <td>Check RootPort P Memory Access</td>
      <td>No</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IN_14</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IN_15</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IN_16</td>
      <td>808</td>
      <td>Check all 1's for out of range</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IN_17</td>
      <td>1515</td>
      <td>Check ARI forwarding enable rule</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>836</td>
      <td>Check ARI forwarding enable rule</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IN_18</td>
      <td>811</td>
      <td>Check RP Byte Enable Rules</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IN_19</td>
      <td>830</td>
      <td>Check Cmd Reg memory space enable</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>831</td>
      <td>Check Type0/1 BIST Register rule</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>832</td>
      <td>Check HDR CapPtr Register rule</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IN_20</td>
      <td>809</td>
      <td>Vendor specific data is PCIe compliant</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_MM_01</td>
      <td>845</td>
      <td>PCIe Device Memory mapping support</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>1516</td>
      <td>PCIe Device Memory access check</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_MM_02</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_MM_03</td>
      <td>894</td>
      <td>PCIe Normal Memory mapping support</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>1539</td>
      <td>PCIe Normal Memory access check</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_MM_04</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_MM_05</td>
      <td>895</td>
      <td>PCIe &amp; PE common physical memory view</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_MM_06</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_MM_07</td>
      <td>905</td>
      <td>No extra address translation</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_MSI_1</td>
      <td>839</td>
      <td>Check MSI support for PCIe dev</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_MSI_2</td>
      <td>897</td>
      <td>Check MSI=X vectors uniqueness</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>1533</td>
      <td>MSI(-X) triggers intr with unique ID</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_LI_01</td>
      <td>806</td>
      <td>Check Legacy Intrrupt is SPI</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_LI_02</td>
      <td>896</td>
      <td>PCI legacy intr SPI ID unique</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>1506</td>
      <td>Generate PCIe legacy interrupt</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_LI_03</td>
      <td>823</td>
      <td>Check Legacy Intr SPI level sensitive</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_LI_04</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_SM_01</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_SM_02</td>
      <td>835</td>
      <td>Check Function level reset</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IC_11</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IC_12</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IC_13</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IC_14</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IC_15</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IC_16</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IC_17</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IC_18</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IO_01</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_IEP_1</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_PP_01</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_PP_02</td>
      <td>1514</td>
      <td>P2P transactions must not deadlock</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_PP_03</td>
      <td>819</td>
      <td>RP must suprt ACS if P2P Txn are allow</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_PP_04</td>
      <td>818</td>
      <td>Check RP Adv Error Report</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>1501</td>
      <td>Check P2P ACS Functionality</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>1502</td>
      <td>Check ACS Redirect Req Valid</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_PP_05</td>
      <td>817</td>
      <td>Check Direct Transl P2P Support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_PAS_1</td>
      <td>842</td>
      <td>PASID support atleast 16 bits</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PCI_PTM_1</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PER_09</td>
      <td>B_PER_09</td>
      <td>604</td>
      <td>Check Memory Attributes of DMA</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PER_10</td>
      <td>B_PER_10</td>
      <td>607</td>
      <td>Memory Attribute of I/O coherent DMA</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PER_12</td>
      <td>B_PER_12</td>
      <td>821</td>
      <td>Type 0 config header rules</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L1</td>
      <td>B_PER_11</td>
      <td>B_PER_11</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>B_PE_16</td>
      <td>B_PE_16</td>
      <td>linux mte app</td>
      <td>Check for MTE support</td>
      <td>No</td>
      <td>No</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>B_PE_17</td>
      <td>B_PE_17</td>
      <td>37</td>
      <td>Check SPE if implemented</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>B_PE_25</td>
      <td>B_PE_25</td>
      <td>15</td>
      <td>Check for FEAT_LSE support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>B_SMMU_03</td>
      <td>B_SMMU_03</td>
      <td>316</td>
      <td>Check SMMU Large VA Support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>B_SMMU_04</td>
      <td>B_SMMU_04</td>
      <td>317</td>
      <td>Check TLB Range Invalidation</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>B_SMMU_05</td>
      <td>B_SMMU_05</td>
      <td>330</td>
      <td>Check DVM capabilities</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>B_SMMU_09</td>
      <td>B_SMMU_09</td>
      <td>310</td>
      <td>Check S-EL2 &amp; SMMU Stage1 support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>B_SMMU_11</td>
      <td>B_SMMU_11</td>
      <td>312</td>
      <td>Check SMMU for MPAM support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>B_SMMU_13</td>
      <td>B_SMMU_13</td>
      <td>318</td>
      <td>Check SMMU 16 Bit ASID Support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>B_SMMU_14</td>
      <td>B_SMMU_14</td>
      <td>319</td>
      <td>Check SMMU Endianess Support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>B_SMMU_20</td>
      <td>B_SMMU_20</td>
      <td>311</td>
      <td>Check S-EL2 &amp; SMMU Stage2 Support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>B_SMMU_23</td>
      <td>B_SMMU_23</td>
      <td>315</td>
      <td>Check SMMU 16 Bit VMID Support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>B_SMMU_24</td>
      <td>B_SMMU_24</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>B_SMMU_25</td>
      <td>B_SMMU_25</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
  </tbody>
</table>

## Latest Checklist Changes
