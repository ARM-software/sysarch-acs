# FIRME Architecture Compliance Suite (FIRME-ACS)

This directory hosts the **FIRME Architecture Compliance Suite** to validate the
TF-A **BL31 (EL3) FIRME service** implementation against DEN0149 FIRME rules.

## About FIRME
FIRME (Firmware Interfaces for RME) defines the EL3 firmware interfaces required
to support Realm-related operations in systems implementing the Realm Management
Extension (RME). FIRME provides standardized ABIs that allow EL3 firmware to
manage GPT state transitions, refresh memory encryption contexts, configure
platform security services, and support attestation flows.

Specification: **[DEN0149 FIRME](https://developer.arm.com/documentation/den0149/latest)**

FIRME-ACS is hosted under **sysarch-acs** to support a unified “System ACS”
umbrella: this is the first suite onboarding from the *Vault* system ACS
projects (CCA-related), and additional Vault suites are expected to follow the
same hosting model.

## ACS release details
- **Suite version (runtime banner):** FIRME Alpha1 ACS v0.5 Alpha
- **DUT:** TF-A BL31 (EL3) FIRME service implementation
- **Current coverage:** Base service / Concepts (R0007, R0008, R0009,
  R0010, R0012, R0013, R0025, R0026, R0033, R0034, R0035, R0038, R0044,
  R0047, R0051, R0053, R0055), MECID management (R0206, R0208), and GPT
  Management (R0085, R0086, R0089)
- **Deferred or not runtime-scheduled:** R0011 until every architected status is
  reachable; R0014 and R0015 until all service/status output shapes and the
  callee non-disclosure clause are auditable; R0023 because legal calls cannot
  observe decoder width; R0024 until service-specific checks identify every
  allocated non-Base service ID; R0031 because it is a specification
  release-policy requirement rather than a runtime DUT check; R0041 because
  register size is not exposed by the ABI; R0048 and R0054 until TF-A advertises
  all mandatory ALP3 ABI bits.
- **Execution environments:** Arm FVP (pre-silicon) and silicon (platform dependent)

## FIRME spec version mapping

| FIRME-ACS version | Git tag | FIRME spec version |
|---|---|---|
| FIRME Alpha1 ACS v0.5 Alpha | v26.03_FIRME_0.5 | DEN0149 FIRME 1.0 ALP3 Concepts/Base service, MECID management, and GPT management coverage |

## How it works (high level)
1. TF-A boots three ACS payload images: **Secure shim** (BL32), **Realm shim**
   (R-EL2 payload image), and **Normal payload** (BL33 host at NS-EL2).
2. The **NS-EL2 host** anchors regression: it probes FIRME, iterates rule tests,
   and prints a single PASS/FAIL per rule.
3. When a rule requires non-NS callers, the host requests **Secure** / **Realm**
   work via EL3 routing (SPMD direct messaging / RMMD forwarding).
4. Each world issues FIRME SMCs from the expected caller security state and
   returns status and fault info back to the host for validation.

## Repository layout (ACS-style)
- `docs/`: porting guide + scenario document.
- `platform/`: PAL implementations (environment + target), for example:
  `platform/pal_baremetal/targets/fvp/` (UART base, GPT window, GIC bases).
- `val_common/`: shared bare-metal infrastructure (vectors, fault handling, logging, SMCCC, MMU helpers).
- `val/`: world payloads and common wrappers (`val/host`, `val/secure`, `val/realm`).
- `test/`: 1:1 rule-to-test mapping (one rule per file under `test/<module>/`).
- `tools/`: build helpers, linker scripts, and regression scripts under `tools/scripts/`.

## Memory layout and MMU notes
FIRME-ACS is designed so that partners typically only need to port **PAL**. The items below are part of the
harness design and are primarily relevant when integrating with a platform that uses different load addresses
than the FVP defaults.

- **Linker scripts:** `cca/firme/tools/linker/{host,secure,realm}.ld` define the default BL33/BL32/R-EL2 payload
  load addresses and sizes used by the harness on Arm FVP. Only update these when your platform’s TF-A/FIP
  packaging loads the images at different bases.
- **MMU setup:** the harness uses a minimal identity-mapped MMU configuration (1GiB blocks).
  - NS-EL2 host enables the MMU in `cca/firme/val/host/val_host_main.c` (`nsel2_platform_init()`).
  - R-EL2 payload enables the MMU in `cca/firme/val/realm/val_realm_main.c` (`rel2_platform_init()`).
  - Secure shim keeps MMU disabled by default (see `cca/firme/val/secure/val_secure_spmc.c`), unless a safe
    mapping is required for your BL32 load region.

## Build FIRME-ACS payloads
Requires an AArch64 cross toolchain. Set `CROSS_COMPILE` to the toolchain prefix (it may include a full path),
for example `/opt/tc/bin/aarch64-none-elf-` or `aarch64-linux-gnu-`.

### CMake (recommended)

```sh
export CROSS_COMPILE=/path/to/toolchain/bin/aarch64-none-elf-
cmake -S cca/firme -B cca/firme/build-cmake \
  -DCMAKE_TOOLCHAIN_FILE="$(pwd)/cca/firme/tools/cmake/toolchains/aarch64-none-elf-gcc.cmake"
cmake --build cca/firme/build-cmake
```

### Make (wrapper around CMake)

```sh
make -C cca/firme CROSS_COMPILE=/path/to/toolchain/bin/aarch64-none-elf-
```

Output binaries:
- `cca/firme/build/nsel2.bin` (Normal world host, BL33)
- `cca/firme/build/secure.bin` (Secure shim, BL32)
- `cca/firme/build/rel2.bin` (Realm shim, R-EL2 payload image)

## Build TF-A + run on Arm FVP
FIRME-ACS uses TF-A as the world-switching monitor and runs against TF-A’s EL3
FIRME implementation (the DUT).

1. Set your FVP model path:
   - `export MODEL_BASE=/path/to/FVP_Base_RevC-2xAEMvA`
2. Run the regression script:
   - `cca/firme/tools/scripts/run_fvp_regression.sh`

Useful knobs:
- `TFA_DIR=/path/to/trusted-firmware-a`
- `BUILD_MODE=auto|always|never` (controls rebuilds of TF-A + FIRME-ACS)
- `BUILD_TFA=auto|always|never` (controls TF-A rebuilds only)
- `FVP_TIMEOUT_SECS=240` (auto-terminate the model)
- `FVP_ENABLE_MEC=1` (enable FVP MEC/MPE parameters for MECID management tests)
- `FVP_MECID_WIDTH=16` (default MECID width when MEC is enabled)
- `FVP_ENABLE_SMMU_MEC=1` (optional device-side SMMU MECID plumbing; not required for current MEC ABI tests)
- `UART_LOG=cca/firme/build-fvp-uart.log`, `MODEL_LOG=cca/firme/build-fvp-model.log`

### TF-A Secure Partition layout file (`tools/config/sp_layout.json`)
FIRME-ACS does not package Secure Partitions by default (it uses a Secure shim at S-EL2 and an internal
S-EL2→S-EL1 entry path for SEL1 caller coverage). For this reason, `cca/firme/tools/config/sp_layout.json` is
an empty JSON object (`{}`).

When the file is empty, `cca/firme/tools/scripts/build_tfa_fvp.sh` can build TF-A without requiring the Python
`fdt` module (it will provide an empty `sp_gen.mk` fallback when needed). If you populate `sp_layout.json` to
package SPs, ensure your Python environment provides `fdt` so TF-A can generate `sp_gen.mk`.

## Documentation
- Porting: `cca/firme/docs/porting_guide.md`
- Scenarios / rule coverage: `cca/firme/docs/FIRME_Architecture_Compliance_Suite_Scenario_Document.rst`

## Feedback, contributions, and support
- Email: [support-firme-acs@arm.com](mailto:support-firme-acs@arm.com)
- GitHub Issues: [sysarch-acs issue tracker](https://github.com/ARM-software/sysarch-acs/issues)
- Contributions: [GitHub Pull Requests](https://github.com/ARM-software/sysarch-acs/pulls)

## License
FIRME-ACS (`cca/firme/`) is licensed under the **BSD 3-Clause License**. See
`cca/firme/LICENSE.md` and individual source headers for details.

--------------

*Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.*