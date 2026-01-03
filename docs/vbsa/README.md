# Virtual Base System Architecture - Architecture Compliance Suite

## Table of Contents

- [Virtual Base System Architecture](#virtual-base-system-architecture)
- [VBSA - Architecture Compliance Suite](#vbsa---architecture-compliance-suite)
- [Release details](#release-details)
- [Documentation & Guides](#documentation--guides)
- [VBSA build steps](#vbsa-build-steps)
	- [UEFI Shell application](#uefi-shell-application)
	- [Linux application](#linux-application)
- [VBSA run steps](#vbsa-run-steps)
	- [For UEFI application](#for-uefi-application)
	- [For Linux application](#for-linux-application)
- [Application arguments](#application-arguments)
- [VBSA coverage overview](#vbsa-coverage-overview)
- [Limitations](#limitations)
- [Troubleshoot guide](#troubleshoot-guide)
- [Feedback, contributions, and support](#feedback-contributions-and-support)
- [License](#license)

## Virtual Base System Architecture
**Virtual Base System Architecture** (VBSA) defines the requirements and
runtime features needed by a virtual environment to install, boot, and run an
operating system. VBSA supplements the
[Arm BSA specification](https://developer.arm.com/documentation/den0094/latest/)
to target virtual platforms.

See the [VBSA specification](https://developer.arm.com/documentation/den0150/latest/) for the authoritative rules.

## VBSA - Architecture Compliance Suite
VBSA ACS provides self-checking tests for virtual platforms. The majority of the
tests run through the UEFI application, with a subset executed in a Linux guest
environment to exercise OS-visible behavior.

## Release details
- **Code quality:** BETA
- **Latest release version:** v0.7.0
- **Release tag:** `v25.12_VBSA_0.7.0`
- **Specification coverage:** VBSA v1.0
- **Execution levels:** Virtual platforms / pre-silicon validation
- **Scope:** ACS is **not** a substitute for design verification.
- **Test collateral:** [VBSA Test Scenario Document](arm_vbsa_architecture_compliance_test_scenario.md)
- **Prebuilt binaries:** [`prebuilt_images/VBSA/v25.12_VBSA_0.7.0`](../../prebuilt_images/VBSA/v25.12_VBSA_0.7.0)

### GitHub branch
- Use the appropriate **tag** on the **main** branch for a released build.
- Track the **main** branch for the most recent fixes and features.

## Documentation & Guides
- [VBSA specification](https://developer.arm.com/documentation/den0150/latest/)
- [Arm VBSA Test Scenario Document](arm_vbsa_architecture_compliance_test_scenario.md)
- [Arm VBSA Testcase Checklist](arm_vbsa_testcase_checklist.md)
- [Common UEFI build guide](../common/uefi_build.md)
- [Common Linux application guide](../common/linux_build.md)
- [Common CLI arguments](../common/cli_args.md)

## VBSA coverage overview
VBSA ACS combines firmware-level checks with guest-OS validation. Run each path
below (and retain console logs) to claim full coverage for a virtual platform.

### Sections covering VBSA rules
- [UEFI-based tests](#for-uefi-application) — execute `Vbsa.efi` from the UEFI
	shell image (raw disk, VHDX, or integrated firmware) with the required rule
	filters.
- [Linux-based tests](#for-linux-application) — reuse the BSA Linux application
	(`bsa_app`) with the VBSA skip list to exercise guest-visible behavior.
- [Manual verification](arm_vbsa_testcase_checklist.md) — cross-check residual
	rules in the checklist and capture VE-owner evidence where ACS cannot
	auto-verify.

## VBSA build steps

### UEFI Shell application
Set up the workspace and toolchain using the
[Common UEFI build guide](../common/uefi_build.md), then build the VBSA binary:
   1. `source ShellPkg/Application/sysarch-acs/tools/scripts/acsbuild.sh vbsa`
   2. `Vbsa.efi` is written to `Build/Shell/<TOOL_CHAIN_TAG>/AARCH64/` under the edk2 tree.

### Linux application
Use the [Common Linux application guide](../common/linux_build.md) to build the
shared Linux artifacts. VBSA reuses `bsa_acs.ko` and `bsa_app`; run-time skip
lists tailor the coverage to VBSA rules (see the Linux run steps below).

## VBSA run steps

### For UEFI application
1. Place `Vbsa.efi` and `Shell.efi` (renamed to `\EFI\BOOT\bootaa64.efi`) on a
   bootable image.
2. Boot the virtual environment to the UEFI shell.
3. Refresh filesystem mappings with:
   `map -r`
4. Switch to the filesystem containing `Vbsa.efi` (for example, `fs0:`)
5. Run the binary with the desired arguments (see [Common CLI arguments](../common/cli_args.md)).
6. Capture the UART console output for reporting.

**Example**

`Shell> Vbsa.efi -v 1 -skip V_L2PE_01 -el1physkip -f vbsa.log`

Runs at INFO level, skips rule `V_L2PE_01`, enables `-el1physkip` for
hypervisors trapping EL1 timers, and captures logs in `vbsa.log`.

> Use VBSA rule IDs that follow the `V_L<level><module>_<nn>` pattern from the
	[VBSA checklist](arm_vbsa_testcase_checklist.md)
	(for example, `V_L2PE_01`) with `-skip`/`-r`, and enable `-el1physkip` only when
	the hypervisor traps EL1 physical timer accesses; document any coverage gaps.

**Creating a bootable `.img` (Linux host)**
1. `mkfs.vfat -C -n HD0 vbsa.img 1048576`
2. `sudo mount vbsa.img /mnt/acs`
3. `sudo mkdir -p /mnt/acs/EFI/BOOT`
4. `sudo cp Shell.efi /mnt/acs/EFI/BOOT/bootaa64.efi`
5. `sudo cp Vbsa.efi /mnt/acs/`
6. `sudo umount /mnt/acs`

**Creating a bootable `.vhdx` (Windows host)**
1. Create and attach a VHDX using Hyper-V Manager or Disk Management.
2. Inside the mounted VHDX, create `\EFI\BOOT`.
3. Copy `Shell.efi` to `\EFI\BOOT\bootaa64.efi`.
4. Copy `Vbsa.efi` to the root of the volume.
5. Detach the VHDX safely.

### For Linux application
VBSA uses the BSA Linux application with the VBSA skip list:

1. Load the shared kernel module:\
   `sudo insmod bsa_acs.ko`
2. Run the Linux application with the VBSA skip list (see [Common CLI arguments](../common/cli_args.md)).\
   `./bsa_app --skip B_REP_1,B_IEP_1,B_PCIe_11,B_MEM_06`
3. Remove the module after the run:\
   `sudo rmmod bsa_acs`

Inspect logs with `sudo dmesg | tail -500` as needed. Adjust the skip list as
the specification evolves.

### Application arguments
Refer to [Common CLI arguments](../common/cli_args.md) for the complete
flag list, including VBSA-specific guidance on skip lists and logging options.\
That guide also documents VBSA-specific behavior such as the extended module list and the `-el1physkip`
option for hypervisor scenarios.

## Limitations
- VBSA tests span UEFI and Linux components; both must be executed for complete
	coverage.
- Exerciser-dependent PCIe rules cannot be validated in virtual environments;
	affected rules are marked as PASSED*(PARTIAL) or SKIPPED.
- Manual verification is required for rules that cannot be exercised on the
	available virtual platform.

## Troubleshoot guide
- Some hypervisors trap EL1 physical timer access, causing exceptions during
	VBSA ACS runs. Use `-el1physkip` when necessary, but document the resulting
	coverage gap.

## Feedback, contributions and support

- Email: [support-systemready-acs@arm.com](mailto:support-systemready-acs@arm.com)
- GitHub Issues: [sysarch-acs issue tracker](https://github.com/ARM-software/sysarch-acs/issues)
- Contributions: [GitHub Pull Requests](https://github.com/ARM-software/sysarch-acs/pulls)

## License
VBSA ACS is distributed under the [Apache v2.0 License](https://www.apache.org/licenses/LICENSE-2.0).

--------------

*Copyright (c) 2025-2026, Arm Limited and Contributors. All rights reserved.*
