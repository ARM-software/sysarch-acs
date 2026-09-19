# ACS EL3 SMC porting guide

This guide describes how to enable the ACS EL3 services on a platform other
than the current RDV3 reference implementation, or with an EL3 firmware
implementation other than TF-A.

## 1. Confirm the EL3 dispatch interface

The ACS client invokes SMC64 FID `0xC7000030`. EL3 must reserve this FID and
route it to the ACS handler with the following arguments:

```text
x0 = 0xC7000030
x1 = ACS service selector
x2 = service argument 0
x3 = service argument 1
x4 = service argument 2
```

On success, return status `0` in `x0` and the service value in `x1`. For an
unknown FID or selector, return the SMCCC unknown-function value, observed by
ACS as all ones. TF-A defines this value as `SMC_UNK`.

### TF-A requirement

Use TF-A v2.14 or newer. The required support was introduced by commit
[`f69f551`](https://github.com/ARM-software/arm-trusted-firmware/commit/f69f551269f1d126877889a1cab27cc1692316ea)
and includes:

- the `0xC7000030` ACS allocation in the vendor-specific EL3 service range;
- `ENABLE_ACS_SMC` and `PLAT_ARM_ACS_SMC_HANDLER` build controls;
- dispatch from the vendor EL3 runtime service to `plat_arm_acs_smc_handler()`; and
- a default platform ACS handler symbol.

## 2. Add the platform configuration

Copy the RDV3 reference configuration for the target platform:

```sh
cp -r services/acs_smc/platform/rdv3 services/acs_smc/platform/<platform name>
```

When TF-A is used, no changes to `platform_el3.h` are required; it maps the
override to TF-A's `ARM_SYS_CNTCTL_BASE`. For non-TF-A firmware, update the
copied header with the platform-specific system counter control base:

```c
#define PLATFORM_OVERRIDE_SYS_COUNTER_CNTCTL_BASE  <system counter control base>
```

## 3. Integrate the handler into TF-A

This section applies only when TF-A is used as the EL3 firmware.

`acs_smc.mk` adds the C and AArch64 sources to `VENDOR_EL3_SRCS`, adds the
required include paths, and passes this linker option:

```text
-Wl,--wrap=plat_arm_acs_smc_handler
```

The provided script performs the integration automatically:

```sh
./tools/scripts/build_acs_smc.sh \
    TFA_PATH=/path/to/arm-trusted-firmware \
    PLAT=<plat> \
    CC=aarch64-linux-gnu-gcc \
    <platform TF-A flags>
```

The script enables `ENABLE_ACS_SMC=1`, copies `services/acs_smc` under
`TF_A/plat/arm/common/acs_smc` when that directory is absent, removes stale
BL31 objects, and runs the requested TF-A build target (`bl31` by default or
`all` with `--all`).

If the copied ACS directory already exists, the script leaves it in place.
Remove or update a stale copy before relying on it in a platform-specific TF-A
integration.

## 4. Port to another EL3 implementation

This section applies only when the EL3 firmware is not TF-A.

The supplied build script cannot build non-TF-A firmware. Integrate these
sources into the firmware's EL3 image using its native build system:

```text
services/acs_smc/src/acs_el3_handler.c
services/acs_smc/src/AArch64/PlatAcsSysreg.S
services/acs_smc/include/acs_el3_handler.h
services/acs_smc/platform/<plat>/platform_el3.h
```

Then adapt the TF-A-specific interfaces used by `acs_el3_handler.c`:

| TF-A interface | Required equivalent |
| --- | --- |
| `SMC_RET1(handle, value)` | Return `value` in `x0` from the active SMC context |
| `SMC_RET2(handle, status, value)` | Return `status` in `x0` and `value` in `x1` |
| `SMC_UNK` | TF-A macro for the SMCCC unknown-function return value |
| `mmio_read_32()` / `mmio_write_32()` | EL3 32-bit MMIO accessors |
| `read_cntpct_el0()` | Ordered read of the physical count register |
| `INFO()` / `WARN()` | Firmware logging, or suitable no-op definitions |

Register an SMC64 handler for `0xC7000030` in the firmware's runtime service
dispatcher. Preserve the selector numbers, argument registers, status values,
and result registers documented in the README. The sysarch-acs VAL wrappers depend on that exact ABI.

## 5. Integrate stack packaging

Without `STACK_PATH`, the requested build outputs remain in the TF-A build
directory. With `STACK_PATH`, Step 6 of `tools/scripts/build_acs_smc.sh`
assumes the RDV3 stack convention:

```text
BL31 destination: STACK_PATH/output/PLAT/tf-bl31.bin
Packaging command: STACK_PATH/build-scripts/rdinfra/build-test-acs.sh -p PLAT package
```

For another stack, choose one of these approaches:

1. Provide `PACKAGE_SCRIPT` in the environment with a script accepting
   `-p <plat> package` and retain the expected stack output layout.
2. Modify Step 6 to copy the required TF-A build outputs to the platform's
   required destinations and call its native packaging command.

Keep `ACS_BL31_REPACKAGE_ACTIVE` or an equivalent guard if the packaging
script can invoke the ACS build script recursively.

## 6. Validate the port

1. Confirm the generated `bl31.bin` is installed in the image that the
   platform actually boots.
2. Check the EL3 log for `ACS/EL3: handler entered` when an SMC-dependent test
   runs.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| `Platform '<plat>' is not supported by ACS SMC` | Add `services/acs_smc/platform/<plat>/platform_el3.h` and match the `PLAT` spelling exactly |
| ACS tests report that the EL3 handler is absent | Verify TF-A v2.14+ ACS dispatch support, `ENABLE_ACS_SMC=1`, the linker wrapper, and the deployed BL31 |
| Base-frequency test returns zero or an implausible value | Verify the counter-control physical base and the `CNTFID0` implementation |

## License

The ACS EL3 SMC service is distributed under the Apache License 2.0.

--------------

*Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.*
