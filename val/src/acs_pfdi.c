/** @file
 * PFDI API
 *
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

#include "include/val_interface.h"
#include "include/acs_common.h"
#include "include/acs_val.h"
#include "include/acs_std_smc.h"
#include "include/acs_memory.h"

extern int32_t gPsciConduit;

/**
  @brief   Probe to see if the platform implements any PFDI entry points.
           1. Caller       -  Application layer.
           2. Prerequisite - None.
  @return  ACS_STATUS_PASS if at least one call is supported, else PFDI_ACS_NOT_IMPLEMENTED.
**/
uint32_t
val_pfdi_check_implementation(void)
{
    uint32_t f_id;
    uint32_t test_skip = 1;
    int64_t  pfdi_fn_status;

    for (f_id = PFDI_FN_PFDI_VERSION; f_id <= PFDI_FN_PFDI_FORCE_ERROR; f_id++) {
        pfdi_fn_status = val_invoke_pfdi_fn(f_id, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
        if (pfdi_fn_status != PFDI_ACS_NOT_SUPPORTED) {
            test_skip = 0;
            break;
        }
    }

    if (test_skip)
        return PFDI_ACS_NOT_IMPLEMENTED;

    return ACS_STATUS_PASS;
}

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
              unsigned long *ret3, unsigned long *ret4)
{
    int64_t status;
    ARM_SMC_ARGS args;

    args.Arg0 = function_id;
    args.Arg1 = arg1;
    args.Arg2 = arg2;
    args.Arg3 = arg3;
    args.Arg4 = arg4;
    args.Arg5 = arg5;

    pal_pe_call_smc(&args, gPsciConduit);
    status = args.Arg0;

    if (ret1)
      *ret1 = args.Arg1;
    if (ret2)
      *ret2 = args.Arg2;
    if (ret3)
      *ret3 = args.Arg3;
    if (ret4)
      *ret4 = args.Arg4;

    return status;
}

/**
 *  @brief   This function returns the version of the PFDI interface.
 *
 *  @return  status and version
 */
int64_t val_pfdi_version(int64_t *x1, int64_t *x2, int64_t *x3, int64_t *x4)
{
    int64_t status;
    unsigned long _x1, _x2, _x3, _x4;

    status = val_invoke_pfdi_fn(PFDI_FN_PFDI_VERSION, 0, 0, 0, 0, 0,
                              &_x1, &_x2, &_x3, &_x4);

    if (x1)
        *x1 = (int64_t)_x1;
    if (x2)
        *x2 = (int64_t)_x2;
    if (x3)
        *x3 = (int64_t)_x3;
    if (x4)
        *x4 = (int64_t)_x4;

    return status;
}

/**
 *  @brief   Query the PFDI features.
 *
 *  @return  status and feature details
 */
int64_t val_pfdi_features(uint32_t function_id, int64_t *x1, int64_t *x2, int64_t *x3, int64_t *x4)
{
    int64_t status;
    unsigned long _x1, _x2, _x3, _x4;

    status = val_invoke_pfdi_fn(PFDI_FN_PFDI_FEATURES, function_id, 0, 0, 0, 0,
                              &_x1, &_x2, &_x3, &_x4);

    if (x1)
        *x1 = (int64_t)_x1;
    if (x2)
        *x2 = (int64_t)_x2;
    if (x3)
        *x3 = (int64_t)_x3;
    if (x4)
        *x4 = (int64_t)_x4;

    return status;

}

/**
 *  @brief   Query PFDI Test ID
 *
 *  @return  status and PFDI Test ID
 */
int64_t val_pfdi_pe_test_id(int64_t *x1, int64_t *x2, int64_t *x3, int64_t *x4)
{
    int64_t status;
    unsigned long _x1, _x2, _x3, _x4;

    status = val_invoke_pfdi_fn(PFDI_FN_PFDI_PE_TEST_ID, 0, 0, 0, 0, 0,
                              &_x1, &_x2, &_x3, &_x4);

    if (x1)
        *x1 = (int64_t)_x1;
    if (x2)
        *x2 = (int64_t)_x2;
    if (x3)
        *x3 = (int64_t)_x3;
    if (x4)
        *x4 = (int64_t)_x4;

    return status;
}

/**
 *  @brief   Query number of PFDI Test part supported
 *
 *  @return  status and PFDI part tests count
 */
int64_t val_pfdi_pe_test_part_count(int64_t *x1, int64_t *x2, int64_t *x3, int64_t *x4)
{
    int64_t status;
    unsigned long _x1, _x2, _x3, _x4;

    status = val_invoke_pfdi_fn(PFDI_FN_PFDI_PE_TEST_PART_COUNT, 0, 0, 0, 0, 0,
                              &_x1, &_x2, &_x3, &_x4);

    if (x1)
        *x1 = (int64_t)_x1;
    if (x2)
        *x2 = (int64_t)_x2;
    if (x3)
        *x3 = (int64_t)_x3;
    if (x4)
        *x4 = (int64_t)_x4;

    return status;
}

/**
 *  @brief   Calls PFDI PE Test run function
 *  @return  status and fault found test part
 */
int64_t val_pfdi_pe_test_run(int64_t start, int64_t end,
                             int64_t *x1, int64_t *x2, int64_t *x3, int64_t *x4)
{
    int64_t status;
    unsigned long _x1, _x2, _x3, _x4;

    status = val_invoke_pfdi_fn(PFDI_FN_PFDI_PE_TEST_RUN, start, end, 0, 0, 0,
                               &_x1, &_x2, &_x3, &_x4);

    if (x1)
        *x1 = (int64_t)_x1;
    if (x2)
        *x2 = (int64_t)_x2;
    if (x3)
        *x3 = (int64_t)_x3;
    if (x4)
        *x4 = (int64_t)_x4;

    return status;
}

/**
 *  @brief   Calls PFDI PE Test Results function
 *  @return  status and fault test part if test report fault
 */
int64_t val_pfdi_pe_test_result(int64_t *x1, int64_t *x2, int64_t *x3, int64_t *x4)
{
    int64_t status;
    unsigned long _x1, _x2, _x3, _x4;

    status = val_invoke_pfdi_fn(PFDI_FN_PFDI_PE_TEST_RESULT, 0, 0, 0, 0, 0,
                                &_x1, &_x2, &_x3, &_x4);

    if (x1)
        *x1 = (int64_t)_x1;
    if (x2)
        *x2 = (int64_t)_x2;
    if (x3)
        *x3 = (int64_t)_x3;
    if (x4)
        *x4 = (int64_t)_x4;

    return status;
}

/**
 *  @brief   Calls PFDI Firmware Check function
 *
 *  @return  status
 */
int64_t val_pfdi_fw_check(int64_t *x1, int64_t *x2, int64_t *x3, int64_t *x4)
{
    int64_t status;
    unsigned long _x1, _x2, _x3, _x4;

    status = val_invoke_pfdi_fn(PFDI_FN_PFDI_FW_CHECK, 0, 0, 0, 0, 0,
                                 &_x1, &_x2, &_x3, &_x4);

    if (x1)
        *x1 = (int64_t)_x1;
    if (x2)
        *x2 = (int64_t)_x2;
    if (x3)
        *x3 = (int64_t)_x3;
    if (x4)
        *x4 = (int64_t)_x4;

    return status;
}

/**
 *  @brief   Query the PFDI Force Error function.
 *
 *  @return  status
 */
int64_t val_pfdi_force_error(uint32_t function_id, int64_t error_value,
                             int64_t *x1, int64_t *x2, int64_t *x3, int64_t *x4)

{
    int64_t status;
    unsigned long _x1, _x2, _x3, _x4;

    status = val_invoke_pfdi_fn(PFDI_FN_PFDI_FORCE_ERROR, function_id,
                                error_value, 0, 0, 0, &_x1, &_x2, &_x3, &_x4);

    if (x1)
        *x1 = (int64_t)_x1;
    if (x2)
        *x2 = (int64_t)_x2;
    if (x3)
        *x3 = (int64_t)_x3;
    if (x4)
        *x4 = (int64_t)_x4;

    return status;
}

/**
  @brief  Make the SMC call using AARCH64 Assembly code

  @param  args to pass to the EL3 firmware
  @param  conduit  SMC or HVC
  @param  pre_smc_regs  regs x5 to x17 before smc/hvc call
  @param  post_smc_regs regs x5 to x17 after smc/hvc call

  @return  None
**/
void
val_pfdi_verify_regs(ARM_SMC_ARGS *args, int32_t conduit,
              uint64_t pre_smc_regs[REG_COUNT_X5_X17],
              uint64_t post_smc_regs[REG_COUNT_X5_X17])
{
    pal_pfdi_verify_regs(args, conduit, pre_smc_regs, post_smc_regs);
}

/**
  @brief  Clean and Invalidate the Data cache line containing
          the input address tag

  @param  addr Address

  @return Status
**/
void
val_pfdi_invalidate_ret_params(PFDI_RET_PARAMS *args)
{
  val_data_cache_ops_by_va((addr_t)&args->x0, CLEAN_AND_INVALIDATE);
  val_data_cache_ops_by_va((addr_t)&args->x1, CLEAN_AND_INVALIDATE);
  val_data_cache_ops_by_va((addr_t)&args->x2, CLEAN_AND_INVALIDATE);
  val_data_cache_ops_by_va((addr_t)&args->x3, CLEAN_AND_INVALIDATE);
  val_data_cache_ops_by_va((addr_t)&args->x4, CLEAN_AND_INVALIDATE);
}


