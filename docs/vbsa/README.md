# Virtual Base System Architecture - Architecture Compliance Suite

## Virtual Base System Architecture
**Virtual Base System Architecture** (VBSA) specifies the requirements and run-time features that a base Virtual
Environment (VE) needs to install, boot, and run an operating system. VBSA specification is a supplement to the  [Arm BSA specification](https://developer.arm.com/documentation/den0094/latest/).

For more information, please refer the [VBSA specification](https://developer.arm.com/documentation/den0150/latest/).

## Table of Contents
* [Release Details](#release-details)
* [VBSA Coverage Overview](#vbsa-coverage-overview)
* [UEFI Shell Build Instructions](#vbsa-uefi-shell-application-build-instructions)
* [Application arguments](#application-arguments)
* [VBSA Linux Application](#vbsa-linux-application)
* [Limitations](#limitations)
* [Troubleshoot guide](#troubleshoot-guide)
* [License](#license)
* [Feedback and support](#feedback-contributions-and-support)


## Release Details
 - Code Quality: BETA
 - The tests are written for version 1.0 of the VBSA specification.
 - For more details on tests implemented in this release, Please refer [VBSA Test Scenario Document](arm_vbsa_architecture_compliance_test_scenario.md).

## VBSA Coverage Overview
The VBSA tests are primarily UEFI-based and run on the virtual platform’s UEFI firmware, with a subset of tests executed in a Linux guest environment.

### Below are the sections that collectively cover VBSA rules:
- [UEFI-based Tests](#vbsa-uefi-shell-application-build-instructions)
- [Linux-based Tests](#vbsa-linux-application)


## VBSA UEFI Shell Application Build Instructions

### Prerequisites
Before starting the build, ensure that the following requirements are met.
- Any mainstream Linux based OS distribution running on a x86 or AArch64 machine.
- Install GCC-ARM 14.3 [toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads).
- Install the build prerequisite packages to build EDK2.<br>
Note: The details of the packages are beyond the scope of this document.

### 1. Build Steps

#### 1. Setup edk2 build directory

>	1. git clone --branch edk2-stable202505 --depth 1 https://github.com/tianocore/edk2<br>
>	2. git clone https://github.com/tianocore/edk2-libc edk2/edk2-libc<br>
>	3. cd edk2<br>
>	4. git submodule update --init --recursive<br>

#### 2. Download source files
>	 git clone https://github.com/ARM-software/sysarch-acs ShellPkg/Application/sysarch-acs <br>

#### 3. Build VBSA UEFI Application <br>
Note :  Install GCC-ARM 14.3 from [toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) <br>
For a x86 host build,
>  export GCC_AARCH64_PREFIX=<path to arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-><br>

For an AArch64 build,
>  export GCC_AARCH64_PREFIX=/usr/bin/aarch64-linux-gnu-

Build the binary,
> 1. export PACKAGES_PATH=$PWD/edk2-libc<br>
> 2. source edksetup.sh<br>
> 3. make -C BaseTools/Source/C<br>
> 4. source ShellPkg/Application/sysarch-acs/tools/scripts/acsbuild.sh vbsa<br>

#### 4. VBSA EFI application path
- The EFI executable file is generated at \<edk2-path\>/Build/Shell/DEBUG_GCC/AARCH64/Vbsa.efi

### 2. Execution Steps
VBSA ACS is provided as a UEFI application (Vbsa.efi) and is intended to run on top of the virtual platform’s UEFI Shell.
Because different virtual environments have different boot media requirements (e.g. .img, .vhdx, etc.), the exact steps can vary.

Below are examples for 2 common cases:
 - Raw disk image (.img)
 - Virtual Hard Disk (VHDX)

In both the cases, you must create a bootable image that contains:
 - \Vbsa.efi
 - \EFI\BOOT\bootaa64.efi (this is the UEFI shell binary, typically Shell.efi)

#### 2.1 Creating a bootable .img

Environment - Any mainstream Linux distro
1. mkfs.vfat -C -n HD0 vbsa.img 1048576<br>
2. sudo mount vbsa.img /mnt/acs/<br>
3. sudo mkdir -p /mnt/acs/EFI/BOOT<br>
4. sudo cp Shell.efi /mnt/acs/EFI/BOOT/bootaa64.efi<br>
5. sudo cp Vbsa.efi /mnt/acs<br>
6. sudo umount /mnt/acs<br>

Note: If /mnt/acs/ is not already created, you may need to create it using mkdir -p /mnt/acs/. You can obtain Shell.efi from the pre-built images provided with the VBSA ACS release.

#### 2.2 Creating a bootable .vhdx

Environment - Windows 10/11 OS
1. Create a VHDX image using Hyper-V Manager or Disk Management.
2. Attach/mount the VHDX in Windows.
3. Inside the mounted VHDX - create the \EFI\BOOT boot directory
4. Copy Shell.efi to \EFI\BOOT\bootaa64.efi
5. Copy Vbsa.efi to the root \Vbsa.efi
6. Safely eject the VHDX

#### 2.3 Running the Vbsa.efi binary

1. Boot the Virtual environment using vbsa.img/vbsa.vhdx file. The steps to load the image file are environment-specific and beyond the scope of this document.
2. Boot the VE to UEFI shell.
3. To determine the file system number, execute 'map -r' command.
4. Type 'fsx:' where 'x' is replaced by the number determined in step 4.
5. To start the compliance tests, run the executable Vbsa.efi with the appropriate parameters.
6. Copy the UART console output to a log file for analysis.

## Application arguments
Command line arguments are similar for uefi application, with some exceptions.
Some of the mostly used uefi arguments are listed below,

#### -v
Choose the verbosity level.

- 5 - ERROR
- 4 - WARN and ERROR
- 3 - TEST and above
- 2 - DEBUG and above
- 1 - INFO and above

#### -skip
Overrides the suite to skip the execution of a particular rule.
For example, <i>-skip V_L1PE_01</i> skips the rule V_L1PE_01.

#### -r
Overrides the suite to run a particular rule.
For example, <i>-r V_L1PE_02</i> runs only V_L1PE_02

#### -f
Save the test output into a file. For example <i>-f vbsa.log</i> creates a file vbsa.log with test output.

#### -fr
Use this option to run VBSA tests intended for future requirement validation.

#### -help
Displays all available command-line arguments and their usage.

#### -el1physkip
Skips tests that access the EL1 physical timer.

#### Sample command line

Shell> Vbsa.efi -v 1 -fr -skip V_L2PE_01 -f vbsa.log

Runs VBSA ACS including FR rules with verbosity INFO, skips V_L2PE_01 and saves the test results in <i>vbsa.log</i>.

## VBSA Linux Application
The VBSA ACS Linux application is run in the same way as the BSA ACS Linux application, skipping rules that are not required for VBSA ACS.
### 1.Build Steps
1. wget https://gitlab.arm.com/linux-arm/linux-acs/-/raw/master/acs-drv/files/build.sh
2. chmod +x build.sh
3. source build.sh

### 2. Build Output

The following output folder is created in __build__ folder:
(As part of build pc-bsa and sbsa module and app also gets created)
 - bsa_acs.ko
 - bsa_app

### 3. Loading the kernel module
Before the BSA ACS Linux application can be run, load the BSA ACS kernel module respectively using the insmod command.

```sh
shell> insmod bsa_acs.ko
```

### 3. Running BSA ACS while skipping rules that are not required for VBSA ACS

```sh
shell> ./bsa_app --skip B_REP_1,B_IEP_1,B_PCIe_11,B_MEM_06
```
### 4. BSA Linux Test Log View

```sh
shell> sudo dmesg | tail -500 # print last 500 kernel logs
```
After the run is complete, you can remove the BSA module from the system if it is no longer needed.

```sh
shell> sudo rmmod bsa_acs
```


## Limitations

- The VBSA tests are distributed across various ACS components - UEFI-based tests, and Linux-based tests. To achieve complete validation, all test suites must be executed. Additionally, some rules require manual verification by the VE owner, for these rules compliance must be manually declared to confirm VBSA compliance for the VE. see the [VBSA testcase checklist](arm_vbsa_testcase_checklist.md) for rules not testable by ACS.

- Several PCIe rules require the Exerciser VIP to achieve complete coverage. Exerciser-based tests cannot be executed in virtual environments. These rules are reported as PASSED*(PARTIAL) or SKIPPED depending on other tests that cover the rule.

## Troubleshoot guide

- When running VBSA ACS in virtual environments, some hypervisors may trap EL1 physical timer accesses, which can cause certain VBSA ACS tests to raise exceptions. In such cases, use the -el1physkip command line parameter to skip EL1 physical timer accesses. Note that using this parameter reduces test coverage, and the VE owner must manually verify the skipped rules to achieve compliance.

## License
VBSA ACS is distributed under Apache v2.0 License.

## Feedback, contributions, and support

 - For feedback, use the GitHub Issue Tracker that is associated with this repository.
 - For support, send an email to "support-systemready-acs@arm.com" with details.
 - Arm licensees may contact Arm directly through their partner managers.
 - Arm welcomes code contributions through GitHub pull requests. See the GitHub documentation on how to raise pull requests.

--------------

*Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.*
