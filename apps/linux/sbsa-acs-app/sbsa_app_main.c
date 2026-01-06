/** @file
 * Copyright (c) 2016-2025, Arm Limited or its affiliates. All rights reserved.
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


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include "include/sbsa_app.h"
#include "val/include/rule_based_execution_enum.h"
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>

#define RULE_ID_LIST_MAX 20u
#define LEVEL_PRINT_FORMAT(level, filter_mode, fr_level) ((filter_mode == LVL_FILTER_FR) ? \
    ((filter_mode == LVL_FILTER_ONLY && level == fr_level) ? \
    "\n Starting tests for only level FR " : "\n Starting tests for level FR ") : \
    ((filter_mode == LVL_FILTER_ONLY) ? \
    "\n Starting tests for only level %2d " : "\n Starting tests for level %2d "))

/* Global extern for rule ID string map (defined in val/src/rule_enum_string_map.c) */
extern char *rule_id_string[RULE_ID_SENTINEL];

/* Global variables for app */
int  g_print_level = 3;
bool g_pcie_skip_dp_nic_ms = 0;
uint32_t g_level_filter_mode = LVL_FILTER_MAX;  /* LEVEL_FILTER_MODE_e */
uint32_t g_level_value = SBSA_LEVEL_3;          /* Default SBSA level */

/* Legacy numeric skip support kept inert for compatibility with other files */
unsigned int  *g_skip_test_num;
unsigned int  g_num_skip = 3;
unsigned int  g_sw_view[3] = {1, 1, 1}; //Operating System, Hypervisor, Platform Security

/* New rule-based skip list parsed from -skip */
static RULE_ID_e g_skip_rule_buf[RULE_ID_LIST_MAX];
unsigned int g_skip_rule_count = 0;

/*  Helpers for rule parsing  */
static int rule_id_from_string(const char *tok)
{
    unsigned int rid;
    if (!tok || !*tok) return -1;
    for (rid = 0; rid < RULE_ID_SENTINEL; rid++) {
        if (rule_id_string[rid] && strcmp((const char *)rule_id_string[rid], tok) == 0)
            return (int)rid;
    }
    return -1;
}

static void skip_list_append(RULE_ID_e rid)
{
    if (g_skip_rule_count >= RULE_ID_LIST_MAX) {
        fprintf(stderr, "Warning: -skip list exceeds max %u; extra entries ignored\n",
                RULE_ID_LIST_MAX);
        return;
    }
    g_skip_rule_buf[g_skip_rule_count++] = rid;
}


int
initialize_test_environment(unsigned int print_level)
{
    return call_drv_init_test_env(print_level);
}

void
cleanup_test_environment()
{
    call_drv_clean_test_env();
}

void print_help(){
    printf ("\nUsage: Sbsa [-v <n>] | [-l <n>] | [--only <n>] | [-r <rule_id>[,<rule_id>...]] | "
        "[--fr] | [--skip <rule_id>[,<rule_id>...]]\n"
        "Options:\n"
        "-v      Verbosity of the Prints\n"
        "        1 shows all prints, 5 shows Errors\n"
        "-l      Level of compliance to be tested for\n"
        "        As per SBSA specification, valid levels are 3 to 7\n"
        "--only  To only run tests belonging to a specific level of compliance\n"
        "-r      Comma-separated rule IDs to run (overwrites default rule list) [no spaces]\n"
        "--fr    Run future requirement tests (FR); use without -l\n"
        "--skip  Rules to skip as comma-separated RULE IDs (e.g. B_PE_01,B_PE_02) [no spaces]\n"
        "--skip-dp-nic-ms Skip PCIe tests for DisplayPort, Network, and Mass Storage devices\n"
    );
}

int
main (int argc, char **argv)
{

    int   c = 0;
    char *endptr;
    int   status;
    struct option long_opt[] =
    {
      {"skip", required_argument, NULL, 'n'},
      {"skip-dp-nic-ms", no_argument, NULL, 'c'},
      {"help", no_argument, NULL, 'h'},
      {"only", required_argument, NULL, 'o'},
      {"fr", no_argument, NULL, 'f'},
      {"rules", required_argument, NULL, 'r'},
      {NULL, 0, NULL, 0}
    };

    /* Keep legacy array allocated and zeroed to avoid NULL deref in call_update_skip_list */
    g_skip_test_num = (unsigned int *) calloc(g_num_skip, sizeof(unsigned int));

    /* Process Command Line arguments */
    while ((c = getopt_long(argc, argv, "hfr:v:l:oc", long_opt, NULL)) != -1)
    {
       switch (c)
       {
       case 'v':
         g_print_level = strtol(optarg, &endptr, 10);
         break;
       case 'l':
         g_level_value =  strtol(optarg, &endptr, 10);
         g_level_filter_mode = LVL_FILTER_MAX;
         break;
       case 'o':
         g_level_value =  strtol(optarg, &endptr, 10);
         g_level_filter_mode = LVL_FILTER_ONLY;
         break;
       case 'f':
         g_level_filter_mode = LVL_FILTER_FR;
         g_level_value = 0;
         break;
       case 'r':
       {
         char *arg = strdup(optarg);
         char *saveptr = NULL;
         char *tok;
         unsigned int rule_count_tmp = 0;
         RULE_ID_e rule_buf_tmp[RULE_ID_LIST_MAX];
         if (!arg) { fprintf(stderr, "Error: no memory for -r\n"); break; }
         for (tok = strtok_r(arg, ",", &saveptr);
              tok != NULL;
              tok = strtok_r(NULL, ",", &saveptr)) {
            int rid = rule_id_from_string(tok);
            if (rid < 0) {
                fprintf(stderr, "Warning: unknown RULE ID '%s' in -r; ignoring\n", tok);
                continue;
            }
            if (rule_count_tmp >= RULE_ID_LIST_MAX) {
                fprintf(stderr, "Warning: -r list exceeds max %u; extra entries ignored\n",
                        RULE_ID_LIST_MAX);
                break;
            }
            rule_buf_tmp[rule_count_tmp++] = (RULE_ID_e)rid;
         }
         free(arg);
         if (rule_count_tmp > 0) {
             if (sbsa_send_array_u32(RULE_LIST, (const uint32_t *)rule_buf_tmp, rule_count_tmp)
                 != 0) {
                 fprintf(stderr, "Warning: failed to send rule list to driver\n");
             }
         }
         break;
       }
       case 'h':
         print_help();
         return 1;
         break;
       case 'c':
         g_pcie_skip_dp_nic_ms = 1;
         break;
       case 'n': /* --skip: parse comma-separated RULE IDs */
       {
         char *arg = strdup(optarg);
         char *saveptr = NULL;
         char *tok;
         if (!arg) { fprintf(stderr, "Error: no memory for -skip\n"); break; }
         for (tok = strtok_r(arg, ",", &saveptr);
              tok != NULL;
              tok = strtok_r(NULL, ",", &saveptr)) {
            int rid = rule_id_from_string(tok);
             if (rid < 0) {
                 fprintf(stderr, "Warning: unknown RULE ID '%s' in -skip; ignoring\n", tok);
                 continue;
             }
             skip_list_append((RULE_ID_e)rid);
          }
         free(arg);
         /* Send skip rule list to driver now that --skip parsing is complete */
         if (g_skip_rule_count > 0) {
             const uint32_t *u32_list = (const uint32_t *)&g_skip_rule_buf[0];
             if (sbsa_send_array_u32(SKIP_RULE_LIST, u32_list, g_skip_rule_count) != 0) {
                 fprintf(stderr, "Warning: failed to send skip rule list to driver\n");
             }
         }
         break;
       }
       case '?':
         if (isprint (optopt))
           fprintf (stderr, "Unknown option `-%c'.\n", optopt);
         else
           fprintf (stderr,
                    "Unknown option character `\\x%x'.\n",
                    optopt);
         return 1;
       default:
         abort ();
       }
    }

    printf("\n ************ SBSA Architecture Compliance Suite *********\n");
    printf("                        Version %d.%d.%d\n", SBSA_APP_VERSION_MAJOR,
            SBSA_APP_VERSION_MINOR, SBSA_APP_VERSION_SUBMINOR);

    printf(LEVEL_PRINT_FORMAT(g_level_value, g_level_filter_mode, SBSA_LEVEL_FR), g_level_value);

    printf("(Print level is %2d)\n\n", g_print_level);

    printf(" Gathering system information....\n");
    status = initialize_test_environment(g_print_level);
    if (status) {
        printf("Cannot initialize test environment. Exiting....\n");
        return 0;
    }

    /* Trigger rule-based run */
    call_drv_execute_test(RUN_TESTS, 0, 0, g_print_level, 0);
    (void)call_drv_wait_for_completion();

    printf("\n                    *** SBSA tests complete ***\n\n");
    cleanup_test_environment();
    return 0;
}
