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

#define TEST_NUM   (ACS_DRTM_DL_TEST_NUM_BASE  +  16)
#define TEST_RULE  "R44160"
#define TEST_DESC  "Validate zero digest for non-distinct DCE "

static const uint8_t sha256_zero_digest[] = {
  0x6e, 0x34, 0x0b, 0x9c, 0xff, 0xb3, 0x7a, 0x98,
  0x9c, 0xa5, 0x44, 0xe6, 0xbb, 0x78, 0x0a, 0x2c,
  0x78, 0x90, 0x1d, 0x3f, 0xb3, 0x37, 0x38, 0x76,
  0x85, 0x11, 0xa3, 0x06, 0x17, 0xaf, 0xa0, 0x1d
};

static const uint8_t sha384_zero_digest[] = {
  0xbe, 0xc0, 0x21, 0xb4, 0xf3, 0x68, 0xe3, 0x06,
  0x91, 0x34, 0xe0, 0x12, 0xc2, 0xb4, 0x30, 0x70,
  0x83, 0xd3, 0xa9, 0xbd, 0xd2, 0x06, 0xe2, 0x4e,
  0x5f, 0x0d, 0x86, 0xe1, 0x3d, 0x66, 0x36, 0x65,
  0x59, 0x33, 0xec, 0x2b, 0x41, 0x34, 0x65, 0x96,
  0x68, 0x17, 0xa9, 0xc2, 0x08, 0xa1, 0x17, 0x17
};

static const uint8_t sha512_zero_digest[] = {
  0xb8, 0x24, 0x4d, 0x02, 0x89, 0x81, 0xd6, 0x93,
  0xaf, 0x7b, 0x45, 0x6a, 0xf8, 0xef, 0xa4, 0xca,
  0xd6, 0x3d, 0x28, 0x2e, 0x19, 0xff, 0x14, 0x94,
  0x2c, 0x24, 0x6e, 0x50, 0xd9, 0x35, 0x1d, 0x22,
  0x70, 0x4a, 0x80, 0x2a, 0x71, 0xc3, 0x58, 0x0b,
  0x63, 0x70, 0xde, 0x4c, 0xeb, 0x29, 0x3c, 0x32,
  0x4a, 0x84, 0x23, 0x34, 0x25, 0x57, 0xd4, 0xe5,
  0xc3, 0x84, 0x38, 0xf0, 0xe3, 0x69, 0x10, 0xee
};

static
const uint8_t *
get_zero_digest(uint16_t hash_alg, uint32_t *digest_size)
{
  if (digest_size == NULL)
    return NULL;

  *digest_size = val_drtm_get_digest_size(hash_alg);
  switch (hash_alg) {
  case DRTM_TPM_ALG_SHA256:
    return sha256_zero_digest;
  case DRTM_TPM_ALG_SHA384:
    return sha384_zero_digest;
  case DRTM_TPM_ALG_SHA512:
    return sha512_zero_digest;
  default:
    *digest_size = 0;
    return NULL;
  }
}

static
int32_t
validate_zero_digest_event(const DRTM_EVENT_LOG_STATE *event_log,
                           const DRTM_EVENT_LOG_ENTRY *entry, uint16_t hash_alg,
                           const uint8_t *expected_digest, uint32_t expected_digest_size)
{
  int32_t  status;
  uint8_t  *digest;
  uint16_t digest_size;

  status = val_drtm_event_log_get_digest(event_log, entry, hash_alg, &digest, &digest_size);
  if (status != ACS_STATUS_PASS)
    return status;

  if (digest_size != expected_digest_size)
    return ACS_STATUS_FAIL;

  if (val_memory_compare(digest, (void *)expected_digest, expected_digest_size))
    return ACS_STATUS_FAIL;

  return ACS_STATUS_PASS;
}

static
void
payload(uint32_t num_pe)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  int32_t  status;
  uint16_t fw_hash_alg;
  uint32_t digest_size;
  uint32_t dce_seen = 0;
  uint32_t dce_pubkey_seen = 0;
  uint64_t drtm_params_size = DRTM_SIZE_4K;
  const uint8_t *zero_digest;

  DRTM_PARAMETERS      *drtm_params;
  DRTM_EVENT_LOG_STATE event_log;
  DRTM_EVENT_LOG_ENTRY entry;

  if (val_drtm_are_dce_and_drtm_images_distinct()) {
    val_print(DEBUG, "\n       D-CRTM and DCE images are distinct, skip check");
    val_set_status(index, RESULT_SKIP(1));
    return;
  }

  /* Get the firmware hash algorithm from DRTM features */
  fw_hash_alg = (uint16_t)val_drtm_get_feature(DRTM_DRTM_FEATURES_FW_HASH_ALGOROTHM);
  zero_digest = get_zero_digest(fw_hash_alg, &digest_size);
  if ((zero_digest == NULL) || (digest_size == 0)) {
    val_print(ERROR, "\n       Unsupported hash algorithm: 0x%x", fw_hash_alg);
    val_set_status(index, RESULT_FAIL(1));
    return;
  }

  /* Allocate Memory For DRTM Parameters 4KB Aligned */
  drtm_params = (DRTM_PARAMETERS *)((uint64_t)val_aligned_alloc(DRTM_SIZE_4K, drtm_params_size));
  if (!drtm_params) {
    val_print(ERROR, "\n    Failed to allocate memory for DRTM Params");
    val_set_status(index, RESULT_FAIL(2));
    return;
  }

  status = val_drtm_init_drtm_params(drtm_params);
  if (status != ACS_STATUS_PASS) {
    val_print(ERROR, "\n       DRTM Init Params failed err=%d", status);
    val_set_status(index, RESULT_FAIL(3));
    goto free_drtm_params;
  }

  /* Invoke DRTM Dynamic Launch, This will return only in case of error */
  status = val_drtm_dynamic_launch(drtm_params);
  /* This will return only in fail*/
  if (status < DRTM_ACS_SUCCESS) {
    val_print(ERROR, "\n       DRTM Dynamic Launch failed err=%d", status);
    val_set_status(index, RESULT_FAIL(4));
    goto free_dlme_region;
  }

  /* Call DRTM Unprotect Memory */
  status = val_drtm_unprotect_memory();
  if (status < DRTM_ACS_SUCCESS) {
    val_print(ERROR, "\n       DRTM Unprotect Memory failed err=%d", status);
    val_set_status(index, RESULT_FAIL(5));
    goto free_dlme_region;
  }

  status = val_drtm_event_log_init(drtm_params, &event_log);
  if (status != ACS_STATUS_PASS) {
    val_print(ERROR, "\n       Event log initialization failed");
    val_set_status(index, RESULT_FAIL(6));
    goto free_dlme_region;
  }

  /* Iterate through event log entries */
  while ((status = val_drtm_event_log_next(&event_log, &entry)) != DRTM_ACS_NOT_FOUND) {
    if (status != ACS_STATUS_PASS) {
      val_print(ERROR, "\n       Event log parsing failed");
      val_set_status(index, RESULT_FAIL(7));
      goto free_dlme_region;
    }

    /* DCE Image required*/
    if (entry.event->event_type == DRTM_EVTYPE_ARM_DCE) {
      /* DCE must be recorded in PCR[17] */
      if (entry.event->pcr_index != 17) {
        val_print(ERROR, "\n       DCE event recorded in wrong PCR");
        val_set_status(index, RESULT_FAIL(8));
        goto free_dlme_region;
      }

      /* DCE digest must match digest of the 1-byte value zero */
      if (validate_zero_digest_event(&event_log, &entry, fw_hash_alg,
                                     zero_digest, digest_size) != ACS_STATUS_PASS) {
        val_print(ERROR, "\n       DCE event digest does not match zero digest");
        val_set_status(index, RESULT_FAIL(9));
        goto free_dlme_region;
      }

      dce_seen = 1;
    } else if (entry.event->event_type == DRTM_EVTYPE_ARM_DCE_PUBKEY) {
      /* DCE_PUBKEY must be recorded in PCR[18] */
      if (entry.event->pcr_index != 18) {
        val_print(ERROR, "\n       DCE_PUBKEY event recorded in wrong PCR");
        val_set_status(index, RESULT_FAIL(10));
        goto free_dlme_region;
      }

      /* DCE_PUBKEY digest must match digest of the 1-byte value zero */
      if (validate_zero_digest_event(&event_log, &entry, fw_hash_alg,
                                     zero_digest, digest_size) != ACS_STATUS_PASS) {
        val_print(ERROR, "\n       DCE_PUBKEY event digest does not match zero digest");
        val_set_status(index, RESULT_FAIL(11));
        goto free_dlme_region;
      }

      dce_pubkey_seen = 1;
    }
  }

  /* DCE event must be present */
  if (!dce_seen) {
    val_print(ERROR, "\n       DCE event not found in event log");
    val_set_status(index, RESULT_FAIL(12));
    goto free_dlme_region;
  }

  /* DCE_PUBKEY event must be present */
  if (!dce_pubkey_seen) {
    val_print(ERROR, "\n       DCE_PUBKEY event not found in event log");
    val_set_status(index, RESULT_FAIL(13));
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

uint32_t dl016_entry(uint32_t num_pe)
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
