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

#include <common/runtime_svc.h>
#include <acs_el3_handler.h>

/* Write timer control register. */
static void timer_write_ctl(acs_timer_type_t timer, uint64_t value)
{
    if (timer == ACS_TIMER_CNTHPS)
        acs_write_cnthps_ctl_el2(value);
    else
        acs_write_cnthvs_ctl_el2(value);
}

/* Write timer compare value register. */
static void timer_write_cval(acs_timer_type_t timer, uint64_t value)
{
    if (timer == ACS_TIMER_CNTHPS)
        acs_write_cnthps_cval_el2(value);
    else
        acs_write_cnthvs_cval_el2(value);
}

/* Read timer control register. */
static uint64_t timer_read_ctl(acs_timer_type_t timer)
{
    if (timer == ACS_TIMER_CNTHPS)
        return acs_read_cnthps_ctl_el2();

    return acs_read_cnthvs_ctl_el2();
}

/* Read timer compare value register. */
static uint64_t timer_read_cval(acs_timer_type_t timer)
{
    if (timer == ACS_TIMER_CNTHPS)
        return acs_read_cnthps_cval_el2();

    return acs_read_cnthvs_cval_el2();
}

/* Probe the Secure EL2 timer PPI INTID. */
static acs_smc_status_t probe_secure_timer_ppi(uint64_t timeout,
    uintptr_t gicr_base, acs_timer_type_t timer, uint32_t *intid)
{
    uint64_t start;
    uint64_t ctl;
    uint64_t poll_timeout;
    uint32_t pending_before;
    uint32_t pending_after;
    uint32_t new_pending;
    uint64_t saved_ctl;
    uint64_t saved_cval;
    uint32_t i;

    /* Validate input parameters. */
    if ((gicr_base == 0U) || (intid == NULL))
        return ACS_SMC_ERR_INVALID_PARAM;

    /* Validate the timer probe timeout. */
    if ((timeout == 0U) || (timeout > ACS_TIMER_PROBE_MAX_TIMEOUT)) {
        WARN("ACS/EL3: invalid timer probe timeout: 0x%llx\n", (unsigned long long)timeout);
        return ACS_SMC_ERR_INVALID_PARAM;
    }

    poll_timeout = timeout * 2ULL;

    /* Save the PPI pending state before starting the probe. */
    pending_before = mmio_read_32(gicr_base + ACS_GICR_ISPENDR0);
    pending_before &= 0xffff0000U;

    /*
     * Caution: The Secure EL2 timer is temporarily reprogrammed for
     * PPI discovery and restored after the probe.
     */
    saved_ctl = timer_read_ctl(timer);
    saved_cval = timer_read_cval(timer);

    /* Program and enable the timer for the probe. */
    timer_write_ctl(timer, 0U);
    timer_write_cval(timer, read_cntpct_el0() + timeout);
    timer_write_ctl(timer, 1U);

    start = read_cntpct_el0();

    /* Wait for the timer to expire within the polling timeout. */
    do {
        ctl = timer_read_ctl(timer);
    } while (((ctl & 0x4ULL) == 0ULL) &&
             ((read_cntpct_el0() - start) < poll_timeout));

    if ((ctl & 0x4ULL) == 0ULL) {
        timer_write_ctl(timer, 0U);
        timer_write_cval(timer, saved_cval);
        timer_write_ctl(timer, saved_ctl);
        return ACS_SMC_ERR_TIMEOUT;
    }

    /* Identify PPIs that became pending during the probe. */
    pending_after = mmio_read_32(gicr_base + ACS_GICR_ISPENDR0);
    new_pending = (pending_after & ~pending_before) & 0xffff0000U;

    /* Restore original timer state. */
    timer_write_ctl(timer, 0U);
    timer_write_cval(timer, saved_cval);
    timer_write_ctl(timer, saved_ctl);

    /* Exactly one new PPI must be pending. */
    if ((new_pending == 0U) ||
        ((new_pending & (new_pending - 1U)) != 0U)) {
        WARN("ACS/EL3: unable to uniquely identify timer PPI: 0x%x\n", new_pending);
        return ACS_SMC_ERR_INVALID_STATE;
    }

    /* Get and clear the PPI generated during the probe. */
    for (i = 16U; i < 32U; i++) {
        if ((new_pending & (1U << i)) != 0U) {
            *intid = i;
            mmio_write_32(gicr_base + ACS_GICR_ICPENDR0, 1U << i);
            return ACS_SMC_SUCCESS;
        }
    }
    return ACS_SMC_ERR_INVALID_STATE;
}

/* Handle ACS EL3 SMC services. */
uintptr_t __wrap_plat_arm_acs_smc_handler(unsigned int smc_fid, uint64_t services,
    uint64_t arg0, uint64_t arg1, uint64_t arg2, void *handle)
{
    acs_smc_status_t status;
    uint32_t intid;

    INFO("ACS/EL3: handler entered fid=0x%x service=0x%lx\n", smc_fid, services);

    if (!is_acs_fid(smc_fid)) {
        SMC_RET1(handle, SMC_UNK);
    }

    switch ((acs_service_t)services) {
    case ACS_SMC_READ_BASE_CNTFREQ: {
        uintptr_t cntctl = PLATFORM_OVERRIDE_SYS_COUNTER_CNTCTL_BASE;
        uintptr_t cntfid_addr;
        uint32_t cntsr, freq;
        if (cntctl == 0U) {
            WARN("ACS/EL3: null CNTCTL base\n");
            SMC_RET1(handle, SMC_UNK);
        }
        /* Read the System Counter Status Register. */
        cntsr = mmio_read_32(cntctl + CNTSR_OFFSET);
        /* Read the first implemented counter frequency (CNTFID0). */
        cntfid_addr = cntctl + CNTFID_BASE_OFFSET;
        freq = mmio_read_32(cntfid_addr);
        INFO("ACS/EL3: CNTSR=0x%x CNTFID0@0x%lx=%u Hz\n",
            cntsr, (unsigned long)cntfid_addr, freq);
        SMC_RET2(handle, ACS_SMC_SUCCESS, (uint64_t)freq);
    }
    case ACS_SMC_GET_CNTHPS_INTID: {
        intid = 0U;
        status = probe_secure_timer_ppi(arg0, (uintptr_t)arg1, ACS_TIMER_CNTHPS, &intid);
        if (status != ACS_SMC_SUCCESS) {
            WARN("ACS/EL3: failed to probe CNTHPS PPI, status=%u\n", (unsigned int)status);
            SMC_RET2(handle, status, 0U);
        }
        INFO("ACS/EL3: CNTHPS INTID=%u\n", intid);
        SMC_RET2(handle, ACS_SMC_SUCCESS, intid);
    }
    case ACS_SMC_GET_CNTHVS_INTID: {
        intid = 0U;
        status = probe_secure_timer_ppi(arg0, (uintptr_t)arg1, ACS_TIMER_CNTHVS, &intid);
        if (status != ACS_SMC_SUCCESS) {
            WARN("ACS/EL3: failed to probe CNTHVS PPI, status=%u\n", (unsigned int)status);
            SMC_RET2(handle, status, 0U);
        }
        INFO("ACS/EL3: CNTHVS INTID=%u\n", intid);
        SMC_RET2(handle, ACS_SMC_SUCCESS, intid);
    }
    default:
        WARN("ACS/EL3: unknown service 0x%llx\n", (unsigned long long)services);
        SMC_RET1(handle, SMC_UNK);
    }
 }
