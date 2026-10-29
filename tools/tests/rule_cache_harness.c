/* Copyright (c) 2026, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "rule_based_execution.h"
#include "acs_execution_policy.h"
#include "acs_pe.h"
#include "acs_runtime_init.h"
#include "pal_execution_policy.h"

#define ROOT_A B_PE_01
#define ROOT_B B_PE_02
#define LEAF_A PCI_IN_02
#define LEAF_B PCI_IN_16
#define NUM_ALIASES 12

uint8_t g_current_pal = 1;
rule_test_map_t rule_test_map[RULE_ID_SENTINEL];
uint32_t rule_status_map[RULE_ID_SENTINEL] __attribute__((aligned(8)));
uint32_t test_entry_status_map[TEST_ENTRY_SENTINEL];
char *rule_id_string[RULE_ID_SENTINEL];
static RULE_ID_e children[NUM_ALIASES][3];
const alias_rule_map_t alias_rule_map[] = {
    {B_PE_01, children[0]}, {B_PE_02, children[1]},
    {B_PE_03, children[2]}, {B_PE_04, children[3]},
    {B_PE_05, children[4]}, {B_PE_06, children[5]},
    {B_PE_07, children[6]}, {B_PE_08, children[7]},
    {B_PE_09, children[8]}, {B_PE_10, children[9]},
    {B_PE_11, children[10]}, {B_PE_12, children[11]}
};
const uint32_t alias_rule_map_count = NUM_ALIASES;

static uint32_t calls, prechecks, cached, cycles, depth_errors, pe_count, last_pe;
static uint32_t starts[RULE_ID_SENTINEL], reports[RULE_ID_SENTINEL];
static uint32_t leaf_status;
static bool distinguish_rules;
static const acs_execution_policy_t *platform_policy;
static acs_test_status_counters_t stats;

acs_test_status_counters_t *acs_get_test_status(void)
{
    return &stats;
}

void test_report_status(uint32_t status)
{
    (void)status;
}

const acs_execution_policy_t *acs_get_platform_execution_policy_defaults(void)
{
    return platform_policy;
}

static uint32_t leaf_entry(uint32_t num_pe)
{
    const RULE_ID_e *path = rule_reference_path_get();
    RULE_ID_e leaf = RULE_ID_SENTINEL;

    ++calls;
    last_pe = num_pe;
    while (*path != RULE_ID_SENTINEL)
        leaf = *path++;
    if (distinguish_rules && leaf == LEAF_B)
        return RESULT_FAIL(7);
    return leaf_status;
}

static uint32_t precheck_entry(uint32_t num_pe)
{
    (void)num_pe;
    return ++prechecks == 1 ? RESULT_PASS : RESULT_SKIP(9);
}

test_entry_fn_t test_entry_func_table[TEST_ENTRY_SENTINEL] = {
    [PE001_ENTRY] = leaf_entry,
    [PE002_ENTRY] = precheck_entry
};

uint32_t val_pe_get_num(void)
{
    return pe_count;
}

uint32_t val_log_get_indent(void)
{
    return acs_policy_get_log_indent();
}

void val_log_set_indent(uint32_t indent)
{
    acs_policy_set_log_indent(indent);
}

uint32_t val_printf(print_verbosity_t level, const char *message, ...)
{
    (void)level;
    if (strstr(message, "(cached)"))
        ++cached;
    if (strstr(message, "Recursive alias reference"))
        ++cycles;
    if (strstr(message, "path depth exceeded"))
        ++depth_errors;
    return 0;
}

void __wrap_print_rule_test_start(uint32_t rule, uint32_t indent)
{
    (void)indent;
    ++starts[rule];
}

void __real_print_rule_test_status(uint32_t rule, uint32_t indent, uint32_t status);

void __wrap_print_rule_test_status(uint32_t rule, uint32_t indent, uint32_t status)
{
    assert(rule_status_map[rule] == status);
    ++reports[rule];
    __real_print_rule_test_status(rule, indent, status);
}

static void reset(bool enable)
{
    uint32_t i;

    memset(rule_test_map, 0, sizeof(rule_test_map));
    memset(starts, 0, sizeof(starts));
    memset(reports, 0, sizeof(reports));
    memset(&stats, 0, sizeof(stats));
    for (i = 0; i < RULE_ID_SENTINEL; ++i)
        rule_id_string[i] = "TEST_RULE";
    for (i = 0; i < NUM_ALIASES; ++i) {
        children[i][0] = LEAF_A;
        children[i][1] = RULE_ID_SENTINEL;
        children[i][2] = RULE_ID_SENTINEL;
        rule_test_map[alias_rule_map[i].alias_rule_id] = (rule_test_map_t) {
            .flag = ALIAS_RULE, .platform_bitmask = 1, .module_id = PE
        };
    }
    rule_test_map[LEAF_A] = (rule_test_map_t) {
        .flag = BASE_RULE, .platform_bitmask = 1, .module_id = PCIE,
        .test_entry_id = PE001_ENTRY
    };
    rule_test_map[LEAF_B] = rule_test_map[LEAF_A];
    calls = prechecks = cached = cycles = depth_errors = last_pe = 0;
    pe_count = 1;
    distinguish_rules = false;
    leaf_status = RESULT_PASS;
    acs_reset_execution_policy();
    assert(!acs_policy_get_rule_cache_enabled());
    acs_get_execution_policy_mut()->rule_cache_enabled = enable;
}

static acs_run_request_t request(RULE_ID_e *rules, uint32_t count)
{
    acs_run_request_t ctx = {0};
    ctx.rule_list = rules;
    ctx.rule_count = count;
    ctx.arch_selection = ARCH_SBSA;
    return ctx;
}

static void shared_result(bool enable, uint32_t status)
{
    RULE_ID_e rules[] = {ROOT_A, ROOT_B, LEAF_A};
    acs_run_request_t ctx = request(rules, 3);

    reset(enable);
    leaf_status = status;
    run_tests(&ctx);
    assert(calls == (enable && status != TEST_STATE_UNKNOWN ? 1u : 3u));
    assert(cached == (enable && status != TEST_STATE_UNKNOWN ? 2u : 0u));
    assert(starts[LEAF_A] == 3 && reports[LEAF_A] == 3);
    assert(reports[ROOT_A] == 1 && reports[ROOT_B] == 1);
    assert(rule_status_map[ROOT_A] == status && rule_status_map[ROOT_B] == status);
    assert(rule_status_map[LEAF_A] == status);
    assert(stats.total_rules_run == 3);
    if (GET_STATE(status) == TEST_PASS)
        assert(stats.passed == 3);
    if (GET_STATE(status) == TEST_FAIL)
        assert(stats.failed == 3);
    assert(rule_reference_path_get()[0] == RULE_ID_SENTINEL);
    assert(val_log_get_indent() == 0);
}

static void successive_runs(void)
{
    RULE_ID_e rules[] = {ROOT_A, ROOT_B};
    acs_run_request_t ctx = request(rules, 2);

    reset(true);
    run_tests(&ctx);
    assert(calls == 1);
    leaf_status = RESULT_FAIL(8);
    pe_count = 4;
    run_tests(&ctx);
    assert(calls == 2 && last_pe == 4);
    assert(rule_status_map[ROOT_A] == leaf_status);
    assert(rule_status_map[ROOT_B] == leaf_status);
    acs_get_execution_policy_mut()->rule_cache_enabled = false;
    run_tests(&ctx);
    assert(calls == 4);
}

static void distinct_rules(bool mpam)
{
    RULE_ID_e rules[] = {LEAF_A, LEAF_B};
    acs_run_request_t ctx = request(rules, 2);
    uint32_t status;

    reset(!mpam);
    distinguish_rules = !mpam;
    if (mpam)
        ctx.arch_selection = ARCH_SYS_MPAM;
    run_tests(&ctx);
    assert(calls == (mpam ? 1u : 2u));
    assert(rule_status_map[LEAF_B] == (mpam ? RESULT_PASS : RESULT_FAIL(7)));
    assert(!test_entry_status_cached(PE001_ENTRY, &status));
}

static void skip_child(bool module)
{
    RULE_ID_e rules[] = {ROOT_A, ROOT_B};
    RULE_ID_e skip[] = {LEAF_A};
    uint32_t modules[] = {PCIE};
    acs_run_request_t ctx = request(rules, 2);

    reset(true);
    if (module) {
        ctx.skip_modules = modules;
        ctx.num_skip_modules = 1;
    } else {
        ctx.skip_rule_list = skip;
        ctx.skip_rule_count = 1;
    }
    run_tests(&ctx);
    assert(calls == 0 && cached == 0 && starts[LEAF_A] == 0);
}

static void alias_prechecks(void)
{
    RULE_ID_e rules[] = {ROOT_A, ROOT_B};
    acs_run_request_t ctx = request(rules, 2);

    reset(true);
    children[0][0] = B_PE_03;
    children[1][0] = B_PE_03;
    rule_test_map[B_PE_03].test_entry_id = PE002_ENTRY;
    run_tests(&ctx);
    assert(calls == 1 && prechecks == 2);
    assert(rule_status_map[ROOT_A] == RESULT_PASS);
    assert(rule_status_map[ROOT_B] == RESULT_SKIP(9));
    assert(reports[B_PE_03] == 2);
}

static void cycle_or_depth(bool depth)
{
    RULE_ID_e rules[] = {ROOT_A, ROOT_B};
    acs_run_request_t ctx = request(rules, 1);
    uint32_t i;

    reset(true);
    if (depth) {
        /* Warm a base-rule result before visiting it below the depth limit. */
        ctx.rule_count = 2;
        rule_test_map[ROOT_A] = rule_test_map[LEAF_A];
        for (i = 1; i < RULE_REFERENCE_PATH_MAX_DEPTH; ++i)
            children[i][0] = alias_rule_map[i + 1].alias_rule_id;
        children[RULE_REFERENCE_PATH_MAX_DEPTH][0] = ROOT_A;
    } else {
        children[0][0] = ROOT_B;
        children[1][0] = ROOT_A;
    }
    run_tests(&ctx);
    assert(IS_TEST_FAIL(rule_status_map[depth ? ROOT_B : ROOT_A]));
    assert(calls == (depth ? 1u : 0u) && cached == 0);
    assert(depth ? depth_errors == 1 : cycles == 1);
    assert(rule_reference_path_get()[0] == RULE_ID_SENTINEL);
}

static void top_level_first(void)
{
    RULE_ID_e rules[] = {ROOT_A, ROOT_B};
    acs_run_request_t ctx = request(rules, 2);

    reset(true);
    rule_test_map[ROOT_A] = rule_test_map[LEAF_A];
    children[1][0] = ROOT_A;
    run_tests(&ctx);
    assert(calls == 1 && cached == 1);
    assert(reports[ROOT_A] == 2 && reports[ROOT_B] == 1);
}

static void unsupported_child(void)
{
    RULE_ID_e rules[] = {ROOT_A, ROOT_B};
    acs_run_request_t ctx = request(rules, 2);

    reset(true);
    rule_test_map[LEAF_A].platform_bitmask = 2;
    run_tests(&ctx);
    assert(calls == 0 && cached == 0);
    assert(reports[LEAF_A] == 2);
}

static void baremetal_configuration(void)
{
    acs_execution_policy_t detached, defaults = {0};
    acs_run_request_t ctx = {0};

    memset(&detached, 0xA5, sizeof(detached));
    platform_policy = NULL;
    acs_load_execution_policy_defaults(&detached);
    assert(!detached.rule_cache_enabled);
    acs_apply_compile_params(&ctx, &detached);
    assert(detached.rule_cache_enabled == (ACS_RULE_CACHE != 0));

    defaults.rule_cache_enabled = true;
    platform_policy = &defaults;
    acs_load_execution_policy_defaults(&detached);
    assert(detached.rule_cache_enabled);

    platform_policy = NULL;
    acs_get_execution_policy_mut()->rule_cache_enabled = true;
    acs_load_execution_policy_defaults(acs_get_execution_policy_mut());
    assert(!acs_policy_get_rule_cache_enabled());
}

int main(void)
{
    shared_result(false, RESULT_PASS);
    shared_result(true, RESULT_PASS);
    shared_result(true, RESULT_FAIL(3));
    shared_result(true, RESULT_SKIP(3));
    shared_result(true, RESULT_WARNING(3));
    shared_result(true, RESULT_PARTIAL_COVERED);
    shared_result(true, TEST_STATE_UNKNOWN);
    successive_runs();
    distinct_rules(false);
    distinct_rules(true);
    skip_child(false);
    skip_child(true);
    alias_prechecks();
    cycle_or_depth(false);
    cycle_or_depth(true);
    top_level_first();
    unsupported_child();
    baremetal_configuration();
    puts("18 rule-cache regressions passed");
    return 0;
}
