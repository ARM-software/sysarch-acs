/** @file
 * Copyright (c) 2020-2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/acs_pcie_enumeration.h"
#include "val/include/acs_pcie.h"
#include "val/include/acs_pe.h"
#include "val/include/acs_smmu.h"
#include "val/include/acs_memory.h"
#include "val/include/acs_exerciser.h"
#include "val/include/val_interface.h"

static const
test_config_t test_entries[] = {
    { ACS_EXERCISER_TEST_NUM_BASE + 21, "Arrival order & Gathering Check: RCiEP", "RE_ORD_1"},
    { ACS_EXERCISER_TEST_NUM_BASE + 31, "Arrival order & Gathering Check: iEP  ", "IE_ORD_1"}
};

/* Declare and define struct - passed as argument to payload */
typedef struct {
    uint32_t dev_type1;
    uint32_t test_num;
} test_data_t;

/* 0 means read transction, 1 means write transaction */
static uint32_t transaction_order[] = {1, 1, 0, 1, 0, 0, 0, 0};
static uint32_t pattern[16] = {0};
static uint32_t run_flag;
static uint32_t fail_cnt;

static uint32_t read_config_space(uint32_t *addr)
{
  uint32_t idx;

  for (idx = 0; idx < 16; idx++) {
    addr = addr + idx;
    pattern[idx] = val_mmio_read((addr_t)addr);
  }

  return 0;
}

/* num of transactions captured and thier attributes is checked */
static uint32_t test_sequence_check(uint32_t instance)
{
  uint64_t idx;
  uint64_t num_transactions;
  uint64_t transaction_type;

  num_transactions = sizeof(transaction_order)/sizeof(transaction_order[0]);

  /* Check transactions arrival order */
  for (idx = 0; idx < sizeof(transaction_order)/sizeof(transaction_order[0]); idx++) {
      val_exerciser_get_param(TRANSACTION_TYPE, &idx, &transaction_type, instance);
      if (transaction_type !=  transaction_order[idx]) {
          val_print(ACS_PRINT_ERR, "\n       Exerciser %d arrival order check failed", instance);
          return 1;
      }
  }

  /* Get number of transactions captured from exerciser */
  if (num_transactions != idx) {
      val_print(ACS_PRINT_ERR, "\n       Exerciser %d gathering check failed", instance);
      return 1;
  }

  val_exerciser_get_param(CLEAR_TXN, 0, 0, instance);
  return 0;
}

/* Performs reads/write on 1B data */
static uint32_t test_sequence_1B(uint8_t *addr, uint8_t increment_addr, uint32_t instance)
{
  uint64_t idx;
  uint8_t write_val;
  uint32_t pidx;
  uint32_t e_bdf;
  uint8_t *pattern_ptr;

  e_bdf = val_exerciser_get_bdf(instance);

  /* Start monitoring exerciser transactions */
  if (val_exerciser_ops(START_TXN_MONITOR, CFG_READ, instance)) {
      val_print(ACS_PRINT_DEBUG,
               "\n       Exerciser BDF 0x%x - Unable to start transaction monitoring", e_bdf);
      return ACS_STATUS_SKIP;
  }


  run_flag = 1;

  /* Send the transaction on incrementalt addresses */
  for (idx = 0; idx < sizeof(transaction_order)/sizeof(transaction_order[0]); idx++) {
      pidx = ((uint64_t)addr & 0xFF);
      pattern_ptr = (uint8_t *)&pattern;
      write_val = pattern_ptr[pidx];
      /* Write transaction */
      if (transaction_order[idx])
          val_mmio_write8((addr_t)addr, write_val);
      else
          val_mmio_read8((addr_t)addr);
      if (increment_addr)
          addr++;
  }

  /* Stop monitoring exerciser transactions */
  val_exerciser_ops(STOP_TXN_MONITOR, CFG_READ, instance);

  return test_sequence_check(instance);
}

/* Performs reads/write on 2B data */
static uint32_t test_sequence_2B(uint16_t *addr, uint8_t increment_addr, uint32_t instance)
{
  uint64_t idx;
  uint16_t write_val;
  uint32_t pidx;
  uint32_t e_bdf;
  uint16_t *pattern_ptr;

  e_bdf = val_exerciser_get_bdf(instance);

  /* Start monitoring exerciser transactions */
  if (val_exerciser_ops(START_TXN_MONITOR, CFG_READ, instance)) {
      val_print(ACS_PRINT_DEBUG,
               "\n       Exerciser BDF 0x%x - Unable to start transaction monitoring", e_bdf);
      return ACS_STATUS_SKIP;
  }

  run_flag = 1;

  /* Send the transaction on incrementalt addresses */
  for (idx = 0; idx < sizeof(transaction_order)/sizeof(transaction_order[0]); idx++) {
      pidx = ((uint64_t)addr & 0xFF)/2;
      pattern_ptr = (uint16_t *)&pattern;
      write_val = pattern_ptr[pidx];
      /* Write transaction */
      if (transaction_order[idx])
          val_mmio_write16((addr_t)addr, write_val);
      else
          val_mmio_read16((addr_t)addr);
      if (increment_addr)
          addr++;
  }

  /* Stop monitoring exerciser transactions */
  val_exerciser_ops(STOP_TXN_MONITOR, CFG_READ, instance);

  return test_sequence_check(instance);
}

/* Performs reads/write on 4B data */
static uint32_t test_sequence_4B(uint32_t *addr, uint8_t increment_addr, uint32_t instance)
{
  uint64_t idx;
  uint32_t write_val, pidx;
  uint32_t *pattern_ptr;
  uint32_t e_bdf;

  e_bdf = val_exerciser_get_bdf(instance);

  /* Start monitoring exerciser transactions */
  if (val_exerciser_ops(START_TXN_MONITOR, CFG_READ, instance)) {
      val_print(ACS_PRINT_DEBUG,
               "\n       Exerciser BDF 0x%x - Unable to start transaction monitoring", e_bdf);
      return ACS_STATUS_SKIP;
  }

  run_flag = 1;

  /* Send the transaction on incrementalt addresses */
  for (idx = 0; idx < sizeof(transaction_order)/sizeof(transaction_order[0]); idx++) {
      pidx = ((uint64_t)addr & 0xFF)/4;
      pattern_ptr = (uint32_t *)&pattern;
      write_val = pattern_ptr[pidx];
      /* Write transaction */
      if (transaction_order[idx])
          val_mmio_write((addr_t)addr, write_val);
      else
          val_mmio_read((addr_t)addr);
      if (increment_addr)
          addr++;
  }

  /* Stop monitoring exerciser transactions */
  val_exerciser_ops(STOP_TXN_MONITOR, CFG_READ, instance);

  return test_sequence_check(instance);
}

/* Performs reads/write on 8B data */
static uint32_t test_sequence_8B(uint64_t *addr, uint8_t increment_addr, uint32_t instance)
{
  uint64_t idx;
  uint64_t write_val;
  uint32_t pidx;
  uint32_t e_bdf;
  uint64_t *pattern_ptr;

  e_bdf = val_exerciser_get_bdf(instance);

  /* Start monitoring exerciser transactions */
  if (val_exerciser_ops(START_TXN_MONITOR, CFG_READ, instance)) {
      val_print(ACS_PRINT_DEBUG,
               "\n       Exerciser BDF 0x%x - Unable to start transaction monitoring", e_bdf);
      return ACS_STATUS_SKIP;
  }

  run_flag = 1;

  /* Send the transaction on incrementalt addresses */
  for (idx = 0; idx < sizeof(transaction_order)/sizeof(transaction_order[0]); idx++) {
      pidx = ((uint64_t)addr & 0xFF)/8;
      pattern_ptr = (uint64_t *)&pattern;
      write_val = pattern_ptr[pidx];
      /* Write transaction */
      if (transaction_order[idx])
          val_mmio_write64((addr_t)addr, write_val);
      else
          val_mmio_read64((addr_t)addr);
      if (increment_addr)
          addr++;
  }

  /* Stop monitoring exerciser transactions */
  val_exerciser_ops(STOP_TXN_MONITOR, CFG_READ, instance);

  return test_sequence_check(instance);
}

static void cfgspace_test_sequence(uint32_t *baseptr, uint32_t instance)
{

    /* Test Scenario 1 : Transactions on incremental aligned address */
    fail_cnt += test_sequence_1B((uint8_t *)baseptr, 1, instance);
    fail_cnt += test_sequence_2B((uint16_t *)baseptr, 1, instance);
    fail_cnt += test_sequence_4B((uint32_t *)baseptr, 1, instance);

    /* Test Scenario 2 : Transactions on same address */
    fail_cnt += test_sequence_1B((uint8_t *)baseptr, 0, instance);
    fail_cnt += test_sequence_2B((uint16_t *)baseptr, 0, instance);
    fail_cnt += test_sequence_4B((uint32_t *)baseptr, 0, instance);

}

static void barspace_test_sequence(uint64_t *baseptr, uint32_t instance)
{

    /* Test Scenario 1 : Transactions on incremental aligned address */
    fail_cnt += test_sequence_1B((uint8_t *)baseptr, 1, instance);
    fail_cnt += test_sequence_2B((uint16_t *)baseptr, 1, instance);
    fail_cnt += test_sequence_4B((uint32_t *)baseptr, 1, instance);
    fail_cnt += test_sequence_8B((uint64_t *)baseptr, 1, instance);

    /* Test Scenario 2 : Transactions on same address */
    fail_cnt += test_sequence_1B((uint8_t *)baseptr, 0, instance);
    fail_cnt += test_sequence_2B((uint16_t *)baseptr, 0, instance);
    fail_cnt += test_sequence_4B((uint32_t *)baseptr, 0, instance);
    fail_cnt += test_sequence_8B((uint64_t *)baseptr, 0, instance);

}

/* Read and Write on config space mapped to Device memory */

static
void
cfgspace_transactions_order_check(test_data_t *test_data)
{
  uint32_t instance;
  uint32_t bdf;
  char *baseptr;
  uint32_t cid_offset, dp_type;
  uint64_t bdf_addr;

  /* Read the number of excerciser cards */
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS);

  while (instance-- != 0) {

    /* if init fail moves to next exerciser */
    if (val_exerciser_init(instance))
        continue;

    /* Get exerciser bdf */
    bdf = val_exerciser_get_bdf(instance);
    dp_type = val_pcie_device_port_type(bdf);

    /* Check entry is RCiEP/ iEP. Else move to next BDF. */
    if (dp_type != test_data->dev_type1)
        continue;

    val_print(ACS_PRINT_DEBUG, "\n       Exerciser BDF - 0x%x", bdf);

    /* If exerciser doesn't have PCI_CAP skip the bdf */
    if (val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &cid_offset) == PCIE_CAP_NOT_FOUND)
        continue;

    bdf_addr = val_pcie_get_bdf_config_addr(bdf);

    /* Map config space to ARM device(nGnRnE) memory in MMU page tables */
    baseptr = (char *)val_memory_ioremap((void *)bdf_addr, 512, DEVICE_nGnRnE);

    if (!baseptr) {
        val_print(ACS_PRINT_ERR, "\n       Failed in config ioremap for instance %x", instance);
        continue;
    }

    read_config_space((uint32_t *)baseptr);

    /* Perform Transactions on incremental aligned address and on same address */
    cfgspace_test_sequence((uint32_t *)baseptr, instance);

    /* Map config space to ARM device(nGnRE) memory in MMU page tables */
    baseptr = (char *)val_memory_ioremap((void *)bdf_addr, 512, DEVICE_nGnRE);

    if (!baseptr) {
        val_print(ACS_PRINT_ERR, "\n       Failed in config ioremap for instance %x", instance);
        continue;
    }

    read_config_space((uint32_t *)baseptr);
    /* Perform Transactions on incremental aligned address and on same address */
    cfgspace_test_sequence((uint32_t *)baseptr, instance);
  }
}

/* Read and Write on BAR space mapped to Device memory */

static
void
barspace_transactions_order_check(test_data_t *test_data)
{
  uint32_t instance;
  exerciser_data_t e_data;
  char *baseptr;
  uint32_t status;
  uint32_t bdf, dp_type;

  /* Read the number of excerciser cards */
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS);

  while (instance-- != 0) {

    /* if init fail moves to next exerciser */
    if (val_exerciser_init(instance))
        continue;

    /* Get exerciser bdf */
    bdf = val_exerciser_get_bdf(instance);
    dp_type = val_pcie_device_port_type(bdf);

    /* Check entry is RCiEP/ iEP. Else move to next BDF. */
    if (dp_type != test_data->dev_type1)
        continue;

    /* Get BAR 0 details for this instance */
    status = val_exerciser_get_data(EXERCISER_DATA_MMIO_SPACE, &e_data, instance);
    if (status == NOT_IMPLEMENTED) {
        val_print(ACS_PRINT_ERR, "\n       pal_exerciser_get_data() for MMIO not implemented", 0);
        continue;
    } else if (status) {
        val_print(ACS_PRINT_ERR, "\n       Exerciser %d data read error     ", instance);
        continue;
    }

    /* If BAR region is not Prefetchable, skip the exerciser */
    if (e_data.bar_space.type != MMIO_PREFETCHABLE)
        continue;

    /* Map mmio space to ARM device(nGnRnE) memory in MMU page tables */
    baseptr = (char *)val_memory_ioremap((void *)e_data.bar_space.base_addr, 512, DEVICE_nGnRnE);
    if (!baseptr) {
        val_print(ACS_PRINT_ERR, "\n       Failed in BAR ioremap for instance %x", instance);
        continue;
    }

    /* Perform Transactions on incremental aligned address and on same address */
    barspace_test_sequence((uint64_t *)baseptr, instance);

    /* Map mmio space to ARM device(nGnRE) memory in MMU page tables */
    baseptr = (char *)val_memory_ioremap((void *)e_data.bar_space.base_addr, 512, DEVICE_nGnRE);
    if (!baseptr) {
        val_print(ACS_PRINT_ERR, "\n       Failed in BAR ioremap for instance %x", instance);
        continue;
    }

    /* Perform Transactions on incremental aligned address and on same address */
    barspace_test_sequence((uint64_t *)baseptr, instance);

  }
}

static
void
payload(void *arg)
{
  uint32_t pe_index;
  test_data_t *test_data = (test_data_t *)arg;
  run_flag = 0;

  pe_index = val_pe_get_index_mpid (val_pe_get_mpid());

  cfgspace_transactions_order_check(test_data);
  barspace_transactions_order_check(test_data);

  if (!run_flag) {
      val_set_status(pe_index, RESULT_SKIP(test_data->test_num, 01));
      return;
  }

  if (fail_cnt)
      val_set_status(pe_index, RESULT_FAIL(test_data->test_num, fail_cnt));
  else
      val_set_status(pe_index, RESULT_PASS(test_data->test_num, 01));
}

uint32_t
e021_entry(void)
{
  uint32_t num_pe = 1;
  uint32_t status = ACS_STATUS_FAIL;
  test_data_t data = {.test_num = test_entries[0].test_num, .dev_type1 = (uint32_t)RCiEP};

  status = val_initialize_test(test_entries[0].test_num, test_entries[0].desc, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_configurable_payload(&data, payload);

  /* Get the result from all PE and check for failure */
  status = val_check_for_error(test_entries[0].test_num, num_pe, test_entries[0].rule);

  val_report_status(0, ACS_END(test_entries[0].test_num), test_entries[0].rule);

  return status;
}

uint32_t
e031_entry(void)
{
  uint32_t num_pe = 1;
  uint32_t status = ACS_STATUS_FAIL;
  test_data_t data = {.test_num = test_entries[1].test_num, .dev_type1 = (uint32_t)iEP_EP};

  status = val_initialize_test(test_entries[1].test_num, test_entries[1].desc, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_configurable_payload(&data, payload);

  /* Get the result from all PE and check for failure */
  status = val_check_for_error(test_entries[1].test_num, num_pe, test_entries[1].rule);

  val_report_status(0, ACS_END(test_entries[1].test_num), test_entries[1].rule);

  return status;
}