## SBSA ACS Testcase checklist

The SBSA ACS test checklist is based on **SBSA 7.2 specification** and **SBSA ACS 7.2.3** tag.

The checklist provides information about:

1. The SBSA level at which each test runs.
2. The SBSA rules covered by each test.
3. The SBSA rules that are not currently covered by ACS.
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
      <th>Additional Information</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>L3</td>
      <td>S_L3_01</td>
      <td>S_L3_01</td>
      <td>-</td>
      <td>Run BSA ACS</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L3</td>
      <td>S_L3PE_01</td>
      <td>S_L3PE_01</td>
      <td>23</td>
      <td>Check PE Granule Support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L3</td>
      <td>S_L3PE_02</td>
      <td>S_L3PE_02</td>
      <td>24</td>
      <td>Check for 16-bit ASID support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L3</td>
      <td>S_L3PE_03</td>
      <td>S_L3PE_03</td>
      <td>25</td>
      <td>Check AARCH64 implementation</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L3</td>
      <td>S_L3PE_04</td>
      <td>S_L3PE_04</td>
      <td>26</td>
      <td>Check FEAT_LPA Requirements</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L3</td>
      <td>S_L3MM_01</td>
      <td>S_L3MM_01</td>
      <td>105</td>
      <td>NS-EL2 Stage-2 64KB Mapping Check</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L3</td>
      <td>S_L3MM_02</td>
      <td>S_L3MM_02</td>
      <td>108</td>
      <td>Check peripherals addr 64Kb apart</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L3</td>
      <td>S_L3GI_01</td>
      <td>S_L3GI_01</td>
      <td>212</td>
      <td>Check GIC version</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L3</td>
      <td>S_L3GI_02</td>
      <td>S_L3GI_02</td>
      <td>846</td>
      <td>Check all MSI(X) vectors are LPIs</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td>L3</td>
      <td>S_L3PP_01</td>
      <td>S_L3PP_01</td>
      <td>214</td>
      <td>Check All PPI Interrupt IDs</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L3</td>
      <td>S_L3SM_01</td>
      <td>S_L3SM_01</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L3</td>
      <td>S_L3WD_01</td>
      <td>S_L3WD_01</td>
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
      <td></td>
      <td>702</td>
      <td>Check Watchdog WS0 interrupt</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L3</td>
      <td>S_L3PR_01</td>
      <td>S_L3PR_01</td>
      <td>603</td>
      <td>Check UART type Arm Generic or 16550</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L3</td>
      <td>S_PCIe_09</td>
      <td>S_PCIe_09</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L4</td>
      <td>S_L4PE_01</td>
      <td>S_L4PE_01</td>
      <td>27</td>
      <td>Check for RAS extension</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L4</td>
      <td>S_L4PE_02</td>
      <td>S_L4PE_02</td>
      <td>28</td>
      <td>Check DC CVAP support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L4</td>
      <td>S_L4PE_03</td>
      <td>S_L4PE_03</td>
      <td>29</td>
      <td>Check for 16-Bit VMID</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L4</td>
      <td>S_L4PE_04</td>
      <td>S_L4PE_04</td>
      <td>30</td>
      <td>Check for Virtual host extensions</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L4</td>
      <td>S_L4SM_01</td>
      <td>S_L4SM_01</td>
      <td>308</td>
      <td>Check Stage 1 SMMUv3 functionality</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L4</td>
      <td>S_L4SM_02</td>
      <td>S_L4SM_02</td>
      <td>325</td>
      <td>Check Stage 2 SMMUv3 functionality</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L4</td>
      <td>S_L4SM_03</td>
      <td>S_L4SM_03</td>
      <td>320</td>
      <td>Check SMMU Coherent Access Support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L4</td>
      <td>S_L4PCI_1</td>
      <td>S_L4PCI_1</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L4</td>
      <td>S_L4PCI_2</td>
      <td>S_L4PCI_2</td>
      <td>887</td>
      <td>Check EA Capability</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L5</td>
      <td>S_L5PE_01</td>
      <td>S_L5PE_01</td>
      <td>31</td>
      <td>Support Page table map size change</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L5</td>
      <td>S_L5PE_02</td>
      <td>S_L5PE_02</td>
      <td>32</td>
      <td>Check for addr and generic auth</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L5</td>
      <td>S_L5PE_03</td>
      <td>S_L5PE_03</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L5</td>
      <td>S_L5PE_04</td>
      <td>S_L5PE_04</td>
      <td>33</td>
      <td>Check Activity monitors extension</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L5</td>
      <td>S_L5PE_05</td>
      <td>S_L5PE_05</td>
      <td>34</td>
      <td>Check for SHA3 and SHA512 support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L5</td>
      <td>S_L5PE_06</td>
      <td>S_L5PE_06</td>
      <td>35</td>
      <td>Stage 2 control of mem and cache</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L5</td>
      <td>S_L5PE_07</td>
      <td>S_L5PE_07</td>
      <td>36</td>
      <td>Check for nested virtualization</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L5</td>
      <td>S_L5GI_01</td>
      <td>S_L5GI_01</td>
      <td>216</td>
      <td>Check Non GIC Interrupts</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L5</td>
      <td>S_L5SM_01</td>
      <td>S_L5SM_01</td>
      <td>309</td>
      <td>Check SMMUv3.2 or higher</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L5</td>
      <td>S_L5SM_02</td>
      <td>S_L5SM_02</td>
      <td>326</td>
      <td>Check SMMU L1 and L2 table resizing</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L5</td>
      <td>S_L5SM_03</td>
      <td>S_L5SM_03</td>
      <td>312</td>
      <td>Check SMMU for MPAM support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L5</td>
      <td>S_L5SM_04</td>
      <td>S_L5SM_04</td>
      <td>Rejected</td>
      <td>https://jira.arm.com/browse/PJ03164-979</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L5</td>
      <td>S_L5TI_01</td>
      <td>S_L5TI_01</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L5</td>
      <td>S_L5PP_01</td>
      <td>S_L5PP_01</td>
      <td>213</td>
      <td>Check Reserved PPI Assignments</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L6</td>
      <td>S_L6PE_01</td>
      <td>B_SEC_01</td>
      <td>43</td>
      <td>Check Speculation Restriction</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>B_SEC_02</td>
      <td>44</td>
      <td>Check Speculative Str Bypass Safe</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>B_SEC_03</td>
      <td>45</td>
      <td>Check PEs Impl CSDB,SSBB,PSSBB</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>B_SEC_04</td>
      <td>46</td>
      <td>Check PEs Implement SB Barrier</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>B_SEC_05</td>
      <td>47</td>
      <td>Check PE Impl CFP,DVP,CPP RCTX</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L6</td>
      <td>S_L6PE_02</td>
      <td>S_L6PE_02</td>
      <td>38</td>
      <td>Check Branch Target Support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L6</td>
      <td>S_L6PE_03</td>
      <td>S_L6PE_03</td>
      <td>39</td>
      <td>Check Protect Against Timing Fault</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L6</td>
      <td>S_L6PE_04</td>
      <td>S_L6PE_04</td>
      <td>40</td>
      <td>Check PMU Version v3.5 or higher</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L6</td>
      <td>S_L6PE_05</td>
      <td>S_L6PE_05</td>
      <td>41</td>
      <td>Check AccessFlag DirtyState Update</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L6</td>
      <td>S_L6PE_06</td>
      <td>S_L6PE_06</td>
      <td>42</td>
      <td>Check Enhanced Virtualization Trap</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L6</td>
      <td>S_L6PE_07</td>
      <td>S_L6PE_07</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L6</td>
      <td>S_L6PE_08</td>
      <td>S_L6PE_08</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L6</td>
      <td>S_L6SM_02</td>
      <td>S_L6SM_02</td>
      <td>313</td>
      <td>Check SMMU HTTU Support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L6</td>
      <td>S_L6SM_03</td>
      <td>S_L6SM_03</td>
      <td>314</td>
      <td>Check SMMU MSI Support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L6</td>
      <td>S_L6SM_04</td>
      <td>S_L6SM_04</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L6</td>
      <td>S_L6WD_01</td>
      <td>S_L6WD_01</td>
      <td>703</td>
      <td>Check NS Watchdog Revision</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L6</td>
      <td>S_RAS_01</td>
      <td>S_RAS_01</td>
      <td>1214</td>
      <td>Check RAS SR Interface ERI/FHI are PPI</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L6</td>
      <td>S_RAS_03</td>
      <td>S_RAS_03</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L6</td>
      <td>S_L6PCI_1 - B_REP_1</td>
      <td>RE_PCI_1</td>
      <td>885</td>
      <td>Check RCiEP Hdr type &amp; link Cap</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RE_PCI_2</td>
      <td>884</td>
      <td>Check RCEC Class code and Ext Cap</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RE_CFG_1</td>
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
      <td>RE_CFG_2</td>
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
      <td>RE_CFG_3</td>
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
      <td>RE_CRS_1</td>
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
      <td>RE_BAR_1</td>
      <td>883</td>
      <td>Read and write to BAR reg: RCiEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RE_BAR_2</td>
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
      <td>RE_BAR_3</td>
      <td>862</td>
      <td>Check BAR memory space &amp; type: RCiEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RE_INT_1</td>
      <td>869</td>
      <td>Check MSI and MSI-X support: RCiEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RE_ORD_1</td>
      <td>1521</td>
      <td>Arrival order &amp; Gathering Check: RCiEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RE_ORD_2</td>
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
      <td>RE_ORD_3</td>
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
      <td>RE_ORD_4</td>
      <td>1508</td>
      <td>Tx pending bit clear correctness RCiEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RE_SMU_1</td>
      <td>919</td>
      <td>PCIe Address translation check</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RE_SMU_2</td>
      <td>828</td>
      <td>Check ATS Support Rule: RCiEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>1519</td>
      <td>PCIe Address translation check</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>1520</td>
      <td>ATS Functionality Check</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RE_SMU_3</td>
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
      <td>RE_SMU_4</td>
      <td>1536</td>
      <td>Generate PASID transactions: RCiEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RE_SMU_5</td>
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
      <td>RE_RST_1</td>
      <td>863</td>
      <td>Check Function level reset: RCiEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RE_PWR_1</td>
      <td>870</td>
      <td>Check Power Management rules: RCiEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RE_PWR_2</td>
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
      <td>RE_PWR_3</td>
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
      <td>RE_ACS_1</td>
      <td>815</td>
      <td>Check ACS Cap on p2p support: RCiEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RE_ACS_2</td>
      <td>816</td>
      <td>Check AER Cap on ACS Cap support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RE_ACS_3</td>
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
      <td>RE_REG_1</td>
      <td>848</td>
      <td>Check config header rule: RCEC/RCiEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RE_REG_2</td>
      <td>856</td>
      <td>Check Power Mgmt Cap/Ctrl/Status - RC</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RE_REG_3</td>
      <td>852</td>
      <td>Check Dev Cap &amp; Ctrl Reg rule - RCiEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RE_REC_1</td>
      <td>861</td>
      <td>Check Max payload size support: RCEC</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>853</td>
      <td>Check Dev Cap &amp; Ctrl Reg rule - RCEC</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RE_REC_2</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L6</td>
      <td>S_L6PCI_1 - B_IEP_1</td>
      <td>IE_CFG_1</td>
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
      <td>IE_CFG_2</td>
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
      <td>IE_CFG_3</td>
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
      <td>IE_CFG_4</td>
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
      <td>IE_CRS_1</td>
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
      <td>IE_BAR_1</td>
      <td>829</td>
      <td>Read and write to BAR reg: iEP pair</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>IE_BAR_2</td>
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
      <td>IE_BAR_3</td>
      <td>841</td>
      <td>Check BAR memory space &amp; type: iEP/RP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>IE_INT_1</td>
      <td>855</td>
      <td>Check MSI and MSI-X support: iEP/RP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>IE_ORD_1</td>
      <td>1531</td>
      <td>Arrival order &amp; Gathering Check: iEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>IE_ORD_2</td>
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
      <td>IE_ORD_3</td>
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
      <td>IE_ORD_4</td>
      <td>1538</td>
      <td>Tx pending bit clear correctness iEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>IE_SMU_1</td>
      <td>880</td>
      <td>Check ATS Support Rule: iEP/RP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>IE_SMU_2</td>
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
      <td>IE_SMU_3</td>
      <td>1537</td>
      <td>Generate PASID transactions: iEP EP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>IE_SMU_4</td>
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
      <td>IE_RST_1</td>
      <td>840</td>
      <td>Check Function level reset: iEP_EP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>IE_RST_2</td>
      <td>879</td>
      <td>Check Sec Bus Reset For iEP_RP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>IE_RST_3</td>
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
      <td>IE_PWR_1</td>
      <td>834</td>
      <td>Check Power Management rules: iEP/RP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>IE_PWR_2</td>
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
      <td>IE_PWR_3</td>
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
      <td>IE_ACS_1</td>
      <td>882</td>
      <td>Check ACS Cap on p2p support: iEP EP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>IE_ACS_2</td>
      <td>881</td>
      <td>Check iEP-RootPort P2P Support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>IE_REG_1</td>
      <td>849</td>
      <td>Check config header rule: iEP_EP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>IE_REG_2</td>
      <td>854</td>
      <td>Check Dev Cap &amp; Ctrl Reg rule - iEP_EP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>867</td>
      <td>Check i-EP AtomicOp rule</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>865</td>
      <td>Check OBFF supported rule</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>IE_REG_3</td>
      <td>850</td>
      <td>Check config header rule: iEP_RP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>IE_REG_4</td>
      <td>888</td>
      <td>Slot Cap, Control and Status reg rules</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>866</td>
      <td>Check CTRS and CTDS rule</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>864</td>
      <td>Check ARI forwarding support rule</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>851</td>
      <td>Check PCIe capability rules: iEP_RP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>857</td>
      <td>Check Power Mgmt Cap/Ctrl/Status - iEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>IE_REG_5</td>
      <td>857</td>
      <td>Check Power Mgmt Cap/Ctrl/Status - iEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>IE_REG_6</td>
      <td>892</td>
      <td>Secondary PCIe ECap Check: iEP Pair</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>IE_REG_7</td>
      <td>812</td>
      <td>Datalink feature ECap Check: iEP Pair</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>IE_REG_8</td>
      <td>813</td>
      <td>Phy Layer 16GT/s ECap Check: iEP Pair</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>IE_REG_9</td>
      <td>814</td>
      <td>Lane Margining at Rec ECap Check: iEP</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7PE_01</td>
      <td>S_L7PE_01</td>
      <td>48</td>
      <td>Check Fine Grain Trap Support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7PE_02</td>
      <td>S_L7PE_02</td>
      <td>49</td>
      <td>Check for ECV support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7PE_04</td>
      <td>S_L7PE_04</td>
      <td>51</td>
      <td>Checks ASIMD Int8 matrix multiplc</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7PE_05</td>
      <td>S_L7PE_05</td>
      <td>52</td>
      <td>Check for BFLOAT16 extension</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7PE_06</td>
      <td>S_L7PE_06</td>
      <td>53</td>
      <td>Check PAuth2, FPAC &amp; FPACCOMBINE</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7PE_07</td>
      <td>S_L7PE_07</td>
      <td>54</td>
      <td>Check for SVE Int8 matrix multiplc</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7PE_08</td>
      <td>S_L7PE_08</td>
      <td>35</td>
      <td>Check for data gathering hint</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7PE_09</td>
      <td>S_L7PE_09</td>
      <td>36</td>
      <td>Check WFE Fine tune delay feature</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7RAS_1</td>
      <td>S_L7RAS_1</td>
      <td>1209</td>
      <td>Data abort on Containable err</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7RAS_2</td>
      <td>S_L7RAS_2</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7TME_1</td>
      <td>S_L7TME_1</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7TME_2</td>
      <td>S_L7TME_2</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7TME_3</td>
      <td>S_L7TME_3</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7TME_4</td>
      <td>S_L7TME_4</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7TME_5</td>
      <td>S_L7TME_5</td>
      <td>Not Covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7MP_01</td>
      <td>S_L7MP_01</td>
      <td>1001</td>
      <td>Check for MPAM extension</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7MP_02</td>
      <td>S_L7MP_02</td>
      <td>1008</td>
      <td>Check for MPAM partition IDs</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7MP_03</td>
      <td>S_L7MP_03</td>
      <td>1002</td>
      <td>Check for MPAM LLC CSU Support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>1006</td>
      <td>Check PMG storage by CPOR nodes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>1007</td>
      <td>Check MPAM LLC Requirements</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7MP_04</td>
      <td>S_L7MP_04</td>
      <td>1009</td>
      <td>Check for MPAM LLC CSU Monitor count</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7MP_05</td>
      <td>S_L7MP_05</td>
      <td>1003</td>
      <td>Check for MPAM MBWUs Monitor func</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7MP_07</td>
      <td>S_L7MP_07</td>
      <td>1004</td>
      <td>Check for MBWU counter size</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7MP_08</td>
      <td>S_L7MP_08</td>
      <td>1005</td>
      <td>Check for MPAM MSC address overlap</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7ENT_1</td>
      <td>S_L7ENT_1</td>
      <td>1301</td>
      <td>NIST Statistical Test Suite</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7SM_01</td>
      <td>S_L7SM_01</td>
      <td>322</td>
      <td>Check if all DMA reqs behind SMMU</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7SM_02</td>
      <td>S_L7SM_02</td>
      <td>323</td>
      <td>Check for SMMU/CATU in ETR Path</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7SM_03</td>
      <td>S_L7SM_03</td>
      <td>321</td>
      <td>Check SMMU PMU Extension presence</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7SM_04</td>
      <td>S_L7SM_04</td>
      <td>327</td>
      <td>Check SMMU PMCG has &gt;= 4 counters</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_L7PMU</td>
      <td>PMU_PE_01</td>
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
      <td>PMU_PE_02</td>
      <td>1101</td>
      <td>Check PMU Overflow signal</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PMU_PE_03</td>
      <td>1102</td>
      <td>Check number of PMU counters</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PMU_EV_11</td>
      <td>1103</td>
      <td>Check for multi-threaded PMU ext</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PMU_SPE</td>
      <td>1106</td>
      <td>Check for PMU SPE Requirements</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PMU_EV_01</td>
      <td>Linux PMU app</td>
      <td>IPC events</td>
      <td>No</td>
      <td>No</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PMU_EV_02</td>
      <td>Linux PMU app</td>
      <td>Cache effectiveness</td>
      <td>No</td>
      <td>No</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PMU_EV_03</td>
      <td>Linux PMU app</td>
      <td>TLB effectiveness</td>
      <td>No</td>
      <td>No</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PMU_EV_04</td>
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
      <td>PMU_EV_05</td>
      <td>Linux PMU app</td>
      <td>Cycle accounting</td>
      <td>No</td>
      <td>No</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PMU_EV_06</td>
      <td>Linux PMU app</td>
      <td>top down accounting</td>
      <td>No</td>
      <td>No</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PMU_EV_07</td>
      <td>Linux PMU app</td>
      <td>Workload events</td>
      <td>No</td>
      <td>No</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PMU_EV_08</td>
      <td>Linux PMU app</td>
      <td>Branch predictor effectiveness events</td>
      <td>No</td>
      <td>No</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PMU_EV_09</td>
      <td>Linux PMU app</td>
      <td>BR_RETIRED</td>
      <td>No</td>
      <td>No</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PMU_EV_10</td>
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
      <td>PMU_BM_1</td>
      <td>1107</td>
      <td>Check for memory bandwidth monitors</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PMU_BM_2</td>
      <td>1111</td>
      <td>Check for PCIe bandwidth monitors</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PMU_BM_3</td>
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
      <td>PMU_BM_4</td>
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
      <td>PMU_MEM_1</td>
      <td>1110</td>
      <td>Check for memory latency monitors</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PMU_SYS_1</td>
      <td>1104</td>
      <td>Test Simultaneous 6 traffic measures</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PMU_SYS_2</td>
      <td>1105</td>
      <td>Test Simultaneous 6 traffic measures</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PMU_SYS_5</td>
      <td>1108</td>
      <td>Check System PMU for NUMA systems</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PMU_SYS_6</td>
      <td>1109</td>
      <td>Check multiple type traffic measurement</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>PMU_SYS_7</td>
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
      <td>PMU_SEC_1</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>SYS_RAS</td>
      <td>RAS_01</td>
      <td>1201</td>
      <td>Check Error Counter</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RAS_02</td>
      <td>1202</td>
      <td>Check CFI, DUI, UI Controls</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RAS_03</td>
      <td>1203</td>
      <td>Check FHI in Error Record Group</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RAS_04</td>
      <td>1204</td>
      <td>Check ERI in Error Record Group</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RAS_05</td>
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
      <td>RAS_06</td>
      <td>1205</td>
      <td>Check ERI/FHI Connected to GIC</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RAS_07</td>
      <td>1206</td>
      <td>RAS ERR&lt;n&gt;ADDR.AI bit status check</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RAS_08</td>
      <td>1207</td>
      <td>Check Error Group Status</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RAS_10</td>
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
      <td>RAS_11</td>
      <td>1208</td>
      <td>Software Fault Error Check</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>RAS_12</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>SYS_RAS_1</td>
      <td>SYS_RAS_1</td>
      <td>1210</td>
      <td>Check for patrol scrubbing support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>SYS_RAS_2</td>
      <td>SYS_RAS_2</td>
      <td>1211</td>
      <td>Check Poison Storage &amp; Forwarding</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>1212</td>
      <td>Check Pseudo Fault Injection</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>SYS_RAS_3</td>
      <td>SYS_RAS_3</td>
      <td>1215</td>
      <td>Check Error when Poison unsupported</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_PCIe_01</td>
      <td>S_PCIe_01</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_PCIe_02</td>
      <td>S_PCIe_02</td>
      <td>886</td>
      <td>Check RootPort P&amp;NP Memory Access</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_PCIe_03</td>
      <td>S_PCIe_03</td>
      <td>1522</td>
      <td>PE 2/4/8B writes to PCIe as 2/4/8B</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_PCIe_04</td>
      <td>S_PCIe_04</td>
      <td>1525</td>
      <td>Check 2/4/8 Bytes targeted writes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td>L7</td>
      <td>S_PCIe_05</td>
      <td>S_PCIe_05</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>PCI_ER_01</td>
      <td>PCI_ER_01</td>
      <td>810</td>
      <td>Check RP AER feature</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>PCI_ER_04</td>
      <td>PCI_ER_04</td>
      <td>1523</td>
      <td>Check AER functionality for RPs</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td>L7</td>
      <td>PCI_ER_05</td>
      <td>PCI_ER_05</td>
      <td>807</td>
      <td>Check RP DPC feature</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>L7</td>
      <td>PCI_ER_06</td>
      <td>PCI_ER_06</td>
      <td>1524</td>
      <td>Check DPC funcionality for RPs</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td>FR</td>
      <td>S_L8PE_01</td>
      <td>S_L8PE_01</td>
      <td>58</td>
      <td>Check XS attribute functionality</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>S_L8PE_02</td>
      <td>S_L8PE_02</td>
      <td>59</td>
      <td>Check WFET and WFIT functionality</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>S_L8PE_03</td>
      <td>S_L8PE_03</td>
      <td>60</td>
      <td>Check atomic 64 byte store support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>S_L8PE_04</td>
      <td>S_L8PE_04</td>
      <td>57</td>
      <td>Check for enhanced PAN feature</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>S_L8PE_05</td>
      <td>S_L8PE_05</td>
      <td>64</td>
      <td>Check PMU Version v3.7 or higher</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>S_L8PE_07</td>
      <td>S_L8PE_07</td>
      <td>62</td>
      <td>Check for unsupported PBHA bits</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>S_L8PE_08</td>
      <td>S_L8PE_08</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>S_L8RME_1</td>
      <td>S_L8RME_1</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>S_L8SM_01</td>
      <td>S_L8SM_01</td>
      <td>328</td>
      <td>Check SMMUv3.3 or higher</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>SYS_RAS_4</td>
      <td>SYS_RAS_4</td>
      <td>1213</td>
      <td>Check RAS memory mapped view supp</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>S_L8TI_01</td>
      <td>S_L8TI_01</td>
      <td>406</td>
      <td>Check Minimum Counter Frequency 50MHz</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>S_L8GI_01</td>
      <td>S_L8GI_01</td>
      <td>215</td>
      <td>Check GICv4.1 or higher compliant</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>S_PCIe_06</td>
      <td>S_PCIe_06</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>S_PCIe_07</td>
      <td>S_PCIe_07</td>
      <td>1526</td>
      <td>Check Inbound writes seen in order</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td>FR</td>
      <td>S_PCIe_08</td>
      <td>S_PCIe_08</td>
      <td>1532</td>
      <td>Check ordered writes flush prev writes</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td>FR</td>
      <td>S_PCIe_10</td>
      <td>S_PCIe_10</td>
      <td>1530</td>
      <td>Enable and disable STE.DCP bit</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td>FR</td>
      <td>S_PCIe_11</td>
      <td>S_PCIe_11</td>
      <td>891</td>
      <td>Steering Tag value properties</td>
      <td>No</td>
      <td>Yes</td>
      <td>Yes</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>PCI_ER_07</td>
      <td>PCI_ER_07</td>
      <td>1529</td>
      <td>RAS ERR record for external abort</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td>FR</td>
      <td>PCI_ER_08</td>
      <td>PCI_ER_08</td>
      <td>1528</td>
      <td>RAS ERR record for poisoned data</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td>FR</td>
      <td>PCI_ER_09</td>
      <td>PCI_ER_09</td>
      <td>890</td>
      <td>Check RP Extensions for DPC</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>PCI_ER_10</td>
      <td>PCI_ER_10</td>
      <td>1527</td>
      <td>DPC trig when RP-PIO unimplemented</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td>Exerciser VIP required</td>
    </tr>
    <tr>
      <td>FR</td>
      <td>GPU_01</td>
      <td>GPU_01</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>GPU_02</td>
      <td>GPU_02</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>GPU_03</td>
      <td>GPU_03</td>
      <td>893</td>
      <td>Switches must support ACS if P2P</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>GPU_04</td>
      <td>GPU_04</td>
      <td>889</td>
      <td>Check ATS support for RC</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td></td>
      <td>324</td>
      <td>Check ATS Support for SMMU</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>S_L8CXL_1</td>
      <td>S_L8CXL_1</td>
      <td>Not covered</td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>S_L8PE_06</td>
      <td>S_L8PE_06</td>
      <td>61</td>
      <td>Check for FEAT_BRBE support</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td>FR</td>
      <td>S_L8SHD_1</td>
      <td>ETE_01</td>
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
      <td>ETE_02</td>
      <td>1401</td>
      <td>Check for FEAT_ETE</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ETE_03</td>
      <td>1402</td>
      <td>Check trace unit ETE supports</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ETE_04</td>
      <td>1403</td>
      <td>Check ETE Same Trace Timestamp Source</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ETE_05</td>
      <td>1404</td>
      <td>Check Trace Same Timestamp Source</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ETE_06</td>
      <td>1409</td>
      <td>Check Concurrent Trace Generation</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ETE_07</td>
      <td>1405</td>
      <td>Check for FEAT_TRBE</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ETE_08</td>
      <td>1406</td>
      <td>Check trace buffers flag updates</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ETE_09</td>
      <td>1407</td>
      <td>Check TRBE trace buffers alignment</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td></td>
      <td>ETE_10</td>
      <td>1408</td>
      <td>Check GICC TRBE Interrupt field</td>
      <td>Yes</td>
      <td>Yes</td>
      <td>No</td>
      <td></td>
    </tr>
  </tbody>
</table>

## Latest Checklist Changes
