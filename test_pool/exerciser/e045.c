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

#include "acs_val.h"
#include "acs_pe.h"
#include "acs_pcie.h"
#include "acs_exerciser.h"
#include "acs_cxl.h"
#include "gic.h"
#include "val_interface.h"

#define TEST_NUM   (ACS_EXERCISER_TEST_NUM_BASE + 45)
#define TEST_RULE  "CXL_06"
#define TEST_DESC  "CXL MEFN VDM sink handling on host    "
#define PCIE_COMMAND_MASK 0xFFFFU

#define NOT_MEFN_IRQ 0x2

static void *branch_to_test;
static uint64_t branch_stack_pointer;
static uint32_t mefn_bdf;
static uint32_t mefn_irq;
static uint32_t mefn_instance;
static volatile uint32_t exception_observed;
static volatile uint32_t mefn_irq_pending;
static volatile uint32_t mefn_service_failed;

static
void
clear_mefn_irq_state(uint32_t irq)
{
  mefn_irq_pending = 0;

  val_gic_disableInterruptSource(irq);
  val_gic_clear_interrupt(irq);
  val_gic_free_irq(irq, 0);

  mefn_irq = 0;
  mefn_bdf = 0;
  mefn_instance = 0;
  mefn_service_failed = 0;
  branch_to_test = NULL;
  branch_stack_pointer = 0;
}

static
uint32_t
save_mefn_serr_state(uint32_t bdf, uint32_t *serr_enable)
{
  uint32_t reg_value;

  if (val_pcie_read_cfg(bdf, TYPE01_CR, &reg_value))
    return ACS_STATUS_ERR;

  *serr_enable = ((reg_value >> CR_SERRE_SHIFT) & CR_SERRE_MASK);
  return ACS_STATUS_PASS;
}

static
void
restore_mefn_serr_state(uint32_t bdf, uint32_t serr_enable)
{
  uint32_t reg_value;
  uint32_t command;

  if (val_pcie_read_cfg(bdf, TYPE01_CR, &reg_value))
    return;

  command = reg_value & PCIE_COMMAND_MASK;
  if (serr_enable)
    command |= (CR_SERRE_MASK << CR_SERRE_SHIFT);
  else
    command &= ~(CR_SERRE_MASK << CR_SERRE_SHIFT);

  val_pcie_write_cfg(bdf, TYPE01_CR, command);
}

static
void
esr(uint64_t interrupt_type, void *context)
{
  if ((interrupt_type == EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS) && branch_to_test &&
      branch_stack_pointer) {
    val_pe_update_elr_and_sp(context, (uint64_t)branch_to_test, branch_stack_pointer);
  }

  val_print(ERROR, "\n       Received exception of type: %d", interrupt_type);
  exception_observed = 1;
}

static
void
intr_handler(void)
{
  uint32_t status;

  if (!mefn_irq_pending)
    val_print(WARN, "\n       Spurious MEFN/SERR interrupt %d", mefn_irq);

  /* For certain platforms the message handler might be used to route all
   * interrupts. Calling MEFN_IRQ_SERVICE to validate that the received
   * record is MEFN before this interrupt can satisfy CXL_06.
   */
  status = val_exerciser_set_param(MEFN_IRQ_SERVICE, 0, mefn_bdf, mefn_instance);
  if (status == NOT_MEFN_IRQ) {
    /* INTID may be an aggregate source. An unrelated record may race with
     * the MEFN trigger; it has been acknowledged, so wait for MEFN.
     */
    goto clear_intr;
  }

  if (status != ACS_STATUS_PASS) {
    val_print(ERROR, "\n       MEFN interrupt service failed for BDF 0x%x", mefn_bdf);
    mefn_service_failed = 1;
  }

  /* This is endpoint cleanup only; it is not the test pass criterion. */
  val_pcie_clear_signaled_system_error(mefn_bdf);

  mefn_irq_pending = 0;
  exception_observed = 1;

clear_intr:
  val_gic_end_of_interrupt(mefn_irq);
}

static
uint32_t
is_cxl_root_port(uint32_t rp_bdf)
{
  uint32_t comp_count;
  uint32_t comp_idx;
  uint32_t comp_bdf;
  uint32_t comp_role;

  comp_count = val_cxl_get_component_info(CXL_COMPONENT_INFO_COUNT, 0);
  for (comp_idx = 0; comp_idx < comp_count; comp_idx++) {
    comp_bdf = val_cxl_get_component_info(CXL_COMPONENT_INFO_BDF_INDEX, comp_idx);
    comp_role = val_cxl_get_component_info(CXL_COMPONENT_INFO_ROLE, comp_idx);
    if ((comp_role == CXL_COMPONENT_ROLE_ROOT_PORT) && (comp_bdf == rp_bdf))
      return ACS_STATUS_PASS;
  }

  return ACS_STATUS_ERR;
}

static
void
payload(void)
{
  uint32_t pe_index;
  uint32_t num_instances;
  uint32_t instance;
  uint32_t e_bdf;
  uint32_t rp_bdf;
  uint32_t status;
  uint32_t timeout;
  uint32_t mefn_serr_enable;
  uint32_t mefn_observed;
  uint64_t mefn_irq_value;
  uint64_t mefn_irq_unused;
  uint32_t fail_count = 0;
  uint32_t test_skip = 1;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  num_instances = val_exerciser_get_info(EXERCISER_NUM_CARDS);

  status = val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
  status |= val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);
  if (status) {
    val_print(ERROR, "\n       Failed in installing the exception handler");
    val_set_status(pe_index, RESULT_FAIL(01));
    return;
  }

  status = val_exerciser_check_firmware_handle_support();
  if (status == ACS_STATUS_PAL_NOT_IMPLEMENTED) {
    val_set_status(pe_index, RESULT_WARNING(2));
    return;
  }

  if (status != ACS_STATUS_PASS) {
    val_print(ERROR, "\n       Firmware first handling not supported");
    val_set_status(pe_index, RESULT_SKIP(01));
    return;
  }

  while (num_instances-- != 0) {
    instance = num_instances;

    if (val_exerciser_init(instance))
      continue;

    e_bdf = val_exerciser_get_bdf(instance);
    if (val_cxl_device_is_cxl(e_bdf) != ACS_STATUS_PASS)
      continue;

    if (val_pcie_get_rootport(e_bdf, &rp_bdf))
      continue;

    if (is_cxl_root_port(rp_bdf) != ACS_STATUS_PASS)
      continue;

    status = val_exerciser_get_param(MEFN_SERR_IRQ, &mefn_irq_value, &mefn_irq_unused, instance);
    if (status != ACS_STATUS_PASS) {
      val_print(WARN, "\n       MEFN/SERR INTID not provided by PAL for BDF 0x%x", e_bdf);
      continue;
    }

    status = save_mefn_serr_state(e_bdf, &mefn_serr_enable);
    if (status != ACS_STATUS_PASS) {
      val_print(ERROR, "\n       Failed to save SERR state for BDF 0x%x", e_bdf);
      fail_count++;
      continue;
    }

    mefn_irq = (uint32_t)mefn_irq_value;
    mefn_bdf = e_bdf;
    mefn_instance = instance;
    val_pcie_clear_signaled_system_error(e_bdf);
    val_gic_clear_interrupt(mefn_irq);

    status = val_gic_install_isr(mefn_irq, intr_handler);
    if (status) {
      val_print(ERROR, "\n       Failed to install ISR for MEFN/SERR INTID %d", mefn_irq);
      fail_count++;
      restore_mefn_serr_state(e_bdf, mefn_serr_enable);
      clear_mefn_irq_state(mefn_irq);
      continue;
    }

    val_gic_route_interrupt_to_pe(mefn_irq, val_pe_get_mpid_index(pe_index));

    exception_observed = 0;
    mefn_irq_pending = 1;
    mefn_service_failed = 0;
    branch_to_test = &&exception_return;
    branch_stack_pointer = val_pe_save_exception_return_context();

    status = val_exerciser_set_param(GENERATE_MEFN_VDM, 0, e_bdf, instance);
    if (status != ACS_STATUS_PASS) {
      val_print(ERROR, "\n       MEFN trigger not successful BDF 0x%x", e_bdf);
      val_exerciser_set_param(MEFN_IRQ_RESTORE, 0, e_bdf, instance);
      fail_count++;
      restore_mefn_serr_state(e_bdf, mefn_serr_enable);
      clear_mefn_irq_state(mefn_irq);
      continue;
    }

    test_skip = 0;
    timeout = TIMEOUT_LARGE;
    while ((--timeout > 0) && !exception_observed);

exception_return:
    mefn_observed = exception_observed;
    branch_to_test = NULL;
    branch_stack_pointer = 0;

    restore_mefn_serr_state(e_bdf, mefn_serr_enable);

    if (!mefn_observed) {
      val_print(ERROR, "\n      MEFN trigger did not raise exception for BDF 0x%x", e_bdf);
      val_print(ERROR, " RP BDF is 0x%x", rp_bdf);
      val_pcie_clear_signaled_system_error(e_bdf);
      fail_count++;
    } else if (mefn_service_failed) {
      val_print(ERROR, "\n      INTID %d did not contain a valid MEFN record", mefn_irq);
      fail_count++;
    }

    status = val_exerciser_set_param(MEFN_IRQ_RESTORE, 0, e_bdf, instance);
    if (status != ACS_STATUS_PASS) {
      val_print(ERROR, "\n       MEFN interrupt restore failed for BDF 0x%x", e_bdf);
      fail_count++;
    }

    clear_mefn_irq_state(mefn_irq);
  }

  if (fail_count) {
    val_set_status(pe_index, RESULT_FAIL(fail_count));
  } else if (test_skip) {
    val_set_status(pe_index, RESULT_SKIP(01));
  } else {
    val_set_status(pe_index, RESULT_PASS);
  }
}

uint32_t
e045_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP) {
    if (val_exerciser_test_init() != ACS_STATUS_PASS)
      return val_exerciser_get_init_result(TEST_RULE);
    val_run_test_payload(TEST_NUM, num_pe, payload, 0);
  }

  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
