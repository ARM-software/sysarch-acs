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


/** This file is common to all test cases and Val layer of the Suite */


#ifndef __ACS_COMMON_H__
#define __ACS_COMMON_H__

#define G_SW_OS    0
#define G_SW_HYP   1
#define G_SW_PS    2

#define ACS_PE_TEST_NUM_BASE         0
#define ACS_MEMORY_MAP_TEST_NUM_BASE 100
#define ACS_GIC_TEST_NUM_BASE        200
#define ACS_GIC_V2M_TEST_NUM_BASE    (ACS_GIC_TEST_NUM_BASE + 25)
#define ACS_GIC_ITS_TEST_NUM_BASE    (ACS_GIC_V2M_TEST_NUM_BASE + 25)
#define ACS_SMMU_TEST_NUM_BASE       300
#define ACS_TIMER_TEST_NUM_BASE      400
#define ACS_WAKEUP_TEST_NUM_BASE     500
#define ACS_PER_TEST_NUM_BASE        600
#define ACS_WD_TEST_NUM_BASE         700
#define ACS_PCIE_TEST_NUM_BASE       800
#define ACS_EXERCISER_TEST_NUM_BASE  900
#define ACS_MPAM_TEST_NUM_BASE       1000
#define ACS_PMU_TEST_NUM_BASE        1100
#define ACS_RAS_TEST_NUM_BASE        1200
#define ACS_NIST_TEST_NUM_BASE       1300
#define ACS_ETE_TEST_NUM_BASE        1400
#define ACS_TPM2_TEST_NUM_BASE       1600

/* Module specific print APIs */

typedef enum {
    PE_MODULE,
    MEM_MAP_MODULE,
    GIC_MODULE,
    SMMU_MODULE,
    TIMER_MODULE,
    WAKEUP_MODULE,
    PERIPHERAL_MODULE,
    WD_MODULE,
    PCIE_MODULE,
    EXERCISER_MODULE,
    MPAM_MODULE,
    PMU_MODULE,
    RAS_MODULE,
    ETE_MODULE,
    NIST_MODULE,
    TPM2_MODULE
} MODULE_ID_e;

#define STATE_BIT   28
#define STATE_MASK 0xF

/*These are the states a test can be in */
#define TEST_START_VAL   0x1
#define TEST_END_VAL     0x2
#define TEST_PASS_VAL    0x4
#define TEST_FAIL_VAL    0x8
#define TEST_SKIP_VAL    0x9
#define TEST_PENDING_VAL 0xA

#define CPU_NUM_BIT  32
#define CPU_NUM_MASK 0xFFFFFFFF

#define STATUS_MASK 0xFFF

#define TEST_NUM_BIT    12
#define TEST_NUM_MASK   0xFFF

/* status definations*/

#define STATUS_SYS_REG_ACCESS_FAIL 0x78

/* TEST start and Stop defines */

#define ACS_START(test_num) (((TEST_START_VAL) << STATE_BIT) | ((test_num) << TEST_NUM_BIT))
#define ACS_END(test_num) (((TEST_END_VAL) << STATE_BIT) | ((test_num) << TEST_NUM_BIT))

/* TEST Result defines */

#define RESULT_PASS(test_num, status) (((TEST_PASS_VAL) << STATE_BIT) | ((test_num) << TEST_NUM_BIT) | (status))

#define RESULT_FAIL(test_num, status) (((TEST_FAIL_VAL) << STATE_BIT) | ((test_num) << TEST_NUM_BIT) | (status))

#define RESULT_SKIP(test_num, status) (((TEST_SKIP_VAL) << STATE_BIT) | ((test_num) << TEST_NUM_BIT) | (status))

#define RESULT_PENDING(test_num) (((TEST_PENDING_VAL) << STATE_BIT) | \
                        ((test_num) << TEST_NUM_BIT))

#define IS_TEST_START(value)     (((value >> STATE_BIT) & (STATE_MASK)) == TEST_START_VAL)
#define IS_TEST_END(value)       (((value >> STATE_BIT) & (STATE_MASK)) == TEST_END_VAL)
#define IS_RESULT_PENDING(value) (((value >> STATE_BIT) & (STATE_MASK)) == TEST_PENDING_VAL)
#define IS_TEST_PASS(value)      (((value >> STATE_BIT) & (STATE_MASK)) == TEST_PASS_VAL)
#define IS_TEST_FAIL(value)      (((value >> STATE_BIT) & (STATE_MASK)) == TEST_FAIL_VAL)
#define IS_TEST_SKIP(value)      (((value >> STATE_BIT) & (STATE_MASK)) == TEST_SKIP_VAL)
#define IS_TEST_FAIL_SKIP(value) ((IS_TEST_FAIL(value)) || (IS_TEST_SKIP(value)))

uint8_t
val_mmio_read8(addr_t addr);

uint16_t
val_mmio_read16(addr_t addr);

uint32_t
val_mmio_read(addr_t addr);

uint64_t
val_mmio_read64(addr_t addr);

void
val_mmio_write8(addr_t addr, uint8_t data);

void
val_mmio_write16(addr_t addr, uint16_t data);

void
val_mmio_write(addr_t addr, uint32_t data);

void
val_mmio_write64(addr_t addr, uint64_t data);

uint32_t
val_check_skip_module(uint32_t module_base);

uint32_t
val_initialize_test(uint32_t test_num, char8_t * desc, uint32_t num_pe);

uint32_t
val_check_for_error(uint32_t test_num, uint32_t num_pe, char8_t *ruleid);

void
val_run_test_payload(uint32_t test_num, uint32_t num_pe, void (*payload)(void), uint64_t test_input);

void
val_data_cache_ops_by_va(addr_t addr, uint32_t type);

void
val_test_entry(void);

#endif
