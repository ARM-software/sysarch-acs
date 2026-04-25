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
/* These APIs and definitions can be used throughout ACS*/
#ifndef __ACS_INTERFACE_H__
#define __ACS_INTERFACE_H__

#include "acs_stdint.h"

/* Test status counters visible across ACS */
typedef struct {
    uint32_t total_rules_run;     /* Total rules/tests that reported a status */
    uint32_t passed;              /* Count of TEST_PASS */
    uint32_t partial_coverage;    /* Count of TEST_PARTIAL_COV */
    uint32_t warnings;            /* Count of TEST_WARN */
    uint32_t skipped;             /* Count of TEST_SKIP */
    uint32_t failed;              /* Count of TEST_FAIL */
    uint32_t not_implemented;     /* Count of TEST_NO_IMP */
    uint32_t pal_not_supported;   /* Count of TEST_PAL_NS */
} acs_test_status_counters_t;

acs_test_status_counters_t *acs_get_test_status(void);
void acs_reset_test_status(void);

#endif /* __ACS_INTERFACE_H__ */
