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
#define ACS_EL3_PARAM_VERSION   0x1

/* Versioned parameter block from EL3 */
typedef struct {
  uint64_t version;

  /* Optional: test selection override */
  uint64_t test_array_addr;      /* uint32_t[] of test IDs (can be 0) */
  uint64_t test_array_count;     /* number of entries in test_array_addr */

  /* Optional: module selection override */
  uint64_t module_array_addr;    /* uint32_t[] of module IDs (can be 0) */
  uint64_t module_array_count;   /* number of entries in module_array_addr */

  /* Reserved / future use */
  uint64_t reserved0;
  uint64_t reserved1;
} acs_el3_params;

void acs_apply_el3_params(void);
bool acs_list_contains(const uint32_t *list, uint32_t count, uint32_t value);
bool acs_is_module_enabled(uint32_t module_base);
void acs_apply_compile_params(void);

#endif /* ACS_EL3_PARAM_H */

