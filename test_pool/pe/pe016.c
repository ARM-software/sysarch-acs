/** @file
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
#include "acs_val.h"
#include "acs_pe.h"
#include "acs_memory.h"

#define TEST_NUM   (ACS_PE_TEST_NUM_BASE  +  16)
#define TEST_RULE  "B_PE_14"
#define TEST_DESC  "Check SVE2 for v9 PE                  "

typedef struct {
  uint64_t data;
  uint32_t status;
  uint16_t vector_len_bits;
} sve_reg_details;

sve_reg_details *g_sve_reg_info;

static
uint16_t
pe_get_max_sve_vector_length()
{
  uint16_t vl;
  uint64_t el;
  uint64_t cpacr, cptr_el2, hcr;
  uint64_t original_zcr, programmed_zcr, effective_zcr;
  bool is_host;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  el = val_pe_reg_read(CurrentEL);
  val_print_primary_pe(DEBUG, "\n     Current EL = %llx", el, index);

  if (el == AARCH64_EL1) {
    /* Read CPACR_EL1.ZEN bits [17:16] */
    cpacr = VAL_EXTRACT_BITS(val_pe_reg_read(CPACR_EL1), 16, 17);

    /* If ZEN = 00 or 10 (LSB == 0), SVE access is trapped */
    if ((cpacr & 0x1) == 0) {
      val_print_primary_pe(DEBUG, "\n       SVE disabled at EL1 (CPACR_EL1.ZEN = 0x%x)",
          cpacr, index);
      return 0;
    }
  } else if (el == AARCH64_EL2) {
    /* Read CPTR_EL2 it controls SVE access at EL2*/
    cptr_el2 = val_pe_reg_read(CPTR_EL2);
    hcr = val_pe_reg_read(HCR_EL2);
    is_host = VAL_EXTRACT_BITS(hcr, 34, 34);

    if (is_host == 0)
    {
      /* If TZ == 1 trap SVE access */
      if (VAL_EXTRACT_BITS(cptr_el2, 8, 8) == 1) {
        val_print_primary_pe(DEBUG, "\n       SVE trapped at EL2 (CPTR_EL2.TZ = 0x%x)",
            VAL_EXTRACT_BITS(cptr_el2, 8, 8), index);
        return 0;
      }
    } else {
      /* If ZEN = 00 or 10 (LSB == 0), SVE access is trapped */
      if ((VAL_EXTRACT_BITS(cptr_el2, 16, 17) & 0x1) == 0) {
        val_print_primary_pe(DEBUG, "\n       SVE disabled at EL2 (CPTR_EL2.ZEN = 0x%x)",
            VAL_EXTRACT_BITS(cptr_el2, 16, 17), index);
        return 0;
      }
    }
  }

  /*Request maximum SVE vector length by setting LEN field to max
   *Read back effective value and Restore original ZCR_EL1 to avoid
   *modifying system state.
   */
  original_zcr = val_pe_reg_read(ZCR_EL1);
  programmed_zcr = (original_zcr & ~ZCR_EL1_LEN_MASK) | ZCR_EL1_LEN_MASK;
  val_pe_reg_write(ZCR_EL1, programmed_zcr);
  isb();

  effective_zcr = val_pe_reg_read(ZCR_EL1);
  val_pe_reg_write(ZCR_EL1, original_zcr);
  isb();

  vl = (uint16_t)((VAL_EXTRACT_BITS(effective_zcr, 0, 3) + 1) * 128);

  return vl;
}

static
void
payload()
{
  uint32_t pe_family;
  uint64_t data;
  uint64_t sve_present;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  sve_reg_details *tmp_reg_data;
  uint64_t buffer_ptr, addr;

  val_get_test_data(index, &addr, &buffer_ptr);
  tmp_reg_data = (sve_reg_details *)buffer_ptr;
  tmp_reg_data = tmp_reg_data + index;
  tmp_reg_data->vector_len_bits = 0;

  /* Read ID_AA64PFR0_EL1.SVE[35:32] == 1 for SVE support */
  sve_present = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 32, 35);
  if (sve_present != 0)
    tmp_reg_data->vector_len_bits = pe_get_max_sve_vector_length();

 /* Read ID_AA64ZFR0_EL1 for SVE2 support */
  data = val_pe_reg_read(ID_AA64ZFR0_EL1);
  tmp_reg_data->data = data;

  /* For Armv9, if ID_AA64ZFR0_EL1 is present the SVEver bits [3:0]
   * value 0b0000 is not permitted */
  /* ID_AA64ZFR0_EL1.SVEver > 0 indicates FEAT_SVE2 or greater is implemented */
  /* If PE implements SVE2, it's a pass. No need to check architecture family, as
   * BSA mandates SVE2 from v9 */
  if (VAL_EXTRACT_BITS(data, 0, 3) > 0) {
    tmp_reg_data->status = ACS_STATUS_PASS;
    val_set_status(index, RESULT_PASS);
  } else {

    /* Get PE family for each PE index*/
    pe_family = val_get_pe_architecture(index);

    /* SVE2 not implemented, SMBIOS info missing, cannot confirm if PE is v9. Skipping the test */
    if (pe_family == ACS_STATUS_ERR) {
      val_set_status(index, RESULT_SKIP(2));
      tmp_reg_data->status = ACS_STATUS_ERR;
      return;
    }

    /* SVE2 not implemented, SMBIOS does not report Armv9, skipping the test */
    if (pe_family != PROCESSOR_FAMILY_ARMV9) {
      val_set_status(index, RESULT_SKIP(3));
      tmp_reg_data->status = ACS_STATUS_SKIP;
      return;
    }

    /* SVE2 not implemented, SMBIOS reports Armv9, failing the test */
    val_set_status(index, RESULT_FAIL(1));
    tmp_reg_data->status = ACS_STATUS_FAIL;
    return;
  }
}

uint32_t
pe016_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;  //default value
  uint32_t i, smbios_slots, index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint16_t ref_vector_len = 0;
  bool vector_len_set = 0;
  bool vector_len_mismatch = 0;
  uint32_t sve_pe_count = 0;
  sve_reg_details *reg_buffer;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  smbios_slots = val_get_num_smbios_slots();
  if (smbios_slots == 0) {
    val_print(WARN, "\n       SMBIOS Table Not Found, Skipping the test\n");
    status = ACS_STATUS_SKIP;
    val_set_status(index, RESULT_SKIP(1));

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, 1, TEST_RULE);
  }

  /* This check is when user is forcing us to skip this test */
  if (GET_STATE(status) != TEST_SKIP) {
    g_sve_reg_info = (sve_reg_details *) val_memory_calloc(num_pe, sizeof(sve_reg_details));
    if (g_sve_reg_info == NULL) {
      val_print(ERROR, "\n       Memory Allocation for SVE Register data Failed");
      return ACS_STATUS_FAIL;
    }

    val_set_test_data(index, (uint64_t)payload, (uint64_t)g_sve_reg_info);
    val_run_test_payload(TEST_NUM, num_pe, payload, (uint64_t)g_sve_reg_info);

    for (i = 0; i < num_pe; i++) {
      reg_buffer = g_sve_reg_info + i;
      val_print(DEBUG, "\n       PE Index = %d", i);

      if (reg_buffer->status == ACS_STATUS_SKIP)
        val_print(DEBUG, "\n       Processor is not v9, Skipping the test");
      else if (reg_buffer->status == ACS_STATUS_ERR)
        val_print(DEBUG, "\n       Processor Family Not Found in SMBIOS Table");
      else if (reg_buffer->status == ACS_STATUS_FAIL) {
        val_print(DEBUG, "\n       Processor is v9 and FEAT_SVE2 is not implemented.");
        val_print(DEBUG, " ID_AA64ZFR0_EL1.SVEver 0x%llx  FAIL", reg_buffer->data);
      } else {
        val_print(DEBUG, "\n       ID_AA64ZFR0_EL1.SVEver 0x%llx PASS", reg_buffer->data);
        if (reg_buffer->vector_len_bits)
          val_print(INFO, "\n       PE[%d] VL = %u bits", i, reg_buffer->vector_len_bits);
        else
          val_print(DEBUG, "\n       PE[%d] VL probe skipped or not accessible", i);
      }

      /* Only PEs that implement SVE and return a valid VL are considered.
       * PEs without SVE or inaccessible VL are ignored. */
      if ((reg_buffer->status == ACS_STATUS_PASS) && reg_buffer->vector_len_bits) {
        sve_pe_count++;
        if (!vector_len_set) {
          ref_vector_len = reg_buffer->vector_len_bits;
          vector_len_set = 1;
        } else if (reg_buffer->vector_len_bits != ref_vector_len) {
          vector_len_mismatch = 1;
        }
      }
    }

    if (vector_len_mismatch && sve_pe_count > 1) {
      val_print(INFO, "\n       PEs report differing maximum SVE vector lengths across PEs");
    }

    val_memory_free((void *) g_sve_reg_info);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  }

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
