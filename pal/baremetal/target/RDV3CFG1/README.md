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
Some PCIe and Exerciser tests, primarily related to interrupt/MSI generation, are expected to fail on the RDV3-Cfg1 platform. This is due to overlapping BAR register mappings observed during PCIe enumeration in the Baremetal environment. While a fix is under development, the following tests may fail:

| Test ID | Description                                | Suite    | Status |
|---------|--------------------------------------------|----------|--------|
| 830     | Check Cmd Reg memory space enable          | BSA      | Skipped in config   |
| 858     | Check MSE, CapPtr & BIST: RCiEP, RCEC      | SBSA     | Skipped in config   |
| 1506    | Generate PCIe legacy interrupt             | BSA      | Fail   |
| 1507    | Check PCIe I/O Coherency                   | BSA      | Fail   |
| 1533    | MSI(-X) triggers interrupt with unique ID  | BSA      | Fail   |
| 1535    | MSI-cap device can target any ITS block    | BSA      | Fail   |
| 1508    | Tx pending bit clear correctness RCiEP     | SBSA     | Fail   |
| 1523    | Check AER functionality for RPs            | SBSA     | Fail   |
| 1524    | Check DPC functionality for RPs            | SBSA     | Fail   |

-----------------

*Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.*