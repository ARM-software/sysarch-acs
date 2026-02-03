# Platform Override Configuration Guide — PCIe ECAM and PCIe Device Hierarchy

This guide explains how to populate **platform override macros** for:
1) **PCIe ECAM windows** (configuration-space access)
2) **PCIe device hierarchy entries** (a static inventory used by the test/validation stack)

The examples in this guide mirror the macro style based on RD N2, but the intent is to help you fill these fields for a **new platform**.

---

## 1) What you are configuring

### 1.1 ECAM windows (config space discovery)
An **ECAM window** defines where PCIe configuration space is memory-mapped for a given:
- **Segment Group** (PCI Segment)
- **Bus range** (Start Bus .. End Bus)
- **Base address** (ECAM base for bus 0 of that window)

This is what allows software (OS / firmware / ACS) to do:
- `cfg_read(seg,bus,dev,func,offset)` → MMIO at ECAM base + B:D:F:offset

### 1.2 PCIe device hierarchy table (static inventory)
Many validation frameworks keep a **platform override list** of PCIe devices to:
- sanity-check enumeration
- apply feature expectations (DMA, coherency, ATC, behind SMMU, P2P capability)
- drive directed tests (e.g., exerciser endpoints, bridges)


---

## 2) ECAM configuration

Two kinds of ECAM-related macros:
- A set of “ECAM window” fields (`PCIE_ECAM_BASE_ADDR_n`, segment, start/end bus)
- A set of “ECAM0 host-bridge” resource fields (BAR windows, bus range, etc.)

Treat them as:
- **(A)** Config-space mapping windows (what address range maps to config space)
- **(B)** Host-bridge resource windows (what address ranges are assigned to PCIe MMIO regions)

### 2.1 ECAM window macros (config-space mapping)

Example:

```c
#define PLATFORM_OVERRIDE_PCIE_ECAM_BASE_ADDR_0   0x1010000000
#define PLATFORM_OVERRIDE_PCIE_SEGMENT_GRP_NUM_0  0x0
#define PLATFORM_OVERRIDE_PCIE_START_BUS_NUM_0    0x0
#define PLATFORM_OVERRIDE_PCIE_END_BUS_NUM_0      0x8
```

#### How to fill these for a new platform

**(1) Determine segments (PCI segment groups)**
- If you have a single PCIe domain, segment is usually `0`.
- If you have multiple independent PCIe domains (common in server SoCs), you may have segments 0..N-1.

**(2) Determine bus ranges per segment**
- Each root complex (or host bridge) typically owns a bus range.
- Bus numbering scheme is platform/firmware dependent.
- For static platforms, you often allocate **non-overlapping bus ranges** per host bridge.

**(3) Determine ECAM base address**
From your system memory map / integration doc:
- ECAM base is where config space for **start_bus** begins.
- For standard ECAM mapping, each bus consumes **1 MiB**:
  - bus stride = 1 MB
  - dev stride = 32 KB
  - func stride = 4 KB

So, ECAM size required for a bus range is:

```
ecam_size = (end_bus - start_bus + 1) * 1 MiB
```

**(4) Multiple windows with the same base address**
In the example:
- Window 0 covers buses 0..8
- Window 1 covers buses 0x40..0x7F
- Both have the same base address

This can occur if:
- your platform uses **non-contiguous bus numbering** but maps them into one ECAM aperture, or
- the override layer expects “logical windows” for test partitioning.

For a new platform, prefer a clean mapping:
- Each window base points to the correct bus0-of-window mapping.
- Use separate bases if the hardware maps them separately.

> Practical rule: if bus numbers are non-contiguous, only do this “same base, different bus ranges” if your ECAM mapping logic explicitly supports it.

---

### 2.2 Host bridge (HB) and resource window macros

Example (ECAM0 host bridge resource config):

```c
#define PLATFORM_OVERRIDE_PCIE_ECAM0_HB_COUNT       1
#define PLATFORM_OVERRIDE_PCIE_ECAM0_SEG_NUM        0x0
#define PLATFORM_OVERRIDE_PCIE_ECAM0_START_BUS_NUM  0x0
#define PLATFORM_OVERRIDE_PCIE_ECAM0_END_BUS_NUM    0x8

#define PLATFORM_OVERRIDE_PCIE_ECAM0_EP_BAR64       0x4000100000
#define PLATFORM_OVERRIDE_PCIE_ECAM0_RP_BAR64       0x4000000000
#define PLATFORM_OVERRIDE_PCIE_ECAM0_EP_NPBAR32     0x60000000
#define PLATFORM_OVERRIDE_PCIE_ECAM0_EP_PBAR32      0x60600000
#define PLATFORM_OVERRIDE_PCIE_ECAM0_RP_BAR32       0x60850000
```

These are not ECAM addresses. They are **MMIO apertures** reserved for PCIe:
- **EP_BAR64**: 64-bit prefetchable region for Endpoints (MMIO64, prefetch)
- **RP_BAR64**: 64-bit region for Root Ports
- **EP_NPBAR32**: 32-bit non-prefetchable endpoint region (MMIO32 NP)
- **EP_PBAR32**: 32-bit prefetchable endpoint region (MMIO32 P)
- **RP_BAR32**: 32-bit region used for root-port internal BARs / RP regs

#### How to fill these for a new platform

1) From your SoC memory map / firmware resource map, identify the **PCIe MMIO apertures**:
- A 64-bit prefetchable window (preferred for high BAR devices)
- Optionally a 64-bit non-prefetchable window (less common)
- 32-bit MMIO windows if your platform supports/needs them

2) Ensure the windows do not overlap with:
- DRAM ranges
- device MMIO ranges (UART, GIC, SMMU, etc.)
- reserved regions

3) Ensure the windows are aligned appropriately (typical):
- 64-bit windows aligned to their size (power-of-two is ideal)
- 32-bit windows aligned to at least 1 MiB or 64 KiB depending on platform conventions

4) Match the firmware/OS assignment strategy:
- If firmware assigns resources dynamically, your overrides should reflect the **reserved apertures** the firmware uses.
- If you are in a bare-metal test environment, you must ensure the enumerator uses these windows consistently.

---

### 2.3 Bare-metal enumeration limits

Example:

```c
#define PLATFORM_BM_OVERRIDE_PCIE_MAX_BUS      0x9
#define PLATFORM_BM_OVERRIDE_PCIE_MAX_DEV      32
#define PLATFORM_BM_OVERRIDE_PCIE_MAX_FUNC     8
```

Guidance:
- `MAX_BUS` should be at least `(max_end_bus + 1)` across all windows used by BM tests.
- `MAX_DEV` is typically 32 (PCI spec max devices per bus).
- `MAX_FUNC` is typically 8 (PCI spec max functions per device).

If your platform uses bus numbers > 0xFF, that’s not standard PCI; most platforms stay within 0..255.

---

## 3) PCIe device hierarchy table configuration

### 3.1 What each entry represents
Each `PLATFORM_PCIE_DEVn_*` entry describes one enumerated PCI function:
- location: Segment, Bus, Device, Function
- identity: Vendor ID, Device ID, Class Code
- behavior expectations: DMA, coherency, P2P, behind SMMU, ATC, 64-bit DMA

Example:

```c
#define PLATFORM_PCIE_DEV5_CLASSCODE     0xED000000
#define PLATFORM_PCIE_DEV5_VENDOR_ID     0x13B5
#define PLATFORM_PCIE_DEV5_DEV_ID        0xED01
#define PLATFORM_PCIE_DEV5_BUS_NUM       2
#define PLATFORM_PCIE_DEV5_DEV_NUM       0
#define PLATFORM_PCIE_DEV5_FUNC_NUM      0
#define PLATFORM_PCIE_DEV5_SEG_NUM       0
#define PLATFORM_PCIE_DEV5_DMA_SUPPORT   1
#define PLATFORM_PCIE_DEV5_DMA_COHERENT  0
#define PLATFORM_PCIE_DEV5_P2P_SUPPORT   0
#define PLATFORM_PCIE_DEV5_DMA_64BIT     0
#define PLATFORM_PCIE_DEV5_BEHIND_SMMU   1
#define PLATFORM_PCIE_DEV5_ATC_SUPPORT   0
```

### 3.2 How to build the device list for a new platform

Set:
```c
#define PLATFORM_PCIE_NUM_ENTRIES <count>
```

#### Fill capability expectations
These flags are **expectations** used by tests. They must match the platform behavior.

**DMA_SUPPORT**
- `1` if device can perform DMA (almost all endpoints do)
- `0` for bridges / ports that don’t initiate DMA

**DMA_COHERENT**
- `1` if the device is cache-coherent with CPU (e.g., CXL.cache capable devices, or coherent PCIe endpoints on a coherent interconnect)
- `0` otherwise

**DMA_64BIT**
- `1` if device supports 64-bit DMA addressing (common for modern devices)
- `0` if restricted to 32-bit addressing

**BEHIND_SMMU**
- `1` if the device’s transactions are translated/managed by an SMMU (typical on Arm servers)
- `0` if it bypasses SMMU

How to decide:
- Use IORT/SMMU topology: if the requester ID maps through an SMMU node, it’s “behind SMMU”.
- Some devices (e.g., host bridge internal functions) may bypass translation.

**ATC_SUPPORT**
- `1` if device supports Address Translation Cache (ATS/ATC capability) and platform enables it
- `0` otherwise
Notes:
- ATC/ATS typically depends on both endpoint capability and SMMU/RC support.
- If you mark ATC supported but firmware doesn’t enable ATS, tests may fail.

**P2P_SUPPORT**
- `1` if the device supports/permits peer-to-peer transactions in your platform configuration
- `0` otherwise
This is platform-policy dependent. Many validation setups set P2P not supported unless explicitly enabled.

You also have a global:
```c
#define PLATFORM_PCIE_P2P_NOT_SUPPORTED  1
```
Treat this as “platform-level policy”: if set, you likely want `*_P2P_SUPPORT` = 0 for all entries except explicit exceptions.

---

## 4) Common pitfalls and how to avoid them

### 4.1 ECAM base doesn’t match bus numbering
Symptoms:
- config reads return 0xFFFF_FFFF
- enumeration only sees bus 0 or fails beyond a bus

Fix:
- Validate ECAM base + bus stride mapping:
  - bus X config space must be at `ecam_base + (X - start_bus) * 1MiB`

### 4.2 End bus too small
Symptoms:
- downstream devices not discovered
Fix:
- Ensure `END_BUS_NUM` covers the full topology, including bridges/switches.

### 4.3 Resource windows overlap or are too small
Symptoms:
- BAR assignment failures
- devices disable memory space enable (MSE), or ACS exerciser failures
Fix:
- Increase EP/RP MMIO apertures and ensure alignment / non-overlap.

### 4.4 Wrong “behind SMMU” / ATS expectations
Symptoms:
- SMMU tests fail, DMA faults, PRI/ATS tests fail
Fix:
- Cross-check with IORT stream-ID mapping and SMMU configuration.
- Only enable ATC if both endpoint and platform enable it.

---

## 5) Minimal templates you can copy/paste

### 5.1 ECAM window template
```c
#define PLATFORM_OVERRIDE_PCIE_ECAM_BASE_ADDR_<n>   <ecam_base>
#define PLATFORM_OVERRIDE_PCIE_SEGMENT_GRP_NUM_<n>  <segment>
#define PLATFORM_OVERRIDE_PCIE_START_BUS_NUM_<n>    <start_bus>
#define PLATFORM_OVERRIDE_PCIE_END_BUS_NUM_<n>      <end_bus>
```

### 5.2 Host bridge resource windows template
```c
#define PLATFORM_OVERRIDE_PCIE_ECAM<n>_HB_COUNT       <hb_count>
#define PLATFORM_OVERRIDE_PCIE_ECAM<n>_SEG_NUM        <segment>
#define PLATFORM_OVERRIDE_PCIE_ECAM<n>_START_BUS_NUM  <start_bus>
#define PLATFORM_OVERRIDE_PCIE_ECAM<n>_END_BUS_NUM    <end_bus>

#define PLATFORM_OVERRIDE_PCIE_ECAM<n>_EP_BAR64       <mmio64_ep_pref_base>
#define PLATFORM_OVERRIDE_PCIE_ECAM<n>_RP_BAR64       <mmio64_rp_base>
#define PLATFORM_OVERRIDE_PCIE_ECAM<n>_EP_NPBAR32     <mmio32_ep_np_base>
#define PLATFORM_OVERRIDE_PCIE_ECAM<n>_EP_PBAR32      <mmio32_ep_p_base>
#define PLATFORM_OVERRIDE_PCIE_ECAM<n>_RP_BAR32       <mmio32_rp_base>
```

### 5.3 PCIe device entry template
```c
#define PLATFORM_PCIE_DEV<n>_CLASSCODE     <classcode>
#define PLATFORM_PCIE_DEV<n>_VENDOR_ID     <vendor>
#define PLATFORM_PCIE_DEV<n>_DEV_ID        <device>
#define PLATFORM_PCIE_DEV<n>_BUS_NUM       <bus>
#define PLATFORM_PCIE_DEV<n>_DEV_NUM       <dev>
#define PLATFORM_PCIE_DEV<n>_FUNC_NUM      <func>
#define PLATFORM_PCIE_DEV<n>_SEG_NUM       <segment>

#define PLATFORM_PCIE_DEV<n>_DMA_SUPPORT   <0_or_1>
#define PLATFORM_PCIE_DEV<n>_DMA_COHERENT  <0_or_1>
#define PLATFORM_PCIE_DEV<n>_DMA_64BIT     <0_or_1>
#define PLATFORM_PCIE_DEV<n>_BEHIND_SMMU   <0_or_1>
#define PLATFORM_PCIE_DEV<n>_ATC_SUPPORT   <0_or_1>
#define PLATFORM_PCIE_DEV<n>_P2P_SUPPORT   <0_or_1>
```

---
