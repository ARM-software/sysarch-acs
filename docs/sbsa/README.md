> **Important change — SBSA ACS rule-based execution**
>
> SBSA ACS has undergone a framework change to support **specification rule-based execution and reporting**.  
> For details on using SBSA Specification rule IDs to run the latest SBSA binaries and to collate results per rule, see the [Rule-Based Guide](../common/RuleBasedGuide.md)

## Table of Contents

- [Server Base System Architecture](#server-base-system-architecture)
- [SBSA - Architecture Compliance Suite](#sbsa---architecture-compliance-suite)
- [Release details](#release-details)
- [Documentation & Guides](#documentation-and-guides)
- [SBSA build steps](#sbsa-build-steps)
  - [UEFI Shell application](#uefi-shell-application)
  - [Linux application](#linux-application)
  - [Baremetal application](#baremetal-application)
- [SBSA run steps](#sbsa-run-steps)
  - [For UEFI application](#for-uefi-application)
  - [For Linux application](#for-linux-application)
- [Limitations](#limitations)
- [License](#license)

## Server Base System Architecture
**Server Base System Architecture** (SBSA) specification specifies a hardware system architecture based on the Arm 64-bit architecture.
Server system software such as operating systems, hypervisors, and firmware rely on this. It addresses processing element features and key aspects of system architecture.

For more information, download the [SBSA specification](https://developer.arm.com/documentation/den0029/i/?lang=en)


## SBSA - Architecture Compliance Suite

SBSA **Architecture Compliance Suite** (ACS) is a collection of self-checking, portable C-based tests.
This suite includes a set of examples of the invariant behaviors that are provided by the SBSA specification, so that implementers can verify if these behaviours have been interpreted correctly.

Most of the tests are executed from UEFI Shell by executing the SBSA UEFI shell application.
A few tests are executed by running the SBSA ACS Linux application which in turn depends on the SBSA ACS Linux kernel module.
The tests can also be executed in a Bare-metal environment. The initialization of the Bare-metal environment is specific to the environment and is out of scope of this document.

## Release details
- **Latest release version:** v8.0.0
- **Execution levels:** Pre-Silicon and Silicon.
- **Scope:** The compliance suite is **not** a substitute for design verification.
- **Access to logs:** Arm licensees can contact Arm through their partner managers.
- For complete coverage of the SBSA rules, availability of an Exerciser is required for Exerciser tests to be run during verficiation at Pre-Silicon level.
- For complete coverage, both SBSA and BSA ACS should be run.

#### SBSA ACS version mapping
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
|   SBSA Spec Version   |   ACS Coverage Mapping   |   SBSA ACS Version   |        SBSA Tag ID         |   BSA ACS Version   |          BSA Tag ID         |    Pre-Si Support    |
|-----------------------|:------------------------:|:--------------------:|:--------------------------:|:-------------------:|:---------------------------:|:--------------------:|
|       SBSA v8.0       |    BSA ACS + SBSA ACS    |      v8.0.0          |   v25.12_SBSA_8.0.0        |        v1.2.0       |       v25.12_BSA_1.2.0      |       Yes            |
|       SBSA v7.2       |    BSA ACS + SBSA ACS    |      v7.2.2          |   v25.03_REL7.2.2          |        v1.1.0       |       v25.03_REL1.1.0       |       Yes            |
|       SBSA v7.2       |    BSA ACS + SBSA ACS    |      v7.2.1          |   v24.11_REL7.2.1          |        v1.0.9       |       v24.11_REL1.0.9       |       Yes            |
|       SBSA v7.1       |    BSA ACS + SBSA ACS    |      v7.2.0 BETA-0   |   v24.03_REL7.2.0_BETA-0   |        v1.0.8       | v24.03_SBSA_REL7.2.0_BETA-0 |       Yes            |
|       SBSA v7.1       |    BSA ACS + SBSA ACS    |      v7.1.5          |   v24.03_REL7.1.5          |        v1.0.8       |       v24.03_REL1.0.8       |       Yes            |
|       SBSA v7.1       |    BSA ACS + SBSA ACS    |      v7.1.4          |   v23.12_REL7.1.4          |        v1.0.7       |       v23.12_REL1.0.7       |       Yes            |
|       SBSA v7.1       |    BSA ACS + SBSA ACS    |      v7.1.3          |   v23.11_BootFramework     |        v1.0.6       |    v23.11_BootFramework     |       Yes            |
|       SBSA v7.1       |    BSA ACS + SBSA ACS    |      v7.1.3          |   v23.09_REL7.1.3          |        v1.0.6       |       v23.09_REL1.0.6       |       Yes            |
|       SBSA v7.1       |    BSA ACS + SBSA ACS    |      v7.1.2          |   v23.07_REL7.1.2          |        v1.0.5       |       v23.07_REL1.0.5       |       Yes            |
|       SBSA v7.1       |    BSA ACS + SBSA ACS    |      v7.1.1 BETA-1   |   v23.03_REL7.1.1_BETA-1   |        v1.0.4       |       v23.03_REL1.0.4       |       Yes            |
|       SBSA v7.1       |    BSA ACS + SBSA ACS    |      v7.1.0 BETA-0   |   v23.01_REL7.1.0_BETA-0   |        v1.0.3       |       v23.01_REL1.0.3       |       Yes            |
|       SBSA v6.1       |    BSA ACS + SBSA ACS    |      v6.1.0          |   v22.10_REL6.1.0          |        v1.0.2       |       v22.10_REL1.0.2       |       Yes            |
|       SBSA v6.0       |    SBSA ACS              |      v3.2            |   v22.07_REL3.2            |          -          |              -              |       Yes            |
|       SBSA v6.0       |    SBSA ACS              |      v3.1            |   v21.09_REL3.1            |          -          |              -              |       Yes            |
|       SBSA v5.0       |    SBSA ACS              |      v2.5            |   v20.08_RELv2.5           |          -          |              -              |       Yes            |
|       SBSA v3.0       |    SBSA ACS              |      v1.6            |   v18.12_REL1.7            |          -          |              -              |       No             |
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#### GitHub branch
  - To pick up the release version of the code, checkout the corresponding **tag** from the **main** branch.
  - To get the latest version of the code with bug fixes and new features, use the **main** branch.

##### Prebuilt release binaries
Prebuilt images for each release are available in the [`prebuilt_images`](../../prebuilt_images/SBSA) folder of the main branch

## Documentation and Guides
- [Arm SBSA Test Scenario Document](arm_sbsa_architecture_compliance_test_scenario.pdf) — algorithms for implementable rules and notes on unimplemented rules.  
- [Arm SBSA Test Checklist](arm_sbsa_testcase_checklist.md) — test categories (UEFI, Linux, Bare-metal) and applicable systems (IR, ES, SR, Pre-Silicon).  
- [Arm SBSA Validation Methodology](arm_sbsa_architecture_compliance_validation_methodology.pdf).  
- [Arm SBSA ACS User Guide](arm_sbsa_architecture_compliance_user_guide.pdf).  
- Bare-metal porting guides  
  - [Arm SBSA ACS Bare-metal User Guide](arm_sbsa_architecture_compliance_bare-metal_user_guide.pdf)  
  - [Bare-metal Code](../../pal/baremetal/)  

 **Note:** The Bare-metal PCIe enumeration code provided with BSA ACS must be used and **must not be replaced**. It is essential for accurate analysis of test results.

- Exerciser VIP guides - The **Exerciser** is a client device wrapped by a PCIe Endpoint, created to satisfy BSA requirements for PCIe capability validation. Running Exerciser tests increases platform coverage.
  - [Exerciser.md](../pcie/Exerciser.md)
  - [Exerciser_API_porting_guide.md](../pcie/Exerciser_API_porting_guide.md).


## SBSA build steps

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
source ShellPkg/Application/sysarch-acs/tools/scripts/acsbuild.sh sbsa
```

#### Build output
The EFI executable is generated at: `workspace/edk2/Build/Shell/DEBUG_GCC/AARCH64/Sbsa.efi`

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
 - `sbsa_acs.ko` : Kernel module which needs to be insmod before running sbsa_app
 - `sbsa_app` : bsa linux app

### Baremetal application
The bare-metal build environment is platform-specific.
For details on generating binaries for bare-metal environments, refer to [README.md](../../pal/baremetal/README.md).


## SBSA run steps

### For UEFI application 

#### Silicon System
On a system with a functional USB port:
1. Copy `Sbsa.efi` to a USB device which is fat formatted.  
2. In UEFI Shell, refresh mappings:
   ```sh
   map -r
   ```
3. Change to the USB filesystem (e.g., `fs0:`).  
4. Run `Sbsa.efi` with appropriate parameters.  
5. Capture UART console output to a log file.

- For application parameters, see the [User Guide](arm_sbsa_architecture_compliance_user_guide.pdf).

#### Emulation environment with secondary storage
1. Create an image containing `Sbsa.efi`:
   ```
   mkfs.vfat -C -n HD0 hda.img 2097152
   sudo mount -o rw,loop=/dev/loop0,uid=$(whoami),gid=$(whoami) hda.img /mnt/sbsa
   sudo cp "<path to application>/Sbsa.efi" /mnt/sbsa/
   sudo umount /mnt/sbsa
   ```
   *(If `/dev/loop0` is busy, select a free loop device.)*
2. Load the image to secondary storage via a backdoor (environment-specific).
3. Boot to UEFI Shell.  
4. Identify the filesystem with `map -r`.  
5. Switch to the filesystem (`fs<x>:`).  
6. Run `Sbsa.efi` with parameters.  
7. Save UART console output for analysis/certification.

- For application parameters, see the [User Guide](arm_sbsa_architecture_compliance_user_guide.pdf).

### For Linux application

1. Copy the sbsa_acs.ko and sbsa_app into USB drive
2. Boot to Linux and identify the USB drive
3. Load the SBSA kernel module
  ```sh
  sudo insmod sbsa_acs.ko
  ```
4. Run the SBSA application
  ```sh
  ./sbsa_app
  ```
5. Remove the SBSA kernel module after run
  ```sh
  sudo rmmod sbsa_acs
  ```
- For application parameters, see the [User Guide](arm_sbsa_architecture_compliance_user_guide.pdf).


## Limitations
- Validating the compliance of certain PCIe rules defined in the SBSA specification requires the PCIe end-point to generate specific stimulus during the runtime of the test. Examples of such stimulus are  P2P, PASID, ATC, etc. The tests that requires these stimuli are grouped together in the exerciser module. The exerciser layer is an abstraction layer that enables the integration of hardware capable of generating such stimuli to the test framework.
The details of the hardware or Verification IP which enable these exerciser tests are platform specific and are beyond the scope of this document.

- The MPAM MSC PCC (ACPI Platform Communication Channel) support has been implemented but not yet verified on any platform. Please raise an issue if any failures or errors are encountered during the ACS run.

 #### Tests dependencies
 - MPAM tests will require EL3 firmware to enable access to MPAM registers from lower EL's.
   If arm trusted firmware is used as EL3 fimrware, enable ENABLE_MPAM_FOR_LOWER_ELS=1 during arm TF build.
   If the above flags are not enabled, MPAM tests can lead to exception at EL3.
 - RAS test will require EL3 firmware to enable access to RAS registers from lower EL's and forward RAS related exceptions to lower EL's.
   If arm trusted firmware is used as EL3 fimrware, enable EL3_EXCEPTION_HANDLING=1 RAS_EXTENSION=1 HANDLE_EA_EL3_FIRST=1 RAS_TRAP_LOWER_EL_ERR_ACCESS=0 during arm TF build
   If the above flags are not enabled, RAS tests can lead to exception at EL3.
 - SBSA Future Requirements ETE test will require EL3 firmware to enable access to Trace registers from lower EL's.
   If arm trusted firmware is used as EL3 fimrware, ENABLE_TRF_FOR_NS=1 ENABLE_TRBE_FOR_NS=1 ENABLE_SYS_REG_TRACE_FOR_NS=1 during arm TF build
   If the above flags are not enabled, ETE tests can lead to exception at EL3.
 - MPAM test have dependency on MPAM, SRAT, HMAT, PPTT tables.
 - RAS test have dependency on AEST, RAS2, SRAT, HMAT, PPTT tables.
 - PMU test have dependency on APMT table.
 - Entrophy rule will require ACS to build with NIST-STS package

**Note:** To build the ACS with NIST Statistical Test Suite, see the [arm SBSA_NIST_User_Guide Document](arm_sbsa_nist_user_guide.md)

|APIs                         |Description                                                                   |Affected tests          |
|-----------------------------|------------------------------------------------------------------------------|------------------------|
|pal_pcie_dev_p2p_support     |Return 0 if the test system PCIe supports peer to peer transaction, else 1    |856, 857                |
|pal_pcie_is_cache_present    |Return 1 if the test system supports PCIe address translation cache, else 0   |852                     |
|pal_pcie_get_legacy_irq_map  |Return 0 if system legacy irq map is filled, else 1                           |850                     |

   Below exerciser capabilities are required by exerciser test.
   - MSI-X interrupt generation.
   - Incoming Transaction Monitoring (order, type).
   - Initiating transactions from and to the exerciser.
   - Ability to check on BDF and register address seen for each configuration address along with access type.

 - SBSA Test 803 (Check ECAM Memory accessibility) execution time depends on the system PCIe hierarchy. For systems with multiple ECAMs the time taken to complete can be long which is normal. Please wait until the test completes.


## License
SBSA ACS is distributed under Apache v2.0 License.

--------------

*Copyright (c) 2018-2025, Arm Limited and Contributors. All rights reserved.*
