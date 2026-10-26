## Table of Contents

- [MPAM Memory System Component Specification](#mpam-memory-system-component-specification)
- [SYS-MPAM Architecture Compliance Suite](#sys-mpam-architecture-compliance-suite)
- [Release details](#release-details)
- [Documentation & Guides](#documentation-and-guides)
- [SYS-MPAM build steps](#sys-mpam-build-steps)
  - [UEFI Shell application](#uefi-shell-application)
  - [Baremetal application](#baremetal-application)
- [SYS-MPAM run steps](#sys-mpam-run-steps)
  - [For UEFI application](#for-uefi-application)
- [Application arguments](#application-arguments)
- [Limitations](#limitations)
- [Feedback, contributions and support](#feedback-contributions-and-support)
- [License](#license)

## MPAM Memory System Component Specification
**Memory System Resource Partitioning and Monitoring (MPAM)** describes
propagation of Partition ID (PARTID) and Performance Monitoring Group (PMG)
through the memory system, along with controls that partition performance
resources of a memory-system component (MSC).

For more information, download the
[MPAM System Component Specification](https://developer.arm.com/documentation/ihi0099/aa/).

## SYS-MPAM Architecture Compliance Suite

The SYS-MPAM **Architecture Compliance Suite (ACS)** is a collection of
self-checking, portable C-based tests.
This suite provides examples of the invariant behaviors defined in the MPAM
System Component specification, enabling verification that these behaviors have
been implemented and interpreted correctly.

## Release details
- **Code quality:** Beta
- **Latest release version:** v0.7.0
- **Release tag:** `v26.03_MPAM_0.7.0`
- **Specification coverage:** MPAM Memory System Component Specification vA.a
- **Scope:** The compliance suite is **not** a substitute for design verification.
- **Prebuilt binaries:** [`prebuilt_images/SYS-MPAM/v26.03_MPAM_0.7.0`](../../prebuilt_images/SYS-MPAM/v26.03_MPAM_0.7.0)
- For details on tests implemented in this release, see the
  [SYS-MPAM Test Scenario Document](arm_mpam_architecture_compliance_test_scenario.md).

#### SYS-MPAM ACS version mapping

| SYS-MPAM ACS Version | SYS-MPAM Tag ID | Spec Version |
|:--------------------:|:---------------:|:------------:|
| v0.7.0 | v26.03_MPAM_0.7.0     | MPAM Memory System Component A.a |
| v0.5.0 | v25.03_MPAM_0.5.0_ALP | MPAM Memory System Component A.a |

## Documentation and Guides
- [SYS-MPAM Test Scenario Document](arm_mpam_architecture_compliance_test_scenario.md)
- [SYS-MPAM Rule Coverage Checklist](arm_sys-mpam_testcase_checklist.md)
- [Rule-based execution guide](../common/RuleBasedGuide.md)
- Bare-metal porting guides:
  - [SYS-MPAM ACS Bare-metal User Guide](arm_mpam_architecture_compliance_bare-metal_user_guide.md)
  - [Common Bare-metal Build Guide](../../pal/baremetal/README.md)
- [Common UEFI build guide](../common/uefi_build.md)

## SYS-MPAM build steps

### UEFI Shell application

Follow the [Common UEFI build guide](../common/uefi_build.md) to set up the
edk2 workspace and Arm toolchain, then run:

- `source ShellPkg/Application/sysarch-acs/tools/scripts/acsbuild.sh mpam`

The flow emits `Mpam.efi` under `Build/Shell/<TOOL_CHAIN_TAG>/AARCH64/` inside
the edk2 workspace.

### Baremetal application

For SYS-MPAM platform setup, required information tables, and porting details,
refer to the
[SYS-MPAM ACS Bare-metal User Guide](arm_mpam_architecture_compliance_bare-metal_user_guide.md).

For common bare-metal build and execution steps, refer to the
[Bare-metal README](../../pal/baremetal/README.md).

## SYS-MPAM run steps

### For UEFI application

#### Silicon System
On a system with a functional USB port:
1. Copy `Mpam.efi` to a FAT-formatted USB device.
2. Boot to the UEFI shell.
3. Refresh mappings with: `map -r`.
4. Switch to the USB filesystem (for example, `fs0:`).
5. Run `Mpam.efi` with the required parameters (see
   [Application arguments](#application-arguments)).
6. Capture the UART console output to a log file for analysis.

**Example**

`Shell> Mpam.efi -v 1 -skip CFXGZ,RNSLZ -f mpam_uefi.log`

Runs at INFO level, skips rules `CFXGZ` and `RNSLZ`, and saves the UART log to
`mpam_uefi.log`.

#### Emulation environment with secondary storage
1. Create a FAT image containing `Mpam.efi`:
   `mkfs.vfat -C -n HD0 hda.img 31457280`
   `sudo mount hda.img /mnt/mpam`
   `sudo cp <path to Mpam.efi> /mnt/mpam/`
   `sudo umount /mnt/mpam`
2. Attach the image to the virtual platform or emulator using its documented
   backdoor method.
3. Boot to the UEFI shell.
4. Refresh filesystem mappings with: `map -r`.
5. Switch to the assigned filesystem (`fs<x>`), then run `Mpam.efi`.
6. Capture UART console output for certification and debug reports.

## Application arguments

SYS-MPAM ACS uses rule IDs as the primary unit of execution and reporting.
Legacy numeric test IDs cannot be used to select or skip tests.

```text
Shell> Mpam.efi [-v <verbosity>] [-l <version>] [-r <rule_ids|rules_file>] [-skip <rule_ids|rules_file>] [-m <module_names>] [-skipmodule <module_names>] [-f <filename>]
```

With no rule or module selector, the application runs all SYS-MPAM rules for
MPAM architecture version B.c.

#### `-v`
Choose the verbosity level.

- 1 - INFO and above
- 2 - DEBUG and above
- 3 - TEST and above
- 4 - WARN and ERROR
- 5 - ERROR

#### `-l`

Select the MPAM architecture version. The supported value is `1`, which maps
to `MPAM_VERSION_B_c`.

#### `-r`

Run one or more comma-separated SYS-MPAM rule IDs, or provide a text file that
contains a comma-separated rule list. For example, `-r CFXGZ,RNSLZ` runs only
those rules.

#### `-skip`

Skip one or more comma-separated SYS-MPAM rule IDs, or provide a text file that
contains a comma-separated rule list. For example, `-skip CFXGZ,RNSLZ` skips
those rules.

#### `-m`

Run only the specified comma-separated module names. Accepted values are
`CACHE`, `REG`, `ERROR`, and `MBWU`.

#### `-skipmodule`

Skip the specified comma-separated module names. Accepted values are `CACHE`,
`REG`, `ERROR`, and `MBWU`.

#### `-f` (UEFI only)

Save test output to a file in secondary storage. For example,
`-f mpam.log` creates `mpam.log` with test output.

#### `-h`, `-help`

Display the application usage information.

## Limitations

- Some tests related to MSC error handling have been verified on limited
  platforms. If you encounter failures or errors during ACS runs, please raise
  an issue.
- Memory Bandwidth Partitioning tests have been implemented but not yet
  verified on any platform.
- Some Cache partitioning tests have not been validated on silicon-based systems. Users should treat these outcomes as preliminary and perform additional validation before relying on them for compliance.

## Feedback, contributions and support

- Email: [support-systemready-acs@arm.com](mailto:support-systemready-acs@arm.com)
- GitHub Issues: [sysarch-acs issue tracker](https://github.com/ARM-software/sysarch-acs/issues)
- Arm licensees can contact Arm through their partner managers.

## License
SYS-MPAM ACS is distributed under Apache v2.0 License.

--------------

*Copyright (c) 2025-2026, Arm Limited and Contributors. All rights reserved.*
