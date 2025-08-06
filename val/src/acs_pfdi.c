/** @file
 * PFDI API
 *
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
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

#include "include/val_interface.h"
#include "include/acs_common.h"
#include "include/acs_val.h"
#include "include/acs_std_smc.h"
#include "include/acs_memory.h"

/**
  @brief   This function checks if reserved_bits received are zero
  @param   reserved_bits reserved bits value received
  @return  status
**/
uint32_t val_pfdi_reserved_bits_check_is_zero(uint32_t reserved_bits)
{
    if (reserved_bits != VAL_PFDI_RESERVED_BYTE_ZERO) {
        val_print(ACS_PRINT_ERR, "\n       CHECK RSVD BITS: FAILED [0x%08x]", reserved_bits);
        return ACS_STATUS_FAIL;
    } else
        val_print(ACS_PRINT_DEBUG, "\n       CHECK RSVD BITS: PASSED", 0);
    return ACS_STATUS_PASS;
}

int64_t val_invoke_pfdi_fn(unsigned long function_id, unsigned long arg1,
              unsigned long arg2, unsigned long arg3,
              unsigned long arg4, unsigned long arg5,
              unsigned long *ret1, unsigned long *ret2,
              unsigned long *ret3)
{
    int64_t status;
    ARM_SMC_ARGS args;

    args.Arg0 = function_id;
    args.Arg1 = arg1;
    args.Arg2 = arg2;
    args.Arg3 = arg3;
    args.Arg4 = arg4;
    args.Arg5 = arg5;

    pal_pe_call_smc(&args, CONDUIT_SMC);
    status = args.Arg0;

    if (ret1)
      *ret1 = args.Arg1;
    if (ret2)
      *ret2 = args.Arg2;
    if (ret3)
      *ret3 = args.Arg3;

    return status;
}

/**
 *  @brief   This function returns the version of the PFDI interface.
 *
 *  @return  status and version
 */
int64_t val_pfdi_version(void)
{
    return val_invoke_pfdi_fn(PFDI_FN_PFDI_VERSION, 0, 0, 0, 0, 0,
                              NULL, NULL, NULL);
}

/**
 *  @brief   Query the PFDI features.
 *
 *  @return  status and feature details
 */
int64_t val_pfdi_features(uint32_t function_id)
{
    return val_invoke_pfdi_fn(PFDI_FN_PFDI_FEATURES, function_id, 0, 0, 0, 0,
                                NULL, NULL, NULL);
}

/**
 *  @brief   Query PFDI Test ID
 *
 *  @return  status and PFDI Test ID
 */
int64_t val_pfdi_pe_test_id(int64_t *test_id)
{
    int64_t status;
    unsigned long _test_id;

    status = val_invoke_pfdi_fn(PFDI_FN_PFDI_PE_TEST_ID, 0, 0, 0, 0, 0,
                              &_test_id, NULL, NULL);
    if (test_id)
        *test_id = (int64_t)_test_id;

    return status;
}

/**
 *  @brief   Query number of PFDI Test part supported
 *
 *  @return  status and PFDI part tests count
 */
int64_t val_pfdi_pe_test_part_count(void)
{
    return val_invoke_pfdi_fn(PFDI_FN_PFDI_PE_TEST_PART_COUNT, 0, 0, 0, 0, 0,
                              NULL, NULL, NULL);
}

/**
 *  @brief   Calls PFDI PE Test run function
 *  @return  status and fault found test part
 */
int64_t val_pfdi_pe_test_run(int64_t start, int64_t end, int64_t *fault_test_id)
{
    int64_t status;
    unsigned long _fault_test_id;

    status = val_invoke_pfdi_fn(PFDI_FN_PFDI_PE_TEST_RUN, start, end, 0, 0, 0,
                              &_fault_test_id, NULL, NULL);
    if (fault_test_id)
        *fault_test_id = (int64_t)_fault_test_id;

    return status;
}

/**
 *  @brief   Calls PFDI PE Test Results function
 *  @return  status and fault test part if test report fault
 */
int64_t val_pfdi_pe_test_result(int64_t *fault_test_part_id)
{
    int64_t status;
    unsigned long _fault_test_part_id;

    status = val_invoke_pfdi_fn(PFDI_FN_PFDI_PE_TEST_RESULT, 0, 0, 0, 0, 0,
                              &_fault_test_part_id, NULL, NULL);
    if (fault_test_part_id)
        *fault_test_part_id = (int64_t)_fault_test_part_id;

    return status;
}

/**
 *  @brief   Calls PFDI Firmware Check function
 *
 *  @return  status
 */
int64_t val_pfdi_fw_check(void)
{
    return val_invoke_pfdi_fn(PFDI_FN_PFDI_FW_CHECK, 0, 0, 0, 0, 0,
                              NULL, NULL, NULL);
}

/**
 *  @brief   Query the PFDI Force Error function.
 *
 *  @return  status
 */
int64_t val_pfdi_force_error(uint32_t function_id, int64_t error_value)
{
    return val_invoke_pfdi_fn(PFDI_FN_PFDI_FORCE_ERROR, function_id,
                                error_value, 0, 0, 0, NULL, NULL, NULL);
}

