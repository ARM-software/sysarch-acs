/** @file
 * Copyright (c) 2020-2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/val_interface.h"
#include "val/include/acs_smmu.h"
#define TEST_NUM   (ACS_SMMU_TEST_NUM_BASE + 9)
#define TEST_RULE  "S_L5SM_01"
#define TEST_DESC  "Check SMMUv3.2 or higher              "

#define TEST_NUM1   (ACS_SMMU_TEST_NUM_BASE + 26)
#define TEST_RULE1  "S_L5SM_02"
#define TEST_DESC1  "Check SMMU L1 and L2 table resizing   "

#define TEST_NUM2   (ACS_SMMU_TEST_NUM_BASE + 28)
#define TEST_RULE2  "S_L8SM_01"
#define TEST_DESC2  "Check SMMUv3.3 or higher              "

/* This test checks whether SMMUv3.2 or higher */
static
void
check_smmuv3_2_or_higher(void)
{
  uint64_t data;
  uint32_t num_smmu;
  uint32_t fail_cnt = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  num_smmu = val_smmu_get_info(SMMU_NUM_CTRL, 0);

  if (num_smmu == 0) {
      val_print(ACS_PRINT_ERR, "\n       No SMMU Controllers are discovered ", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
      return;
  }

  while (num_smmu--) {
      /* Read SMMU major version */
      if (val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, num_smmu) < 3) {
          val_print(ACS_PRINT_ERR,
                    "\n       SMMU implementation must be SMMUv3.2 or higher required.", 0);
          fail_cnt++;
      } else {
          /* Read SMMU minor version */
          data = VAL_EXTRACT_BITS(val_smmu_read_cfg(SMMUv3_AIDR, num_smmu), 0, 7);
          if (data < 0x2) { /* SMMUv3.2 or higher not implemented */
              val_print(ACS_PRINT_ERR,
                        "\n       SMMU implementation must be SMMUv3.2 or higher required.", 0);
              fail_cnt++;
          }
      }
  }

  if (fail_cnt) {
      val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
  } else {
      val_set_status(index, RESULT_PASS(TEST_NUM, 01));
  }

}


static
void
payload_check_l1_l2_table_resizing(void)
{
  uint64_t data;
  uint32_t num_smmu;
  uint32_t fail_cnt = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  num_smmu = val_smmu_get_info(SMMU_NUM_CTRL, 0);

  if (num_smmu == 0) {
      val_print(ACS_PRINT_ERR, "\n       No SMMU Controllers are discovered ", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM1, 01));
      return;
  }

  while (num_smmu--) {
      /* Read SMMU major version */
      if (val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, num_smmu) >= 3) {
          /* Read SMMU_IDR3.BBML to check level 1 or level 2 for translation table resizing */
          data = VAL_EXTRACT_BITS(val_smmu_read_cfg(SMMUv3_IDR3, num_smmu), 11, 12);
          /* Non zero value indicates either level or level 2 is supported */
          if (data == 0) {
              val_print(ACS_PRINT_ERR,
                       "\n       SMMU index = 0x%x doesn't provide level 1 or level 2 support."
                       , num_smmu);
              fail_cnt++;
          }
      } else {
          /* SMMU_IDR3.BBML is SMMUv3 register check */
          val_print(ACS_PRINT_ERR,
                    "\n       SMMUv2 implementations cannot provide level 1 or level 2 support."
                    , 0);
          fail_cnt++;
      }
  }

  if (fail_cnt) {
      val_set_status(index, RESULT_FAIL(TEST_NUM1, 01));
  } else {
      val_set_status(index, RESULT_PASS(TEST_NUM1, 01));
  }
}

static
void
payload_check_smmuv3_3_or_higher(void)
{
  uint64_t data;
  uint32_t num_smmu;
  uint32_t fail_cnt = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  num_smmu = val_smmu_get_info(SMMU_NUM_CTRL, 0);

  if (num_smmu == 0) {
      val_print(ACS_PRINT_ERR, "\n       No SMMU Controllers are discovered ", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM2, 01));
      return;
  }

  while (num_smmu--) {
      /* Read SMMU major version */
      if (val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, num_smmu) < 3) {
          val_print(ACS_PRINT_ERR,
                    "\n       SMMU implementation must be SMMUv3.3 or higher required.", 0);
          fail_cnt++;
      } else {
          /* Read SMMU minor version */
          data = VAL_EXTRACT_BITS(val_smmu_read_cfg(SMMUv3_AIDR, num_smmu), 0, 7);
          if (data < 0x3) { /* SMMUv3.3 or higher not implemented */
              val_print(ACS_PRINT_ERR,
                        "\n       SMMU implementation must be SMMUv3.3 or higher required.", 0);
              fail_cnt++;
          }

      }
  }

  if (fail_cnt) {
    val_set_status(index, RESULT_FAIL(TEST_NUM2, 01));
  } else {
    val_set_status(index, RESULT_PASS(TEST_NUM2, 01));
  }

}

uint32_t
i009_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  /* This test is run on single processor */
  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, check_smmuv3_2_or_higher, 0);
  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}

uint32_t
i026_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  /* This test is run on single processor */
  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM1, TEST_DESC1, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM1, num_pe, payload_check_l1_l2_table_resizing, 0);
  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM1, num_pe, TEST_RULE1);
  val_report_status(0, ACS_END(TEST_NUM1), TEST_RULE1);

  return status;
}

uint32_t
i028_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  /* This test is run on single processor */
  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM2, TEST_DESC2, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM2, num_pe, payload_check_smmuv3_3_or_higher, 0);
  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM2, num_pe, TEST_RULE2);
  val_report_status(0, ACS_END(TEST_NUM2), TEST_RULE2);

  return status;
}
