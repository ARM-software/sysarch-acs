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

#include "include/rule_based_execution.h"
#include "include/acs_common.h"
#include "include/val_interface.h"
#include "include/acs_pe.h"
#include "include/acs_memory.h"

extern uint8_t g_current_pal;
extern rule_test_map_t rule_test_map[RULE_ID_SENTINEL];
extern alias_rule_map_t alias_rule_map[];
extern test_entry_fn_t test_entry_func_table[TEST_ENTRY_SENTINEL];
extern char *rule_id_string[RULE_ID_SENTINEL];
/* Access selections from app */
extern RULE_ID_e *g_skip_rule_list;
extern uint32_t   g_skip_rule_count;
extern uint32_t  *g_execute_modules;
extern uint32_t   g_num_modules;
extern uint32_t  *g_skip_modules;
extern uint32_t   g_num_skip_modules;

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

   // val_print(ACS_PRINT_ERR, "\n plat_bitmask %x", plat_bitmask);

    if (!(g_current_pal & plat_bitmask)) {
        /* Report if rule is not supported by ACS across all available PALs*/
        if (plat_bitmask == 0) {
            /* Not covered by ACS in any supported PAL */
            return TEST_NOT_IMPLEMENTED;
        } else {
            /* Test not supported with current PAL */
            return TEST_PAL_NOT_SUPPORTED;
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
static bool is_rule_skipped(RULE_ID_e rule_id)
{
    uint32_t i;
    MODULE_NAME_e module;

    /* Check explicit rule skip list (-skip) */
    if (g_skip_rule_count > 0 && g_skip_rule_list != NULL) {
        for (i = 0; i < g_skip_rule_count; i++) {
            if (g_skip_rule_list[i] == rule_id)
                return 1;
        }
    }

    /* Check module skip list (-skipmodule) */
    if (g_num_skip_modules > 0 && g_skip_modules != NULL) {
        module = rule_test_map[rule_id].module_id;
        for (i = 0; i < g_num_skip_modules; i++) {
            if (g_skip_modules[i] == (uint32_t)module)
                return 1;
        }
    }

    return 0;
}

/**
 * @brief Filter the provided rule list in place based on CLI selections.
 *
 * Applies the following filters to the rule list, compacting it in-place while
 * preserving the original relative order of rules kept:
 * - Rules listed in -skip (g_skip_rule_list) are removed.
 * - Rules whose module matches any in -skipmodule (g_skip_modules) are removed.
 * - If -m (g_execute_modules) is provided and non-empty, only rules whose module
 *   is in that list are kept.
 *
 * No new memory is allocated. Elements beyond the returned count remain
 * unchanged but are considered out of range by callers.
 *
 * @param rule_list Pointer to the array of rule IDs to filter.
 * @param list_size Number of elements in the input list.
 * @return New count of rules after filtering.
 */
uint32_t filter_rule_list_by_cli(RULE_ID_e **rule_list, uint32_t list_size)
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
    uint32_t tbl_count = 0;

    /* if pointer is NULL, API misuse return */
    if (rule_list == NULL)
        return 0;

    /* If architecture is selected (-a), merge its rules into the list, deduped */
    if (g_arch_selection != ARCH_NONE) {
        uint32_t add_count = 0;

        if (g_arch_selection == ARCH_BSA) {
            /* count BSA entries */
            while (bsa_rule_list[add_count].rule_id != RULE_ID_SENTINEL)
                add_count++;
            bsa_tbl = bsa_rule_list;
        } else if (g_arch_selection == ARCH_SBSA) {
            while (sbsa_rule_list[add_count].rule_id != RULE_ID_SENTINEL)
                add_count++;
            sbsa_tbl = sbsa_rule_list;
        } else if (g_arch_selection == ARCH_PCBSA) {
            while (pcbsa_rule_list[add_count].rule_id != RULE_ID_SENTINEL)
                add_count++;
            pcbsa_tbl = pcbsa_rule_list;
        } else if (g_arch_selection == ARCH_VBSA) {
            while (vbsa_rule_list[add_count].rule_id != RULE_ID_SENTINEL)
                add_count++;
            vbsa_tbl = vbsa_rule_list;
        }

        if (add_count > 0) {
            /* Allocate a new buffer sized for worst-case unique merge */
            RULE_ID_e *old_list = *rule_list;
            uint32_t old_count = list_size;
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
                }

                /* Free old buffer (if allocated via VAL) and update pointer */
                if (old_list)
                    val_memory_free(old_list);
                *rule_list = new_list;
                list_size = new_count;
            }
        }

        /* Also capture the selected table pointer and count for filtering by level */
        if (g_arch_selection == ARCH_BSA) {
            bsa_tbl = bsa_rule_list;
            while (bsa_tbl[tbl_count].rule_id != RULE_ID_SENTINEL) tbl_count++;
        } else if (g_arch_selection == ARCH_SBSA) {
            sbsa_tbl = sbsa_rule_list;
            while (sbsa_tbl[tbl_count].rule_id != RULE_ID_SENTINEL) tbl_count++;
        } else if (g_arch_selection == ARCH_PCBSA) {
            pcbsa_tbl = pcbsa_rule_list;
            while (pcbsa_tbl[tbl_count].rule_id != RULE_ID_SENTINEL) tbl_count++;
        } else if (g_arch_selection == ARCH_VBSA) {
            vbsa_tbl = vbsa_rule_list;
            while (vbsa_tbl[tbl_count].rule_id != RULE_ID_SENTINEL) tbl_count++;
        }
    }

    /* if rule list is NULL no filtering required */
    if (*rule_list == NULL || list_size == 0)
        return 0;

    out = 0;
    for (i = 0; i < list_size; i++) {
        rule = (*rule_list)[i];
        skip = 0;

        /* Skip explicit rule IDs provided via -skip */
        if (!skip && g_skip_rule_count > 0 && g_skip_rule_list != NULL) {
            for (si = 0; si < g_skip_rule_count; si++) {
                if (g_skip_rule_list[si] == rule) {
                    skip = 1;
                    break;
                }
            }
        }

        module = rule_test_map[rule].module_id;

        /* Skip rules belonging to modules listed in -skipmodule */
        if (!skip && g_num_skip_modules > 0 && g_skip_modules != NULL) {
            for (mi = 0; mi < g_num_skip_modules; mi++) {
                if (g_skip_modules[mi] == (uint32_t)module) {
                    skip = 1;
                    break;
                }
            }
        }

        /* If -m provided, keep only selected modules */
        if (!skip && g_num_modules > 0 && g_execute_modules != NULL) {
            found = 0;
            for (mi = 0; mi < g_num_modules; mi++) {
                if (g_execute_modules[mi] == (uint32_t)module) {
                    found = 1;
                    break; }
            }
            if (!found)
                skip = 1;
        }

        /* Level-based filtering and software view filtering (BSA) */
        if (!skip && g_arch_selection != ARCH_NONE) {
            if (g_level_filter_mode != LVL_FILTER_NONE ||
                (g_arch_selection == ARCH_BSA && g_bsa_sw_view_mask != 0)) {
                /* Find rule in the selected arch table */
                bool found_entry = 0;
                if (bsa_tbl) {
                    for (uint32_t ti = 0; ti < tbl_count; ti++) {
                        if (bsa_tbl[ti].rule_id == rule) {
                            found_entry = 1;
                            /* Software view filter if requested: keep if any selected */
                            if (g_bsa_sw_view_mask != 0) {
                                uint32_t bit = (1u << (uint32_t)bsa_tbl[ti].sw_view);
                                if ((g_bsa_sw_view_mask & bit) == 0) {
                                    skip = 1;
                                    break;
                                }
                            }
                            /* Level filtering */
                            if (g_level_filter_mode == LVL_FILTER_FR) {
                                /* Treat FR mode as MAX up to FR */
                                if ((uint32_t)bsa_tbl[ti].level > (uint32_t)BSA_LEVEL_FR)
                                    skip = 1;
                            } else if (g_level_filter_mode == LVL_FILTER_ONLY) {
                                if ((uint32_t)bsa_tbl[ti].level != g_level_value)
                                    skip = 1;
                            } else if (g_level_filter_mode == LVL_FILTER_MAX) {
                                if ((uint32_t)bsa_tbl[ti].level > g_level_value)
                                    skip = 1;
                            }
                            break;
                        }
                    }
                } else if (sbsa_tbl) {
                    for (uint32_t ti = 0; ti < tbl_count; ti++) {
                        if (sbsa_tbl[ti].rule_id == rule) {
                            found_entry = 1;
                            if (g_level_filter_mode == LVL_FILTER_FR) {
                                /* Treat FR mode as MAX up to FR */
                                if ((uint32_t)sbsa_tbl[ti].level > (uint32_t)SBSA_LEVEL_FR)
                                    skip = 1;
                            } else if (g_level_filter_mode == LVL_FILTER_ONLY) {
                                if ((uint32_t)sbsa_tbl[ti].level != g_level_value)
                                    skip = 1;
                            } else if (g_level_filter_mode == LVL_FILTER_MAX) {
                                if ((uint32_t)sbsa_tbl[ti].level > g_level_value)
                                    skip = 1;
                            }
                            break;
                        }
                    }
                } else if (pcbsa_tbl) {
                    for (uint32_t ti = 0; ti < tbl_count; ti++) {
                        if (pcbsa_tbl[ti].rule_id == rule) {
                            found_entry = 1;
                            if (g_level_filter_mode == LVL_FILTER_FR) {
                                /* Treat FR mode as MAX up to FR */
                                if ((uint32_t)pcbsa_tbl[ti].level > (uint32_t)PCBSA_LEVEL_FR)
                                    skip = 1;
                            } else if (g_level_filter_mode == LVL_FILTER_ONLY) {
                                if ((uint32_t)pcbsa_tbl[ti].level != g_level_value)
                                    skip = 1;
                            } else if (g_level_filter_mode == LVL_FILTER_MAX) {
                                if ((uint32_t)pcbsa_tbl[ti].level > g_level_value)
                                    skip = 1;
                            }
                            break;
                        }
                    }
                } else if (vbsa_tbl) {
                    for (uint32_t ti = 0; ti < tbl_count; ti++) {
                        if (vbsa_tbl[ti].rule_id == rule) {
                            found_entry = 1;
                            if (g_level_filter_mode == LVL_FILTER_FR) {
                                /* Treat FR mode as MAX up to FR */
                                if ((uint32_t)vbsa_tbl[ti].level > (uint32_t)VBSA_LEVEL_FR)
                                    skip = 1;
                            } else if (g_level_filter_mode == LVL_FILTER_ONLY) {
                                if ((uint32_t)vbsa_tbl[ti].level != g_level_value)
                                    skip = 1;
                            } else if (g_level_filter_mode == LVL_FILTER_MAX) {
                                if ((uint32_t)vbsa_tbl[ti].level > g_level_value)
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
            (*rule_list)[out++] = rule;
    }

    return out;
}

/**
 * @brief Execute the provided list of rules and report status per rule.
 *
 * Assumes the list has already been filtered for CLI selections (-skip, -m,
 * -skipmodule). Sorts for module-wise execution, checks PAL support, and for
 * alias rules executes their base rules while aggregating status. Records and
 * prints status per rule.
 *
 * @param rule_list Pointer to the array of rule IDs to execute.
 * @param list_size Number of elements in the list.
 */
void
run_tests(RULE_ID_e *rule_list, uint32_t list_size)
{
    bool test_ns_flag;
    uint32_t i, j;
    uint32_t alias_rule_map_index;
    uint32_t rule_test_status;
    uint32_t base_rule_status;
    uint32_t precheck_status;
    uint32_t rule_support_status;
    uint32_t num_pe;
    RULE_ID_e base_rule_id;
    RULE_ID_e *base_rule_list;

    val_print(ACS_PRINT_ERR, "\n---------------------- Running tests ------------------------", 0);

    /* Initialize per-rule status map to TEST_STATUS_UNKNOWN for this run */
    rule_status_map_reset();

    /* Get number of PEs in the system */
    num_pe = val_pe_get_num();

    /* quick sort the rule list so that it is module wise as in RULE_ID_e typedef definition */
    quick_sort_rule_list(rule_list, list_size);

    for (i = 0 ; i < list_size; i++) {
        /* Invalid  rule_test_map entry check */
        // if (rule_test_map[rule_list[i]].flag == INVALID_ENTRY) {
        //     val_print(ACS_PRINT_ERR, "\n", 0);
        //     val_print(ACS_PRINT_ERR, rule_id_string[rule_list[i]], 0);
        //     val_print(ACS_PRINT_ERR, " has invalid rule_test_map[] entry.", 0);
        //     continue;
        // }

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

        /* Check if rule id is alias, if yes do the table walk to find base rules */
        if (rule_test_map[rule_list[i]].flag == ALIAS_RULE) {
            /* Get base rules for the alias rule */
            /* Use the actual rule id value, not loop index */
            alias_rule_map_index = alias_rule_map_get_index(rule_list[i]);

            /* Validate lookup result before dereferencing the map */
            if (alias_rule_map_index == INVALID_IDX) {
                val_print(ACS_PRINT_ERR, " alias map index not found for rule id: 0x%x",
                          rule_list[i]);
                /* Skip executing base rules for an unknown alias */
                continue;
            }

            /* If a precheck exists then use rule_test_map.test_entry_id to mention enum to entry
               function to do the precheck, if NULL_ENTRY then consider no precheck for
               the ALIAS */
            if (rule_test_map[rule_list[i]].test_entry_id != NULL_ENTRY) {
                precheck_status =
                    test_entry_func_table[rule_test_map[rule_list[i]].test_entry_id](num_pe);

                /* If precheck fails, report alias rule status as SKIP as it wont be applicable */
                if (precheck_status == TEST_FAIL) {
                    rule_test_status = TEST_SKIP;
                    goto report_status;
                }
            }

            /* reset rule test status to unknown */
            rule_test_status = TEST_STATUS_UNKNOWN;
            /* init a flag to track partial coverage */
            test_ns_flag = 0;
            /* convenience alias to the base rule list for this alias */
            base_rule_list = alias_rule_map[alias_rule_map_index].base_rule_list;

            /* Print start header for alias rule */
            val_print(ACS_PRINT_TEST, "\n\n  === Start tests for rules referenced by ", 0);
            val_print(ACS_PRINT_TEST, rule_id_string[rule_list[i]], 0);
            val_print(ACS_PRINT_TEST, " ===", 0);

            /* Run the base rules required by the alias; list is sentinel-terminated */
            for (j = 0; base_rule_list[j] != RULE_ID_SENTINEL; j++) {
                /* Check if test for the base rule is present in current PAL */
                rule_support_status = check_rule_support(base_rule_list[j]);
#ifdef TARGET_LINUX
                /* Workaround for linux apps, to skip rule silently */
                if (rule_support_status != TEST_SUPPORTED) {
                    continue;
                }
#endif
                /* -skip and -skipmodule only apply to initial rule list; ensure
                   base rules of an alias honor these selections here. */
                if (is_rule_skipped(base_rule_list[j])) {
                    /* Skip executing this base rule as per CLI selection */
                    continue;
                }

                /* Print base rule header */
                print_rule_test_start(base_rule_list[j], 1);

                if (rule_support_status != TEST_SUPPORTED) {
                    /* set a flag to track partial coverage */
                    test_ns_flag = 1;
                    base_rule_status = rule_support_status;
                    /* record base rule status */
                    rule_status_map[base_rule_list[j]] = base_rule_status;
                    print_rule_test_status(base_rule_list[j], 1,  base_rule_status);
                    continue;
                }

                /* Run the base rule */
                base_rule_id = alias_rule_map[alias_rule_map_index].base_rule_list[j];
                base_rule_status =
                    test_entry_func_table[rule_test_map[base_rule_id].test_entry_id](num_pe);
                /* record base rule status */
                rule_status_map[base_rule_id] = base_rule_status;
                /* report status of base rule run */
                print_rule_test_status(base_rule_list[j], 1, base_rule_status);
                /* update overall alias rule status */
                if ((base_rule_status > rule_test_status)
                                      || (rule_test_status == TEST_STATUS_UNKNOWN)) {
                    rule_test_status = base_rule_status;
                }
            }
            /* Post-check: if any base rule was not supported but overall
               status is PASS, mark the alias as partial coverage. */
            if (test_ns_flag && (rule_test_status == TEST_PASS)) {
                rule_test_status = TEST_PART_COV;
            }

            /* Print end header for alias rule */
            val_print(ACS_PRINT_TEST, "\n\n  === End tests for rules referenced by ", 0);
            val_print(ACS_PRINT_TEST, rule_id_string[rule_list[i]], 0);
            val_print(ACS_PRINT_TEST, " ===\n", 0);

        } else if (rule_test_map[rule_list[i]].flag == BASE_RULE) {
            /* Base rule would have single test entry, could be wrapper too */
            rule_test_status =
                test_entry_func_table[rule_test_map[rule_list[i]].test_entry_id](num_pe);

        }
report_status:
        /* Record and print overall rule status */
        rule_status_map[rule_list[i]] = rule_test_status;
        print_rule_test_status(rule_list[i], 0, rule_test_status);

    }
    val_print(ACS_PRINT_TEST,
              "\n-------------------- Suite run complete --------------------\n",
              0);
}
