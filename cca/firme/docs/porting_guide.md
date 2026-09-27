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
- PCIe ECAM and IDE keyset fields for IDE key management tests:
  - `PLAT_FIRME_PCIE_ECAM_BASE`, `PLAT_FIRME_PCIE_ECAM_SIZE`
  - `PLAT_FIRME_PCIE_START_BUS`, `PLAT_FIRME_PCIE_END_BUS`
  - `PLAT_FIRME_PCIE_SEGMENT`, `PLAT_FIRME_PCIE_ROOT_PORT_ID`
  - `PLAT_FIRME_PCIE_ENDPOINT_ID` if the endpoint requester ID is fixed
  - `PLAT_FIRME_PCIE_STREAM_ID`, `PLAT_FIRME_PCIE_SUBSTREAM_ID`

The GPT test window must be a PA range that is mapped and accessible in the caller world you are testing from,
and must not overlap your payload images or TF-A reserved regions. The ECAM base must identify the target
Root port namespace used by the platform FIRME IDE key management implementation. `PLAT_FIRME_PCIE_ROOT_PORT_ID` and `PLAT_FIRME_PCIE_ENDPOINT_ID` use PCI requester-ID encoding: `bus[15:8]`, `device[7:3]`, and `function[2:0]`.

Reserve the PAL-configured Selective IDE stream exclusively for ACS before
running the suite. PROG, GO, STOP and POLL tests reuse that stream sequentially;
they do not assume additional stream IDs exist. Tests drain their pending
responses and stop the stream before reuse. A cleanup error remains a failure.

## 2. Build + FVP regression

FIRME-ACS is cross-compiled. Set `CROSS_COMPILE` to your AArch64 toolchain prefix (it may include a full path),
for example `/opt/tc/bin/aarch64-none-elf-`.

Optional `FIRME_ENABLE_CXL_SPDM=1` builds require the bare-metal GNU toolchain's
newlib `libc.a`, including when Clang compiles the payload. CMake discovers it
from the GCC compiler, or from `CROSS_COMPILE` for Clang, and links only required
archive members into the host payload. This supplies mbedTLS's standard string
runtime without ACS-owned `strlen`, hosted Linux libc, or OS startup code.
Default builds with SPDM disabled do not require newlib.

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
export FVP_ENABLE_IDE=1          # enable TF-A/FVP IDE key management support for IDE tests
export FVP_IDE_KM_PENDING_OPS_MAX=1  # enable non-blocking IDE operation tracking
export FVP_SIMULATE_IDE_KM_UNIT=1    # enable the FVP IDE KM simulation path in TF-A
```

## 3. Adding new tests

- Maintain a **1:1 mapping**: one DEN0149 rule per file under `cca/firme/test/<module>/`.
- Keep rule logic in `test/`, and expose reusable helpers through `val/` (not `test/include/`).
- Use the NS dispatcher as the single point that aggregates results and prints a final PASS/FAIL per rule.
