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

#define TEST_NUM   (ACS_DRTM_DL_TEST_NUM_BASE  +  13)
#define TEST_RULE  "R48020"
#define TEST_DESC  "Validate default PCR schema event ordering"

static
void
payload(uint32_t num_pe)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  int32_t  status;
  int64_t  pcr_schema;
  uint64_t drtm_params_size = DRTM_SIZE_4K;
  uint32_t pcr17_events[256];
  uint32_t pcr17_count = 0;
  uint32_t pcr18_events[256];
  uint32_t pcr18_count = 0;
  uint32_t event_idx = 0;

  DRTM_PARAMETERS      *drtm_params;
  DRTM_EVENT_LOG_STATE event_log;
  DRTM_EVENT_LOG_ENTRY entry;

  /* Verify if Default PCR schema is supported. If not, the test is skipped */
  pcr_schema = val_drtm_get_feature(DRTM_DRTM_FEATURES_PCR_SCHEMA);
  if ((pcr_schema & DRTM_PCR_SCHEMA_USAGE_MASK_DEF_SCHEMA) != DRTM_TPM_FEAT_PCR_SCHEMA_DEF_SUPP) {
    val_print(DEBUG,
              "\n       DRTM implementation does not support default PCR schema, skip check");
    val_set_status(index, RESULT_SKIP(1));
    return;
  }

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

  /* TODO - Enable below line after implementing DRTM_ENABLE_SECURE_INTERRUPTS function */
  /* drtm_params->launch_features |= DRTM_LAUNCH_FEAT_SECURE_INT_DISABLE; */

  /* Invoke DRTM Dynamic Launch, This will return only in case of error */
  status = val_drtm_dynamic_launch(drtm_params);
  /* This will return only in fail*/
  if (status < DRTM_ACS_SUCCESS) {
    val_print(ERROR, "\n       DRTM Dynamic Launch failed err=%d", status);
    val_set_status(index, RESULT_FAIL(3));
    goto free_dlme_region;
  }

  /* Call DRTM Unprotect Memory */
  status = val_drtm_unprotect_memory();
  if (status < DRTM_ACS_SUCCESS) {
    val_print(ERROR, "\n       DRTM Unprotect Memory failed err=%d", status);
    val_set_status(index, RESULT_FAIL(4));
    goto free_dlme_region;
  }

  status = val_drtm_event_log_init(drtm_params, &event_log);
  if (status != ACS_STATUS_PASS) {
    val_print(ERROR, "\n       Event log initialization failed", 0);
    val_set_status(index, RESULT_FAIL(5));
    goto free_dlme_region;
  }

  while ((status = val_drtm_event_log_next(&event_log, &entry)) != DRTM_ACS_NOT_FOUND) {
    if (status != ACS_STATUS_PASS) {
      val_print(ERROR, "\n       Event log parsing failed");
      val_set_status(index, RESULT_FAIL(6));
      goto free_dlme_region;
    }

    if (entry.event->pcr_index == 17 && pcr17_count < 256) {
      pcr17_events[pcr17_count++] = entry.event->event_type;
    } else if (entry.event->pcr_index == 18 && pcr18_count < 256) {
      pcr18_events[pcr18_count++] = entry.event->event_type;
    }
  }

  /* Validate PCR[17] event sequence */
  event_idx = 0;
  /* DCE Image required*/
  if ((event_idx >= pcr17_count) || (pcr17_events[event_idx] != DRTM_EVTYPE_ARM_DCE)) {
    val_print(ERROR, "\n       PCR[17] DCE event not found or out of order");
    val_set_status(index, RESULT_FAIL(7));
    goto free_dlme_region;
  }
  event_idx++;
  /* PCR_SCHEMA required */
  if (event_idx >= pcr17_count || pcr17_events[event_idx] != DRTM_EVTYPE_ARM_PCR_SCHEMA) {
    val_print(ERROR, "\n       PCR[17] PCR_SCHEMA not found or out of order");
    val_set_status(index, RESULT_FAIL(8));
    goto free_dlme_region;
  }
  event_idx++;
  /* TZFW optional repeatable */
  while (event_idx < pcr17_count && pcr17_events[event_idx] == DRTM_EVTYPE_ARM_TZFW)
    event_idx++;
  /* DCE_SECONDARY optional repeatable */
  while (event_idx < pcr17_count && pcr17_events[event_idx] == DRTM_EVTYPE_ARM_DCE_SECONDARY)
    event_idx++;

  /* SECURE_INT_DISABLE optional */
  /* TODO - Change this to required after enabling DRTM_EVTYPE_ARM_SECURE_INT_DISABLE */
  if (event_idx < pcr17_count && pcr17_events[event_idx] == DRTM_EVTYPE_ARM_SECURE_INT_DISABLE)
    event_idx++;
  /* SEPARATOR required */
  if (event_idx >= pcr17_count || pcr17_events[event_idx] != DRTM_EVTYPE_ARM_SEPARATOR) {
    val_print(ERROR, "\n       PCR[17] SEPARATOR not found or out of order");
    val_set_status(index, RESULT_FAIL(9));
    goto free_dlme_region;
  }
  event_idx++;
  /* No more events */

  if (event_idx != pcr17_count) {
    val_print(ERROR, "\n       PCR[17] extra events after SEPARATOR");
    val_set_status(index, RESULT_FAIL(10));
    goto free_dlme_region;
  }

  /* Validate PCR[18] event sequence */
  event_idx = 0;
  /* PCR_SCHEMA required */
  if (event_idx >= pcr18_count || pcr18_events[event_idx] != DRTM_EVTYPE_ARM_PCR_SCHEMA) {
    val_print(ERROR, "\n       PCR[18] PCR_SCHEMA not found or out of order");
    val_set_status(index, RESULT_FAIL(11));
    goto free_dlme_region;
  }
  event_idx++;
  /* DCE_PUBKEY optional repeatable */
  while (event_idx < pcr18_count && pcr18_events[event_idx] == DRTM_EVTYPE_ARM_DCE_PUBKEY)
    event_idx++;
  /* DLME required */
  if (event_idx >= pcr18_count || pcr18_events[event_idx] != DRTM_EVTYPE_ARM_DLME) {
    val_print(ERROR, "\n       PCR[18] DLME not found or out of order");
    val_set_status(index, RESULT_FAIL(12));
    goto free_dlme_region;
  }
  event_idx++;
  /* DLME_ENTRY_POINT required */
  if (event_idx >= pcr18_count || pcr18_events[event_idx] != DRTM_EVTYPE_ARM_DLME_ENTRY_POINT) {
    val_print(ERROR, "\n       PCR[18] DLME_ENTRY_POINT not found or out of order");
    val_set_status(index, RESULT_FAIL(13));
    goto free_dlme_region;
  }
  event_idx++;
  /* DEBUG_CONFIG required */
  if (event_idx >= pcr18_count || pcr18_events[event_idx] != DRTM_EVTYPE_ARM_DEBUG_CONFIG) {
    val_print(ERROR, "\n       PCR[18] DEBUG_CONFIG not found or out of order");
    val_set_status(index, RESULT_FAIL(14));
    goto free_dlme_region;
  }
  event_idx++;
  /* NONSECURE_CONFIG required */
  if (event_idx >= pcr18_count || pcr18_events[event_idx] != DRTM_EVTYPE_ARM_NONSECURE_CONFIG) {
    val_print(ERROR, "\n       PCR[18] NONSECURE_CONFIG not found or out of order");
    val_set_status(index, RESULT_FAIL(15));
    goto free_dlme_region;
  }
  event_idx++;
  /* SEPARATOR required */
  if (event_idx >= pcr18_count || pcr18_events[event_idx] != DRTM_EVTYPE_ARM_SEPARATOR) {
    val_print(ERROR, "\n       PCR[18] SEPARATOR not found or out of order");
    val_set_status(index, RESULT_FAIL(16));
    goto free_dlme_region;
  }
  event_idx++;
  /* No more events */
  if (event_idx != pcr18_count) {
    val_print(ERROR, "\n       PCR[18] extra events after SEPARATOR");
    val_set_status(index, RESULT_FAIL(17));
    goto free_dlme_region;
  }

  val_set_status(index, RESULT_PASS);

free_dlme_region:
  if (drtm_params->dlme_region_address)
    val_memory_free_aligned((void *)drtm_params->dlme_region_address);
free_drtm_params:
  val_memory_free_aligned((void *)drtm_params);

  return;
}

uint32_t dl013_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
      payload(num_pe);

  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
