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
- PE topology: `include/pal_pe.h` and `src/pal_pe.c` provide the immutable
  `PAL_PE_COUNT` / `PAL_PE_MPIDRS` table. Match the actual affinities, including
  the MPIDR.MT interpretation, rather than deriving them from a core count.
  Provide only one entry on a genuine single-PE platform. Worker support
  requires coherent memory and PSCI CPU_ON/CPU_OFF/AFFINITY_INFO over SMC.
  See [MP execution](MP_Execution.md) for the full partner contract.
- GIC bases: `PLAT_GICD_BASE`, `PLAT_GICR_BASE`
- SMMU inventory: `platform/pal_baremetal/targets/<target>/include/pal_iovirt.h`
  and the table in `src/pal_iovirt.c`. Enumerate every SMMU and its NS/Realm
  page-0 addresses independently of DUT feature responses. A zero count means
  the platform genuinely has no SMMUs, not that discovery failed. R0239 reads
  hardware widths from Realm and combines them with widths from all PEs.
- Direct messaging endpoint IDs (if Secure direct messaging is used): `PLAT_FFA_*_ENDPOINT_ID`
- GPT test window + granule size:
  - `PLAT_FIRME_GPT_BASE_{NS,SECURE,REALM}`
  - `PLAT_FIRME_GPT_GRANULE_SIZE`
  - `PLAT_FIRME_GPT_PROTECTED_RANGE_SIZE`
  - `PLAT_FIRME_GPT_PROTECTED_PA_SIZE`
  - `PLAT_FIRME_GPT_INFO_PAR_STATE`
- PCIe ECAM and IDE keyset fields for IDE key management tests:
  - `PLAT_FIRME_PCIE_ECAM_BASE`, `PLAT_FIRME_PCIE_ECAM_SIZE`
  - `PLAT_FIRME_PCIE_START_BUS`, `PLAT_FIRME_PCIE_END_BUS`
  - `PLAT_FIRME_PCIE_SEGMENT`, `PLAT_FIRME_PCIE_ROOT_PORT_ID`
  - `PLAT_FIRME_PCIE_ENDPOINT_ID` if the endpoint requester ID is fixed
  - `PLAT_FIRME_PCIE_STREAM_ID`, `PLAT_FIRME_PCIE_SUBSTREAM_ID`

The GPT test window must be a PA range that is mapped and accessible in the caller world you are testing from,
and must not overlap your payload images or TF-A reserved regions. The ECAM base must identify the target
Root port namespace used by the platform FIRME IDE key management implementation. `PLAT_FIRME_PCIE_ROOT_PORT_ID` and `PLAT_FIRME_PCIE_ENDPOINT_ID` use PCI requester-ID encoding: `bus[15:8]`, `device[7:3]`, and `function[2:0]`.

Run the destructive IDE tests only on a PCIe path dedicated to ACS, with no
other driver or firmware task programming it concurrently. VAL discovers the
Root Port and endpoint IDE capabilities, selects unused stream blocks and
assigns a Stream ID; the Selective PROG/GO/STOP/POLL tests do not require a
PAL-configured Stream ID. Disabled but configured or locked blocks are not
reused. For an unenumerated Root Port, VAL temporarily assigns an unused bus
without overlapping an existing bridge window, then restores it during cleanup.
Existing stream control defaults are preserved. In Realm execution, VAL sets
and verifies only its own RMEDA stream lock, when present; it does not modify
EL3 or toggle global TDISP enablement.
Tests drain pending responses and stop keys before VAL removes its stream
configuration and releases its lock. A cleanup error remains a failure and
blocks subsequent allocation rather than reusing uncertain hardware state.

This setup configures an inactive Selective stream pair for ABI checks; it
does not enable unkeyed traffic or establish a PCIe IDE-KM endpoint session.
The authenticated CXL.cachemem endpoint flow and functional-effect checks
remain separate. Missing Selective resources are explicitly reported as
unavailable, never counted as successful end-to-end validation.

Native unit checks exercise ACS discovery, resource ownership and failure
cleanup with mocked configuration space. They are development checks, not
FIRME architectural rule coverage; the rule tests remain under `test/ide/`.

`PLAT_FIRME_GPT_PROTECTED_RANGE_SIZE` and
`PLAT_FIRME_GPT_PROTECTED_PA_SIZE` independently describe the platform
L0GPTSZ coverage and PPS. FIRME-ACS cross-checks these values against Granule
Management feature register 1 before using them; do not derive the PAL values
from the DUT response.

`PLAT_FIRME_GPT_INFO_PAR_STATE` describes the independently known initial
state of the protected address range containing the GPT test window. Use `0`
for EL3-managed/Block, `1` for EL3-managed/Table, `2` for
Host-managed/Block, or `3` for Host-managed/Table. This value is used to
cross-check the state reported by `FIRME_GM_GPT_INFO`; it must not be derived
from that ABI's output.

Optional deterministic GPT scenarios are exposed through
`platform/pal_baremetal/targets/<your_platform>/include/pal_firme_gpt.h`.
Implement `pal_firme_gpt_scenario_prepare()` and
`pal_firme_gpt_scenario_cleanup()` only when the platform can reliably create
the requested long-running or error condition. Implement the
`pal_firme_gpt_concurrent_info_*()` hooks when MP execution can overlap a
real protected-range state change with `FIRME_GM_GPT_INFO`. PAL prepares,
synchronizes, and cleans up the second PE; the rule test invokes the
production GPT_INFO wrapper and validates its raw result. MP workers are
already available, but a GPI change alone need not change the reported
protected-range state. The default FVP PAL reports this scenario as
unavailable; R0144 remains gated until a real state-changing operation and
safe cleanup are available.

The optional L1 GPT lifecycle tests use the same PAL header. Implement
`pal_firme_gpt_l1_scenario_prepare()` and
`pal_firme_gpt_l1_scenario_cleanup()` with separate Host-managed protected
ranges and L1 object storage. Each status condition must isolate only the
condition named by `pal_firme_gpt_l1_condition_t`; do not return whichever
status the DUT happens to produce. Implement
`pal_firme_gpt_l1_gpi_range_check()` so every granule in the requested range
is independently checked, not sampled. Bridge-object and scrub rules also
require the `pal_firme_gpt_l1_monitor_*()` hooks to observe temporary Root
ownership, bridge reclamation, complete object overwrite, and ordering before
the object is returned to the Non-secure PAS. PAL must restore every range,
object, retained operation, and injected failure during cleanup.

The generated GPT adapters use `pal_firme_gpt_memory_register()` to install
permanent reservation, observation and release callbacks before dispatch.
Reservations must cover backed, disposable memory and be exclusive across
all PEs and caller worlds. Observe the actual GPI independently of the ABI
response. L1 object storage includes the complete containing granules, even
when the object is smaller than one granule. The adapters retain real
continuation cookies, restore completed transitions and quarantine a lease
when safe restoration cannot be established. The default FVP does not install
these callbacks: an address constant alone is not a qualified fixture.

The IDE namespace rules R0211 and R0212 use
`platform/pal_baremetal/targets/<your_platform>/include/pal_firme_ide_namespace.h`.
Provide two real Root Port/endpoint paths with distinct keys and caller
handles. For R0211, use the same keyset ID under two ECAM bases; for R0212,
use different keyset IDs in one ECAM space. The PAL `prepare` hook must
configure the endpoints and reserve streams without programming the Root
Ports. The test invokes the FIRME PROG, GO, POLL and STOP ABIs. The `observe`
hook must independently verify protected traffic and stopped-state behavior
for both paths; do not infer success from ABI status alone. `cleanup` must
restore endpoint and platform state even after a failing check. The reference
FVP PAL reports these scenarios unavailable, so both rules remain outside
the runtime registry until an observable two-path setup is ported.

The generated IDE interface plans R0358-R0365 use
`platform/pal_baremetal/targets/<your_platform>/include/pal_firme_ide_abi.h`.
Implement `pal_firme_ide_abi_prepare()` with one reserved Root Port/endpoint
path, a valid keyset and key, a caller handle, an invalid ECAM address, and a
cleanup token. The path must support safe PROG/GO/STOP sequencing and
asynchronous POLL where available. `pal_firme_ide_abi_cleanup()` must restore
Root Port and endpoint state even after a failed case; it must not return
`READY` when cleanup is incomplete. A pending operation retains its handle,
keyset, owning PE, endpoint session and buffers until POLL completes; an
uncertain result quarantines those resources rather than releasing them.
Return `UNAVAILABLE` rather than a fake path on platforms without those
facilities. The paired NS SPDM backend provides valid GO/STOP observations;
Realm-coordinated generated calls, PROG/POLL and negative-case fixtures remain
unavailable. These eight generated plans remain registry-gated until their
complete required cases are qualified. ABI responses alone are not endpoint
evidence.

The NS shared-buffer adapter can invoke `FIRME_ATTEST_PAT_GET` when the DUT
advertises it. Its FVP PAL reserves a private buffer, checks the live caller
mapping and serializes PAT ownership. Partial retrieval is bounded; incomplete
cleanup quarantines the buffer and instance. Other targets can implement
`val_firme_shared_buffer_acquire_ns_pat()` and its observation/release hooks.
EL3 mapping attributes and PAS/GPI evidence must be obtained independently:
a successful PAT invocation does not prove them. A service without a defined
remaining-size output on SUCCESS cannot qualify that output rule.

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
export BUILD_MODE=auto           # auto (default) | source | always | never
export BUILD_TFA=auto            # auto (default) | always | never
export TFA_LOG_LEVEL=40          # 10..50 (ERROR..VERBOSE)
export FVP_GPT_PROFILE=gdi       # baseline | gpc2 | gdi; gdi for all/GPT by default
export FVP_DRAM_SIZE_GB=8        # default for all/GPT; other suites default to 4
export FVP_CACHE_STATE_MODELLED=1 # default for all/GPT; required for cache validation
export FVP_ENABLE_MEC=1          # enable FVP MEC/MPE model parameters for MEC tests
export FVP_MECID_WIDTH=16        # default MECID width for FVP MEC runs
export FVP_ENABLE_SMMU_MEC=1     # automatic with MEC; includes SMMUs in common-width validation
export FVP_ENABLE_IDE=1          # enable TF-A/FVP IDE key management support for IDE tests
export FVP_IDE_KM_PENDING_OPS_MAX=1  # enable non-blocking IDE operation tracking
export FVP_SIMULATE_IDE_KM_UNIT=1    # enable the FVP IDE KM simulation path in TF-A
```

### GPT feature profiles

`rme_support_level=2` alone does not enable GPC2 or GDI. The build and run
scripts share these profiles, including when CI builds and runs separately:

| Profile | GPC2 | GDI | TF-A `ENABLE_FEAT_RME_GDI` |
|---|---|---|---|
| `baseline` | Off | Off | 0 |
| `gpc2` | On | Off | 0 |
| `gdi` | On | On | 1 |

The runner uses `cluster{0,1}.has_rme_gpc2=1` for GPC2 and
`cluster{0,1}.has_rme_gdi=2` for GDI. The latter is a tri-state model parameter:
`1` is conditional on Armv9.6, not unconditional enablement. Do not confuse
it with the TF-A build flag. `TFA_ENABLE_RME_GDI=2` optionally selects TF-A
runtime feature detection; contradictory fixed firmware/model settings fail.

For positive and feature-absent coverage, run each profile with matching
firmware; `BUILD_MODE=auto` rebuilds when the profile changes:

```sh
SUITE=gpt FVP_GPT_PROFILE=gdi cca/firme/tools/scripts/run_fvp_regression.sh
SUITE=gpt FVP_GPT_PROFILE=gpc2 cca/firme/tools/scripts/run_fvp_regression.sh
SUITE=gpt FVP_GPT_PROFILE=baseline cca/firme/tools/scripts/run_fvp_regression.sh
```

The boot log reports discovered RME/GPC2/GDI support. Missing or mismatched
discovery is a setup failure, not a passing regression. Absent GPC2/GDI still
requires `INVALID_PARAMETERS` for its unallocated target GPI encodings.
Enabling a feature does not excuse incorrect negative-case responses.
Feature profiles do not change registry gates. Missing GPT_INFO, L1 lifecycle,
or retained-operation support cannot be enabled by these model parameters.
When `TFA_DIR` is supplied, automatic runs also compare its checkout path,
Git revision and tracked/untracked source changes against the firmware stamp.
`BUILD_MODE=never` rejects a source mismatch instead of testing stale firmware.

## 3. Adding new tests

- Maintain a **1:1 mapping**: one DEN0149 rule per file under `cca/firme/test/<module>/`.
- Keep rule logic in `test/`, and expose reusable helpers through `val/` (not `test/include/`).
- Use the NS dispatcher as the single point that aggregates results and prints a final PASS/FAIL per rule.
