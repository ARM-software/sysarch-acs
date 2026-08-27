/** @file
 * Copyright (c) 2025-2026, Arm Limited or its affiliates. All rights reserved.
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
 **/

#include "val_interface.h"
#include "acs_common.h"
#include "pal_interface.h"
#include "acs_std_smc.h"
#include "acs_cfg.h"

/**
  @brief   This API will execute all MPAM Error tests
           1. Caller       -  Application layer.
           2. Prerequisite
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_mpam_execute_error_tests(uint32_t num_pe)
{
  uint32_t status, i;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == ACS_MPAM_ERROR_TEST_NUM_BASE) {
        val_print(TRACE, "\n       USER Override - Skipping all ERROR tests\n");
        return ACS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  status = val_check_skip_module(ACS_MPAM_ERROR_TEST_NUM_BASE);
  if (status) {
      val_print(TRACE, "\n       USER Override - Skipping all ERROR tests\n");
      return ACS_STATUS_SKIP;
  }

  status = ACS_STATUS_PASS;

  val_print_test_start("ERROR");
  g_curr_module = 1 << ERROR_MODULE;

  status = error001_entry(num_pe);
  status |= error002_entry(num_pe);
  status |= error003_entry(num_pe);
  status |= error004_entry(num_pe);
  status |= error005_entry(num_pe);
  status |= error006_entry(num_pe);
  status |= error007_entry(num_pe);
  status |= error008_entry(num_pe);
  status |= error009_entry(num_pe);
  status |= error010_entry(num_pe);
  status |= error011_entry(num_pe);
  status |= error012_entry(num_pe);
  status |= error013_entry(num_pe);
  status |= error014_entry(num_pe);
  status |= intr001_entry(num_pe);
  status |= intr002_entry(num_pe);
  status |= intr003_entry(num_pe);
  status |= intr004_entry(num_pe);

  /* Setup ITS for MSI Tests */
  if (g_its_init != 1) {
      val_print(TRACE, "\n       Initializing ITS");
      if (val_gic_its_configure() == ACS_STATUS_ERR) {
          val_print(ERROR, "\n       val_gic_its_configure() failed");
          status = ACS_STATUS_SKIP;
          return status;
      }
      g_its_init = 1;
  }

  if (g_its_init) {
    status |= intr005_entry(num_pe);
    status |= intr006_entry(num_pe);
  }

  val_print_test_end(status, "ERROR");

  return status;

}

/**
  @brief   This API will execute all MPAM Memory Bandwidth tests
           1. Caller       -  Application layer.
           2. Prerequisite
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_mpam_execute_membw_tests(uint32_t num_pe)
{
  uint32_t status, i;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == ACS_MPAM_MEMORY_TEST_NUM_BASE) {
        val_print(TRACE,
                             "\n       USER Override - Skipping all Memory Bandwidth tests\n");
        return ACS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  status = val_check_skip_module(ACS_MPAM_MEMORY_TEST_NUM_BASE);
  if (status) {
      val_print(TRACE,
                             "\n       USER Override - Skipping all Memory Bandwidth tests\n");
      return ACS_STATUS_SKIP;
  }

  status = ACS_STATUS_PASS;

  val_print_test_start("MEMORY BANDWIDTH");
  g_curr_module = 1 << MEMORY_MODULE;

  status = mem001_entry(num_pe);
  status |= mem002_entry(num_pe);
  status |= mem003_entry(num_pe);

  status |= monitor006_entry(num_pe);
  status |= monitor007_entry(num_pe);
  status |= monitor008_entry(num_pe);

  val_print_test_end(status, "MEMORY BANDWIDTH");

  return status;

}

/**
  @brief   This API will execute all MPAM Register tests
           1. Caller       -  Application layer.
           2. Prerequisite
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_mpam_execute_register_tests(uint32_t num_pe)
{
  uint32_t status, i;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == ACS_MPAM_REGISTER_TEST_NUM_BASE) {
        val_print(TRACE, "\n       USER Override - Skipping all Register tests\n");
        return ACS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  status = val_check_skip_module(ACS_MPAM_REGISTER_TEST_NUM_BASE);
  if (status) {
      val_print(TRACE, "\n       USER Override - Skipping all Register tests\n");
      return ACS_STATUS_SKIP;
  }

  status = ACS_STATUS_PASS;

  val_print_test_start("REGISTER");
  g_curr_module = 1 << REGISTER_MODULE;

  status |= reg001_entry(num_pe);
  status |= reg002_entry(num_pe);
  status |= reg003_entry(num_pe);
  status |= reg004_entry(num_pe);
  status |= reg005_entry(num_pe);
  status |= reg006_entry(num_pe);

  val_print_test_end(status, "REGISTER");

  return status;
}


/**
  @brief   This API will execute all MPAM Cache tests
           1. Caller       -  Application layer.
           2. Prerequisite
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_mpam_execute_cache_tests(uint32_t num_pe)
{
  uint32_t status, i;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == ACS_MPAM_CACHE_TEST_NUM_BASE) {
        val_print(TRACE, "\n       USER Override - Skipping all CACHE tests\n");
        return ACS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  status = val_check_skip_module(ACS_MPAM_CACHE_TEST_NUM_BASE);
  if (status) {
      val_print(TRACE, "\n       USER Override - Skipping all CACHE tests\n");
      return ACS_STATUS_SKIP;
  }

  status = ACS_STATUS_PASS;

  val_print_test_start("CACHE");
  g_curr_module = 1 << CACHE_MODULE;

  status |= partition001_entry(num_pe);
  status |= partition002_entry(num_pe);
  status |= partition003_entry(num_pe);
  status |= partition004_entry(num_pe);
  status |= partition005_entry(num_pe);
  status |= partition006_entry(num_pe);

  status |= feat001_entry(num_pe);

  status |= monitor001_entry(num_pe);
  status |= monitor002_entry(num_pe);
  status |= monitor003_entry(num_pe);
  status |= monitor004_entry(num_pe);
  status |= monitor005_entry(num_pe);

  val_print_test_end(status, "CACHE");

  return status;
}
