
## Table of Contents

- [Platform Fault Detection Interface](#platform-fault-detection-interface)
- [PFDI - Architecture Compliance Suite](#pfdi---architecture-compliance-suite)
- [Release details](#release-details)
- [Documentation & Guides](#documentation-and-guides)
- [PFDI build steps](#pfdi-build-steps)
  - [UEFI Shell application](#uefi-shell-application)
- [PFDI run steps for UEFI application](#pfdi-run-steps-for-uefi-application)
- [Application parameters](#application-parameters)
- [Limitations](#limitations)
- [License](#license)

## Platform Fault Detection Interface
**PFDI** specification defines a set of functions provided by Platform Firmware to System Software so that System Software can schedule the execution of tests that detect faults in PEs and are executed within or controlled by the Platform Firmware.

For more information, download the [PFDI specification](https://developer.arm.com/documentation/110468/latest/)

## PFDI - Architecture Compliance Suite

The PFDI **Architecture Compliance Suite (ACS)** is a collection of self-checking, portable C-based tests.
This suite provides examples of the invariant behaviors defined in the PFDI specification, enabling verification that these behaviors have been implemented and interpreted correctly.

All tests run from the UEFI (Unified Extensible Firmware Interface) Shell via the PFDI UEFI shell application.

## Release details
- **Latest release version:** v0.9.0
- **Execution levels:** Silicon.
- **Scope:** The compliance suite is **not** a substitute for design verification.
- **Access to logs:** Arm licensees can contact Arm through their partner managers.

#### PFDI ACS version mapping

|  PFDI ACS Version   |     PFDI Tag ID     | PFDI Spec Version |   Pre-Si Support |
|:-------------------:|:-------------------:|:-----------------:|-----------------:|
|        v0.9.0       |  v26.03_PFDI_0.9.0  |  PFDI v1.0 BET1   |       No         |
|        v0.8.0       |  v25.09_PFDI_0.8.0  |  PFDI v1.0 BET0   |       No         |

#### GitHub branch
- To pick up the release version of the code, check out the corresponding **tag** from the **main** branch.
- To get the latest code with bug fixes and new features, use the **main** branch.

#### Prebuilt release binaries
Prebuilt images for each release are available in the [`prebuilt_images`](../../prebuilt_images/PFDI) folder of the main branch.

## Documentation and Guides
- [Arm PFDI Test Scenario Document](arm_pfdi_architecture_compliance_test_scenario.pdf) - algorithms for implementable rules and notes on unimplemented rules.
- [Arm PFDI Testcase Checklist](arm_pfdi_testcase_checklist.md) - checklist for test cases and coverage details.

## PFDI build steps

### UEFI Shell application

Follow the [Common UEFI build guide](../common/uefi_build.md) to set up the
edk2 workspace and Arm toolchain, then run:

- `source ShellPkg/Application/sysarch-acs/tools/scripts/acsbuild.sh pfdi      # Device Tree flow`

## PFDI run steps for UEFI application

#### Silicon System
On a system with a functional USB port:
1. Copy `pfdi.efi` to a USB device which is FAT formatted.

- **For U-Boot firmware systems, additional steps**
  1. Copy `Shell.efi` to the USB device.
  *Note:* `Shell.efi` is available in [prebuilt_images](../../prebuilt_images/PFDI).

  2. Boot to the **U-Boot** shell.
  3. Determine the USB device with:
    ```
    usb start
    ```
  4. Load `Shell.efi` to memory and boot UEFI Shell:
    ```
    fatload usb <dev_num> ${kernel_addr_r} Shell.efi
    fatload usb 0 ${kernel_addr_r} Shell.efi
    ```

2. In UEFI Shell, refresh mappings:
   ```sh
   map -r
   ```
3. Change to the USB filesystem (e.g., `fs0:`).
4. Run `pfdi.efi` with required parameters (see [Common CLI arguments](../common/cli_args.md)).
5. Capture UART console output to a log file.

**Example**

`Shell> pfdi.efi -v 1 -skip R0053,R0104 -f pfdi_uefi.log`

Runs PFDI ACS with verbosity INFO, skips rules `R0053`/`R0104`and stores the UART output in `pfdi_uefi.log`.

> Use PFDI rule IDs defined in [PFDI checklist](arm_pfdi_testcase_checklist.md) (for example, `R0053`, `R0104`)

#### Emulation environment with secondary storage
1. Create a FAT image containing `pfdi.efi` and **`Shell.efi` (only for U-Boot systems)**:
   ```
   mkfs.vfat -C -n HD0 hda.img 2097152
   sudo mount -o rw,loop=/dev/loop0,uid=$(whoami),gid=$(whoami) hda.img /mnt/pfdi/
   sudo cp "<path to application>/pfdi.efi" /mnt/pfdi/
   sudo umount /mnt/pfdi
   ```
   *(If `/dev/loop0` is busy, select a free loop device.)*
2. Load the image to secondary storage via a backdoor (environment-specific).
3. Boot to UEFI Shell.
4. Identify the filesystem with `map -r`.
5. Switch to the filesystem (`fs<x>:`).
6. Run `pfdi.efi` with parameters.
7. Save UART console output for analysis/certification.

## Application parameters
Refer to [Common CLI arguments](../common/cli_args.md) for detailed flag
descriptions, logging options, and sample invocations.

## Limitations
 - PFDI ACS currently supports only Device Tree (DT)-based platforms. ACPI support is planned for a future release.

## License
PFDI ACS is distributed under Apache v2.0 License.

--------------

*Copyright (c) 2024-2026, Arm Limited and Contributors. All rights reserved.*
