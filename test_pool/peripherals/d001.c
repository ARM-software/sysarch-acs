/** @file
 * Copyright (c) 2016-2019, 2024-2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/acs_peripherals.h"
#include "val/include/acs_pcie.h"

#define TEST_NUM   (ACS_PER_TEST_NUM_BASE + 1)
#define TEST_RULE  "B_PER_01"
#define TEST_DESC  "USB CTRL Interface EHCI check         "

#define TEST_NUM1   (ACS_PER_TEST_NUM_BASE + 8)
#define TEST_RULE1  "B_PER_02"
#define TEST_DESC1  "USB CTRL Interface XHCI check         "

/**
  @brief   This functions checks if USBs in system implements preferred type passed as input.
  @param   usb_type - Preferred USB type
  @return  test_status_t - Status of the check (PASS/FAIL/SKIP)
**/
test_status_t check_for_usb_intrf (uint32_t usb_type)
{
    uint32_t interface = 0;
    uint32_t ret;
    uint32_t bdf;
    uint32_t fail_cnt = 0;
    uint32_t usb_pref, usb_alt, progif_pref, progif_alt;
    uint64_t count = val_peripheral_get_info(NUM_USB, 0);

    val_print(ACS_PRINT_DEBUG, "\n       Num of  USB CTRL %d detected", count);
    /* If USB peripheral count is zero, skip the test */
    if (count == 0) {
        return TEST_SKIP;
    }

    /* Deduce preferred and alternate USB type based on input */
    if (usb_type == USB_TYPE_EHCI) {
        usb_pref = USB_TYPE_EHCI;
        progif_pref = PCIE_PROGIF_EHCI;
        usb_alt = USB_TYPE_XHCI;
        progif_alt = PCIE_PROGIF_XHCI;
    } else {
        usb_pref = USB_TYPE_XHCI;
        progif_pref = PCIE_PROGIF_XHCI;
        usb_alt = USB_TYPE_EHCI;
        progif_alt = PCIE_PROGIF_EHCI;
    }

    while (count != 0) {
        /* If DT system */
        if (val_peripheral_get_info(USB_PLATFORM_TYPE, count - 1) == PLATFORM_TYPE_DT) {
            val_print(ACS_PRINT_INFO, "\n       USB %d info from DT table", count - 1);

            interface = val_peripheral_get_info(USB_INTERFACE_TYPE, count - 1);
            val_print(ACS_PRINT_DEBUG, "\n       USB interface is %d", interface);
            if (interface != usb_pref) {
                /* Continue if USB implements allowed alternative else fail */
                if (interface == usb_alt) {
                    count--;
                    continue;
                } else {
                    val_print(ACS_PRINT_WARN, "\n       Detected USB CTRL %d supports", count - 1);
                    val_print(ACS_PRINT_WARN, " %x interface and not EHCI/XHCI", interface);
                    fail_cnt++;
                }
            }
        /* For non-DT system */
        } else {
            bdf = val_peripheral_get_info(USB_BDF, count - 1);
            val_print(ACS_PRINT_DEBUG, "\n       USB bdf %lx info from non DT table", bdf);
            val_print(ACS_PRINT_DEBUG, "\n       USB %d info from non DT", count - 1);

            ret = val_pcie_read_cfg(bdf, TYPE01_CCR_SHIFT, &interface);
            /* Extract programming interface field as per PCI Code and ID Assignment
               Specification */
            interface = (interface >> TYPE01_CCR_SHIFT) & 0xFF;
            val_print(ACS_PRINT_DEBUG, "\n       USB interface value is %lx", interface);

            if (ret == PCIE_NO_MAPPING || (interface < PCIE_PROGIF_EHCI) || (interface == 0xFF)) {
                val_print(ACS_PRINT_INFO, "\n       WARN: USB CTRL ECAM access failed 0x%x  ",
                          interface);
                val_print(ACS_PRINT_INFO, "\n       Re-checking using PCIIO protocol",
                          0);
                ret = val_pcie_io_read_cfg(bdf, TYPE01_CCR_SHIFT, &interface);
                if (ret == PCIE_NO_MAPPING) {
                    val_print(ACS_PRINT_DEBUG,
                              "\n       Reading device class code using PciIo protocol failed "
                              , 0);
                    fail_cnt++;
                }

                interface = (interface >> TYPE01_CCR_SHIFT) & 0xFF;
                val_print(ACS_PRINT_DEBUG, "\n       (PCIIO) USB interface value is %lx",
                                                                                        interface);

                if (interface != progif_pref) {
                    /* Continue if USB implements allowed alternative else fail */
                    if (interface == progif_alt) {
                        count--;
                        continue;
                    } else {
                        val_print(ACS_PRINT_WARN, "\n       Detected USB CTRL %d supports",
                                  count - 1);
                        val_print(ACS_PRINT_WARN, " %x interface and not EHCI/XHCI", interface);
                        fail_cnt++;
                    }
                }
            }
        }
        count--;
    }

    return fail_cnt ? TEST_FAIL : TEST_PASS;
}

static
void
payload_ehci_check()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  test_status_t status;

  /* Check if USB implements EHCI. If not, skip if it's XHCI; otherwise, fail. */
  status = check_for_usb_intrf(USB_TYPE_EHCI);

  if (status == TEST_SKIP) {
      val_set_status(index, RESULT_SKIP(TEST_NUM, 1));
  } else if (status == TEST_FAIL) {
      val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
  } else {
     val_set_status(index, RESULT_PASS(TEST_NUM, 1));
  }
}

static
void
payload_xhci_check()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  test_status_t status;

  /* Check if USB implements XHCI. If not, skip if it's EHCI; otherwise, fail. */
  status = check_for_usb_intrf(USB_TYPE_XHCI);

  if (status == TEST_SKIP) {
      val_set_status(index, RESULT_SKIP(TEST_NUM1, 1));
  } else if (status == TEST_FAIL) {
      val_set_status(index, RESULT_FAIL(TEST_NUM1, 1));
  } else {
     val_set_status(index, RESULT_PASS(TEST_NUM1, 1));
  }
}


uint32_t
d001_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  num_pe = 1;  //This test is run on single processor
  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload_ehci_check, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);
  return status;
}

uint32_t
d008_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  num_pe = 1;  //This test is run on single processor
  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM1, TEST_DESC1, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM1, num_pe, payload_xhci_check, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM1, num_pe, TEST_RULE1);
  val_report_status(0, ACS_END(TEST_NUM1), NULL);
  return status;
}