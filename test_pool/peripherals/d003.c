
/** @file
 * Copyright (c) 2016-2018, 2021, 2023-2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/val_interface.h"
#include "val/include/acs_pe.h"
#include "val/include/acs_peripherals.h"
#include "val/include/acs_gic.h"
#include "val/driver/gic/v3/gic_v3.h"

#define TEST_NUM   (ACS_PER_TEST_NUM_BASE + 3)
#define TEST_RULE_BSA  "B_PER_05"
#define TEST_RULE_SBSA "S_L3PR_01"
#define TEST_DESC  "Check UART type Arm Generic or 16550  "

#define TEST_NUM1  (ACS_PER_TEST_NUM_BASE + 6)
#define TEST_RULE1 "B_PER_06"
#define TEST_DESC1 "Check Arm GENERIC UART Interrupt      "

static uint64_t l_uart_base;
static uint32_t int_id;
static void *branch_to_test;
static uint32_t test_fail;

static
void
esr(uint64_t interrupt_type, void *context)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Update the ELR to point to next instrcution */
  val_pe_update_elr(context, (uint64_t)branch_to_test);

  val_print(ACS_PRINT_ERR, "\n       Error : Received Exception of type %d", interrupt_type);
  val_set_status(index, TEST_FAIL);
}

/* Following APIs are for 16550 compatible UART access */

static
void
uart_16550_reg_write(uint64_t uart_base, uint32_t offset, uint32_t reg_shift,
               uint32_t width_mask, uint32_t data)
{
  if (width_mask & WIDTH_BIT8)
      *((volatile uint8_t *)(uart_base + (offset << reg_shift))) = (uint8_t)data;

  if (width_mask & WIDTH_BIT16)
      *((volatile uint16_t *)(uart_base + (offset << reg_shift))) = (uint16_t)data;

  if (width_mask & WIDTH_BIT32)
      *((volatile uint32_t *)(uart_base + (offset << reg_shift))) = (uint32_t)data;

}

static
uint32_t
uart_16550_reg_read(uint64_t uart_base, uint32_t offset, uint32_t reg_shift, uint32_t width_mask)
{
  if (width_mask & WIDTH_BIT8)
      return *((volatile uint8_t *)(uart_base + (offset << reg_shift)));

  if (width_mask & WIDTH_BIT16)
      return *((volatile uint16_t *)(uart_base + (offset << reg_shift)));

  if (width_mask & WIDTH_BIT32)
      return *((volatile uint32_t *)(uart_base + (offset << reg_shift)));

  return 0;
}

/* Following APIs are for Arm Generic UART access */

uint32_t
uart_generic_reg_read(uint32_t offset, uint32_t width_mask)
{
  if (width_mask & WIDTH_BIT8)
      return *((volatile uint8_t *)(l_uart_base + offset));

  if (width_mask & WIDTH_BIT16)
      return *((volatile uint16_t *)(l_uart_base + offset));

  if (width_mask & WIDTH_BIT32)
      return *((volatile uint32_t *)(l_uart_base + offset));

  return 0;
}

void
uart_generic_reg_write(uint32_t offset, uint32_t width_mask, uint32_t data)
{
  if (width_mask & WIDTH_BIT8)
      *((volatile uint8_t *)(l_uart_base + offset)) = (uint8_t)data;

  if (width_mask & WIDTH_BIT16)
      *((volatile uint16_t *)(l_uart_base + offset)) = (uint16_t)data;

  if (width_mask & WIDTH_BIT32)
      *((volatile uint32_t *)(l_uart_base + offset)) = (uint32_t)data;

}

void
uart_enable_txintr()
{
  uint32_t data;

  /* Enable TX interrupt by setting mask bit[5] in UARTIMSC */
  data = uart_generic_reg_read(BSA_UARTIMSC, WIDTH_BIT32);
  data = data | (1<<5);
  uart_generic_reg_write(BSA_UARTIMSC, WIDTH_BIT32, data);
}

void
uart_disable_txintr()
{
  uint32_t data;

  /* Disable TX interrupt by clearing mask bit[5] in UARTIMSC */
  data = uart_generic_reg_read(BSA_UARTIMSC, WIDTH_BIT32);
  data = data & (~(1<<5));
  uart_generic_reg_write(BSA_UARTIMSC, WIDTH_BIT32, data);

}

/* Write to a read only register*/
uint32_t
validate_register_access(uint32_t offset, uint32_t width)
{
  if (width & WIDTH_BIT8) {
      uart_generic_reg_write(offset, WIDTH_BIT8, 0xFF);
  }
  if (width & WIDTH_BIT16) {
      uart_generic_reg_write(offset, WIDTH_BIT16, 0xFFFF);
  }
  if (width & WIDTH_BIT32) {
      uart_generic_reg_write(offset, WIDTH_BIT32, 0xFFFFFFFF);
  }
  return ACS_STATUS_PASS;
}

static
void
isr()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  uart_disable_txintr();
  val_print(ACS_PRINT_DEBUG, "\n       Received interrupt on %d     ", int_id);
  val_set_status(index, RESULT_PASS(TEST_NUM, 1));
  val_gic_end_of_interrupt(int_id);
}

static
test_status_t
check_arm_generic_uart()
{

  uint32_t count = val_peripheral_get_info(NUM_UART, 0);
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t interface_type;
  test_status_t status = TEST_STATUS_UNKNOWN;

  val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
  val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);

  branch_to_test = &&exception_taken;
  if (count == 0) {
      val_print(ACS_PRINT_ERR, "\n       No UART defined by Platform      ", 0);
      if (g_build_sbsa)
          return TEST_FAIL;
      else
          return TEST_SKIP;
  }
  status = TEST_SKIP;

  while (count != 0) {
      interface_type = val_peripheral_get_info(UART_INTERFACE_TYPE, count - 1);
      if (interface_type != COMPATIBLE_FULL_16550
           && interface_type != COMPATIBLE_SUBSET_16550
           && interface_type != COMPATIBLE_GENERIC_16550)
      {
          l_uart_base = val_peripheral_get_info(UART_BASE0, count - 1);
          if (l_uart_base == 0) {
              return TEST_SKIP;
          }

          val_peripheral_uart_setup();

          /*Make sure  write to a read only register doesn't cause any exceptions*/
          validate_register_access(BSA_UARTFR, WIDTH_BIT8 | WIDTH_BIT16 | WIDTH_BIT32);
          validate_register_access(BSA_UARTRIS, WIDTH_BIT16 | WIDTH_BIT32);
          validate_register_access(BSA_UARTMIS, WIDTH_BIT16 | WIDTH_BIT32);

          status = TEST_PASS;
      }

      count--;
  }
exception_taken:
  /* Check if exception happened during the test, if yes FAIL else return test status */
  if (val_get_status(index) == TEST_FAIL)
      return TEST_FAIL;
  else
      return status;
}

static
test_status_t
check_uart_16550()
{
  uint64_t count = val_peripheral_get_info(NUM_UART, 0);
  uint32_t interface_type;
  uint32_t baud_rate;
  uint32_t access_width;
  uint32_t reg_shift;
  uint32_t width_mask;
  uint32_t ier_reg;
  uint32_t ier_scratch2;
  uint32_t ier_scratch3;
  uint32_t mcr_reg;
  uint32_t msr_status;
  uint64_t uart_base;
  uint32_t lcr_reg;
  uint32_t lcr_scratch2;
  uint32_t lcr_scratch3;
  uint32_t test_skip = 1;
  uint32_t test_fail = 0;

  if (count == 0) {
      val_print(ACS_PRINT_ERR, "\n       No UART defined by Platform      ", 0);
      if (g_build_sbsa)
          return TEST_FAIL;
      else
          return TEST_SKIP;
  }

  while (count != 0) {
      interface_type = val_peripheral_get_info(UART_INTERFACE_TYPE, count - 1);
      if (interface_type == COMPATIBLE_FULL_16550
           || interface_type == COMPATIBLE_SUBSET_16550
           || interface_type == COMPATIBLE_GENERIC_16550)
      {
          test_skip = 0;
          val_print(ACS_PRINT_DEBUG,
              "\n         UART 16550 found with instance: %x",
              count - 1);

          /* Check the I/O base address */
          uart_base = val_peripheral_get_info(UART_BASE0, count - 1);
          if (uart_base == 0)
          {
              val_print(ACS_PRINT_ERR, "\n         UART base must be specified"
                                       " for instance: %x", count - 1);
              return TEST_FAIL;
          }

          /* Check the access width (use width for reg_shift like linux earlycon) */
          access_width = val_peripheral_get_info(UART_WIDTH, count - 1);
          switch (access_width) {
          case 8:
              reg_shift  = 0;
              width_mask = WIDTH_BIT8;
              break;
          case 16:
              reg_shift  = 1;
              width_mask = WIDTH_BIT16;
              break;
          case 32:
              reg_shift  = 2;
              width_mask = WIDTH_BIT32;
              break;
          default:
              val_print(ACS_PRINT_ERR, "\n         UART access width must be specified"
                                       " for instance: %x", count - 1);
              return TEST_FAIL;
          }

          /* Check the Baudrate from the hardware map */
          baud_rate = val_peripheral_get_info(UART_BAUDRATE, count - 1);
          if (baud_rate < BAUDRATE_9600 || baud_rate > BAUDRATE_115200)
          {
              if (baud_rate != 0)
              {
                  val_print(ACS_PRINT_ERR, "\n         Baud rate %d outside"
                                           " supported range", baud_rate);
                  val_print(ACS_PRINT_ERR, " for instance %x", count - 1);
                  test_fail = 1;
              }
          }

          val_print(ACS_PRINT_ERR, "\nDEBUG: uart_base %llx", uart_base);
          val_print(ACS_PRINT_ERR, "\nDEBUG: access_width %d", access_width);


          /* Check the read/write property of Line Control Register */
          lcr_reg = uart_16550_reg_read(uart_base, LCR, reg_shift, width_mask);
          uart_16550_reg_write(uart_base, LCR, reg_shift, width_mask, 0);
          lcr_scratch2 = uart_16550_reg_read(uart_base, LCR, reg_shift, width_mask);
          uart_16550_reg_write(uart_base, LCR, reg_shift, width_mask, 0xFF);
          lcr_scratch3 = uart_16550_reg_read(uart_base, LCR, reg_shift, width_mask);
          uart_16550_reg_write(uart_base, LCR, reg_shift, width_mask, lcr_reg);
          if ((lcr_scratch2 != 0) || (lcr_scratch3 != 0xFF))
          {
              val_print(ACS_PRINT_ERR, "\n   LCR register are not read/write"
                                       " for instance: %x", count - 1);
              test_fail = 1;
          }

          /* Check the read/write property of Interrupt Enable Register */
          ier_reg = uart_16550_reg_read(uart_base, IER, reg_shift, width_mask);
          uart_16550_reg_write(uart_base, IER, reg_shift, width_mask, 0);
          ier_scratch2 = uart_16550_reg_read(uart_base, IER, reg_shift, width_mask) & 0xF;
          uart_16550_reg_write(uart_base, IER, reg_shift, width_mask, 0xF);
          ier_scratch3 = uart_16550_reg_read(uart_base, IER, reg_shift, width_mask);
          uart_16550_reg_write(uart_base, IER, reg_shift, width_mask, ier_reg);
          if ((ier_scratch2 != 0) || (ier_scratch3 != 0xF))
          {
              val_print(ACS_PRINT_ERR, "\n   IER register[0:3] are not read/write"
                                       " for instance: %x", count - 1);
              test_fail = 1;
          }

          /* Check if UART is really present using loopback test mode */
          mcr_reg = uart_16550_reg_read(uart_base, MCR, reg_shift, width_mask);
          uart_16550_reg_write(uart_base, MCR, reg_shift, width_mask, MCR_LOOP | 0xA);
          msr_status = uart_16550_reg_read(uart_base, MSR, reg_shift, width_mask);
          uart_16550_reg_write(uart_base, MCR, reg_shift, width_mask, mcr_reg);
          if ((msr_status & 0xF0) != CTS_DCD_EN)
          {
              val_print(ACS_PRINT_ERR, "\n   Loopback test mode failed"
                                       " for instance: %x", count - 1);
              test_fail = 1;
          }

      }
      count--;
  }

  if (test_skip)
      return TEST_SKIP;
  else if (test_fail)
      return TEST_FAIL;
  else
      return TEST_PASS;
}

/* This test checks if the system includes a UART that is OS-enumerable (for console/debug) or
user-accessible, it must be either a Arm Generic UART or fully 16550-compatible */
static
void
payload_check_uart_compliance()
{
  test_status_t gen_uart_status, uart_16550_status;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Run the checks for Arm generic UART and 16550 compatible UART */
  gen_uart_status = check_arm_generic_uart();
  uart_16550_status = check_uart_16550();

  /* Pass if UART is found and is either one of the type required by test */
  if (gen_uart_status == TEST_PASS || uart_16550_status == TEST_PASS)
      val_set_status(index, RESULT_PASS(TEST_NUM, 1));
  /* Skip if UART not found by test */
  else if (gen_uart_status == TEST_SKIP || uart_16550_status == TEST_SKIP)
      val_set_status(index, RESULT_SKIP(TEST_NUM, 1));
  /* Fail if neither passed or both skipped */
  else
      val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
  return;
}

static
void
payload_check_arm_generic_uart_interrupt()
{
  uint32_t count = val_peripheral_get_info(NUM_UART, 0);
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t timeout;
  uint32_t interface_type;

  if (count == 0) {
      val_print(ACS_PRINT_ERR, "\n       No UART defined by Platform      ", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM1, 1));
      return;
  }
  val_set_status(index, RESULT_SKIP(TEST_NUM1, 2));
  while (count != 0) {
      timeout = TIMEOUT_LARGE;
      int_id    = val_peripheral_get_info(UART_GSIV, count - 1);
      interface_type = val_peripheral_get_info(UART_INTERFACE_TYPE, count - 1);
      l_uart_base = val_peripheral_get_info(UART_BASE0, count - 1);
      if (interface_type != COMPATIBLE_FULL_16550
           && interface_type != COMPATIBLE_SUBSET_16550
           && interface_type != COMPATIBLE_GENERIC_16550) {

          /* If Interrupt ID is available, check for interrupt generation */
          if (int_id != 0x0) {
              /* PASS will be set from ISR */
              val_set_status(index, RESULT_PENDING(TEST_NUM1));

              /* Check int_id is SPI or ESPI */
              if (!(IsSpi(int_id)) && !(val_gic_is_valid_espi(int_id))) {
                 val_print(ACS_PRINT_ERR, "\n       Interrupt-%d is neither SPI nor ESPI", int_id);
                 val_set_status(index, RESULT_FAIL(TEST_NUM1, 2));
                 return;
              }

               val_peripheral_uart_setup();
               /* If UART is used by other app (optee/TF) skip it */
               if (((uart_generic_reg_read(BSA_UARTIMSC, WIDTH_BIT32)) & 0x10))
               {
                    count--;
                    continue;
               }

              /* Install ISR */
              if (val_gic_install_isr(int_id, isr)) {
                 val_print(ACS_PRINT_ERR, "\n       GIC Install Handler Failed...", 0);
                 val_set_status(index, RESULT_FAIL(TEST_NUM1, 3));
                 return;
              }

              uart_enable_txintr();
              val_print_raw(l_uart_base, g_print_level,
                            "\n       Test Message                          ", 0);

              while ((--timeout > 0) && (IS_RESULT_PENDING(val_get_status(index)))) {
              };

              if (timeout == 0) {
                 val_print(ACS_PRINT_ERR,
                 "\n       Did not receive UART interrupt on %d  ",
                 int_id);
                 test_fail++;
              }
          } else {
              val_set_status(index, RESULT_SKIP(TEST_NUM1, 3));
          }
      }
      count--;
  }

  if (test_fail)
    val_set_status(index, RESULT_FAIL(TEST_NUM1, 4));
  else
    val_set_status(index, RESULT_PASS(TEST_NUM1, 2));

  return;
}


/**
   @brief    Verify UART registers for Read-only bits and also
             enable interrupt generation
**/
uint32_t
d003_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  /* This test is run on single processor */
  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload_check_uart_compliance, 0);

  /* get the result from all PE and check for failure */
  if (g_build_sbsa)
      status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE_SBSA);
  else
      status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE_BSA);

  val_report_status(0, ACS_END(TEST_NUM), NULL);
  return status;
}

uint32_t
d006_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  /* This test is run on single processor */
  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM1, TEST_DESC1, val_pe_get_num());
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM1, num_pe, payload_check_arm_generic_uart_interrupt, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM1, num_pe, TEST_RULE1);
  val_report_status(0, ACS_END(TEST_NUM1), NULL);
  return status;
}