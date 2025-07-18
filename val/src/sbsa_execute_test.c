/** @file
 * Copyright (c) 2024-2025, Arm Limited or its affiliates. All rights reserved.
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

#include "include/acs_val.h"
#include "include/acs_smmu.h"
#include "include/acs_pcie.h"
#include "include/acs_common.h"
#include "include/val_interface.h"
#include "include/acs_smmu.h"
#include "include/acs_pe.h"
#include "include/acs_memory.h"
#include "include/acs_gic.h"
#include "include/acs_wd.h"
#include "include/acs_exerciser.h"
#include "include/acs_mpam.h"
#include "include/acs_pmu.h"
#include "include/acs_ras.h"
#include "include/acs_nist.h"
#include "include/acs_ete.h"
#include "include/acs_timer.h"

extern uint32_t pcie_bdf_table_list_flag;
extern pcie_device_bdf_table *g_pcie_bdf_table;
extern uint32_t g_pcie_integrated_devices;
extern uint32_t g_its_init;

#ifndef TARGET_LINUX
/**
  @brief   This API will execute all PE tests designated for a given compliance level
           1. Caller       -  Application layer.
           2. Prerequisite -  val_pe_create_info_table, val_allocate_shared_mem
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_sbsa_pe_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_PASS, i;

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

  if (((level > 2) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 3)) {
      status |= pe023_entry(num_pe);
      status |= pe024_entry(num_pe);
      status |= pe025_entry(num_pe);
      status |= pe026_entry(num_pe);
  }

  if (((level > 3)  && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 4)) {
      status |= pe027_entry(num_pe);
      status |= pe028_entry(num_pe);
      status |= pe029_entry(num_pe);
      status |= pe030_entry(num_pe);
  }

  if (((level > 4)  && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 5)) {
      status |= pe031_entry(num_pe);
      status |= pe032_entry(num_pe);
      status |= pe033_entry(num_pe);
      status |= pe034_entry(num_pe);
      status |= pe035_entry(num_pe);
      status |= pe036_entry(num_pe);
  }

  if (((level > 5)  && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 6)) {
      status |= pe037_entry(num_pe);
      status |= pe038_entry(num_pe);
      status |= pe039_entry(num_pe);
      status |= pe040_entry(num_pe);
      status |= pe041_entry(num_pe);
      status |= pe042_entry(num_pe);
      status |= pe043_entry(num_pe);
      status |= pe044_entry(num_pe);
      status |= pe045_entry(num_pe);
      status |= pe046_entry(num_pe);
      status |= pe047_entry(num_pe);
  }

  if (((level > 6)  && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 7)) {
      status |= pe048_entry(num_pe);
      status |= pe049_entry(num_pe);
      status |= pe051_entry(num_pe);
      status |= pe052_entry(num_pe);
      status |= pe053_entry(num_pe);
      status |= pe054_entry(num_pe);
  }

  if (((level > 7)  && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 8)) {
      status |= pe057_entry(num_pe);
      status |= pe058_entry(num_pe);
      status |= pe059_entry(num_pe);
      status |= pe060_entry(num_pe);
      status |= pe061_entry(num_pe);
      status |= pe062_entry(num_pe);
   }

  val_print_test_end(status, "PE");

  return status;

}

/**
  @brief   This API executes all the GIC tests sequentially
           1. Caller       -  Application layer.
           2. Prerequisite -  val_gic_create_info_table()
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_sbsa_gic_execute_tests(uint32_t level, uint32_t num_pe)
{

  uint32_t status = 0, i, module_skip;

  if (!(((level > 2) && (g_sbsa_only_level == 0)) ||
            (g_sbsa_only_level == 3) || (g_sbsa_only_level == 5) || (g_sbsa_only_level == 8)))
      return ACS_STATUS_SKIP;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == ACS_GIC_TEST_NUM_BASE) {
          val_print(ACS_PRINT_INFO, "      USER Override - Skipping all GIC tests\n", 0);
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

  if (((level > 2) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 3)) {
      status |= g012_entry(num_pe);
      status |= g014_entry(num_pe);
  }

  if (((level > 4) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 5)) {
      status |= g013_entry(num_pe);
      status |= g016_entry(num_pe);
  }

  if (((level > 7) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 8))
      status |= g015_entry(num_pe);

  val_print_test_end(status, "GIC");

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
val_sbsa_wd_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_PASS, i;

  if (!(((level > 5) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 6)))
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

  if (((level > 5) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 6))
      status |= w003_entry(num_pe);

  val_print_test_end(status, "Watchdog");

  return status;
}

/**
  @brief   This API executes all the timer tests sequentially
           1. Caller       -  Application layer.
           2. Prerequisite -  val_timer_create_info_table
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_sbsa_timer_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_PASS, i;

  if (!(((level > 7) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 8)))
      return ACS_STATUS_SKIP;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == ACS_TIMER_TEST_NUM_BASE) {
          val_print(ACS_PRINT_INFO, "      USER Override - Skipping all Timer tests\n", 0);
          return ACS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  status = val_check_skip_module(ACS_TIMER_TEST_NUM_BASE);
  if (status) {
      val_print(ACS_PRINT_INFO, "\n USER Override - Skipping all Timer tests\n", 0);
      return ACS_STATUS_SKIP;
  }

  val_print_test_start("Timer");
  g_curr_module = 1 << TIMER_MODULE;

  if (((level > 7) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 8))
      status |= t006_entry(num_pe);

  val_print_test_end(status, "Timer");

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
val_sbsa_pcie_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_PASS, i;
  uint32_t ecam_status = ACS_STATUS_PASS;

#ifdef TARGET_LINUX
  if (!(((level > 2) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 3)
                     || (g_sbsa_only_level == 6)))
      return ACS_STATUS_SKIP;
#elif defined TARGET_EMULATION
  if (!(((level > 2) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 3)
                     || (g_sbsa_only_level == 4) || (g_sbsa_only_level >= 6)))
      return ACS_STATUS_SKIP;
#else
 if (!(((level > 2) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 4)
                    || (g_sbsa_only_level >= 6)))
      return ACS_STATUS_SKIP;
#endif

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

  #if defined(TARGET_LINUX) || defined(TARGET_EMULATION)
    if (((level > 2) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 3))
      status |= p046_entry(num_pe);  /* This covers GIC rule */
  #endif

  ecam_status = p043_entry(num_pe);
  if (ecam_status == ACS_STATUS_FAIL) {
    val_print(ACS_PRINT_WARN, "\n     *** Skipping remaining PCIE tests ***\n", 0);
    return status;
  }

  status |= ecam_status;

  #ifndef TARGET_LINUX
    if (((level > 2) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 3))
      status |= p068_entry(num_pe);

    if (((level > 3) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 4)) {
      status |= p087_entry(num_pe);
    }

    if (((level > 5) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 6)) {
      status |= p044_entry(num_pe);
    }

    if (((level > 6) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 7)) {
      status |= p086_entry(num_pe);
    }

    if (((level > 7) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 8)) {
      status |= p089_entry(num_pe);
      status |= p090_entry(num_pe);
      /* This test is for swtiches, Hence run before the other tests */
      status |= p093_entry(num_pe);
    }

  #endif

#if defined(TARGET_LINUX) || defined(TARGET_EMULATION)
  if (((level > 7) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 8))
    status |= p091_entry(num_pe);
#endif

  if (((level > 5) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 6)) {
    #if defined(TARGET_LINUX) || defined(TARGET_EMULATION)
      status |= p045_entry(num_pe);
    #endif

    if (g_pcie_integrated_devices == 0) {
      val_print(ACS_PRINT_WARN, "\n     *** No integrated PCIe Devices Found, \
                Skipping remaining PCIE tests ***\n", 0);
      return ACS_STATUS_SKIP;
    }

#ifndef TARGET_LINUX
      status |= p047_entry(num_pe);
      status |= p048_entry(num_pe);
      status |= p049_entry(num_pe);
      status |= p050_entry(num_pe); /* iEP/RP only */
      status |= p051_entry(num_pe);
      status |= p052_entry(num_pe);
      status |= p053_entry(num_pe);
      status |= p054_entry(num_pe);
      status |= p055_entry(num_pe);
      status |= p056_entry(num_pe);
      status |= p057_entry(num_pe);
      status |= p058_entry(num_pe);
      status |= p059_entry(num_pe);
      status |= p060_entry(num_pe);
      status |= p061_entry(num_pe);
      status |= p062_entry(num_pe);
      status |= p063_entry(num_pe);
      status |= p064_entry(num_pe); /* iEP/RP only */
      status |= p065_entry(num_pe); /* iEP/RP only */
      status |= p066_entry(num_pe); /* iEP/RP only */
      status |= p067_entry(num_pe); /* iEP/RP only */
      status |= p069_entry(num_pe);
      status |= p070_entry(num_pe);
      status |= p071_entry(num_pe); /* iEP/RP only */
      status |= p072_entry(num_pe); /* iEP/RP only */
      status |= p073_entry(num_pe); /* iEP/RP only */
      status |= p074_entry(num_pe);
      status |= p075_entry(num_pe); /* iEP/RP only */
#endif
#if defined(TARGET_EMULATION) && !defined(TARGET_LINUX)
      status |= p076_entry(num_pe); /* iEP/RP only */
      status |= p077_entry(num_pe);
#endif
#ifndef TARGET_LINUX
      status |= p078_entry(num_pe);
      status |= p079_entry(num_pe); /* iEP/RP only */
      status |= p080_entry(num_pe);
      status |= p081_entry(num_pe); /* iEP/RP only */
      status |= p082_entry(num_pe);
      status |= p083_entry(num_pe);
      status |= p084_entry(num_pe);
      status |= p085_entry(num_pe);
      status |= p088_entry(num_pe); /* iEP/RP only */
      status |= p092_entry(num_pe); /* iEP/RP only */
#endif
  }

  val_print_test_end(status, "PCIe");

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
val_sbsa_smmu_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_PASS, i;

  if (!(((level > 3) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level >= 4)))
      return ACS_STATUS_SKIP;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == ACS_SMMU_TEST_NUM_BASE) {
          val_print(ACS_PRINT_INFO, "      USER Override - Skipping all SMMU tests\n", 0);
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

#ifndef TARGET_LINUX
  if (((level > 3) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 4)) {
      status = i008_entry(num_pe);

      if (status != ACS_STATUS_PASS) {
         val_print(ACS_PRINT_WARN, "\n     SMMU Compatibility Check Failed, ", 0);
         val_print(ACS_PRINT_WARN, "Skipping SMMU tests...\n", 0);
         return ACS_STATUS_FAIL;
      }

      status |= i020_entry(num_pe);
  }

  if (((level > 4) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 5)) {
      status |= i009_entry(num_pe);
      status |= i010_entry(num_pe);
      status |= i011_entry(num_pe);
      status |= i012_entry(num_pe);
  }

  if (((level > 5) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 6)) {
      status |= i013_entry(num_pe);
      status |= i014_entry(num_pe);
      status |= i015_entry(num_pe);
      status |= i016_entry(num_pe);
      status |= i017_entry(num_pe);
      status |= i018_entry(num_pe);
      status |= i019_entry(num_pe);
      status |= i025_entry(num_pe);
  }

if (((level > 6) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 7)) {
     status |= i021_entry(num_pe);
     status |= i022_entry(num_pe);
  }

if (((level > 7) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 8))
     status |= i024_entry(num_pe);
#endif  // TARGET_LINUX

#if defined(TARGET_LINUX) || defined(TARGET_EMULATION)
  if (((level > 6) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 7))
     status |= i023_entry(num_pe);
#endif
  val_print_test_end(status, "SMMU");

  return status;
}

#ifndef TARGET_LINUX
/**
  @brief   This API will execute all Memory tests designated for a given compliance level
           1. Caller       -  Application layer.
           2. Prerequisite -  val_memory_create_info_table
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_sbsa_memory_execute_tests(uint32_t level, uint32_t num_pe)
{

  uint32_t status = 0, i;

  if (!(((level > 2) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 3)))
      return ACS_STATUS_SKIP;

  for (i = 0 ; i < g_num_skip ; i++) {
      if (g_skip_test_num[i] == ACS_MEMORY_MAP_TEST_NUM_BASE) {
          val_print(ACS_PRINT_INFO, "      USER Override - Skipping all memory tests\n", 0);
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

  if (((level > 2) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 3))
      status = m005_entry(num_pe);

  val_print_test_end(status, "Memory");

  return status;
}

/**
  @brief   This API executes all the Exerciser tests sequentially
           1. Caller       -  Application layer.
  @param   level  - level of compliance being tested for.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_sbsa_exerciser_execute_tests(uint32_t level)
{
  uint32_t status, i;
  uint32_t num_instances;
  uint32_t instance;
  uint32_t num_smmu;

  if (!(((level > 2) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 3)
         || (g_sbsa_only_level == 6) || (g_sbsa_only_level == 7) || (g_sbsa_only_level == 8)))
  return ACS_STATUS_SKIP;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == ACS_EXERCISER_TEST_NUM_BASE) {
          val_print(ACS_PRINT_INFO, "\n USER Override - Skipping the Exerciser tests\n", 0);
          return ACS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  status = val_check_skip_module(ACS_EXERCISER_TEST_NUM_BASE);
  if (status) {
    val_print(ACS_PRINT_INFO, "\n USER Override - Skipping all Exerciser tests\n", 0);
    return ACS_STATUS_SKIP;
  }

  if (val_pcie_create_device_bdf_table()) {
      val_print(ACS_PRINT_WARN, "\n     Create BDF Table Failed, Skipping Exerciser tests...\n", 0);
      return ACS_STATUS_SKIP;
  }

   if (pcie_bdf_table_list_flag == 1) {
    val_print(ACS_PRINT_WARN, "\n     *** Created device list with valid bdf doesn't match \
                with the platform pcie device hierarchy, Skipping exerciser tests ***\n", 0);
    return ACS_STATUS_SKIP;
  }

  val_print(ACS_PRINT_INFO, "\n      Starting Exerciser Setup\n", 0);

  val_exerciser_create_info_table();
  num_instances = val_exerciser_get_info(EXERCISER_NUM_CARDS);

  if (num_instances == 0) {
      val_print(ACS_PRINT_WARN,
                "\n     No Exerciser Devices Found, Skipping Exerciser tests...\n", 0);
      return ACS_STATUS_SKIP;
  }

  val_print(ACS_PRINT_INFO, "\n      Initializing SMMU\n", 0);
  num_smmu = val_iovirt_get_smmu_info(SMMU_NUM_CTRL, 0);
  val_smmu_init();

  /* Disable All SMMU's */
  for (instance = 0; instance < num_smmu; ++instance)
      val_smmu_disable(instance);

  if (g_its_init != 1) {
      val_print(ACS_PRINT_INFO, "\n      Initializing ITS\n", 0);
      val_gic_its_configure();
      g_its_init = 1;
  }

  val_print_test_start("PCIe Exerciser");

  g_curr_module = 1 << EXERCISER_MODULE;

  if (((level > 2) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 3))
      status = e018_entry();

  if (((level > 5) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 6)) {
      status |= e019_entry();
      status |= e020_entry();
      status |= e021_entry();
  }

  if (((level > 6) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 7)) {
      status |= e022_entry();
      status |= e023_entry();
      status |= e024_entry();
      status |= e025_entry();
  }

  if (((level > 7) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 8)) {
      status |= e026_entry();
      status |= e027_entry();
      status |= e028_entry();
      status |= e029_entry();
      status |= e030_entry();
  }

  val_print_test_end(status, "Exerciser");

  val_smmu_stop();

  return status;
}

/**
  @brief   This API executes all the PMU tests sequentially
           1. Caller       -  Application layer.
           2. Prerequisite -  val_pmu_create_info_table
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_sbsa_pmu_execute_tests(uint32_t level, uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;
  uint32_t skip_module;
  uint32_t i;

  if (!(((level > 6) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 7)))
      return ACS_STATUS_SKIP;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == ACS_PMU_TEST_NUM_BASE) {
          val_print(ACS_PRINT_INFO, "      USER Override - Skipping all PMU tests\n", 0);
          return ACS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  skip_module = val_check_skip_module(ACS_PMU_TEST_NUM_BASE);
  if (skip_module) {
      val_print(ACS_PRINT_INFO, "\n USER Override - Skipping all PMU tests\n", 0);
      return ACS_STATUS_SKIP;
  }

  /* check if PE supports PMU extension, else skip all PMU tests */
  if (val_pe_feat_check(PE_FEAT_PMU)) {
      val_print(ACS_PRINT_TEST,
                "\n       PE PMU extension unimplemented. Skipping all PMU tests\n", 0);
      return ACS_STATUS_SKIP;
  }

  val_print_test_start("PMU");
  g_curr_module = 1 << PMU_MODULE;

 if (((level > 6) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 7)) {
      /* run tests which don't check PMU nodes */
      status  = pmu001_entry(num_pe);
      status |= pmu002_entry(num_pe);
      status |= pmu003_entry(num_pe);
      status |= pmu004_entry(num_pe);
      status |= pmu005_entry(num_pe);
      status |= pmu006_entry(num_pe);
      status |= pmu007_entry(num_pe);
      status |= pmu009_entry(num_pe);
  }
  val_print_test_end(status, "PMU");

  return status;
}

/**
  @brief   This API executes all the MPAM tests sequentially
           1. Caller       -  Application layer.
           2. Prerequisite -  val_mpam_create_info_table
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_sbsa_mpam_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL, i;
  uint32_t skip_module;
  uint32_t msc_node_cnt;

  if (!(((level > 6) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 7)))
      return ACS_STATUS_SKIP;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == ACS_MPAM_TEST_NUM_BASE) {
          val_print(ACS_PRINT_INFO, "      USER Override - Skipping all MPAM tests\n", 0);
          return ACS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in the current module with user override*/
  skip_module = val_check_skip_module(ACS_MPAM_TEST_NUM_BASE);
  if (skip_module) {
      val_print(ACS_PRINT_INFO, "\n USER Override - Skipping all MPAM tests\n", 0);
      return ACS_STATUS_SKIP;
  }

  /* check if PE supports MPAM extension, else skip all MPAM tests */
  if (val_pe_feat_check(PE_FEAT_MPAM)) {
      val_print(ACS_PRINT_TEST,
                "\n       PE MPAM extension unimplemented. Skipping all MPAM tests\n", 0);
      return ACS_STATUS_SKIP;
  }

  val_print_test_start("MPAM");
  g_curr_module = 1 << MPAM_MODULE;

 if (((level > 6) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 7)) {
      /* run tests which don't check MPAM MSCs */
      status = mpam001_entry(num_pe);

      msc_node_cnt = val_mpam_get_msc_count();
      if (msc_node_cnt == 0) {
          val_print(ACS_PRINT_TEST,
                "\n       MPAM MSCs not found. Skipping remaining MPAM tests\n", 0);
          return ACS_STATUS_SKIP;
      }

      status |= mpam002_entry(num_pe);
      status |= mpam003_entry(num_pe);
      status |= mpam005_entry(num_pe);
      status |= mpam006_entry(num_pe);
      status |= mpam007_entry(num_pe);
  }
  val_print_test_end(status, "MPAM");

  return status;
}

/**
  @brief   This API executes all the RAS tests sequentially
           1. Caller       -  Application layer.
           2. Prerequisite -  val_ras_create_info_table
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_sbsa_ras_execute_tests(uint32_t level, uint32_t num_pe)
{

  uint32_t status, i;
  uint32_t skip_module;
  uint64_t num_ras_nodes = 0;

  if (!(((level > 5) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 6)
                     || (g_sbsa_only_level == 7) || (g_sbsa_only_level == 8)))
      return ACS_STATUS_SKIP;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == ACS_RAS_TEST_NUM_BASE) {
          val_print(ACS_PRINT_INFO, "\n      USER Override - Skipping all RAS tests\n", 0);
          return ACS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  skip_module = val_check_skip_module(ACS_RAS_TEST_NUM_BASE);
  if (skip_module) {
      val_print(ACS_PRINT_INFO, "\n USER Override - Skipping all RAS tests\n", 0);
      return ACS_STATUS_SKIP;
  }

  /* check if PE supports RAS extension, else skip all RAS tests */
  if (val_pe_feat_check(PE_FEAT_RAS)) {
      val_print(ACS_PRINT_TEST,
                "\n       PE RAS extension unimplemented. Skipping all RAS tests\n", 0);
      return ACS_STATUS_SKIP;
  }

  g_curr_module = 1 << RAS_MODULE;

  status = val_ras_get_info(RAS_INFO_NUM_NODES, 0, &num_ras_nodes);
  if (status || (num_ras_nodes == 0)) {
    val_print(ACS_PRINT_TEST, "\n       RAS nodes not found. Skipping all RAS tests\n", 0);
    return ACS_STATUS_SKIP;
  }

  val_print_test_start("RAS");

  if (((level > 5) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 6))
      status |= ras014_entry(num_pe);

  if (((level > 6) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 7)) {
      status |= ras001_entry(num_pe);
      status |= ras002_entry(num_pe);
      status |= ras003_entry(num_pe);
      status |= ras004_entry(num_pe);
      status |= ras005_entry(num_pe);
      status |= ras006_entry(num_pe);
      status |= ras007_entry(num_pe);
      status |= ras008_entry(num_pe);
      status |= ras009_entry(num_pe);
      status |= ras010_entry(num_pe);
      status |= ras011_entry(num_pe);
      status |= ras012_entry(num_pe);
  }

  if (((level > 7) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 8))
      status |= ras013_entry(num_pe);

  val_print_test_end(status, "RAS");

  return status;
}

uint32_t
val_sbsa_ete_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_PASS, i;
  uint32_t ete_status = ACS_STATUS_PASS;
  uint32_t trbe_status = ACS_STATUS_PASS;

  if (!(((level > 7) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 8)))
      return ACS_STATUS_SKIP;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == ACS_ETE_TEST_NUM_BASE) {
          val_print(ACS_PRINT_INFO, "      USER Override - Skipping all ETE tests \n", 0);
          return ACS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  status = val_check_skip_module(ACS_ETE_TEST_NUM_BASE);
  if (status) {
      val_print(ACS_PRINT_INFO, "\n USER Override - Skipping all ETE tests \n", 0);
      return ACS_STATUS_SKIP;
  }

  val_print_test_start("ETE");
  g_curr_module = 1 << ETE_MODULE;

  if (((level > 7) && (g_sbsa_only_level == 0)) || (g_sbsa_only_level == 8)) {
      ete_status = ete001_entry(num_pe);

      if (ete_status == ACS_STATUS_FAIL) {
          val_print(ACS_PRINT_ERR, "\n FEAT_ETE Not Supported, Skipping FEAT_ETE tests \n", 0);
      } else {
          ete_status |= ete002_entry(num_pe);
          ete_status |= ete003_entry(num_pe);
          ete_status |= ete004_entry(num_pe);
      }
      trbe_status = ete005_entry(num_pe);

      if (trbe_status == ACS_STATUS_FAIL) {
          val_print(ACS_PRINT_ERR, "\n FEAT_TRBE Not Supported, Skipping FEAT_TRBE tests \n", 0);
      } else {
          trbe_status |= ete006_entry(num_pe);
          trbe_status |= ete007_entry(num_pe);
          trbe_status |= ete008_entry(num_pe);
      }
  }

  val_print_test_end((ete_status | trbe_status), "ETE");

  return (ete_status | trbe_status);
}

#ifndef TARGET_BM_BOOT
/**
  @brief   This API executes all the PCIe tests sequentially
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.

  @return  Consolidated status of all the tests run.
**/
uint32_t
val_sbsa_nist_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status, i;
  (void) level;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == ACS_NIST_TEST_NUM_BASE) {
          val_print(ACS_PRINT_INFO, "      USER Override - Skipping all NIST tests\n", 0);
          return ACS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  status = val_check_skip_module(ACS_NIST_TEST_NUM_BASE);
  if (status) {
      val_print(ACS_PRINT_INFO, "\n USER Override - Skipping all NIST tests\n", 0);
      return ACS_STATUS_SKIP;
  }

  val_print_test_start("NIST");
  status = n001_entry(num_pe);

  val_print_test_end(status, "NIST");

  return status;
}

#endif


uint32_t
val_sbsa_execute_tests(uint32_t g_sbsa_level)
{

  uint32_t Status;
  /***         Starting PE tests                     ***/
  Status = val_sbsa_pe_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting Memory tests                 ***/
  Status |= val_sbsa_memory_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting GIC tests                    ***/
  Status |= val_sbsa_gic_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting SMMU tests                   ***/
  Status |= val_sbsa_smmu_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting Timer tests               ***/
  Status |= val_sbsa_timer_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting Watchdog tests               ***/
  Status |= val_sbsa_wd_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting PCIe tests                   ***/
  Status |= val_sbsa_pcie_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting Exerciser tests              ***/
  Status |= val_sbsa_exerciser_execute_tests(g_sbsa_level);

  /***         Starting MPAM tests                   ***/
  Status |= val_sbsa_mpam_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting PMU tests                    ***/
  Status |= val_sbsa_pmu_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting RAS tests                    ***/
  Status |= val_sbsa_ras_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting ETE tests                    ***/
  Status |= val_sbsa_ete_execute_tests(g_sbsa_level, val_pe_get_num());

  return Status;

}

#endif
