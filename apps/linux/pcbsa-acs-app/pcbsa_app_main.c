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


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include "pcbsa_app.h"
#include <getopt.h>

unsigned int *g_skip_test_num;
#define PC_BSA_LEVEL_PRINT_FORMAT(level, only) ((level > PCBSA_MAX_LEVEL_SUPPORTED) ? \
    ((only) != 0 ? "\n Starting tests for only level FR " : "\n Starting tests for level FR ") : \
    ((only) != 0 ? "\n Starting tests for only level %2d " : "\n Starting tests for level %2d "))



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

void print_help(void)
{
  printf("\nUsage: PC Bsa [-v <n>] | [-l <n>] | [--skip <n>]\n"
         "Options:\n"
         "-v      Verbosity of the Prints\n"
         "        1 shows all prints, 5 shows Errors\n"
         "-l      Level of compliance to be tested for\n"
         "        As per PC BSA spec only level 1\n"
         "--skip  Test(s) to be skipped\n"
         "        To skip a module, use Module_id as mentioned in user guide\n"
         "        To skip a particular test within a module, use the exact testcase number\n"
  );
}

int main(int argc, char **argv)
{

    int   c = 0, i = 0;
    char *endptr, *pt;
    int   status;
    int pcbsa_level = 1;
    int pcbsa_only_level = 0;
    unsigned int print_level = 3;
    const unsigned int num_skip = 3;

    struct option long_opt[] = {
      {"skip", required_argument, NULL, 'n'},
      {"help", no_argument, NULL, 'h'},
      {"only", no_argument, NULL, 'o'},
      {"fr", no_argument, NULL, 'r'},
      {NULL, 0, NULL, 0}
    };

    g_skip_test_num = (unsigned int *) malloc(num_skip * sizeof(unsigned int));

    /* Process Command Line arguments */
    while ((c = getopt_long(argc, argv, "hrv:l:oe:", long_opt, NULL)) != -1)
    {
       switch (c)
       {
       case 'v':
         print_level = strtol(optarg, &endptr, 10);
         break;
       case 'l':
         pcbsa_level = strtol(optarg, &endptr, 10);
         break;
       case 'o':
         pcbsa_only_level = 1;
         break;
       case 'r':
         pcbsa_level = 8;
         break;
       case 'h':
         print_help();
         return 1;
         break;
       case 'n':/*SKIP tests */
         pt = strtok(optarg, ",");
         while ((pt != NULL) && (i < num_skip)) {
           int a = atoi(pt);

           g_skip_test_num[i++] = a;
           pt = strtok(NULL, ",");
         }
         break;
       case '?':
         if (isprint (optopt))
           fprintf(stderr, "Unknown option `-%c'.\n", optopt);
         else
           fprintf(stderr,
                    "Unknown option character `\\x%x'.\n",
                    optopt);
         return 1;
       default:
         abort();
       }
    }


    printf("\n ************ PC BSA Architecture Compliance Suite *********\n");
    printf("                        Version %d.%d.%d\n", PC_BSA_ACS_MAJOR_VER,
            PC_BSA_ACS_MINOR_VER, PC_BSA_ACS_SUBMINOR_VER);


    printf(PC_BSA_LEVEL_PRINT_FORMAT(pcbsa_level, pcbsa_only_level),
                                   (pcbsa_level > PCBSA_MAX_LEVEL_SUPPORTED) ? 0 : pcbsa_level);

    printf("(Print level is %2d)\n\n", print_level);

    if (pcbsa_only_level)
        pcbsa_only_level = pcbsa_level;

    printf(" Gathering system information....\n");
    status = initialize_test_environment(print_level);
    if (status) {
        printf("Cannot initialize test environment. Exiting....\n");
        return 0;
    }

    execute_tests_pcie(1, pcbsa_level, print_level);

    printf("\n  ** For complete PCBSA test coverage, it is necessary to run the BSA test **\n");
    printf("\n                    *** PC BSA tests complete ***\n\n");

    cleanup_test_environment();

    return 0;
}
