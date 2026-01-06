
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
- **Latest release version:** v0.8.0 
- **Execution levels:** Silicon.  
- **Scope:** The compliance suite is **not** a substitute for design verification.  
- **Access to logs:** Arm licensees can contact Arm through their partner managers.  

#### PFDI ACS version mapping

|  PFDI ACS Version   |     PFDI Tag ID     | PFDI Spec Version |   Pre-Si Support |
|:-------------------:|:-------------------:|:-----------------:|-----------------:|
|        v0.8.0       |  v25.09_PFDI_0.8.0  |  PFDI v1.0 BET0   |       No         |

#### GitHub branch
- To pick up the release version of the code, check out the corresponding **tag** from the **main** branch.
- To get the latest code with bug fixes and new features, use the **main** branch.

#### Prebuilt release binaries
Prebuilt images for each release are available in the [`prebuilt_images`](../../prebuilt_images/PFDI) folder of the main branch.  

## Documentation and Guides
- [Arm PFDI Test Scenario Document](arm_pfdi_architecture_compliance_test_scenario.pdf) — algorithms for implementable rules and notes on unimplemented rules.

## PFDI build steps

### UEFI Shell application

#### Prerequisites
ACS build requires that the following requirements are met, Please skip this if you are using [PFDI Application Build Script](../../tools/scripts/build_pfdi_uefi.sh).

- A mainstream Linux distribution on x86 or AArch64.
- Bash Shell for build
- Install prerequisite packages to build EDK2.  
  *Note: Package details are beyond the scope of this document.*

#### Setup the workspace and clone required repositories
```
mkdir workspace && cd workspace
git clone -b edk2-stable202508 https://github.com/tianocore/edk2
cd edk2
git submodule update --init --recursive
git clone https://github.com/tianocore/edk2-libc
git clone https://github.com/ARM-software/sysarch-acs.git ShellPkg/Application/sysarch-acs
cd -
```
- On x86 machine download and setup toolchain
```
wget https://developer.arm.com/-/media/Files/downloads/gnu/14.3.rel1/binrel/arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-linux-gnu.tar.xz
tar -xf arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-linux-gnu.tar.xz
export GCC_AARCH64_PREFIX=$PWD/arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-
```
- On Aarch64 machine, export toolchain variable to native tools
```
export GCC_AARCH64_PREFIX=/usr/bin
```

#### Build edk2 by following below steps:
```
export PACKAGES_PATH=$PWD/edk2-libc
source edksetup.sh
make -C BaseTools/Source/C
```

#### To start the ACS build for platform using Device tree, perform the following steps:
```
rm -rf Build/
source ShellPkg/Application/sysarch-acs/tools/scripts/acsbuild.sh pfdi
```

#### Build output
The EFI executable is generated at: `workspace/edk2/Build/Shell/DEBUG_GCC/AARCH64/pfdi.efi`

## PFDI run steps for UEFI application

#### Silicon System
On a system with a functional USB port:
1. Copy `pfdi.efi` to a USB device which is fat formatted.  

- **For u-boot firmware Systems, additional steps**
  1. Copy `Shell.efi` to the USB device.
  *Note:* `Shell.efi` is available in [prebuilt_images](../../prebuilt_images/BSA).
  
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
4. Run `pfdi.efi` with appropriate parameters.  
5. Capture UART console output to a log file.

#### Emulation environment with secondary storage
1. Create an image containing `pfdi.efi` and **'Shell.efi` (only for u-boot systems)**:
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

Command line arguments are similar for uefi application, with some exceptions.

#### UEFI

Shell> pfdi.efi [-v &lt;verbosity&gt;] [-skip &lt;test_id&gt;] [-f &lt;filename&gt;]

##### -v
Choose the verbosity level.

- 1 - ERROR
- 2 - WARN and ERROR
- 3 - TEST and above
- 4 - DEBUG and above
- 5 - INFO and above

##### -skip
Overrides the suite to skip the execution of a particular
test. For example, <i>-skip 10</i> skips test 10.

#### -f (Only for UEFI application)
Save the test output into a file in secondary storage. For example <i>-f pfdi.log</i> creates a file pfdi.log with test output.

##### UEFI example

Shell> pfdi.efi -v 5 -skip 15,20,30 -f pfdi_uefi.log

Runs PFDI ACS with verbosity INFO, skips test 15, 20 and 30 and saves the test results in <i>pfdi_uefi.log</i>.

## Limitations
 - PFDI ACS currently supports only Device Tree (DT)-based platforms. ACPI support is planned for a future release.

## License
PFDI ACS is distributed under Apache v2.0 License.

--------------

*Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.*
