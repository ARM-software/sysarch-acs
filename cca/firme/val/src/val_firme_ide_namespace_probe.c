/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_ide_namespace_probe.h"

#include "pal_cxl.h"
#include "val_common/val_fault.h"
#include "val_common/val_mp.h"
#include "val_log.h"

#define VAL_IDE_NAMESPACE_PATTERN UINT64_C(0x4e53494450414952)
#define VAL_IDE_NAMESPACE_DATA_ABORT_CURRENT 0x25U
#define VAL_IDE_NAMESPACE_DATA_ABORT_LOWER 0x24U
#define VAL_IDE_NAMESPACE_EXTERNAL_ABORT 0x10U
#define VAL_IDE_NAMESPACE_FAULT_STATUS_MASK 0x3fU

/** @brief Reject observations collected after a fixture release or remapping. */
static uint32_t val_namespace_same_aperture(const pal_cxl_traffic_snapshot_t *first,
                                            const pal_cxl_traffic_snapshot_t *second)
{
    return first->lease != 0ULL && first->lease == second->lease &&
           first->address == second->address;
}

/**
 * @brief Prove an RX-keyset STOP does not disable a different TX keyset.
 * @param probe Output witness tied to this PE and the owned CXL aperture.
 * @return VAL_SUCCESS only for an authenticated TX write and RX-rejected read.
 */
int val_firme_ide_namespace_rx_stopped(val_firme_ide_namespace_probe_t *probe)
{
    pal_cxl_traffic_snapshot_t before = {0};
    pal_cxl_traffic_snapshot_t written = {0};
    pal_cxl_traffic_snapshot_t rejected = {0};
    val_fault_info_t fault = {0};
    uint64_t readback = 0ULL;
    uint64_t pattern;
    uint32_t exception_class;
    int status;

    if (!probe || probe->valid)
        return VAL_ERROR;
    *probe = (val_firme_ide_namespace_probe_t){0};
    if (pal_cxl_traffic_snapshot(&before) != VAL_SUCCESS || !before.lease)
        return VAL_ERROR;
    pattern = VAL_IDE_NAMESPACE_PATTERN ^ before.lease ^ before.tx_completed;

    /* Step 1: Issue a fresh write while only the Root Port RX keyset is stopped. */
    if (val_probe_write64(before.address, pattern, &fault) != VAL_SUCCESS ||
        pal_cxl_traffic_snapshot(&written) != VAL_SUCCESS ||
        !val_namespace_same_aperture(&before, &written) ||
        written.tx_completed == before.tx_completed ||
        written.rx_completed != before.rx_completed || written.failures != before.failures) {
        LOG_ERROR(NULL, "IDE namespace: stopping RX also blocked/unqualified TX write\n");
        return VAL_ERROR;
    }

    /* Step 2: Require the read request to transmit, but its RX response to be rejected. */
    status = val_probe_read64(before.address, &readback, &fault);
    exception_class = val_esr_ec(fault.esr);
    if (status == VAL_SUCCESS ||
        (exception_class != VAL_IDE_NAMESPACE_DATA_ABORT_CURRENT &&
         exception_class != VAL_IDE_NAMESPACE_DATA_ABORT_LOWER) ||
        (fault.esr & VAL_IDE_NAMESPACE_FAULT_STATUS_MASK) != VAL_IDE_NAMESPACE_EXTERNAL_ABORT ||
        pal_cxl_traffic_snapshot(&rejected) != VAL_SUCCESS ||
        !val_namespace_same_aperture(&before, &rejected) ||
        rejected.tx_completed == written.tx_completed ||
        rejected.rx_completed != written.rx_completed || rejected.failures == written.failures) {
        LOG_ERROR(NULL, "IDE namespace: RX STOP lacks isolated rejection evidence ESR=%llx\n",
                  (unsigned long long)fault.esr);
        return VAL_ERROR;
    }

    *probe = (val_firme_ide_namespace_probe_t){.lease = before.lease,
        .address = before.address, .pattern = pattern, .pe_index = val_pe_index(),
        .valid = VAL_TRUE};
    return VAL_SUCCESS;
}

/**
 * @brief Verify the TX-only write after GO restores the original RX keyset.
 * @param probe Witness produced while RX was stopped; consumed on success.
 * @return VAL_ERROR on ownership changes, corruption or missing cipher activity.
 */
int val_firme_ide_namespace_rx_restored(val_firme_ide_namespace_probe_t *probe)
{
    pal_cxl_traffic_snapshot_t before = {0};
    pal_cxl_traffic_snapshot_t after = {0};
    val_fault_info_t fault = {0};
    uint64_t readback = 0ULL;

    if (!probe || !probe->valid || probe->pe_index != val_pe_index() ||
        pal_cxl_traffic_snapshot(&before) != VAL_SUCCESS ||
        before.lease != probe->lease || before.address != probe->address)
        return VAL_ERROR;

    /* Step 3: Read without rewriting so a dropped TX-only write cannot pass. */
    if (val_probe_read64(probe->address, &readback, &fault) != VAL_SUCCESS ||
        readback != probe->pattern || pal_cxl_traffic_snapshot(&after) != VAL_SUCCESS ||
        !val_namespace_same_aperture(&before, &after) ||
        after.rx_completed == before.rx_completed || after.tx_completed == before.tx_completed ||
        after.failures != before.failures) {
        LOG_ERROR(NULL, "IDE namespace: RX restart did not preserve the independent TX write\n");
        return VAL_ERROR;
    }

    probe->valid = VAL_FALSE;
    return VAL_SUCCESS;
}
