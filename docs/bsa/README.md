> **Important change — BSA ACS rule-based execution**
>
> BSA ACS has undergone a framework change to support **specification rule-based execution and reporting**.  
> For details on using BSA Specification rule IDs to run the latest BSA binaries and to collate results per rule, see the [Rule-Based Guide](../common/RuleBasedGuide.md)

## Table of Contents

- [Base System Architecture](#base-system-architecture)
- [BSA - Architecture Compliance Suite](#bsa---architecture-compliance-suite)
- [Release details](#release-details)
- [Documentation & Guides](#documentation-and-guides)
- [BSA build steps](#bsa-build-steps)
  - [UEFI Shell application](#uefi-shell-application)
  - [Linux application](#linux-application)
  - [Baremetal application](#baremetal-application)
- [BSA run steps](#bsa-run-steps)
  - [For UEFI application](#for-uefi-application)
  - [For Linux application](#for-linux-application)
- [Limitations](#limitations)
- [License](#license)


## Base System Architecture
**Base System Architecture (BSA)** describes a 64-bit Arm-based hardware system architecture relied upon by operating systems, hypervisors, and firmware. It addresses PE features and key aspects of system architecture.

For more information, download the [BSA specification](https://developer.arm.com/documentation/den0094/latest/)

## BSA - Architecture Compliance Suite

The BSA **Architecture Compliance Suite (ACS)** is a collection of self-checking, portable C-based tests.
This suite provides examples of the invariant behaviors defined in the BSA specification, enabling verification that these behaviors have been implemented and interpreted correctly.

Most tests run from the UEFI (Unified Extensible Firmware Interface) Shell via the BSA UEFI shell application.
A subset runs from Linux via a BSA ACS user-space application and its kernel module.
Tests can also run in a bare-metal environment. Initialization of the bare-metal environment is platform-specific and out of scope for this document.

## Release details
- **Latest release version:** v1.2.0
- **Execution levels:** Pre-Silicon and Silicon.
- **Scope:** The compliance suite is **not** a substitute for design verification.
- **Access to logs:** Arm licensees can contact Arm through their partner managers.

#### BSA ACS version mapping

|   BSA ACS Version   |      BSA Tag ID     | BSA Spec Version |    Pre-Si Support |
|:-------------------:|:-------------------:|:----------------:|------------------:|
|        v1.2.0       |   v25.12_BSA_1.2.0  |   BSA v1.2       |       Yes         |
|        v1.1.2       |   v25.10_BSA_1.1.2  |   BSA v1.1       |       Yes         |
|        v1.1.0       |   v25.03_REL1.1.0   |   BSA v1.1       |       Yes         |
|        v1.0.9       |   v24.11_REL1.0.9   |   BSA v1.1       |       Yes         |
|        v1.0.8       |   v24.03_REL1.0.8   |   BSA v1.0(c)    |       Yes         |
|        v1.0.7       |   v23.12_REL1.0.7   |   BSA v1.0(c)    |       Yes         |
|        v1.0.6       |v23.11_BootFramework |   BSA v1.0(c)    |       Yes         |
|        v1.0.6       |   v23.09_REL1.0.6   |   BSA v1.0(c)    |       Yes         |
|        v1.0.5       |   v23.07_REL1.0.5   |   BSA v1.0(c)    |       Yes         |
|        v1.0.4       |   v23.03_REL1.0.4   |   BSA v1.0(c)    |       Yes         |
|        v1.0.3       |   v23.01_REL1.0.3   |   BSA v1.0       |       No          |
|        v1.0.2       |   v22.10_REL1.0.2   |   BSA v1.0       |       No          |
|        v1.0.1       |   v22.06_REL1.0.1   |   BSA v1.0       |       No          |
|        v1.0         |   v21.09_REL1.0     |   BSA v1.0       |       No          |

#### GitHub branch
- To pick up the release version of the code, check out the corresponding **tag** from the **main** branch.
- To get the latest code with bug fixes and new features, use the **main** branch.

#### Prebuilt release binaries
Prebuilt images for each release are available in the [`prebuilt_images`](../../prebuilt_images/BSA) folder of the main branch.  

## Documentation and Guides
- [Arm BSA Test Scenario Document](arm_bsa_architecture_compliance_test_scenario.pdf) — algorithms for implementable rules and notes on unimplemented rules.  
- [Arm BSA Test Checklist](arm_bsa_testcase_checklist.rst) — test categories (UEFI, Linux, Bare-metal) and applicable systems (IR, ES, SR, Pre-Silicon).  
- [Arm BSA Validation Methodology](arm_bsa_architecture_compliance_validation_methodology.pdf).  
- [Arm BSA ACS User Guide](arm_bsa_architecture_compliance_user_guide.pdf).  
- Bare-metal porting guides  
  - [Arm BSA ACS Bare-metal User Guide](arm_bsa_architecture_compliance_bare-metal_user_guide.pdf)  
  - [Bare-metal Code](../../pal/baremetal/)  

  **Note:** The Bare-metal PCIe enumeration code provided with BSA ACS must be used and **must not be replaced**. It is essential for accurate analysis of test results.

- Exerciser VIP guides - The **Exerciser** is a client device wrapped by a PCIe Endpoint, created to satisfy BSA requirements for PCIe capability validation. Running Exerciser tests increases platform coverage.
  - [Exerciser.md](../pcie/Exerciser.md)
  - [Exerciser_API_porting_guide.md](../pcie/Exerciser_API_porting_guide.md).

## BSA build steps

### UEFI Shell application

#### Prerequisites
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

#### To start the ACS build for platform using ACPI table, perform the following steps:
```
rm -rf Build/
source ShellPkg/Application/sysarch-acs/tools/scripts/acsbuild.sh bsa
```

#### To start the ACS build for platform using Device tree, perform the following steps:
```
rm -rf Build/
source ShellPkg/Application/sysarch-acs/tools/scripts/acsbuild.sh bsa_dt
```

#### Build output
The EFI executable is generated at: `workspace/edk2/Build/Shell/DEBUG_GCC/AARCH64/Bsa.efi`

### Linux application
Certain Peripheral, PCIe, and Memory Map tests require a Linux OS. This section covers building and executing these tests from the Linux application.

#### Prerequisites
- A mainstream Linux distribution on x86 or AArch64.
- Bash Shell for build

#### Setup the workspace and clone required repositories
```
mkdir workspace && cd workspace
wget https://gitlab.arm.com/linux-arm/linux-acs/-/raw/master/acs-drv/files/build.sh
chmod +x build.sh
```

#### To start the build, perform below steps:
Build Script arguments :The following arguments can be used with `build.sh`:
 - `-v` or `--version` — Linux kernel version for cross-compilation. Default: **6.10**.  
 - `--GCC_TOOLS` — GCC toolchain version for cross-compilation. Default: **13.2.rel1**.  
 - `--help` — Displays environment info, defaults, usage, and notes.  
 - `--clean` — Removes the `build/` output folder (modules and apps).  
 - `--clean_all` — Removes all downloaded repositories and build artifacts, including the output directory.

```
source build.sh $args (as needed)
```

#### Build Output
The ACS kernel module and app will be generated at `workspace/build/`
 - `bsa_acs.ko` : Kernel module which needs to be insmod before running bsa_app
 - `bsa_app` : bsa linux app

### Baremetal application
The bare-metal build environment is platform-specific.
For details on generating binaries for bare-metal environments, refer to [README.md](../../pal/baremetal/README.md).


## BSA run steps

### For UEFI application 

#### Silicon System
On a system with a functional USB port:
1. Copy `Bsa.efi` to a USB device which is fat formatted.  

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
4. Run `Bsa.efi` with appropriate parameters.  
5. Capture UART console output to a log file.

- For application parameters, see the [User Guide](arm_bsa_architecture_compliance_user_guide.pdf).

#### Emulation environment with secondary storage
1. Create an image containing `Bsa.efi` and **'Shell.efi` (only for u-boot systems)**:
   ```
   mkfs.vfat -C -n HD0 hda.img 2097152
   sudo mount -o rw,loop=/dev/loop0,uid=$(whoami),gid=$(whoami) hda.img /mnt/bsa
   sudo cp "<path to application>/Bsa.efi" /mnt/bsa/
   sudo umount /mnt/bsa
   ```
   *(If `/dev/loop0` is busy, select a free loop device.)*
2. Load the image to secondary storage via a backdoor (environment-specific).
3. Boot to UEFI Shell.  
4. Identify the filesystem with `map -r`.  
5. Switch to the filesystem (`fs<x>:`).  
6. Run `Bsa.efi` with parameters.  
7. Save UART console output for analysis/certification.

- For application parameters, see the [User Guide](arm_bsa_architecture_compliance_user_guide.pdf).

### For Linux application

1. Copy the bsa_acs.ko and bsa_app into USB drive
2. Boot to Linux and identify the USB drive
3. Load the BSA kernel module
  ```sh
  sudo insmod bsa_acs.ko
  ```
4. Run the BSA application
  ```sh
  ./bsa_app
  ```
5. Remove the BSA kernel module after run
  ```sh
  sudo rmmod bsa_acs
  ```
- For application parameters, see the [User Guide](arm_bsa_architecture_compliance_user_guide.pdf).

## Limitations
- For systems with firmware compliant to SBBR, BSA depends on the **SPCR ACPI table** to obtain UART information.  
  Set the UEFI console to **serial**.
- ITS tests are available only for systems presenting firmware compliant to SBBR.
- Some PCIe and Exerciser tests depend on platform PCIe features. Please populate the required PAL APIs with platform details:
  - `pal_pcie_p2p_support` — whether peer-to-peer transactions are supported.
  - `pal_pcie_is_cache_present` — whether PCIe address translation cache is supported.
  - `pal_pcie_get_legacy_irq_map` — fill the system legacy IRQ map.
- Required Exerciser capabilities:
  - MSI-X interrupt generation.
  - Incoming Transaction Monitoring (order, type).
  - Initiating transactions from and to the Exerciser.
  - Ability to check BDF and register addresses observed for each configuration access along with access type.
- Linux DMA-related tests have **not** been verified


## License
BSA ACS is distributed under the **Apache v2.0 License**.

--------------

*Copyright (c) 2021-2025, Arm Limited and Contributors. All rights reserved.*
