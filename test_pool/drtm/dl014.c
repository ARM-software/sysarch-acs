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

#include "val/include/acs_val.h"
#include "val/include/acs_memory.h"
#include "val/include/val_interface.h"

#define TEST_NUM   (ACS_DRTM_DL_TEST_NUM_BASE  +  14)
#define TEST_RULE  "R45460"
#define TEST_DESC  "Check NS async excp masked on boot PE "

static
void
payload(uint32_t num_pe)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  int64_t  status;
  uint64_t read_daif = 0;

  DRTM_PARAMETERS *drtm_params;
  uint64_t drtm_params_size = DRTM_SIZE_4K;

  /* Allocate Memory For DRTM Parameters 4KB Aligned */
  drtm_params = (DRTM_PARAMETERS *)((uint64_t)val_aligned_alloc(DRTM_SIZE_4K, drtm_params_size));
  if (!drtm_params) {
    val_print(ERROR, "\n    Failed to allocate memory for DRTM Params");
    val_set_status(index, RESULT_FAIL(1));
    return;
  }

  status = val_drtm_init_drtm_params(drtm_params);
  if (status != ACS_STATUS_PASS) {
    val_print(ERROR, "\n       DRTM Init Params failed err=%d", status);
    val_set_status(index, RESULT_FAIL(2));
    goto free_drtm_params;
  }

  /* Invoke DRTM Dynamic Launch, This will return only in case of error */
  status = val_drtm_dynamic_launch(drtm_params);
  /* This will return only in fail*/
  if (status < DRTM_ACS_SUCCESS) {
    val_print(ERROR, "\n       DRTM Dynamic Launch failed");
    val_set_status(index, RESULT_FAIL(3));
    goto free_dlme_region;
  }

  status = val_drtm_unprotect_memory();
  if (status < DRTM_ACS_SUCCESS) {
    val_print(ERROR, "\n       DRTM Unprotect Memory failed err=%d", status);
    val_set_status(index, RESULT_FAIL(4));
    goto free_dlme_region;
  }

  /* Read DAIF Register on current PE */
  read_daif = val_drtm_read_daif();
  val_print(DEBUG, "\n       DAIF register value 0x%llx", read_daif);

  /* verify non-secure async exceptions (SError, IRQ, FIQ) are masked */
  if (((read_daif >> DRTM_NS_EXECPTION_SHIFT_AIF) & DRTM_NS_EXECPTION_MASK_AIF) !=
                                            DRTM_NON_SECURE_EXCP_MASKED) {
    val_print(ERROR, "\n       Non secure exceptions are not masked on Boot PE");
    val_set_status(index, RESULT_FAIL(5));
  } else {
    val_set_status(index, RESULT_PASS);
  }

free_dlme_region:
  val_memory_free_aligned((void *)drtm_params->dlme_region_address);
free_drtm_params:
  val_memory_free_aligned((void *)drtm_params);

  return;
}

uint32_t
dl014_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;
  num_pe = 1;
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
  /* execute payload, which will execute relevant functions on Boot PE */
      payload(num_pe);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
