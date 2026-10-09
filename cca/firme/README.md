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
- **Suite version (runtime banner):** FIRME Alpha1 ACS v1.0.1 Alpha
- **Release target:** `v26.10_FIRME_1.0.1`
- **DUT:** TF-A BL31 (EL3) FIRME service implementation
- **ALP4 inventory:** 123 rules across Concepts/Base, GPT, MECID, IDE Key
  Management, and their ABI interfaces. The proposed Alpha1 DUT-validation
  target is 121 rules: 38 active and 83 with gated bodies or plans. R0023 and
  R0041 have Architect-agreed exclusions from DUT validation;
  catalog checks for them are not DUT compliance results. Active
  tests can still report SKIP when a platform prerequisite is absent. This is
  development status, not release coverage.
- **Temporary CI gate:** R0207, R0209, and R0210 retain their strict CXL.cachemem
  test bodies but are not registered while CI lacks the paired firmware/model
  support. Restore them with final qualification and the CI golden-log update.
- **GPT CI gate:** R0111, R0113, and R0117 retain their strict bodies but are
  not registered while CI's TF-A rejects their NS setup/restoration with
  `DENIED` under the GDI profile. Re-enable with qualified firmware and CI logs.
- **Rule status and algorithms:** See the
  [scenario document](docs/FIRME_Architecture_Compliance_Suite_Scenario_Document.rst)
  and [ABI interface coverage](docs/FIRME_Alpha1_ABI_Interface_Coverage.md).
- **Execution environments:** Arm FVP (pre-silicon) and silicon (platform dependent)

## Release quality and known results

**Release quality: Alpha (qualification incomplete).** This is development
software, not a fully qualified compliance release. Registered tests do not
imply complete coverage, and a completed regression is not a passing verdict.
The startup log reports this quality level separately from the suite version.

### Recorded qualification snapshot

The following results are from **14 October 2026**, for the complete-coverage
candidate `5328f36e`, `SUITE=all`, with 121 registered tests. GCC and Clang
reported the same per-rule results. These are historical measurements, not
expected results for every commit, firmware revision or platform; earlier
review commits and module-only builds can register fewer tests.

| Firmware/model configuration | PASS | FAIL | SKIP |
|---|---:|---:|---:|
| Stock TF-A `510355478850` and reference FVP | 28 | 31 | 62 |
| TF-A with the proposed same-GPI fix and reference FVP | 35 | 38 | 48 |
| Locally extended TF-A/model with CXL IDE and MEC refresh backends | 50 | 30 | 41 |

The experimental configuration is not the default setup and its results do
not qualify unmodified TF-A/FVP. A fix can expose additional failures by
allowing previously blocked tests to execute.

### Failure and skip interpretation

- **DUT failures:** missing mandatory Granule Management ABIs, IDE feature
  register 1 returning `NOT_SUPPORTED`, and incorrect GPT status/cleanup
  responses remain failures; tests must not accept incorrect results.
- **Firmware/model limitations:** valid CXL requests are rejected by the
  recorded stock path, and its MEC refresh callback returns success without
  refreshing the context. Functional backends are needed to qualify effects.
- **ACS gaps:** Realm shared-buffer adapters, L1/concurrency fixtures,
  Selective IDE/two-ECAM observations and some Base evidence remain incomplete.
  On stock/proposed TF-A, NS checks for R0076, R0077 and R0081 pass;
  missing ACS Realm adapters prevent overall PASS for those rules.
- **Dependent skips:** the stock run blocks 18 GPT entries after R0094 leaves
  scratch memory unsafe to reuse. Those bodies do not execute and are not
  18 independently unsupported features. Keep the original failure visible.
- **Optional or conditional cases:** absent optional L1 support and unavailable
  applicable observations must be identified explicitly. Neither SKIP nor
  missing ACS setup counts as PASS; unresolved architectural observability
  needs an explicit disposition rather than a fabricated support diagnosis.

Use the per-test diagnostics and final summary for the actual run. Retain
firmware/model revisions and configuration alongside results, and reassess
these limitations after fixes. Do not use the historical totals as a golden
log or a release sign-off criterion.

## FIRME spec version mapping

| FIRME-ACS version | Git tag | FIRME spec version |
|---|---|---|
| FIRME Alpha1 ACS v1.0.1 Alpha | v26.10_FIRME_1.0.1 (target) | DEN0149 FIRME 1.0 ALP4 |
| FIRME Alpha1 ACS v0.5 Alpha | v26.03_FIRME_0.5 | DEN0149 FIRME 1.0 ALP1 |

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
- `docs/`: validation methodology, porting guide and scenario document.
- `platform/`: PAL implementations (environment + target), for example:
  `platform/pal_baremetal/targets/fvp/` (UART base, GPT window, PCIe ECAM, GIC bases).
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
- **Compiler alignment:** all payload, PAL, VAL and test C sources are built
  with `-mstrict-align` for both GCC and Clang. Shared code can execute before
  MMU setup or in the MMU-disabled Secure shim, where unaligned memory accesses
  can fault. Keep this option in partner builds; it prevents compiler-generated
  unaligned accesses without disabling optimization or changing test behavior.
- **Atomic operations:** C sources use `-mno-outline-atomics` so synchronization
  stays self-contained in the bare-metal images instead of requiring compiler
  runtime atomic-dispatch helpers.

## Multiprocessor infrastructure

The FVP build includes per-PE worker boot, coherent job/result exchange,
timeouts and verified CPU_OFF cleanup. Tests can register an MP entry with a
specific PE count or `VAL_TEST_ALL_PES`, then coordinate payload phases through
VAL. R0239 uses this normal execution path to collect every PE's MECID width,
include every platform-listed SMMU's hardware width, and check the DUT against
the system minimum. No separate qualification binary is required.
Collection runs serially across PEs; concurrent scenarios require explicit
synchronization rather than parallelizing existing rule bodies automatically.
Enable modelled caches with `FVP_CACHE_STATE_MODELLED=1` when qualifying MP changes.
See [MP execution](docs/MP_Execution.md) for APIs, cache ownership, platform
requirements and the remaining limits on parallel rule execution.

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

The runner's exit status reports execution completion, not test PASS/FAIL.
A completed run returns zero even when individual tests fail; CI evaluates
the unchanged UART results against its golden log. Build/configuration errors,
abnormal model exits and incomplete runs return nonzero. Normal completion
uses PSCI SYSTEM_OFF; the configured timeout remains a watchdog. A complete
summary does not override a model crash.

Useful knobs:
- `TFA_DIR=/path/to/trusted-firmware-a`
- `BUILD_MODE=auto|source|always|never` (controls firmware artifact reuse)
- `BUILD_TFA=auto|always|never` (controls TF-A rebuilds only)
- `FVP_TIMEOUT_SECS=240` (auto-terminate the model)
- `FVP_GPT_PROFILE=baseline|gpc2|gdi` (defaults to `gdi` for `SUITE=all|gpt`,
  otherwise `baseline`). Both model clusters and TF-A are configured together;
  the runner verifies actual CPU feature discovery before accepting results.
- `FVP_DRAM_SIZE_GB=8` and `FVP_CACHE_STATE_MODELLED=1` are the defaults for
  `SUITE=all|gpt`. Extra DRAM backs future high-DRAM GPT fixtures; it does not
  provide missing firmware ABIs or authorize use of unreserved memory.
- `FVP_ENABLE_MEC=1` (enable FVP MEC/MPE parameters for MECID management tests)
- `FVP_MECID_WIDTH=16` (default MECID width when MEC is enabled)
- SMMU MEC discovery is enabled automatically when MEC is enabled, including
  `SUITE=all` and `SUITE=mec`. `FVP_ENABLE_SMMU_MEC=0` is a diagnostic override;
  without usable SMMU evidence R0239 cannot claim complete requester coverage.
- `FVP_ENABLE_IDE=1` (enable TF-A/FVP IDE key management support when running `SUITE=all|ide`)
- The standalone `tools/scripts/build_tfa_fvp.sh` uses the same IDE defaults
  as the runner: `SUITE=all|ide` enables IDE support, the pending-operation
  tracker and the FVP simulator unless explicitly overridden. Separate CI
  build/run stages therefore use the same feature configuration.
- `FVP_IDE_KM_PENDING_OPS_MAX=1` (enable non-blocking IDE operation tracking for POLL coverage)
- `FVP_SIMULATE_IDE_KM_UNIT=1` (enable the FVP IDE KM simulation path in TF-A)
- `FVP_PCIE_HIERARCHY=/path/to/topology.json` (load a PCIe topology through the
  model's `pci.hierarchy_file_name` parameter; the default topology is unchanged)
- `UART_LOG=cca/firme/build-fvp-uart.log`, `MODEL_LOG=cca/firme/build-fvp-model.log`

### Experimental CXL programming backend

`FVP_FIRME_CXL_IDE=1` selects an optional, local TF-A/FVP Root Port programming
backend. It requires `FVP_ENABLE_IDE=1`, an explicit `FVP_PCIE_HIERARCHY`, and
matching firmware and model implementations. It is disabled by default and is
not available in an unmodified upstream TF-A or standard FVP release.

The backend option is recorded in both build stamps. `BUILD_MODE=auto` reuses
existing images and builds only when they are missing. `never` runs packaged
images without a TF-A checkout. Build manifests verify hashes and configuration.
Use `BUILD_MODE=source` to rebuild after source/profile changes or `always` for
an explicit build; tracked diffs and untracked sources participate in freshness
checks. Run-only stages do not compare checkout paths or source timestamps.
A JSON topology or successful key-programming acknowledgment alone does not
qualify IDE traffic protection. Endpoint discovery, authenticated key exchange,
Root Port programming, traffic checks, and cleanup must all be validated before
enabling the corresponding end-to-end tests.

Initialize the pinned dependencies once, including their nested submodules:

```sh
git submodule update --init --recursive
```

Run this from the repository root before configuring the optional SPDM build.
The build uses `cca/firme/ext/libspdm` and `cca/firme/ext/spdm-emu` directly.
Configuration and builds do not download or update dependency revisions.
Re-run the submodule command after switching to a patchset that changes the
pinned dependency revisions.
Optional `FIRME_LIBSPDM_DIR` and `FIRME_SPDM_EMU_DIR` overrides are available for dependency development, but are not required for normal use.

For opt-in paired-key and traffic checks, use an AArch64 GCC or Clang
toolchain and a single DER-encoded root CA certificate trusted for the
endpoint's SPDM chain:

```sh
export FIRME_ENABLE_CXL_ENDPOINT_PROBE=1
export FIRME_ENABLE_CXL_SPDM=1
export FIRME_SPDM_ROOT_CERT=/path/to/responder-root-ca.der
export FVP_FIRME_CXL_IDE=1
export FVP_PCIE_HIERARCHY=/path/to/type2-topology.json
cca/firme/tools/scripts/run_fvp_regression.sh
```

The SPDM crypto configuration accepts DER certificates; PEM text parsing is
not built into the payload.

The runner enables FEAT_RNG and places the experimental CXL.mem aperture at
4 GiB. The topology must reserve the fixture's Root Port/endpoint BARs and HDM
decoder window; these resources must not overlap firmware or ordinary DRAM.
The FVP PAL checks for unused resources and restores the temporary endpoint
BAR, bridge window, decoders, EL2 mapping and address-size setting afterward.

R0214, R0216 and R0218 each run a fresh host-side functional check after their
Realm ABI status checks. The check uses that same FIRME caller instance;
the host only coordinates endpoint setup and traffic observation. Each rule
keeps one authenticated SPDM session open while it:

1. Discovers the configured endpoint and confirms IDE service availability in
   the caller instance already exercised by the rule.
2. Generates independent keys and programs each Root Port RX/TX key together
   with its corresponding endpoint TX/RX key over secured DOE.
3. Completes FIRME operations, using POLL when required, and enables receivers
   before transmitters.
4. Writes and reads CXL.mem through the model's transaction-level AES-GCM path.
   It requires matching data and evidence that both key directions were used.
5. For R0214, deliberately mismatches each direction's endpoint key, requires
   link-level rejection, restores the key, and verifies traffic works again.
   For R0216/R0218, stops only one Root Port direction, leaves the endpoint
   active, requires rejection, and issues Root Port GO to restore traffic.
   Both directions are checked. An MMU/GPC fault without a link rejection
   does not satisfy the negative check.
6. Stops transmitters before receivers, ends the
   SPDM session, and releases ACS key material and temporary resources.

Failures are attributed to the owning rule with stage/direction diagnostics,
and included in the normal regression summary. Without an observation backend,
these three rules report SKIP after the status checks, not a status-only PASS.
The check remains a functional scenario, not proof of every conditional error
path or IDE type. Namespace isolation and generated ABI scenario gates remain
until their own prerequisites and validation are complete. This local fixture
validates paired key use on CXL.mem transactions, not CXL flit/IV protocol,
CXL.cache coherency, replay protection, or CDA/TDISP device assignment. It
does not change release rule gates or make an unmodified FVP compliant.
The NS test harness owns the SPDM session; the Realm shim issues FIRME calls
when IDE is advertised there. This is not a production RMM device owner.
The synchronous local Root Port backend does not itself qualify asynchronous
CXL completion. Existing Selective IDE status tests retain their separate
TF-A simulation path.

This build negotiates SPDM 1.2 with RSA-3072, SHA-256, P-256 ECDHE and
AES-256-GCM; the model's algorithms and certificate chain must match.

### TF-A Secure Partition layout file (`tools/config/sp_layout.json`)
FIRME-ACS does not package Secure Partitions by default (it uses a Secure shim at S-EL2 and an internal
S-EL2→S-EL1 entry path for SEL1 caller coverage). For this reason, `cca/firme/tools/config/sp_layout.json` is
an empty JSON object (`{}`).

When the file is empty, `cca/firme/tools/scripts/build_tfa_fvp.sh` can build TF-A without requiring the Python
`fdt` module (it will provide an empty `sp_gen.mk` fallback when needed). If you populate `sp_layout.json` to
package SPs, ensure your Python environment provides `fdt` so TF-A can generate `sp_gen.mk`.

## Documentation
- Validation methodology (updated RST draft):
  [FIRME-ACS validation methodology](docs/arm_firme_architecture_compliance_suite_validation_methodology.rst).
- IDE model/firmware prerequisites and qualification limits: [IDE validation setup](docs/IDE_Validation_Setup.md)
- Porting: `cca/firme/docs/porting_guide.md`
- Scenarios / rule coverage: `cca/firme/docs/FIRME_Architecture_Compliance_Suite_Scenario_Document.rst`

## Release readiness audit

Run `python3 cca/firme/tools/scripts/audit_alpha1_coverage.py --inventory-only`
to check the 123-rule inventory, test links, generated ABI catalog, and active
registry. After a complete `SUITE=all` FVP run, pass the UART0 log with
`--uart-log <path>`. The release check fails while any rule is gated, skipped,
or failing; a passing inventory check alone is not a release qualification.

## Experimental MEC refresh backend

`FVP_FIRME_MEC_REFRESH=1` selects a local TF-A/FVP MEC refresh interface.
It requires matching firmware and model support, RME/MEC enabled, and
`FVP_CACHE_STATE_MODELLED=1`. It is off by default and is not an upstream FVP
capability. The runner checks the firmware build stamp to prevent mismatched
backend settings; rebuild firmware after changing the option.

This functional model path is for validating context-replacement sequencing
and stale-data/new-context observations, not cryptographic-strength evidence.
It does not automatically enable registry-gated rules.

## Native ACS regression checks

These checks validate ACS logic with controlled fixtures, not DUT compliance.
Run them from the repository root with Python 3 and a native C compiler:

```sh
python3 cca/firme/tools/scripts/test_check_abi_callers.py
python3 cca/firme/tools/scripts/check_mec_regressions.py
bash cca/firme/tools/scripts/check_dispatch_native.sh
bash cca/firme/tools/scripts/check_gpt_native.sh
bash cca/firme/tools/scripts/check_ide_native.sh
python3 cca/firme/tools/scripts/check_build_config.py
python3 cca/firme/tools/scripts/check_gpt_config.py
python3 cca/firme/tools/scripts/check_alpha1_coverage.py
```

Use `HOST_CC=clang` for Python-driven C checks and `CC=clang` for shell-driven
checks. IDE checks require the pinned submodule headers; `SANITIZE=1` enables
their ASan/UBSan coverage. The optional MEC `--mmio` check also requires the
Python Unicorn package and GNU AArch64 tools selected with `CROSS_COMPILE`.

## Feedback, contributions, and support
- Email: [support-firme-acs@arm.com](mailto:support-firme-acs@arm.com)
- GitHub Issues: [sysarch-acs issue tracker](https://github.com/ARM-software/sysarch-acs/issues)
- Contributions: [GitHub Pull Requests](https://github.com/ARM-software/sysarch-acs/pulls)

## License
FIRME-ACS (`cca/firme/`) is licensed under the **BSD 3-Clause License**. See
`cca/firme/LICENSE.md` and individual source headers for details.

--------------

*Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.*
