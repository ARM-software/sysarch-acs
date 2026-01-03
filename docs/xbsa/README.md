## Table of Contents

- [xBSA UEFI application](#xbsa-uefi-application)
- [Release Details](#release-details)
- [Documentation & Guides](#documentation--guides)
- [xBSA build steps](#xbsa-build-steps)
  - [UEFI Shell application](#uefi-shell-application)
    - [Prerequisites](#prerequisites)
    - [Setup the workspace and clone required repositories](#setup-the-workspace-and-clone-required-repositories)
    - [Build edk2 prerequisites](#build-edk2-prerequisites)
    - [Start the xBSA application build](#start-the-xbsa-application-build)
    - [Build output](#build-output)
  - [Linux application](#linux-application)
- [xBSA run steps](#xbsa-run-steps)
  - [For UEFI application](#for-uefi-application)
  - [For Linux application](#for-linux-application)
- [Feedback, contributions and support](#feedback-contributions-and-support)
- [License](#license)

## xBSA UEFI application

The **xBSA UEFI application** packages the BSA, SBSA and PC-BSA test suites into a single, self-checking UEFI binary.
It is intended for platform teams that want to validate a design once and cover the combined requirements of the Base System Architecture (BSA), Server Base System Architecture (SBSA) and PCBase System Architecture (PC-BSA) specifications.

Most tests run from the **UEFI Shell** by invoking the xBSA UEFI application.
Selected PCIe and peripheral tests require the Exerciser VIP to achieve complete coverage.
The test suite can also be executed in bare-metal environments; initialization of those environments remains platform-specific.

## Release Details

- **Coverage:** Aggregates validation for [BSA 1.2](https://developer.arm.com/documentation/den0094/e/?lang=en) and [SBSA 8.0](https://developer.arm.com/documentation/den0029/j/?lang=en).
- **Execution levels:** Suitable for both Pre-Silicon and Silicon validation.
- **Complementary requirements:** Running with the Exerciser VIP is recommended for complete PCIe compliance coverage.
- **Linux dependencies:** The xBSA UEFI application relies on the BSA and SBSA Linux applications for tests that require an OS environment. Refer to the dedicated BSA and SBSA documentation for details.

## Documentation & Guides
- [Rule-Based Guide](../common/RuleBasedGuide.md) — rule filters and reporting flow shared across ACS variants.
- [Common UEFI build guide](../common/uefi_build.md)
- [Common Linux application guide](../common/linux_build.md)
- [Common CLI arguments](../common/cli_args.md)
- [BSA ACS README](../bsa/README.md), [SBSA ACS README](../sbsa/README.md), and [PC BSA ACS README](../pc_bsa/README.md) for component-specific notes referenced by xBSA.

## xBSA build steps

Follow the steps below to build the combined UEFI shell application (`xbsa_acpi.efi`) inside an edk2 workspace.

### UEFI Shell application

#### Prerequisites
- A mainstream Linux distribution on x86 or AArch64.
- Bash shell for building.
- edk2 build dependencies (compiler toolchains, required packages).
  *Note: Package specifics vary by distribution and are out of scope for this guide.*

#### Build steps
Use the [Common UEFI build guide](../common/uefi_build.md) to set up the edk2
workspace, clone `edk2-libc`, and install the Arm GNU AArch64 toolchain. After
the environment is configured, build the xBSA binary:

1. `rm -rf Build/` *(optional clean build)*
2. `source ShellPkg/Application/sysarch-acs/tools/scripts/acsbuild.sh xbsa_acpi`

#### Build output
The xBSA EFI binary is generated at:
`workspace/edk2/Build/Shell/DEBUG_GCC/AARCH64/xbsa_acpi.efi`

> **Note:** The xBSA UEFI application currently supports ACPI-based builds. Platform-specific device tree enablement is not available for the xBSA target. Ensure Exerciser PAL APIs are implemented when the Exerciser VIP is present.

### Linux application
Follow the [Common Linux application guide](../common/linux_build.md) to build the
shared ACS kernel modules and user applications. xBSA reuses `bsa_acs.ko`,
`bsa_app`, `sbsa_acs.ko`, and `sbsa_app`; deploy the binaries that match the
spec revisions you plan to validate alongside the xBSA UEFI binary.

## xBSA run steps

### For UEFI application

#### Silicon system
On platforms with USB access:
1. Copy `xbsa_acpi.efi` to a FAT-formatted USB device.\
  **For U-Boot firmware systems, additional steps**
   - Copy `Shell.efi` to the USB device (available under `prebuilt_images/IR`).
   - Boot to the **U-Boot** shell.
   - Discover the USB device: `usb start`
   - Load `Shell.efi` into memory: `fatload usb <dev_num> ${kernel_addr_r} Shell.efi`
   - Start the UEFI shell: `fatload usb 0 ${kernel_addr_r} Shell.efi`
2. In the UEFI shell refresh the filesystem mappings: `map -r`
3. Switch to the USB filesystem (for example, `fs0:`).
4. Run `xbsa_acpi.efi` with the required parameters (see [Common CLI arguments](../common/cli_args.md)).
5. Capture the UART console output for log retention.

**Example**

`Shell> xbsa_acpi.efi -v 1 -m PE,GIC -skip B_PE_01 -f xbsa.log`

Runs the combined PE and GIC modules, skips `B_PE_01`, and writes logs to
`xbsa.log`.

> xBSA aggregates BSA (`B_*`), SBSA (`S_*`), and PC BSA (`P_*`) rule IDs.

#### Emulation environment with secondary storage
1. Create an image containing `xbsa_acpi.efi` and `Shell.efi` (for U-Boot systems):
   1. `mkfs.vfat -C -n HD0 hda.img 2097152`
   2. `sudo mount -o rw,loop=/dev/loop0,uid=$(whoami),gid=$(whoami) hda.img /mnt/acs`
   3. `sudo cp "<path to application>/xbsa_acpi.efi" /mnt/acs/`
   4. `sudo umount /mnt/acs`
  *(Select an available loop device if `/dev/loop0` is busy.)*
2. Load the image into the emulated secondary storage using the platform-specific mechanism.
3. Boot to the UEFI shell.
4. Identify the filesystem with `map -r`.
5. Switch to the filesystem (`fs<x>:`).
6. Execute `xbsa_acpi.efi` with the appropriate parameters (see [Common CLI arguments](../common/cli_args.md)).
7. Preserve the UART console output for debug or certification review.

### For Linux application

The xBSA UEFI application reuses the BSA and SBSA Linux applications to exercise OS-reliant tests:
1. Transfer the built binaries (`bsa_acs.ko`, `bsa_app`, `sbsa_acs.ko`, `sbsa_app`) to the target system (for example, on a USB drive).
2. Boot into Linux and locate the removable storage device.
3. Load each kernel module before running the corresponding user-space application:
   `sudo insmod bsa_acs.ko`
   `sudo insmod sbsa_acs.ko`
4. Execute the user-space applications to run the Linux portions of the suite (see [Common CLI arguments](../common/cli_args.md)).\
   `./bsa_app`
   `./sbsa_app`
5. Remove the kernel modules once testing is complete:
   `sudo rmmod sbsa_acs`
   `sudo rmmod bsa_acs`

### Application arguments
Refer to [Common CLI arguments](../common/cli_args.md) for the
  canonical flag list, module selectors, xBSA-specific options such as options such as `-a`, `-cache`,
  `-skip-dp-nic-ms` and log-file options shared across ACS binaries.
- Use the appropriate namespace with `-skip`, `-r`, or any rule-based filters
  when narrowing coverage, and capture sample command lines in test evidence.

## Feedback, contributions and support

- Email: [support-systemready-acs@arm.com](mailto:support-systemready-acs@arm.com)
- GitHub Issues: [sysarch-acs issue tracker](https://github.com/ARM-software/sysarch-acs/issues)
- Contributions: [GitHub Pull Requests](https://github.com/ARM-software/sysarch-acs/pulls)

## License
xBSA ACS is distributed under the [Apache v2.0 License](https://www.apache.org/licenses/LICENSE-2.0).

--------------

*Copyright (c) 2025-2026, Arm Limited and Contributors. All rights reserved.*