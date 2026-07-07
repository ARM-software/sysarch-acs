/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (ACS_DRTM_DL_TEST_NUM_BASE  +  19)
#define TEST_RULE  ""
#define TEST_DESC  "Check Enable Secure Interrupts cmd    "

static
void
payload(uint32_t num_pe)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  int64_t  status;

  DRTM_PARAMETERS *drtm_params;
  uint64_t drtm_params_size = DRTM_SIZE_4K;

  if (g_drtm_features.enable_secure_interrupts != DRTM_ACS_SUCCESS) {
    val_print(ERROR,
              "\n       DRTM_ENABLE_SECURE_INTERRUPTS function not supported err=%d",
              g_drtm_features.enable_secure_interrupts);
    val_set_status(index, RESULT_SKIP(1));
    return;
  }

  /* Allocate Memory For DRTM Parameters 4KB Aligned */
  drtm_params = (DRTM_PARAMETERS *)((uint64_t)val_aligned_alloc(DRTM_SIZE_4K,
                                                                drtm_params_size));
  if (!drtm_params) {
    val_print(ERROR, "\n    Failed to allocate memory for DRTM Params");
    val_set_status(index, RESULT_FAIL(1));
    return;
  }

  status = val_drtm_init_drtm_params(drtm_params);
  if (status != ACS_STATUS_PASS) {
    val_print(ERROR, "\n       DRTM Init Params failed err=%ld", status);
    val_set_status(index, RESULT_FAIL(2));
    goto free_drtm_params;
  }

  drtm_params->launch_features |= DRTM_LAUNCH_FEAT_SECURE_INT_DISABLE;

  /* Invoke DRTM Dynamic Launch, This will return only in case of error */
  status = val_drtm_dynamic_launch(drtm_params);
  /* This will return only in fail*/
  if (status < DRTM_ACS_SUCCESS) {
    val_print(ERROR, "\n       DRTM Dynamic Launch failed err=%ld", status);
    val_set_status(index, RESULT_FAIL(3));
    goto free_dlme_region;
  }

  /* The first enable request must succeed after secure interrupts are disabled. */
  status = val_drtm_enable_secure_interrupts();
  if (status != DRTM_ACS_SUCCESS) {
    val_print(ERROR, "\n       DRTM Enable Secure Interrupts failed err=%ld", status);
    val_print(ERROR, " Expected %d,", DRTM_ACS_SUCCESS);
    val_set_status(index, RESULT_FAIL(4));
    goto unprotect_memory;
  }

  /* A repeated enable request must be denied after interrupts are already enabled. */
  status = val_drtm_enable_secure_interrupts();
  if (status != DRTM_ACS_DENIED) {
    val_print(ERROR, "\n       Unexpected Status for second enable secure interrupts %ld", status);
    val_print(ERROR, " Expected %d,", DRTM_ACS_DENIED);
    val_set_status(index, RESULT_FAIL(5));
    goto unprotect_memory;
  }

  /* Unprotect memory after dynamic launch. */
  status = val_drtm_unprotect_memory();
  if (status < DRTM_ACS_SUCCESS) {
    val_print(ERROR, "\n       DRTM Unprotect Memory failed err=%ld", status);
    val_set_status(index, RESULT_FAIL(6));
    goto free_dlme_region;
  }

  val_set_status(index, RESULT_PASS);
  goto free_dlme_region;

unprotect_memory:
  status = val_drtm_unprotect_memory();
  if (status < DRTM_ACS_SUCCESS)
    val_print(ERROR, "\n       DRTM Unprotect Memory failed err=%ld", status);

free_dlme_region:
  val_memory_free_aligned((void *)drtm_params->dlme_region_address);
free_drtm_params:
  val_memory_free_aligned((void *)drtm_params);

  return;
}

uint32_t
dl019_entry(uint32_t num_pe)
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
