/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt_probe.h"

#include "pal_firme_gpt_memory.h"
#include "val_common/val_mmu.h"
#include "val_common/val_status.h"

extern uint8_t __text_start__[];
extern uint8_t __stack_end__[];

/** @brief Return actual native-PAS access evidence only for PAL-reserved test RAM. */
void val_firme_gpt_probe_command(uint64_t address, firme_acs_call_res_t *response)
{
    val_fault_info_t fault = {0};
    uint64_t value = 0ULL;

    if (!response)
        return;
    *response = (firme_acs_call_res_t){.x0 = ACS_STATUS_ERR};
    if (!pal_firme_gpt_memory_probe_allowed(address) ||
        val_mmu_probe_native_read64_el2(address,
            (uint64_t)(uintptr_t)__text_start__, (uint64_t)(uintptr_t)__stack_end__,
            &value, &fault) != VAL_SUCCESS)
        return;

    response->x0 = ACS_STATUS_PASS;
    response->x1 = fault.faulted;
    response->x2 = fault.faulted ? fault.esr : value;
    response->x3 = fault.far;
}
