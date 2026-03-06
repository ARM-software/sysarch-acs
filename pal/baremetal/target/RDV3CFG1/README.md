# Baremetal README for RDV3-Cfg1 Platform

## Steps to Build
Follow the build steps mentioned in [README](../../README.md) with the TARGET parameter as:

```bash
-DTARGET=RDV3CFG1
```

## Running ACS with Bootwrapper on RDV3-Cfg1
**1. In RDV3-Cfg1 software stack make following change:**

  In <rdv3cfg1_path>/build-scripts/build-target-bins.sh - replace uefi.bin with acs.bin

```
  if [ "${!tfa_tbbr_enabled}" == "1" ]; then
      $TOP_DIR/$TF_A_PATH/tools/cert_create/cert_create  \
      ${cert_tool_param} \
-     ${bl33_param_id} ${OUTDIR}/${!uefi_out}/uefi.bin
+     ${bl33_param_id} ${OUTDIR}/${!uefi_out}/acs.bin
  fi

  ${fip_tool} update \
  ${fip_param} \
- ${bl33_param_id} ${OUTDIR}/${!uefi_out}/uefi.bin \
+ ${bl33_param_id} ${OUTDIR}/${!uefi_out}/acs.bin \
  ${PLATDIR}/${!target_name}/fip-uefi.bin

```

**2. Repackage the FIP image with this new binary**
- cp <sysarch_acs>/<bsa/sbsa_build>/build/output/<acs>.bin <rdv3cfg1_path>/output/rdv3cfg1/components/css-common/acs.bin

- cd <rdv3cfg1_path>

- ./build-scripts/build-test-uefi.sh -p rdv3cfg1 package

- export MODEL=<path_to_FVP_RDV3_Cfg1_model>

- cd <rdv3cfg1>/model-scripts/rdinfra/platforms/rdv3cfg1

- ./run_model.sh

## Known Limitations

**Note:** To run PCIe MSE tests on RDV3-Cfg1 FVP, add `-C pcie_group_0.pcie0.ur_rao_wi=1` to `run_model.sh` so UR handling is enabled and the tests do not trigger an EL3 exception. If PCIe MSE tests still fail, set `g_pcie_skip_dp_nic_ms=1` in `apps/baremetal/acs_globals.c` locally.

-----------------

*Copyright (c) 2025-2026, Arm Limited and Contributors. All rights reserved.*
