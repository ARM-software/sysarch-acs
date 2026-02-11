/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
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

#ifndef ACS_EL3_PARAM_H
#define ACS_EL3_PARAM_H

#include "rule_based_execution_enum.h"

/*
 * EL3 convention:
 *   X19 = ACS_EL3_PARAM_MAGIC
 *   X20 = address of bsa_el3_params in shared/shared-visible memory
 *
 * If X19 != ACS_EL3_PARAM_MAGIC, ACS ignores X20 and behaves as usual.
 */

/* Example magic: "BSAEL3P1" in ASCII */
#define ACS_EL3_PARAM_MAGIC     0x425341454C335031ULL  /* 'BSAEL3P1' */
#define ACS_EL3_PARAM_VERSION   0x3

/* Versioned parameter block from EL3 */
typedef struct {
  uint64_t version;

  /* Optional: rule selection override */
  uint64_t rule_array_addr;      /* RULE_ID_e[] of test IDs (can be 0) */
  uint64_t rule_array_count;     /* number of entries in rule_array_addr */

  /* Optional: module selection override */
  uint64_t module_array_addr;    /* uint32_t[] of module IDs (can be 0) */
  uint64_t module_array_count;   /* number of entries in module_array_addr */

  /* Optional: rules to skip */
  uint64_t skip_rule_array_addr;  /* RULE_ID_e[] of rule IDs to skip (can be 0) */
  uint64_t skip_rule_array_count; /* number of entries in skip_rule_array_addr */

  /* Optional: rules to skip */
  uint64_t skip_module_array_addr;  /*  uint32_t[] of module IDs (can be 0) */
  uint64_t skip_module_array_count; /* number of entries in skip_module_array_addr */

  uint64_t cache  :1;               /*Pass this flag to indicate that if the test system supports
                                    PCIe address translation cache\n*/
  uint64_t el1skiptrap_mask  :1;    /*Skips EL1 register checks*/
  uint64_t mmio   :1;               /*enable pal_mmio_read/write prints use with **verbose** */
  uint64_t no_crypto_ext :1;        /*cryptography extension not supported*/
  uint64_t software_view_filter :3; /*b0 : OS software view test b1 : Hypervisior view test
                                    b2:platform security view test (can be used in combination)*/
  uint64_t p2p :1;                  /* PCIe Hierarchy Supports Peer-to-Peer*/
  uint64_t skip_dp_nic_ms :1;       /*Skip PCIe tests for DisplayPort, Network,
                                    and Mass Storage devices*/
  uint64_t verbose :3;              /*Verbosity of the prints*/
  uint64_t timeout :32;             /*Set pass timeout (in us) for wakeup tests (500 us - 2 sec)*/
  uint64_t level_selection :3;      /*
                                    0: Level none
                                    1: run test till input level
                                    2: run test only for that level
                                    3:future requirement level*/
  uint64_t level :4;                /*Input level use along with level_selection*/
  uint64_t sys_cache :2;            /*Specify SLC cache type 0-unknown
                                    1-PPTT PE-side LLC
                                    2 - HMAT mem-side LLC */
  uint64_t reserved :11;
} acs_el3_params;

void acs_apply_el3_params(void);
bool acs_list_contains(const uint32_t *list, uint32_t count, uint32_t value);
bool acs_is_module_enabled(uint32_t module_base);
void acs_apply_compile_params(void);
#endif /* ACS_EL3_PARAM_H */

