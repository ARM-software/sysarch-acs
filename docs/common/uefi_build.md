# Common UEFI Build Guide

This guide captures the shared steps required to build any of the ACS UEFI applications (BSA, SBSA, PC-BSA, VBSA, and derivatives) from source. Each individual README can reference this document and only highlight the variant-specific details.

## Prerequisites
- Linux host (x86_64 or AArch64) with a POSIX-compatible shell
- `git`, `python`, and the packages required by edk2
- Arm GNU toolchain 14.3 or newer (or native AArch64 compiler on AArch64 hosts)
- Network access to clone the upstream repositories

## Workspace setup
```bash
mkdir -p workspace && cd workspace
git clone -b edk2-stable202511 https://github.com/tianocore/edk2.git
cd edk2
git submodule update --init --recursive
git clone https://github.com/tianocore/edk2-libc.git
git clone https://github.com/ARM-software/sysarch-acs.git ShellPkg/Application/sysarch-acs
cd -
```

## Toolchain selection
- **x86 host:** download the Arm GNU toolchain tarball, extract it, and export `GCC_AARCH64_PREFIX=<toolchain_dir>/bin/aarch64-none-linux-gnu-`
- **AArch64 host:** export `GCC_AARCH64_PREFIX=/usr/bin/aarch64-linux-gnu-` (or the native compiler prefix available on the system)

## Build the edk2 base components
```bash
cd workspace/edk2
export PACKAGES_PATH=$PWD/edk2-libc
source edksetup.sh
make -C BaseTools/Source/C
```

## Build an ACS UEFI binary
Clean `Build/` when switching flavors or toolchains, then run:

```bash
rm -rf Build/
source ShellPkg/Application/sysarch-acs/tools/scripts/acsbuild.sh <target>
```

| Variant | `acsbuild` target | Output binary | Notes |
|---------|-------------------|---------------|-------|
| BSA (ACPI) | `bsa` | `Build/Shell/DEBUG_GCC/AARCH64/Bsa.efi` | Default path assuming DEBUG+GCC |
| BSA (Device Tree) | `bsa_dt` | `Build/Shell/DEBUG_GCC/AARCH64/Bsa.efi` | Use for DT-based platforms |
| SBSA | `sbsa` | `Build/Shell/DEBUG_GCC/AARCH64/Sbsa.efi` | |
| PC-BSA | `pc_bsa` | `Build/Shell/DEBUG_GCC/AARCH64/PC_Bsa.efi` | |
| VBSA | `vbsa` | `Build/Shell/DEBUG_GCC/AARCH64/Vbsa.efi` | |

### Tips
- Pass extra edk2 build options via `BUILD_OPTIONS` before sourcing `acsbuild.sh`
- Cache the edk2 workspace outside the repo when multiple variants share the same binaries
- CMake presets apply only to bare-metal builds; edk2 builds always use `acsbuild.sh`

---
*Copyright (c) 2026, Arm Limited and Contributors. All rights reserved.*
