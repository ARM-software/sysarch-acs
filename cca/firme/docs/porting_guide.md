# FIRME-ACS Porting Guide

This guide describes how to port the FIRME bare-metal ACS hosted under `cca/firme/` to a new platform, while
keeping the same three-world execution model (NS-EL2 host + S-EL2 shim + R-EL2 shim).

## 1. Platform configuration (PAL)

PAL sources are organized by **environment** and **target**:

- Environment: `baremetal`
- Target (default): `fvp`

To port to a new platform, create a new target folder under:

- `cca/firme/platform/pal_baremetal/targets/<your_platform>/{include,src}/`

and update the platform configuration header:

- `cca/firme/platform/pal_baremetal/targets/<your_platform>/include/pal_platform.h`

Key PAL knobs:

- UART base for logs: `PLAT_NS_UART_BASE`
- GIC bases: `PLAT_GICD_BASE`, `PLAT_GICR_BASE`
- Direct messaging endpoint IDs (if Secure direct messaging is used): `PLAT_FFA_*_ENDPOINT_ID`
- GPT test window + granule size:
  - `PLAT_FIRME_GPT_BASE_{NS,SECURE,REALM}`
  - `PLAT_FIRME_GPT_GRANULE_SIZE`

The GPT test window must be a PA range that is mapped and accessible in the caller world you are testing from,
and must not overlap your payload images or TF-A reserved regions.

## 2. Build + FVP regression

FIRME-ACS is cross-compiled. Set `CROSS_COMPILE` to your AArch64 toolchain prefix (it may include a full path),
for example `/opt/tc/bin/aarch64-none-elf-`.

End-to-end run (builds payloads + TF-A as needed, then runs FVP and captures logs):

```sh
export CROSS_COMPILE=/path/to/toolchain/bin/aarch64-none-elf-
export MODEL_BASE=/path/to/FVP_Base_RevC-2xAEMv8A
export TFA_DIR=/path/to/trusted-firmware-a
cca/firme/tools/scripts/run_fvp_regression.sh
```

Optional knobs:

```sh
export CROSS_COMPILE=/path/to/toolchain/bin/aarch64-none-elf-
export FIRME_TOOLCHAIN=clang     # gcc (default) | clang
export SUITE=gpt                 # all (default) | base | gpt | mec | ide | attest
export BUILD_MODE=auto           # auto (default) | always | never
export BUILD_TFA=auto            # auto (default) | always | never
export TFA_LOG_LEVEL=40          # 10..50 (ERROR..VERBOSE)
export FVP_ENABLE_MEC=1          # enable FVP MEC/MPE model parameters for MEC tests
export FVP_MECID_WIDTH=16        # default MECID width for FVP MEC runs
export FVP_ENABLE_SMMU_MEC=1     # optional SMMU MECID plumbing for future device-side MEC scenarios
```

## 3. Adding new tests

- Maintain a **1:1 mapping**: one DEN0149 rule per file under `cca/firme/test/<module>/`.
- Keep rule logic in `test/`, and expose reusable helpers through `val/` (not `test/include/`).
- Use the NS dispatcher as the single point that aggregates results and prints a final PASS/FAIL per rule.
