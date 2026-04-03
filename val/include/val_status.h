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

#ifndef VAL_STATUS_H
#define VAL_STATUS_H

#if !defined(TARGET_UEFI) && !defined(TARGET_LINUX)
#include <stdint.h>
#endif

#include "pal_interface.h"

/* Structure to capture test state */
typedef struct {
    uint32_t index;
    uint8_t  state;
    uint16_t status_code;
} val_test_status_t;

/* =========================================================
 * TEST STATES
 * ========================================================= */
#define TEST_START             0x1u  //Test execution has started
#define TEST_END               0x2u  //Test execution is completed
#define TEST_PASS              0x4u  //Test is passed
#define TEST_PARTIAL_COVERED   0x5u  //Test is Partially covered
#define TEST_WARNING           0x6u  //Test executed with warnings
#define TEST_SKIP              0x7u  //Test is Skipped
#define TEST_FAIL              0x8u  //Test reported Failure
#define TEST_NOT_IMPLEMENTED   0x9u  //Test is not implemented
#define TEST_PAL_NOT_SUPPORTED 0xAu  //Test has no PAL support
#define TEST_SUPPORTED         0x0u  //Test is supported for a particular platform/rules
#define TEST_STATE_UNKNOWN     0xFu //Test state not applicable/unknown

/* =========================================================
 * STATUS / RESULT CODES
 * ========================================================= */
#define STATUS_SUCCESS    0x0u //ACS reported success
#define STATUS_ERROR      0x1u //ACS reported error
#define STATUS_SKIP       0x2u //ACS reported Skipped
#define ERROR_POINT(n)    ((uint16_t)(n)) //Checkpoint for FAIL/WARN/SKIP
#define STATUS_UNKNOWN    0xFFFFu // status code not applicable/unknown

/* =========================================================
 * PACKING FORMAT (32-bit)
 * [31:28] state (4 bits) | [15:0] status/code (16 bits)
 * ========================================================= */
#define VAL_TEST_STATE_SHIFT   28u
#define VAL_RESULT_CODE_SHIFT  0u
#define VAL_TEST_STATE_MASK    0xFu
#define VAL_RESULT_CODE_MASK   0xFFFFu

#define GENERATE_TEST_RESULT(state, code) \
    ((uint32_t)((((uint32_t)(state) & VAL_TEST_STATE_MASK) << VAL_TEST_STATE_SHIFT) | \
                ((uint32_t)(code) & VAL_RESULT_CODE_MASK)))

#define GET_CODE(RESULT) \
    ((uint16_t)(((uint32_t)(RESULT) >> VAL_RESULT_CODE_SHIFT) & VAL_RESULT_CODE_MASK))

#define GET_STATE(RESULT) \
    ((uint8_t)((((uint32_t)(RESULT) >> VAL_TEST_STATE_SHIFT) & VAL_TEST_STATE_MASK)))

/* =========================================================
 * RESULT_* MACROS (combine state + status)
 * ========================================================= */
#define RESULT_PASS                GENERATE_TEST_RESULT(TEST_PASS,              STATUS_SUCCESS)
#define RESULT_PARTIAL_COVERED     GENERATE_TEST_RESULT(TEST_PARTIAL_COVERED,   STATUS_SUCCESS)
#define RESULT_WARNING(n)          GENERATE_TEST_RESULT(TEST_WARNING,           ERROR_POINT(n))
#define RESULT_FAIL(n)             GENERATE_TEST_RESULT(TEST_FAIL,              ERROR_POINT(n))
#define RESULT_SKIP(n)             GENERATE_TEST_RESULT(TEST_SKIP,              ERROR_POINT(n))
#define RESULT_NOT_IMPLEMENTED     GENERATE_TEST_RESULT(TEST_NOT_IMPLEMENTED,   STATUS_UNKNOWN)
#define RESULT_PAL_NOT_SUPPORTED   GENERATE_TEST_RESULT(TEST_PAL_NOT_SUPPORTED, STATUS_UNKNOWN)
#define RESULT_UNKNOWN             GENERATE_TEST_RESULT(TEST_STATE_UNKNOWN,     STATUS_UNKNOWN)

/* =========================================================
 * IS_TEST_* MACROS (check STATE from packed status)
 * ========================================================= */
#define IS_TEST_PASS(p)               (GET_STATE(p) == TEST_PASS)
#define IS_TEST_PARTIAL_COVERED(p)    (GET_STATE(p) == TEST_PARTIAL_COVERED)
#define IS_TEST_WARNING(p)            (GET_STATE(p) == TEST_WARNING)
#define IS_TEST_SKIP(p)               (GET_STATE(p) == TEST_SKIP)
#define IS_TEST_FAIL(p)               (GET_STATE(p) == TEST_FAIL)
#define IS_TEST_NOT_IMPLEMENTED(p)    (GET_STATE(p) == TEST_NOT_IMPLEMENTED)
#define IS_TEST_PAL_NOT_SUPPORTED(p)  (GET_STATE(p) == TEST_PAL_NOT_SUPPORTED)

/* =========================================================
 * APIs
 * ========================================================= */
void val_data_cache_ops_by_va(addr_t addr, uint32_t type);
void     val_set_status(uint32_t index, uint32_t status);
uint32_t val_get_status(uint32_t index);
void     test_report_status(uint32_t status);

#endif /* VAL_STATUS_H */

