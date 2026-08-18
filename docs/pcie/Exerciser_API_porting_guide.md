# Exerciser API Porting Guide
This document gives details of the various PCIe capabilities that exerciser device supports and how the exerciser is supposed to behave. It targets the Silicon partners and/or EDA vendors who are porting Exerciser API's on their platform.

## Introduction to PCIe Exerciser Endpoint Device
PCIe Exerciser is a client device wrapped up by PCIe Endpoint. This device was created to generate custom stimuli to meet BSA (Base System Architecture) requirements for various PCIe capability validation tests.

### Generating DMA
- Before triggering DMA all the required DMA attribute fields like DMA bus address, DMA length, exerciser instance fields should be correctly set <br/>
**pal_exerciser_set_param(Type, Value1, Value2, Bdf)** <br/>
Type - DMA_ATTRIBUTES<br/>
Value1 - Buffer containing the data or the Buffer where the data to be copied<br/>
Value2 - Size of the data<br/>
BDF - BDF of the exerciser<br/>
- Trigger the DMA to/from the buffer<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - START_DMA<br/>
Param - EDMA_TO_DEVICE or EDMA_FROM_DEVICE<br/>
Bdf - BDF of the exerciser<br/>
<br/>

### Generating DMA with PASID TLP Prefixes
- Program exerciser to start sending TLPs with PASID TLP Prefixes. This includes setting PASID Enable bit in exerciser PASID Control register and the implementation specific PASID Enable bit of the Root Port.<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - PASID_TLP_START<br/>
Param - Substream ID<br/>
Bdf - BDF of the exerciser<br/>
**pal_exerciser_set_param(Type, Value1, Value2, Bdf)**<br/>
Type - DMA_ATTRIBUTES<br/>
Value1 - Buffer containing the data or the Buffer where the data to be copied<br/>
Value2 - Size of the data<br/>
BDF - BDF of the exerciser<br/>
- Trigger the DMA to/from the buffer<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - START_DMA<br/>
Param - EDMA_TO_DEVICE or EDMA_FROM_DEVICE<br/>
Bdf - BDF of the exerciser<br/>
- Disable exerciser to stop sending TLPs with PASID TLP Prefixes.<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - PASID_TLP_STOP<br/>
Param - Substream ID<br/>
Bdf - BDF of the exerciser<br/>
<br/>

### Generating DMA with No Snoop TLP
- Program the exerciser hierarchy to control the No Snoop attribute on transmitted and received TLPs.<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>

| Ops | Param | Description |
| --- | --- | --- |
| TXN_NO_SNOOP_ENABLE | Null | Enable the No Snoop attribute |
| TXN_NO_SNOOP_DISABLE | Null | Disable the No Snoop attribute |

Bdf - BDF of the exerciser<br/>
**pal_exerciser_set_param(Type, Value1, Value2, Bdf)**<br/>
Type - DMA_ATTRIBUTES<br/>
Value1 - Buffer containing the data or the Buffer where the data to be copied<br/>
Value2 - Size of the data<br/>
BDF - BDF of the exerciser<br/>
- Trigger the DMA to/from the buffer<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - START_DMA<br/>
Param - EDMA_TO_DEVICE or EDMA_FROM_DEVICE<br/>
Bdf - BDF of the exerciser<br/>
<br/>

### ATS Request
- Before starting an ATS request, untranslated input address for ATSRequest must be written onto Bus Address Register<br/>
**pal_exerciser_set_param(Type, Value1, Value2, Bdf)**<br/>
Type - DMA_ATTRIBUTES<br/>
Value1 -  Untranslated input address<br/>
Value2 - Size<br/>
BDF - BDF of the exerciser<br/>
- Send an ATS Translation Request for the VA<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>

| Ops | Param | Description |
| --- | --- | --- |
| ATS_TXN_REQ | VA | Send an ATS translation request for the virtual address |
| ATS_TXN_CLEAR | Null | Clear the ATS transaction state |

Bdf - BDF of the exerciser<br/>
- Get ATS Translation Response<br/>
**pal_exerciser_get_param(Type, Value1, Value2, Bdf)**<br/>
Type - ATS_RES_ATTRIBUTES<br/>
Value1 - Buffer to store translated address<br/>
Value2 - Null<br/>
Bdf -  BDF of the exerciser<br/>
<br/>

### Generating DMA with Address Translated(AT)
- Configure Exerciser to issue subsequent DMA transactions with AT(Address Translated) bit Set<br/>
**pal_exerciser_set_param(Type, Value1, Value2, Bdf)**<br/>
Type - CFG_TXN_ATTRIBUTES<br/>
Value1 - TXN_ADDR_TYPE<br/>
Value2 - AT_TRANSLATED<br/>
BDF - BDF of the exerciser<br/>
**pal_exerciser_set_param(Type, Value1, Value2, Bdf)**<br/>
Type - DMA_ATTRIBUTES<br/>
Value1 - Buffer containing the data or the Buffer where the data to be copied<br/>
Value2 - Size of the data<br/>
BDF - BDF of the exerciser<br/>
- Trigger the DMA to/from the buffer<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - START_DMA<br/>
Param - EDMA_TO_DEVICE or EDMA_FROM_DEVICE<br/>
Bdf - BDF of the exerciser<br/>
<br/>

### Trigerring MSI
- Trigger the interrupt for this Exerciser instance<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - GENERATE_MSI<br/>
Param - MSI index<br/>
Bdf - BDF of the exerciser<br/>
<br/>

### Trigerring Legacy Interrupts
-  Clear any pending interrupts<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - CLEAR_INTR<br/>
Param - Legacy interrupt IRQ<br/>
Bdf - BDF of the exerciser<br/>
- Trigger the interrupt for this Exerciser instance<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - GENERATE_L_INTR<br/>
Param - Legacy interrupt IRQ<br/>
Bdf - BDF of the exerciser<br/>
<br/>

### Transaction Monitoring
- Transaction monitoring capabilities in the exerciser provides the ability to record the incoming transactions, for both config and memory transactions. This includes,<br/>
	-   config read and write transactions serviced in PCIe endpoints.<br/>
	-   memory transactions serviced in PCIe endpoint BARs.<br/>
	-   memory transactions which are forwarded from PCIe endpoint to device(like exerciser).<br/>
- To start transaction monitoring<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - START_TXN_MONITOR<br/>
Param - CFG_READ<br/>
Bdf - BDF of the exerciser<br/>
- After the transactions are performed, stop the transaction monitoring<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - STOP_TXN_MONITOR<br/>
Param - CFG_READ<br/>
Bdf - BDF of the exerciser<br/>
- Read the transaction trace<br/>
**pal_exerciser_get_param(Type, Value1, Value2, Bdf)**<br/>

| Type | Description | Value1 | Value2 |
| --- | --- | --- | --- |
| CFG_TXN_ATTRIBUTES | Transaction attributes (configuration or memory transaction) | Returns the transaction attributes | Null |
| TRANSACTION_TYPE | Transaction type (Read or Write) | Specifies the transaction index | Returns the transaction type |
| ADDRESS_ATTRIBUTES | Configuration or memory address | Returns the transaction address | Specifies the transaction index |
| DATA_ATTRIBUTES | Transaction data read or written | Returns the transaction data | Specifies the transaction index |
| CLEAR_TXN | Clear the recorded transaction trace | Null | Null |

Bdf -  BDF of the exerciser<br/>
<br/>

### Error Injection
Error injection capability enables user to inject error in a PCIe endpoint.<br/>
- The user can inject an error at that endpoint with the error configured using the error_code field. The error_codes are defined in Error Codes section. This bit is cleared once the error has been injected.
**pal_exerciser_set_param(Type, Value1, Value2, Bdf)**<br/>
Type - ERROR_INJECT_TYPE<br/>
Value1 - Type of error to inject(Refer error code section)<br/>
Value2 - Fatal-error flag: 0 for a non-fatal error, 1 for a fatal error<br/>
Bdf -  BDF of the exerciser<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - INJECT_ERROR<br/>
Param - Type of error to inject(Refer error code section)<br/>
Bdf -  BDF of the exerciser<br/>
<br/>

### PCIe RAS Error Recording

The exerciser RAS flow verifies that a PCIe error is reported to, and recorded by, a RAS-compliant node. The platform must support the common setup below and the error-generation paths applicable to it.

#### Common RAS Setup

- Enable error reporting for the exerciser and its upstream Root Port.
- Locate the RAS-compliant node that records PCIe errors.<br/>
**pal_exerciser_get_pcie_ras_compliant_err_node(Bdf, RpBdf)**<br/>
Bdf - BDF of the exerciser<br/>
RpBdf - BDF of its upstream Root Port<br/>
Return - RAS-node identifier, or NOT_IMPLEMENTED when no suitable node is available<br/>
- Enable error detection and logging in the selected RAS node.<br/>
**pal_exerciser_set_param(Type, Value1, Value2, Bdf)**<br/>
Type - ENABLE_RAS_CTRL<br/>
Value1 - RAS-node identifier<br/>
Value2 - BDF of the upstream Root Port<br/>
Bdf - BDF of the exerciser<br/>
- Obtain the exerciser BAR0 address before changing memory-space decoding.<br/>
**pal_exerciser_get_data(Type, Data, Bdf, Ecam)**<br/>
Type - EXERCISER_DATA_BAR0_SPACE<br/>
Data - Output structure receiving the BAR0 base address and attributes<br/>
Bdf - BDF of the exerciser<br/>
Ecam - Base address of the PCIe ECAM region containing Bdf<br/>
- Read the RAS status after generating the error.<br/>
**pal_exerciser_get_ras_status(RasNode, Bdf, RpBdf)**<br/>
RasNode - RAS-node identifier returned above<br/>
Bdf - BDF of the exerciser<br/>
RpBdf - BDF of its upstream Root Port<br/>
Return - RAS error-status register value, or NOT_IMPLEMENTED when it cannot be read<br/>
- For both paths, validate that SERR is 0x19, UE and ER are set, UET is 0x3, and DE is clear.

#### Poisoned-Data Path

- Enable memory-space access for the exerciser and its upstream Root Port.
- Enable poison mode in the exerciser.<br/>
**pal_exerciser_set_param(Type, Value1, Value2, Bdf)**<br/>
Type - ENABLE_POISON_MODE<br/>
Value1 - Null<br/>
Value2 - Null<br/>
Bdf - BDF of the exerciser<br/>
- Check whether the platform forwards poisoned data.<br/>
**pal_exerciser_check_poison_data_forwarding_support()**<br/>
Return - 1 if poison forwarding is supported; otherwise 0<br/>
- Read the saved BAR0 address. The read must return PCIE_UNKNOWN_RESPONSE (all ones) and send a poisoned transaction upstream.
- In addition to the common RAS status checks, validate that PN is set.
- Disable poison mode on every completion and error path.<br/>
**pal_exerciser_set_param(Type, Value1, Value2, Bdf)**<br/>
Type - DISABLE_POISON_MODE<br/>
Value1 - Null<br/>
Value2 - Null<br/>
Bdf - BDF of the exerciser<br/>

#### External-Abort Path

- Install handlers for synchronous exceptions and SError before accessing the BAR.
- Configure the platform so that the test BAR access produces an external abort.<br/>
**pal_exerciser_set_bar_response(Bdf)**<br/>
Bdf - BDF of the exerciser<br/>
Return - 0 if configured; otherwise NOT_IMPLEMENTED or another failure status<br/>
- Disable memory-space access for the exerciser after saving its BAR0 address. This forces BAR address-decode failure.
- Read the saved BAR0 address and verify that a synchronous exception or SError is received.
- In addition to the common RAS status checks, validate that PN is clear because this path does not inject poisoned data.
- Re-enable memory-space access on every completion and error path.
<br/>

### CXL Exerciser Flows

The CXL tests use PCIe exerciser operations together with CXL component discovery and platform address configuration. Only Type-1 and Type-2 CXL devices are applicable.

#### ATS Translation and DMA over a CXL Path

- Find the Root Complex and exerciser instance associated with the CXL device BDF.
- Require the ATS extended capability and enable ATS caching in the device ATS Control register.
- Allocate input and output buffers, enable the associated SMMU, and create a stage-1 mapping from an IOVA to the input buffer.
- Use the ATS Request flow described above for the IOVA and verify that the returned translated address equals the mapped physical address.
- Configure CFG_TXN_ATTRIBUTES with TXN_ADDR_TYPE and AT_TRANSLATED.
- Perform EDMA_TO_DEVICE using the translated physical address, followed by EDMA_FROM_DEVICE to the output IOVA.
- Compare the input and output data.
- On every exit path, remove the SMMU mapping, disable the SMMU and ATS caching, clear the exerciser ATS state, and free the buffers.

#### CXL.cache Coherency

- Require a Type-1 or Type-2 device that advertises CXL.cache support and locate its exerciser instance.
- Enable coherent CXL.cache transactions.<br/>
**pal_exerciser_set_param(Type, Value1, Value2, Bdf)**<br/>
Type - ENABLE_CACHE_TXN<br/>
Value1 - 1 to enable CXL.cache reads<br/>
Value2 - 1 to enable CXL.cache writes<br/>
Bdf - BDF of the exerciser<br/>
- Allocate cacheable memory for the target and staging buffers and disable No Snoop transactions.
- Verify the CPU-to-device coherent path: write a pattern from the CPU without cleaning the cache line, transfer it through the exerciser, and confirm that the device observed the pattern.
- Verify the device-to-CPU coherent path: transfer a second pattern from the exerciser into the cached target and confirm that the CPU observes it without an explicit invalidate.
- Free the cacheable allocation on every exit path.

#### CXL.mem Write

- Require a CXL exerciser and identify the CXL host bridge associated with its upstream Root Port.
- Obtain the host bridge CFMWS window and map an HDM address within that window.
- Enable poison mode so that the exerciser generates poison data for the CXL.mem write.<br/>
**pal_exerciser_set_param(Type, Value1, Value2, Bdf)**<br/>
Type - ENABLE_POISON_MODE<br/>
Value1 - Null<br/>
Value2 - Null<br/>
Bdf - BDF of the exerciser<br/>
- Write test data to the mapped HDM address and verify that the CXL.mem write does not raise an exception.
- Disable poison mode on every completion and error path using DISABLE_POISON_MODE.

#### CXL PMReq/PMRes VDM

- Require a CXL exerciser whose upstream port is a CXL Root Port and whose PCI PM capability is accessible.
- Read the initial device power state; skip devices already in D3hot because no transition can be validated.
- Request a transition by generating a PMReq VDM.<br/>
**pal_exerciser_set_param(Type, Value1, Value2, Bdf)**<br/>
Type - GENERATE_PMREQ_VDM<br/>
Value1 - PM_STATE_D3HOT<br/>
Value2 - BDF of the upstream CXL Root Port<br/>
Bdf - BDF of the exerciser<br/>
- Allow time for PMReq/PMRes handling, then verify that the device transitioned to D3hot and that its final state differs from its initial state.

#### CXL MEFN VDM

- Require platform firmware-first handling of memory-error notifications.<br/>
**pal_exerciser_check_firmware_handle_support()**<br/>
Return - ACS_STATUS_PASS when supported, ACS_STATUS_PAL_NOT_IMPLEMENTED when the check is unavailable, or another status when unsupported<br/>
- Install the synchronous-exception handler before generating the notification.
- Require a CXL exerciser whose upstream port is a CXL Root Port.
- Generate a Memory Error Fault Notification VDM.<br/>
**pal_exerciser_set_param(Type, Value1, Value2, Bdf)**<br/>
Type - GENERATE_MEFN_VDM<br/>
Value1 - Null<br/>
Value2 - BDF of the CXL exerciser<br/>
Bdf - BDF of the exerciser<br/>
- Allow time for firmware handling and verify that the expected exception is observed.
<br/>

## Exerciser PAL API Reference

The following APIs form the complete platform interface used by the exerciser validation code. Platform implementations must return the status values defined by the PAL interface and must not report success for unsupported operations.

### Common Exerciser APIs

#### Identify an Exerciser

**pal_is_bdf_exerciser(Bdf)**<br/>
Bdf - PCI segment/bus/device/function identifier to inspect<br/>
Return - 1 if Bdf identifies an exerciser; otherwise 0<br/>

The implementation must use platform-specific discovery information, such as the exerciser Vendor ID and Device ID, to distinguish exercisers from other PCIe functions.

#### Get Exerciser State

**pal_exerciser_get_state(State, Bdf)**<br/>
State - Output pointer receiving EXERCISER_RESET, EXERCISER_ON, EXERCISER_OFF, or EXERCISER_ERROR<br/>
Bdf - BDF of the exerciser<br/>
Return - 0 on success; otherwise a failure or not-implemented status<br/>

The implementation must report the current operational state of the stimulus-generation hardware.

#### Get Exerciser Address-Space Data

**pal_exerciser_get_data(Type, Data, Bdf, Ecam)**<br/>
Type - EXERCISER_DATA_CFG_SPACE, EXERCISER_DATA_BAR0_SPACE, or EXERCISER_DATA_MMIO_SPACE<br/>
Data - Output structure receiving the requested configuration-space or BAR information<br/>
Bdf - BDF of the exerciser<br/>
Ecam - Base address of the PCIe ECAM region containing Bdf<br/>
Return - 0 on success; otherwise a failure or not-implemented status<br/>

For configuration-space requests, the implementation must return the requested register offsets, access attributes, and values. For BAR requests, it must return the base address and memory attributes of the selected region.

### SBSA Exerciser APIs

#### Disable Root Port PIO Error Registers

**pal_exerciser_disable_rp_pio_register(Bdf)**<br/>
Bdf - BDF of the Root Port whose RP PIO error-register support must be disabled<br/>
Return - None<br/>

The implementation must perform any platform-specific operation required to prevent Root Port PIO error registers from affecting the test sequence. If the platform has no such control, the function may be empty.

<br/>

## Error Codes
   |Error Name                                |Error Code|
   |------------------------------------------|----------|
   |Correctable Receiver Error                |0x00      |
   |Correctable Bad TLP                       |0x01      |
   |Correctable Bad DLLP                      |0x02      |
   |Correctable Replay Num Rollover           |0x03      |
   |Correctable Replay Timer Timeout          |0x04      |
   |Correctable Advisory Non-Fatal Error      |0x05      |
   |Correctable Internal Error                |0x06      |
   |Correctable Header Log OverFlow           |0x07      |
   |Uncorrectable Data Link Error             |0x08      |
   |Uncorrectable Surprise Down Error         |0x09      |
   |Uncorrectable Poisoned TLP Received       |0x0A      |
   |Uncorrectable Flow Control Error          |0x0B      |
   |Uncorrectable Completion Timeout          |0x0C      |
   |Uncorrectable Completer Abort             |0x0D      |
   |Uncorrectable Unexpected Completion       |0x0E      |
   |Uncorrectable Receiver Overflow           |0x0F      |
   |Uncorrectable Malformed TLP               |0x10      |
   |Uncorrectable ECRC Error                  |0x11      |
   |Uncorrectable Unsupported Request         |0x12      |
   |Uncorrectable ACS Violation               |0x13      |
   |Uncorrectable Internal Error              |0x14      |
   |Uncorrectable MultiCast Blocked TLP       |0x15      |
   |Uncorrectable Atomic Op Egress Blocked    |0x16      |
   |Uncorrectable TLP Prefix Blocked Egress   |0x17      |
   |Uncorrectable Poisoned TLP Egress Blocked |0x18      |
   |Invalid configuration                     |0x19      |
<br/>
<br/>

## License
Arm BSA ACS is distributed under Apache v2.0 License.

--------------

*Copyright (c) 2022-2026, Arm Limited and Contributors. All rights reserved.*

