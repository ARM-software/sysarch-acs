# SYS-MPAM ACS Bare-metal User Guide

## Table of Contents

- [Overview](#overview)
- [Before you begin](#before-you-begin)
- [Platform setup](#platform-setup)
- [Required information tables](#required-information-tables)
- [MPAM information table](#mpam-information-table)
- [Cross-table consistency](#cross-table-consistency)
- [Build SYS-MPAM ACS](#build-sys-mpam-acs)
- [Run SYS-MPAM ACS](#run-sys-mpam-acs)
- [Select tests and modules](#select-tests-and-modules)
- [Bring-up checks and troubleshooting](#bring-up-checks-and-troubleshooting)
- [Limitations](#limitations)

## Overview

SYS-MPAM ACS can run as a bare-metal payload without UEFI or an operating
system. In this environment, firmware tables are not available to discover the
platform. The bare-metal Platform Abstraction Layer (PAL) must therefore supply
the processor, interrupt-controller, MPAM, cache, memory-locality, and I/O
topology information required by the selected tests.

The code under `pal/baremetal/` is a reference PAL implementation. Adapt and
validate it for the target platform before relying on ACS results. The supplied
RD-family target implementations are examples; their addresses, topology, and
capabilities must not be copied to another platform without verification.

The SYS-MPAM bare-metal application entry point is
[`apps/baremetal/mpam_main.c`](../../apps/baremetal/mpam_main.c). It creates the
required information tables and then runs the selected register, cache, error,
monitoring, and memory-bandwidth tests.

## Before you begin

Prepare the following:

- An AArch64 bare-metal GNU toolchain. Set `CROSS_COMPILE` to its executable
  prefix, such as `<toolchain>/bin/aarch64-none-elf-`.
- A platform boot flow capable of loading SYS-MPAM ACS as the non-secure
  bare-metal payload.
- A working UART console for test output.
- The platform memory map, GIC and PE topology, MPAM MSC register map, MPAM
  resource topology, and any applicable cache, IOVIRT, PCC, HMAT, and SRAT
  information.
- The MPAM extension enabled and accessible at the exception level where ACS
  runs.

Read the shared [Bare-metal README](../../pal/baremetal/README.md) for the common
build and bootwrapper flow. For general PAL bring-up, see the
[PAL Porting Overview](../baremetal/porting-pal/overview.md) and the
[platform override guides](../baremetal/porting-pal/platform-override-guides/README.md).

## Platform setup

Use an existing target under `pal/baremetal/target/` as a reference. A target
normally provides the following files:

| File | Purpose |
|---|---|
| `include/platform_override_fvp.h` | Platform counts, addresses, interrupts, timeouts, and MPAM resource values. |
| `include/platform_override_struct.h` | Fixed-size platform override structures and their maximum entry counts. |
| `include/platform_image_def.h` | Image placement, stack, heap, and memory-pool configuration. |
| `src/platform_cfg_fvp.c` | Instances of the platform information used to populate PAL tables and the bare-metal test-selection arrays. |
| `src/pal_bsa.c` and `src/pal_sbsa.c` | Platform-specific PAL hooks reused by the common bare-metal framework. |

When adding a platform, keep the directory name consistent with the value
passed through `-DTARGET=<platform>`. CMake obtains the supported target names
from the directories under `pal/baremetal/target/`.

Bring up the common bare-metal services first:

1. Confirm that the payload boots and UART output is stable.
2. Populate the PE and GIC topology.
3. Describe normal memory, device MMIO, and reserved regions in the platform
   memory map.
4. Confirm exception handling, timer-based delays, and aligned memory
   allocation.
5. Add the MPAM MSC and resource descriptions.
6. Add the suite-dependent cache, IOVIRT, PCC, HMAT, and SRAT information.

## Required information tables

The application creates tables according to the selected SYS-MPAM modules:

| Information table | Requirement | Used for |
|---|---|---|
| PE | Always | PE enumeration and verification of the MPAM architectural extension. |
| GIC | Always | Interrupt routing and MPAM interrupt tests. |
| MPAM | Always | MSC interfaces, registers, interrupts, and managed resources. |
| IOVIRT | Error, cache, or memory modules, or monitor tests | ITS/device mappings and resources associated with I/O components. |
| Cache | Cache module or monitor tests | Cache topology and cache-resource matching. |
| PCC | Required when an MSC uses a PCC interface | PCC subspace and shared-memory communication details. The application initializes this table even when MSCs are MMIO based. |
| HMAT | Memory module | Memory-side cache and bandwidth information. |
| SRAT | Memory module | Memory ranges and proximity-domain information. |

The table-selection logic is implemented in
[`apps/baremetal/mpam_main.c`](../../apps/baremetal/mpam_main.c). Populate every
table required by the modules you plan to run; a successful build alone does
not validate the table contents.

Use these field-level references:

- [PE and GIC](../baremetal/porting-pal/platform-override-guides/pe-gic.md)
- [Memory map](../baremetal/porting-pal/platform-override-guides/memory.md)
- [MPAM and PCC](../baremetal/porting-pal/platform-override-guides/mpam-pcc.md)
- [Cache topology](../baremetal/porting-pal/platform-override-guides/cache.md)
- [IOVIRT and IORT](../baremetal/porting-pal/platform-override-guides/iort.md)
- [HMAT](../baremetal/porting-pal/platform-override-guides/hmat.md)
- [SRAT](../baremetal/porting-pal/platform-override-guides/srat.md)

## MPAM information table

The platform configuration supplies one `PLATFORM_OVERRIDE_MPAM_MSC_NODE` for
each Memory System Component (MSC). Each MSC contains one or more managed
resource nodes.

### Global sizing fields

| Field | Description |
|---|---|
| `MPAM_MAX_MSC_NODE` | Capacity of the platform override array for MSC nodes. |
| `MPAM_MAX_RSRC_NODE` | Per-MSC capacity of the platform override array for resource nodes. |
| `PLATFORM_MPAM_MSC_COUNT` | Number of MSC nodes populated for the platform. |

Ensure the populated counts do not exceed the corresponding maximum values.
The current bare-metal allocation helper reserves one resource entry per MSC.
If the total number of populated resource nodes exceeds the MSC count, update
the allocation in `createMpamInfoTable()` before using that topology.

### MSC fields

Populate the following for every MSC:

| Field | Description |
|---|---|
| `intrf_type` | Interface used to access the MSC. Use System Memory/MMIO or PCC as implemented by the platform. |
| `identifier` | Unique MSC identifier. Keep it consistent with any associated device description. |
| `msc_base_addr` | MPAM register-space base for MMIO, or the PCC subspace ID for a PCC interface. |
| `msc_addr_len` | Size of the MMIO register region. Set this field to `0` for a PCC interface. |
| `of_intr` and `of_intr_flags` | Wired overflow interrupt GSIV and flags, when implemented. |
| `err_intr` and `err_intr_flags` | Wired error interrupt GSIV and flags, when implemented. |
| `max_nrdy` | Maximum time, in microseconds, for the MSC to become ready after a configuration change. |
| `device_obj_name` | Name used to associate the MSC with a platform device when required by device/MSI flows. |
| `rsrc_count` | Number of resource nodes managed by this MSC. |

Do not use placeholder interrupt, PCC, or address values for enabled tests.
Unused optional fields should be set according to the MPAM specification and
the PAL implementation's expected encoding.

### Resource fields

Populate the following for every resource controlled by an MSC:

| Field | Description |
|---|---|
| `ris_index` | Resource Instance Selection index. Use zero when RIS is not implemented. |
| `locator_type` | Type of component managed by the resource: PE cache, memory, SMMU, memory-side cache, ACPI device, or unknown (`0xFF`). |
| `descriptor1` | Primary identifier for the selected locator type. |
| `descriptor2` | Secondary identifier or reserved value defined for the locator type. |

The locator descriptors identify objects in other platform tables. Derive them
from the actual hardware topology; they are not arbitrary ACS-local numbers.

## Cross-table consistency

Check the following relationships before running tests:

- Every MMIO-based MSC register range must be mapped as device memory. The
  current VAL installs this mapping dynamically with `val_mmu_update_entry()`;
  verify that the mapping succeeds before accessing the MSC registers.
- A processor-cache locator must refer to the corresponding cache identifier in
  the cache topology.
- A memory locator must use the matching SRAT proximity domain. HMAT entries
  used for memory-bandwidth tests must describe the same memory topology.
- An SMMU locator must agree with the applicable IOVIRT nodes and identifiers.
- For a PCC-based MSC, `msc_base_addr` contains the PCC subspace ID. That ID
  must select a valid PCC table entry whose shared-memory region, doorbell,
  completion registers, and timing values match the firmware implementation.
- Wired overflow and error GSIVs must exist in the GIC configuration and use
  the correct trigger and polarity flags.
- `identifier`, `device_obj_name`, DeviceID, and ITS mappings must agree for
  device-associated MSC interrupt flows.

## Build SYS-MPAM ACS

From the repository root:

```bash
export CROSS_COMPILE=<path-to-toolchain>/bin/aarch64-none-elf-
cmake --preset mpam -DTARGET=<platform>
cmake --build --preset mpam
```

If `-DTARGET` is omitted, the current default target is `RDN2`. List the
available configure and build presets with:

```bash
cmake --list-presets
cmake --build --list-presets
```

A successful build places `mpam.bin`, `mpam.elf`, the image, and debug artifacts
under `build/mpam_build/output/`.

## Run SYS-MPAM ACS

SYS-MPAM ACS runs as the non-secure bare-metal payload. The exact packaging and
launch procedure is platform-specific. For the reference RDN2 bootwrapper flow:

1. Copy `build/mpam_build/output/mpam.bin` into the platform software stack as
   its ACS payload.
2. Package it as the BL33/non-trusted firmware image.
3. Launch the model or hardware platform and capture the UART output.
4. Confirm that the banner reports the expected target and that platform table
   creation completes before tests start.
5. Reset the system after the suite completes, as requested by the application.

See the [Bare-metal README](../../pal/baremetal/README.md) for the reference
bootwrapper commands. Adapt those steps to the target platform's trusted
firmware and image-packaging flow.

## Select tests and modules

SYS-MPAM bare-metal uses numeric, non-rule-based test and module selectors.
Configure the following arrays in the target's `src/platform_cfg_fvp.c` before
building:

- `g_module_array`: run only the listed MPAM module base numbers.
- `g_test_array`: run only the listed test numbers.
- `g_skip_array`: exclude listed test numbers or module base numbers.

Use the MPAM module constants in `val/include/val_interface.h` and test numbers
defined under `test_pool/mpam/`. Valid module base values are `0` (register),
`100` (cache), `200` (error), and `300` (memory). Monitoring tests run within
the cache or memory groups rather than as a standalone module. The skip list
takes precedence over included tests and modules. Leave all three arrays empty
to use the default selection.

EL3 firmware can also supply non-rule-based MPAM selectors at boot. See
[`pal/baremetal/run_time_params.rst`](../../pal/baremetal/run_time_params.rst)
for the register convention and reduced MPAM parameter layout.

## Bring-up checks and troubleshooting

Before interpreting compliance results, check the initial UART log:

- The binary reports the intended `TARGET`.
- PE table creation succeeds and the PE reports the MPAM extension.
- GIC initialization succeeds.
- The MPAM information dump shows the expected MSC count, base addresses,
  resources, RIS indices, locator types, and descriptors.
- At least one MSC node is present; otherwise the suite exits without running
  tests.
- The platform memory pool is large enough for the selected tables and shared
  test buffers.

Common failure patterns:

| Symptom | Checks |
|---|---|
| Abort on the first MSC register access | Verify the MSC base, address length, MMIO memory-map entry, and access permissions. |
| Suite reports no MPAM nodes | Verify `PLATFORM_MPAM_MSC_COUNT` and the `platform_mpam_cfg` initializer. |
| Cache or monitor tests skip/fail | Verify cache-table identifiers and MPAM cache locator descriptors. |
| Memory-bandwidth tests skip/fail | Verify SRAT proximity domains, HMAT entries, MPAM memory locators, and usable memory ranges. |
| PCC access times out | Verify the subspace ID, shared-memory mapping, doorbell/completion registers, and `max_nrdy`/turnaround timing. |
| Overflow or error interrupt is not observed | Verify MSC interrupt fields, GIC GSIVs/flags, and any DeviceID-to-ITS mapping. |

## Limitations

- The bare-metal PAL is reference code and must be validated on each target
  platform.
- Some error-handling, cache-partitioning, and memory-bandwidth tests have
  limited platform validation. See the main [SYS-MPAM README](README.md) for
  current suite limitations.

---

*Copyright (c) 2026, Arm Limited and Contributors. All rights reserved.*
