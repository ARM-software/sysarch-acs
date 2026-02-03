[![SYSARCH-ACS Build Check](https://github.com/ARM-software/sysarch-acs/actions/workflows/sysarch_daily.yml/badge.svg)](https://github.com/ARM-software/sysarch-acs/actions/workflows/sysarch_daily.yml)


## Table of Contents

* [Introduction](#introduction)
* [Repository Structure](#-repository-structure)
* [Architecture Compliance Suites](#architecture-compliance-suites)
  * [BSA ACS](#bsa-architecture-compliance-suite)
  * [SBSA ACS](#sbsa-architecture-compliance-suite)
  * [SYS-MPAM ACS](#sys-mpam-architecture-compliance-suite)
  * [PC-BSA ACS](#pc-bsa-architecture-compliance-suite)
  * [DRTM ACS](#drtm-architecture-compliance-suite)
  * [PFDI ACS](#pfdi-architecture-compliance-suite)
  * [VBSA ACS](#vbsa-architecture-compliance-suite)
  * [MemTest ACS](#memory-model-consistency-tests)
* [xBSA UEFI application](#xbsa-uefi-application)
* [Security Implications](#-security-implications)
* [Feedback, contributions and support](#-feedback-contributions-and-support)
* [License](#-license)

## Introduction

The **Arm sysarch-acs** repository is a collection of **Architecture Compliance Suites (ACS)** that verify compliance with the following Arm System Architecture specifications:

* **[BSA – Base System Architecture](https://developer.arm.com/documentation/den0094/latest/)**
* **[SBSA – Server Base System Architecture](https://developer.arm.com/documentation/den0029/latest/)**
* **[PC-BSA – Personal Computing Base System Architecture](https://developer.arm.com/documentation/den0151/latest/)**
* **[MPAM (Memory system resource partitioning and monitoring) system component](https://developer.arm.com/documentation/ihi0099/latest/)**
* **[DRTM – Dynamic Root of Trust for Measurement](https://developer.arm.com/documentation/den0113/latest/)**
* **[PFDI – Platform Fault Detection Interface](https://documentation-service.arm.com/static/68593b17961937560be90fbe)**
* **[VBSA – Virtual Base System Architecture](https://developer.arm.com/documentation/den0150/latest/)**

Additionally, it also includes ACS for ...
* **[MemTest](#memory-model-consistency-tests) – Memory Model Consistency tests**

These suites enable silicon vendors, system integrators, and firmware developers to ensure platform compliance with Arm architectural standards across pre-silicon and silicon phases.

## 📁 Repository Structure

| Path/Folder            | Purpose                                                                 |
|------------------------|-------------------------------------------------------------------------|
| `apps`                 | Application code acting as ACS entry points for each environment        |
| `docs`                 | Guides and references to help partners onboard each ACS                 |
| `mem_test`             | Memory model consistency tests                                          |
| `pal`                  | Platform Abstraction Layer used for platform integration                |
| `patches`              | Patches for different ACS build with EDK2                               |
| `prebuilt_images`      | Location of prebuilt ACS **release** binaries for supported suites      |
| `test_pool`            | Module-organized ACS test sources                                       |
| `tools`                | Build scripts                                                           |
| `val`                  | Validation Abstraction Layer that supplies platform-independent APIs    |

## Architecture Compliance Suites

###  BSA Architecture Compliance Suite
Validates platform compliance with Arm BSA specification.

#### Latest Release

| Version | Code Quality | Prebuilt Binary | Spec Reference | Complementary Tests / Dependencies    |
|---------|--------------|-----------------|---------------------|-------------------|
| v1.2.0  | BET          | [Bsa.efi](prebuilt_images/BSA/v25.12_BSA_1.2.0) | [BSA 1.2](https://developer.arm.com/documentation/den0094/e/?lang=en) | * Exerciser VIP needed for complete coverage of PCIe rules |

####  Reference for Build, Execution, and More
Refer to the [BSA ACS README](docs/bsa/README.md) for detailed build steps, execution procedures, additional information, and known limitations.

---

###  SBSA Architecture Compliance Suite
Validates platform compliance with Arm SBSA specification.

####  Latest Release

| Version | Code Quality | Prebuilt Binary | Spec Reference | Complementary Tests / Dependencies    |
|---------|--------------|-----------------|---------------------|-------------------|
| v8.0.0  | BET          | [Sbsa.efi](prebuilt_images/SBSA/v25.12_SBSA_8.0.0) | [SBSA 8.0](https://developer.arm.com/documentation/den0029/j/?lang=en) | * BSA ACS needs to run for complete SBSA coverage <br> * Exerciser VIP needed for complete coverage of PCIe rules |

####  Reference for Build, Execution, and More
Refer to the [SBSA ACS README](docs/sbsa/README.md) for detailed build steps, execution procedures, additional information, and known limitations.

---

###  SYS-MPAM Architecture Compliance Suite
Validates platform compliance with Arm MPAM system component specification.

####  Latest Release

| Version | Code Quality | Prebuilt Binary | Spec Reference |
|---------|--------------|-----------------|---------------------|
| v0.5.0  | ALP          | [Mpam.efi](https://github.com/ARM-software/bsa-acs/tree/main/prebuilt_images/MPAM/v25.03_MPAM_0.5.0_ALP) | [MPAM system component 1.1](https://developer.arm.com/documentation/den0094/d/?lang=en) |

####  Reference for Build, Execution, and More
Refer to the [MPAM ACS README](docs/mpam/README.md) for detailed build steps, execution procedures, additional information, and known limitations.

---

###  PC-BSA Architecture Compliance Suite
Validates platform compliance with Arm PCBSA specification.

####  Latest Release

| Version | Code Quality | Prebuilt Binary | Spec Reference |
|---------|--------------|-----------------|---------------------|
| v0.8.0  | BET          | [PC_Bsa.efi](prebuilt_images/PCBSA/v25.07_PCBSA_0.8.0)| [PCBSA 1.0](https://developer.arm.com/documentation/den0151/latest/) |
| v1.0.0  | EAC          | [PC_Bsa.efi](prebuilt_images/PCBSA/v25.12_PCBSA_1.0.0)| [PCBSA 1.0](https://developer.arm.com/documentation/den0151/latest/) |

####  Reference for Build, Execution, and More
Refer to the [PC BSA ACS README](docs/pc_bsa/README.md) for detailed build steps, execution procedures, additional information, and known limitations.

---

###  DRTM Architecture Compliance Suite
Validates platform compliance with Arm DRTM specification.

####  Latest Release

| Version | Code Quality | Prebuilt Binary | Spec Reference |
|---------|--------------|-----------------|---------------------|
| v0.6.0  | ALP          | [Drtm.efi](https://github.com/ARM-software/bsa-acs/tree/main/prebuilt_images/DRTM/v25.03_DRTM_0.6_ALP2)| [DRTM 1.1](https://developer.arm.com/documentation/den0113/c/?lang=en) |

####  Reference for Build, Execution, and More
Refer to the [DRTM ACS README](docs/drtm/README.md) for detailed build steps, execution procedures, additional information, and known limitations.

---

###  Memory Model Consistency Tests
Evaluates the correctness and consistency of system memory model.

####  Latest Release

| Version | Code Quality | Prebuilt Binary | Spec Reference |
|---------|--------------|-----------------|---------------------|
| v1.0.0  | EAC          | [Memtest binary](https://github.com/ARM-software/bsa-acs/tree/main/prebuilt_images/MEM_TEST/v24.04_MEMTEST_REL1.0.0)| - |

####  Reference for Build, Execution, and More
Refer to the [MemTest ACS README](docs/mem_test/README.md) for detailed build steps, execution procedures, additional information, and known limitations.

---

###  PFDI Architecture Compliance Suite
Validates platform compliance with Arm PFDI specification.

####  Latest Release

| Version | Code Quality | Prebuilt Binary | Spec Reference |
|---------|--------------|-----------------|---------------------|
| v0.8.0  | BET          | [pfdi.efi](prebuilt_images/PFDI/v25.09_PFDI_0.8.0)| [PFDI 1.0 BET0 ](https://developer.arm.com/documentation/110468/1-0bet0/?lang=en) |

####  Reference for Build, Execution, and More
Refer to the [PFDI ACS README](docs/pfdi/README.md) for detailed build steps, execution procedures, additional information, and known limitations.

---

###  VBSA Architecture Compliance Suite
Validates platform compliance with Arm VBSA specification.

####  Latest Release

| Version | Code Quality | Prebuilt Binary | Spec Reference |
|---------|--------------|-----------------|---------------------|
| v0.7.0  | BET          | [Vbsa.efi](prebuilt_images/VBSA/v25.12_VBSA_0.7.0/)             | [VBSA 1.0 ](https://developer.arm.com/documentation/den0150/a/?lang=en) |

####  Reference for Build, Execution, and More
Refer to the [VBSA ACS README](docs/vbsa/README.md) for detailed build steps, execution procedures, additional information, and known limitations.


## xBSA UEFI application
Provides a unified entry point for running BSA, SBSA, and PC-BSA validation from a single UEFI application.

####  Reference for Build, Execution, and More
Refer to the [xBSA README](docs/xbsa/README.md) for build steps, execution procedures, additional information, and known limitations.


## 🔐 Security Implications

Running ACS requires elevated privileges. <br>
Only execute on **development** systems. Do **not** run on production systems without sanitization.


## 💬 Feedback, contributions and support

- Email: [support-systemready-acs@arm.com](mailto:support-systemready-acs@arm.com)
- GitHub Issues: [sysarch-acs issue tracker](https://github.com/ARM-software/sysarch-acs/issues)
- Contributions: [GitHub Pull Requests](https://github.com/ARM-software/sysarch-acs/pulls)


## 📄 License

Distributed under [Apache v2.0 License](https://www.apache.org/licenses/LICENSE-2.0).
© 2025-2026 Arm Limited and Contributors.

