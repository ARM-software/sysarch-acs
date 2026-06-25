/** @file
 * Copyright (c) 2024-2026, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
**/
#ifndef __VAL_SPECIFICATION_H
#define __VAL_SPECIFICATION_H

#include "val_interface.h"

#define ACS_DRTM_INTERFACE_TEST_NUM_BASE    0
#define ACS_DRTM_DL_TEST_NUM_BASE           100

/* ACPI Defines */
#define ACPI_HEADER_LEN_OFFSET  4
#define ACPI_HEADER_SIZE        36
#define ACS_ACPI_SIGNATURE(A, B, C, D)  ((D << 24) | (C << 16) | (B << 8) | (A))
#define PRINT_ACPI_NAME_FROM_SIGNATURE(sig)                    \
  do {                                     \
    val_print(DEBUG, "\n         Table : %c", (char)((sig) & 0xFF));          \
    val_print(DEBUG, "%c", (char)(((sig) >> 8) & 0xFF));   \
    val_print(DEBUG, "%c", (char)(((sig) >> 16) & 0xFF));  \
    val_print(DEBUG, "%c", (char)(((sig) >> 24) & 0xFF)); \
  } while (0)

#define DRTM_1_0_FN_BASE            0xC4000110
#define DRTM_1_0_FN(n)              (DRTM_1_0_FN_BASE + (n))

#define DRTM_1_0_FN_DRTM_VERSION            DRTM_1_0_FN(0x00)
#define DRTM_1_0_FN_DRTM_FEATURES           DRTM_1_0_FN(0x01)
#define DRTM_1_0_FN_DRTM_UNPROTECT_MEMORY   DRTM_1_0_FN(0x03)
#define DRTM_1_0_FN_DRTM_DYNAMIC_LAUNCH     DRTM_1_0_FN(0x04)
#define DRTM_1_0_FN_DRTM_CLOSE_LOCALITY     DRTM_1_0_FN(0x05)
#define DRTM_1_0_FN_DRTM_GET_ERROR          DRTM_1_0_FN(0x06)
#define DRTM_1_0_FN_DRTM_SET_ERROR          DRTM_1_0_FN(0x07)
#define DRTM_1_0_FN_DRTM_SET_TCB_HASH       DRTM_1_0_FN(0x08)
#define DRTM_1_0_FN_DRTM_LOCK_TCB_HASH      DRTM_1_0_FN(0x09)

#define DRTM_1_0_FEAT_ID_BIT                ((uint64_t)0x1 << 63)
#define DRTM_1_0_FEAT_ID(n)                 (DRTM_1_0_FEAT_ID_BIT | (n))

#define DRTM_1_0_DRTM_FEATURES_TPM           DRTM_1_0_FEAT_ID(1u)
#define DRTM_1_0_DRTM_FEATURES_MEM_REQ       DRTM_1_0_FEAT_ID(2u)
#define DRTM_1_0_DRTM_FEATURES_DMA_PROT      DRTM_1_0_FEAT_ID(3u)
#define DRTM_1_0_DRTM_FEATURES_BOOT_PE_ID    DRTM_1_0_FEAT_ID(4u)
#define DRTM_1_0_DRTM_FEATURES_TCB_HASHES    DRTM_1_0_FEAT_ID(5u)
#define DRTM_1_0_DRTM_FEATURES_DLME_IMG_AUTH DRTM_1_0_FEAT_ID(6u)

#define DRTM_VERSION_GET_MAJOR(version)     ((version >> 16) & 0x7fffU)
#define DRTM_VERSION_GET_MINOR(version)     ((version) & 0x7fffU)
#define PSCI_VERSION_GET_MAJOR(version)     ((version >> 16) & 0x7fffU)
#define SMCCC_VERSION_GET_MAJOR(version)    ((version >> 16) & 0x7fffU)

#define VAL_DRTM_RESERVED_BYTE_ZERO         0x0
#define VAL_DRTM_PARAMETERS_REVISION        2

#define DRTM_SIZE_4K                        0x1000
#define ROUND_UP_TO_4K(Size)                (((Size) + DRTM_SIZE_4K - 1) & ~(DRTM_SIZE_4K - 1))
#define DRTM_IS_4KB_ALIGNED(addr)           (((addr) & 0xFFF) == 0)

#define DRTM_LOC_1                          1
#define DRTM_LOC_2                          2
#define DRTM_LOC_3                          3

#define DRTM_DRTM_FEATURES_DLME_IMG_AUTH          0x0
#define DRTM_DRTM_FEATURES_DMA_PROTECTION         0x1
#define DRTM_DRTM_FEATURES_PCR_SCHEMA             0x2
#define DRTM_DRTM_FEATURES_TPM_BASED_HASHING      0x3
#define DRTM_DRTM_FEATURES_FW_HASH_ALGOROTHM      0x4
#define DRTM_LAUNCH_FEAT_SECURE_INT_DISABLE       (1 << 7)
#define DRTM_DMA_FEATURES_DMA_PROTECTION_ALL      0x1
#define DRTM_DMA_FEATURES_DMA_PROTECTION_REGION   0x2
#define DRTM_DLME_IMG_FEAT_DLME_IMG_AUTH_SUPP     0x1
#define DRTM_LAUNCH_FEAT_MEM_PROT_ALL_SUPP        0x0
#define DRTM_LAUNCH_FEAT_MEM_PROT_REGION_SUPP     0x1
#define DRTM_LAUNCH_FEAT_REQ_DLME_IMG_AUTH        0x1
#define DRTM_TPM_FEAT_PCR_SCHEMA_DEF_SUPP         0x1
#define DRTM_TPM_FEAT_PCR_SCHEMA_DLME_AUTH_SUPP   0x10
#define DRTM_LAUNCH_FEAT_PCR_SCHEMA_DEF_SUPP      0x0
#define DRTM_LAUNCH_FEAT_DLME_AUTH_SUPP           0x1
#define DRTM_TPM_BASED_HASHING_SUPPORT            0x1

#define DRTM_LAUNCH_FEATURES_SHIFT_PCR_SCHEMA     0x1
#define DRTM_LAUNCH_FEATURES_MASK_PCR_SCHEMA      (0x3 << DRTM_LAUNCH_FEATURES_SHIFT_PCR_SCHEMA)

#define DRTM_LAUNCH_FEATURES_SHIFT_DLME_IMAGE_AUTH 6
#define DRTM_LAUNCH_FEATURES_MASK_DLME_IMAGE_AUTH \
                         (0x1 << DRTM_LAUNCH_FEATURES_SHIFT_DLME_IMAGE_AUTH)

#define DRTM_LAUNCH_FEATURES_SHIFT_MEM_PROTECTION 3
#define DRTM_LAUNCH_FEATURES_MASK_MEM_PROTECTION \
                         (0x7 << DRTM_LAUNCH_FEATURES_SHIFT_MEM_PROTECTION)

#define DRTM_LAUNCH_FEATURES_SHIFT_TYPE_HASH      0
#define DRTM_LAUNCH_FEATURES_MASK_TYPE_HASH       (0x1 << DRTM_LAUNCH_FEATURES_SHIFT_TYPE_HASH)
#define DRTM_PCR_SCHEMA_USAGE_MASK_DEF_SCHEMA     0x1
#define DRTM_PCR_SCHEMA_USAGE_MASK_AUTH_SCHEMA    0x10

#define DRTM_GET_FEATURES_MASK_DLME_IMAGE_AUTH    0x1
#define DRTM_GET_FEATURES_MASK_DMA_PROTECTION     0x7
#define DRTM_GET_FEATURES_MASK_PCR_SCHEMA         0xF
#define DRTM_GET_FEATURES_MASK_TPM_BASED_HASHING  0x1
#define DRTM_GET_FEATURES_MASK_FW_HASH_ALG        0XFFFF

#define DRTM_GET_FEATURES_SHIFT_PCR_SCHEMA        33
#define DRTM_GET_FEATURES_SHIFT_TPM_BASED_HASHING 32
#define DRTM_LAUNCH_FEAT_SHIFT_DLME_IMG_AUTH      6

#define DRTM_NS_EXECPTION_SHIFT_AIF               6
#define DRTM_NS_EXECPTION_MASK_AIF                0x7
#define DRTM_NON_SECURE_EXCP_MASKED               0x7

#define DRTM_CACHEABILITY_NOT_CACHEABLE           0x0UL
#define DRTM_CACHEABILITY_WRITE_COMBINE           0x1UL
#define DRTM_CACHEABILITY_WRITE_THROUGH           0x2UL
#define DRTM_CACHEABILITY_WRITE_BACK              0x3UL

#define DRTM_REGION_TYPE_NORMAL                   0x0UL
#define DRTM_REGION_TYPE_NORMAL_CACHEABLE         0x1UL
#define DRTM_REGION_TYPE_DEVICE                   0x2UL
#define DRTM_REGION_TYPE_NON_VOLATILE             0x3UL
#define DRTM_REGION_TYPE_RESERVED                 0x4UL

#define DRTM_MEM_PROT_SHIFT_CACHEABILITY_ATTR     55
#define DRTM_MEM_PROT_SHIFT_REGION_TYPE           52
#define DRTM_MEM_PROT_SHIFT_NUMBER_4KB_PAGES      0
#define DRTM_MEM_PROT_CACHEABILITY_MASK           0x3ULL
#define DRTM_MEM_PROT_REGION_TYPE_MASK            0x7ULL
#define DRTM_MEM_PROT_MAX_4KB_PAGES \
    ((1ULL << DRTM_MEM_PROT_SHIFT_REGION_TYPE) - 1ULL)

#define DRTM_MEM_PROT_ENCODE(cacheability, region_type, page_count) \
    ((((uint64_t)(cacheability) & DRTM_MEM_PROT_CACHEABILITY_MASK) << \
        DRTM_MEM_PROT_SHIFT_CACHEABILITY_ATTR) | \
     (((uint64_t)(region_type) & DRTM_MEM_PROT_REGION_TYPE_MASK) << \
        DRTM_MEM_PROT_SHIFT_REGION_TYPE) | \
     ((uint64_t)(page_count) & DRTM_MEM_PROT_MAX_4KB_PAGES))

#define DRTM_MEM_PROT_FULL_RANGE_SIZE_TYPE \
    DRTM_MEM_PROT_ENCODE(DRTM_CACHEABILITY_NOT_CACHEABLE, \
                         DRTM_REGION_TYPE_NON_VOLATILE, \
                         DRTM_MEM_PROT_MAX_4KB_PAGES)

#define DRTM_EVTYPE_ARM_PCR_SCHEMA                0x9001
#define DRTM_EVTYPE_ARM_DCE                       0x9002
#define DRTM_EVTYPE_ARM_DCE_PUBKEY                0x9003
#define DRTM_EVTYPE_ARM_DLME                      0x9004
#define DRTM_EVTYPE_ARM_DLME_ENTRY_POINT          0x9005
#define DRTM_EVTYPE_ARM_DEBUG_CONFIG              0x9006
#define DRTM_EVTYPE_ARM_NONSECURE_CONFIG          0x9007
#define DRTM_EVTYPE_ARM_DCE_SECONDARY             0x9008
#define DRTM_EVTYPE_ARM_TZFW                      0x9009
#define DRTM_EVTYPE_ARM_SEPARATOR                 0x900A
#define DRTM_EVTYPE_ARM_DLME_PUBKEY               0x900B
#define DRTM_EVTYPE_ARM_DLME_SVN                  0x900C
#define DRTM_EVTYPE_ARM_SECURE_INT_DISABLE        0x900E

#define DRTM_TPM_ALG_SHA256                       0xB
#define DRTM_TPM_ALG_SHA384                       0xC
#define DRTM_TPM_ALG_SHA512                       0xD

#define REQUEST_TPM_BASED_HASHING 1

/* Event Log Defines*/
#define EVENT_SPEC_ID_STR_LEN  15
#define EV_NO_ACTION            3

#define TPM_SHA1_160_HASH_LEN  20
#define SHA256_DIGEST_SIZE     32

#define DRTM_UINT32_MAX 0xFFFFFFFFu
#define DRTM_UINT64_MAX 0xFFFFFFFFFFFFFFFFull

/*
 * The g_drtm_features structure is a global structure that
 * stores details about the DRTM implementation. This structure
 * is initialized during val_initialize_env, and serves to inform
 * testing. For instance, if TPM-based hashing is not supported,
 * tests for TPM-based hashing will be skipped.
 */
typedef struct {
    int64_t  status;
    uint64_t value;
} DRTM_ACS_FEAT_VALUE64;

typedef struct {
    int64_t  status;
    uint32_t value;
} DRTM_ACS_FEAT_VALUE32;

typedef struct {
    /* DRTM supported features */
    DRTM_ACS_FEAT_VALUE32 version;
    DRTM_ACS_FEAT_VALUE64 tpm_features;
    DRTM_ACS_FEAT_VALUE64 min_memory_req;
    DRTM_ACS_FEAT_VALUE64 dma_prot_features;
    DRTM_ACS_FEAT_VALUE64 boot_pe_aff;
    DRTM_ACS_FEAT_VALUE64 tcb_hash_features;
    DRTM_ACS_FEAT_VALUE64 dlme_image_authentication;
    /* DRTM supported functions */
    int64_t               dynamic_launch;
    int64_t               unprotect_memory;
    int64_t               close_locality;
    int64_t               get_error;
    int64_t               set_error;
    int64_t               set_tcb_hash;
    int64_t               lock_tcb_hashes;
} DRTM_ACS_FEATURES;

extern DRTM_ACS_FEATURES g_drtm_features;

typedef enum {
    DRTM_ACS_SUCCESS                =  0,
    DRTM_ACS_NOT_SUPPORTED          = -1,
    DRTM_ACS_INVALID_PARAMETERS     = -2,
    DRTM_ACS_DENIED                 = -3,
    DRTM_ACS_NOT_FOUND              = -4,
    DRTM_ACS_INTERNAL_ERROR         = -5,
    DRTM_ACS_MEM_PROTECT_INVALID    = -6,
    DRTM_ACS_COPROCESSOR_ERROR      = -7,
    DRTM_ACS_OUT_OF_RESOURCE        = -8,
    DRTM_ACS_INVALID_DATA           = -9,
    DRTM_ACS_SECONDARY_PE_NOT_OFF   = -10,
    DRTM_ACS_ALREADY_CLOSED         = -11
} DTRM_ACS_RET_CODE;

/* 3.13 MEMORY_REGION_DESCRIPTOR_TABLE */
typedef struct {
    uint16_t    revision;
    uint16_t    reserved;
    uint32_t    num_regions;
} DRTM_MEMORY_REGION_HDR;

typedef struct {
    uint64_t start_addr;
    uint64_t size_type;
} DRTM_MEMORY_REGION;

typedef struct {
    DRTM_MEMORY_REGION_HDR  header;
    DRTM_MEMORY_REGION      regions[];
} DRTM_MEMORY_REGION_DESCRIPTOR_TABLE;

/* 3.14 DLME Data Header */
typedef struct {
    uint16_t revision;
    uint16_t size;
    uint32_t reserved;
    uint64_t dlme_data_size;
    uint64_t protected_regions_size;
    uint64_t address_map_size;
    uint64_t drtm_event_log_size;
    uint64_t tcb_hash_table_size;
    uint64_t acpi_table_region_size;
    uint64_t implementation_region_size;
} DRTM_DLME_DATA_HDR;

typedef struct {
    uint16_t revision;
    uint16_t reserved;
    uint32_t launch_features;
    uint64_t dlme_region_address;
    uint64_t dlme_region_size;
    uint64_t dlme_image_start;
    uint64_t dlme_entry_point_offset;
    uint64_t dlme_image_size;
    uint64_t dlme_data_offset;
    uint64_t nw_dce_region_address;
    uint64_t nw_dce_region_size;
    uint64_t mem_prot_table_address;
    uint64_t mem_prot_table_size;
} DRTM_PARAMETERS;

typedef struct {
    uint16_t revision;
    uint16_t num_hashes;
    uint16_t hash_algo;
    uint16_t reserved;
} DRTM_TCB_HASH_TABLE_HDR;

typedef struct {
    uint32_t hash_id;
    uint8_t  hash_val[];
} DRTM_TCB_HASHES;

typedef struct {
    DRTM_TCB_HASH_TABLE_HDR header;
    DRTM_TCB_HASHES         hashes[];
} DRTM_TCB_HASH_TABLE;

/* Event Log Structures */
typedef struct {
    uint16_t hashalg;
    uint8_t  digest[SHA256_DIGEST_SIZE];
} TPMT_HA;

typedef struct {
    uint32_t count;       /* number of digests */
    uint8_t  digests[];    /* Count digests */
} TPML_DIGEST_VALUES;

typedef struct {
    uint32_t event_size;   /* Size of the event data */
    uint8_t  event[];      /* The event data */
} EVENT_DATA;

/* This Declaration is for parsing the eventlog header which
 * is defined to be 20 bytes in TCG EFI Protocol Spec
 */
typedef struct {
    uint8_t digest[TPM_SHA1_160_HASH_LEN];
} TCG_DIGEST;

typedef struct {
    uint32_t pcr_index;       /* PCRIndex event */
    uint32_t event_type;     /* Type of event  */
    TPML_DIGEST_VALUES digests;  /* List of digests */
} TCG_PCR_EVENT2;

typedef struct {
    uint32_t   pcr_index; /* PCRIndex event */
    uint32_t   event_type; /* Type of event */
    TCG_DIGEST digest;  /* Value extended into PCRIndex */
    uint32_t   event_size; /* Size of the event data */
} TCG_PCR_EVENT;

typedef struct {
    uint8_t vendor_info_size;
    uint8_t vendor_info[];
} VENDOR_INFO;

typedef struct {
    uint16_t algorithm_id;
    uint16_t digest_size;
} TCG_EFI_SPECID_EVENT_ALGO_SIZE;

typedef struct {
    uint8_t  signature[16];
    uint32_t platform_class;
    uint8_t  spec_version_minor;
    uint8_t  spec_version_major;
    uint8_t  spec_errata;
    uint8_t  uintn_size;
    uint32_t number_of_algorithms;
    uint8_t  digest_sizes[];
} TCG_EFI_SPECID_EVENT;

typedef struct {
    uint8_t              *event_log_start;
    uint8_t              *event_log_end;
    uint8_t              *next_event;
    uint32_t              remaining_size;
    TCG_EFI_SPECID_EVENT *event_spec;
} DRTM_EVENT_LOG_STATE;

typedef struct {
    TCG_PCR_EVENT2 *event;
    uint8_t        *digest_data;
    EVENT_DATA     *event_data;
} DRTM_EVENT_LOG_ENTRY;

#define ARM_GICR_CTLR           0x0000  /* Redistributor Control Register      */
#define ARM_GICR_PENDBASER      0x0078  /* Redistributor LPI Pending Table Base Addr Register */
#define ARM_GITS_CTLR           0x0000  /* ITS Control Register */

int64_t val_drtm_features(uint64_t fid, uint64_t *feat1, uint64_t *feat2);
uint32_t val_drtm_get_version(void);
int64_t val_drtm_simulate_dl(DRTM_PARAMETERS *drtm_params);
int64_t val_drtm_dynamic_launch(DRTM_PARAMETERS *drtm_params);
int64_t val_drtm_close_locality(uint32_t locality);
int64_t val_drtm_unprotect_memory(void);
int64_t val_drtm_get_error(uint64_t *feat1);
int64_t val_drtm_set_tcb_hash(uint64_t tcb_hash_table_addr);
int64_t val_drtm_lock_tcb_hashes(void);
uint32_t val_drtm_reserved_bits_check_is_zero(uint32_t reserved_bits);
uint32_t val_drtm_get_psci_ver(void);
uint32_t val_drtm_get_smccc_ver(void);
uint64_t val_drtm_read_daif(void);

uint64_t DrtmReadDaif(void);

uint32_t val_drtm_create_info_table(void);
int64_t val_drtm_check_dl_result(uint64_t dlme_base_addr, uint64_t dlme_data_offset);
int64_t val_drtm_init_drtm_params(DRTM_PARAMETERS *drtm_params);
uint64_t val_drtm_get_feature(uint64_t feature_type);
int val_drtm_is_range_valid(uint8_t *start, uint8_t *end, uint8_t *ptr, uint64_t len);
uint32_t val_drtm_get_digest_size(uint16_t hash_alg);
uint32_t val_drtm_are_dce_and_drtm_images_distinct(void);
int32_t val_drtm_event_log_init(DRTM_PARAMETERS *drtm_params, DRTM_EVENT_LOG_STATE *event_log);
int32_t val_drtm_event_log_next(DRTM_EVENT_LOG_STATE *event_log, DRTM_EVENT_LOG_ENTRY *entry);
int32_t val_drtm_event_log_get_digest(const DRTM_EVENT_LOG_STATE *event_log,
                                      const DRTM_EVENT_LOG_ENTRY *entry, uint16_t hash_alg,
                                      uint8_t **digest, uint16_t *digest_size);

uint32_t interface001_entry(uint32_t num_pe);
uint32_t interface002_entry(uint32_t num_pe);
uint32_t interface003_entry(uint32_t num_pe);
uint32_t interface004_entry(uint32_t num_pe);
uint32_t interface005_entry(uint32_t num_pe);
uint32_t interface006_entry(uint32_t num_pe);
uint32_t interface007_entry(uint32_t num_pe);
uint32_t interface008_entry(uint32_t num_pe);
uint32_t interface009_entry(uint32_t num_pe);
uint32_t interface010_entry(uint32_t num_pe);
uint32_t interface011_entry(uint32_t num_pe);
uint32_t interface012_entry(uint32_t num_pe);
uint32_t interface013_entry(uint32_t num_pe);
uint32_t interface014_entry(uint32_t num_pe);
uint32_t interface015_entry(uint32_t num_pe);

uint32_t dl001_entry(uint32_t num_pe);
uint32_t dl002_entry(uint32_t num_pe);
uint32_t dl003_entry(uint32_t num_pe);
uint32_t dl004_entry(uint32_t num_pe);
uint32_t dl005_entry(uint32_t num_pe);
uint32_t dl006_entry(uint32_t num_pe);
uint32_t dl007_entry(uint32_t num_pe);
uint32_t dl008_entry(uint32_t num_pe);
uint32_t dl009_entry(uint32_t num_pe);
uint32_t dl010_entry(uint32_t num_pe);
uint32_t dl011_entry(uint32_t num_pe);
uint32_t dl012_entry(uint32_t num_pe);
uint32_t dl013_entry(uint32_t num_pe);
uint32_t dl014_entry(uint32_t num_pe);
uint32_t dl015_entry(uint32_t num_pe);
uint32_t dl016_entry(uint32_t num_pe);
uint32_t dl017_entry(uint32_t num_pe);


#endif /* __VAL_SPECIFICATION_H */
