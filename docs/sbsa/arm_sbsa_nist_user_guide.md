# National Institute of Standards and Technology: Statistical Test Suite

**Need for Randomness?**

Randomness relates to many areas of computer science, in particular with cryptography. Well-designed cryptographic primitives like hash functions, stream ciphers should produce pseudorandom data. The outputs of such generators may be used in many cryptographic applications like the generation of key material. Generators suitable for use in cryptographic applications need to meet stronger requirements than for other applications. In particular, their outputs must be unpredictable in the absence of knowledge of the inputs.

**Statistical Test Suites**

Randomness testing plays a fundamental role in cryptography. Randomness is tested using test suites consisting of many tests of randomness each focusing on different feature. These tests can be used as first steps in determining whether or not a generator is suitable for a particular cryptographic application.

**NIST with SBSA**

There are five well-known statistical test suites -- NIST STS, Diehard, TestU01, ENT and CryptX. Only the first three test suites are commonly used for the randomness analysis since CryptX is a commercial software and ENT provides only basic randomness testing. Since NIST STS has a special position by being published as an official document, it is often used in the preparation of formal certifications or approvals.

**Building NIST STS with SBSA ACS**

To build NIST statistical test suite with SBSA ACS, NIST STS 2.1.2 package is required. This package is obtained from <https://csrc.nist.gov/CSRC/media/Projects/Random-Bit-Generation/documents/sts-2_1_2.zip>  and is downloaded automatically as part of the build process.

This is an updated version of [NIST Statistical Test Suite (STS)](http://csrc.nist.gov/groups/ST/toolkit/rng/documentation_software.html) tool for randomness testing. The reason for the update is, the original source code provided with NIST does not compile cleanly in UEFI because it does not provide erf() and erfc() functions in the standard math library and (harcoded the inputs -- needs to be rephrased). Implementation of these functions has been added as part of SBSA val and a patch file is created.

**Tool Requirement**

Current release require the below tool:

1. unzip

### 1. Building from source
    Before you start the ACS build, ensure that the following requirements are met.

- Any mainstream Linux-based OS distribution running on a x86 or AArch64 machine.
- git clone the [EDK2 tree](https://github.com/tianocore/edk2). Recommended edk2 commit is edk2-stable202511
- git clone the [EDK2 port of libc](https://github.com/tianocore/edk2-libc) to local <edk2_path>.
- Install GCC-ARM 14.3 [toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads).
- Install the build prerequisite packages to build EDK2.<br />
Note:<br />
- The details of the packages are beyond the scope of this document.

#### 1.1 Target Platform
##### To start the ACS build for platform using ACPI table, perform the following steps:
1.  cd local_edk2_path
2.  git submodule update --init --recursive
3.  git clone https://github.com/ARM-software/sysarch-acs.git ShellPkg/Application/sysarch-acs

####    1.2 Build environment
##### If the build environment is Linux, perform the following steps:
1.  export GCC_AARCH64_PREFIX= GCC14.3 toolchain path pointing to /bin/aarch64-linux-gnu- in case of x86 machine.<br /> For an AArch64 build it should point to /usr/bin/
2.  export PACKAGES_PATH= path pointing to edk2-libc
3.  source edksetup.sh
4.  make -C BaseTools/Source/C
5.  source ShellPkg/Application/sysarch-acs/tools/scripts/acsbuild.sh nist

#### 1.3 Build output

The EFI executable file is generated at <edk2_path>/Build/Shell/DEBUG_GCC/AARCH64/SbsaNist.efi
`

**Directory structure of SBSA ACS**

The following figure shows the source code directory for SBSA ACS

```text
    sbsa
    ├── docs
    ├── linux_app
    ├── patches
    │   ├── nist_sbsa_sts.diff   ────────> Patch to compile NIST STS with SBSA ACS
    │   └── edk2_sbsa_nist.patch ────────> EDK2 SBSA NIST compliance patch
    │
    ├── platform
    │   ├── pal_baremetal
    │   ├── pal_uefi
    |
    ├── test_pool
    │   ├── exerciser
    │   ├── gic
    │   ├── io_virt
    │   ├── pcie
    │   ├── pe
    │   ├── peripherals
    │   ├── power_wakeup
    │   ├── mpam
    │   ├── ras
    │   ├── pmu
    │   ├── smmu
    │   ├── timer_wd
    │   └── nist_sts
    │       ├── test_n001.c     ────────>  NIST entry point to STS
    │       └── sts-2.1.2
    │           └── sts-2.1.2   ────────>  NIST STS package
    │
    ├── tools
    │   └── scripts
    ├── uefi_app
    └── val
        ├── include
        └── src
            └── avs_nist.c      ────────>  erf and erfc() implementations
```
**Running NIST STS**

Run the UEFI Shell application with the "-nist" as an argument argument

    uefi shell> sbsa.efi -nist

**Interpreting the results**

Final analysis report is generated when statistical testing is complete. The report contains a summary of empirical results which is displayed on the console. A test is unsuccessful when P-value < 0.01 and then the sequence under test should be considered as non-random. Example result as below

    ------------------------------------------------------------------------------
    RESULTS FOR THE UNIFORMITY OF P-VALUES AND THE PROPORTION OF PASSING SEQUENCES
    ------------------------------------------------------------------------------
    ------------------------------------------------------------------------------
     C1  C2  C3  C4  C5  C6  C7  C8  C9 C10  P-VALUE  PROPORTION  STATISTICAL TEST
    ------------------------------------------------------------------------------
      4   1   1   2   1   1   0   0   0   0  0.122325     10/10      OverlappingTemplate
     10   0   0   0   0   0   0   0   0   0  0.000000 *    0/10   *  Universal
      7   2   1   0   0   0   0   0   0   0  0.000001 *    8/10      ApproximateEntropy
      3   4   1   0   2   0   0   0   0   0  0.017912      9/10      Serial
      4   0   1   0   2   1   0   1   1   0  0.122325     10/10      Serial
      1   1   1   1   0   2   0   3   0   1  0.534146     10/10      LinearComplexity


    - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    The minimum pass rate for each statistical test with the exception of the
    random excursion (variant) test is approximately = 8 for a
    sample size = 10 binary sequences.

    The minimum pass rate for the random excursion (variant) test is undefined.

    For further guidelines construct a probability table using the MAPLE program
    provided in the addendum section of the documentation.
    - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


For more details on NIST STS, see: <https://doi.org/10.6028/NIST.SP.800-22r1a>

**Note**: For SBSA level 7 compliance, passing the NIST statistical test suite is required for S_L7ENT_1 rule.

--------------

*Copyright (c) 2020, 2023-2026, Arm Limited and Contributors. All rights reserved.*
