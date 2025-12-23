## Table of Contents

- [xBSA UEFI application](#xbsa-uefi-application)
- [Release Details](#release-details)
- [xBSA build steps](#xbsa-build-steps)
  - [UEFI Shell application](#uefi-shell-application)
    - [Prerequisites](#prerequisites)
    - [Setup the workspace and clone required repositories](#setup-the-workspace-and-clone-required-repositories)
    - [Build edk2 prerequisites](#build-edk2-prerequisites)
    - [Start the xBSA application build](#start-the-xbsa-application-build)
    - [Build output](#build-output)
  - [Linux application](#linux-application)
- [xBSA run steps](#xbsa-run-steps)
  - [For UEFI application](#for-uefi-application)
  - [For Linux application](#for-linux-application)
- [Related Documentation](#related-documentation)
- [Support](#support)

## xBSA UEFI application

The **xBSA UEFI application** packages the BSA, SBSA and PC-BSA test suites into a single, self-checking UEFI binary.  
It is intended for platform teams that want to validate a design once and cover the combined requirements of the Base System Architecture (BSA), Server Base System Architecture (SBSA) and PCBase System Architecture (PC-BSA) specifications.

Most tests run from the **UEFI Shell** by invoking the xBSA UEFI application.  
Selected PCIe and peripheral tests require the Exerciser VIP to achieve complete coverage.  
The test suite can also be executed in bare-metal environments; initialization of those environments remains platform-specific.

## Release Details

- **Coverage:** Aggregates validation for [BSA 1.2](https://developer.arm.com/documentation/den0094/e/?lang=en) and [SBSA 8.0](https://developer.arm.com/documentation/den0029/j/?lang=en).  
- **Execution levels:** Suitable for both Pre-Silicon and Silicon validation.  
- **Complementary requirements:** Running with the Exerciser VIP is recommended for complete PCIe compliance coverage.  
- **Linux dependencies:** The xBSA UEFI application relies on the BSA and SBSA Linux applications for tests that require an OS environment. Refer to the dedicated BSA and SBSA documentation for details.

## xBSA build steps

Follow the steps below to build the combined UEFI shell application (`xbsa_acpi.efi`) inside an edk2 workspace.

### UEFI Shell application

#### Prerequisites
- A mainstream Linux distribution on x86 or AArch64.
- Bash shell for building.
- edk2 build dependencies (compiler toolchains, required packages).  
  *Note: Package specifics vary by distribution and are out of scope for this guide.*

#### Setup the workspace and clone required repositories
```
mkdir workspace && cd workspace
git clone -b edk2-stable202508 https://github.com/tianocore/edk2
cd edk2
git submodule update --init --recursive
git clone https://github.com/tianocore/edk2-libc
git clone https://github.com/ARM-software/sysarch-acs.git ShellPkg/Application/sysarch-acs
cd -
```
- On x86 hosts download and extract the Arm GNU AArch64 toolchain:
```
wget https://developer.arm.com/-/media/Files/downloads/gnu/14.3.rel1/binrel/arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-linux-gnu.tar.xz
tar -xf arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-linux-gnu.tar.xz
export GCC_AARCH64_PREFIX=$PWD/arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-
```
- On native AArch64 hosts export the system toolchain prefix:
```
export GCC_AARCH64_PREFIX=/usr/bin/
```

#### Build edk2 prerequisites
```
cd edk2
export PACKAGES_PATH=$PWD/edk2-libc
source edksetup.sh
make -C BaseTools/Source/C
```

#### Start the xBSA application build
```
rm -rf Build/
source ShellPkg/Application/sysarch-acs/tools/scripts/acsbuild.sh xbsa_acpi
```

#### Build output
The xBSA EFI binary is generated at:
`workspace/edk2/Build/Shell/DEBUG_GCC/AARCH64/xbsa_acpi.efi`

> **Note:** The xBSA UEFI application currently supports ACPI-based builds. Platform-specific device tree enablement is not available for the xBSA target. Ensure Exerciser PAL APIs are implemented when the Exerciser VIP is present.

### Linux application
The xBSA UEFI application relies on the existing BSA and SBSA Linux applications when OS-based tests are required. Build these components using the instructions in `docs/bsa/README.md` and `docs/sbsa/README.md`, then deploy them alongside the xBSA UEFI binary.

## xBSA run steps

### For UEFI application

#### Silicon system
On platforms with USB access:
1. Copy `xbsa_acpi.efi` to a FAT-formatted USB device.

- **For U-Boot firmware systems, additional steps**
  1. Copy `Shell.efi` to the USB device (available under `prebuilt_images/IR`).
  2. Boot to the **U-Boot** shell.
  3. Discover the USB device:
     ```
     usb start
     ```
  4. Load `Shell.efi` into memory and start the UEFI shell:
     ```
     fatload usb <dev_num> ${kernel_addr_r} Shell.efi
     fatload usb 0 ${kernel_addr_r} Shell.efi
     ```
2. In the UEFI shell refresh the filesystem mappings:
   ```sh
   map -r
   ```
3. Switch to the USB filesystem (for example, `fs0:`).
4. Run `xbsa_acpi.efi` with the required parameters.
5. Capture the UART console output for log retention.

- For application parameters, see the [xBSA User Guide](user_guide.rst).

#### Emulation environment with secondary storage
1. Create an image containing `xbsa_acpi.efi` and `Shell.efi` (for U-Boot systems):
   ```
   mkfs.vfat -C -n HD0 hda.img 2097152
   sudo mount -o rw,loop=/dev/loop0,uid=$(whoami),gid=$(whoami) hda.img /mnt/acs
   sudo cp "<path to application>/xbsa_acpi.efi" /mnt/acs/
   sudo umount /mnt/acs
   ```
   *(Select an available loop device if `/dev/loop0` is busy.)*
2. Load the image into the emulated secondary storage using the platform-specific mechanism.
3. Boot to the UEFI shell.
4. Identify the filesystem with `map -r`.
5. Switch to the filesystem (`fs<x>:`).
6. Execute `xbsa_acpi.efi` with the appropriate parameters.
7. Preserve the UART console output for debug or certification review.

- For application parameters, see the [xBSA User Guide](user_guide.rst).

### For Linux application

The xBSA UEFI application reuses the BSA and SBSA Linux applications to exercise OS-reliant tests:
1. Transfer the built binaries (`bsa_acs.ko`, `bsa_app`, `sbsa_acs.ko`, `sbsa_app`) to the target system (for example, on a USB drive).
2. Boot into Linux and locate the removable storage device.
3. Load each kernel module before running the corresponding user-space application:
   ```sh
   sudo insmod bsa_acs.ko
   sudo insmod sbsa_acs.ko
   ```
4. Execute the user-space applications to run the Linux portions of the suite:
   ```sh
   ./bsa_app
   ./sbsa_app
   ```
5. Remove the kernel modules once testing is complete:
   ```sh
   sudo rmmod sbsa_acs
   sudo rmmod bsa_acs
   ```

- For application parameters, see the [xBSA User Guide](user_guide.rst).

## Related Documentation

- [BSA ACS README](../bsa/README.md) — UEFI, Linux, and bare-metal build/run guidance.  
- [SBSA ACS README](../sbsa/README.md) — UEFI, Linux, and bare-metal build/run guidance.  
- [PC-BSA ACS README](../pc_bsa/README.md) — UEFI, Linux, and bare-metal build/run guidance.  
- [PCIe Exerciser documentation](../pcie/Exerciser.md) — Reference implementation and PAL requirements.  

## Support

- Email: [support-systemready-acs@arm.com](mailto:support-systemready-acs@arm.com)  
- GitHub Issues: [sysarch-acs issue tracker](https://github.com/ARM-software/sysarch-acs/issues)  
- Contributions: [GitHub Pull Requests](https://github.com/ARM-software/sysarch-acs/pulls)
