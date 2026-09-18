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

        .text
        .align  2

        .globl acs_write_cnthps_ctl_el2
        .globl acs_read_cnthps_ctl_el2
        .globl acs_write_cnthps_cval_el2
        .globl acs_write_cnthvs_ctl_el2
        .globl acs_read_cnthvs_ctl_el2
        .globl acs_write_cnthvs_cval_el2
        .globl acs_read_cnthps_cval_el2
        .globl acs_read_cnthvs_cval_el2

/* Write CNTHPS_CTL_EL2 register. */
acs_write_cnthps_ctl_el2:
        msr     CNTHPS_CTL_EL2, x0
        isb
        ret

/* Read CNTHPS_CTL_EL2 register. */
acs_read_cnthps_ctl_el2:
        mrs     x0, CNTHPS_CTL_EL2
        ret

/* Write CNTHPS_CVAL_EL2 register. */
acs_write_cnthps_cval_el2:
        msr     CNTHPS_CVAL_EL2, x0
        isb
        ret

/* Write CNTHVS_CTL_EL2 register. */
acs_write_cnthvs_ctl_el2:
        msr     CNTHVS_CTL_EL2, x0
        isb
        ret

/* Read CNTHVS_CTL_EL2 register. */
acs_read_cnthvs_ctl_el2:
        mrs     x0, CNTHVS_CTL_EL2
        ret

/* Write CNTHVS_CVAL_EL2 register. */
acs_write_cnthvs_cval_el2:
        msr     CNTHVS_CVAL_EL2, x0
        isb
        ret

/* Read CNTHPS_CVAL_EL2 register. */
acs_read_cnthps_cval_el2:
        mrs     x0, CNTHPS_CVAL_EL2
        ret

/* Read CNTHVS_CVAL_EL2 register. */
acs_read_cnthvs_cval_el2:
        mrs     x0, CNTHVS_CVAL_EL2
        ret