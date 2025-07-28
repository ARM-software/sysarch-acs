# Personal Computing Base System Architecture - Architecture Compliance Suite

## Personal Computing Base System Architecture
**PC Base System Architecture** (PC-BSA) specifies a standard hardware system architecture for Personal Computers (PCs) that are based on the Arm 64-bit Architecture. PC system software, for example operating systems, hypervisors, and firmware can rely on this standard system architecture. PC-BSA extends the requirements specified in the [Arm BSA specification](https://developer.arm.com/documentation/den0094/latest/).

For more information, please refer the [PC-BSA specification](https://developer.arm.com/documentation/den0151/latest).

## Release Details
 - Code Quality: BETA
 - The tests are written for version 1.0 of the PC BSA specification.
 - For more details on tests implemented in this release, Please refer [PC-BSA Test Scenario Document](arm_pc-bsa_architecture_compliance_test_scenario.pdf).

## PC BSA Coverage Overview
The PC BSA tests are distributed across various ACS components, including SCT tests, UEFI-based tests, and a Linux-based test. Full compliance also requires running the Bsa.efi application and includes some manual verification.

### Below are the sections that collectively cover PC BSA rules:
- [UEFI-based Tests](#pc-bsa-uefi-shell-application-build-instructions)
- [Linux-based Tests](#pc-bsa-linux-application)
- [SCT-based Tests](#guidance-on-running-sct-testcase)
- [BSA Uefi Coverage](#guidance-on-running-bsa-uefi-coverage)


## PC BSA UEFI Shell Application Build Instructions

### Prerequisites
Before starting the build, ensure that the following requirements are met.
- Any mainstream Linux based OS distribution running on a x86 or AArch64 machine.
- Install GCC-ARM 13.2 [toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads).
- Install the build prerequisite packages to build EDK2.<br>
Note: The details of the packages are beyond the scope of this document.

### 1. Build Steps

##### 1. Setup edk2 build directory

>	1. git clone --branch edk2-stable202505 --depth 1 https://github.com/tianocore/edk2<br>
>	2. git clone https://github.com/tianocore/edk2-libc edk2/edk2-libc<br>
>	3. cd edk2<br>
>	4. git submodule update --init --recursive<br>

##### 2. Download source files
>	 git clone https://github.com/ARM-software/sysarch-acs ShellPkg/Application/sysarch-acs <br>

##### 3. Build PC-BSA UEFI Application <br>
Note :  Install GCC-ARM 13.2 from [toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) <br>
For a x86 host build,
>  export GCC_AARCH64_PREFIX=<path to arm-gnu-toolchain-13.2.Rel1-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-><br>

For an AArch64 build,
>  export GCC_AARCH64_PREFIX=/usr/bin/aarch64-linux-gnu-

Build the binary,
> 1. export PACKAGES_PATH=$PWD/edk2-libc<br>
> 2. source edksetup.sh<br>
> 3. make -C BaseTools/Source/C<br>
> 4. source ShellPkg/Application/sysarch-acs/tools/scripts/acsbuild.sh pc_bsa<br>

##### 4. PC BSA EFI application path
- The EFI executable file is generated at \<edk2-path\>/Build/Shell/DEBUG_GCC/AARCH64/PC_Bsa.efi

### 2. Execution Steps
The execution of the compliance suite varies depending on the test environment. These steps assume that the test suite is invoked through the ACS UEFI shell application

#### 2.1 Emulation environment with secondary storage
On an emulation environment with secondary storage, perform the following steps:

1. Create an image file which contains the '.efi' file. For example:
> 1. mkfs.vfat -C -n HD0 hda.img 2097152<br>
> 2. sudo mount hda.img /mnt/acs/<br>
> 3. sudo cp PC_Bsa.efi /mnt/acs<br>
> 4. sudo umount /mnt/acs<br>

Note: If /mnt/acs/ is not already created, you may need to create it using mkdir -p /mnt/acs/.

2. Load the image file to the secondary storage using a backdoor. The steps to load the image file are emulation environment-specific and beyond the scope of this document.
3. Boot the system to UEFI shell.
4. To determine the file system number of the secondary storage, execute 'map -r' command.
5. Type 'fs<x>' where '<x>' is replaced by the number determined in step 4.
6. To start the compliance tests, run the executable PC_Bsa.efi with the appropriate parameters.
7. Copy the UART console output to a log file for analysis.

#### 2.2 Emulation environment without secondary storage

On an emulation platform where secondary storage is not available, perform the following steps:

1. Add the path to PC_bsa.efi file in the UEFI FD file.
2. Build UEFI image including the UEFI Shell.
3. Boot the system to UEFI shell.
4. Copy the UART console output to a log file for analysis.

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
Overrides the suite to skip the execution of a particular
test. For example, <i>-skip 10</i> skips test 10.

#### -f (Only for UEFI application)
Save the test output into a file in secondary storage. For example <i>-f pc_bsa.log</i> creates a file pc_bsa.log with test output.

#### -fr
Use this option to run PC BSA tests intended for future requirement validation.

#### -help
Displays all available command-line arguments and their usage.

### UEFI example

Shell> PC_Bsa.efi -v 1 -skip 15,20,30 -f pcbsa_uefi.log

Runs PCBSA ACS with verbosity INFO, skips test 15, 20 and 30 and saves the test results in <i>pcbsa_uefi.log</i>.

## PC BSA Linux Application

### 1.Build Steps
1. wget https://gitlab.arm.com/linux-arm/linux-acs/-/raw/master/acs-drv/files/build.sh
2. chmod +x build.sh
3. source build.sh

### 2. Build Output

The following output folder is created in __build__ folder:
(As part of build bsa and sbsa module and app also got created)
 - pcbsa_acs.ko
 - pcbsa_app

### 3. Loading the kernel module
Before the PC BSA ACS Linux application can be run, load the PC BSA ACS kernel module respectively using the insmod command.

```sh
shell> insmod pcbsa_acs.ko
```

### 3. Running PC BSA ACS

```sh
shell> ./pcbsa_app
```
### 4. PC BSA Linux Test Log View

```sh
shell> sudo dmesg | tail -500 # print last 500 kernel logs
```
After the run is complete, you can remove the PC BSA module from the system if it is no longer needed.

```sh
shell> sudo rmmod bsa_acs
shell> sudo rmmod sbsa_acs
```
- For information on the PC BSA Linux build parameters and limitation, see the [**Build Parameters and Limitations**](../../README.md#build-script-arguments)

## Running PC-BSA UEFI Application on TC3 Platform
**Note:** This section outlines instructions to run the PC-BSA UEFI application out of box using TC3 Arm FVP.

### 1. Downloading and Building TC3 software stack

Please follow instructions provided in [**LSC23 TC3 Setup Guide**](https://totalcompute.docs.arm.com/en/lsc23.1/totalcompute/lsc23/user-guide.html) to download and build TC3 software stack.

### 2. Download the TC3 FVP Model

Refer to the official Arm Total Compute FVP download page: [**TC3 FVP - Arm Developer**](https://developer.arm.com/Tools%20and%20Software/Fixed%20Virtual%20Platforms/Total%20Compute%20FVPs)


### 3. Set Up Environment Variable

```bash
export MODEL=<path of FVP_TC3>
```

### 4. Prepare ACS Disk Image

Follow the steps in [**Emulation environment with secondary-storage**](#21-emulation-environment-with-secondary-storage) to create a FAT-formatted `.img` file containing the `PC_bsa.efi` binary.


### 5. Update ACS Image Path in Model Run Script

- **Note:** Replace <TC_WORKSPACE> with actual working directory created during [**Downloading and Building TC3 software stack**](#1-downloading-and-building-tc3-software-stack)

```bash
sed -i 's|ACS_DISK_IMAGE="\$DEPLOY_DIR/systemready_acs_live_image.img"|ACS_DISK_IMAGE="\$DEPLOY_DIR/hda.img"|' <TC_WORKSPACE>/run-scripts/common/run_model.sh
```

- Copy `hda.img` created in step [**Emulation environment with secondary-storage**](#21-emulation-environment-with-secondary-storage) to `<TC_WORKSPACE>/output/tc3/debian-official/fvp/deploy/` directory.


### 6. Run the TC3 Model

Navigate to the TC3 run-script directory and launch the model:

```bash
cd <TC_WORKSPACE>/run-scripts/tc3
./run_model.sh -m $MODEL -d acs-test-suite
```

### 7. Run the PC BSA Test from UEFI Shell

- When the model boots, press **Esc** to enter the UEFI Boot Manager.
- Select **Built-in EFI Shell**
- Execute the following commands:

```shell
map -r          # Refresh and list file systems
fsX:            # Replace X with the correct disk number
PC_bsa.efi      # Launch the PC BSA test
```

### Notes

- Ensure the `.efi` binary is copied correctly into the image using the steps in the [secondary-storage setup section]((#21-emulation-environment-with-secondary-storage).
- Capture UART output logs for test verification and reporting.

## Guidance on running SCT testcase
- PC-BSA rules `P_L1NV_01` and `P_L1SE_01` requires [VariableServicesTest](https://github.com/tianocore/edk2-test/tree/master/uefi-sct/SctPkg/TestCase/UEFI/EFI/RuntimeServices/VariableServices/BlackBoxTest) SCT test to be run for compliance check. Please refer to [BBR ACS User Guide](https://github.com/ARM-software/bbr-acs/blob/main/README.md) for instructions to build and run SCT test suite. see the [SCT User Guide](http://www.uefi.org/testtools)for instructions on choosing and running individual testcase from UEFI Shell.

## Guidance on running Bsa Uefi Coverage
- Rule **P_L1_01** is covered by BSA ACS. Compliance requires running Bsa.efi. Refer to the [BSA ACS README](../../README.md#bsa-architecture-compliance-suite) for instructions to build and run Bsa.efi from the UEFI Shell.


## Limitations

- The PC BSA tests are distributed across various ACS components, including SCT tests, UEFI-based tests, and a Linux-based test. To achieve complete validation, all test suites must be executed. Additionally, some rules require manual verification by the DUT owner, for these rules compliance must be manually declared to confirm PC BSA compliance for the DUT. see the [PC BSA testcase checklist](../../docs/pc_bsa/arm_pc-bsa_testcase_checklist.rst) for rules not testable by ACS.

## License
PC BSA ACS is distributed under Apache v2.0 License.

## Feedback, contributions, and support

 - For feedback, use the GitHub Issue Tracker that is associated with this repository.
 - For support, send an email to "support-systemready-acs@arm.com" with details.
 - Arm licensees may contact Arm directly through their partner managers.
 - Arm welcomes code contributions through GitHub pull requests. See the GitHub documentation on how to raise pull requests.

--------------

*Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.*
