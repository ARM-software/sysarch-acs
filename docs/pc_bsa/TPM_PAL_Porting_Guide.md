# TPM PAL Porting Guide

## Introduction

This document provides detailed steps to port the Trusted Platform Module (TPM) Platform Abstraction Layer (PAL) for PC-BSA, including TPM PAL data structures, APIs, configuration macros.

## TPM Configuration

This section provides information on the TPM interface and the information required for TPM configuration and validation on the platform.
The TPM configuration macros (`PLATFORM_TPM_*`) are defined in the platform override header:
`pal/baremetal/target/<platform>/include/platform_override_fvp.h`


```c
#define PLATFORM_TPM_PRESENT        0x1         /* Set to 1 when TPM exists */
#define PLATFORM_TPM_VERSION        0x2         /* 2 = TPM 2.0 */
#define PLATFORM_TPM_BASE_ADDR      0x40000000ULL
#define PLATFORM_TPM_INTERFACE_TYPE 0x7         /* StartMethod encoding */
```

- **PLATFORM_TPM_PRESENT**       : Indicates whether a TPM device is present in the system. Set to `1` when the TPM is enabled and `0` when no TPM is available.
- **PLATFORM_TPM_VERSION**       : Indicates the TPM family implemented on the platform. Use `2` for TPM 2.0 and `1` for legacy TPM 1.2 devices.
- **PLATFORM_TPM_BASE_ADDR**     : Physical base address of the TPM registers.
- **PLATFORM_TPM_INTERFACE_TYPE**: Specifies the TPM StartMethod encoding used by the platform.(Example : `6` for FIFO/TIS, `7` for CRB, etc).

#### Header file representation

```c
typedef struct {
    uint64_t tpm_present;
    uint64_t tpm_version;
    uint64_t tpm_base;
    uint64_t tpm_interface_type;
} PLATFORM_OVERRIDE_TPM2_INFO_TABLE;

typedef struct {
    uint64_t tpm_presence;
    uint64_t tpm_interface;
    uint64_t base;
} TPM2_INFO_TABLE;
```

## Porting Requirements

The following table lists the different types of APIs in TPM.

These TPM PAL APIs are implemented in the bare-metal PAL source located at:
`pal/baremetal/base/src/pal_tpm2.c`

### TPM APIs and their details

| API name | Function prototype  | Implementation |
| --- | --- | --- |
| `pal_tpm2_create_info_table` | `void pal_tpm2_create_info_table(TPM2_INFO_TABLE *TpmTable);` | Yes |
| `pal_tpm2_get_version` | `uint64_t pal_tpm2_get_version(void);` | Yes |

### Notes

There are two implementation types for the PAL APIs and are classified in the above table:

• **Yes**: indicates that the implementation of this API is already present. Since the values are platform-specific, it must be taken from the platform configuration file.

• **Platform-specific**: you must implement all the APIs that are marked as platform-specific.

*Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.*
