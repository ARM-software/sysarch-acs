# FIRME Architecture Compliance Suite (FIRME-ACS)

## About FIRME

Firmware Interfaces for RME (FIRME) defines the EL3 firmware interfaces used
to manage resources supporting the Realm Management Extension. Its services
include Granule Protection Table (GPT) management, Memory Encryption Context
(MEC) refresh, IDE key management, attestation and integrated-device management.

FIRME-ACS validates an EL3 implementation against the
[DEN0149 FIRME 1.0 ALP4 specification](https://support.arm.com/documentation/den0149/1-0alp4/?lang=en).
The DUT can be supplied by any firmware implementer; the provided FVP
integration uses Trusted Firmware-A (TF-A).

## Release scope and specification mapping

Alpha1 targets Concepts/Base, GPT, MECID and IDE Key Management, including
their ABI-interface rules. The ALP4 inventory contains 123 rules; R0023 and
R0041 are excluded from DUT validation with Architect agreement because their
type/size declarations have no independent runtime observation. Their local
catalog checks do not count as DUT compliance tests.

The default registry schedules the remaining 121 rule checks, including 24
generated ABI tests. Registration does not imply complete coverage: some
checks still need independent observations or qualified fixtures.

Tests follow the specification, not the behavior of a particular TF-A or model
revision. Missing mandatory support and incorrect results fail. Optional
absence or unavailable independent evidence is reported explicitly; SKIP and
unexecuted scenarios never count as PASS. See the
[scenario document](docs/FIRME_Architecture_Compliance_Suite_Scenario_Document.rst)
and [ABI interface coverage](docs/FIRME_Alpha1_ABI_Interface_Coverage.md).

| ACS revision | Git tag | Specification baseline |
|---|---|---|
| FIRME Alpha1 ACS v0.5 Alpha | v26.03_FIRME_0.5 | [DEN0149 FIRME 1.0 ALP1](https://developer.arm.com/documentation/den0149/1-0alp1) |
| FIRME Alpha1 ACS v1.0.1 Alpha | `v26.10_FIRME_1.0.1` | [DEN0149 FIRME 1.0 ALP4](https://support.arm.com/documentation/den0149/1-0alp4/?lang=en) |

The runtime banner reports the ACS version, release target and Alpha quality.
The target tag alone does not establish publication or completed qualification.

## Release quality and known results

**Release quality: Alpha (qualification incomplete).** This is development
software, not a fully qualified compliance release. A completed regression
is not a passing compliance verdict.

### Recorded UART snapshot

The following runs completed normally on **16 October 2026**. GCC and Clang
agree per rule. These measurements apply to the specified configurations,
not to every firmware revision or platform.

| Firmware/model configuration | Suite | PASS | FAIL | SKIP |
|---|---|---:|---:|---:|
| Stock TF-A `510355478850` and reference FVP | `all` | 31 | 43 | 47 |
| Experimental paired CXL firmware/model | `ide` | 12 | 1 | 8 |

The experimental IDE run does not qualify unmodified TF-A/FVP or the other
suites. Use the actual run's diagnostics and retain its firmware/model
revisions and configuration; these totals are not golden-log expectations.

### Failure and skip interpretation

- **DUT failures:** missing mandatory Granule Management ABIs, IDE feature
  register 1 returning `NOT_SUPPORTED`, and incorrect GPT status/cleanup
  responses remain failures; tests must not accept incorrect results.
- **Firmware/model limitations:** valid CXL requests are rejected by the
  recorded stock path, and its MEC refresh callback returns success without
  refreshing the context. Functional backends are needed to qualify effects.
- **ACS gaps:** deterministic GPT/L1 fixtures, Selective IDE observations and
  some shared-buffer evidence remain incomplete. Two-ECAM namespace work is
  deferred, not waived. R0070 needs independent EL3 attribute evidence;
  R0071 needs buffer PAS/GPI evidence. Mapping lifetime is informational I0073.
- **Dependent skips:** unresolved operations or unproved restoration prevent
  resource reuse. Later dependent tests skip; the original failure remains.
- **Conditional coverage:** absent optional support or unavailable evidence
  must be reported explicitly. Missing ACS setup is not a DUT limitation.

Reassess these limitations when firmware, model or ACS support changes.

## Execution and source layout

The NS-EL2 host dispatches one-rule tests to Normal, Secure and Realm caller
payloads, which invoke the EL3 DUT and return observations. Required caller
coverage, asynchronous completion and cleanup contribute to the final result.

- `test/<module>/`: rule tests and generated ABI entries.
- `val/` and `val_common/`: dispatch, ABI adapters, observations and runtime.
- `platform/pal_baremetal/targets/<target>/`: platform access and ownership data.
- `tools/`: scripts, CMake toolchains, linkers, configuration and manifests.
- `docs/`: methodology, scenarios, porting and setup instructions.

Linker layouts and MMU initialization belong to the common runtime integration.
Reserve non-overlapping payload and test memory. Preserve `-mstrict-align`,
`-mgeneral-regs-only` and `-mno-outline-atomics` for safe bare-metal execution
with GCC and Clang; vector-state probes use explicit assembly.

## Build

Run from the repository root with CMake, a build tool and an AArch64 compiler.
Set the compiler prefix explicitly; it can include its absolute path:

```sh
export CROSS_COMPILE=/path/to/toolchain/bin/aarch64-none-elf-
cmake -S cca/firme -B cca/firme/build-cmake \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/cca/firme/tools/cmake/toolchains/aarch64-none-elf-gcc.cmake"
cmake --build cca/firme/build-cmake
```

The default `SUITE=all` builds every implemented module. For a module-specific
build, add `-DSUITE=base`, `gpt`, `mec` or `ide` at configuration time;
use a separate build directory for each configuration. The normal build
produces `cca/firme/build/nsel2.bin`, `secure.bin` and `rel2.bin`.
See the [porting guide](docs/porting_guide.md) for Clang and platform settings.

## Build firmware and run

Set the reference firmware checkout and licensed model executable:

```sh
export TFA_DIR=/path/to/trusted-firmware-a
export MODEL_BASE=/path/to/FVP_Base_RevC-2xAEMvA
bash cca/firme/tools/scripts/build_tfa_fvp.sh
BUILD_MODE=never FVP_TIMEOUT_SECS=240 \
  bash cca/firme/tools/scripts/run_fvp_regression.sh
```

Separate build/run commands allow CI to stop on a build failure. For the
combined out-of-box path, run `run_fvp_regression.sh` without
`BUILD_MODE=never`. Set the same `SUITE` and build directories for both
stages when selecting a single module. A timeout without a complete test
summary is a failed run, not a PASS.

`BUILD_MODE=auto` reuses existing firmware and builds only when images are
missing. Use `BUILD_MODE=source` to rebuild after source changes, or `always`
for an explicit build. `never` runs packaged images without a TF-A checkout.
Build manifests check image hashes and configuration; a mismatched profile
requires a matching build, not a silent rebuild during artifact execution.

The runner's exit status reports execution completion, not test PASS/FAIL.
A completed run returns zero even when individual tests fail; CI evaluates
the UART results against its golden log. Build/configuration errors, abnormal
model exits and incomplete runs return nonzero. Normal completion uses PSCI
`SYSTEM_OFF` with `bp.ve_sysregs.exit_on_shutdown=1`; the timeout remains a
watchdog. A complete UART summary does not override a model crash.

The runner enables the parameters required by the selected suite. For
`SUITE=all|gpt`, the defaults include the GDI GPT profile, 8 GiB DRAM and
modelled caches. Model settings cannot supply missing firmware ABIs, and
extra DRAM is not permission to use memory owned by firmware.

For firmware configured with Delegated attestation and the legacy private
P-384 RAK setup interface, set `FIRME_REALM_PAT_BINDING=delegated-rmm-el3`
for both build and run, and initialize submodules first. The default is
`none`; select the binding only when it matches the firmware configuration.
The runner enables RNDR for the binding's cryptographic operations.

The default run does not provide a paired CXL endpoint/Root Port fixture.
See [IDE validation setup](docs/IDE_Validation_Setup.md) for its optional
dependencies, topology and matching firmware/model requirements. Initialize
the pinned dependencies before configuring SPDM:

```sh
git submodule update --init --recursive
```

The optional `FVP_FIRME_MEC_REFRESH=1` backend also requires matching local
firmware/model support and modelled caches. It validates refresh sequencing
and memory observations, not cryptographic strength. Neither experimental
backend changes architectural expectations.

## Validation and release accounting

Tests use MP workers through the normal dispatcher when required; no separate
MP regression binary is needed. R0239 combines the widths read from all
platform-listed PEs and SMMUs. See [MP execution](docs/MP_Execution.md).

```sh
python3 cca/firme/tools/scripts/audit_alpha1_coverage.py --inventory-only
python3 cca/firme/tools/scripts/audit_alpha1_coverage.py \
  --uart-log cca/firme/build-fvp-uart.log
```

Inventory validation checks source/registry consistency only. The release
audit also reports missing, skipped, gated and failing rules. Native ACS
checks validate harness logic, not DUT compliance:

```sh
bash cca/firme/tools/scripts/check_gpt_native.sh
python3 cca/firme/tools/scripts/check_mec_regressions.py
bash cca/firme/tools/scripts/check_ide_native.sh
bash cca/firme/tools/scripts/check_dispatch_native.sh
python3 cca/firme/tools/scripts/check_alpha1_coverage.py
```

Use `CC=clang` for shell-driven checks and `check_realm_pat_binding.py`;
the other Python-driven C checks use `HOST_CC=clang`. IDE native checks
require pinned dependency headers; `SANITIZE=1` enables their ASan/UBSan checks.

## Documentation

- [Validation methodology](docs/arm_firme_architecture_compliance_suite_validation_methodology.rst)
- [Test scenarios and exclusions](docs/FIRME_Architecture_Compliance_Suite_Scenario_Document.rst)
- [Porting guide](docs/porting_guide.md)
- [IDE setup and qualification limits](docs/IDE_Validation_Setup.md)

`tools/config/sp_layout.json` is intentionally empty: the default Secure
payload does not package Secure Partitions.

## Feedback, contributions, and support

- Email: [support-firme-acs@arm.com](mailto:support-firme-acs@arm.com)
- Issues: [sysarch-acs issue tracker](https://github.com/ARM-software/sysarch-acs/issues)
- Contributions: [GitHub Pull Requests](https://github.com/ARM-software/sysarch-acs/pulls)

## License

FIRME-ACS is licensed under the **BSD 3-Clause License**.
See [LICENSE.md](LICENSE.md) and individual source headers.

*Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.*
