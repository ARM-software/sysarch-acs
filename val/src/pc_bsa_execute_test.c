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

#include "acs_val.h"
#include "acs_pcie.h"
#include "acs_common.h"
#include "val_interface.h"
#include "acs_smmu.h"
#include "acs_pe.h"
#include "acs_memory.h"
#include "acs_gic.h"
#include "acs_timer.h"
#include "acs_wd.h"
#include "acs_tpm.h"

extern uint32_t pcie_bdf_table_list_flag;

#ifndef TARGET_LINUX
uint32_t ecam_status = ACS_STATUS_FAIL;

/**
  @brief   This API will execute all PE tests designated for a given compliance level
           1. Caller       -  Application layer.
           2. Prerequisite -  val_pe_create_info_table, val_allocate_shared_mem
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_pcbsa_pe_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_PASS, i;

  if (!(((level >= 1) && (g_pcbsa_only_level == 0)) || (g_pcbsa_only_level == 1)))
      return ACS_STATUS_SKIP;


  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == ACS_PE_TEST_NUM_BASE) {
          val_print(ACS_PRINT_INFO, "\n USER Override - Skipping all PE tests\n", 0);
          return ACS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  status = val_check_skip_module(ACS_PE_TEST_NUM_BASE);
  if (status) {
      val_print(ACS_PRINT_INFO, "\n USER Override - Skipping all PE tests\n", 0);
      return ACS_STATUS_SKIP;
  }

  val_print_test_start("PE");
  g_curr_module = 1 << PE_MODULE;

  if (((level >= 1) && (g_pcbsa_only_level == 0)) || (g_pcbsa_only_level == 1)) {
      status |= pe004_entry(num_pe);
      status |= pe018_entry(num_pe);
      status |= pe024_entry(num_pe);
      status |= pe025_entry(num_pe);
      status |= pe026_entry(num_pe);
      status |= pe028_entry(num_pe);
      status |= pe029_entry(num_pe);
      status |= pe030_entry(num_pe);
      status |= pe015_entry(num_pe);
  }

  val_print_test_end(status, "PE");

  return status;

}

/**
  @brief   This API executes all the GIC tests sequentially
           1. Caller       -  Application layer.
           2. Prerequisite -  val_gic_create_info_table(), val_drtm_create_info_table()
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_pcbsa_gic_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_PASS, i, module_skip;

  if (!(((level >= 1) && (g_pcbsa_only_level == 0)) || (g_pcbsa_only_level == 1)))
      return ACS_STATUS_SKIP;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == ACS_GIC_TEST_NUM_BASE) {
          val_print(ACS_PRINT_INFO, "\n USER Override - Skipping all GIC tests\n", 0);
          return ACS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  module_skip = val_check_skip_module(ACS_GIC_TEST_NUM_BASE);
  if (module_skip) {
      val_print(ACS_PRINT_INFO, "\n USER Override - Skipping all GIC tests\n", 0);
      return ACS_STATUS_SKIP;
  }

  val_print_test_start("GIC");
  g_curr_module = 1 << GIC_MODULE;

  val_print(ACS_PRINT_INFO, "  Initializing ITS\n", 0);
  val_gic_its_configure();

  if (((level >= 1) && (g_pcbsa_only_level == 0)) || (g_pcbsa_only_level == 1)) {
      status = g012_entry(num_pe);
      status |= g014_entry(num_pe);
      /* Making num_pe as 1 for interface tests */
      num_pe = 1;

      status |= interface010_entry(num_pe);
      status |= interface011_entry(num_pe);
  }
  val_print_test_end(status, "GIC");

  return status;

}

/**
  @brief   This API executes all the SMMU tests sequentially
           1. Caller       -  Application layer.
           2. Prerequisite -  val_smmu_create_info_table()
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_pcbsa_smmu_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_PASS, i;

  if (!(((level >= 1) && (g_pcbsa_only_level == 0)) || (g_pcbsa_only_level == 1)))
      return ACS_STATUS_SKIP;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == ACS_SMMU_TEST_NUM_BASE) {
          val_print(ACS_PRINT_INFO, "\n USER Override - Skipping all SMMU tests\n", 0);
          return ACS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  status = val_check_skip_module(ACS_SMMU_TEST_NUM_BASE);
  if (status) {
      val_print(ACS_PRINT_INFO, "\n USER Override - Skipping all SMMU tests\n", 0);
      return ACS_STATUS_SKIP;
  }

  val_print_test_start("SMMU");
  g_curr_module = 1 << SMMU_MODULE;

  if (((level >= 1) && (g_pcbsa_only_level == 0)) || (g_pcbsa_only_level == 1)) {
      status = i008_entry(num_pe);
      status |= i025_entry(num_pe);

      if (status != ACS_STATUS_PASS) {
         val_print(ACS_PRINT_WARN, "\n     SMMU Compatibility Check Failed, ", 0);
         val_print(ACS_PRINT_WARN, "Skipping SMMU tests...\n", 0);
         val_print_test_end(status, "SMMU");
         return ACS_STATUS_FAIL;
      }

      status |= i020_entry(num_pe);
      status |= i022_entry(num_pe);
  }

  val_print_test_end(status, "SMMU");

  return status;
}

/**
  @brief   This API will execute all Memory tests designated for a given compliance level
           1. Caller       -  Application layer.
           2. Prerequisite -  val_memory_create_info_table
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_pcbsa_memory_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_PASS, i;

  if (!(((level >= 1) && (g_pcbsa_only_level == 0)) || (g_pcbsa_only_level == 1)))
      return ACS_STATUS_SKIP;

  for (i = 0 ; i < g_num_skip ; i++) {
      if (g_skip_test_num[i] == ACS_MEMORY_MAP_TEST_NUM_BASE) {
          val_print(ACS_PRINT_INFO, "\n USER Override - Skipping all memory tests\n", 0);
          return ACS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in the current module with user override*/
  status = val_check_skip_module(ACS_MEMORY_MAP_TEST_NUM_BASE);
  if (status) {
      val_print(ACS_PRINT_INFO, "\n USER Override - Skipping all memory tests\n", 0);
      return ACS_STATUS_SKIP;
  }

  val_print_test_start("Memory");
  g_curr_module = 1 << MEM_MAP_MODULE;

  if (((level >= 1) && (g_pcbsa_only_level == 0)) || (g_pcbsa_only_level == 1)) {
      status = m005_entry(num_pe);
      status |= m008_entry(num_pe);
  }

  val_print_test_end(status, "Memory");

  return status;
}

/**
  @brief   This API executes all the Watchdog tests sequentially
           1. Caller       -  Application layer.
           2. Prerequisite -  val_wd_create_info_table
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_pcbsa_wd_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_PASS, i;

  if (!(((level >= 2) && (g_pcbsa_only_level == 0)) || (g_pcbsa_only_level == 2)))
      return ACS_STATUS_SKIP;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == ACS_WD_TEST_NUM_BASE) {
          val_print(ACS_PRINT_INFO, "      USER Override - Skipping all Watchdog tests\n", 0);
          return ACS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  status = val_check_skip_module(ACS_WD_TEST_NUM_BASE);
  if (status) {
      val_print(ACS_PRINT_INFO, "\n USER Override - Skipping all Watchdog tests\n", 0);
      return ACS_STATUS_SKIP;
  }

  val_print_test_start("Watchdog");
  g_curr_module = 1 << WD_MODULE;

  if (((level >= 2) && (g_pcbsa_only_level == 0)) || (g_pcbsa_only_level == 2)) {
      status |= w001_entry(num_pe);
      status |= w002_entry(num_pe);
  }

  val_print_test_end(status, "Watchdog");

  return status;
}


/**
  @brief   This API executes all the Tpm2 tests sequentially
           1. Caller       -  Application layer.
           2. Prerequisite -  val_tpm2_create_info_table
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_pcbsa_tpm2_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_PASS, i;

  if (!(((level >= 1) && (g_pcbsa_only_level == 0)) || (g_pcbsa_only_level == 1)))
      return ACS_STATUS_SKIP;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == ACS_TPM2_TEST_NUM_BASE) {
          val_print(ACS_PRINT_INFO, "      USER Override - Skipping all TPM2 tests\n", 0);
          return ACS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  status = val_check_skip_module(ACS_TPM2_TEST_NUM_BASE);
  if (status) {
      val_print(ACS_PRINT_INFO, "\n USER Override - Skipping all TPM2 tests\n", 0);
      return ACS_STATUS_SKIP;
  }

  val_print_test_start("TPM2");
  g_curr_module = 1 << TPM2_MODULE;

  if (((level >= 1) && (g_pcbsa_only_level == 0)) || (g_pcbsa_only_level == 1)) {
      status |= tpm001_entry(num_pe);
      status |= tpm002_entry(num_pe);
  }

  val_print_test_end(status, "TPM2");

  return status;
}

#endif

/**
  @brief   This API executes all the PCIe tests sequentially
           1. Caller       -  Application layer.
           2. Prerequisite -  val_pcie_create_info_table()
  @param   level       - level of compliance being tested for.
  @param   num_pe      - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_pcbsa_pcie_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_PASS, i;

  if (!(((level >= 1) && (g_pcbsa_only_level == 0)) || (g_pcbsa_only_level == 1)))
      return ACS_STATUS_SKIP;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == ACS_PCIE_TEST_NUM_BASE) {
          val_print(ACS_PRINT_INFO, "\n USER Override - Skipping all PCIe tests\n", 0);
          return ACS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  status = val_check_skip_module(ACS_PCIE_TEST_NUM_BASE);
  if (status) {
      val_print(ACS_PRINT_INFO, "\n USER Override - Skipping all PCIe tests\n", 0);
      return ACS_STATUS_SKIP;
  }

  if (pcie_bdf_table_list_flag == 1) {
    val_print(ACS_PRINT_WARN, "\n     *** Created device list with valid bdf doesn't match \
                    with the platform pcie device hierarchy, Skipping PCIE tests ***\n", 0);
    return ACS_STATUS_SKIP;
  }

  val_print_test_start("PCIe");
  g_curr_module = 1 << PCIE_MODULE;

#if defined(TARGET_LINUX)
    if (((level >= 1) && (g_pcbsa_only_level == 0)) || (g_pcbsa_only_level == 1))
      status |= p046_entry(num_pe);  /* This covers GIC rule */
#else

  ecam_status = p001_entry(num_pe);
  status |= ecam_status;
  if (ecam_status == ACS_STATUS_FAIL) {
    val_print(ACS_PRINT_WARN, "\n     *** Skipping remaining PCIE tests ***\n", 0);
    val_print_test_end(status, "PCIe");
    return status;
  }

  if (((level >= 1) && (g_pcbsa_only_level == 0)) || (g_pcbsa_only_level == 1)) {
    /* Only the test p087 will be run with the test number (ACS_PER_TEST_NUM_BASE + 1) */
      status |= p087_entry(num_pe);
  }
#endif

  val_print_test_end(status, "PCIe");

  return status;

}

#ifndef TARGET_LINUX

uint32_t
val_pcbsa_execute_tests(uint32_t g_pcbsa_level)
{

  uint32_t Status;
  /***         Starting PE tests                     ***/
  Status = val_pcbsa_pe_execute_tests(g_pcbsa_level, val_pe_get_num());

  /***         Starting Memory tests                ***/
  Status |= val_pcbsa_memory_execute_tests(g_pcbsa_level, val_pe_get_num());

  /***         Starting GIC tests                    ***/
  Status |= val_pcbsa_gic_execute_tests(g_pcbsa_level, val_pe_get_num());

  /***         Starting SMMU tests                   ***/
  Status |= val_pcbsa_smmu_execute_tests(g_pcbsa_level, val_pe_get_num());

  /***         Starting PCIe tests                   ***/
  Status |= val_pcbsa_pcie_execute_tests(g_pcbsa_level, val_pe_get_num());

  /***         Starting Watchdog tests               ***/
  Status |= val_pcbsa_wd_execute_tests(g_pcbsa_level, val_pe_get_num());

  /***         Starting TPM2 tests               ***/
  Status |= val_pcbsa_tpm2_execute_tests(g_pcbsa_level, val_pe_get_num());

  return Status;

}

#endif
