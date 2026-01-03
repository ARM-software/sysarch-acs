# Common Linux Application Build Guide

The ACS Linux application and kernel module share the same build script across BSA, SBSA, PC-BSA, and VBSA. This document centralizes the instructions and variant-specific outputs.

## Prerequisites
- Linux host (x86_64 or AArch64)
- Bash shell
- Internet access to download the helper script and Linux sources
- Appropriate cross-compilation toolchain (GNU Arm 13.2.rel1 by default)

## Build steps
```bash
mkdir -p workspace && cd workspace
wget https://gitlab.arm.com/linux-arm/linux-acs/-/raw/master/acs-drv/files/build.sh
chmod +x build.sh
source build.sh [args]
```

### Script arguments
- `-v`, `--version` — Linux kernel version to build (default: 6.10)
- `--GCC_TOOLS` — GCC toolchain release (default: 13.2.rel1)
- `--help` — display full usage and environment info
- `--clean` — remove the `build/` output folder (modules and apps)
- `--clean_all` — remove all downloaded repos and artifacts

## Build outputs
The script creates `workspace/build/` and emits the following artifacts:

| Variant | Kernel module | User-space app | Notes |
|---------|---------------|----------------|-------|
| BSA | `bsa_acs.ko` | `bsa_app` | Load the module before running the app |
| SBSA | `sbsa_acs.ko` | `sbsa_app` | Shares the same sources as BSA |
| PC-BSA | `pcbsa_acs.ko` | `pcbsa_app` | Built alongside BSA/SBSA artifacts |
| VBSA | `bsa_acs.ko` | `bsa_app` | Run with the VBSA skip list |

## Basic runtime flow
```bash
sudo insmod <variant>_acs.ko
./<variant>_app [arguments]
sudo rmmod <variant>_acs
```

Consult each specification README for variant-specific arguments or skip lists when invoking the Linux application.

---
*Copyright (c) 2026, Arm Limited and Contributors. All rights reserved.*