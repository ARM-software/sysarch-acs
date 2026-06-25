/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/acs_val.h"
#include "val/include/acs_memory.h"
#include "val/include/val_interface.h"

#define TEST_NUM   (ACS_DRTM_DL_TEST_NUM_BASE  +  17)
#define TEST_RULE  "R45440"
#define TEST_DESC  "Check Trustworthy ACPI Tables         "

static DRTM_DLME_DATA_HDR *dlme_data_header;
static DRTM_TCB_HASH_TABLE *tcb_hash_table;
static uint64_t acpi_region_address;

/* Required ACPI table signatures */
static const uint32_t required_tables[] = {
  ACS_ACPI_SIGNATURE('M', 'A', 'D', 'T'), /* MADT */
  ACS_ACPI_SIGNATURE('M', 'C', 'F', 'G'), /* MCFG */
  ACS_ACPI_SIGNATURE('G', 'T', 'D', 'T'), /* GTDT */
  ACS_ACPI_SIGNATURE('I', 'O', 'R', 'T'), /* IORT */
  ACS_ACPI_SIGNATURE('T', 'P', 'M', '2')  /* TPM2 */
};

static uint32_t num_required = sizeof(required_tables)/sizeof(required_tables[0]);

static void print_hex_dump(uint64_t address, uint64_t size)
{
  uint8_t *data = (uint8_t *)address;
  uint64_t offset;
  uint32_t i;

  for (offset = 0; offset < size; offset += 16) {
    val_print(DEBUG, "\n         %08llx :", offset);
    for (i = 0; (i < 16) && ((offset + i) < size); i++)
      val_print(DEBUG, " %02x", data[offset + i]);
  }
}

static void print_sig(uint32_t sig)
{
  char s0 = (char)(sig & 0xFF);
  char s1 = (char)((sig >> 8) & 0xFF);
  char s2 = (char)((sig >> 16) & 0xFF);
  char s3 = (char)((sig >> 24) & 0xFF);
  val_print(DEBUG, "\n       %c", s0);
  val_print(DEBUG, "%c", s1);
  val_print(DEBUG, "%c", s2);
  val_print(DEBUG, "%c", s3);
}

static void print_missing_table(uint32_t sig)
{
  char s0 = (char)(sig & 0xFF);
  char s1 = (char)((sig >> 8) & 0xFF);
  char s2 = (char)((sig >> 16) & 0xFF);
  char s3 = (char)((sig >> 24) & 0xFF);

  val_print(ERROR, "\n       Missing trustworthy ACPI table/hash: %c", s0);
  val_print(ERROR, "%c", s1);
  val_print(ERROR, "%c", s2);
  val_print(ERROR, "%c", s3);
}

static void print_acpi_table_debug(uint64_t acpi_addr, uint64_t acpi_size)
{
  uint32_t sig;
  uint32_t xsdt_len;
  uint32_t num_entries;
  uint64_t next_table_addr;
  uint64_t acpi_end;
  uint32_t i;

  val_print(DEBUG, "\n\n       DL017 ACPI Table Region Debug");
  val_print(DEBUG, "\n         Base Address : 0x%llx", acpi_addr);
  val_print(DEBUG, "\n         Region Size  : %ld Bytes", acpi_size);

  if (acpi_size == 0) {
    val_print(DEBUG, "\n         ACPI table region is not present");
    return;
  }

  if (acpi_size < ACPI_HEADER_SIZE) {
    val_print(DEBUG, "\n         ACPI table region smaller than ACPI header");
    print_hex_dump(acpi_addr, acpi_size);
    return;
  }

  acpi_end = acpi_addr + acpi_size;
  if (acpi_end < acpi_addr) {
    val_print(DEBUG, "\n         ACPI table region address overflow");
    return;
  }

  sig = *(uint32_t *)acpi_addr;
  val_print(DEBUG, "\n         Region Signature:");
  print_sig(sig);
  val_print(DEBUG, "\n         Raw ACPI Region Data:");
  print_hex_dump(acpi_addr, acpi_size);

  if (sig != ACS_ACPI_SIGNATURE('X', 'S', 'D', 'T')) {
    val_print(DEBUG, "\n         ACPI region does not start with XSDT");
    return;
  }

  xsdt_len = *((uint32_t *)(acpi_addr + ACPI_HEADER_LEN_OFFSET));
  val_print(DEBUG, "\n\n         XSDT Length  : %d Bytes", xsdt_len);

  if ((xsdt_len < ACPI_HEADER_SIZE) || (xsdt_len > acpi_size)) {
    val_print(DEBUG, "\n         XSDT length is outside ACPI table region");
    return;
  }

  num_entries = (xsdt_len - ACPI_HEADER_SIZE) >> 3;
  val_print(DEBUG, "\n         XSDT Entries : %d", num_entries);

  for (i = 0; i < num_entries; i++) {
    next_table_addr = *((uint64_t *)(acpi_addr + ACPI_HEADER_SIZE + (i * 8)));
    val_print(DEBUG, "\n           Entry[%d] Address : 0x%llx", i, next_table_addr);
    val_print(DEBUG, "\n           Entry[%d] Signature:", i);
    if (!val_drtm_is_range_valid((uint8_t *)acpi_addr, (uint8_t *)acpi_end,
                                 (uint8_t *)next_table_addr, ACPI_HEADER_SIZE)) {
      val_print(DEBUG, "\n           Entry[%d] outside ACPI region", i);
      continue;
    }

    print_sig(*(uint32_t *)next_table_addr);
  }
}

static void print_dlme_layout_debug(uint64_t dlme_data_address, uint64_t tcb_hash_address,
                                    uint64_t acpi_table_address)
{
  val_print(DEBUG, "\n\n       DL017 DLME Layout Debug");
  val_print(DEBUG, "\n         DLME Data Address          : 0x%llx", dlme_data_address);
  val_print(DEBUG, "\n         DLME Header Size           : %d Bytes",
            dlme_data_header->size);
  val_print(DEBUG, "\n         Protected Regions Size     : %ld Bytes",
            dlme_data_header->protected_regions_size);
  val_print(DEBUG, "\n         Address Map Size           : %ld Bytes",
            dlme_data_header->address_map_size);
  val_print(DEBUG, "\n         DRTM Event Log Size        : %ld Bytes",
            dlme_data_header->drtm_event_log_size);
  val_print(DEBUG, "\n         TCB Hash Table Address     : 0x%llx", tcb_hash_address);
  val_print(DEBUG, "\n         TCB Hash Table Size        : %ld Bytes",
            dlme_data_header->tcb_hash_table_size);
  val_print(DEBUG, "\n         ACPI Table Region Address  : 0x%llx", acpi_table_address);
  val_print(DEBUG, "\n         ACPI Table Region Size     : %ld Bytes",
            dlme_data_header->acpi_table_region_size);
  val_print(DEBUG, "\n         Implementation Region Size : %ld Bytes",
            dlme_data_header->implementation_region_size);
}

static void print_tcb_hash_debug(DRTM_TCB_HASH_TABLE *tbl, uint64_t tbl_size)
{
  uint32_t digest_size;
  uint8_t *hash_entry;
  DRTM_TCB_HASHES *hash;
  uint64_t entry_offset;
  uint32_t i;

  val_print(DEBUG, "\n\n       DL017 TCB Hash Table Debug");
  val_print(DEBUG, "\n         Base Address : 0x%llx", (uint64_t)tbl);
  val_print(DEBUG, "\n         Table Size   : %ld Bytes", tbl_size);

  if (tbl_size == 0) {
    val_print(DEBUG, "\n         TCB hash table is not present");
    return;
  }

  if (tbl_size < sizeof(DRTM_TCB_HASH_TABLE_HDR)) {
    val_print(DEBUG, "\n         TCB hash table smaller than header");
    print_hex_dump((uint64_t)tbl, tbl_size);
    return;
  }

  digest_size = val_drtm_get_digest_size(tbl->header.hash_algo);
  val_print(DEBUG, "\n         Revision     : 0x%x", tbl->header.revision);
  val_print(DEBUG, "\n         Num Hashes   : %d", tbl->header.num_hashes);
  val_print(DEBUG, "\n         Hash Algo    : 0x%x", tbl->header.hash_algo);
  val_print(DEBUG, "\n         Digest Size  : %d Bytes", digest_size);

  if (digest_size == 0) {
    val_print(DEBUG, "\n         Unsupported hash algorithm, raw table dump:");
    print_hex_dump((uint64_t)tbl, tbl_size);
    return;
  }

  hash_entry = (uint8_t *)tbl->hashes;
  for (i = 0; i < tbl->header.num_hashes; i++) {
    hash = (DRTM_TCB_HASHES *)hash_entry;
    entry_offset = (uint64_t)(hash_entry - (uint8_t *)tbl);

    if ((entry_offset > tbl_size) ||
        ((tbl_size - entry_offset) < (sizeof(hash->hash_id) + digest_size))) {
      val_print(DEBUG, "\n         Hash[%d] extends beyond table size", i);
      return;
    }

    val_print(DEBUG, "\n           Hash[%d] Offset : 0x%llx", i, entry_offset);
    val_print(DEBUG, "\n           Hash[%d] ID     : 0x%08x", i, hash->hash_id);
    val_print(DEBUG, "\n           Hash[%d] Value  :", i);
    print_hex_dump((uint64_t)hash->hash_val, digest_size);

    hash_entry += sizeof(hash->hash_id) + digest_size;
  }
}

/* Check if an ACPI table signature is present in XSDT */
static int acpi_table_present(uint64_t xsdt_addr, uint64_t xsdt_region_size, uint32_t sig)
{
  uint32_t xsdt_len, num_entries;
  uint64_t next_table_addr;
  uint64_t xsdt_region_end;
  uint32_t i;

  if (xsdt_region_size < ACPI_HEADER_SIZE)
    return 0;

  xsdt_region_end = xsdt_addr + xsdt_region_size;
  if (xsdt_region_end < xsdt_addr)
    return 0;

  if (*(uint32_t *)xsdt_addr != ACS_ACPI_SIGNATURE('X', 'S', 'D', 'T'))
    return 0;

  xsdt_len = *((uint32_t *)(xsdt_addr + ACPI_HEADER_LEN_OFFSET));
  if ((xsdt_len < ACPI_HEADER_SIZE) || (xsdt_len > xsdt_region_size)) {
    val_print(ERROR, "\n       Invalid XSDT length: %d", xsdt_len);
    return 0;
  }

  num_entries = (xsdt_len - ACPI_HEADER_SIZE) >> 3;

  for (i = 0; i < num_entries; i++) {
    next_table_addr = *((uint64_t *)(xsdt_addr + ACPI_HEADER_SIZE + (i * 8)));
    if (!val_drtm_is_range_valid((uint8_t *)xsdt_addr, (uint8_t *)xsdt_region_end,
                                 (uint8_t *)next_table_addr, ACPI_HEADER_SIZE)) {
      val_print(DEBUG, "\n       XSDT entry[%d] outside ACPI region: 0x%llx",
                i, next_table_addr);
      continue;
    }

    if (*(uint32_t *)next_table_addr == sig)
      return 1;
  }
  return 0;
}

/* Check if ACPI table hash is present in TCB hash table */
static int acpi_table_hash_present(DRTM_TCB_HASH_TABLE *tbl, uint64_t tbl_size, uint32_t sig)
{
  uint32_t digest_size;
  uint8_t *hash_entry;
  DRTM_TCB_HASHES *hash;
  uint32_t hash_id;
  uint64_t entry_offset;
  uint64_t entry_size;
  uint32_t i;

  if (tbl_size < sizeof(DRTM_TCB_HASH_TABLE_HDR))
    return 0;

  digest_size = val_drtm_get_digest_size(tbl->header.hash_algo);
  if (digest_size == 0) {
    val_print(ERROR, "\n       Unsupported TCB hash algorithm: 0x%x",
              tbl->header.hash_algo);
    return 0;
  }

  entry_size = sizeof(hash->hash_id) + digest_size;
  hash_entry = (uint8_t *)tbl->hashes;
  for (i = 0; i < tbl->header.num_hashes; i++) {
    hash = (DRTM_TCB_HASHES *)hash_entry;
    entry_offset = (uint64_t)(hash_entry - (uint8_t *)tbl);

    if ((entry_offset > tbl_size) || ((tbl_size - entry_offset) < entry_size)) {
      val_print(ERROR, "\n       TCB hash table truncated at hash index %d", i);
      return 0;
    }

    hash_id = hash->hash_id & 0x7FFFFFFF;

    if (hash_id == (sig & 0x7FFFFFFF))
      return 1;

    hash_entry += entry_size;
  }
  return 0;
}

static void payload(uint32_t num_pe)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  int64_t status;
  DRTM_PARAMETERS *drtm_params;
  uint64_t drtm_params_size = DRTM_SIZE_4K;
  uint64_t dlme_data_address;
  uint64_t tcb_hash_address;
  uint32_t missing = 0;
  uint32_t i;
  int found;

  drtm_params = (DRTM_PARAMETERS *)((uint64_t)val_aligned_alloc(DRTM_SIZE_4K, drtm_params_size));
  if (!drtm_params) {
    val_print(ERROR, "\n    Failed to allocate memory for DRTM Params");
    val_set_status(index, RESULT_FAIL(1));
    return;
  }

  status = val_drtm_init_drtm_params(drtm_params);
  if (status < DRTM_ACS_SUCCESS) {
    val_set_status(index, RESULT_FAIL(2));
    goto free_drtm_params;
  }

  status = val_drtm_dynamic_launch(drtm_params);
  if (status < DRTM_ACS_SUCCESS) {
    val_print(ERROR, "\n       DRTM Dynamic Launch failed");
    val_set_status(index, RESULT_FAIL(3));
    goto free_dlme_region;
  }

  status = val_drtm_unprotect_memory();
  if (status < DRTM_ACS_SUCCESS) {
    val_print(ERROR, "\n       DRTM Unprotect Memory failed err=%ld", status);
    val_set_status(index, RESULT_FAIL(4));
    goto free_dlme_region;
  }

  status = val_drtm_check_dl_result(drtm_params->dlme_region_address,
                                    drtm_params->dlme_data_offset);
  if (status != ACS_STATUS_PASS) {
    val_print(ERROR, "\n       DRTM check DL result failed");
    val_set_status(index, RESULT_FAIL(5));
    goto free_dlme_region;
  }

  dlme_data_address = drtm_params->dlme_region_address + drtm_params->dlme_data_offset;
  dlme_data_header = (DRTM_DLME_DATA_HDR *)dlme_data_address;

  tcb_hash_address = dlme_data_address + dlme_data_header->size +
                     dlme_data_header->protected_regions_size +
                     dlme_data_header->address_map_size +
                     dlme_data_header->drtm_event_log_size;

  tcb_hash_table = (DRTM_TCB_HASH_TABLE *)(tcb_hash_address);
  acpi_region_address = tcb_hash_address + dlme_data_header->tcb_hash_table_size;

  print_dlme_layout_debug(dlme_data_address, tcb_hash_address, acpi_region_address);
  print_tcb_hash_debug(tcb_hash_table, dlme_data_header->tcb_hash_table_size);
  print_acpi_table_debug(acpi_region_address, dlme_data_header->acpi_table_region_size);

  for (i = 0; i < num_required; i++) {
    found = 0;
    val_print(DEBUG, "\n       Checking:");
    print_sig(required_tables[i]);

    if (dlme_data_header->tcb_hash_table_size != 0) {
      if (acpi_table_hash_present(tcb_hash_table, dlme_data_header->tcb_hash_table_size,
                                  required_tables[i])) {
        val_print(DEBUG, "\n       Found in TCB hash table");
        found = 1;
      }
    }

    if (!found && (dlme_data_header->acpi_table_region_size != 0)) {
      if (acpi_table_present(acpi_region_address, dlme_data_header->acpi_table_region_size,
                             required_tables[i])) {
        val_print(DEBUG, "\n       Found in ACPI region");
        found = 1;
      }
    }

    if (!found) {
      print_missing_table(required_tables[i]);
      missing++;
    }
  }

  if (missing)
    val_set_status(index, RESULT_FAIL(6));
  else
    val_set_status(index, RESULT_PASS);

free_dlme_region:
  val_memory_free_aligned((void *)drtm_params->dlme_region_address);
free_drtm_params:
  val_memory_free_aligned((void *)drtm_params);
  return;
}

uint32_t dl017_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
    payload(num_pe);

  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
