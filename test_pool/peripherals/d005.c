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

#include "acs_val.h"
#include "val_interface.h"
#include "acs_pe.h"
#include "acs_peripherals.h"

#define TEST_NUM  (ACS_PER_TEST_NUM_BASE + 5)
#define TEST_RULE "B_PER_07"
#define TEST_DESC "Check UART Non-secure register access "

static void *uart_probe_recovery_addr;
static volatile uint32_t uart_probe_fault;

static
uint32_t
uart_is_16550(uint32_t interface_type)
{
  return (interface_type == COMPATIBLE_FULL_16550
          || interface_type == COMPATIBLE_SUBSET_16550
          || interface_type == COMPATIBLE_GENERIC_16550);
}

static
uint32_t
uart_is_generic(uint32_t interface_type)
{
  return (interface_type == ARM_PL011_UART
          || interface_type == ARM_SBSA_GENERIC_UART);
}

#define UART_PROBE_READ(addr, width_mask, data)                              \
  do {                                                                       \
    if ((width_mask) & WIDTH_BIT8)                                           \
        (data) = *(volatile uint8_t *)(addr);                                \
    else if ((width_mask) & WIDTH_BIT16)                                     \
        (data) = *(volatile uint16_t *)(addr);                               \
    else if ((width_mask) & WIDTH_BIT32)                                     \
        (data) = *(volatile uint32_t *)(addr);                               \
  } while (0)

#define UART_PROBE_WRITE(addr, width_mask, data)                             \
  do {                                                                       \
    if ((width_mask) & WIDTH_BIT8)                                           \
        *(volatile uint8_t *)(addr) = (uint8_t)(data);                       \
    else if ((width_mask) & WIDTH_BIT16)                                     \
        *(volatile uint16_t *)(addr) = (uint16_t)(data);                     \
    else if ((width_mask) & WIDTH_BIT32)                                     \
        *(volatile uint32_t *)(addr) = (uint32_t)(data);                     \
  } while (0)

static
void
uart_probe_esr(uint64_t interrupt_type, void *context)
{
  if (uart_probe_recovery_addr == NULL) {
      val_pe_default_esr(interrupt_type, context);
      return;
  }

  val_pe_update_elr(context, (uint64_t)uart_probe_recovery_addr);
  val_print(DEBUG, "\n       Received Exception of type %d", interrupt_type);
  uart_probe_fault = 1;
}

static
void
uart_probe_restore_esr(void)
{
  val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, val_pe_default_esr);
  uart_probe_recovery_addr = NULL;
  uart_probe_fault = 0;
}

static
uint32_t
uart_probe_non_secure_access(uint64_t uart_base, uint32_t interface_type,
                             uint32_t reg_shift, uint32_t width_mask)
{
  uint32_t data = 0;
  uint32_t status;
  uint64_t access_addr;

  if (!uart_is_16550(interface_type) && !uart_is_generic(interface_type))
      return ACS_STATUS_SKIP;

  uart_probe_fault = 1;
  uart_probe_recovery_addr = &&uart_probe_done;

  val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, uart_probe_esr);

  if (uart_is_16550(interface_type)) {
      access_addr = uart_base + (LCR << reg_shift);
      UART_PROBE_READ(access_addr, width_mask, data);
      UART_PROBE_WRITE(access_addr, width_mask, data);
      UART_PROBE_READ(access_addr, width_mask, data);
  } else {
      /*
       * For PL011/generic UARTs, use mask/status registers that are safe to
       * read and write back while still proving Non-secure MMIO access.
       */
      access_addr = uart_base + BSA_UARTIMSC;
      UART_PROBE_READ(access_addr, WIDTH_BIT32, data);
      UART_PROBE_WRITE(access_addr, WIDTH_BIT32, data);

      access_addr = uart_base + BSA_UARTFR;
      UART_PROBE_READ(access_addr, WIDTH_BIT32, data);
  }

  uart_probe_fault = 0;

uart_probe_done:
  status = uart_probe_fault ? ACS_STATUS_FAIL : ACS_STATUS_PASS;
  uart_probe_restore_esr();

  return status;
}

static
void
payload_check_uart_non_secure_access()
{
  uint32_t count = val_peripheral_get_info(NUM_UART, 0);
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t interface_type;
  uint32_t access_width;
  uint32_t reg_shift = 0;
  uint32_t width_mask = WIDTH_BIT32;
  uint32_t status;
  uint32_t non_secure_uart_found = 0;
  uint64_t uart_base;

  if (count == 0) {
      val_print(WARN, "\n       No UART defined by Platform      ");
      val_set_status(index, RESULT_SKIP(1));
      return;
  }

  while (count != 0) {
      interface_type = val_peripheral_get_info(UART_INTERFACE_TYPE, count - 1);
      uart_base = val_peripheral_get_info(UART_BASE0, count - 1);

      if (uart_base == 0) {
          val_print(DEBUG, "\n       UART base not specified for instance: %x", count - 1);
          count--;
          continue;
      }

      if (uart_is_16550(interface_type)) {
          /*
           * 16550 UART registers can be exposed with 8/16/32-bit spacing. Use
           * UART_WIDTH to choose the correct access size and register offset.
           */
          access_width = val_peripheral_get_info(UART_WIDTH, count - 1);
          if (val_peripheral_uart_16550_width_to_access(access_width, &reg_shift,
                                                        &width_mask)) {
              val_print(DEBUG, "\n       UART access width not specified for instance: %x",
                        count - 1);
              count--;
              continue;
          }

      } else if (!uart_is_generic(interface_type)) {
          val_print(DEBUG, "\n       UART interface type %x is not BSA compatible",
                    interface_type);
          count--;
          continue;
      }

      status = uart_probe_non_secure_access(uart_base, interface_type, reg_shift, width_mask);
      if (status != ACS_STATUS_PASS) {
          val_print(DEBUG, "\n       UART instance %x is not Non-secure accessible", count - 1);
          count--;
          continue;
      }

      non_secure_uart_found = 1;
      count--;
  }

  if (non_secure_uart_found == 0) {
      val_print(ERROR, "\n       No BSA compatible Non-secure accessible UART found");
      val_set_status(index, RESULT_FAIL(2));
  } else {
      val_set_status(index, RESULT_PASS);
  }
}

uint32_t
d005_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  /* This test is run on single processor */
  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload_check_uart_non_secure_access, 0);

  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);
  return status;
}
