# ACS EL3 SMC service

The ACS EL3 SMC service gives sysarch-acs tests controlled access to information that is not available from the non-secure ACS execution environment. It is intended only for ACS validation firmware.

Trusted Firmware-A (TF-A) reserves a vendor-specific SMC FIDs for ACS. This integration uses that reserved interface to include the ACS EL3 handler in BL31 so that ACS can perform validation that requires secure-world support. The build script integrates the ACS SMC handler with TF-A and builds either a standalone BL31 binary or the complete TF-A build.

We currently provide RDV3 as the reference implementation. To support a new platform or a non-TF-A EL3 implementation, see the
[porting guide](../../docs/common/acs_smc_porting_guide.md).

## Supported rules

| Rule | Requirement | EL3 service dependency |
| --- | --- | --- |
| B_PPI_03 | Secure EL2 physical timer (CNTHPS) PPI mapping | `ACS_SMC_GET_CNTHPS_INTID` |
| B_PPI_03 | Secure EL2 virtual timer (CNTHVS) PPI mapping | `ACS_SMC_GET_CNTHVS_INTID` |
| B_TIME_02 | System counter frequency is at least 10 MHz | `ACS_SMC_READ_BASE_CNTFREQ` |
| S_L8TI_01 | System counter frequency is at least 50 MHz | `ACS_SMC_READ_BASE_CNTFREQ` |

If EL3 does not implement the ACS FID, rules that depend on the SMC service are
skipped when the call returns the SMCCC unknown-function value (`SMC_UNK` in
TF-A).

## SMC interface

ACS uses the vendor-specific SMC64 FID `0xC7000030`.

## TF-A compatibility

TF-A must reserve the ACS FID and dispatch it through `plat_arm_acs_smc_handler()`. This support was introduced by TF-A commit
[`f69f551`](https://github.com/ARM-software/arm-trusted-firmware/commit/f69f551269f1d126877889a1cab27cc1692316ea)
and is present from TF-A v2.14. The TF-A build must set `ENABLE_ACS_SMC=1` to enable the dispatch path; the supplied
[build script](../../tools/scripts/build_acs_smc.sh) sets this flag automatically.

## Prerequisites

- AArch64 GCC compiler (or cross compiler) and binutils
- GNU Make
- Python 3
- Device Tree Compiler (`dtc`)
- OpenSSL
- A TF-A v2.14-or-newer source tree, or a TF-A fork containing the ACS FID reservation and dispatch support
- The Mbed TLS version required by the selected TF-A/platform configuration
- Any images and build flags normally required by the target TF-A platform

The script checks these inputs before starting the build.

## Standalone BL31 build

Run the script from the sysarch-acs root:

```sh
./tools/scripts/build_acs_smc.sh \
    TFA_PATH=/path/to/arm-trusted-firmware \
    PLAT=$PLAT \
    CC=aarch64-linux-gnu-gcc \
    DEBUG=1
```

The default target is `bl31`. Pass `--all` to request the TF-A `all` target.
Other `NAME=value` arguments are passed to TF-A. The generated image is under the platform's TF-A build directory, for example:

```text
/path/to/arm-trusted-firmware/build/$PLAT/debug/bl31.bin
```

`TFA_PATH` and `PLAT` are required. `ACS_PATH`, `CC`, and `MBEDTLS_DIR` can be set explicitly when automatic detection is unsuitable. Add any other flags
required by the target platform and TF-A revision.

## RDV3 software-stack build and packaging

The following command shape has been used with a RDV3 software stack (2025/26 version). Replace every path with the corresponding path in the local stack:

```sh
./tools/scripts/build_acs_smc.sh \
    TFA_PATH=/path/to/rdv3-stack/tf-a \
    PLAT=rdv3 \
    CC=aarch64-linux-gnu-gcc \
    STACK_PATH=/path/to/rdv3-stack \
    NRD_PLATFORM_VARIANT=0 \
    DEBUG=1 \
    ENABLE_RME=1 \
    RME_GPT_BITLOCK_BLOCK=0 \
    RMM=/path/to/rdv3-stack/rmm/build/Debug/rmm.img \
    SPD=spmd \
    SPMD_SPM_AT_SEL2=1 \
    BL32=1 \
    SP_LAYOUT_FILE=/path/to/rdv3-stack/build-scripts/sp_metadata/rdv3/rdv3_sp_layout.json \
    ENABLE_PMF=1 \
    SEPARATE_CODE_AND_RODATA=1
```

When `STACK_PATH` is present, the script:

1. Copies `bl31.bin` to `STACK_PATH/output/PLAT/tf-bl31.bin`.
2. Invokes `STACK_PATH/build-scripts/rdinfra/build-test-acs.sh -p PLAT package`.

## Implementation layout

```text
services/acs_smc/
|-- acs_smc.mk                    TF-A build integration
|-- include/acs_el3_handler.h     FID, selectors, status, and prototypes
|-- platform/rdv3/platform_el3.h  Platform specific details (for RDV3)
`-- src/
    |-- acs_el3_handler.c         EL3 service implementation
    `-- AArch64/PlatAcsSysreg.S   Secure EL2 timer register accessors
```

The non-secure System ACS wrappers and tests are in `val/` and `test_pool/`.

## License

The ACS EL3 SMC service is distributed under the Apache License 2.0.

--------------

*Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.*
