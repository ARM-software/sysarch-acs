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

#include "include/val_interface.h"
#include "include/val_status.h"
#include "val_logger.h"

/* Map shared memory as an array of status records */
static inline volatile val_test_status_t *val_get_shared_address(void)
{
    return (volatile val_test_status_t *)val_get_status_region_base();
}

/**
 * @brief Stores encoded test result for a PE into shared memory.
 *
 * Extracts state and status code from test_res and updates
 * the entry corresponding to the given PE index.
 *
 * @param index     index of PE reporting the test result.
 * @param test_res  Encoded test result (state + status code).
 */
void val_set_status(uint32_t index, uint32_t test_res)
{
    volatile val_test_status_t *mem = val_get_shared_address();

    if (index >= val_pe_get_num()) {
        val_print(ERROR, "\n       val_set_status: invalid PE index %u",
                  (unsigned int)index);
        return;
    }
    mem[index].index = index;
    mem[index].state = (uint8_t)GET_STATE(test_res);
    mem[index].status_code = (uint16_t)GET_CODE(test_res);
    val_data_cache_ops_by_va((addr_t)&mem[index], CLEAN_AND_INVALIDATE);
}

/**
 * @brief Retrieves encoded test result for a PE.
 *
 * Reads state and status code for the given PE index
 * and generates the combined test result.
 *
 * @param index  PE (Processing Element) index.
 *
 * @return Encoded test result (state + status code).
 */
uint32_t val_get_status(uint32_t index)
{
    volatile val_test_status_t *mem = val_get_shared_address();

    if (index >= val_pe_get_num()) {
        val_print(ERROR, "\n       val_get_status: invalid PE index %u",
                  (unsigned int)index);
        return RESULT_UNKNOWN;
    }
    val_data_cache_ops_by_va((addr_t)&mem[index], INVALIDATE);
    return GENERATE_TEST_RESULT(mem[index].state, mem[index].status_code);
}

/**
 * @brief Prints test result.
 *
 * Decodes the test result and prints PASS/FAIL/SKIP/WARN
 * along with checkpoint information when applicable.
 *
 * @param test_res  Encoded test result (state + status code).
 */
void test_report_status(uint32_t test_res)
{
    uint8_t  state  = (uint8_t)GET_STATE(test_res);

    switch (state) {

    case TEST_SKIP:
        val_print(INFO, "SKIPPED");
        break;

    case TEST_FAIL:
        val_print(INFO, "FAILED");
        break;

    case TEST_WARNING:
        val_print(INFO, "WARNING");
        break;

    case TEST_PASS:
        val_print(INFO, "PASSED");
        break;

    case TEST_PARTIAL_COVERED:
        val_print(INFO, "PASSED(*PARTIAL)");
        break;

    case TEST_NOT_IMPLEMENTED:
        val_print(INFO, "NOT TESTED (TEST NOT IMPLEMENTED)");
        break;

    case TEST_PAL_NOT_SUPPORTED:
        val_print(INFO, "NOT TESTED (PAL NOT SUPPORTED)");
        break;

    default:
        val_print(INFO, "STATUS:0x%08x", (unsigned int)test_res);
        break;
    }
}
