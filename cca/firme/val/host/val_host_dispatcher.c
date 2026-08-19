/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_platform.h"
#include "val_firme_diag.h"
#include "val_firme_rules.h"
#include "val_firme_suite.h"
#include "val_firme_dispatch.h"
#include "val_firme_abi.h"
#include "val_log.h"
#include "val_common/val_smccc.h"
#include "val_firme_spmd_msg.h"

static int is_pass_or_skip(uint64_t s)
{
    return (s == (uint64_t)ACS_STATUS_PASS) || (s == (uint64_t)ACS_STATUS_SKIP);
}

/**
 *   @brief    - Combine a per-world rule result into an overall rule result.
 *   @param    - overall    : Overall result record to update.
 *   @param    - candidate  : Per-world result to include.
 *   @param    - have_result: Whether an earlier result was included.
 *   @return   - void
**/
static void combine_rule_result(firme_acs_call_res_t *overall,
                                const firme_acs_call_res_t *candidate,
                                uint32_t *have_result)
{
    if (!overall || !candidate || !have_result)
        return;

    if (*have_result == 0U) {
        *overall = *candidate;
        *have_result = 1U;
        return;
    }

    if (!is_pass_or_skip(candidate->x0)) {
        *overall = *candidate;
        return;
    }

    if (overall->x0 == (uint64_t)ACS_STATUS_SKIP)
        *overall = *candidate;
}

/**
 *   @brief    - Map an internal status code to a printable string.
 *   @param    - s : Status code (ACS_STATUS_*).
 *   @return   - Status string.
**/
static const char *status_str(uint64_t s)
{
    if (s == (uint64_t)ACS_STATUS_PASS)
        return "PASS";
    if (s == (uint64_t)ACS_STATUS_SKIP)
        return "SKIP";
    if (s == (uint64_t)ACS_STATUS_ERR)
        return "ERR";
    return "FAIL";
}

/**
 *   @brief    - Print a consistent test banner (begin).
 *   @param    - suite : Suite name.
 *   @param    - test  : Test name.
 *   @param    - rule  : Rule identifier (e.g. "R0087").
 *   @param    - desc  : Optional description line.
 *   @return   - void
**/
static void print_test_begin(const char *suite, const char *test,
                             const char *rule, const char *desc)
{
    LOG_TEST(NULL, "");
    LOG_TEST(NULL, "*******************************************************");
    LOG_TEST(NULL, "Suite: %s, Test: %s", suite ? suite : "?", test ? test : "?");
    LOG_TEST(NULL, "Rule: %s", rule ? rule : "?");
    if (desc)
        LOG_TEST(NULL, "Desc: %s", desc);
    LOG_TEST(NULL, "");
}

/**
 *   @brief    - Print the final rule result and minimal debug details (within the banner).
 *   @param    - rule : Rule identifier string (unused but kept for callsite clarity).
 *   @param    - r    : Result record returned by the test implementation.
 *   @return   - void
**/
static void log_rule_status(const char *rule, const firme_acs_call_res_t *r)
{
    if (!rule || !r) {
        LOG_ERROR(NULL, "rule status: invalid args");
        return;
    }

    LOG_TEST(NULL, "");
    if (r->x0 == (uint64_t)ACS_STATUS_PASS) {
        LOG_TEST(NULL, "Result: PASS");
        return;
    }
    if (r->x0 == (uint64_t)ACS_STATUS_SKIP) {
        LOG_TEST(NULL, "Result: SKIPPED");
        if ((r->x1 != 0ULL) || (r->x2 != 0ULL) || (r->x3 != 0ULL)) {
            LOG_TEST(NULL,
                     "  Skip: reason=%llu detail_x0=%llx detail_x1=%llx",
                     (unsigned long long)r->x1,
                     (unsigned long long)r->x2,
                     (unsigned long long)r->x3);
        }
        return;
    }
    LOG_TEST(NULL, "Result: FAIL");
    LOG_TEST(NULL, "  Debug: fail_step=%llu last_x0=%llx last_x1=%llx",
             (unsigned long long)r->x1,
             (unsigned long long)r->x2,
             (unsigned long long)r->x3);
}

/**
 *   @brief    - Print a consistent test banner (end).
 *   @return   - void
**/
static void print_test_end(void)
{
    LOG_TEST(NULL, "*******************************************************");
    LOG_TEST(NULL, "");
}

/**
 *   @brief    - Update the test summary counters for a single rule outcome.
 *   @param    - status  : ACS_STATUS_* outcome for the rule.
 *   @param    - total   : Total rules executed (in/out).
 *   @param    - passed  : Passed count (in/out).
 *   @param    - failed  : Failed count (in/out).
 *   @param    - skipped : Skipped count (in/out).
 *   @return   - void
**/
static void update_counts(uint64_t status,
                          uint32_t *total, uint32_t *passed, uint32_t *failed, uint32_t *skipped)
{
    if (!total || !passed || !failed || !skipped)
        return;
    (*total)++;
    if (status == (uint64_t)ACS_STATUS_PASS) {
        (*passed)++;
        return;
    }
    if (status == (uint64_t)ACS_STATUS_SKIP) {
        (*skipped)++;
        return;
    }
    (*failed)++;
}

/**
 *   @brief    - Run a single test case from the registry (may involve multiple worlds).
 *   @param    - tc      : Test case descriptor.
 *   @param    - total   : Total rules executed (in/out).
 *   @param    - passed  : Passed count (in/out).
 *   @param    - failed  : Failed count (in/out).
 *   @param    - skipped : Skipped count (in/out).
 *   @return   - void
**/
static void run_test_case(const firme_test_case_t *tc,
                          uint32_t *total, uint32_t *passed, uint32_t *failed, uint32_t *skipped)
{
    firme_acs_call_res_t res_ns = {0};
    firme_acs_call_res_t res_sec = {0};
    firme_acs_call_res_t res_realm = {0};
    firme_acs_call_res_t overall = {0};
    uint32_t check_idx = 1;
    uint32_t have_result = 0U;
    uint32_t secure_required = 0U;
    uint32_t realm_required = 0U;
    uint32_t ns_required = 0U;

    if (!tc)
        return;

    secure_required = ((tc->required_worlds_mask & FIRME_WORLD_MASK_SECURE) != 0U) ?
                      VAL_TRUE : VAL_FALSE;
    realm_required = ((tc->required_worlds_mask & FIRME_WORLD_MASK_REALM) != 0U) ?
                     VAL_TRUE : VAL_FALSE;
    ns_required = ((tc->required_worlds_mask & FIRME_WORLD_MASK_NS) != 0U) ?
                  VAL_TRUE : VAL_FALSE;

    print_test_begin(tc->suite, tc->test_name, tc->rule_str, tc->desc);

    /* Secure world (optional/required depending on registry flags). */
    if (tc->run_secure) {
        const uint64_t base_pa = val_firme_gpt_base_for_world(FIRME_ACS_WORLD_SECURE);
        if (val_firme_world_call(FIRME_ACS_WORLD_SECURE, FIRME_ACS_CMD_RUN_RULE,
                                 (uint64_t)tc->rule_id, base_pa, 0, 0, &res_sec) == 0) {
            LOG_TEST(NULL, "Check %u: %s ... %s", (unsigned)check_idx,
                     tc->check_secure ? tc->check_secure : "Secure caller",
                     status_str(res_sec.x0));
            if (!is_pass_or_skip(res_sec.x0)) {
                LOG_TEST(NULL, "  Debug: fail_step=%llu last_x0=%llx last_x1=%llx",
                         (unsigned long long)res_sec.x1,
                         (unsigned long long)res_sec.x2,
                         (unsigned long long)res_sec.x3);
            }
        } else {
            res_sec.x0 = secure_required ? (uint64_t)ACS_STATUS_ERR :
                         (uint64_t)ACS_STATUS_SKIP;
            res_sec.x1 = FIRME_WORLD_MASK_SECURE;
            if (secure_required) {
                LOG_ERROR(NULL, "Check %u: required Secure world not available",
                          (unsigned)check_idx);
            } else {
                LOG_WARN(NULL, "Check %u: Secure world not available",
                         (unsigned)check_idx);
            }
        }
        if (!is_pass_or_skip(res_sec.x0)) {
            overall = res_sec;
            log_rule_status(tc->rule_str, &overall);
            print_test_end();
            update_counts(overall.x0, total, passed, failed, skipped);
            return;
        }
        check_idx++;
    } else if (secure_required) {
        res_sec.x0 = (uint64_t)ACS_STATUS_ERR;
        res_sec.x1 = FIRME_WORLD_MASK_SECURE;
        LOG_ERROR(NULL, "Check %u: required Secure body missing", (unsigned)check_idx);
        check_idx++;
    }

    /* Realm world (optional/required depending on registry flags). */
    if (tc->run_realm) {
        const uint64_t base_pa = val_firme_gpt_base_for_world(FIRME_ACS_WORLD_REALM);
        if (val_firme_world_call(FIRME_ACS_WORLD_REALM, FIRME_ACS_CMD_RUN_RULE,
                                 (uint64_t)tc->rule_id, base_pa, 0, 0, &res_realm) == 0) {
            LOG_TEST(NULL, "Check %u: %s ... %s", (unsigned)check_idx,
                     tc->check_realm ? tc->check_realm : "Realm caller",
                     status_str(res_realm.x0));
            if (!is_pass_or_skip(res_realm.x0)) {
                LOG_TEST(NULL, "  Debug: fail_step=%llu last_x0=%llx last_x1=%llx",
                         (unsigned long long)res_realm.x1,
                         (unsigned long long)res_realm.x2,
                         (unsigned long long)res_realm.x3);
            }
        } else {
            res_realm.x0 = realm_required ? (uint64_t)ACS_STATUS_ERR :
                           (uint64_t)ACS_STATUS_SKIP;
            res_realm.x1 = FIRME_WORLD_MASK_REALM;
            if (realm_required) {
                LOG_ERROR(NULL, "Check %u: required Realm world not available",
                          (unsigned)check_idx);
            } else {
                LOG_WARN(NULL, "Check %u: Realm world not available",
                         (unsigned)check_idx);
            }
        }
        if (!is_pass_or_skip(res_realm.x0)) {
            overall = res_realm;
            log_rule_status(tc->rule_str, &overall);
            print_test_end();
            update_counts(overall.x0, total, passed, failed, skipped);
            return;
        }
        check_idx++;
    } else if (realm_required) {
        res_realm.x0 = (uint64_t)ACS_STATUS_ERR;
        res_realm.x1 = FIRME_WORLD_MASK_REALM;
        LOG_ERROR(NULL, "Check %u: required Realm body missing", (unsigned)check_idx);
        check_idx++;
    }

    /* NS world body. */
    if (tc->run_ns) {
        firme_rule_res_t out = {0};
        const uint64_t base_pa = val_firme_gpt_base_for_world(FIRME_ACS_WORLD_NSEL2);

        tc->run_ns(base_pa, &out);

        res_ns.x0 = out.status;
        res_ns.x1 = out.fail_step;
        res_ns.x2 = out.last_x0;
        res_ns.x3 = out.last_x1;

        LOG_TEST(NULL, "Check %u: %s ... %s", (unsigned)check_idx,
                 tc->check_ns ? tc->check_ns : "NS caller",
                 status_str(res_ns.x0));
        if (!is_pass_or_skip(res_ns.x0)) {
            LOG_TEST(NULL, "  Debug: fail_step=%llu last_x0=%llx last_x1=%llx",
                     (unsigned long long)res_ns.x1,
                     (unsigned long long)res_ns.x2,
                     (unsigned long long)res_ns.x3);
        }
    } else {
        res_ns.x0 = ns_required ? (uint64_t)ACS_STATUS_ERR : (uint64_t)ACS_STATUS_SKIP;
        res_ns.x1 = FIRME_WORLD_MASK_NS;
        if (ns_required) {
            LOG_ERROR(NULL, "Check %u: required NS body missing", (unsigned)check_idx);
        } else {
            LOG_WARN(NULL, "Check %u: NS body missing", (unsigned)check_idx);
        }
    }

    /* Combine: fail if any required or executed world fails; otherwise prefer PASS over SKIP. */
    if (tc->run_secure || secure_required)
        combine_rule_result(&overall, &res_sec, &have_result);
    if (tc->run_realm || realm_required)
        combine_rule_result(&overall, &res_realm, &have_result);
    if (tc->run_ns || ns_required)
        combine_rule_result(&overall, &res_ns, &have_result);
    if (have_result == 0U)
        overall.x0 = (uint64_t)ACS_STATUS_SKIP;

    /* A required rule-body SKIP keeps the overall result from becoming PASS. */
    if (is_pass_or_skip(overall.x0)) {
        if (((tc->required_worlds_mask & FIRME_WORLD_MASK_SECURE) != 0U) &&
            (tc->run_secure || secure_required) &&
            (res_sec.x0 == (uint64_t)ACS_STATUS_SKIP))
            overall = res_sec;
        if (((tc->required_worlds_mask & FIRME_WORLD_MASK_REALM) != 0U) &&
            (tc->run_realm || realm_required) &&
            (res_realm.x0 == (uint64_t)ACS_STATUS_SKIP))
            overall = res_realm;
        if (((tc->required_worlds_mask & FIRME_WORLD_MASK_NS) != 0U) &&
            (tc->run_ns || ns_required) &&
            (res_ns.x0 == (uint64_t)ACS_STATUS_SKIP))
            overall = res_ns;
    }

    log_rule_status(tc->rule_str, &overall);
    print_test_end();
    update_counts(overall.x0, total, passed, failed, skipped);
}

/**
 *   @brief    - NS-EL2 dispatcher main: bring-up probes, then run rule tests.
 *   @return   - void
**/
void dispatcher_main(void)
{
    firme_res_t v = { 0 };
    firme_res_t feat = { 0 };
    uint32_t total = 0, passed = 0, failed = 0, skipped = 0;
    uint32_t count = 0;
    const firme_test_case_t *cases = 0;
    firme_chapter_id_t last_ch = 0;

#if ACS_PRINT_LEVEL <= 2
    smccc_res_t smc_res = {0};
    uint32_t src_dst = 0;
    firme_acs_call_res_t sec_local = {0};
    firme_acs_call_res_t *sec = &sec_local;
    firme_acs_call_res_t rr = { 0 };
#endif

    LOG_TEST(NULL, "");
    LOG_TEST(NULL, "[Init]");

    /* Baseline: invoke a FIRME ABI from NS-EL2 to validate EL3 routing. */
    LOG_TEST(NULL, "Call: FIRME_VERSION ...");
    val_firme_version(&v);
    LOG_TEST(NULL, "FIRME_VERSION (ns): %llx", (unsigned long long)v.x0);

    LOG_TEST(NULL, "Call: FIRME_FEATURES(reg0) ...");
    val_firme_features(/* feature register index */ 0, &feat);
    LOG_TEST(NULL, "FIRME_FEATURES(reg0) (ns): status=%llx value=%llx",
             (unsigned long long)feat.x0, (unsigned long long)feat.x1);

    /*
     * Optional transport diagnostics:
     * Keep this behind compile-time verbosity so normal regressions do not issue
     * extra direct-messaging calls into TF-A/SPMD.
     */
#if ACS_PRINT_LEVEL <= 2
    LOG_DEBUG(NULL, "Secure transport probe: VERSION...");
    val_smccc_smc_call(FFA_VERSION, FFA_MAKE_VERSION(1, 1), 0, 0, 0, 0, 0, 0, &smc_res);
    LOG_DEBUG(NULL, "Secure transport probe: VERSION -> x0=%llx", (unsigned long long)smc_res.x0);

    LOG_DEBUG(NULL, "Secure transport probe: ID_GET...");
    val_smccc_smc_call(FFA_ID_GET, 0, 0, 0, 0, 0, 0, 0, &smc_res);
    LOG_DEBUG(NULL, "Secure transport probe: ID_GET -> x0=%llx x2=%llx",
              (unsigned long long)smc_res.x0, (unsigned long long)smc_res.x2);

    LOG_DEBUG(NULL, "Secure transport probe: SPM_ID_GET...");
    val_smccc_smc_call(FFA_SPM_ID_GET, 0, 0, 0, 0, 0, 0, 0, &smc_res);
    LOG_DEBUG(NULL, "Secure transport probe: SPM_ID_GET -> x0=%llx x2=%llx",
              (unsigned long long)smc_res.x0, (unsigned long long)smc_res.x2);

    src_dst = ((uint32_t)PLAT_FFA_NS_ENDPOINT_ID << 16) | (uint32_t)PLAT_FFA_ACS_SECURE_ENDPOINT_ID;
    LOG_DEBUG(NULL, "Secure transport diag: direct req (SMC32) -> SPMC...");
    val_smccc_smc_call(FFA_MSG_SEND_DIRECT_REQ_SMC32, src_dst, 0,
                       (uint64_t)FIRME_ACS_CMD_PING, 0, 0, 0, 0, &smc_res);
    LOG_DEBUG(NULL, "Secure transport diag: direct resp x0=%llx x1=%llx x2=%llx x3=%llx x4=%llx",
              (unsigned long long)smc_res.x0, (unsigned long long)smc_res.x1,
              (unsigned long long)smc_res.x2, (unsigned long long)smc_res.x3,
              (unsigned long long)smc_res.x4);

    LOG_DEBUG(NULL, "calling secure: PING...");
    if (val_firme_world_call(FIRME_ACS_WORLD_SECURE, FIRME_ACS_CMD_PING, 0, 0, 0, 0, sec) == 0) {
        LOG_DEBUG(NULL, "secure ping status=%llx out0=%llx",
                  (unsigned long long)sec->x0, (unsigned long long)sec->x1);

        val_firme_world_call(FIRME_ACS_WORLD_SECURE, FIRME_ACS_CMD_RUN_DIAG, 0, 0, 0, 0, sec);
        LOG_DEBUG(NULL, "secure test0 status=%llx out0=%llx",
                  (unsigned long long)sec->x0, (unsigned long long)sec->x1);

        val_firme_world_call(FIRME_ACS_WORLD_SECURE, FIRME_ACS_CMD_RUN_DIAG,
                             /* test_id */ 1,
                             /* feature_reg_index */ 0, 0, 0, sec);
        LOG_DEBUG(NULL, "secure test1(FEATURES reg0) status=%llx out0=%llx out1=%llx",
                  (unsigned long long)sec->x0, (unsigned long long)sec->x1,
                  (unsigned long long)sec->x2);

        /*
         * SEL1 caller coverage: run a couple of Secure tests from S-EL1 while
         * still hosting the SPMC shim at S-EL2 (no packaged SPs required).
         */
        val_firme_world_call(FIRME_ACS_WORLD_SECURE, FIRME_ACS_CMD_RUN_DIAG_SEL1,
                             /* test_id */ 1,
                             /* feature_reg_index */ 0, 0, 0, sec);
        LOG_DEBUG(NULL, "secure(sel1) test1(FEATURES reg0) status=%llx out0=%llx out1=%llx",
                  (unsigned long long)sec->x0, (unsigned long long)sec->x1,
                  (unsigned long long)sec->x2);

        if (PLAT_FIRME_GPT_BASE_SECURE != 0ULL) {
            val_firme_world_call(FIRME_ACS_WORLD_SECURE, FIRME_ACS_CMD_RUN_RULE_SEL1,
                                 (uint64_t)FIRME_RULE_R0087, PLAT_FIRME_GPT_BASE_SECURE,
                                 0, 0, sec);
            LOG_DEBUG(NULL,
                      "secure(sel1) R0087 status=%llx fail_step=%llx last_x0=%llx last_x1=%llx",
                      (unsigned long long)sec->x0, (unsigned long long)sec->x1,
                      (unsigned long long)sec->x2, (unsigned long long)sec->x3);
        }
    } else {
        LOG_ERROR(NULL, "secure not available (SPMD/SPMC not wired)");
    }

    /* Realm (R-EL2 shim) path: forwarded via TF-A RMMD when ENABLE_RME=1. */
    if (val_firme_world_call(FIRME_ACS_WORLD_REALM, FIRME_ACS_CMD_RUN_DIAG, 0, 0, 0, 0, &rr) == 0) {
        LOG_DEBUG(NULL, "realm test0 status=%llx out0=%llx",
                  (unsigned long long)rr.x0, (unsigned long long)rr.x1);

        if (val_firme_world_call(FIRME_ACS_WORLD_REALM, FIRME_ACS_CMD_RUN_DIAG,
                                /* test_id */ 1,
                                /* feature_reg_index */ 0, 0, 0, &rr) == 0) {
            LOG_DEBUG(NULL, "realm test1(FEATURES reg0) status=%llx out0=%llx out1=%llx",
                      (unsigned long long)rr.x0, (unsigned long long)rr.x1,
                      (unsigned long long)rr.x2);
        }

    } else {
        LOG_ERROR(NULL, "realm not available (RMMD forwarding not wired)");
    }
#endif

    LOG_TEST(NULL, "");
    LOG_TEST(NULL, " Starting FIRME rule tests...");
    cases = val_firme_suite_cases(&count);
    last_ch = 0;
    for (uint32_t i = 0; i < count; i++) {
        const firme_test_case_t *tc = &cases[i];
        if (tc->chapter != last_ch) {
            last_ch = tc->chapter;
            LOG_TEST(NULL, "");
            LOG_TEST(NULL, "[Chapter] %s", val_firme_chapter_str(tc->chapter));
        }
        run_test_case(tc, &total, &passed, &failed, &skipped);
    }

    LOG_TEST(NULL, "");
    LOG_TEST(NULL, " *******************************************************");
    LOG_TEST(NULL, " ");
    LOG_TEST(NULL, "  -------------------------------------------------------------- ");
    LOG_TEST(NULL, " ");
    LOG_TEST(NULL, "  Total Tests run  = %u ; Tests Passed  = %u ; Tests Failed = %u",
             (unsigned int)total, (unsigned int)passed, (unsigned int)failed);
    if (skipped != 0u)
        LOG_TEST(NULL, "  Tests Skipped    = %u", (unsigned int)skipped);
    LOG_TEST(NULL, " ");
    LOG_TEST(NULL, "  -------------------------------------------------------------- ");
    LOG_TEST(NULL, " ");
    LOG_TEST(NULL, " ");
    LOG_TEST(NULL, " ********* FIRME tests complete. Reset the system *********");
}
