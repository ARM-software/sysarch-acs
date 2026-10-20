/** @file
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
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
#include "acs_pcie_enumeration.h"
#include "acs_iovirt.h"
#include "acs_pcie.h"
#include "acs_pe.h"
#include "acs_smmu.h"
#include "acs_memory.h"
#include "acs_exerciser.h"
#include "val_interface.h"

#define TEST_NUM   (ACS_EXERCISER_TEST_NUM_BASE + 24)
#define TEST_DESC  "Check DPC funcionality for RPs        "
#define TEST_RULE  "PCI_ER_06"

#define ERR_FATAL 1
#define ERR_FATAL_NONFATAL 2
#define ERR_UNCORR   0x3
#define DPC_COMPLETION_UR 0x40000
#define MAX_DEVICES  256
#define PCIE_BDF_TABLE_MAX_ENTRIES \
        ((PCIE_DEVICE_BDF_TABLE_SZ - sizeof(pcie_device_bdf_table)) / \
         sizeof(pcie_device_attr))

static uint32_t msg_type[] = {ERR_FATAL_NONFATAL, ERR_FATAL};
static volatile uint32_t irq_pending;
static uint32_t lpi_int_id = 0x204C;
static volatile uint32_t instance;

/* Save downstream config state under the tested RP. */
static void     *cfg_space_buf[MAX_DEVICES];
static uint32_t cfg_space_bdf[MAX_DEVICES];
static uint32_t cfg_space_count;

static
void
intr_handler(void)
{
  /* Clear the interrupt pending state */
  irq_pending = 0;

  val_print(TRACE, "\n       Received MSI interrupt %x       ", lpi_int_id + instance);
  val_gic_end_of_interrupt(lpi_int_id + instance);
  return;
}

static void
free_msi_interrupt(uint32_t bdf, uint32_t device_id, uint32_t its_id,
                   uint32_t int_id, uint32_t msi_index)
{
  val_gic_free_irq(int_id, 0);
  val_gic_free_msi(bdf, device_id, its_id, int_id, msi_index);
}

static void
free_config_space(void)
{
  uint32_t idx;

  for (idx = 0; idx < MAX_DEVICES; idx++)
  {
      if (cfg_space_buf[idx] != NULL)
      {
          val_memory_free_aligned(cfg_space_buf[idx]);
          cfg_space_buf[idx] = NULL;
      }
  }

  cfg_space_count = 0;
}

static uint32_t
restore_downstream_config_space(uint32_t rp_bdf)
{
  uint32_t tbl_index;
  uint32_t dev_rp_bdf;
  pcie_saved_state_t *cfg_state;

  for (tbl_index = 0;
       tbl_index < cfg_space_count && tbl_index < MAX_DEVICES;
       tbl_index++) {
      cfg_state = (pcie_saved_state_t *)cfg_space_buf[tbl_index];
      if (!cfg_state)
          continue;

      if (val_pcie_get_rootport(cfg_space_bdf[tbl_index], &dev_rp_bdf))
          continue;

      if (rp_bdf != dev_rp_bdf)
          continue;

      val_pcie_restore_config_state(cfg_space_bdf[tbl_index], cfg_state);
      if (!val_pcie_get_rootport(cfg_space_bdf[tbl_index], &dev_rp_bdf) &&
          rp_bdf == dev_rp_bdf)
          val_pcie_restore_config_state(cfg_space_bdf[tbl_index], cfg_state);

      val_memory_free_aligned(cfg_space_buf[tbl_index]);
      cfg_space_buf[tbl_index] = NULL;
  }

  cfg_space_count = 0;
  return 0;
}

static uint32_t
save_downstream_config_space(uint32_t rp_bdf)
{
  uint32_t bdf, dev_rp_bdf;
  uint32_t tbl_index;
  uint32_t num_entries;
  uint32_t pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  pcie_saved_state_t *cfg_state;
  pcie_device_bdf_table *bdf_tbl_ptr;

  bdf_tbl_ptr = val_pcie_bdf_table_ptr();
  if (!bdf_tbl_ptr) {
      val_print(ERROR, "\n       PCIe BDF table is NULL");
      val_set_status(pe_index, RESULT_FAIL(02));
      return 1;
  }

  free_config_space();

  num_entries = bdf_tbl_ptr->num_entries;
  if (num_entries > PCIE_BDF_TABLE_MAX_ENTRIES) {
      val_print(WARN, "\n       PCIe BDF table entries exceed allocation");
      num_entries = PCIE_BDF_TABLE_MAX_ENTRIES;
  }

  for (tbl_index = 0; tbl_index < num_entries; tbl_index++) {
      bdf = bdf_tbl_ptr->device[tbl_index].bdf;

      if (val_pcie_get_rootport(bdf, &dev_rp_bdf))
          continue;

      if (rp_bdf != dev_rp_bdf)
          continue;

      if (cfg_space_count >= MAX_DEVICES) {
          val_print(ERROR, "\n       Too many devices under RP 0x%x", rp_bdf);
          val_set_status(pe_index, RESULT_FAIL(02));
          free_config_space();
          return 1;
      }

      cfg_space_buf[cfg_space_count] = val_aligned_alloc(MEM_ALIGN_4K,
                                                         sizeof(pcie_saved_state_t));
      if (cfg_space_buf[cfg_space_count] == NULL) {
          val_print(ERROR, "\n       Memory allocation failed.");
          val_set_status(pe_index, RESULT_FAIL(02));
          free_config_space();
          return 1;
      }

      cfg_space_bdf[cfg_space_count] = bdf;
      cfg_state = (pcie_saved_state_t *)cfg_space_buf[cfg_space_count];
      val_pcie_save_config_state(bdf, cfg_state);
      cfg_space_count++;
  }

  return 0;
}

static
void
payload(void)
{

  uint32_t pe_index;
  uint32_t e_bdf;
  uint32_t erp_bdf;
  uint32_t reg_value;
  uint32_t fail_cnt;
  uint32_t test_skip = 1;
  uint32_t status;
  uint32_t rp_dpc_cap_base;
  uint32_t aer_offset;
  uint32_t rp_aer_offset;
  uint32_t error_source_id;
  uint32_t source_id;
  uint32_t dpc_trigger_reason;
  uint32_t timeout;
  uint32_t msi_check = 0;

  uint32_t device_id = 0;
  uint32_t stream_id = 0;
  uint32_t its_id = 0;
  uint32_t msi_index = 0;
  uint32_t msi_cap_offset = 0;
  uint64_t delay_status;

  fail_cnt = 0;
  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS);

  while (instance-- != 0)
  {
      msi_check = 0;

      if (val_exerciser_init(instance))
          continue;

      e_bdf = val_exerciser_get_bdf(instance);
      val_print(DEBUG, "\n       Exerciser BDF - 0x%x", e_bdf);

      if (val_pcie_get_rootport(e_bdf, &erp_bdf))
          continue;

      /* Check DPC capability */
      status = val_pcie_find_capability(erp_bdf, PCIE_ECAP, ECID_DPC, &rp_dpc_cap_base);
      if (status == PCIE_CAP_NOT_FOUND)
      {
          val_print(ERROR, "\n       ECID_DPC not found for BDF 0x%x", erp_bdf);
          continue;
      }

      /* BSA mandates software-visible PCIe compliance and PCIe
       * requires DLL Link Active reporting for DPC-capable ports
       */
      if (val_pcie_data_link_layer_status(erp_bdf) == PCIE_DLL_LINK_ACTIVE_NOT_SUPPORTED)
      {
          val_print(ERROR, "\n       DLL Link Active reporting not supported, BDF 0x%x", erp_bdf);
          fail_cnt++;
          continue;
      }

      val_pcie_enable_eru(e_bdf);
      val_pcie_enable_eru(erp_bdf);

      /* Check AER capability for both exerciser and RP */
      if (val_pcie_find_capability(e_bdf, PCIE_ECAP, ECID_AER, &aer_offset) != PCIE_SUCCESS) {
          val_print(ERROR, "\n       AER Capability not supported, Bdf : 0x%x", e_bdf);
          continue;
      }

      if (val_pcie_find_capability(erp_bdf, PCIE_ECAP, ECID_AER, &rp_aer_offset) != PCIE_SUCCESS) {
          val_print(ERROR, "\n       AER Capability not supported for RP : 0x%x", erp_bdf);
          fail_cnt++;
      }

      /* Search for MSI/MSI-X Capability */
      if ((val_pcie_find_capability(erp_bdf, PCIE_CAP, CID_MSIX, &msi_cap_offset)) &&
        (val_pcie_find_capability(erp_bdf, PCIE_CAP, CID_MSI, &msi_cap_offset))) {
        val_print(ERROR, "\n       No MSI/MSI-X Capability for Bdf 0x%x", erp_bdf);
        goto err_check;
      }

      msi_check = 1;
      /* Get DeviceID & ITS_ID for this device */
      status = val_iovirt_get_device_info(PCIE_CREATE_BDF_PACKED(erp_bdf),
                                        PCIE_EXTRACT_BDF_SEG(erp_bdf), &device_id,
                                        &stream_id, &its_id);

      if (status) {
          val_print(ERROR, "\n       iovirt_get_device failed for bdf 0x%x", e_bdf);
          val_set_status(pe_index, RESULT_FAIL(01));
          return;
      }

       /* Get DeviceID & ITS_ID for this device */
      status = val_gic_request_msi(erp_bdf, device_id, its_id, lpi_int_id + instance, msi_index);
      if (status) {
          val_print(ERROR, "\n       MSI Assignment failed for bdf : 0x%x", erp_bdf);
          val_set_status(pe_index, RESULT_FAIL(2));
          return;
      }

      status = val_gic_install_isr(lpi_int_id + instance, intr_handler);

      if (status) {
          val_print(ERROR, "\n       Intr handler registration failed: 0x%x", lpi_int_id);
          free_msi_interrupt(erp_bdf, device_id, its_id,
                             lpi_int_id + instance, msi_index);
          val_set_status(pe_index, RESULT_FAIL(02));
          return;
      }

err_check:
      status = val_exerciser_set_param(ERROR_INJECT_TYPE, UNCORR_CMPT_TO, 1, instance);
      if (status != ERR_UNCORR) {
          val_print(ERROR, "\n       Error Injection failed, Bdf : 0x%x", e_bdf);
          if (msi_check == 1)
              free_msi_interrupt(erp_bdf, device_id, its_id,
                                 lpi_int_id + instance, msi_index);
          continue;
      }

      test_skip = 0;

      /* check for both fatal and non-fatal error */
      for (int i = 0; i < 2; i++)
      {
          /* Save downstream config state under this RP before SBR. */
          if (save_downstream_config_space(erp_bdf))
          {
              free_config_space();
              if (msi_check == 1)
                  free_msi_interrupt(erp_bdf, device_id, its_id,
                                     lpi_int_id + instance, msi_index);
              return;
          }
          val_print(TRACE, "\n       EP BDF : 0x%x", e_bdf);

          irq_pending = 1;
          /* Enable DPC */
          val_pcie_enable_dpc(erp_bdf, msg_type[i]);

          /* Select UR completions during DPC and enable the DPC interrupt. */
          /* If UR completions is not selected, then DPC completions could return Completer Abort */
          val_pcie_read_cfg(erp_bdf, rp_dpc_cap_base + DPC_CTRL_OFFSET, &reg_value);
          reg_value |= DPC_INTR_ENABLE | DPC_COMPLETION_UR;
          val_pcie_write_cfg(erp_bdf, rp_dpc_cap_base + DPC_CTRL_OFFSET, reg_value);

          val_pcie_read_cfg(erp_bdf, rp_dpc_cap_base + DPC_CTRL_OFFSET, &reg_value);

          if (msg_type[i] == ERR_FATAL)
          {
              val_pcie_write_cfg(e_bdf, aer_offset + AER_UNCORR_SEVR_OFFSET, AER_UNCORR_SEVR_FATAL);
              val_pcie_write_cfg(e_bdf, aer_offset + AER_UNCORR_MASK_OFFSET, 0x0);
          }
          else
          {
              val_pcie_write_cfg(e_bdf, aer_offset + AER_UNCORR_SEVR_OFFSET, 0x0);
          }

          /*Inject error immediately*/
          val_exerciser_ops(INJECT_ERROR, CFG_READ, instance);

          /* Wait for the link to go down before accessing the EP or releasing DPC. */
          val_time_delay_ms(10 * ONE_MILLISECOND);
          timeout = TIMEOUT_LARGE;
          do
          {
              status = val_pcie_data_link_layer_status(erp_bdf);
          } while ((status == PCIE_DLL_LINK_STATUS_ACTIVE) && --timeout);

          if (status != PCIE_DLL_LINK_STATUS_NOT_ACTIVE) {
              val_print(ERROR, "\n       DPC link-down wait failed, BDF 0x%x", erp_bdf);
              fail_cnt++;

              /* Stop new triggers before deciding whether restoration is safe. */
              val_pcie_disable_dpc(erp_bdf);
              if (msi_check == 1) {
                  free_msi_interrupt(erp_bdf, device_id, its_id,
                                     lpi_int_id + instance, msi_index);
                  msi_check = 0;
              }
              goto dpc_recover;
          }

          val_pcie_read_cfg(e_bdf, CFG_READ, &reg_value);
          if (reg_value != PCIE_UNKNOWN_RESPONSE)
          {
              val_print(ERROR, "\n       EP not contained due to DPC");
              fail_cnt++;
          }

          val_pcie_read_cfg(erp_bdf, rp_dpc_cap_base + DPC_STATUS_OFFSET, &reg_value);

          /* Check DPC Trigger status */
          if ((reg_value & 1) == 0)
          {
              val_print(ERROR, "\n       DPC Trigger status bit not set %x", reg_value);
              fail_cnt++;
          }

          dpc_trigger_reason = (reg_value & DPC_TRIGGER_MASK) >> 1;
          if (msg_type[i] == ERR_FATAL)
          {
              if (dpc_trigger_reason != 2)
              {
                  val_print(ERROR, "\n       DPC Trigger reason incorrect");
                  fail_cnt++;
              }
          } else {
              if (dpc_trigger_reason != 1)
              {
                  val_print(ERROR, "\n       DPC Trigger reason incorrect");
                  fail_cnt++;
              }
          }

          source_id = PCIE_CREATE_BDF_PACKED(e_bdf);
          error_source_id = (reg_value >> DPC_SOURCE_ID_SHIFT);
          if (source_id != error_source_id)
          {
              val_print(ERROR, "\n       DPC Error source Identification failed");
              fail_cnt++;
          }

          if (msi_check == 1)
          {
              timeout = TIMEOUT_LARGE;
              while ((--timeout > 0) && irq_pending)
              {};

              if (timeout == 0) {
                  val_print(ERROR, "\n       Interrupt trigger failed for bdf 0x%x", e_bdf);
                  fail_cnt++;
              }
          }

dpc_recover:
          /* RP Busy is applicable only when RP Extensions for DPC are supported. */
          val_pcie_read_cfg(erp_bdf, rp_dpc_cap_base + DPC_CTRL_OFFSET, &reg_value);
          if ((reg_value >> DPC_RP_EXT_OFFSET) & DPC_RP_EXT_MASK)
          {
              /* Wait for RP Busy to clear while DPC Trigger Status is set. */
              timeout = TIMEOUT_LARGE;
              do
              {
                  val_pcie_read_cfg(erp_bdf, rp_dpc_cap_base + DPC_STATUS_OFFSET, &reg_value);
                  if (!(reg_value & DPC_STATUS_MASK) || !(reg_value & DPC_RP_BUSY_MASK))
                      break;
              } while (--timeout);

              if (timeout == 0)
              {
                  val_print(ERROR, "\n       DPC RP Busy did not clear for BDF 0x%x", erp_bdf);
                  fail_cnt++;
                  /* Do not access the EP or clear DPC Trigger Status while RP Busy is set. */
                  free_config_space();
                  if (msi_check == 1)
                      free_msi_interrupt(erp_bdf, device_id, its_id,
                                         lpi_int_id + instance, msi_index);
                  val_set_status(pe_index, RESULT_FAIL(fail_cnt));
                  return;
              }
          }

          /* Disable further DPC triggers before releasing containment. */
          val_pcie_disable_dpc(erp_bdf);

          /* Clear DPC Trigger Status and Interrupt Status (RW1C). */
          /* Until this trigger is cleared, the LTSSM will be in the disabled state */
          val_pcie_write_cfg(erp_bdf, rp_dpc_cap_base + DPC_STATUS_OFFSET, 0x9);

          val_pcie_read_cfg(erp_bdf, TYPE01_ILR, &reg_value);
          reg_value = reg_value | BRIDGE_CTRL_SBR_SET;
          val_pcie_write_cfg(erp_bdf, TYPE01_ILR, reg_value);

          /* Wait for Timeout */
          val_time_delay_ms(2 * ONE_MILLISECOND);

          val_pcie_read_cfg(erp_bdf, TYPE01_ILR, &reg_value);
          reg_value = reg_value & ~BRIDGE_CTRL_SBR_SET;
          val_pcie_write_cfg(erp_bdf, TYPE01_ILR, reg_value);

          timeout = TIMEOUT_LARGE;
          while (--timeout)
          {};

          status = val_pcie_data_link_layer_status(erp_bdf);
          if (status != PCIE_DLL_LINK_ACTIVE_NOT_SUPPORTED)
          {
              if (!status)
              {
                  /* Wait for for additional Timeout and check the status*/
                  delay_status = val_time_delay_ms(100 * ONE_MILLISECOND);
                  if (delay_status)
                  {
                      val_print(ERROR,
                               "\n       Failed to time delay for BDF 0x%x ", erp_bdf);
                      fail_cnt++;
                  }

                  status = val_pcie_data_link_layer_status(erp_bdf);
              }
          }

          if (status == PCIE_DLL_LINK_STATUS_NOT_ACTIVE)
          {
              val_print(ERROR,
                       "\n       The link not active after reset for BDF 0x%x: ", erp_bdf);
              fail_cnt++;
              val_set_status(pe_index, RESULT_FAIL(fail_cnt));
              free_config_space();
              if (msi_check == 1)
                  free_msi_interrupt(erp_bdf, device_id, its_id,
                                     lpi_int_id + instance, msi_index);
              return;
          }

          /* Allow post-reset initialization before restoring configuration. */
          delay_status = val_time_delay_ms(100 * ONE_MILLISECOND);

          restore_downstream_config_space(erp_bdf);
          status = val_pcie_read_cfg(e_bdf, CFG_READ, &reg_value);
          if (delay_status || status || (reg_value == PCIE_UNKNOWN_RESPONSE))
          {
              val_print(ERROR, "\n       EP config read failed, BDF 0x%x", e_bdf);
              fail_cnt++;
              val_set_status(pe_index, RESULT_FAIL(fail_cnt));
              free_config_space();
              if (msi_check == 1)
                  free_msi_interrupt(erp_bdf, device_id, its_id,
                                     lpi_int_id + instance, msi_index);
              return;
          }

          val_pcie_read_cfg(e_bdf, aer_offset + AER_UNCORR_STATUS_OFFSET, &reg_value);
          val_pcie_write_cfg(e_bdf, aer_offset + AER_UNCORR_STATUS_OFFSET, reg_value & 0xFFFFFFFF);

      }

      if (msi_check == 1)
          free_msi_interrupt(erp_bdf, device_id, its_id,
                             lpi_int_id + instance, msi_index);
  }

  if (fail_cnt)
      val_set_status(pe_index, RESULT_FAIL(fail_cnt));
  else if (test_skip)
      val_set_status(pe_index, RESULT_SKIP(01));
  else
      val_set_status(pe_index, RESULT_PASS);

  return;

}

uint32_t
e024_entry(uint32_t num_pe)
{
  /* Run test on single PE */
  num_pe = 1;
  uint32_t status = ACS_STATUS_FAIL;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP) {
      if (val_exerciser_test_init() != ACS_STATUS_PASS)
          return val_exerciser_get_init_result(TEST_RULE);
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);
  }

  /* Get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
