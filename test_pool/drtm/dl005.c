/** @file
 * Copyright (c) 2024-2026, Arm Limited or its affiliates. All rights reserved.
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
#include "acs_memory.h"
#include "val_interface.h"

#define TEST_NUM   (ACS_DRTM_DL_TEST_NUM_BASE  +  5)
#define TEST_RULE  "R45250, R45260, R45270, R314050, R314060, R314150"
#define TEST_DESC  "Check DLME Data Rules                 "

static uint64_t acpi_region_address;
static DRTM_MEMORY_REGION_DESCRIPTOR_TABLE *prot_region;
static DRTM_MEMORY_REGION_DESCRIPTOR_TABLE *addr_map;
static DRTM_TCB_HASH_TABLE *tcb_hash_table;
static DRTM_DLME_DATA_HDR *dlme_data_header;

static void print_dlme_data_header(DRTM_DLME_DATA_HDR *dlme_data_header)
{
  val_print(DEBUG, "\n       DLME Data Header");
  val_print(DEBUG, "\n         Revision                  = 0x%08lx",
                                                dlme_data_header->revision);
  val_print(DEBUG, "\n         Size                      = %d Bytes",
                                                dlme_data_header->size);
  val_print(DEBUG, "\n         DLME_data_size            = %d Bytes",
                                                dlme_data_header->dlme_data_size);
  val_print(DEBUG, "\n         Protected Regions Size    = %d Bytes",
                                                dlme_data_header->protected_regions_size);
  val_print(DEBUG, "\n         Address Map Size          = %d Bytes",
                                                dlme_data_header->address_map_size);
  val_print(DEBUG, "\n         DRTM Event Log Size       = %d Bytes",
                                                dlme_data_header->drtm_event_log_size);
  val_print(DEBUG, "\n         TCB Hash Table Size       = %d Bytes",
                                                dlme_data_header->tcb_hash_table_size);
  val_print(DEBUG, "\n         ACPI Table Region Size    = %d Bytes",
                                                dlme_data_header->acpi_table_region_size);
  val_print(DEBUG, "\n         Implementation Region Size= %d Bytes",
                                                dlme_data_header->implementation_region_size);
}

static void print_protected_region_info(void)
{
  val_print(DEBUG, "\n\n       Protected Region");
  val_print(DEBUG, "\n         Revision          : 0x%08lx",
                                            prot_region->header.revision);
  val_print(DEBUG, "\n         Number of Regions : 0x%08lx",
                                            prot_region->header.num_regions);
  for (uint32_t i = 0; i < prot_region->header.num_regions; i++) {
    val_print(DEBUG, "\n           Region           : 0x%lx", i);
    val_print(DEBUG, "\n           Start Address    : 0x%08lx",
                                            prot_region->regions[i].start_addr);
    val_print(DEBUG, "\n           Region Size/Type : 0x%08lx",
                                            prot_region->regions[i].size_type);
  }
}

static void print_address_map_info(void)
{
  val_print(DEBUG, "\n\n       Address Map");
  val_print(DEBUG, "\n         Revision          : 0x%08lx",
                                            addr_map->header.revision);
  val_print(DEBUG, "\n         Number of Regions : 0x%08lx",
                                            addr_map->header.num_regions);
  for (uint32_t i = 0; i < addr_map->header.num_regions; i++) {
    val_print(DEBUG, "\n           Region           : 0x%lx", i);
    val_print(DEBUG, "\n           Start Address    : 0x%08lx",
                                            addr_map->regions[i].start_addr);
    val_print(DEBUG, "\n           Region Size/Type : 0x%08lx",
                                            addr_map->regions[i].size_type);
  }
}

static void print_dlme_data_info(uint64_t dlme_data_address)
{
  uint64_t prot_region_address;
  uint64_t addr_map_address;
  uint64_t evt_log_address;
  uint64_t tcb_hash_address;

  uint32_t xsdt_len, num_acpi_entries;
  uint64_t next_table_addr;

  dlme_data_header = (DRTM_DLME_DATA_HDR *)(dlme_data_address);

  prot_region_address = dlme_data_address + dlme_data_header->size;
  addr_map_address    = prot_region_address + dlme_data_header->protected_regions_size;
  evt_log_address     = addr_map_address + dlme_data_header->address_map_size;
  tcb_hash_address    = evt_log_address + dlme_data_header->drtm_event_log_size;
  acpi_region_address = tcb_hash_address + dlme_data_header->tcb_hash_table_size;

  print_dlme_data_header(dlme_data_header);

  prot_region = (DRTM_MEMORY_REGION_DESCRIPTOR_TABLE *)(prot_region_address);
  print_protected_region_info();

  addr_map = (DRTM_MEMORY_REGION_DESCRIPTOR_TABLE *)(addr_map_address);
  print_address_map_info();

  if (dlme_data_header->tcb_hash_table_size != 0) {
    val_print(DEBUG, "\n\n       TCB Hash Table");
    tcb_hash_table = (DRTM_TCB_HASH_TABLE *)(tcb_hash_address);
    val_print(DEBUG, "\n         Revision          : 0x%08lx",
                                            tcb_hash_table->header.revision);
    val_print(DEBUG, "\n         Number of Hashes  : 0x%08lx",
                                            tcb_hash_table->header.num_hashes);
    val_print(DEBUG, "\n         Hash Algorithm    : 0x%08lx",
                                            tcb_hash_table->header.hash_algo);
    for (uint32_t i = 0; i < tcb_hash_table->header.num_hashes; i++) {
      val_print(DEBUG, "\n           HASH Index : 0x%lx", i);
      val_print(DEBUG, "\n             HASH ID    : 0x%08lx",
                                            tcb_hash_table->hashes[i].hash_id);
      for (uint32_t j = 0; j < 32; j = j+4) {
        val_print(DEBUG, "\n             HASH Val   : ");
        val_print(DEBUG, "%02x ", (tcb_hash_table->hashes[i]).hash_val[j]);
        val_print(DEBUG, "%02x ", (tcb_hash_table->hashes[i]).hash_val[j+1]);
        val_print(DEBUG, "%02x ", (tcb_hash_table->hashes[i]).hash_val[j+2]);
        val_print(DEBUG, "%02x ", (tcb_hash_table->hashes[i]).hash_val[j+3]);
      }
    }
  }

  if (dlme_data_header->acpi_table_region_size != 0) {
    /* Print the XSDT Address and Present Tables */
    val_print(DEBUG, "\n\n       ACPI Tables");
    val_print(DEBUG, "\n         Signature : %llx",
                                            (uint32_t)(*((uint64_t *)acpi_region_address)));
    if ((uint32_t)(*((uint64_t *)acpi_region_address)) == ACS_ACPI_SIGNATURE('X', 'S', 'D', 'T')) {
      /* Print all Present ACPI Tables */
      PRINT_ACPI_NAME_FROM_SIGNATURE(*((uint64_t *)acpi_region_address));
      xsdt_len = *((uint64_t *)(acpi_region_address + ACPI_HEADER_LEN_OFFSET));
      num_acpi_entries = (xsdt_len - ACPI_HEADER_SIZE) >> 3;
      for (uint32_t i = 0; i < num_acpi_entries; i++) {
        next_table_addr = *((uint64_t *)(acpi_region_address + ACPI_HEADER_SIZE + (i*8)));
        PRINT_ACPI_NAME_FROM_SIGNATURE(*(uint64_t *)next_table_addr);
      }
    }
  }
  val_print(DEBUG, "\n\n");

}

static
void
payload(uint32_t num_pe)
{

  /* This test will verify the DRTM Dynamic Launch
   * Input parameter will be 64 bit address of DRTM Parameters
   * */
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  int64_t  status;

  DRTM_PARAMETERS *drtm_params;
  uint64_t drtm_params_size = DRTM_SIZE_4K;
  uint64_t dlme_data_address;
  uint32_t test_fails = 0;

  /* Allocate Memory For DRTM Parameters 4KB Aligned */
  drtm_params = (DRTM_PARAMETERS *)((uint64_t)val_aligned_alloc(DRTM_SIZE_4K, drtm_params_size));
  if (!drtm_params) {
    val_print(ERROR, "\n    Failed to allocate memory for DRTM Params");
    val_set_status(index, RESULT_FAIL(1));
    return;
  }

  status = val_drtm_init_drtm_params(drtm_params);
  if (status < DRTM_ACS_SUCCESS) {
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

  /* Check DL return values after DLME Image launch */
  status = val_drtm_check_dl_result(drtm_params->dlme_region_address,
                                    drtm_params->dlme_data_offset);
  if (status == ACS_STATUS_FAIL) {
    val_print(ERROR, "\n       DRTM check DL result failed");
    val_set_status(index, RESULT_FAIL(5));
    goto free_dlme_region;
  }

  dlme_data_address = drtm_params->dlme_region_address + drtm_params->dlme_data_offset;

  /* Part 1 : Print DLME Data Information */
  print_dlme_data_info(dlme_data_address);

  /* R314050 : All the sub-regions referenced by DLME_DATA_HEADER must be within DLME region.*/
  if ((dlme_data_address + dlme_data_header->dlme_data_size) >
      (drtm_params->dlme_region_address + drtm_params->dlme_region_size)) {
    val_print(ERROR, "\n       DLME Data Header outside DLME Region");
    val_set_status(index, RESULT_FAIL(6));
    goto free_dlme_region;
  }

  /* R314060 : Check if Protected Region size is not zero */
  if (dlme_data_header->protected_regions_size == 0) {
    val_print(ERROR, "\n       DLME Data Protected Region Size is 0");
    val_set_status(index, RESULT_FAIL(7));
    goto free_dlme_region;
  }
  /* R314060 : Check if Address Map size is not zero */
  /* R45260  : Check if the address map is populated by the DCE */
  if (dlme_data_header->address_map_size == 0) {
    val_print(ERROR, "\n       DLME Data Address Map Size is 0");
    val_set_status(index, RESULT_FAIL(8));
    goto free_dlme_region;
  }
  /* R314060 : Check if Event Log size is not zero */
  if (dlme_data_header->drtm_event_log_size <= 0) {
    val_print(ERROR, "\n       DLME Data Event Log Size is incorrect");
    val_set_status(index, RESULT_FAIL(9));
    goto free_dlme_region;
  }

  /* R314090 : Size of fields in the DATA_HEADER must not extend beyond bounds of DLME data.*/
  if ((dlme_data_header->dlme_data_size) >
      (drtm_params->dlme_region_address + drtm_params->dlme_region_size - dlme_data_address)) {
    val_print(ERROR, "\n       DLME Data Header exceeds DLME Data");
    val_set_status(index, RESULT_FAIL(10));
    goto free_dlme_region;
  }

  /* R314110 Part 2 : Check Protected Regions is populated */
  /* Do revision check and num of regions check */
  if ((prot_region->header.revision != 1) || (prot_region->header.num_regions == 0)) {
    val_print(ERROR, "\n       Protected region not populated");
    val_set_status(index, RESULT_FAIL(11));
    goto free_dlme_region;
  }

  /* R45160 If complete DMA protection in use, DCE must define a single region */
  if (VAL_EXTRACT_BITS(g_drtm_features.dma_prot_features.value, 0, 7) == 1) {
    if ((prot_region->header.num_regions != 1) ||
        (prot_region->regions[0].start_addr != 0)) {
      val_print(ERROR, "\n       Protected region wrongly populated");
      val_set_status(index, RESULT_FAIL(12));
      goto free_dlme_region;
    }
  }

  /* R314100 Part 3 : Check Address Map is populated */
  /* Do revision check and num of regions check */
  if ((addr_map->header.revision != 1) || (addr_map->header.num_regions == 0)) {
    val_print(ERROR, "\n       Address Map not populated");
    val_set_status(index, RESULT_FAIL(13));
    goto free_dlme_region;
  }

  /* R314120 Part 4 : Regions must be sorted with first desc in table the lowest address */
  /* R313010 : All region addresses must be 4KB Aligned */
  /* R313020 : All region addresses must not overlap */
  for (uint32_t i = 0; i < prot_region->header.num_regions; i++) {
    if (!(DRTM_IS_4KB_ALIGNED(prot_region->regions[i].start_addr))) {
      val_print(ERROR, "\n       Protected Memory Regions Not 4KB Aligned");
      val_set_status(index, RESULT_FAIL(14));
      test_fails++;
    }

    if (i == 0)
      continue;

    if (prot_region->regions[i].start_addr < prot_region->regions[i-1].start_addr) {
      val_print(ERROR, "\n       Protected Regions Memory Regions Not Sorted");
      val_set_status(index, RESULT_FAIL(15));
      test_fails++;
    }
  }

  for (uint32_t i = 0; i < addr_map->header.num_regions; i++) {
    if (!(DRTM_IS_4KB_ALIGNED(addr_map->regions[i].start_addr))) {
      val_print(ERROR, "\n       Address Map Memory Regions Not 4KB Aligned");
      val_set_status(index, RESULT_FAIL(16));
      test_fails++;
    }

    if (i == 0)
      continue;

    if (addr_map->regions[i].start_addr < addr_map->regions[i-1].start_addr) {
      val_print(ERROR, "\n       Address Map Memory Regions Not Sorted");
      val_set_status(index, RESULT_FAIL(17));
      test_fails++;
    }
  }

  /* R314130 Part 5 : TCB hash table, if present, must be formatted as a TCB_HASH_TABLE */
  if (dlme_data_header->tcb_hash_table_size != 0) {
    /* Check num of hashes is not zero & revision = 1 */
    if ((tcb_hash_table->header.revision != 1) || (tcb_hash_table->header.num_hashes == 0)) {
      /* Fail The test */
      val_print(ERROR, "\n       TCB_HASH_TABLE Not Correctly Formatted");
      val_set_status(index, RESULT_FAIL(18));
      goto free_dlme_region;
    }
    /* R315040 In Success Case. Check if Maximum number of entries is greator than num_hashes */
    if (tcb_hash_table->header.num_hashes >
        VAL_EXTRACT_BITS(g_drtm_features.tcb_hash_features.value, 0, 7)) {
      val_print(ERROR, "\n       Number of hashes exceeds maximum allowed value");
      val_set_status(index, RESULT_FAIL(19));
      test_fails++;
    }
  }

  /* R314140 Part 6 : ACPI tables region, if present, must start with an XSDT table at */
  /*                  offset 0 which contains the physical addresses of all the other  */
  /*                  ACPI tables in the region                                        */
  if (dlme_data_header->acpi_table_region_size != 0) {
    if ((uint32_t)(*((uint64_t *)acpi_region_address)) !=
                        ACS_ACPI_SIGNATURE('X', 'S', 'D', 'T')) {
      val_print(ERROR, "\n       ACPI XSDT Table Check Failed");
      val_set_status(index, RESULT_FAIL(20));
      test_fails++;
    }
  }

  /* R314150 Part 7 : Check one of tcb_hash_table or acpi_table_region is present */
  if ((dlme_data_header->tcb_hash_table_size != 0) &&
      (dlme_data_header->acpi_table_region_size != 0)) {
    val_print(ERROR, "\n       TCB Hash Table & ACPI Table Region Check Failed");
    val_set_status(index, RESULT_FAIL(21));
    test_fails++;
  }

  if (test_fails == 0)
    val_set_status(index, RESULT_PASS);

free_dlme_region:
  val_memory_free_aligned((void *)drtm_params->dlme_region_address);
free_drtm_params:
  val_memory_free_aligned((void *)drtm_params);

  return;
}

uint32_t dl005_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
  /* execute payload, which will execute relevant functions on current and other PEs */
      payload(num_pe);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
