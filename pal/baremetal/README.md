# Baremetal README
**Please Note**: The code in the "baremetal" directory is only a reference code for implementation of PAL API's and it has not been verified on any model or SoC's.
The directory baremetal consists of the reference code of the PAL API's specific to a platform.
Description of each directory are as follows:

## Directory Structure
&emsp; - `base`: The implementation for all modules are present in this directory.\
&emsp; &emsp; - `include`: Consists of the include files\
&emsp; &emsp; - `src`: Source files for all modules which do not require user modification.\
&emsp; &emsp; &emsp; Eg: Info tables parsing, PCIe enumeration code, etc.

&emsp; - `target`: Contains Platform specific code. The details in this folder need to be modified w.r.t the platform\
&emsp; &emsp; - `<platformname>`: Info tables parsing, PCIe enumeration code, etc\
&emsp; &emsp; &emsp; - `include`: Consists of the include files for the platform specific information\
&emsp; &emsp; &emsp; - `src`: Source files for all modules which require user modification.\

## Build Steps

### Pre-requisite

> Note: <acs>.bin stands for either bsa.bin or sbsa.bin or pc_bsa.bin. Any platform specific changes can be done by using `TARGET_BAREMETAL` macro definition. The baremetal reference code is located in [baremetal](.).

Run the command
- `cd sysarch-acs`
- `python tools/scripts/generate.py <platformname>`

> Eg: `python tools/scripts/generate.py RDN2`
> This command will create a folder RDN2 under the `pal/target` folder path and the files `pal_bsa.c` and `pal_sbsa.c` files within the `RDN2/src` folder.

1. To compile BSA, perform the following steps\
&emsp; 1.1 `cd sysarch-acs`\
&emsp; 1.2 `export CROSS_COMPILE=<path_to_the_toolchain>/bin/aarch64-none-elf-`\
&emsp; 1.3 `cmake --preset bsa -DTARGET="Target platform"`\
&emsp; 1.4 `cmake --build --preset bsa`

2. To compile SBSA, perform the following steps\
&emsp; 2.1 `cd sysarch-acs`\
&emsp; 2.2 `export CROSS_COMPILE=<path_to_the_toolchain>/bin/aarch64-none-elf-`\
&emsp; 2.3 `cmake --preset sbsa -DTARGET="Target platform"`\
&emsp; 2.4 `cmake --build --preset sbsa`

3. To compile PC_BSA, perform the following steps\
&emsp; 3.1 `cd sysarch-acs`\
&emsp; 3.2 `export CROSS_COMPILE=<path_to_the_toolchain>/bin/aarch64-none-elf-`\
&emsp; 3.3 `cmake --preset pc_bsa -DTARGET="Target platform"`\
&emsp; 3.4 `cmake --build --preset pc_bsa`

</br>

> **Note:**
> You can check available presets using `cmake --list-presets` 
> If you do not provide `-DTARGET`, defaults to `RDN2`.  
> If you like to use make command do `cmake --preset acs_all; cd build; make bsa` (for all baremetal acs `make acs_all`)   
> Recommended: CMake v3.21 (min version to support --preset), GCC v12.2

</br>

```
CMake Command Line Options:
 `-DARM_ARCH_MAJOR` = Arch major version. Default value is 9.
 `-DARM_ARCH_MINOR` = Arch minor version. Default value is 0.
 `-DCROSS_COMPILE`  = Cross compiler path
 `-DTARGET`         = Target platform. Should be same as folder under baremetal/target/
 `-DACS`            = To compile <bsa/sbsa/pc_bsa> ACS
```

</br>

> On a successful build, *.bin, *.elf, *.img and debug binaries are generated at `build/<acs>_build/output` directory. The output library files will be generated at `build/<acs>_build/tools/cmake` directory.

## Running ACS with Bootwrapper on RDN2

**1. In RDN2 software stack make following change:**

  In `<rdn2_path>/build-scripts/build-target-bins.sh` - replace `uefi.bin` with `acs_latest.bin`

```bash
  if [ "${!tfa_tbbr_enabled}" == "1" ]; then
      $TOP_DIR/$TF_A_PATH/tools/cert_create/cert_create  \
      ${cert_tool_param} \
-     ${bl33_param_id} ${OUTDIR}/${!uefi_out}/uefi.bin
+     ${bl33_param_id} ${OUTDIR}/${!uefi_out}/acs_latest.bin
  fi

  ${fip_tool} update \
  ${fip_param} \
- ${bl33_param_id} ${OUTDIR}/${!uefi_out}/uefi.bin \
+ ${bl33_param_id} ${OUTDIR}/${!uefi_out}/acs_latest.bin \
  ${PLATDIR}/${!target_name}/fip-uefi.bin

```

**2. Repackage the FIP image with this new binary**
- `cp <sysarch-acs>/build/<acs>_build/output/<acs>.bin <rdn2_path>/output/rdn2/components/css-common/acs_latest.bin`
- `cd <rdn2_path>`
- `./build-scripts/rdinfra/build-test-acs.sh -p rdn2 package`
- `export MODEL=<path_to_FVP_RDN2_model>`
- `cd <rdn2>/model-scripts/rdinfra/platforms/rdn2`
- `./run_model.sh`


For more details on how to port the reference code to a specific platform and for further customisation please refer to the [User Guide](../../docs/arm_bsa_architecture_compliance_bare-metal_user_guide.pdf)

-----------------

*Copyright (c) 2023-2025, Arm Limited and Contributors. All rights reserved.*
