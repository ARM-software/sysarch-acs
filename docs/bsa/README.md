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
- [Feedback, contributions and support](#feedback-contributions-and-support)
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
- **Code quality:** BETA
- **Latest release version:** v1.2.0
- **Release tag:** `v25.12_BSA_1.2.0`
- **Specification coverage:** BSA v1.2
- **Execution levels:** Pre-Silicon and Silicon.
- **Scope:** The compliance suite is **not** a substitute for design verification.
- **Prebuilt binaries:** [`prebuilt_images/BSA/v25.12_BSA_1.2.0`](../../prebuilt_images/BSA/v25.12_BSA_1.2.0)
- **Access to logs:** Arm licensees can contact Arm through their partner managers.

> **For complete coverage of PCIe capability rules**
> - Run the Exerciser VIP alongside the BSA ACS to exercise PCIe endpoint behaviors referenced by the specification.
> - Capture Exerciser logs with the test output to demonstrate compliance evidence.

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
Prebuilt images for each release, including [`v25.12_BSA_1.2.0`](../../prebuilt_images/BSA/v25.12_BSA_1.2.0), are available in the [`prebuilt_images`](../../prebuilt_images/BSA) folder of the main branch.

## Documentation and Guides
- [Arm BSA Test Scenario Document](arm_bsa_architecture_compliance_test_scenario.pdf) — algorithms for implementable rules and notes on unimplemented rules.
- [Arm BSA Test Checklist](arm_bsa_testcase_checklist.md) — test categories (UEFI, Linux, Bare-metal) and applicable systems.
- [Arm BSA Validation Methodology](arm_bsa_architecture_compliance_validation_methodology.pdf).
- [Arm BSA ACS User Guide](arm_bsa_architecture_compliance_user_guide.pdf).
- Bare-metal porting guides:
  - [Arm BSA ACS Bare-metal User Guide](arm_bsa_architecture_compliance_bare-metal_user_guide.pdf)
  - [Bare-metal Code](../../pal/baremetal/)
- Exerciser VIP guides (the Exerciser PCIe endpoint increases coverage of PCIe-capability rules):
  - [Exerciser.md](../pcie/Exerciser.md)
  - [Exerciser_API_porting_guide.md](../pcie/Exerciser_API_porting_guide.md)

> **Important:** The Bare-metal PCIe enumeration code shipped with BSA ACS **must not be replaced**; it is required for accurate analysis of test results.

## BSA build steps

### UEFI Shell application

Follow the [Common UEFI build guide](../common/uefi_build.md) to set up the
edk2 workspace and Arm toolchain, then run:

- `source ShellPkg/Application/sysarch-acs/tools/scripts/acsbuild.sh bsa      # ACPI flow`
- `source ShellPkg/Application/sysarch-acs/tools/scripts/acsbuild.sh bsa_dt   # Device Tree flow`

Both flows emit `Bsa.efi` under `Build/Shell/<TOOL_CHAIN_TAG>/AARCH64/` inside
the edk2 workspace.

### Linux application

Certain Peripheral, PCIe, and Memory Map tests require Linux.\
Use the [Common Linux application guide](../common/linux_build.md) for the shared build
script, arguments, and output locations.

- The BSA artifacts are `bsa_acs.ko` and `bsa_app`, generated under `workspace/build/`.

### Baremetal application
The bare-metal build environment is platform-specific.\
For details on generating binaries for bare-metal environments, refer to [README.md](../../pal/baremetal/README.md).


## BSA run steps

### For UEFI application

#### Silicon System
On a system with a functional USB port:
1. Copy `Bsa.efi` to a FAT-formatted USB device.
  - **If the platform boots through U-Boot then perform below steps:**
    1. Copy `Shell.efi` (from [prebuilt_images](../../prebuilt_images/BSA)) to the same USB device.
    2. Boot to the U-Boot shell and run:\
       `usb start`
    3. Load `Shell.efi` and launch the UEFI shell:\
       `fatload usb <dev_num> ${kernel_addr_r} Shell.efi`\
       `bootefi ${kernel_addr_r}`
2. In the UEFI shell, refresh mappings with:\
   `map -r`
3. Switch to the USB filesystem (for example, `fs0:`).
4. Run `Bsa.efi` with the required parameters (see [Common CLI arguments](../common/cli_args.md)).
5. Capture the UART console output to a log file for analysis.

**Example**

`Shell> Bsa.efi -v 1 -skip B_PE_01,B_GIC_02 -f bsa_uefi.log`

Runs at INFO level, skips rules `B_PE_01`/`B_GIC_02`, and saves the UART log to
`bsa_uefi.log`.

> BSA rule filters follow the `B_<section>_<nn>` pattern defined in the
  [BSA checklist](arm_bsa_testcase_checklist.md)
  (for example, `B_PE_01`, `B_GIC_02`).\
> Module selectors mirror the BSA spec sections such as `PE`, `GIC`, `PCIE`, and
  `MEM_MAP`.


#### Emulation environment with secondary storage
1. Create a FAT image containing `Bsa.efi` (and `Shell.efi` if required for U-Boot platforms):\
    `mkfs.vfat -C -n HD0 hda.img 2097152`\
    `sudo mount -o rw,loop=/dev/loop0,uid=$(whoami),gid=$(whoami) hda.img /mnt/bsa`\
    `sudo cp "<path to application>/Bsa.efi" /mnt/bsa/`\
    `sudo umount /mnt/bsa`\
    *(Pick a free loop device if `/dev/loop0` is busy.)*
2. Attach the image to the virtual platform or emulator using its documented backdoor method.
3. Boot to the UEFI shell.
4. Refresh filesystem mappings with:\
   `map -r`
5. Switch to the assigned filesystem (`fs<x>`), run `Bsa.efi` with the desired arguments (see [Common CLI arguments](../common/cli_args.md)).
6. Capture UART console output for certification and debug reports.

### For Linux application

1. Copy `bsa_acs.ko` and `bsa_app` to removable media or the DUT.
2. Boot into Linux and mount the media if needed.
3. Load the kernel module:\
    `sudo insmod bsa_acs.ko`
4. Run the user-space application (see [Common CLI arguments](../common/cli_args.md)).\
    `./bsa_app`
5. After the run, unload the module:\
    `sudo rmmod bsa_acs`

### Application Arguments
Refer to [Common CLI arguments](../common/cli_args.md) for flag syntax,
module filters, and logging options, and consult the [User Guide](arm_bsa_architecture_compliance_user_guide.pdf)
for additional usage notes.

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

## Feedback, contributions and support

- Email: [support-systemready-acs@arm.com](mailto:support-systemready-acs@arm.com)
- GitHub Issues: [sysarch-acs issue tracker](https://github.com/ARM-software/sysarch-acs/issues)
- Contributions: [GitHub Pull Requests](https://github.com/ARM-software/sysarch-acs/pulls)

## License
BSA ACS is distributed under the [Apache v2.0 License](https://www.apache.org/licenses/LICENSE-2.0).

--------------

*Copyright (c) 2021-2026, Arm Limited and Contributors. All rights reserved.*
