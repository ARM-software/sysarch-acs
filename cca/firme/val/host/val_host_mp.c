/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_mp.h"
#include "aarch64/sysreg.h"
#include "val_common/val_arch_init.h"
#include "val_common/val_fault.h"
#include "val_common/val_mmu.h"
#include "val_common/val_smccc.h"
#include "val_common/val_status.h"
#include "val_log.h"

#define VAL_PSCI_CPU_ON UINT64_C(0xc4000003)
#define VAL_PSCI_CPU_OFF UINT64_C(0x84000002)
#define VAL_PSCI_AFFINITY_INFO UINT64_C(0xc4000004)
#define VAL_PSCI_SUCCESS INT64_C(0)
#define VAL_PSCI_AFFINITY_OFF INT64_C(1)
#define VAL_MP_TIMEOUT_SECONDS UINT64_C(5)
#define VAL_MP_CACHE_ALIGNMENT 256U

typedef enum {
    VAL_MP_OFF,
    VAL_MP_BOOTING,
    VAL_MP_IDLE,
    VAL_MP_RUN,
    VAL_MP_DONE,
    VAL_MP_STOP,
    VAL_MP_ERROR,
} val_mp_state_t;

typedef struct __attribute__((aligned(VAL_MP_CACHE_ALIGNMENT))) {
    uint32_t state;
    uint32_t quarantined;
    val_host_mp_callback_t callback;
    void *context;
    int result;
} val_mp_mailbox_t;

static val_mp_mailbox_t g_mp_mailboxes[PAL_PE_COUNT];
static uint32_t g_mp_primary = PAL_PE_COUNT;
static uint64_t g_mp_timeout_ticks;

extern void val_host_secondary_entry(void);
extern void nsel2_vectors(void);
extern char __text_start__[], __text_end__[];
extern char __stack_start__[], __stack_end__[];

/** @brief Return the controller PE, or an invalid index before initialization. */
uint32_t val_host_mp_primary_index(void)
{
    return (g_mp_timeout_ticks != 0U) ? g_mp_primary : PAL_PE_COUNT;
}

/** @brief Issue a PSCI request; keep firmware status distinct from ACS status. */
static int64_t val_host_psci(uint64_t fid, uint64_t arg0, uint64_t arg1, uint64_t arg2)
{
    smccc_res_t result = {0};

    val_smccc_smc_call(fid, arg0, arg1, arg2, 0U, 0U, 0U, 0U, &result);
    return (int32_t)result.x0;
}

/** @brief Validate a worker handle and enforce the single-controller contract. */
static int val_host_mp_valid(uint32_t pe_index)
{
    return (pe_index < PAL_PE_COUNT) && (pe_index != g_mp_primary) &&
           (val_pe_index() == g_mp_primary) && (g_mp_timeout_ticks != 0U) &&
           (g_mp_mailboxes[pe_index].quarantined == 0U);
}

/** @brief Wait with an architected-counter deadline, quarantining late workers. */
static int val_host_mp_wait_state(uint32_t pe_index, val_mp_state_t expected)
{
    uint64_t start = val_read_counter();
    uint32_t state = VAL_MP_OFF;

    for (;;) {
        state = __atomic_load_n(&g_mp_mailboxes[pe_index].state, __ATOMIC_ACQUIRE);
        if (state == (uint32_t)expected)
            return VAL_SUCCESS;

        if ((state == VAL_MP_ERROR) ||
            ((val_read_counter() - start) >= g_mp_timeout_ticks)) {
            g_mp_mailboxes[pe_index].quarantined = 1U;
            LOG_ERROR(NULL, "MP PE=%u state=%u expected=%u; quarantined",
                      pe_index, state, (uint32_t)expected);
            return VAL_ERROR;
        }

        val_mp_relax();
    }
}

/** @brief Initialize the controller; call once before starting any worker. */
void val_host_mp_init(void)
{
    uint64_t frequency = val_read_counter_frequency();
    uint32_t pe_index = 0U;
    uint32_t other_index = 0U;

    if (g_mp_primary < PAL_PE_COUNT)
        return;

    g_mp_primary = val_pe_index();
    if ((g_mp_primary >= PAL_PE_COUNT) || (frequency == 0U) ||
        (frequency > (UINT64_MAX / VAL_MP_TIMEOUT_SECONDS)) ||
        ((UINT64_C(4) << ((read_ctr_el0() >> 16) & UINT64_C(15))) >
         VAL_MP_CACHE_ALIGNMENT))
        return;

    for (pe_index = 0U; pe_index < PAL_PE_COUNT; pe_index++) {
        if ((g_pal_pe_mpidrs[pe_index] & ~(uint64_t)PAL_PE_AFFINITY_MASK) != 0U)
            return;

        for (other_index = 0U; other_index < pe_index; other_index++) {
            if (g_pal_pe_mpidrs[pe_index] == g_pal_pe_mpidrs[other_index])
                return;
        }
    }

    /* Publish text, boot stacks and tables before any cache-off CPU_ON entry. */
    val_cache_clean_poc((uintptr_t)__text_start__,
                         (uintptr_t)__text_end__ - (uintptr_t)__text_start__);
    val_cache_clean_poc((uintptr_t)__stack_start__,
                         (uintptr_t)__stack_end__ - (uintptr_t)__stack_start__);
    val_mmu_publish_tables();
    g_mp_timeout_ticks = frequency * VAL_MP_TIMEOUT_SECONDS;
}

/** @brief Boot an offline worker; CPU_ON success is not a readiness indication. */
int val_host_mp_start(uint32_t pe_index)
{
    int64_t psci_status = VAL_PSCI_SUCCESS;
    val_mp_mailbox_t *mailbox = NULL;

    if (!val_host_mp_valid(pe_index))
        return VAL_ERROR;

    mailbox = &g_mp_mailboxes[pe_index];
    if (__atomic_load_n(&mailbox->state, __ATOMIC_ACQUIRE) != VAL_MP_OFF)
        return VAL_ERROR;

    psci_status = val_host_psci(VAL_PSCI_AFFINITY_INFO,
                               g_pal_pe_mpidrs[pe_index], 0U, 0U);
    if (psci_status != VAL_PSCI_AFFINITY_OFF) {
        LOG_ERROR(NULL, "MP PE=%u not offline, PSCI=%lld", pe_index,
                  (long long)psci_status);
        return VAL_ERROR;
    }

    mailbox->callback = NULL;
    mailbox->context = NULL;
    mailbox->result = VAL_ERROR;
    __atomic_store_n(&mailbox->state, VAL_MP_BOOTING, __ATOMIC_RELEASE);
    val_cache_clean_poc((uintptr_t)mailbox, sizeof(*mailbox));
    psci_status = val_host_psci(VAL_PSCI_CPU_ON, g_pal_pe_mpidrs[pe_index],
                               (uintptr_t)&val_host_secondary_entry, pe_index);
    if (psci_status != VAL_PSCI_SUCCESS) {
        mailbox->quarantined = 1U;
        LOG_ERROR(NULL, "MP CPU_ON PE=%u failed, PSCI=%lld", pe_index,
                  (long long)psci_status);
        return VAL_ERROR;
    }

    return val_host_mp_wait_state(pe_index, VAL_MP_IDLE);
}

/** @brief Publish a job; its context must remain alive until successful wait. */
int val_host_mp_dispatch(uint32_t pe_index, val_host_mp_callback_t callback,
                         void *context)
{
    val_mp_mailbox_t *mailbox = NULL;

    if (!val_host_mp_valid(pe_index) || (callback == NULL))
        return VAL_ERROR;

    mailbox = &g_mp_mailboxes[pe_index];
    if (__atomic_load_n(&mailbox->state, __ATOMIC_ACQUIRE) != VAL_MP_IDLE)
        return VAL_ERROR;

    mailbox->callback = callback;
    mailbox->context = context;
    mailbox->result = VAL_ERROR;
    __atomic_store_n(&mailbox->state, VAL_MP_RUN, __ATOMIC_RELEASE);
    return VAL_SUCCESS;
}

/** @brief Acquire a completed result, returning VAL_ERROR for a failed callback. */
int val_host_mp_wait(uint32_t pe_index, int *callback_result)
{
    uint32_t state = VAL_MP_OFF;

    if (callback_result != NULL)
        *callback_result = VAL_ERROR;

    if (!val_host_mp_valid(pe_index) || (callback_result == NULL))
        return VAL_ERROR;

    state = __atomic_load_n(&g_mp_mailboxes[pe_index].state, __ATOMIC_ACQUIRE);
    if ((state != VAL_MP_RUN) && (state != VAL_MP_DONE))
        return VAL_ERROR;

    if (val_host_mp_wait_state(pe_index, VAL_MP_DONE) != VAL_SUCCESS)
        return VAL_ERROR;

    *callback_result = g_mp_mailboxes[pe_index].result;
    __atomic_store_n(&g_mp_mailboxes[pe_index].state, VAL_MP_IDLE, __ATOMIC_RELEASE);
    return (*callback_result == VAL_SUCCESS) ? VAL_SUCCESS : VAL_ERROR;
}

/** @brief Request CPU_OFF and verify firmware reports OFF before reusing a PE. */
int val_host_mp_stop(uint32_t pe_index)
{
    uint64_t start = val_read_counter();
    int64_t psci_status = VAL_PSCI_SUCCESS;

    if (!val_host_mp_valid(pe_index) ||
        (__atomic_load_n(&g_mp_mailboxes[pe_index].state, __ATOMIC_ACQUIRE) !=
         VAL_MP_IDLE))
        return VAL_ERROR;

    __atomic_store_n(&g_mp_mailboxes[pe_index].state, VAL_MP_STOP, __ATOMIC_RELEASE);
    for (;;) {
        psci_status = val_host_psci(VAL_PSCI_AFFINITY_INFO,
                                   g_pal_pe_mpidrs[pe_index], 0U, 0U);
        if (psci_status == VAL_PSCI_AFFINITY_OFF) {
            __atomic_store_n(&g_mp_mailboxes[pe_index].state, VAL_MP_OFF,
                             __ATOMIC_RELEASE);
            return VAL_SUCCESS;
        }

        if ((psci_status < 0) || ((val_read_counter() - start) >= g_mp_timeout_ticks)) {
            g_mp_mailboxes[pe_index].quarantined = 1U;
            LOG_ERROR(NULL, "MP CPU_OFF PE=%u unconfirmed, PSCI=%lld", pe_index,
                      (long long)psci_status);
            return VAL_ERROR;
        }

        val_mp_relax();
    }
}

/** @brief Join coherent execution and service jobs on a private secondary stack. */
void val_host_secondary_main(uint64_t context_id)
{
    uint32_t pe_index = val_pe_index();
    uint32_t state = VAL_MP_OFF;
    val_mp_mailbox_t *mailbox = NULL;

    write_vbar_el2((uintptr_t)&nsel2_vectors);
    val_arch_init_basic(MMU_EL2);
    val_fault_reset();
    if ((pe_index >= PAL_PE_COUNT) || (context_id != pe_index)) {
        for (;;)
            val_mp_relax();
    }

    mailbox = &g_mp_mailboxes[pe_index];
    if ((__atomic_load_n(&mailbox->state, __ATOMIC_ACQUIRE) != VAL_MP_BOOTING) ||
        ((UINT64_C(4) << ((read_ctr_el0() >> 16) & UINT64_C(15))) >
         VAL_MP_CACHE_ALIGNMENT)) {
        __atomic_store_n(&mailbox->state, VAL_MP_ERROR, __ATOMIC_RELEASE);
        for (;;)
            val_mp_relax();
    }

    __atomic_store_n(&mailbox->state, VAL_MP_IDLE, __ATOMIC_RELEASE);
    for (;;) {
        state = __atomic_load_n(&mailbox->state, __ATOMIC_ACQUIRE);
        if (state == VAL_MP_RUN) {
            mailbox->result = mailbox->callback(pe_index, mailbox->context);
            __atomic_store_n(&mailbox->state, VAL_MP_DONE, __ATOMIC_RELEASE);
        } else if (state == VAL_MP_STOP) {
            /* PSCI owns final cache teardown; publish our shared state first. */
            val_cache_clean_poc((uintptr_t)mailbox, sizeof(*mailbox));
            val_host_psci(VAL_PSCI_CPU_OFF, 0U, 0U, 0U);
            __atomic_store_n(&mailbox->state, VAL_MP_ERROR, __ATOMIC_RELEASE);
            for (;;)
                val_mp_relax();
        }

        val_mp_relax();
    }
}
