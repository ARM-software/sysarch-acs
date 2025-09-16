/** @file
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
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
#ifndef __ACS_PFDI_H
#define __ACS_PFDI_H

#include "val_interface.h"

#define ACS_PFDI_TEST_NUM_BASE    0

#define PFDI_MAJOR_VERSION      1
#define PFDI_MINOR_VERSION      0

#define PFDI_FN_PFDI_BASE            0xC40002D0
#define PFDI_FN_PFDI(n)              (PFDI_FN_PFDI_BASE + (n))
#define PFDI_FN_PFDI_MAX_NUM         0x08

#define PFDI_FN_PFDI_VERSION            PFDI_FN_PFDI(0x00)
#define PFDI_FN_PFDI_FEATURES           PFDI_FN_PFDI(0x01)
#define PFDI_FN_PFDI_PE_TEST_ID         PFDI_FN_PFDI(0x02)
#define PFDI_FN_PFDI_PE_TEST_PART_COUNT PFDI_FN_PFDI(0x03)
#define PFDI_FN_PFDI_PE_TEST_RUN        PFDI_FN_PFDI(0x04)
#define PFDI_FN_PFDI_PE_TEST_RESULT     PFDI_FN_PFDI(0x05)
#define PFDI_FN_PFDI_FW_CHECK           PFDI_FN_PFDI(0x06)
#define PFDI_FN_PFDI_FORCE_ERROR        PFDI_FN_PFDI(0x07)
#define PFDI_FN_PFDI_RESERVED           PFDI_FN_PFDI(0x08)
#define PFDI_FN_PFDI_RANGE_END          PFDI_FN_PFDI(0x0F)

#define PFDI_VERSION_GET_MAJOR(version)     ((version >> 16) & 0x7fffU)
#define PFDI_VERSION_GET_MINOR(version)     ((version) & 0xffffU)

#define VAL_PFDI_RESERVED_BYTE_ZERO         0x0
#define PFDI_ACS_NOT_IMPLEMENTED            0x1

typedef enum {
    PFDI_ACS_SUCCESS                =  0,
    PFDI_ACS_NOT_SUPPORTED          = -1,
    PFDI_ACS_INVALID_PARAMETERS     = -3,
    PFDI_ACS_FAULT_FOUND            = -4,
    PFDI_ACS_ERROR                  = -5,
    PFDI_ACS_NOT_RAN                = -6,
    PFDI_ACS_UNKNOWN                = -7,
    PFDI_ACS_TEST_COUNT_ZERO        = -8
} PFDI_ACS_RET_CODE;

#endif /* __ACS_PFDI_H */
