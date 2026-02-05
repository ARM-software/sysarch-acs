/** @file
 * Copyright (c) 2025, <Vendor> or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0
 *
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
 *
 * @brief Vendor test template
 *
 * INSTRUCTIONS:
 * 1. Copy this file to your vendor directory (e.g., vendor/nvidia/timer/vt001.c)
 * 2. Update the copyright header with your vendor name
 * 3. Update TEST_NUM, TEST_DESC, and TEST_RULE
 * 4. Implement your test logic in payload()
 * 5. Rename the entry function (e.g., vt001_entry)
 * 6. Add README documentation for your test
 **/

#include "val/include/acs_val.h"
#include "val/include/acs_pe.h"
#include "val/include/acs_common.h"

/*
 * Test Identification
 *
 * TEST_NUM:  Unique test number using vendor base offset
 *            Format: ACS_VENDOR_<VENDOR>_BASE + MODULE_OFFSET + test_number
 *
 * TEST_DESC: Brief description (shown in test output)
 *            Prefix with vendor name for clarity
 *
 * TEST_RULE: Rule identifier for traceability
 *            Format: <VENDOR>_<MODULE>_<number>
 */
#define TEST_NUM   (ACS_VENDOR_TEST_NUM_BASE + 1)  /* Update with your vendor base */
#define TEST_DESC  "VENDOR: Template test description"
#define TEST_RULE  "VENDOR_TEST_01"

/**
 * @brief Per-PE test payload (optional)
 *
 * If your test needs to run on each PE, implement this function
 * and call it via val_execute_on_pe() or val_run_test_payload().
 **/
static void
per_pe_payload(void)
{
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    /* Per-PE test logic here */

    val_set_status(index, RESULT_PASS(TEST_NUM, 1));
}

/**
 * @brief Main test payload
 * @param num_pe Number of PEs in the system
 *
 * This is the main entry point for your test logic.
 * The function runs on the primary PE.
 **/
static void
payload(uint32_t num_pe)
{
    uint32_t my_index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t status = 0;

    /*
     * PREREQUISITE CHECKS (optional)
     *
     * Skip the test if prerequisites are not met:
     *
     * if (some_condition_not_met) {
     *     val_print(ACS_PRINT_DEBUG, "\n       Skipping: reason\n", 0);
     *     val_set_status(my_index, RESULT_SKIP(TEST_NUM, 1));
     *     return;
     * }
     */

    val_print(ACS_PRINT_INFO, "\n       Running vendor test with %d PEs", num_pe);

    /*
     * MAIN TEST LOGIC
     *
     * Implement your test here. Use VAL/PAL APIs for:
     * - Hardware access
     * - Multi-PE execution
     * - Timer operations
     * - Memory operations
     * - etc.
     *
     * Example: Execute something on all PEs
     *
     * for (i = 0; i < num_pe; i++) {
     *     if (i != my_index) {
     *         val_execute_on_pe(i, per_pe_payload, 0);
     *     }
     * }
     * per_pe_payload();  // Also run on primary
     */

    /*
     * RESULT REPORTING
     *
     * Use appropriate result macros:
     * - RESULT_PASS(TEST_NUM, status) - Test passed
     * - RESULT_FAIL(TEST_NUM, status) - Test failed
     * - RESULT_SKIP(TEST_NUM, status) - Test skipped
     * - RESULT_WARN(TEST_NUM, status) - Test passed with warnings
     *
     * The 'status' field can be used for sub-test identification (1-255).
     */

    if (status == 0) {
        val_print(ACS_PRINT_INFO, "\n       Test PASSED", 0);
        val_set_status(my_index, RESULT_PASS(TEST_NUM, 1));
    } else {
        val_print(ACS_PRINT_ERR, "\n       Test FAILED: status=%d", status);
        val_set_status(my_index, RESULT_FAIL(TEST_NUM, status));
    }
}

/**
 * @brief Test entry point
 * @param num_pe Number of PEs to test
 * @return Test status (ACS_STATUS_PASS, ACS_STATUS_FAIL, ACS_STATUS_SKIP)
 *
 * NAMING CONVENTION:
 * - Timer tests: vt<NNN>_entry (e.g., vt001_entry)
 * - PCIe tests:  vp<NNN>_entry (e.g., vp001_entry)
 * - PE tests:    vpe<NNN>_entry (e.g., vpe001_entry)
 * - GIC tests:   vg<NNN>_entry (e.g., vg001_entry)
 **/
uint32_t
vtest_template_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    /* Log context for debugging */
    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);

    /* Initialize test (prints test header) */
    status = val_initialize_test(TEST_NUM, TEST_DESC, val_pe_get_num());

    /* Run payload if not skipped */
    if (status != ACS_STATUS_SKIP)
        payload(num_pe);

    /* Check results and generate final status */
    status = val_check_for_error(TEST_NUM, 1, TEST_RULE);

    /* Print final result */
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}

