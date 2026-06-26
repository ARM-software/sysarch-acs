/** @file
 * Copyright (c) 2025-2026, Arm Limited or its affiliates. All rights reserved.
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

#include "rule_based_execution.h"
#include "acs_common.h"
#include "val_interface.h"
#include "acs_pe.h"
#include "acs_memory.h"

extern uint8_t g_current_pal;
extern rule_test_map_t rule_test_map[RULE_ID_SENTINEL];
extern const alias_rule_map_t alias_rule_map[];
extern test_entry_fn_t test_entry_func_table[TEST_ENTRY_SENTINEL];
extern char *rule_id_string[RULE_ID_SENTINEL];

/**
 * @brief Check PAL support for a rule and report if unsupported.
 *
 * Evaluates the rule's platform_bitmask against the current PAL
 *
 * @param rule_id Rule identifier to check.
 * @return TEST_SUPPORTED if the rule is supported on this PAL.
 *         else, TEST_NOT_IMPLEMENTED Not covered by ACS in any supported PAL.
 *               TEST_PAL_NOT_SUPPORTED Test not supported with current PAL.
 */
static uint32_t check_rule_support(RULE_ID_e rule_id)
{
    uint8_t plat_bitmask = rule_test_map[rule_id].platform_bitmask;

   // val_print(ERROR, "\n plat_bitmask %x", plat_bitmask);

    if (!(g_current_pal & plat_bitmask)) {
        /* Report if rule is not supported by ACS across all available PALs*/
        if (plat_bitmask == 0) {
            /* Not covered by ACS in any supported PAL */
            return RESULT_NOT_IMPLEMENTED;
        } else {
            /* Test not supported with current PAL */
            return RESULT_PAL_NOT_SUPPORTED;
        }
    }

    return TEST_SUPPORTED; /* supported on current PAL */
}

/**
 * @brief Determine if a rule should be skipped based on CLI options.
 *
 * Checks whether the provided rule ID is present in the explicit skip list
 * (-skip) or whether its module is present in the skip-modules list
 * (-skipmodule).
 *
 * @param rule_id Rule identifier to check.
 * @return true (1) if the rule should be skipped, false(0) otherwise.
 */
static bool is_rule_skipped(const acs_run_request_t *ctx, RULE_ID_e rule_id)
{
    uint32_t i;
    MODULE_NAME_e module;

    if (ctx == NULL)
        return 0;

    /* Check explicit rule skip list (-skip) */
    if (ctx->skip_rule_count > 0 && ctx->skip_rule_list != NULL) {
        for (i = 0; i < ctx->skip_rule_count; i++) {
            if (ctx->skip_rule_list[i] == rule_id)
                return 1;
        }
    }

    /* Check module skip list (-skipmodule) */
    if (ctx->num_skip_modules > 0 && ctx->skip_modules != NULL) {
        module = rule_test_map[rule_id].module_id;
        for (i = 0; i < ctx->num_skip_modules; i++) {
            if (ctx->skip_modules[i] == (uint32_t)module)
                return 1;
        }
    }

    return 0;
}

/**
 * @brief Finalize aggregated status for an alias rule.
 *
 * Applies the alias aggregation overrides used when child rules produce mixed
 * PASS/SKIP/WARN or PASS/not-supported outcomes.
 *
 * @param aggregated_status Current aggregated status across child rules.
 * @param test_ns_flag      True if any child rule was not supported.
 * @param test_pass_flag    True if any child rule passed.
 * @param test_warn_flag    True if any child rule warned.
 *
 * @return Final alias result after applying partial-coverage/warn overrides.
 */
static uint32_t finalize_alias_status(uint32_t aggregated_status,
                                      bool test_ns_flag,
                                      bool test_pass_flag,
                                      bool test_warn_flag)
{
    /* If an alias saw a mix of PASS and SKIP/WARN (or PASS plus unsupported
       children), surface it as partial coverage instead of just the max state. */
    if ((test_pass_flag &&
         ((GET_STATE(aggregated_status) == TEST_SKIP) ||
          (GET_STATE(aggregated_status) == TEST_WARNING))) ||
        (test_ns_flag &&
         (GET_STATE(aggregated_status) == TEST_PASS))) {
        aggregated_status = RESULT_PARTIAL_COVERED;
    }

    if (test_warn_flag && (GET_STATE(aggregated_status) == TEST_SKIP)) {
        aggregated_status = RESULT_WARNING(0);
    }

    return aggregated_status;
}

/**
 * @brief Print alias traversal banner at start/end of child execution.
 *
 * @param rule_id Alias rule identifier.
 * @param indent  Indentation to use for the banner.
 * @param start   True for start banner, false for end banner.
 */
static void print_alias_walk_banner(RULE_ID_e rule_id, uint32_t indent, bool start)
{
    uint32_t i;

    val_print(INFO, "\n\n");
    for (i = 0; i < indent; i++) {
        val_print(INFO, "    ");
    }

    val_print(INFO, "  === ");
    if (start) {
        val_print(INFO, "Start");
    } else {
        val_print(INFO, "End");
    }
    val_print(INFO, " tests for rules referenced by ");
    val_print(INFO, rule_id_string[rule_id]);
    val_print(INFO, " ===");
    if (!start) {
        val_print(INFO, "\n");
    }
}

/**
 * @brief Execute a rule, recursively resolving alias rules.
 *
 * The top-level caller is responsible for printing/reporting the top-level
 * rule. Recursive child invocations set @p report_self so child rules print
 * their own headers/status. All recursive descendants use a fixed indentation
 * level to keep logs tidy while still surfacing intermediate alias rules.
 *
 * @param ctx           Run request containing CLI selections.
 * @param rule_id       Rule to execute.
 * @param indent        Indentation for child rule logging.
 * @param num_pe        Number of PEs in the system.
 * @param report_self   True if this invocation should print its own start/end.
 *
 * @return Rule execution status.
 */
static uint32_t execute_rule_recursive(const acs_run_request_t *ctx,
                                       RULE_ID_e rule_id,
                                       uint32_t indent,
                                       uint32_t num_pe,
                                       bool report_self)
{
    bool test_ns_flag;
    bool test_pass_flag;
    bool test_warn_flag;
    bool pushed = 0;
    uint32_t j;
    uint32_t alias_rule_map_index;
    uint32_t rule_test_status = TEST_STATE_UNKNOWN;
    uint32_t child_rule_status;
    uint32_t precheck_status;
    uint32_t rule_support_status;
    RULE_ID_e child_rule_id;
    const RULE_ID_e *child_rule_list;

    /* Detect accidental alias cycles such as A -> B -> A before descending */
    if (rule_reference_path_contains(rule_id)) {
        val_print(ERROR, " Recursive alias reference detected for rule: ");
        val_print(ERROR, rule_id_string[rule_id]);
        return RESULT_FAIL(1);
    }

    if (!rule_reference_path_push(rule_id)) {
        val_print(ERROR, " Rule reference path depth exceeded for rule: ");
        val_print(ERROR, rule_id_string[rule_id]);
        return RESULT_FAIL(1);
    }
    pushed = 1;

    /* Print rule start if report_self is true */
    if (report_self) {
        rule_support_status = check_rule_support(rule_id);

#ifdef TARGET_LINUX
        /* Workaround for linux apps, to skip rule silently */
        if (rule_support_status != TEST_SUPPORTED) {
            rule_test_status = TEST_STATE_UNKNOWN;
            report_self = 0;
            goto exit_rule;
        }
#endif

        print_rule_test_start(rule_id, indent);

        if (rule_support_status != TEST_SUPPORTED) {
            rule_test_status = rule_support_status;
            goto exit_rule;
        }
    }

    if (rule_test_map[rule_id].flag == ALIAS_RULE) {
        alias_rule_map_index = alias_rule_map_get_index(rule_id);
        if (alias_rule_map_index == INVALID_IDX) {
            val_print(ERROR, " alias map index not found for rule id: 0x%x", rule_id);
            rule_test_status = RESULT_FAIL(1);
            goto exit_rule;
        }

        /* Execute any precheck required by the alias rule */
        if (rule_test_map[rule_id].test_entry_id != NULL_ENTRY) {
            precheck_status =
                test_entry_func_table[rule_test_map[rule_id].test_entry_id](num_pe);

            if (GET_STATE(precheck_status) == TEST_FAIL) {
                rule_test_status = RESULT_SKIP(0);
                goto exit_rule;
            }
        }

        test_ns_flag = 0;
        test_pass_flag = 0;
        test_warn_flag = 0;
        child_rule_list = alias_rule_map[alias_rule_map_index].child_rule_list;

        /* Print alias rule banner */
        print_alias_walk_banner(rule_id, indent, 1);

        for (j = 0; child_rule_list[j] != RULE_ID_SENTINEL; j++) {
            child_rule_id = child_rule_list[j];

            if (is_rule_skipped(ctx, child_rule_id)) {
                continue;
            }

            /* Always recurse with indent == 1 for descendants. This keeps logs
               readable while still showing child and intermediate alias nodes. */
            child_rule_status = execute_rule_recursive(ctx, child_rule_id, 1, num_pe, 1);

#ifdef TARGET_LINUX
            if (child_rule_status == TEST_STATE_UNKNOWN) {
                continue;
            }
#endif

            if ((GET_STATE(child_rule_status) == TEST_PAL_NOT_SUPPORTED) ||
                (GET_STATE(child_rule_status) == TEST_NOT_IMPLEMENTED)) {
                test_ns_flag = 1;
                /* Unsupported children are tracked for partial coverage, but
                   should not dominate the aggregated alias status. */
                continue;
            }
            if (GET_STATE(child_rule_status) == TEST_PASS) {
                test_pass_flag = 1;
            }
            if (GET_STATE(child_rule_status) == TEST_WARNING) {
                test_warn_flag = 1;
            }

            if ((child_rule_status > rule_test_status) ||
                (rule_test_status == TEST_STATE_UNKNOWN)) {
                rule_test_status = child_rule_status;
            }
        }

        rule_test_status = finalize_alias_status(rule_test_status, test_ns_flag,
                                                 test_pass_flag, test_warn_flag);

        print_alias_walk_banner(rule_id, indent, 0);
    } else if (rule_test_map[rule_id].flag == BASE_RULE) {
        if (test_entry_func_table[rule_test_map[rule_id].test_entry_id] != NULL) {
            rule_test_status =
                test_entry_func_table[rule_test_map[rule_id].test_entry_id](num_pe);
        } else {
            val_print(ERROR, "\n\n  Rule failed due to NULL entry \n\r ", 0);
            rule_test_status = RESULT_FAIL(1);
        }
    } else {
        rule_test_status = RESULT_FAIL(1);
    }

exit_rule:
    if (pushed) {
        rule_reference_path_pop();
    }

    if (report_self) {
        /* Child rules report themselves inside the recursive walk. Top-level
           rules are still reported exactly once by run_tests(). */
        rule_status_map[rule_id] = rule_test_status;
        print_rule_test_status(rule_id, indent, rule_test_status);
    }

    return rule_test_status;
}

/**
 * @brief Filter the provided rule list in place based on CLI selections.
 *
 * Applies the following filters to the rule list, compacting it in-place while
 * preserving the original relative order of rules kept:
 * - Rules listed in ctx->skip_rule_list are removed.
 * - Rules whose module matches any in ctx->skip_modules are removed.
 * - If ctx->execute_modules is provided and non-empty, only rules whose module
 *   is in that list are kept.
 *
 * No new memory is allocated. Elements beyond the returned count remain
 * unchanged but are considered out of range by callers.
 *
 * @return New count of rules after filtering.
 */
uint32_t filter_rule_list_by_cli(acs_run_request_t *ctx)
{
    uint32_t out;
    uint32_t i;
    uint32_t si;
    uint32_t mi;
    RULE_ID_e rule;
    bool skip;
    MODULE_NAME_e module;
    bool found;
    /* Pointers to lookup tables for selected arch (if any) */
    const bsa_rule_entry_t   *bsa_tbl   = NULL;
    const sbsa_rule_entry_t  *sbsa_tbl  = NULL;
    const pcbsa_rule_entry_t *pcbsa_tbl = NULL;
    const vbsa_rule_entry_t  *vbsa_tbl  = NULL;
    const pfdi_rule_entry_t  *pfdi_tbl  = NULL;
    uint32_t tbl_count = 0;

    if (ctx == NULL)
        return 0;

    /* If architecture is selected (-a), merge its rules into the list, deduped */
    if (ctx->arch_selection != ARCH_NONE) {
        uint32_t add_count = 0;

        if (ctx->arch_selection == ARCH_BSA) {
            /* count BSA entries */
            while (bsa_rule_list[add_count].rule_id != RULE_ID_SENTINEL)
                add_count++;
            bsa_tbl = bsa_rule_list;
        } else if (ctx->arch_selection == ARCH_SBSA) {
            while (sbsa_rule_list[add_count].rule_id != RULE_ID_SENTINEL)
                add_count++;
            sbsa_tbl = sbsa_rule_list;
        } else if (ctx->arch_selection == ARCH_PCBSA) {
            while (pcbsa_rule_list[add_count].rule_id != RULE_ID_SENTINEL)
                add_count++;
            pcbsa_tbl = pcbsa_rule_list;
        } else if (ctx->arch_selection == ARCH_VBSA) {
            while (vbsa_rule_list[add_count].rule_id != RULE_ID_SENTINEL)
                add_count++;
            vbsa_tbl = vbsa_rule_list;
        } else if (ctx->arch_selection == ARCH_PFDI) {
            while (pfdi_rule_list[add_count].rule_id != RULE_ID_SENTINEL)
                add_count++;
            pfdi_tbl = pfdi_rule_list;
        }

        if (add_count > 0) {
            /* Allocate a new buffer sized for worst-case unique merge */
            RULE_ID_e *old_list = ctx->rule_list;
            uint32_t old_count = ctx->rule_count;
            RULE_ID_e *new_list = (RULE_ID_e *)val_memory_alloc((old_count + add_count)
                                   * sizeof(RULE_ID_e));
            if (new_list != NULL) {
                /* Copy existing */
                for (i = 0; i < old_count; i++) new_list[i] = old_list[i];
                uint32_t new_count = old_count;
                /* Append unique entries from table */
                if (bsa_tbl) {
                    for (i = 0; i < add_count; i++) {
                        RULE_ID_e rid = bsa_tbl[i].rule_id;
                        if (!rule_in_list(rid, new_list, new_count))
                            new_list[new_count++] = rid;
                    }
                } else if (sbsa_tbl) {
                    for (i = 0; i < add_count; i++) {
                        RULE_ID_e rid = sbsa_tbl[i].rule_id;
                        if (!rule_in_list(rid, new_list, new_count))
                            new_list[new_count++] = rid;
                    }
                } else if (pcbsa_tbl) {
                    for (i = 0; i < add_count; i++) {
                        RULE_ID_e rid = pcbsa_tbl[i].rule_id;
                        if (!rule_in_list(rid, new_list, new_count))
                            new_list[new_count++] = rid;
                    }
                } else if (vbsa_tbl) {
                    for (i = 0; i < add_count; i++) {
                        RULE_ID_e rid = vbsa_tbl[i].rule_id;
                        if (!rule_in_list(rid, new_list, new_count))
                            new_list[new_count++] = rid;
                    }
                } else if (pfdi_tbl) {
                    for (i = 0; i < add_count; i++) {
                        RULE_ID_e rid = pfdi_tbl[i].rule_id;
                        if (!rule_in_list(rid, new_list, new_count))
                            new_list[new_count++] = rid;
                    }
                }

                if (ctx->rule_list_owned && old_list != NULL)
                    val_memory_free(old_list);
                ctx->rule_list = new_list;
                ctx->rule_count = new_count;
                ctx->rule_list_owned = true;
            }
        }

        /* Also capture the selected table pointer and count for filtering by level */
        if (ctx->arch_selection == ARCH_BSA) {
            bsa_tbl = bsa_rule_list;
            while (bsa_tbl[tbl_count].rule_id != RULE_ID_SENTINEL) tbl_count++;
        } else if (ctx->arch_selection == ARCH_SBSA) {
            sbsa_tbl = sbsa_rule_list;
            while (sbsa_tbl[tbl_count].rule_id != RULE_ID_SENTINEL) tbl_count++;
        } else if (ctx->arch_selection == ARCH_PCBSA) {
            pcbsa_tbl = pcbsa_rule_list;
            while (pcbsa_tbl[tbl_count].rule_id != RULE_ID_SENTINEL) tbl_count++;
        } else if (ctx->arch_selection == ARCH_VBSA) {
            vbsa_tbl = vbsa_rule_list;
            while (vbsa_tbl[tbl_count].rule_id != RULE_ID_SENTINEL) tbl_count++;
        } else if (ctx->arch_selection == ARCH_PFDI) {
            pfdi_tbl = pfdi_rule_list;
            while (pfdi_tbl[tbl_count].rule_id != RULE_ID_SENTINEL) tbl_count++;
        }
    }

    /* if rule list is NULL no filtering required */
    if (ctx->rule_list == NULL || ctx->rule_count == 0)
        return 0;

    out = 0;
    for (i = 0; i < ctx->rule_count; i++) {
        rule = ctx->rule_list[i];
        skip = 0;

        /* Skip explicit rule IDs provided via -skip */
        if (!skip && ctx->skip_rule_count > 0 && ctx->skip_rule_list != NULL) {
            for (si = 0; si < ctx->skip_rule_count; si++) {
                if (ctx->skip_rule_list[si] == rule) {
                    skip = 1;
                    break;
                }
            }
        }

        module = rule_test_map[rule].module_id;

        /* Skip rules belonging to modules listed in -skipmodule */
        if (!skip && ctx->num_skip_modules > 0 && ctx->skip_modules != NULL) {
            for (mi = 0; mi < ctx->num_skip_modules; mi++) {
                if (ctx->skip_modules[mi] == (uint32_t)module) {
                    skip = 1;
                    break;
                }
            }
        }

        /* If -m provided, keep only selected modules */
        if (!skip && ctx->num_modules > 0 && ctx->execute_modules != NULL) {
            found = 0;
            for (mi = 0; mi < ctx->num_modules; mi++) {
                if (ctx->execute_modules[mi] == (uint32_t)module) {
                    found = 1;
                    break; }
            }
            if (!found)
                skip = 1;
        }

        /* Level-based filtering and software view filtering (BSA) */
        if (!skip && ctx->arch_selection != ARCH_NONE) {
            if (ctx->level_filter_mode != LVL_FILTER_NONE ||
                (ctx->arch_selection == ARCH_BSA && ctx->bsa_sw_view_mask != 0)) {
                /* Find rule in the selected arch table */
                bool found_entry = 0;
                if (bsa_tbl) {
                    for (uint32_t ti = 0; ti < tbl_count; ti++) {
                        if (bsa_tbl[ti].rule_id == rule) {
                            found_entry = 1;
                            /* Software view filter if requested: keep if any selected */
                            if (ctx->bsa_sw_view_mask != 0) {
                                uint32_t bit = (1u << (uint32_t)bsa_tbl[ti].sw_view);
                                if ((ctx->bsa_sw_view_mask & bit) == 0) {
                                    skip = 1;
                                    break;
                                }
                            }
                            /* Level filtering */
                            if (ctx->level_filter_mode == LVL_FILTER_FR) {
                                /* Treat FR mode as MAX up to FR */
                                if ((uint32_t)bsa_tbl[ti].level > (uint32_t)BSA_LEVEL_FR)
                                    skip = 1;
                            } else if (ctx->level_filter_mode == LVL_FILTER_ONLY) {
                                if ((uint32_t)bsa_tbl[ti].level != ctx->level_value)
                                    skip = 1;
                            } else if (ctx->level_filter_mode == LVL_FILTER_MAX) {
                                if ((uint32_t)bsa_tbl[ti].level > ctx->level_value)
                                    skip = 1;
                            }
                            break;
                        }
                    }
                } else if (sbsa_tbl) {
                    for (uint32_t ti = 0; ti < tbl_count; ti++) {
                        if (sbsa_tbl[ti].rule_id == rule) {
                            found_entry = 1;
                            if (ctx->level_filter_mode == LVL_FILTER_FR) {
                                /* Treat FR mode as MAX up to FR */
                                if ((uint32_t)sbsa_tbl[ti].level > (uint32_t)SBSA_LEVEL_FR)
                                    skip = 1;
                            } else if (ctx->level_filter_mode == LVL_FILTER_ONLY) {
                                if ((uint32_t)sbsa_tbl[ti].level != ctx->level_value)
                                    skip = 1;
                            } else if (ctx->level_filter_mode == LVL_FILTER_MAX) {
                                if ((uint32_t)sbsa_tbl[ti].level > ctx->level_value)
                                    skip = 1;
                            }
                            break;
                        }
                    }
                } else if (pcbsa_tbl) {
                    for (uint32_t ti = 0; ti < tbl_count; ti++) {
                        if (pcbsa_tbl[ti].rule_id == rule) {
                            found_entry = 1;
                            if (ctx->level_filter_mode == LVL_FILTER_FR) {
                                /* Treat FR mode as MAX up to FR */
                                if ((uint32_t)pcbsa_tbl[ti].level > (uint32_t)PCBSA_LEVEL_FR)
                                    skip = 1;
                            } else if (ctx->level_filter_mode == LVL_FILTER_ONLY) {
                                if ((uint32_t)pcbsa_tbl[ti].level != ctx->level_value)
                                    skip = 1;
                            } else if (ctx->level_filter_mode == LVL_FILTER_MAX) {
                                if ((uint32_t)pcbsa_tbl[ti].level > ctx->level_value)
                                    skip = 1;
                            }
                            break;
                        }
                    }
                } else if (vbsa_tbl) {
                    for (uint32_t ti = 0; ti < tbl_count; ti++) {
                        if (vbsa_tbl[ti].rule_id == rule) {
                            found_entry = 1;
                            if (ctx->level_filter_mode == LVL_FILTER_FR) {
                                /* Treat FR mode as MAX up to FR */
                                if ((uint32_t)vbsa_tbl[ti].level > (uint32_t)VBSA_LEVEL_FR)
                                    skip = 1;
                            } else if (ctx->level_filter_mode == LVL_FILTER_ONLY) {
                                if ((uint32_t)vbsa_tbl[ti].level != ctx->level_value)
                                    skip = 1;
                            } else if (ctx->level_filter_mode == LVL_FILTER_MAX) {
                                if ((uint32_t)vbsa_tbl[ti].level > ctx->level_value)
                                    skip = 1;
                            }
                            break;
                        }
                    }
                } else if (pfdi_tbl) {
                    for (uint32_t ti = 0; ti < tbl_count; ti++) {
                        if (pfdi_tbl[ti].rule_id == rule) {
                            found_entry = 1;
                            if (ctx->level_filter_mode == LVL_FILTER_ONLY) {
                                if ((uint32_t)pfdi_tbl[ti].level != ctx->level_value)
                                    skip = 1;
                            } else if (ctx->level_filter_mode == LVL_FILTER_MAX) {
                                if ((uint32_t)pfdi_tbl[ti].level > ctx->level_value)
                                    skip = 1;
                            }
                            break;
                        }
                    }
                }

                /* If not found in table, conservatively skip or keep? Keep by default */
                (void)found_entry; /* silence unused if not used */
            }
        }

        if (!skip)
            ctx->rule_list[out++] = rule;
    }

    ctx->rule_count = out;
    return out;
}

/**
 * @brief Execute the provided list of rules and report status per rule.
 *
 * Assumes the list has already been filtered for CLI selections (-skip, -m,
 * -skipmodule). Sorts for module-wise execution, checks PAL support, and for
 * alias rules recursively executes their child rules while aggregating status.
 * Records and prints status per rule.
 *
 */
void
run_tests(const acs_run_request_t *ctx)
{
    uint32_t i;
    uint32_t rule_test_status = 0;
    uint32_t rule_support_status;
    uint32_t num_pe;
    RULE_ID_e *rule_list;
    uint32_t list_size;

    if (ctx == NULL || ctx->rule_list == NULL || ctx->rule_count == 0)
        return;

    rule_list = ctx->rule_list;
    list_size = ctx->rule_count;

    val_print(INFO, "\n---------------------- Running tests ------------------------");

    /* Initialize per-rule status map to TEST_STATUS_UNKNOWN for this run */
    rule_status_map_reset();

    /* Get number of PEs in the system */
    num_pe = val_pe_get_num();

    /* quick sort the rule list so that it is module wise as in RULE_ID_e typedef definition */
    quick_sort_rule_list(rule_list, list_size);

    for (i = 0 ; i < list_size; i++) {
        rule_reference_path_reset();

        /* Check for the rule support in current PAL/ACS */
        rule_support_status = check_rule_support(rule_list[i]);

#ifdef TARGET_LINUX
        /* Workaround for linux apps, to skip rule silently */
        if (rule_support_status != TEST_SUPPORTED) {
            continue;
        }
#endif
        /* Print rule header */
        print_rule_test_start(rule_list[i], 0);

        /* Report rule ids not supported by ACS or doesn't have mapping rule_test_map */
        if (rule_support_status != TEST_SUPPORTED) {
            rule_status_map[rule_list[i]] = rule_support_status;
            rule_test_status = rule_support_status;
            goto report_status;
        }

        rule_test_status = execute_rule_recursive(ctx, rule_list[i], 0, num_pe, 0);
report_status:
        /* Record and print overall rule status */
        rule_status_map[rule_list[i]] = rule_test_status;
        print_rule_test_status(rule_list[i], 0, rule_test_status);

    }
    val_print(INFO,
              "\n-------------------- Suite run complete --------------------\n");
}
