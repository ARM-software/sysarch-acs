# IDE validation setup and qualification limits

## Release dependency

`SUITE=all` and `SUITE=ide` select the registered IDE tests. They do not
automatically instantiate an IDE-capable endpoint or enable the experimental
CXL backend. Model topology and firmware support are separate prerequisites.

| Rules | Required setup |
| --- | --- |
| R0205, R0214, R0216, R0218, R0225 | An ACS-owned Root Port and downstream endpoint with usable Selective IDE capabilities. VAL discovers and configures unused stream blocks; Realm performs protected RMEDA writes when required. |
| R0207, R0209, R0210 | A usable CXL.cachemem path, authenticated endpoint IDE key management and a working Root Port programming backend. The local reference experiment uses the extended FVP and matching TF-A backend. |
| R0211 | Two owned paths in different ECAM namespaces with independent traffic observations. The current single-ECAM model and reference PAL do not provide this fixture. |
| R0212 | Distinct keyset IDs and keys within one ECAM namespace. The paired CXL backend isolates RX from TX using directional traffic counters, RX STOP/GO and retained data. |
| R0358-R0365 | Host coordinates the paired endpoint session and observations for NS or Realm callers. Every FIRME setup, test and cleanup call uses the selected caller world. POLL positive cases additionally need a genuinely pending operation. |

Unavailable prerequisites result in SKIP, not successful coverage. Unexpected
DUT responses and cleanup errors remain failures. These rules are registered;
an unavailable ACS observer is an implementation gap, not evidence that the
DUT is correct or that the model alone prevents validation.

## Why the local extensions were needed

The inspected reference implementation could exercise ABI completion handling
without demonstrating the corresponding key programming or traffic effect:

- The endpoint responder acknowledged key programming without retaining the
  complete key and IV needed to validate a matching link.
- The TF-A FVP platform path used a software completion simulator or a no-op
  return. The service also rejected the CXL.cachemem IDE type before reaching
  the platform programming hook.
- Root Port and endpoint key state was not connected to a paired transaction
  check. Successful ABI status alone could therefore conceal a no-op backend.

The local extensions address those gaps; they are not changes to the ACS's
specification-derived expected results.

## Changes made in the local reference experiment

| Component | Change and rationale |
| --- | --- |
| FVP topology | Instantiate a connected Root Port and endpoint with the required IDE, CXL and DOE capabilities. For Selective status tests, enable IDE capability on both ends. |
| Endpoint model | Validate requests, retain per-direction key/IV state, require programming before activation, and implement GO, STOP and reset state transitions. This makes acknowledgements reflect modeled operations. |
| Root Port model | Add an opt-in, implementation-defined programming window for per-direction key storage, PROG/GO/STOP commands and result reporting. This supplies the platform-specific interface that firmware must drive. |
| Transaction model | Use independently programmed Root Port and endpoint keys for AES-GCM processing of modeled CXL.mem transactions. Check matching-key traffic, directional mismatch rejection and recovery, and Root Port-only STOP/GO effects. |
| TF-A service/platform | Accept the CXL.cachemem IDE type with its defined Stream/Substream encodings, route its operations to the FVP programming window, and translate actual backend outcomes. Keep the existing Selective IDE simulator distinct from this CXL backend. |
| ACS | Keep the authenticated SPDM/DOE endpoint session alive, coordinate endpoint and Root Port operations through their proper interfaces, validate completion and functional effects, and perform ordered cleanup. |

The implementation-defined model window is a local platform interface, not an
architected FIRME register block or an interface partners must copy. The DUT
still receives Root Port operations through FIRME; ACS does not bypass it by
programming Root Port keys directly.

## Selecting the experimental setup

Use a compatible locally extended model and TF-A tree, and supply a topology
JSON whose paths and endpoint credentials are valid in that environment.
No personal path or experimental firmware/model source is included in ACS.

```sh
export FVP_PCIE_HIERARCHY=/path/to/ide-capable-topology.json
export FVP_FIRME_CXL_IDE=1
export FIRME_ENABLE_CXL_ENDPOINT_PROBE=1
export FIRME_ENABLE_CXL_SPDM=1
export FIRME_SPDM_ROOT_CERT=/path/to/test-trust-anchor.der
BUILD_MODE=always SUITE=all bash cca/firme/tools/scripts/run_fvp_regression.sh
```

Set the usual `CROSS_COMPILE`, `TFA_DIR` and `MODEL_BASE` first, as described
in the [porting guide](porting_guide.md). These experimental options remain
opt-in; they must not be applied blindly to an unmodified upstream build.

## Qualification boundary

Record the ACS revision, compiler, firmware, model and topology with each run's
PASS, FAIL, SKIP and registry-gate results. A working paired CXL path does not
guarantee a usable Selective IDE stream: those tests can still skip when their
own discovery and ownership prerequisites are absent. Passing active tests
does not establish complete Alpha1 rule coverage or qualify unmodified upstream
TF-A when local firmware extensions were used.

The Selective stream allocator prepares inactive configuration for ABI checks;
it does not itself establish an authenticated PCIe Selective IDE data path.
The separate CXL experiment validates transaction-level CXL.mem effects, not
full CXL.cache behavior, flit/wire-protocol conformance or production hardware
security. Release qualification must identify the exact model, firmware and
topology used, and track incomplete observations and upstream integration
separately. Native fixture tests do not qualify a live paired path.
