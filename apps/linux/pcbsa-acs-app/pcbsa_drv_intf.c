/** @file
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
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


#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include "include/pcbsa_drv_intf.h"

typedef
struct __PCBSA_DRV_PARMS__ {
    unsigned int    api_num;
    unsigned int    num_pe;
    unsigned int    level;
    unsigned long   arg0;
    unsigned long   arg1;
    unsigned long   arg2;
} pcbsa_drv_parms_t;


int
call_drv_get_status(unsigned long int *arg0, unsigned long int *arg1, unsigned long int *arg2)
{

    FILE  *fd = NULL;
    pcbsa_drv_parms_t test_params;


    fd = fopen("/proc/pcbsa", "r");

    if (fd == NULL)
    {
        printf("fopen failed\n");
        return 1;
    }
  fread(&test_params, 1, sizeof(test_params), fd);

  fclose(fd);

  *arg0 = test_params.arg0;
  *arg1 = test_params.arg1;
  *arg2 = test_params.arg2;

  return test_params.api_num;
}

int
call_drv_wait_for_completion()
{
  unsigned long int arg0, arg1, arg2;

  arg0 = DRV_STATUS_PENDING;

  while (arg0 == DRV_STATUS_PENDING) {
    call_drv_get_status(&arg0, &arg1, &arg2);
    read_from_proc_pcbsa_msg();
  }

  return arg1;
}


int
call_drv_init_test_env(unsigned int print_level)
{
    FILE             *fd = NULL;
    pcbsa_drv_parms_t test_params;
    int status = 0;

    fd = fopen("/proc/pcbsa", "rw+");

    if (fd == NULL)
    {
        printf("fopen failed\n");
        return 1;
    }

    test_params.api_num  = PCBSA_CREATE_INFO_TABLES;
    test_params.arg1     = print_level;
    test_params.arg2     = 0;

    fwrite(&test_params, 1, sizeof(test_params), fd);

    fclose(fd);

    status = call_drv_wait_for_completion();

    return status;
}

int
call_drv_clean_test_env()
{
    FILE             *fd = NULL;
    pcbsa_drv_parms_t test_params;

    fd = fopen("/proc/pcbsa", "rw+");

    if (fd == NULL)
    {
        printf("fopen failed\n");
        return 1;
    }

    test_params.api_num  = PCBSA_FREE_INFO_TABLES;
    test_params.arg1     = 0;
    test_params.arg2     = 0;

    fwrite(&test_params, 1, sizeof(test_params), fd);

    fclose(fd);

    call_drv_wait_for_completion();
    return 0;
}

int
call_drv_execute_test(unsigned int api_num, unsigned int num_pe,
  unsigned int level, unsigned int print_level, unsigned long int test_input)
{
    FILE             *fd = NULL;
    pcbsa_drv_parms_t test_params;

    fd = fopen("/proc/pcbsa", "rw+");

    if (fd == NULL)
    {
        printf("fopen failed\n");
        return 1;
    }

    test_params.api_num  = api_num;
    test_params.num_pe   = num_pe;
    test_params.level    = level;
    test_params.arg0     = test_input;
    test_params.arg1     = print_level;
    test_params.arg2     = 0;

    fwrite(&test_params, 1, sizeof(test_params), fd);

    fclose(fd);
    return 0;

}

int
call_update_skip_list(unsigned int api_num, int *p_skip_test_num)
{
    FILE             *fd = NULL;
    pcbsa_drv_parms_t test_params;

    fd = fopen("/proc/pcbsa", "rw+");

    if (fd == NULL)
    {
        printf("fopen failed\n");
        return 1;
    }

    test_params.api_num  = api_num;
    test_params.num_pe   = 0;
    test_params.level    = 0;
    test_params.arg0     = p_skip_test_num[0];
    test_params.arg1     = p_skip_test_num[1];
    test_params.arg2     = p_skip_test_num[2];

    fwrite(&test_params, 1, sizeof(test_params), fd);

    fclose(fd);
    return 0;

}

typedef struct __PCBSA_MSG__ {
    char string[92];
    unsigned long data;
} pcbsa_msg_parms_t;

int read_from_proc_pcbsa_msg(void)
{
  char buf_msg[sizeof(pcbsa_msg_parms_t)];

  FILE  *fd = NULL;
  pcbsa_msg_parms_t msg_params;

  fd = fopen("/proc/pcbsa_msg", "r");

  if (fd == NULL) {
    printf("fopen failed\n");
    return 1;
  }

  /* Print Until buffer is empty */

  while (fread(buf_msg, sizeof(buf_msg), 1, fd))
    printf("%s", buf_msg);

  fclose(fd);
  return 0;
}

