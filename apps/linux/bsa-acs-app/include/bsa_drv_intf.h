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

#ifndef __BSA_DRV_INTF_H__
#define __BSA_DRV_INTF_H__
#include <stdint.h>

/* API NUMBERS to COMMUNICATE with DRIVER */

#define BSA_CREATE_INFO_TABLES      0x1000
#define BSA_PCIE_EXECUTE_TEST       0x2000
#define BSA_UPDATE_SKIP_LIST        0x3000
#define BSA_EXERCISER_EXECUTE_TEST  0x4000
#define BSA_UPDATE_SW_VIEW          0x5000
#define BSA_PER_EXECUTE_TEST        0x6000
#define BSA_MEM_EXECUTE_TEST        0x7000
#define BSA_FREE_INFO_TABLES        0x9000
#define RUN_TESTS                   0xA000

/* STATUS MESSAGES */
#define DRV_STATUS_AVAILABLE     0x10000000
#define DRV_STATUS_PENDING       0x40000000

/* New ioctl-based array update interface (user-space side) */
#define BSA_IOCTL_MAGIC            'B'
#define BSA_IOCTL_UPDATE_ARRAY     _IOW(BSA_IOCTL_MAGIC, 0x01, int)

#define SKIP_RULE_LIST             0x2
#define RULE_LIST                  0x3
#define BSA_ARRAY_ELEM_MAX_COUNT   100U

typedef struct bsa_array_update_u {
    uint32_t hint;
    uint32_t count;
    uint32_t reserved;
    uint64_t user_buf; /* userspace pointer to u32 buffer */
} bsa_array_update_u_t;

/* Function Prototypes */

int
call_drv_init_test_env();

int
call_drv_clean_test_env();

int
call_drv_execute_test(unsigned int test_num, unsigned int num_pe,
  unsigned int print_level, unsigned long int test_input);

int
call_update_skip_list(unsigned int api_num, int *p_skip_test_num);

int
call_update_sw_view(unsigned int api_num, int *p_sw_view);

int
call_drv_wait_for_completion();

int read_from_proc_bsa_msg();

/* send a u32 array to the driver via ioctl */
int bsa_send_array_u32(uint32_t hint, const uint32_t *arr, uint32_t count);

#endif
