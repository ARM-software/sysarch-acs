# Personal Computing Base System Architecture - Architecture Compliance Suite

## Table of Contents

- [Personal Computing Base System Architecture](#personal-computing-base-system-architecture)
- [PC BSA - Architecture Compliance Suite](#pc-bsa---architecture-compliance-suite)
- [Release details](#release-details)
- [Documentation & Guides](#documentation--guides)
- [PC BSA coverage overview](#pc-bsa-coverage-overview)
- [PC BSA build steps](#pc-bsa-build-steps)
  - [UEFI Shell application](#uefi-shell-application)
  - [Linux application](#linux-application)
- [PC BSA run steps](#pc-bsa-run-steps)
  - [For UEFI application](#for-uefi-application)
  - [For Linux application](#for-linux-application)
- [RDV3CFG1 reference flow](#rdv3cfg1-reference-flow)
- [Application arguments](#application-arguments)
- [Coverage guidance](#coverage-guidance)
- [Limitations](#limitations)
- [Feedback, contributions, and support](#feedback-contributions-and-support)
- [License](#license)

## Personal Computing Base System Architecture
**Personal Computing Base System Architecture** (PC BSA) defines hardware and
debug requirements for Arm-based personal computing platforms. It builds on the
Arm **BSA** ruleset and adds PC-specific constraints so that operating systems
and firmware interoperate consistently across implementations. Refer to the
[PC BSA specification](https://developer.arm.com/documentation/den0151/latest)
for the authoritative rule set.

## PC BSA - Architecture Compliance Suite
The **PC BSA Architecture Compliance Suite** provides self-checking tests that
exercise the PC BSA rules across UEFI and Linux execution environments. Most
rules run inside the UEFI shell via `PC_bsa.efi`; OS-visible behavior is
validated by the Linux application and its kernel module companion.

## Release details
- **Code quality:** EAC
- **Latest release version:** v1.0.0
- **Release tag:** `v25.12_PCBSA_1.0.0`
- **Specification coverage:** PC BSA v1.0
- **Execution levels:** Pre-Silicon and Silicon
- **Scope:** ACS is **not** a substitute for full design verification.
- **Prebuilt binaries:** [`prebuilt_images/PCBSA/v25.12_PCBSA_1.0.0`](../../prebuilt_images/PCBSA/v25.12_PCBSA_1.0.0)

## Documentation & Guides
- [PC BSA specification](https://developer.arm.com/documentation/den0151/latest)
- [Arm PC BSA Testcase Checklist](arm_pc-bsa_testcase_checklist.md)
- [Arm PC BSA Test Scenario Document](TPM_PAL_Porting_Guide.md)
- [Common UEFI build guide](../common/uefi_build.md)
- [Common Linux application guide](../common/linux_build.md)
- [Common CLI arguments](../common/cli_args.md)

## PC BSA coverage overview
PC BSA rules are implemented across multiple ACS components. Run every path
below to claim complete coverage.

### Sections covering PC BSA rules
- [UEFI-based tests](#for-uefi-application) — execute `PC_bsa.efi` (or the RDV3CFG1
  reference flow) with the required modules, filters, and logging flags.
- [Linux-based tests](#for-linux-application) — load `pcbsa_acs.ko` and run
  `pcbsa_app` to exercise OS-visible functionality.
- [SCT-based tests](#coverage-guidance) — run the Variable Services suite and
  any other SCT content referenced by the checklist.
- [BSA ACS dependencies](#coverage-guidance) — execute `Bsa.efi` plus the BSA
  Linux artifacts mandated by the PC BSA ruleset for shared requirements.
- [Manual evidence](#coverage-guidance) — document DUT-owner verification items
  called out in the [PC BSA testcase checklist](arm_pc-bsa_testcase_checklist.md).

## PC BSA build steps

### UEFI Shell application
1. Configure edk2, the toolchain, and the workspace using the
  [Common UEFI build guide](../common/uefi_build.md).
2. Build the PC BSA binary:\
    `source ShellPkg/Application/sysarch-acs/tools/scripts/acsbuild.sh pcbsa`
3. Retrieve `PC_bsa.efi` from `Build/Shell/<TOOL_CHAIN_TAG>/AARCH64/`.

### Linux application
1. Download the shared Linux ACS build script:\
    `wget https://gitlab.arm.com/linux-arm/linux-acs/-/raw/master/acs-drv/files/build.sh`\
    `chmod +x build.sh`
2. Build the drivers and applications:\
    `source build.sh`
3. The build artifacts appear under `build/`:\
    `pcbsa_acs.ko`\
    `pcbsa_app`\
    `bsa_acs.ko`, `bsa_app`, `sbsa_acs.ko`, `sbsa_app` (also produced)
4. For build-script arguments and limitations, see the
  [Common Linux application guide](../common/linux_build.md).

## PC BSA run steps

### For UEFI application
#### Silicon system
1. Copy `PC_bsa.efi` to a FAT-formatted USB drive.
2. Boot the DUT to the UEFI shell, refresh mappings, and locate the filesystem:\
    `Shell> map -r`\
    `Shell> fs0:`
3. Run `PC_bsa.efi` with the required parameters (see [Common CLI arguments](../common/cli_args.md)).
4. Capture UART console output for reporting.

**Example**

`Shell> PC_bsa.efi -v 1 -skip P_L1PE_01,P_L1GI_01 -f pcbsa_uefi.log`

Runs PCBSA ACS with verbosity INFO, skips rules `P_L1PE_01`/`P_L1GI_01`and stores the UART output in `pcbsa_uefi.log`.

> Use PC BSA rule IDs that follow the `P_L<level><module>_<nn>` pattern defined in
  [PC BSA checklist](arm_pc-bsa_testcase_checklist.md)
  (for example, `P_L1PE_01`, `P_L1GI_01`) when filtering, and record any resulting
  coverage gap.

#### Emulation environment with secondary storage
1. Create a FAT image containing `PC_bsa.efi`:\
    `mkfs.vfat -C -n HD0 hda.img 2097152`\
    `sudo mount -o rw,loop=/dev/loop0,uid=$(whoami),gid=$(whoami) hda.img /mnt/pcbsa`\
    `sudo cp "<path to>/PC_bsa.efi" /mnt/pcbsa/`\
    `sudo umount /mnt/pcbsa`\
    *(Pick a free loop device if `/dev/loop0` is busy.)*
2. Attach the image to the virtual platform per the model documentation.
3. Boot to the UEFI shell.
4. Refresh mappings and locate the filesystem:\
    `map -r`
5. Switch to the ACS filesystem, launch `PC_bsa.efi` with the required parameters, and capture UART logs for evidence.

#### Emulation environment without secondary storage
Some emulation platforms embed binaries directly into the firmware image instead of exposing a virtual disk. In that case:
1. Add the path of `PC_bsa.efi` to the UEFI FD image used by the model.
2. Rebuild the UEFI image so the binary is packaged alongside the UEFI shell.
3. Boot the platform to the UEFI shell.
4. Launch `PC_bsa.efi` with the desired arguments (see [Common CLI arguments](../common/cli_args.md))
5. Capture the UART console output for analysis.

### For Linux application
1. Copy `pcbsa_acs.ko` and `pcbsa_app` to the system.
2. Load the kernel module:\
    `sudo insmod pcbsa_acs.ko`
3. Run the user-space application (see [Common CLI arguments](../common/cli_args.md)).\
    `./pcbsa_app`
4. Inspect kernel logs as needed:\
    `sudo dmesg | tail -500`
5. Remove the module when testing completes:\
    `sudo rmmod pcbsa_acs`
    *(Unload `bsa_acs` and `sbsa_acs` if they are no longer required.)*

## RDV3CFG1 reference flow
Use this sequence to run the PC BSA UEFI application on the RDV3CFG1 Arm FVP.

1. Build the software stack and download the FVP for the RDV3CFG1 model by following the
  [RDV3CFG1 Setup Guide](https://developer.arm.com/documentation/102858/0201/Running-BSA-ACS-tests-on-the-Arm-Neoverse-V3-reference-design--RD-V3--Fixed-Virtual-Platform--FVP--model/Setting-up-the-RD-V3-Cfg1-FVP?lang=en).
2. Export the model path:\
    `export MODEL=<path of FVP_RDV3CFG1>`
3. Prepare the ACS disk image as described above [FAT image with `PC_bsa.efi`](#emulation-environment-with-secondary-storage)
4. Launch the model:\
    `cd <RDV3CFG1_WORKSPACE>/model-scripts/rdinfra/platforms/rdv3cfg1`\
    `./run_model.sh –v <path of hda.img>`
5. In UEFI Shell, press **Esc**, choose **Built-in EFI Shell**, refresh
  mappings, switch to the ACS filesystem, and run `PC_bsa.efi`.

### Application arguments
Refer to [Common CLI arguments](../common/cli_args.md) for detailed flag
descriptions, logging options, and sample invocations.\

## Coverage guidance
- Execute **UEFI**, **Linux**, and required **SCT** content to claim full PC BSA
  coverage.
- Run the **BSA ACS** content (`Bsa.efi` plus any required Linux modules) in
  addition to PC BSA to satisfy the cross-specification dependencies documented
  in the PC BSA checklist.
- Rules `P_L1NV_01` and `P_L1SE_01` require the
  [VariableServicesTest](https://github.com/tianocore/edk2-test/tree/master/uefi-sct/SctPkg/TestCase/UEFI/EFI/RuntimeServices/VariableServices/BlackBoxTest)
  from the SCT suite.
- Refer to the [BBR ACS User Guide](https://github.com/ARM-software/bbr-acs/blob/main/README.md)
  and the [SCT User Guide](http://www.uefi.org/testtools) for guidance on
  building and selecting SCT test cases.

## Limitations
- Coverage spans UEFI tests, Linux tests, SCT content, and manual verification.
  Some rules require DUT-owner evidence; document those explicitly in reports.
- Exerciser-dependent PCIe features (P2P, PASID, ATC, and similar) require
  appropriate hardware stimulus; without it the respective rules must be marked
  partial or skipped.

## Feedback, contributions and support

- Email: [support-systemready-acs@arm.com](mailto:support-systemready-acs@arm.com)
- GitHub Issues: [sysarch-acs issue tracker](https://github.com/ARM-software/sysarch-acs/issues)
- Contributions: [GitHub Pull Requests](https://github.com/ARM-software/sysarch-acs/pulls)

## License
PC BSA ACS is distributed under the [Apache v2.0 License](https://www.apache.org/licenses/LICENSE-2.0)

--------------

Copyright (c) 2025-2026, Arm Limited and Contributors. All rights reserved.
