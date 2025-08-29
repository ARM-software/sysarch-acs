
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

#include "pal_common_support.h"
#include "platform_image_def.h"
#include "platform_override_struct.h"
#include "platform_override_fvp.h"
#include "pal_common_support.h"
#include "pal_pcie_enum.h"

extern PLATFORM_OVERRIDE_GIC_INFO_TABLE     platform_gic_cfg;
extern PLATFORM_OVERRIDE_TIMER_INFO_TABLE   platform_timer_cfg;
extern PLATFORM_OVERRIDE_IOVIRT_INFO_TABLE  platform_iovirt_cfg;
extern PLATFORM_OVERRIDE_NODE_DATA          platform_node_type;
extern PLATFORM_OVERRIDE_UART_INFO_TABLE    platform_uart_cfg;
extern PLATFORM_OVERRIDE_MEMORY_INFO_TABLE  platform_mem_cfg;
extern PCIE_INFO_TABLE                      platform_pcie_cfg;
extern WD_INFO_TABLE                        platform_wd_cfg;

/** SMMU API's **/
/**
  @brief  Platform defined method to check if CATU is behind an ETR device

  @param  etr_path  full path of ETR device

  @return 0 - Success, NOT_IMPLEMENTED - API not implemented, Other values - Failure
**/
uint32_t
pal_smmu_is_etr_behind_catu(char *etr_path)
{
  (void) etr_path;

  return NOT_IMPLEMENTED;
}

/**
  @brief  Check for the _DSM method to obtain the STE value

  @param  None

  @return 0 - Failure, NOT_IMPLEMENTED - DSM Method not implemented, Other values - PASS
**/
uint32_t pal_pcie_dsm_ste_tags(void)
{

    return NOT_IMPLEMENTED;
}

/**
  @brief   This API checks if pmu monitor count value is valid
  @param   interface_acpiid - acpiid of interface
  @param   count_value - monitor count value
  @param   eventid - eventid
  @return  0 - monitor count value is valid
           non-zero - error or invallid count value
**/
uint32_t
pal_pmu_check_monitor_count_value(uint64_t interface_acpiid, uint32_t count_value, uint32_t eventid)
{
  (void) interface_acpiid;
  (void) count_value;
  (void) eventid;

  return NOT_IMPLEMENTED;
}

/**
  @brief   This API generates required workload for given pmu node and event id
  @param   interface_acpiid - acpiid of interface
  @param   pmu_node_index - pmu node index
  @param   mon_index - monitor index
  @param   eventid - eventid
  @return  0 - success status
           non-zero - error status
**/
uint32_t
pal_generate_traffic(uint64_t interface_acpiid, uint32_t pmu_node_index,
                                     uint32_t mon_index, uint32_t eventid)
{
  (void) interface_acpiid;
  (void) pmu_node_index;
  (void) mon_index;
  (void) eventid;

  return NOT_IMPLEMENTED;
}

/**
  @brief   This API checks if PMU node is secure only.
  @param   interface_acpiid - acpiid of interface
  @param   num_traffic_type_support - num of traffic type supported.
  @return  0 - success status
           non-zero - error status
**/
uint32_t
pal_pmu_get_multi_traffic_support_interface(uint64_t *interface_acpiid,
                                                       uint32_t *num_traffic_type_support)
{
  (void) interface_acpiid;
  (void) num_traffic_type_support;

  return NOT_IMPLEMENTED;
}

/**
  @brief  API to check support for Poison

  @param  None

  @return  0 - Poison storage & forwarding not supported
           1 - Poison storage & forwarding supported
**/
uint32_t
pal_ras_check_plat_poison_support()
{
  return 0;
}

/**
  @brief  Platform Defined way of setting up the Error Environment

  @param  in_param  - Error Input Parameters.
  @param  *out_param  - Parameters returned from platform to be used in the test.

  @return  0 - Success, NOT_IMPLEMENTED - API not implemented, Other values - Failure
**/
uint32_t
pal_ras_setup_error(RAS_ERR_IN_t in_param, RAS_ERR_OUT_t *out_param)
{
  /* Platform Defined way of setting up the Error Environment */

  (void) in_param;
  (void) out_param;

  return NOT_IMPLEMENTED;
}

/**
  @brief  Platform Defined way of injecting up the Error Environment

  @param  in_param  - Error Input Parameters.
  @param  *out_param  - Parameters returned from platform to be used in the test.

  @return  0 - Success, NOT_IMPLEMENTED - API not implemented, Other values - Failure
**/
uint32_t
pal_ras_inject_error(RAS_ERR_IN_t in_param, RAS_ERR_OUT_t *out_param)
{
  (void) in_param;
  (void) out_param;

  return NOT_IMPLEMENTED;
}

