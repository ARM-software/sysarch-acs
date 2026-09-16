/** @file
 * Copyright (c) 2016-2021,2024-2026, Arm Limited or its affiliates. All rights reserved.
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

#include "val_interface.h"
#include "acs_val.h"

#include "acs_gic.h"
#include "acs_pcie.h"

#define TEST_NUM   (ACS_GIC_TEST_NUM_BASE + 2)
#define TEST_RULE  "B_GIC_02"
#define TEST_DESC  "Check GIC Valid Configuration         "

#define GIC_V2_MAX_PE  8U
#define GIC_V3_MAX_PE  (1U << 28)

static
void
payload()
{
  uint32_t gic_version;
  uint32_t num_msi_frame;
  uint32_t num_its;
  uint32_t lpi_support;
  uint32_t pe_count;
  uint32_t num_ecam = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  pe_count = val_pe_get_num();
  num_ecam = val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0);
  gic_version = val_gic_get_info(GIC_INFO_VERSION);
  num_msi_frame = val_gic_get_info(GIC_INFO_NUM_MSI_FRAME);
  num_its = val_gic_get_info(GIC_INFO_NUM_ITS);

  val_print(TRACE, "\n       Received GIC version = %4d      ", gic_version);

  if (gic_version < 3) {
    if (pe_count > GIC_V2_MAX_PE) {
      val_print(ERROR, "\n       GICv2 supports a maximum of %d PEs", GIC_V2_MAX_PE);
      val_print(ERROR, ", but system has %d PEs", pe_count);
      val_set_status(index, RESULT_FAIL(1));
      return;
    }

    if ((num_ecam > 0) && (num_msi_frame == 0)) {
      val_print(ERROR, "\n       GICv2 with PCIe : Invalid Configuration");
      val_set_status(index, RESULT_FAIL(2));
      return;
    }
  } else {
    if (pe_count > GIC_V3_MAX_PE) {
      val_print(ERROR, "\n       GICv3 supports a maximum of 2^28 PEs");
      val_print(ERROR, ", but system has %d PEs", pe_count);
      val_set_status(index, RESULT_FAIL(3));
      return;
    }

    if ((num_ecam > 0) && (num_its == 0)) {
      val_print(ERROR, "\n       GICv3 with PCIe and no ITS : Invalid Configuration");
      val_set_status(index, RESULT_FAIL(4));
      return;
    }

    if (num_its > 0) {
      lpi_support = VAL_EXTRACT_BITS(
                        val_mmio_read(val_get_gicd_base() + GICD_TYPER),
                        GICV3_LPIS_BIT, GICV3_LPIS_BIT);
      if (lpi_support == 0) {
        val_print(ERROR, "\n       GICv3 with ITS does not support LPIs");
        val_set_status(index, RESULT_FAIL(5));
        return;
      }
    }
  }

  val_set_status(index, RESULT_PASS);
}

uint32_t
g002_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This GIC test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
