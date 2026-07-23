# Memory model consistency tests
Memory model consistency tests (litmus tests) are small parallel programs designed to evaluate the correctness and
consistency of a computer system's memory model, particularly in the context of concurrent or parallel programming.
These parallel payloads with specific memory access patterns are run for several iterations to observe how the system
handles memory synchronization, caching, and reordering. By analyzing the outcomes of these tests, developers can gain
insights into the reliability of the memory model implementation and identify any subtle bugs or inconsistencies that
might lead to unpredictable behavior in concurrent programs.

 - NOTE: Running and passing Memory Model consistency tests are not required for SystemReady Certifications and they are not part of BSA Certification Image binary.

## Release details
 - Code quality: v1.0.0 EAC
 - The tests can be run at Silicon level.
 - The tests checks for forbidden behaviors that memory model should not exhibit.
 - For litmus tests scenario and instructions to analyze litmus tests log output, refer the [Memory model tests Scenario & User Guide](../mem_test/memory_model_tests_scenario_user_guide.rst).

## Steps to build litmus tests into bsa-acs
1. Setup edk2 build directory
>          git clone https://github.com/tianocore/edk2.git
>          cd edk2
>          git checkout edk2-stable202511
>          git clone https://github.com/tianocore/edk2-libc.git
>          git submodule update --init --recursive

2. Download source files
>          git clone https://github.com/ARM-software/sysarch-acs.git ShellPkg/Application/sysarch-acs
>          git clone https://github.com/relokin/kvm-unit-tests.git ShellPkg/Application/sysarch-acs/mem_test/kvm-unit-tests
>          git -C ShellPkg/Application/sysarch-acs/mem_test/kvm-unit-tests checkout target-efi-bsa

3. Build bsa-acs UEFI app <br>
Note :  Install GCC-ARM 14.3 [toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
>          export GCCNOLTO_AARCH64_PREFIX=<path to CC>arm-gnu-toolchain-14.3.Rel1-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-
>          export PACKAGES_PATH=`pwd`/edk2-libc
>          source edksetup.sh
>          make -C BaseTools/Source/C
>          source ShellPkg/Application/sysarch-acs/tools/scripts/acsbuild.sh mem_test

4. BSA EFI application path
- The EFI executable file is generated at <edk2-path>/Build/Shell/DEBUG_GCCNOLTO/AARCH64/Bsa.efi

## Verification on Arm Neoverse RD-V3-Cfg1 reference design
### Software stack and model
Follow the steps in the [RD-V3-Cfg1 platform software user guide](https://neoverse-reference-design.docs.arm.com/en/latest/platforms/rdv3cfg1.html) to obtain the RD-V3-Cfg1 FVP.

#### Prerequisites
`sudo` permission is required to build the RD-V3-Cfg1 software stack.

#### Software stack build instructions
Follow the **BusyBox Boot** link under **Supported Features by RD-V3-Cfg1 platform software stack** in the RD-V3-Cfg1 platform software user guide.

### Model run command
1. Set the `MODEL` environment variable to the absolute path of the RD-V3-Cfg1 FVP binary.
>          export MODEL=<absolute path to the RD-V3-Cfg1 FVP binary/FVP_RD_V3_Cfg1>

2. Follow the steps provided in [Emulation environment with secondary-storage](../bsa/README.md#emulation-environment-with-secondary-storage) to create an `.img` file containing the Bsa.efi executable.

3. Launch the RD-V3-Cfg1 FVP with the image containing Bsa.efi.
>          cd <path to RD-V3-Cfg1 software stack>/model-scripts/rdinfra/platforms/rdv3cfg1
>          ./run_model.sh -v <path to .img containing Bsa.efi>

4. Press Esc to enter the shell prompt, navigate to the disk containing Bsa.efi, and run the BSA app.

**Note:** Tests B_MEM_01 (102) and PCI_IN_19 (830) are known to hang on RD-V3-Cfg1 due to model limitations. These tests must be skipped for the test run to progress.
>          Bsa.efi -skip 102,830

## Limitations
 - The kvm-unit-tests print function depends on SPCR ACPI table for UART base address and UEFI console setting must be set to "serial". In case of non-availability of SPCR,
   set `CONFIG_UART_EARLY_BASE` in `sysarch-acs/mem_test/kvm-unit-tests/lib/arm/io.c` to UART base address of system under test, after step 2 in [build steps](#steps-to-build-litmus-tests-into-bsa-acs).
 - Initializing memory model tests infra and transitioning from EL1 to EL2 is observed to be time-consuming (approximately 30 minutes) in systems with large PE caches. Memory tests run significantly slower on Arm FVP models, with each test taking around one hour to complete depending on the host machine load.
 - The printf implementation in kvm-unit-tests works only with a serial interface. For HDMI or other outputs, a console splitter or similar device is required.

## Feedback, contributions, and support

 - For feedback, use the GitHub Issue Tracker that is associated with this repository.
 - For support, send an email to "support-systemready-acs@arm.com" with details.
 - Arm licensees may contact Arm directly through their partner managers.
 - Arm welcomes code contributions through GitHub pull requests. See the GitHub documentation on how to raise pull requests.

*Copyright (c) 2023-2026, Arm Limited and Contributors. All rights reserved.*
