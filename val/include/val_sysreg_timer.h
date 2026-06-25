/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef VAL_SYSREG_TIMER_H
#define VAL_SYSREG_TIMER_H

#include "val_sysreg.h"

SYSREG_READ_FUNC(s3_0_c0_c7_4)
SYSREG_RW_FUNCS(cntfrq_el0)
SYSREG_RW_FUNCS(cnthp_ctl_el2)
SYSREG_RW_FUNCS(cnthp_tval_el2)
SYSREG_RW_FUNCS(cnthv_ctl_el2)
SYSREG_RW_FUNCS(cnthv_tval_el2)
SYSREG_RW_FUNCS(cnthp_cval_el2)
SYSREG_RW_FUNCS(cnthv_cval_el2)
SYSREG_RW_FUNCS(cntps_ctl_el1)
SYSREG_RW_FUNCS(cntps_tval_el1)
SYSREG_RW_FUNCS(cntps_cval_el1)
SYSREG_RW_FUNCS(cntp_ctl_el0)
SYSREG_RW_FUNCS(cntp_ctl_el02)
SYSREG_RW_FUNCS(cntp_tval_el0)
SYSREG_RW_FUNCS(cntp_tval_el02)
SYSREG_RW_FUNCS(cntp_cval_el0)
SYSREG_RW_FUNCS(cntp_cval_el02)
SYSREG_READ_FUNC(cntpctss_el0)
SYSREG_RW_FUNCS(cntv_ctl_el0)
SYSREG_RW_FUNCS(cntv_ctl_el02)
SYSREG_RW_FUNCS(cntv_tval_el0)
SYSREG_RW_FUNCS(cntv_tval_el02)
SYSREG_RW_FUNCS(cntv_cval_el0)
SYSREG_RW_FUNCS(cntv_cval_el02)
SYSREG_READ_FUNC(cntvctss_el0)
SYSREG_RW_FUNCS(cnthctl_el2)
SYSREG_RW_FUNCS(cntkctl_el1)
SYSREG_RW_FUNCS(cntkctl_el12)
SYSREG_RW_FUNCS(cntvoff_el2)

#endif /* VAL_SYSREG_TIMER_H */
