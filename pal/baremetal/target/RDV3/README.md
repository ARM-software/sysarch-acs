# Baremetal README for RDV3 Platform

## Steps to Build
Follow the build steps mentioned in [README](../../README.md) with the TARGET parameter as:

```bash
-DTARGET=RDV3
```

## Running ACS with Bootwrapper on RDV3
**1. In RDV3 software stack make following change:**

  In <rdv3_path>/build-scripts/build-target-bins.sh - replace uefi.bin with acs.bin

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
- cp <sysarch_acs>/<bsa/sbsa_build>/build/output/<acs>.bin <rdv3_path>/output/rdv3/components/css-common/acs.bin

- cd <rdv3_path>

- ./build-scripts/rdinfra/build-test-acs.sh -p rdv3 package

- export MODEL=<path_to_FVP_RDV3_model>

- cd <rdv3>/model-scripts/rdinfra/platforms/rdv3

- ./run_model.sh


*Copyright (c) 2025-2026, Arm Limited and Contributors. All rights reserved.*