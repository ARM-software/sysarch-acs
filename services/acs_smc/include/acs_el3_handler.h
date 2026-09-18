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

#ifndef ACS_EL3_H
#define ACS_EL3_H

#include <stdint.h>

/* ACS vendor EL3 SMC Function ID. */
#define ARM_VEN_EL3_ACS_SMC_HANDLER    UINT64_C(0xC7000030)

#define is_acs_fid(fid) \
        ((fid) == ARM_VEN_EL3_ACS_SMC_HANDLER)

/* ACS EL3 service identifiers. */
typedef enum {
        ACS_SMC_READ_BASE_CNTFREQ = 0x01U,
        ACS_SMC_GET_CNTHPS_INTID,
        ACS_SMC_GET_CNTHVS_INTID,
} acs_service_t;

/* ACS EL3 SMC service return status. */
typedef enum {
    ACS_SMC_SUCCESS = 0U,
    ACS_SMC_ERR_INVALID_PARAM,
    ACS_SMC_ERR_TIMEOUT,
    ACS_SMC_ERR_INVALID_STATE
} acs_smc_status_t;

/* Secure EL2 timer types used for PPI discovery. */
typedef enum {
    ACS_TIMER_CNTHPS,
    ACS_TIMER_CNTHVS
} acs_timer_type_t;

/* System counter register offsets. */
#define CNTSR_OFFSET        0x004U
#define CNTFID_BASE_OFFSET  0x020U

/* GIC Redistributor SGI/PPI frame register offsets from GICR base. */
#define ACS_GICR_ISPENDR0   0x10200U
#define ACS_GICR_ICPENDR0   0x10280U

#define ACS_TIMER_PROBE_MAX_TIMEOUT    UINT64_C(0x1000000)

uintptr_t __wrap_plat_arm_acs_smc_handler(unsigned int smc_fid,
    uint64_t services, uint64_t arg0, uint64_t arg1, uint64_t arg2, void *handle);

/* Secure EL2 physical timer register helpers. */
void acs_write_cnthps_ctl_el2(uint64_t value);
void acs_write_cnthps_cval_el2(uint64_t value);
uint64_t acs_read_cnthps_ctl_el2(void);
uint64_t acs_read_cnthps_cval_el2(void);

/* Secure EL2 virtual timer register helpers. */
void acs_write_cnthvs_ctl_el2(uint64_t value);
void acs_write_cnthvs_cval_el2(uint64_t value);
uint64_t acs_read_cnthvs_ctl_el2(void);
uint64_t acs_read_cnthvs_cval_el2(void);
#endif /* ACS_EL3_H */
