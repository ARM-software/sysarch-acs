# DRTM Architecture for Arm - Architecture Compliance Suite

## Table of Contents

- [DRTM Architecture for Arm](#drtm-architecture-for-arm)
- [DRTM Architecture Compliance Suite](#drtm-architecture-compliance-suite)
- [Release details](#release-details)
- [Documentation and Guides](#documentation-and-guides)
- [DRTM build steps](#drtm-build-steps)
  - [UEFI Shell application](#uefi-shell-application)
- [DRTM run steps](#drtm-run-steps)
  - [For UEFI application](#for-uefi-application)
- [Application arguments](#application-arguments)
- [Limitations](#limitations)
- [Feedback, contributions and support](#feedback-contributions-and-support)
- [License](#license)

## DRTM Architecture for Arm

The **Dynamic Root of Trust for Measurement (DRTM) Architecture for Arm**
defines an architecture for beginning a new chain of trust and launching a
measured, protected payload on processors based on the Arm A-profile
architecture.

For more information, download the
[DRTM Architecture for Arm specification](https://developer.arm.com/documentation/den0113/latest).

## DRTM Architecture Compliance Suite

The DRTM **Architecture Compliance Suite (ACS)** is a collection of
self-checking, portable C-based tests. The tests cover the DRTM interface and
dynamic-launch behavior defined by the specification.

The suite is built as a UEFI Shell application. DRTM ACS currently uses numeric
test and module identifiers for execution and reporting.

## Release details

- **Code quality:** BETA
- **ACS version:** v1.4.0
- **Specification coverage:** DRTM Architecture for Arm v1.4
- **Execution environments:** Silicon and emulation
- **Scope:** The compliance suite is **not** a substitute for design verification.
- For details on the tests implemented in this release, see the
  [DRTM Test Scenario Document](arm_drtm_architecture_compliance_test_scenario.md).

## Documentation and Guides

- [DRTM Test Scenario Document](arm_drtm_architecture_compliance_test_scenario.md)
- [Common UEFI build guide](../common/uefi_build.md)

## DRTM build steps

### UEFI Shell application

Follow the [Common UEFI build guide](../common/uefi_build.md) to set up the
edk2 workspace and Arm toolchain, then run the following command from the edk2
workspace:

```bash
source ShellPkg/Application/sysarch-acs/tools/scripts/acsbuild.sh drtm
```

The build emits `Drtm.efi` under
`Build/Shell/<TOOL_CHAIN_TAG>/AARCH64/` inside the edk2 workspace.

## DRTM run steps

### For UEFI application

#### Silicon system

On a system with a functional USB port:

1. Copy `Drtm.efi` to a FAT-formatted USB device.
2. Boot to the UEFI Shell.
3. Refresh filesystem mappings with `map -r`.
4. Switch to the USB filesystem, for example `fs0:`.
5. Run `Drtm.efi` with the required parameters described in
   [Application arguments](#application-arguments).
6. Capture the UART console output to a log file for analysis.

**Example**

`Shell> Drtm.efi -v 1 -skip 5,10,105 -f drtm_uefi.log`

Runs at INFO level, skips tests 5, 10, and 105, and saves the output to
`drtm_uefi.log`.

#### Emulation environment with secondary storage

1. Create a FAT image containing `Drtm.efi`:

   ```bash
   mkfs.vfat -C -n HD0 hda.img 2097152
   sudo mount hda.img /mnt/drtm
   sudo cp <path-to-Drtm.efi> /mnt/drtm/
   sudo umount /mnt/drtm
   ```

2. Attach the image to the virtual platform or emulator using its documented
   backdoor method.
3. Boot to the UEFI Shell.
4. Refresh filesystem mappings with `map -r`.
5. Switch to the assigned filesystem, for example `fs0:`.
6. Run `Drtm.efi` with the required parameters.
7. Capture the UART console output for analysis.

## Application arguments

DRTM ACS currently supports numeric test and module selectors:

```text
Drtm.efi [-v <verbosity>] [-skip <test_id>] [-t <test_id>] [-m <module_id>] [-f <filename>]
```

### `-v`

Choose the verbosity level:

- `1` - INFO and above
- `2` - DEBUG and above
- `3` - TEST and above
- `4` - WARN and ERROR
- `5` - ERROR

### `-skip`

Skip one or more comma-separated test IDs. For example, `-skip 5,10` skips
tests 5 and 10.

### `-t`

Run only the specified comma-separated test IDs. For example, `-t 5,105` runs
only tests 5 and 105.

### `-m`

Run only the specified comma-separated module IDs:

- `0` - Interface tests (test IDs 1-99)
- `100` - Dynamic Launch tests (test IDs 101-199)

For example, `-m 0` runs only the Interface module, while `-m 100` runs only
the Dynamic Launch module. Module names such as `INTERFACE` are not accepted by
the current command-line parser.

### `-f` (UEFI only)

Save test output to a file on the active filesystem. For example,
`-f drtm.log` creates `drtm.log` containing the test output.

### `-help`, `-h`

Display the application usage information.

## Limitations

- DRTM ACS is an Beta-quality release with limited verification for ACPI Hash Table related scenarios.
- The suite currently targets version 1.4 of the DRTM Architecture for Arm specification.

## Feedback, contributions and support

- Email: [support-systemready-acs@arm.com](mailto:support-systemready-acs@arm.com)
- GitHub Issues: [sysarch-acs issue tracker](https://github.com/ARM-software/sysarch-acs/issues)
- Contributions: [GitHub Pull Requests](https://github.com/ARM-software/sysarch-acs/pulls)
- Arm licensees can contact Arm through their partner managers.

## License

DRTM ACS is distributed under the
[Apache v2.0 License](https://www.apache.org/licenses/LICENSE-2.0).

--------------

*Copyright (c) 2024-2026, Arm Limited and Contributors. All rights reserved.*
