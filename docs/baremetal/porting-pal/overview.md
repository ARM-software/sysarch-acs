# System Bring-up and ACS Debug – Consolidated Guide

> **Purpose**
> This guide provides a single, structured reference for Arm-based platform bring-up and
> BSA/SBSA/RME ACS debugging, covering PCIe, IORT/IOMMU, MMIO, interrupts, and common failure patterns.

## Table of Contents
- Overview
- Platform Bring-up
- PCIe Enumeration
- IORT and IOMMU
- Memory Map and MMIO
- Interrupts, Timers, and UART
- Troubleshooting and Debugging


## Overview

### What PAL is

PAL (Platform Abstraction Layer) is where sysarch-acs obtains **platform-specific information** and implements platform hooks that cannot be discovered automatically in a baremetal environment.

Typical PAL responsibilities:
- memory map and MMIO regions
- console UART
- timer/watchdog properties
- PCIe config access (ECAM) and enumerator limits
- IOMMU/SMMU topology (IORT) and ID mapping helpers
- optional: RAS/PMU topology if required by the suite

### What “platform overrides” are

Many baremetal flows use a `platform_override.h`-style header (or generator input) that provides values used to build ACPI-like tables or to drive tests directly.

In this repo, the override fields are documented under:
- [Platform override guides](platform-override-guides/README.md)

### Where to implement platform support

Common patterns:
- `pal/baremetal/target/<platform>/...`
- `pal/platform_override.h` or generated override headers
- a platform selection CMake option or build-time macro

> Keep platform code minimal. Prefer configuration in overrides + shared code in PAL common helpers.


## Platform Bringup

This section is the **recommended workflow** for enabling a *new* platform for baremetal execution.

### Step 1 — Boot and UART first

1. Ensure you can boot *any* payload on your platform.
2. Bring up UART logging (correct base, IP type, baud/clock).
3. Confirm you can run a “hello world” style payload and capture logs reliably.

See: [UART / console override guide](platform-override-guides/uart.md).

### Step 2 — Memory map sanity

1. Define DRAM ranges as `NORMAL`.
2. Define key MMIO windows as `DEVICE`.
3. Mark firmware carveouts as `RESERVED`.
4. Mark holes/unbacked regions as `NOT_POPULATED` (if the tests intentionally probe them).

See: [Memory map override guide](platform-override-guides/memory.md).

### Step 3 — Timers and watchdogs

Populate:
- per-CPU generic timer PPIs (GSIVs/flags)
- counter frequency (CNTFRQ)
- platform GT frames (if used)
- watchdog frames (if used)

See: [Timers & watchdog override guide](platform-override-guides/timers-watchdog.md).

### Step 4 — PCIe (if applicable)

Populate:
- ECAM window(s)
- bus ranges and enumeration limits
- PCIe MMIO resource windows
- optional static device list used by tests

See: [PCIe ECAM and device hierarchy guide](platform-override-guides/pcie.md).

### Step 5 — IOMMU / IORT (if applicable)

Populate:
- ITS count + grouping
- SMMU instances
- RC/named component nodes
- ID mappings (RID→StreamID, StreamID→DeviceID)

See: [IORT/IOVIRT guide](platform-override-guides/iort.md).

### Step 6 — Optional: cache/PMU/RAS/HMAT

Depending on which suites and tests you run, you may need:
- cache/topology overrides
- PMU nodes (APMT)
- RAS/RAS2
- HMAT bandwidth hints

See: [Platform override guides](platform-override-guides/README.md).

### Step 7 — Run, triage, iterate

- Run a minimal smoke set
- Fix hard faults first (MMIO to wrong region, timer interrupts wrong, ECAM mapping wrong)
- Then iterate on correctness (IORT mapping, feature flags, expectations)

Use the checklists:
- [Porting checklist](checklists/porting-checklist.md)
- [New PAL readiness checklist](checklists/new-pal-readiness-checklist.md)


## PCIe Enumeration

This section explains what you need to get PCIe working in baremetal runs.

### Minimum you need

- ECAM base + segment + bus range
- enumeration limits (max bus/dev/func) used by your baremetal enumerator
- MMIO resource windows (BAR apertures) that don't overlap DRAM/MMIO
- (optional) static inventory list of expected endpoints/bridges used by tests

### Primary reference

See: [PCIe ECAM and device hierarchy guide](platform-override-guides/pcie.md).

### Debug tips

- If cfg reads return `0xFFFF_FFFF`:
  - ECAM base mapping or bus range is wrong
- If devices enumerate but BAR programming fails:
  - MMIO apertures too small or overlapping
- If DMA tests fail:
  - cross-check IORT/SMMU mapping


## IORT and IOMMU

This section explains what you need to wire up **SMMU / IORT** for baremetal test correctness.

### When you need this

You generally need IORT/IOVIRT configuration if:
- devices are behind SMMU translation (typical on Arm servers)
- tests expect correct StreamID / DeviceID mapping
- ATS/PRI expectations are validated
- MSI routing via ITS relies on DeviceID width/mapping

### Primary reference

See: [IOVIRT/IORT guide](platform-override-guides/iort.md).

### Debug tips

- Off-by-one in `ID_COUNT` is the #1 issue.
- Keep `DeviceID == StreamID` when possible to simplify debug.
- Ensure ITS IDs match MADT GIC ITS entries in firmware (if ACPI is generated).


## Memory Map and MMIO

This section explains **what to decide** for memory and MMIO when porting a new platform, and points to the concrete override macros.

### What you must get right

- **DRAM ranges** (Normal memory) must be correct and non-overlapping.
- **MMIO ranges** must be marked as Device to avoid speculative/cached accesses.
- **Reserved carveouts** must not be used by tests as RAM.
- **Holes / not-populated regions** should be explicitly marked when tests probe abort behavior.

### Primary references

- Memory map override values: [Memory Platform Config Guide](platform-override-guides/memory.md)
- Bandwidth and latency details (HMAT): [HMAT guide](platform-override-guides/hmat.md)

### Recommended debugging approach

1. Start with *only* DRAM + essential MMIO (UART, GIC, timers, ECAM if needed).
2. Boot and log.
3. Add secondary MMIO windows incrementally.
4. If you see synchronous aborts:
   - verify the address falls inside a defined region
   - verify the region type is correct (DEVICE vs NORMAL)
   - verify alignment and size fields


## Interrupts, Timers, and UART

This section groups the common “early boot essentials” for baremetal porting.

### UART (console)

Your first milestone is a stable UART console:
- correct UART IP type (PL011 vs 16550 etc.)
- correct base address + access width
- correct baud/clock (or choose “as-is” if firmware configured it)
- optional: correct GSIV if interrupt-driven console is used

See: [UART guide](platform-override-guides/uart.md).

### Timers

Baremetal tests depend on timer correctness for:
- timeouts
- delays
- watchdog interactions
- performance measurement (in some suites)

See: [Timers & watchdog guide](platform-override-guides/timers-watchdog.md).

### Interrupt routing

Many failures are caused by:
- wrong GSIV values (SPI vs PPI mixups)
- incorrect trigger mode/polarity bits
- using a forbidden GSIV range for SPCR/GTDT

When in doubt:
1. validate against GIC integration documentation
2. confirm in firmware logs which interrupts fire


## Troubleshooting

### No UART output
- Verify base address + IP type (PL011 vs 16550)
- Verify clock/baud (or set “as-is”)
- Verify memory map marks UART range as DEVICE
- Verify you are connected to the correct UART instance

### Early synchronous abort / hang
- Check the faulting address (if printed) is within a defined memory/MMIO entry
- Ensure MMIO is DEVICE, not NORMAL
- Ensure reserved regions are not accessed as RAM

### Timer-related timeouts / flakiness
- Verify timer PPIs (GSIV) and flags (mode/polarity)
- Verify CNTFRQ matches what firmware programs
- If using platform GT frames, verify CNTBASE addresses and GSIVs

### PCIe not enumerating
- ECAM base mapping wrong (bus stride mismatch)
- Start/end bus too small
- Segment mismatch
- ECAM region not marked DEVICE in memory map
- Overlapping of MMIO address for BAR programming

### DMA / SMMU / ATS failures
- IORT ID mappings wrong (RID→StreamID / StreamID→DeviceID)
- Wrong “behind SMMU” expectations for endpoints
- ATS marked supported but firmware doesn’t enable it

### RAS / RAS2 / HMAT / PMU failures
- Start with minimal publication and add blocks incrementally
- Ensure counts match the number of blocks populated
- Keep domain/instance identifiers consistent with SRAT/NUMA view (if applicable)
