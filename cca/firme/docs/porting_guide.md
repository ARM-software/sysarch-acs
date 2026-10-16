# FIRME-ACS Porting Guide

This guide describes the platform configuration and fixtures needed to port
FIRME-ACS. The execution model uses an NS-EL2 host, S-EL2 Secure payload and
R-EL2 Realm payload. Build prerequisites and release limitations are in the
[README](../README.md).

## 1. Platform configuration (PAL)

PAL sources are organized by environment and target. Create
`cca/firme/platform/pal_baremetal/targets/<your_platform>/{include,src}/`,
using `fvp` as the reference target. The following paths are relative to
that target directory; general configuration belongs in `include/pal_platform.h`.

### Platform access and inventory

- UART base for logs: `PLAT_NS_UART_BASE`
- End-of-run shutdown: implement `pal_system_off()` after suite cleanup.
  FVP uses PSCI `SYSTEM_OFF`; its runner enables
  `bp.ve_sysregs.exit_on_shutdown=1` for a normal simulator exit.
  Returning from this hook is an execution error, not a test verdict.
- PE topology: `include/pal_pe.h` and `src/pal_pe.c` provide the immutable
  `PAL_PE_COUNT` / `PAL_PE_MPIDRS` table. Match the actual affinities, including
  the MPIDR.MT interpretation, rather than deriving them from a core count.
  Provide only one entry on a genuine single-PE platform. Worker support
  requires coherent memory and PSCI CPU_ON/CPU_OFF/AFFINITY_INFO over SMC.
  See [MP execution](MP_Execution.md) for the full partner contract.
- GIC bases: `PLAT_GICD_BASE`, `PLAT_GICR_BASE`
- SMMU inventory: `include/pal_iovirt.h`
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

### GPT memory and scenario fixtures

Reserve backed, disposable GPT test memory exclusively for ACS and map it in
each participating caller world. It must not overlap payload images, live
firmware or DMA buffers. Increasing model DRAM does not create a reservation.

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

Optional deterministic GPT scenarios are exposed through `include/pal_firme_gpt.h`.
Implement `pal_firme_gpt_scenario_prepare()` and
`pal_firme_gpt_scenario_cleanup()` only when the platform can reliably create
the requested long-running or error condition. Implement the
`pal_firme_gpt_concurrent_info_*()` hooks when MP execution can overlap a
real protected-range state change with `FIRME_GM_GPT_INFO`. PAL prepares,
synchronizes, and cleans up the second PE; the rule test invokes the
production GPT_INFO wrapper and validates its raw result. A GPI change alone
need not change the protected-range state. The default FVP PAL cannot yet
provide this overlap; missing mandatory GPT_INFO support still fails.

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

Register reservation, observation and release callbacks through
`pal_firme_gpt_memory_register()` before dispatch. Reservations must be
exclusive across all PEs and caller worlds; observations must be independent
of the ABI response. VAL coordinates cross-world access probes, preserves
continuation state and quarantines resources when restoration is uncertain.

For optional L1 tests, configure `pal_firme_gpt_memory.h`: set
`PLAT_FIRME_GPT_L1_BACKED_RESERVED` only after reserving the declared
`PLAT_FIRME_GPT_L1_RANGE_BASE/SIZE` and `PLAT_FIRME_GPT_L1_POOL_BASE/SIZE`
against firmware, payload and DMA use. Set `PLAT_FIRME_GPT_L1_INITIAL_GPI`
to the boot-time range GPI. VAL validates geometry, ownership, protected-range
state and restoration. The default FVP configuration leaves L1 reservations
disabled.
L1 also needs an exact Root-GPI observer: faults in NS, Secure and Realm alone
cannot distinguish Root ownership from some invalid GPT states. The reference
Host allocator declines L1 fixtures until that observer is supplied; enabling
the reservation macros alone does not qualify L1 coverage.

### PCIe and IDE fixtures

The ECAM base must identify the Root Port namespace used by the platform's
FIRME IDE implementation. Root Port and endpoint IDs use PCI requester-ID
encoding: `bus[15:8]`, `device[7:3]`, `function[2:0]`.

Run destructive IDE tests only on an ACS-owned PCIe path. VAL discovers
capabilities, selects unused stream blocks and assigns the Selective Stream
ID; a PAL-configured Stream ID is not required for these tests. Configured or
locked blocks cannot be reused simply because they are disabled. VAL restores
temporary bus/stream settings and releases its RMEDA lock after cleanup.
Uncertain completion or failed cleanup prevents reuse.

The inactive Selective stream fixture supports ABI checks, not protected
traffic evidence. Endpoint sessions and independent operation observers need
separate support. See [IDE validation setup](IDE_Validation_Setup.md) for the
authenticated CXL path and its qualification limits.

The IDE namespace rules R0211 and R0212 use
`include/pal_firme_ide_namespace.h`.
Provide two real Root Port/endpoint paths with distinct keys and caller
handles. For R0211, use the same keyset ID under two ECAM bases; for R0212,
use different keyset IDs in one ECAM space. The PAL `prepare` hook must
configure the endpoints and reserve streams without programming the Root
Ports. The test invokes the FIRME PROG, GO, POLL and STOP ABIs. The `observe`
hook must independently verify protected traffic and stopped-state behavior
for both paths; do not infer success from ABI status alone. `cleanup` must
restore endpoint and platform state even after a failing check. The reference
FVP lacks the two-ECAM R0211 fixture; this work is deferred, not waived.
The paired CXL backend supplies R0212 through independently observed RX/TX
contexts. Neither an ABI status nor a single ECAM with extra Root Ports proves
two-ECAM isolation.

The generated IDE interface plans R0358-R0365 use
`include/pal_firme_ide_abi.h`.
Implement `pal_firme_ide_abi_prepare()` with one reserved Root Port/endpoint
path, a valid keyset and key, a caller handle, an invalid ECAM address, and a
cleanup token. The path must support safe PROG/GO/STOP sequencing and
asynchronous POLL where available. `pal_firme_ide_abi_cleanup()` must restore
Root Port and endpoint state even after a failed case; it must not return
`READY` when cleanup is incomplete. A pending operation retains its handle,
keyset, owning PE, endpoint session and buffers until POLL completes; an
uncertain result quarantines those resources rather than releasing them.
Return `UNAVAILABLE` on platforms without those facilities. In the paired
backend, Host owns the endpoint session and the selected NS or Realm caller
executes every FIRME call, including setup and cleanup. POLL positive cases
require a genuinely pending operation; ABI responses alone are not endpoint
evidence.

### Shared-buffer fixtures

The NS shared-buffer adapter can invoke `FIRME_ATTEST_PAT_GET` when the DUT
advertises it. Its FVP PAL reserves a private buffer, checks the live caller
mapping and serializes PAT ownership. Partial retrieval is bounded; incomplete
cleanup quarantines the buffer and instance. Other targets can implement
`val_firme_shared_buffer_acquire_ns_pat()` and its observation/release hooks.
Obtain independent EL3 attribute evidence for R0070 and buffer PAS/GPI
evidence for R0071; a successful PAT invocation does not prove either.
Mapping lifetime is informational I0073, not a separate test requirement.
A service without a defined remaining-size output on SUCCESS cannot qualify
that output rule.

## 2. Build + FVP regression

FIRME-ACS is cross-compiled. Set `CROSS_COMPILE` to the AArch64 toolchain
prefix; see the [README](../README.md#build) for direct CMake commands.

Optional `FIRME_ENABLE_CXL_SPDM=1` builds require the bare-metal GNU toolchain's
newlib `libc.a`, including when Clang compiles the payload. CMake discovers it
from the GCC compiler, or from `CROSS_COMPILE` for Clang, and links the required
runtime into the host payload. Default builds with SPDM disabled do not
require newlib.

End-to-end run (builds payloads + TF-A as needed, then runs FVP and captures logs):

```sh
export CROSS_COMPILE=/path/to/toolchain/bin/aarch64-none-elf-
export MODEL_BASE=/path/to/FVP_Base_RevC-2xAEMvA
export TFA_DIR=/path/to/trusted-firmware-a
cca/firme/tools/scripts/run_fvp_regression.sh
```

Optional knobs:

```sh
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
firmware. Use `BUILD_MODE=source` to rebuild when changing profiles:

```sh
BUILD_MODE=source SUITE=gpt FVP_GPT_PROFILE=gdi cca/firme/tools/scripts/run_fvp_regression.sh
BUILD_MODE=source SUITE=gpt FVP_GPT_PROFILE=gpc2 cca/firme/tools/scripts/run_fvp_regression.sh
BUILD_MODE=source SUITE=gpt FVP_GPT_PROFILE=baseline cca/firme/tools/scripts/run_fvp_regression.sh
```

The boot log reports discovered RME/GPC2/GDI support. Missing or mismatched
discovery is a setup failure, not a passing regression. Absent GPC2/GDI still
requires `INVALID_PARAMETERS` for its unallocated target GPI encodings.
Enabling a feature does not excuse incorrect negative-case responses.
Feature profiles do not change rule registration. Missing GPT_INFO, L1 lifecycle,
or retained-operation support cannot be enabled by these model parameters.
Keep suite and feature settings consistent between separate build and run
stages, and retain the firmware revision with the binaries used for the run.

## 3. Adding new tests

- Maintain a **1:1 mapping**: one DEN0149 rule per file under `cca/firme/test/<module>/`.
- Keep rule logic in `test/`, and expose reusable helpers through `val/` (not `test/include/`).
- Use the NS dispatcher to aggregate results and print a final outcome per rule.
