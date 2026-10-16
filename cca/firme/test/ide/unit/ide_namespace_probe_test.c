/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>

#include "../../../val/src/val_firme_ide_namespace_probe.c"
#include "../../../platform/pal_baremetal/targets/fvp/src/pal_cxl.c"

#define TEST_EXTERNAL_ABORT (UINT64_C(0x25) << 26U | UINT64_C(0x10))
#define TEST_GPF (UINT64_C(0x25) << 26U | UINT64_C(0x28))
#define TEST_TRANSLATION_FAULT (UINT64_C(0x25) << 26U | UINT64_C(0x04))

typedef struct {
    uint32_t rx;
    uint32_t tx;
    uint32_t failures;
    uint32_t pe;
    uint32_t rx_stopped;
    uint32_t write_error;
    uint32_t suppress_write_count;
    uint32_t write_rx_count;
    uint32_t write_failure_count;
    uint32_t drop_write;
    uint32_t stopped_read_success;
    uint32_t suppress_request_count;
    uint32_t suppress_failure_count;
    uint32_t suppress_response_count;
    uint32_t restored_failure_count;
    uint32_t corrupt_read;
    uint32_t snapshot_count;
    uint32_t fail_snapshot;
    uint32_t change_lease_snapshot;
    uint64_t fault_esr;
    uint64_t memory;
} val_namespace_native_t;

static val_namespace_native_t g_native;

/** @brief Supply only the three actual paired-model cipher counters. */
int val_mmio_read32_checked(uint64_t address, uint32_t *value)
{
    assert(value);
    if (address == PAL_CXL_KM_BASE + 0x20U) {
        g_native.snapshot_count++;
        if (g_native.change_lease_snapshot == g_native.snapshot_count)
            g_pal_cxl.lease++;
    }

    if (g_native.fail_snapshot == g_native.snapshot_count)
        return VAL_ERROR;
    if (address == PAL_CXL_KM_BASE + 0x20U)
        *value = g_native.rx;
    else if (address == PAL_CXL_KM_BASE + 0x24U)
        *value = g_native.tx;
    else if (address == PAL_CXL_KM_BASE + 0x28U)
        *value = g_native.failures;
    else
        assert(VAL_FALSE);
    return VAL_SUCCESS;
}

/** @brief Model authenticated one-way writes, including deliberate broken paths. */
int val_probe_write64(uint64_t address, uint64_t value, val_fault_info_t *fault)
{
    assert(address == PAL_CXL_MEMORY && fault);
    if (g_native.write_error) {
        *fault = (val_fault_info_t){.esr = TEST_EXTERNAL_ABORT};
        return VAL_ERROR;
    }

    if (!g_native.suppress_write_count)
        g_native.tx++;
    g_native.rx += g_native.write_rx_count;
    g_native.failures += g_native.write_failure_count;
    if (!g_native.drop_write)
        g_native.memory = value;
    return VAL_SUCCESS;
}

/** @brief Keep read-request TX evidence separate from read-response RX evidence. */
int val_probe_read64(uint64_t address, uint64_t *value, val_fault_info_t *fault)
{
    assert(address == PAL_CXL_MEMORY && value && fault);
    if (!g_native.suppress_request_count)
        g_native.tx++;
    if (g_native.rx_stopped && !g_native.stopped_read_success) {
        if (!g_native.suppress_failure_count)
            g_native.failures++;
        *fault = (val_fault_info_t){.esr = g_native.fault_esr, .faulted = VAL_TRUE};
        return VAL_ERROR;
    }

    if (!g_native.suppress_response_count)
        g_native.rx++;
    g_native.failures += g_native.restored_failure_count;
    *value = g_native.memory ^ g_native.corrupt_read;
    return VAL_SUCCESS;
}

/** @brief Supply a caller identity for reservation-migration rejection. */
uint32_t val_pe_index(void)
{
    return g_native.pe;
}

/** @brief Suppress expected failure diagnostics in fault-injection regressions. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *format, ...)
{
    (void)verbosity;
    (void)tag;
    (void)format;
    return VAL_SUCCESS;
}

/** @brief Start each injection with a mapped, exclusively owned model aperture. */
static void val_namespace_native_reset(void)
{
    g_native = (val_namespace_native_t){.rx_stopped = VAL_TRUE,
        .fault_esr = TEST_EXTERNAL_ABORT, .pe = 3U};
    g_pal_cxl = (pal_cxl_fixture_t){.active = VAL_TRUE, .mapped = VAL_TRUE,
        .decoders_saved = VAL_TRUE, .lease = 12ULL};
}

/** @brief Prove that absent or stale ownership cannot produce counter evidence. */
static void val_namespace_native_ownership(void)
{
    pal_cxl_traffic_snapshot_t snapshot = {0};
    val_firme_ide_namespace_probe_t probe = {0};

    val_namespace_native_reset();
    assert(pal_cxl_traffic_snapshot(NULL) == VAL_ERROR);
    assert(val_firme_ide_namespace_rx_stopped(NULL) == VAL_ERROR);
    assert(val_firme_ide_namespace_rx_restored(NULL) == VAL_ERROR);
    assert(val_firme_ide_namespace_rx_restored(&probe) == VAL_ERROR);
    g_pal_cxl.active = VAL_FALSE;
    assert(pal_cxl_traffic_snapshot(&snapshot) == VAL_ERROR && !snapshot.lease);
    g_pal_cxl.active = VAL_TRUE;
    g_pal_cxl.mapped = VAL_FALSE;
    assert(val_firme_ide_namespace_rx_stopped(&probe) == VAL_ERROR);
    g_pal_cxl.mapped = VAL_TRUE;
    g_pal_cxl.decoders_saved = VAL_FALSE;
    assert(val_firme_ide_namespace_rx_stopped(&probe) == VAL_ERROR);
    g_pal_cxl.decoders_saved = VAL_TRUE;
    g_pal_cxl.lease = 0ULL;
    assert(val_firme_ide_namespace_rx_stopped(&probe) == VAL_ERROR);
}

/** @brief Reject success from missing cipher activity or an unrelated memory fault. */
static void val_namespace_native_stop_failures(void)
{
    val_firme_ide_namespace_probe_t probe = {0};
    uint32_t injection;

    for (injection = 0U; injection < 13U; injection++) {
        val_namespace_native_reset();
        probe = (val_firme_ide_namespace_probe_t){0};
        switch (injection) {
        case 0U:
            g_native.write_error = VAL_TRUE;
            break;
        case 1U:
            g_native.suppress_write_count = VAL_TRUE;
            break;
        case 2U:
            g_native.write_rx_count = 1U;
            break;
        case 3U:
            g_native.write_failure_count = 1U;
            break;
        case 4U:
            g_native.stopped_read_success = VAL_TRUE;
            break;
        case 5U:
            g_native.fault_esr = TEST_GPF;
            break;
        case 6U:
            g_native.fault_esr = TEST_TRANSLATION_FAULT;
            break;
        case 7U:
            g_native.suppress_failure_count = VAL_TRUE;
            break;
        case 8U:
            g_native.suppress_request_count = VAL_TRUE;
            break;
        case 9U:
            g_native.change_lease_snapshot = 2U;
            break;
        case 10U:
            g_native.change_lease_snapshot = 3U;
            break;
        case 11U:
            g_native.fail_snapshot = 1U;
            break;
        case 12U:
            g_native.fail_snapshot = 3U;
            break;
        default:
            assert(VAL_FALSE);
            break;
        }

        assert(val_firme_ide_namespace_rx_stopped(&probe) == VAL_ERROR && !probe.valid);
    }
}

/** @brief Require retained data, matching ownership and both-direction recovery. */
static void val_namespace_native_restore_failures(void)
{
    val_firme_ide_namespace_probe_t probe = {0};
    uint32_t injection;

    for (injection = 0U; injection < 9U; injection++) {
        val_namespace_native_reset();
        probe = (val_firme_ide_namespace_probe_t){0};
        if (injection == 0U)
            g_native.drop_write = VAL_TRUE;
        assert(val_firme_ide_namespace_rx_stopped(&probe) == VAL_SUCCESS);
        g_native.rx_stopped = VAL_FALSE;
        switch (injection) {
        case 0U:
            break;
        case 1U:
            g_native.pe++;
            break;
        case 2U:
            g_pal_cxl.lease++;
            break;
        case 3U:
            g_native.corrupt_read = VAL_TRUE;
            break;
        case 4U:
            g_native.suppress_request_count = VAL_TRUE;
            break;
        case 5U:
            g_native.suppress_response_count = VAL_TRUE;
            break;
        case 6U:
            g_native.restored_failure_count = 1U;
            break;
        case 7U:
            g_native.fail_snapshot = 5U;
            break;
        case 8U:
            g_native.change_lease_snapshot = 5U;
            break;
        default:
            assert(VAL_FALSE);
            break;
        }

        assert(val_firme_ide_namespace_rx_restored(&probe) == VAL_ERROR);
    }
}

/** @brief Exercise the real observer with counter wrap and every negative witness. */
int main(void)
{
    val_firme_ide_namespace_probe_t probe = {0};

    val_namespace_native_ownership();
    val_namespace_native_stop_failures();
    val_namespace_native_restore_failures();
    val_namespace_native_reset();
    g_native.tx = UINT32_MAX;
    g_native.failures = UINT32_MAX;
    assert(val_firme_ide_namespace_rx_stopped(&probe) == VAL_SUCCESS && probe.valid);
    assert(val_firme_ide_namespace_rx_stopped(&probe) == VAL_ERROR && probe.valid);
    g_native.rx_stopped = VAL_FALSE;
    assert(val_firme_ide_namespace_rx_restored(&probe) == VAL_SUCCESS && !probe.valid);
    assert(val_firme_ide_namespace_rx_restored(&probe) == VAL_ERROR);
    puts("IDE namespace: directional cipher, ownership and recovery witnesses PASS");
    return VAL_SUCCESS;
}
