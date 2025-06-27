
# PFDI - Architecture Compliance Suite

## PFDI
**PFDI** specification defines a set of functions provided by Platform Firmware to System Software so that System Software can schedule the execution of tests that detect faults in PEs and are executed within or controlled by the Platform Firmware.


## Release details
 - Code Quality: Alpha 0
 - The tests are written for alpha version of the PFDI Specification.

## Downloading PFDI ACS

PFDI ACS code is present in a sub-directory in sysarch-acs repository.

$ git clone "ssh://balgon01@ap-gerrit-1.ap01.arm.com:29418/avk/sysarch-acs" <br/>
$ cd sysarch-acs <br/>

## Building PFDI ACS
### UEFI application
#### Prerequisites

ACS build requires that the following requirements are met, Please skip this if you are using [PFDI Application Build Script](tools/scripts/build_pfdi_uefi.sh).

- Any mainstream Linux based OS distribution.
- git clone EDK2 tree.
- git clone EDK2-libc tree.
- Install GCC-ARM 13.2 [toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads).
- Install the build prerequisite packages to build EDK2. The details of the packages are beyond the scope of this document.

#### Build Steps

$ cd /path/to/sysarch-acs/<br/>
$ source tools/scripts/pfdi_build.sh

#### Build Output

The following output file is created in /path/to/sysarch-acs/workspace/output/:

- pfdi.efi

#### Note : Steps to get toolchain
- wget https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-linux-gnu.tar.xz
- tar -xf arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-linux-gnu.tar.xz
- export GCC49_AARCH64_PREFIX= GCC 13.2 toolchain path pointing to arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-

## Test suite execution in UEFI

### Post-Silicon

On a system where a USB port is available and functional, perform the following steps:

1. Copy 'pfdi.efi' to a USB Flash drive.
2. Plug in the USB Flash drive to one of the functional USB ports on the system.
3. Boot the system to UEFI shell.
4. To determine the file system number of the plugged in USB drive, execute 'map -r' command.
5. Type 'fsx' where 'x' is replaced by the number determined in step 4.
6. To start the compliance tests, run the executable pfdi.efi with the appropriate arguments.

### Emulation environment with secondary storage

On an emulation environment with secondary storage, perform the following steps:

1. Create an image file which contains the 'pfdi.efi' file. For Example: <br/>
$ mkfs.vfat -C -n HD0 hda.img 2097152 <br/>
$ sudo mount hda.img /mnt/pfdi <br/>
$ cd /path/to/sysarch-acs/workspace/output/ <br/>
$ sudo cp pfdi.efi /mnt/pfdi/ <br/>
$ sudo umount /mnt/pfdi
2. Load the image file to the secondary storage using a backdoor. The steps followed to load the image file are Emulation environment specific and beyond the scope of this document.
3. Boot the system to UEFI shell.
4. To determine the file system number of the secondary storage, execute 'map -r' command.
5. Type 'fsx' where 'x' is replaced by the number determined in step 4.
6. To start the compliance tests, run the executable pfdi.efi with the appropriate arguments.

## Application arguments

Command line arguments are similar for uefi application, with some exceptions.

### UEFI

Shell> pfdi.efi [-v &lt;verbosity&gt;] [-skip &lt;test_id&gt;] [-f &lt;filename&gt;]

#### -v
Choose the verbosity level.

- 1 - ERROR
- 2 - WARN and ERROR
- 3 - TEST and above
- 4 - DEBUG and above
- 5 - INFO and above

#### -skip
Overrides the suite to skip the execution of a particular
test. For example, <i>-skip 10</i> skips test 10.

#### -f (Only for UEFI application)
Save the test output into a file in secondary storage. For example <i>-f pfdi.log</i> creates a file pfdi.log with test output.

### UEFI example

Shell> pfdi.efi -v 5 -skip 15,20,30 -f pfdi_uefi.log

Runs PFDI ACS with verbosity INFO, skips test 15, 20 and 30 and saves the test results in <i>pfdi_uefi.log</i>.

## Limitations

 - Since this is a Alpha quality release, contains limited number of tests based on PFDI Specification.

## License
PFDI ACS is distributed under Apache v2.0 License.

## Feedback, contributions, and support

 - For feedback, use the GitHub Issue Tracker that is associated with this repository.
 - For support, send an email to "support-systemready-acs@arm.com" with details.
 - Arm licensees may contact Arm directly through their partner managers.
 - Arm welcomes code contributions through GitHub pull requests. See the GitHub documentation on how to raise pull requests.

--------------

*Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.*
