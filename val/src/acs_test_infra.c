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

#include "include/acs_val.h"
#include "include/acs_pe.h"
#include "include/acs_common.h"
#include "driver/gic/acs_exception.h"
#include "include/pal_interface.h"
#include "include/val_interface.h"

uint32_t g_override_skip;

/**
  @brief  This API calls PAL layer to print a formatted string
          to the output console.
          1. Caller       - Application layer
          2. Prerequisite - None.

  @param level   the print verbosity (1 to 5)
  @param string  formatted ASCII string
  @param data    64-bit data. set to 0 if no data is to sent to console.

  @return        None
 **/
void
val_print(uint32_t level, char8_t *string, uint64_t data)
{
  if (level >= g_print_level)
      pal_print(string, data);
}


/**
  @brief  Print standardized log context prefix.
          1. Caller       - Application/VAL layers
          2. Prerequisite - None

  @param file   source file name (typically __FILE__)
  @param func   function name (typically __func__)
  @param line   source line number (typically __LINE__)

  @return None
 **/
void
val_log_context(char8_t *file, char8_t *func, uint32_t line)
{
  char8_t *trimmed_file;
  char8_t *marker;
  /* Substring to locate in full file path */
  const char8_t pattern[] = "test_pool";
  uint32_t i;

  trimmed_file = file;
  marker = file;

  /* Scan file path for pattern and trim from first occurrence */
  while (*marker != 0) {
      for (i = 0; (pattern[i] != 0) && (marker[i] == pattern[i]); i++) {
          /* Intentionally empty */
      }
      if (pattern[i] == 0) {
          trimmed_file = marker;
          break;
      }
      marker++;
  }

  val_print(ACS_PRINT_TEST, "\n    ", 0);
  val_print(ACS_PRINT_TEST, trimmed_file, 0);
  val_print(ACS_PRINT_TEST, ":", 0);
  val_print(ACS_PRINT_TEST, "%d", line);
  val_print(ACS_PRINT_TEST, " ", 0);
  val_print(ACS_PRINT_TEST, func, 0);
}

/**
  @brief  This API calls val_print API to print a formatted string
          to the output console if current PE index is Primary PE index .
          1. Caller       - Application layer
          2. Prerequisite - None.

  @param level   the print verbosity (1 to 5)
  @param string  formatted ASCII string
  @param data    64-bit data. set to 0 if no data is to sent to console.
  @param index   the index of the PE

  @return        None
 **/
void val_print_primary_pe(uint32_t level, char8_t *string, uint64_t data, uint32_t index)
{

  if (index == val_pe_get_primary_index())
      val_print(level, string, data);
}

/**
  @brief  This API prints out module header to the output console.
          1. Caller       - Application layer
          2. Prerequisite - None.

  @param string  formatted ASCII string

  @return        None
 **/
void
val_print_test_start(char8_t *string)
{
  val_print(ACS_PRINT_TEST, "\n      *** Starting ", 0);
  val_print(ACS_PRINT_TEST, string, 0);
  val_print(ACS_PRINT_TEST, " tests ***\n", 0);
}

/**
  @brief  This API calls PAL layer to print tests status
          to the output console.
          1. Caller       - Application layer
          2. Prerequisite - None.

  @param status the status of the tests
  @param string formatted ASCII string

  @return        None
 **/
void
val_print_test_end(uint32_t status, char8_t *string)
{
  val_print(ACS_PRINT_TEST, "\n      ", 0);

  if (status != ACS_STATUS_PASS) {
      val_print(ACS_PRINT_TEST, "One or more ", 0);
      val_print(ACS_PRINT_TEST, string, 0);
      val_print(ACS_PRINT_TEST, " tests failed or were skipped.", 0);
  }
  else {
      val_print(ACS_PRINT_TEST, "All ", 0);
      val_print(ACS_PRINT_TEST, string, 0);
      val_print(ACS_PRINT_TEST, " tests passed.", 0);
  }

  val_print(ACS_PRINT_TEST, "\n", 0);

}

/**
  @brief  Print consolidated ACS test status summary from global counters.
          Only top-level rule results are counted in these counters.
          1. Caller       - Application/VAL layers
          2. Prerequisite - Counters updated via print_rule_test_status()

  @return None
 **/
void
val_print_acs_test_status_summary(void)
{
  val_print(ACS_PRINT_TEST, "\n---------- ACS Summary ----------\n", 0);
  val_print(ACS_PRINT_TEST, "   Total Rules Run        : %d\n",
            g_rule_test_stats.total_rules_run);
  val_print(ACS_PRINT_TEST, "   Passed                 : %d\n", g_rule_test_stats.passed);
  val_print(ACS_PRINT_TEST, "   Passed (*Partial)      : %d\n",
            g_rule_test_stats.partial_coverage);
  val_print(ACS_PRINT_TEST, "   Warnings               : %d\n", g_rule_test_stats.warnings);
  val_print(ACS_PRINT_TEST, "   Skipped                : %d\n", g_rule_test_stats.skipped);
  val_print(ACS_PRINT_TEST, "   Failed                 : %d\n", g_rule_test_stats.failed);
  val_print(ACS_PRINT_TEST, "   PAL Not Supported      : %d\n",
            g_rule_test_stats.pal_not_supported);
  val_print(ACS_PRINT_TEST, "   Test Not Implemented   : %d\n",
            g_rule_test_stats.not_implemented);
  val_print(ACS_PRINT_TEST, "---------------------------------\n", 0);

  /* Reset global rule/test status counters after printing summary */
  g_rule_test_stats.total_rules_run = 0;
  g_rule_test_stats.passed = 0;
  g_rule_test_stats.partial_coverage = 0;
  g_rule_test_stats.warnings = 0;
  g_rule_test_stats.skipped = 0;
  g_rule_test_stats.failed = 0;
  g_rule_test_stats.pal_not_supported = 0;
  g_rule_test_stats.not_implemented = 0;

}

/**
  @brief  This API calls PAL layer to print a string to the output console.
          1. Caller       - Application layer
          2. Prerequisite - None.

  @param uart_address  Address of UART to be used
  @param level   the print verbosity (1 to 5)
  @param string  formatted ASCII string
  @param data    64-bit data. set to 0 if no data is to sent to console.

  @return        None
 **/
void
val_print_raw(uint64_t uart_address, uint32_t level, char8_t *string,
                                                                uint64_t data)
{

  if (level >= g_print_level){
      pal_print_raw(uart_address, string, data);
  }

}

/**
  @brief  This API calls PAL layer to read from a Memory address
          and return 8-bit data.
          1. Caller       - Test Suite
          2. Prerequisite - None.

  @param addr   64-bit address

  @return       8-bits of data
 **/
uint8_t
val_mmio_read8(addr_t addr)
{
  return pal_mmio_read8(addr);

}

/**
  @brief  This API calls PAL layer to read from a Memory address
          and return 16-bit data.
          1. Caller       - Test Suite
          2. Prerequisite - None.

  @param addr   64-bit address

  @return       16-bits of data
 **/
uint16_t
val_mmio_read16(addr_t addr)
{
  return pal_mmio_read16(addr);

}

/**
  @brief  This API calls PAL layer to read from a Memory address
          and return 32-bit data.
          1. Caller       - Test Suite
          2. Prerequisite - None.

  @param addr   64-bit address

  @return       32-bits of data
 **/
uint32_t
val_mmio_read(addr_t addr)
{
  return pal_mmio_read(addr);

}

/**
  @brief  This API calls PAL layer to read from a Memory address
          and return 64-bit data.
          1. Caller       - Test Suite
          2. Prerequisite - None.

  @param addr   64-bit address

  @return       64-bits of data
 **/
uint64_t
val_mmio_read64(addr_t addr)
{
  return pal_mmio_read64(addr);

}

/**
  @brief  This function will call PAL layer to write 8-bit data to
          a Memory address.
        1. Caller       - Test Suite
        2. Prerequisite - None.

  @param addr   64-bit address
  @param data   8-bit data

  @return       None
 **/
void
val_mmio_write8(addr_t addr, uint8_t data)
{

  pal_mmio_write8(addr, data);
}

/**
  @brief  This function will call PAL layer to write 16-bit data to
          a Memory address.
        1. Caller       - Test Suite
        2. Prerequisite - None.

  @param addr   64-bit address
  @param data   16-bit data

  @return       None
 **/
void
val_mmio_write16(addr_t addr, uint16_t data)
{

  pal_mmio_write16(addr, data);
}

/**
  @brief  This function will call PAL layer to write 32-bit data to
          a Memory address.
        1. Caller       - Test Suite
        2. Prerequisite - None.

  @param addr   64-bit address
  @param data   32-bit data

  @return       None
 **/
void
val_mmio_write(addr_t addr, uint32_t data)
{

  pal_mmio_write(addr, data);
}
/**
  @brief  This function will call PAL layer to write 64-bit data to
          a Memory address.
        1. Caller       - Test Suite
        2. Prerequisite - None.

  @param addr   64-bit address
  @param data   64-bit data

  @return       None
 **/
void
val_mmio_write64(addr_t addr, uint64_t data)
{

  pal_mmio_write64(addr, data);
}

/**
  @brief  This API checks if all the tests in the current module needs to be skipped.
          Skip if no tests are to be executed with user override options.
          1. Caller       - Test suite
          2. Prerequisite - None.

  @param module_base Base number of the module

  @return         ACS_STATUS_SKIP - if the user override has no tests to run in the current module
                  ACS_STATUS_PASS - if tests are to be run in the current module
 **/
#ifndef COMPILE_RB_EXE
uint32_t
val_check_skip_module(uint32_t module_base)
{
  uint32_t i, skip_module = 0;

  /* Case 1 - Don't skip the module if the module number is mentioned in -m option parameters */
  for (i = 0; i < g_num_modules; i++) {
      if (g_execute_modules[i] == module_base) {
          skip_module++;
      }
  }

  /* Case 2 - Don't skip the module if any of module's tests are in -t option parameters  */
  for (i = 0; i < g_num_tests; i++) {
      if ((g_execute_tests[i] - module_base) < 100) {
          skip_module++;
      }
  }

  /* Skip the module if neither of above 2 cases are true */
  if ((!skip_module) && (g_num_tests || g_num_modules)) {
      return ACS_STATUS_SKIP;
  }

  return ACS_STATUS_PASS;
}
#else
uint32_t
val_check_skip_module(uint32_t module_base)
{
  (void)module_base;
  return ACS_STATUS_PASS;
}
#endif /* COMPILE_RB_EXE */

/**
  @brief  This API prints the test number, description and
          sets the test status to pending for the input number of PEs.
          1. Caller       - Application layer
          2. Prerequisite - val_allocate_shared_mem

  @param test_num unique number identifying this test
  @param desc     brief description of the test
  @param num_pe   the number of PE to execute this test on.

  @return         Skip - if the user has overriden to skip the test.
 **/
#ifndef COMPILE_RB_EXE
uint32_t
val_initialize_test(uint32_t test_num, char8_t *desc, uint32_t num_pe)
{

  uint32_t i;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  g_override_skip = 0;

  for (i = 0; i < num_pe; i++)
      val_set_status(i, RESULT_PENDING(test_num));

  /* Skip the test if it one of the -skip option parameters */
  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == test_num) {
          val_set_status(index, RESULT_SKIP(test_num, 0));
          return ACS_STATUS_SKIP;
      }
  }

  /* Don't skip if test_num is one of the -t option parameters */
  for (i = 0; i < g_num_tests; i++) {
      if (test_num == g_execute_tests[i]) {
          g_override_skip++;
      }
  }

  /* Don't skip if the test belongs to one of the modules in -m option parameters */
  for (i = 0; i < g_num_modules; i++) {
      if ((test_num - g_execute_modules[i]) > 0 &&
          (test_num - g_execute_modules[i]) < 100)
      {
          g_override_skip++;
      }
  }

  if ((!g_override_skip) && (g_num_tests || g_num_modules)) {
      val_set_status(index, RESULT_SKIP(test_num, 0));
      return ACS_STATUS_SKIP;
  }

  g_override_skip = 1;

  val_print(ACS_PRINT_ERR, "%4d : ", test_num); //Always print this
  val_print(ACS_PRINT_TEST, desc, 0);
  val_report_status(0, ACS_START(test_num), NULL);
  val_pe_initialize_default_exception_handler(val_pe_default_esr);

  g_acs_tests_total++;

  return ACS_STATUS_PASS;
}
#else
uint32_t
val_initialize_test(uint32_t test_num, char8_t *desc, uint32_t num_pe)
{
  uint32_t i;
  (void)desc;
  (void)num_pe;

  /* Set TEST_PENDING_VAL status for all PEs, hint for val_wait_for_test_completion */
  for (i = 0; i < num_pe; i++)
      val_set_status(i, RESULT_PENDING(test_num));

  val_pe_initialize_default_exception_handler(val_pe_default_esr);
  return ACS_STATUS_PASS;
}
#endif /* COMPILE_RB_EXE */

/**
  @brief  Allocate memory which is to be shared across PEs

  @param  None

  @result None
**/
void
val_allocate_shared_mem()
{

  pal_mem_allocate_shared(val_pe_get_num(), sizeof(VAL_SHARED_MEM_t));

}

/**
  @brief  Free the memory which was allocated by allocate_shared_mem
        1. Caller       - Application Layer
        2. Prerequisite - val_allocate_shared_mem

  @param  None

  @result None
**/
void
val_free_shared_mem()
{

  pal_mem_free_shared();
}

/**
  @brief  This function sets the address of the test entry and the test
          argument to the shared address space which is picked up by the
          secondary PE identified by index.
          1. Caller       - VAL
          2. Prerequisite - val_allocate_shared_mem

  @param index     the PE Index
  @param addr      Address of the test payload which needs to be executed by PE
  @param test_data 64-bit data to be passed as a parameter to test payload

  @return        None
 **/
void
val_set_test_data(uint32_t index, uint64_t addr, uint64_t test_data)
{
  volatile VAL_SHARED_MEM_t *mem;

  if(index > val_pe_get_num())
  {
      val_print(ACS_PRINT_ERR, "\n Incorrect PE index = %d", index);
      return;
  }

  mem = (VAL_SHARED_MEM_t *)pal_mem_get_shared_addr();
  mem = mem + index;

  mem->data0 = addr;
  mem->data1 = test_data;

  val_data_cache_ops_by_va((addr_t)&mem->data0, CLEAN_AND_INVALIDATE);
  val_data_cache_ops_by_va((addr_t)&mem->data1, CLEAN_AND_INVALIDATE);
}

/**
  @brief  This API returns the optional data parameter between PEs
          to the output console.
          1. Caller       - Test Suite
          2. Prerequisite - val_set_test_data

  @param index   PE index whose data parameter has to be returned.
  @param *data0  Shared Data0.
  @param *data1  Shared Data1.

  @return    64-bit data
 **/

void
val_get_test_data(uint32_t index, uint64_t *data0, uint64_t *data1)
{

  volatile VAL_SHARED_MEM_t *mem;

  if(index > val_pe_get_num())
  {
      val_print(ACS_PRINT_ERR, "\n Incorrect PE index = %d", index);
      return;
  }

  mem = (VAL_SHARED_MEM_t *) pal_mem_get_shared_addr();
  mem = mem + index;

  val_data_cache_ops_by_va((addr_t)&mem->data0, INVALIDATE);
  val_data_cache_ops_by_va((addr_t)&mem->data1, INVALIDATE);

  *data0 = mem->data0;
  *data1 = mem->data1;

}

/**
  @brief  This function will wait for all PEs to report their status
          or we timeout and set a failure for the PE which timed-out
          1. Caller       - Application layer
          2. Prerequisite - val_set_status

  @param test_num  Unique test number
  @param num_pe    Number of PE who are executing this test
  @param timeout   integer value ob expiry the API will timeout and return

  @return        None
 **/

static void
val_wait_for_test_completion(uint32_t test_num, uint32_t num_pe, uint32_t timeout)
{

  uint32_t i = 0, j = 0;

  //For single PE tests, there is no need to wait for the results
  if (num_pe == 1)
      return;

  while(--timeout)
  {
      j = 0;
      for (i = 0; i < num_pe; i++)
      {
          if (IS_RESULT_PENDING(val_get_status(i))) {
              j = i+1;
          }
      }
      //If None of the PE have the status as Pending, return
      if (!j)
          return;
  }
  //We are here if we timed-out, set the last index PE as failed
  val_set_status(j-1, RESULT_FAIL(test_num, 0xF));
}

/**
  @brief  This API Executes the payload function on secondary PEs
          1. Caller       - Application layer
          2. Prerequisite - val_pe_create_info_table

  @param test_num   unique test number
  @param num_pe     The number of PEs to run this test on
  @param payload    Function pointer of the test entry function
  @param test_input optional parameter for the test payload

  @return        None
 **/
void
val_run_test_payload(uint32_t test_num, uint32_t num_pe, void (*payload)(void), uint64_t test_input)
{

  uint32_t my_index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t i;

  payload();  //this is test run separately on present PE
  if (num_pe == 1)
      return;

  //Now run the test on all other PE
  for (i = 0; i < num_pe; i++) {
      if (i != my_index)
          val_execute_on_pe(i, payload, test_input);
  }

  val_wait_for_test_completion(test_num, num_pe, TIMEOUT_LARGE);
}

/**
  @brief  This API Executes the payload function on primary PE with test specific argument as input
          1. Caller       - Application layer

  @param arg        Argument passed to the payload
  @param payload    Function pointer of the test entry function

  @return        None
 **/
void
val_run_test_configurable_payload(void *arg, void (*payload)(void *))
{

  /* TODO:  Consolidate this API with val_run_test_payload to create a unified interface that
            supports configurable payload execution for both single and multi-PE scenarios */

  /* This payload runs on primary PE */
  payload(arg);

  return;
}

/**
  @brief  Prints the status of the completed test
          1. Caller       - Test Suite
          2. Prerequisite - val_set_status

  @param test_num   unique test number
  @param num_pe     The number of PEs to query for status
  @param *ruleid    RuleID of the test

  @return     Success or on failure - status of the last failed PE
 **/
#ifndef COMPILE_RB_EXE
uint32_t
val_check_for_error(uint32_t test_num, uint32_t num_pe, char8_t *ruleid)
{
  uint32_t i;
  uint32_t status = 0;
  uint32_t error_flag = 0;
  uint32_t my_index = val_pe_get_index_mpid(val_pe_get_mpid());
  (void) test_num;

  /* this special case is needed when the Main PE is not the first entry
     of pe_info_table but num_pe is 1 for SOC tests */
  if (num_pe == 1) {
      status = val_get_status(my_index);
      val_report_status(my_index, status, ruleid);
      if (IS_TEST_PASS(status)) {
          g_acs_tests_pass++;
          return ACS_STATUS_PASS;
      }
      if (IS_TEST_SKIP(status))
          return ACS_STATUS_SKIP;

      g_acs_tests_fail++;

      return ACS_STATUS_FAIL;
  }

  for (i = 0; i < num_pe; i++) {
      status = val_get_status(i);
      //val_print(ACS_PRINT_ERR, "Status %4x\n", status);
      if (IS_TEST_FAIL_SKIP(status)) {
          val_report_status(i, status, ruleid);
          error_flag += 1;
          break;
      }
  }

  if (!error_flag)
      val_report_status(my_index, status, ruleid);

  if (IS_TEST_PASS(status)) {
      g_acs_tests_pass++;
      return ACS_STATUS_PASS;
  }
  if (IS_TEST_SKIP(status))
      return ACS_STATUS_SKIP;

  g_acs_tests_fail++;

  return ACS_STATUS_FAIL;
}
#else
uint32_t
val_check_for_error(uint32_t test_num, uint32_t num_pe, char8_t *ruleid)
{
  (void)ruleid;
  (void)test_num;

  uint32_t i;
  uint32_t overall_status;
  uint32_t status = TEST_FAIL;
  uint32_t checkpoint;
  uint32_t my_index = val_pe_get_index_mpid(val_pe_get_mpid());

  if (num_pe == 1) {
      status = val_get_status(my_index);
      checkpoint = status & STATUS_MASK;
      status = (status >> STATE_BIT) & STATE_MASK;
      overall_status = status;
  } else {
      /* Start with least severe status */
      overall_status = TEST_PASS;
      for (i = 0; i < num_pe; i++) {
          status = val_get_status(i);
          /* Checkpoint info from last PE would be reflected */
          checkpoint = status & STATUS_MASK;
          status = (status >> STATE_BIT) & STATE_MASK;
          /* Overwrite status if higher severity status found*/
          if (status > overall_status) {
              overall_status = status;
          }
      }
  }
  if (overall_status == TEST_FAIL) {
      val_print(ACS_PRINT_ERR, "\n        Failed at checkpoint - %2d", checkpoint);
  } else if (overall_status == TEST_SKIP) {
      val_print(ACS_PRINT_ERR, "\n        Skipped at checkpoint - %2d", checkpoint);
  }

  return overall_status;
}
#endif
/**
  @brief  Clean and Invalidate the Data cache line containing
          the input address tag

  @param  addr Address
  @param  type type of invalidation

  @return Status
**/
void
val_data_cache_ops_by_va(addr_t addr, uint32_t type)
{
  pal_pe_data_cache_ops_by_va(addr, type);

}

/**
  @brief  Update ELR based on the offset provided

  @param  *context  Context to re restored
  @param  offset    Address to be saved

  @return None
**/
void
val_pe_update_elr(void *context, uint64_t offset)
{

    if (pal_target_is_dt()) {
#ifndef TARGET_LINUX
        bsa_gic_update_elr(offset);
#endif
    }
    pal_pe_update_elr(context, offset);
    return;
}

/**
  @brief  Get ESR from exception context

  @param  *context  Context to be read

  @return ESR Value
**/
uint64_t
val_pe_get_esr(void *context)
{
    return pal_pe_get_esr(context);
}

/**
  @brief  Get FAR from exception context

  @param  *context  Context to be read

  @return FAR value
**/
uint64_t
val_pe_get_far(void *context)
{
    return pal_pe_get_far(context);
}

/**
  @brief  Write to an address, meant for debugging purpose

  @param  data Data to be written

  @return None
**/
void
val_debug_brk(uint32_t data)
{
   addr_t address = 0x9000F000; // address = pal_get_debug_address();
   *(addr_t *)address = data;
}

/**
  @brief  Compares two strings

  @param  str1  The pointer to a Null-terminated ASCII string.
  @param  str2  The pointer to a Null-terminated ASCII string.
  @param  len   The maximum number of ASCII characters for compare.

  @return Zero if strings are identical, else non-zero value
**/
uint32_t
val_strncmp(char8_t *str1, char8_t *str2, uint32_t len)
{
  return pal_strncmp(str1, str2, len);
}

/**
  Copies a source buffer to a destination buffer, and returns the destination buffer.

  @param  DestinationBuffer   The pointer to the destination buffer of the memory copy.
  @param  SourceBuffer        The pointer to the source buffer of the memory copy.
  @param  Length              The number of bytes to copy from SourceBuffer to DestinationBuffer.

  @return DestinationBuffer.

**/
void*
val_memcpy(void *dst_buffer, void *src_buffer, uint32_t len)
{
  return pal_memcpy(dst_buffer, src_buffer, len);
}

/**
  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return The value of MicroSeconds inputted.

**/
uint64_t
val_time_delay_ms(uint64_t timer_ms)
{
  return pal_time_delay_ms(timer_ms);
}

/**
   Calls pal API to dump dtb

   @param none

   @return none

**/
void
val_dump_dtb(void)
{
  pal_dump_dtb();
}

/**
  @brief  Checks whether a prerequisite test has passed before executing the current test.

  @param  prereq_status  Status of the pre-requisite test
  @param  prereq_config  Pre-requisite test configuration
  @param  curr_config    Current test configuration

  @return Zero on successful pre-req. Else return 1.
**/
uint32_t
val_check_for_prerequisite(uint32_t num_pe, uint32_t prereq_status,
                           const test_config_t *prereq_config, const test_config_t *curr_config)
{
    uint32_t status;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    status = val_initialize_test(curr_config->test_num, curr_config->desc, num_pe);
    if (status == ACS_STATUS_SKIP)
        return ACS_STATUS_SKIP;

    if (prereq_status != (uint32_t)ACS_STATUS_PASS) {

        /* Do not execute the current test if the prerequisite rule results in FAIL or SKIP */
        val_print(ACS_PRINT_ERR, "\n       Pre-requisite rule ", 0);
        val_print(ACS_PRINT_ERR, prereq_config->rule, 0);
        val_print(ACS_PRINT_ERR, " did not pass. Skipping the test", 0);
        val_set_status(index, RESULT_SKIP(curr_config->test_num, 0));
        return ACS_STATUS_SKIP;
    }

    return ACS_STATUS_PASS;
}
