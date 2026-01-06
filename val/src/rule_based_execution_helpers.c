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

#include "include/rule_based_execution.h"
#include "include/val_interface.h"

extern rule_test_map_t rule_test_map[RULE_ID_SENTINEL];
extern char *rule_id_string[RULE_ID_SENTINEL];
extern char *module_name_string[MODULE_ID_SENTINEL];
extern uint32_t alias_rule_map_count;
extern alias_rule_map_t alias_rule_map[];

/**
 * @brief Check if a rule ID exists in a list.
 *
 * Linear scan of the first `count` entries of `list` to see if any equals
 * `rid`.
 *
 * @param rid   Rule ID to find.
 * @param list  Rule ID array to scan.
 * @param count Number of valid entries in `list`.
 * @return 1 if found; 0 otherwise.
 */
bool rule_in_list(RULE_ID_e rid, const RULE_ID_e *list, uint32_t count)
{
    if (!list || count == 0)
        return 0;
    for (uint32_t i = 0; i < count; i++) {
        if (list[i] == rid)
            return 1;
    }
    return 0;
}

/**
 * @brief In-place quicksort for rule ID arrays.
 *
 * Sorts an array of `RULE_ID_e` values in ascending order using an
 * iterative quicksort (no recursion). The implementation processes the
 * smaller partition first to keep the auxiliary stack depth bounded.
 *
 * @param rule_list Pointer to the array of `RULE_ID_e` to sort.
 * @param list_size Number of elements in `rule_list`.
 */
void quick_sort_rule_list(RULE_ID_e *rule_list, uint32_t list_size)
{
    if (!rule_list || list_size < 2) return;          /* quick exit */

    /* Small fixed size stack (32 pairs handle any 32bit length) */
    int lo_stack[32];
    int hi_stack[32];
    int top = 0;

    lo_stack[0] = 0;
    hi_stack[0] = (int)list_size - 1;

    while (top >= 0)
    {
        int lo = lo_stack[top];
        int hi = hi_stack[top];
        --top;

        while (lo < hi)
        {
            /* Partition step */
            RULE_ID_e pivot = rule_list[hi];
            int i = lo - 1;

            for (int j = lo; j < hi; ++j)
            {
                if (rule_list[j] <= pivot)
                {
                    ++i;
                    RULE_ID_e tmp = rule_list[i];
                    rule_list[i] = rule_list[j];
                    rule_list[j] = tmp;
                }
            }

            ++i;                                    /* pivot's final spot */
            RULE_ID_e tmp = rule_list[i];
            rule_list[i] = rule_list[hi];
            rule_list[hi] = tmp;

            /* Handle smaller side first to control stack depth */
            if (i - lo < hi - i)
            {
                /* Push right half */
                if (i + 1 < hi) {
                    ++top;
                    lo_stack[top] = i + 1;
                    hi_stack[top] = hi;
                }
                hi = i - 1;                          /* loop on left half */
            }
            else
            {
                /* Push left half */
                if (lo < i - 1) {
                    ++top;
                    lo_stack[top] = lo;
                    hi_stack[top] = i - 1;
                }
                lo = i + 1;                          /* loop on right half */
            }
        }
    }
}


/**
 * @brief Print an rule header line before running a test.
 *
 * Example output
 * "START PE B_PE_01 18 : Check PE symmetry"
 *
 * @param rule_enum Rule identifier (enum) whose ID string and description
 *                  will be printed.
 * @param indent    Indentation level; output is prefixed by (indent * 4)
 *                  spaces.
 */
void
print_rule_test_start(uint32_t rule_enum, uint32_t indent)
{
    val_print(ACS_PRINT_TEST, "\n\n", 0);
    /* Print indent spaces */
    while (indent) {
        val_print(ACS_PRINT_TEST, "    ", 0);
        indent--;
    }

    val_print(ACS_PRINT_TEST, "START ", 0);

    /* Print module name */
    if (rule_test_map[rule_enum].flag == INVALID_ENTRY) {
        val_print(ACS_PRINT_TEST, "-", 0);
    } else {
        val_print(ACS_PRINT_TEST, module_name_string[rule_test_map[rule_enum].module_id], 0);
    }
    val_print(ACS_PRINT_TEST, " ", 0);
    val_print(ACS_PRINT_TEST, rule_id_string[rule_enum], 0);

    /* TODO
       Note: Test ID print would be deprecated in future, please use rule id as primary key to
       identify tests and comment on coverage */
    val_print(ACS_PRINT_TEST, " ", 0);
    if (rule_test_map[rule_enum].test_num == INVALID_ENTRY) {
        val_print(ACS_PRINT_TEST, "-", 0);
    }
    else {
        val_print(ACS_PRINT_TEST, "%d", rule_test_map[rule_enum].test_num);
    }

    val_print(ACS_PRINT_TEST, " : ", 0);
    /* Print rule  description */
    if (rule_test_map[rule_enum].flag != INVALID_ENTRY) {
        val_print(ACS_PRINT_TEST, rule_test_map[rule_enum].rule_desc, 0);
    }
}

/**
 * @brief Print the status for a rule test result.
 *
 * Example output
 * "END B_PE_01 PASS"
 *
 * @param rule_enum Rule identifier enum
 * @param indent    Indentation level; output is prefixed by (indent * 4)
 *                  spaces.
 * @param status    Result code (e.g. `TEST_PASS`, `TEST_FAIL`).
 */
void
print_rule_test_status(uint32_t rule_enum, uint32_t indent, uint32_t status)
{
    /* Only count top-level rules (indent == 0) */
    uint32_t top_level_rule = (indent == 0);

    val_print(ACS_PRINT_TEST, "\n", 0);
    /* Print indent spaces */
    while (indent) {
        val_print(ACS_PRINT_TEST, "    ", 0);
        indent--;
    }

    val_print(ACS_PRINT_TEST, "END ", 0);
    val_print(ACS_PRINT_TEST, rule_id_string[rule_enum], 0);
    val_print(ACS_PRINT_TEST, " ", 0);

    /* Update global counters for top-level rules only */
    if (top_level_rule) {
        g_rule_test_stats.total_rules_run++;
    }

    switch (status) {
    case TEST_PASS:
        val_print(ACS_PRINT_TEST, "PASSED", 0);
        if (top_level_rule) g_rule_test_stats.passed++;
        break;
    case TEST_PART_COV:
        val_print(ACS_PRINT_TEST, "PASSED(*PARTIAL)", 0);
        if (top_level_rule) g_rule_test_stats.partial_coverage++;
        break;
    case TEST_WARN:
        val_print(ACS_PRINT_TEST, "WARNING", 0);
        if (top_level_rule) g_rule_test_stats.warnings++;
        break;
    case TEST_SKIP:
        val_print(ACS_PRINT_TEST, "SKIPPED", 0);
        if (top_level_rule) g_rule_test_stats.skipped++;
        break;
    case TEST_FAIL:
        val_print(ACS_PRINT_TEST, "FAILED", 0);
        if (top_level_rule) g_rule_test_stats.failed++;
        break;
    case TEST_NO_IMP:
        val_print(ACS_PRINT_TEST, "NOT TESTED (TEST NOT IMPLEMENTED)", 0);
        if (top_level_rule) g_rule_test_stats.not_implemented++;
        break;
    case TEST_PAL_NS:
        val_print(ACS_PRINT_TEST, "NOT TESTED (PAL NOT SUPPORTED)", 0);
        if (top_level_rule) g_rule_test_stats.pal_not_supported++;
        break;
    default:
        val_print(ACS_PRINT_TEST, "STATUS:0x%x", status);
    }
    return;
}

/**
 * @brief Reset all rule execution statuses to unknown.
 *
 * Initializes the global `rule_status_map` so that each entry corresponding
 * to a valid `RULE_ID_e` is set to `TEST_STATUS_UNKNOWN`. Call this before
 * starting a run to ensure clean status reporting.
 */
void rule_status_map_reset(void)
{
    uint32_t i;
    for (i = 0; i < RULE_ID_SENTINEL; i++) {
        rule_status_map[i] = TEST_STATUS_UNKNOWN;
    }
}

/**
 * @brief Get the index of an alias rule in alias_rule_map.
 *
 * Performs a linear scan over the compile-time `alias_rule_map` and returns
 * the zero-based index of the entry whose `alias_rule_id` matches the input.
 * If no entry matches, returns `INVALID_IDX`.
 *
 * @param alias_rule_id Alias rule identifier to look up.
 * @return uint32_t Zero-based index in `alias_rule_map`, or `INVALID_IDX` if not found.
 */
uint32_t
alias_rule_map_get_index(RULE_ID_e alias_rule_id)
{

    uint32_t i;

    /* Iterate over all entries in the alias map */
    for (i = 0; i <  alias_rule_map_count; i++) {
        /* Check if the current entry's alias matches the requested ID */
        if (alias_rule_map[i].alias_rule_id == alias_rule_id) {
            return i;  /* found: return its index */
        }
    }

    /* Not found: return sentinel indicating absence */
    return INVALID_IDX;
}
