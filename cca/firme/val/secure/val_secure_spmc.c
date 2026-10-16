/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_spmd_msg.h"
#include "val_common/val_gicv3.h"
#include "val_common/val_mmu.h"
#include "val_common/val_fault.h"
#include "val_common/val_smc_args.h"
#include "val_common/val_arch_init.h"
#include "val_common/val_mp.h"

#include "aarch64/sysreg.h"
#include "pal_platform.h"
#include "val_log.h"
#include "val_firme_diag.h"
#include "val_firme_gpt_probe.h"
#include "pal_firme_gpt_memory.h"
#include "val_firme_rules.h"
#include "val_firme_mp_test.h"
#include "val_firme_dispatch.h"
#include "val_firme_abi_frame.h"
#include "val_firme_test.h"
#include "val_platform.h"

#include "val_secure_entry.h"

typedef struct {
    uint64_t cmd;
    uint64_t a0;
    uint64_t a1;
    uint64_t a2;
    uint64_t a3;
    uint64_t status;
    uint64_t out0;
    uint64_t out1;
    uint64_t out2;
    uint64_t out3;
    uint32_t active;
    uint32_t reserved;
} spmc_el1_ctx_t;

uint64_t g_el1_saved_regs[12];
uint64_t g_el1_ctx_ptr;
static spmc_el1_ctx_t g_el1_ctx;
static _Alignas(16) uint8_t g_el1_stack[4096];

/*
 * Match TF-A SPMD assumptions:
 * - NS physical endpoint ID is 0.
 * - SPMC endpoint ID comes from the SPMC manifest (FVP uses 0x8000).
 */
static const uint16_t k_ns_endpoint_id = (uint16_t)PLAT_FFA_NS_ENDPOINT_ID;
static const uint16_t k_spmc_endpoint_id = (uint16_t)PLAT_FFA_SPMC_ENDPOINT_ID;
static const uint16_t k_acs_endpoint_id = (uint16_t)PLAT_FFA_ACS_SECURE_ENDPOINT_ID;

static uint32_t g_spmc_inited[PAL_PE_COUNT];
/* Non-BSS cold-boot flag: used to safely clear BSS exactly once. */
uint32_t g_spmc_cold_boot = 1;

static uint32_t g_direct_req_log_budget = 8;
static uint32_t g_probe_fault_log_budget = 8;

extern uint8_t __text_start__[];
extern uint8_t __stack_end__[];

static void dispatch_cmd(uint64_t cmd, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3,
                         uint64_t *status, uint64_t *out0, uint64_t *out1, uint64_t *out2,
                         uint64_t *out3);

/**
 *   @brief    - Ensure the Secure payload logger is initialized.
 *   @return   - void
**/
static void spmc_ensure_log_init(void)
{
    if (val_log_get_uart_base() == 0U)
        val_log_init(val_get_ns_uart_base());
}

const char *val_world_tag(void)
{
    const unsigned int el = (unsigned int)((read_currentel() >> 2) & 0x3);

    return (el == 1U) ? "sel1" : "sel2";
}

/**
 *   @brief    - Return the current exception level as a numeric value.
 *   @return   - Current EL (1 or 2).
**/
static unsigned int current_el_num(void)
{
    return (unsigned int)((read_currentel() >> 2) & 0x3);
}

/**
 *   @brief    - One-time initialization for the Secure payload.
 *   @return   - void
**/
static void spmc_init_once(void)
{
    unsigned int el = 0;
    uint32_t pe_index = val_pe_index();

    if (g_spmc_inited[pe_index] != 0U)
        return;
    g_spmc_inited[pe_index] = 1U;

    spmc_ensure_log_init();
    LOG_DEBUG(NULL, "init el=%u", current_el_num());
    LOG_DEBUG(NULL, "endpoints: spmc=0x%x acs=0x%x", (unsigned int)k_spmc_endpoint_id,
              (unsigned int)k_acs_endpoint_id);

    el = current_el_num();
    val_fault_reset();
    if (el == 2) {
        write_vbar_el2((uint64_t)(uintptr_t)&secure_vectors_el2);
        val_arch_init_basic(MMU_EL2);
    } else {
        write_vbar_el1((uint64_t)(uintptr_t)&secure_vectors_el1);
        val_arch_init_basic(MMU_EL1);
    }

    /*
     * Note: TF-A FVP loads BL32 at 0x0600_0000 (trusted SRAM), which is below
     * PLAT_DRAM_BASE. Our simple `val_mmu_setup_identity_1g()` treats low 4GiB as
     * Device memory (often execute-never), so enabling the MMU from BL32 can
     * fault immediately.
     *
     * Keep MMU disabled for the Secure shim until we introduce a finer-grain
     * mapping for the BL32 region.
     */
    LOG_DEBUG(NULL, "mmu disabled");

    val_gicv3_cpuif_enable();
    enable_irq();
}

/**
 *   @brief    - S-EL1 command body: run a command then trap back to S-EL2 via HVC.
 *   @param    - ctx_ptr : Pointer to shared S-EL1 context structure.
 *   @return   - void (does not return).
**/
void secure_el1_main(void *ctx_ptr)
{
    spmc_el1_ctx_t *ctx = (spmc_el1_ctx_t *)ctx_ptr;
    uint64_t status = 0, out0 = 0, out1 = 0, out2 = 0, out3 = 0;

    spmc_ensure_log_init();
    dispatch_cmd(ctx->cmd, ctx->a0, ctx->a1, ctx->a2, ctx->a3,
                 &status, &out0, &out1, &out2, &out3);

    ctx->status = status;
    ctx->out0 = out0;
    ctx->out1 = out1;
    ctx->out2 = out2;
    ctx->out3 = out3;

    dsb_sy();
    isb_sy();
    val_hvc0();

    for (;;)
        val_wfe();
}

/**
 *   @brief    - Handle lower EL synchronous traps taken to S-EL2.
 *   @param    - esr : ESR_EL2 value.
 *   @param    - elr : ELR_EL2 value.
 *   @param    - far : FAR_EL2 value.
 *   @return   - void
**/
void secure_lower_sync_handler(uint64_t esr, uint64_t elr, uint64_t far)
{
    const uint32_t ec = (uint32_t)((esr >> 26) & 0x3fu);
    const uint32_t k_ec_hvc = 0x16u;

    spmc_ensure_log_init();

    if (ec == k_ec_hvc && g_el1_ctx.active != 0u) {
        g_el1_ctx.active = 0u;
        write_elr_el2((uint64_t)(uintptr_t)&secure_enter_el1_return);
        write_spsr_el2(0x3c9u); /* D,A,I,F masked; EL2h */
        return;
    }

    LOG_ERROR(NULL, "lower sync ec=0x%x esr=%llx elr=%llx far=%llx",
              (unsigned)ec,
              (unsigned long long)esr, (unsigned long long)elr,
              (unsigned long long)far);

    if (g_el1_ctx.active != 0u) {
        g_el1_ctx.status = (uint64_t)ACS_STATUS_ERR;
        g_el1_ctx.active = 0u;
        write_elr_el2((uint64_t)(uintptr_t)&secure_enter_el1_return);
        write_spsr_el2(0x3c9u);
        return;
    }

    for (;;) {
        val_wfe();
    }
}

static int run_cmd_in_el1(uint64_t cmd, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3,
                          uint64_t *status, uint64_t *out0, uint64_t *out1, uint64_t *out2,
                          uint64_t *out3)
{
    uint64_t sp_el1_top = 0;

    if (current_el_num() != 2u)
        return VAL_ERROR;

    g_el1_ctx.cmd = cmd;
    g_el1_ctx.a0 = a0;
    g_el1_ctx.a1 = a1;
    g_el1_ctx.a2 = a2;
    g_el1_ctx.a3 = a3;
    g_el1_ctx.status = (uint64_t)ACS_STATUS_ERR;
    g_el1_ctx.out0 = 0;
    g_el1_ctx.out1 = 0;
    g_el1_ctx.out2 = 0;
    g_el1_ctx.out3 = 0;
    g_el1_ctx.active = 1u;
    g_el1_ctx_ptr = (uint64_t)(uintptr_t)&g_el1_ctx;

    sp_el1_top = (uint64_t)(uintptr_t)(g_el1_stack + sizeof(g_el1_stack));
    secure_enter_el1((void *)&g_el1_ctx, sp_el1_top);

    if (g_el1_ctx.active != 0u) {
        g_el1_ctx.active = 0u;
        return VAL_ERROR;
    }

    if (status)
        *status = g_el1_ctx.status;
    if (out0)
        *out0 = g_el1_ctx.out0;
    if (out1)
        *out1 = g_el1_ctx.out1;
    if (out2)
        *out2 = g_el1_ctx.out2;
    if (out3)
        *out3 = g_el1_ctx.out3;
    return VAL_SUCCESS;
}

/**
 *   @brief    - Secure world synchronous exception handler.
 *   @param    - esr : ESR_ELx for the exception.
 *   @param    - elr : ELR_ELx for the exception.
 *   @param    - far : FAR_ELx for the exception (if applicable).
 *   @return   - void
**/
void secure_sync_handler(uint64_t esr, uint64_t elr, uint64_t far)
{
    spmc_ensure_log_init();
    (void)elr;
    if (val_fault_handle_sync(esr, far) != 0) {
        if (val_mmu_ns_probe_active())
            return;
        if (g_probe_fault_log_budget != 0u) {
            g_probe_fault_log_budget--;
            LOG_DEBUG(NULL, "probe fault handled esr=%llx far=%llx",
                      (unsigned long long)esr, (unsigned long long)far);
        }
        return;
    }
    LOG_ERROR(NULL, "sync esr=%llx elr=%llx far=%llx",
              (unsigned long long)esr, (unsigned long long)elr,
              (unsigned long long)far);
    for (;;) {
        val_wfe();
    }
}

/**
 *   @brief    - Secure world IRQ handler.
 *   @return   - void
**/
void secure_irq_handler(void)
{
    uint32_t intid = 0;

    spmc_ensure_log_init();
    intid = val_gicv3_acknowledge_irq();
    LOG_DEBUG(NULL, "irq intid=%u", (unsigned)intid);
    val_gicv3_end_of_irq(intid);
}

/**
 *   @brief    - Populate a direct-messaging error return in an SMC argument frame.
 *   @param    - args : In/out SMC argument frame.
 *   @param    - err  : Error code (placed in x2).
 *   @return   - void
**/
static void ffa_error(smc_args_t *args, ffa_error_t err)
{
    args->x[0] = (uint64_t)(uint32_t)FFA_ERROR;
    args->x[1] = 0;
    args->x[2] = (uint64_t)(int64_t)err;
}

/**
 *   @brief    - Populate a SUCCESS (SMC32) return in an SMC argument frame.
 *   @param    - args : In/out SMC argument frame.
 *   @param    - arg2 : Value returned in x2 (e.g. endpoint ID).
 *   @return   - void
**/
static void ffa_success32(smc_args_t *args, uint64_t arg2)
{
    args->x[0] = (uint64_t)(uint32_t)FFA_SUCCESS_SMC32;
    args->x[1] = 0;
    args->x[2] = arg2;
}

/**
 *   @brief    - Populate a direct response (SMC32) in an SMC argument frame.
 *   @param    - args    : In/out SMC argument frame.
 *   @param    - src     : Response source endpoint ID.
 *   @param    - dst     : Response destination endpoint ID.
 *   @param    - x2_flags: Flags field (x2).
 *   @param    - x3      : Response payload word 0.
 *   @param    - x4      : Response payload word 1.
 *   @param    - x5      : Response payload word 2.
 *   @param    - x6      : Response payload word 3.
 *   @param    - x7      : Response payload word 4.
 *   @return   - void
**/
static void ffa_direct_resp32(smc_args_t *args, uint16_t src, uint16_t dst,
                              uint32_t x2_flags,
                              uint64_t x3, uint64_t x4, uint64_t x5, uint64_t x6, uint64_t x7)
{
    args->x[0] = (uint64_t)(uint32_t)FFA_MSG_SEND_DIRECT_RESP_SMC32;
    args->x[1] = ((uint32_t)src << 16) | (uint32_t)dst;
    args->x[2] = (uint64_t)x2_flags;
    args->x[3] = x3;
    args->x[4] = x4;
    args->x[5] = x5;
    args->x[6] = x6;
    args->x[7] = x7;
}

/**
 * @brief Read a leased GPT granule using Secure state and an NS output descriptor.
 * @return PASS for access/fault evidence, ERR for unsafe addresses or setup errors.
 */
static uint64_t val_secure_gpt_ns_probe(uint64_t address, firme_diag_out_t *out)
{
    val_fault_info_t fault = {0};
    uint64_t value = 0ULL;

    if (!out || !pal_firme_gpt_memory_probe_allowed(address))
        return ACS_STATUS_ERR;

    if (val_mmu_probe_ns_read64_el2(address,
        (uint64_t)(uintptr_t)__text_start__, (uint64_t)(uintptr_t)__stack_end__,
        &value, &fault) != VAL_SUCCESS)
        return ACS_STATUS_ERR;

    out->x0 = fault.faulted;
    out->x1 = fault.faulted ? fault.esr : value;
    out->x2 = fault.far;
    out->x3 = fault.faulted ? 0ULL : value;
    return ACS_STATUS_PASS;
}

/**
 *   @brief    - Dispatch a FIRME command and return status/output words.
 *   @param    - cmd    : FIRME ACS command selector.
 *   @param    - a0     : Command argument 0.
 *   @param    - a1     : Command argument 1.
 *   @param    - a2     : Command argument 2.
 *   @param    - a3     : Command argument 3.
 *   @param    - status : Optional status output.
 *   @param    - out0   : Optional output word 0.
 *   @param    - out1   : Optional output word 1.
 *   @param    - out2   : Optional output word 2.
 *   @param    - out3   : Optional output word 3.
 *   @return   - void
**/
static void dispatch_cmd(uint64_t cmd, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3,
                         uint64_t *status, uint64_t *out0, uint64_t *out1, uint64_t *out2,
                         uint64_t *out3)
{
    uint64_t st = (uint64_t)ACS_STATUS_PASS;
    uint64_t o0 = 0, o1 = 0, o2 = 0, o3 = 0;
    firme_acs_call_res_t mp_response = {0};

    if (cmd == (uint64_t)FIRME_ACS_CMD_PING) {
        o0 = 0x504f4e47ULL; /* 'PONG' */
    } else if (cmd == FIRME_ACS_CMD_GPT_PROBE) {
        val_firme_gpt_probe_command(a0, &mp_response);
        st = mp_response.x0;
        o0 = mp_response.x1;
        o1 = mp_response.x2;
        o2 = mp_response.x3;
    } else if (cmd == (uint64_t)FIRME_ACS_CMD_RUN_DIAG_SEL1) {
        run_cmd_in_el1((uint64_t)FIRME_ACS_CMD_RUN_DIAG,
                       a0, a1, a2, a3,
                       &st, &o0, &o1, &o2, &o3);
    } else if (cmd == (uint64_t)FIRME_ACS_CMD_RUN_RULE_SEL1) {
        run_cmd_in_el1((uint64_t)FIRME_ACS_CMD_RUN_RULE,
                       a0, a1, a2, a3,
                       &st, &o0, &o1, &o2, &o3);
    } else if (cmd == (uint64_t)FIRME_ACS_CMD_RUN_DIAG) {
        firme_diag_out_t out = { 0 };
        if (a0 == FIRME_DIAG_READ_NS_PAS)
            st = val_secure_gpt_ns_probe(a1, &out);
        else
            st = (uint64_t)val_firme_diag_run(a0, a1, a2, a3, 0, &out);
        o0 = out.x0;
        o1 = out.x1;
        o2 = out.x2;
        o3 = out.x3;
    } else if (cmd == FIRME_ACS_CMD_ABI_FRAME) {
        val_firme_abi_frame_command(FIRME_ACS_WORLD_SECURE, a0, a1, a2, a3, &mp_response);
        st = mp_response.x0;
        o0 = mp_response.x1;
        o1 = mp_response.x2;
    } else if ((cmd == FIRME_ACS_CMD_RUN_TEST_PAYLOAD) ||
               (cmd == FIRME_ACS_CMD_GET_TEST_RESULT)) {
        val_firme_test_payload_command(FIRME_ACS_WORLD_SECURE, (firme_acs_cmd_t)cmd,
                                        a0, a1, a2, a3, &mp_response);
        st = mp_response.x0;
        o0 = mp_response.x1;
        o1 = mp_response.x2;
    } else if (cmd == (uint64_t)FIRME_ACS_CMD_RUN_RULE) {
        firme_rule_res_t rr = { 0 };
        val_firme_rule_run_secure((firme_rule_id_t)a0, a1, &rr);
        st = rr.status;
        o0 = rr.fail_step;
        o1 = rr.last_x0;
        o2 = rr.last_x1;
        o3 = 0;
    } else {
        st = (uint64_t)ACS_STATUS_ERR;
    }

    if (status) {
        *status = st;
    }
    if (out0) {
        *out0 = o0;
    }
    if (out1) {
        *out1 = o1;
    }
    if (out2) {
        *out2 = o2;
    }
    if (out3) {
        *out3 = o3;
    }
}

/**
 *   @brief    - Handle a direct request using the SMC64 calling convention.
 *   @param    - args : In/out SMC argument frame.
 *   @return   - void
**/
static void handle_direct_req64(smc_args_t *args)
{
    uint32_t src_dst = (uint32_t)args->x[1];
    uint16_t src = (uint16_t)(src_dst >> 16);
    uint16_t dst = (uint16_t)(src_dst & 0xffffu);
    uint64_t cmd = args->x[3];
    uint64_t status = 0, out0 = 0, out1 = 0, out2 = 0, out3 = 0;

    if (g_direct_req_log_budget != 0u) {
        g_direct_req_log_budget--;
        LOG_DEBUG(NULL, "direct_req64 src=0x%x dst=0x%x x2=0x%llx cmd=0x%llx",
                  (unsigned)src, (unsigned)dst,
                  (unsigned long long)args->x[2], (unsigned long long)cmd);
    }

    /*
     * Direct requests from Normal world are addressed to a partition endpoint.
     * This experiment hosts a single "ACS endpoint" inside the SPMC shim.
     */
    if (dst != k_acs_endpoint_id) {
        ffa_error(args, FFA_ERROR_INVALID_PARAMETER);
        return;
    }

    dispatch_cmd(cmd, args->x[4], args->x[5], args->x[6], args->x[7],
                 &status, &out0, &out1, &out2, &out3);

    args->x[0] = (uint64_t)(uint32_t)FFA_MSG_SEND_DIRECT_RESP_SMC64;
    args->x[1] = ((uint32_t)k_acs_endpoint_id << 16) | (uint32_t)src;
    args->x[2] = 0;
    args->x[3] = status;
    args->x[4] = out0;
    args->x[5] = out1;
    args->x[6] = out2;
    args->x[7] = out3;
}

/**
 *   @brief    - Handle a direct request using the SMC32 calling convention.
 *   @param    - args : In/out SMC argument frame.
 *   @return   - void
**/
static void handle_direct_req32(smc_args_t *args)
{
    uint32_t src_dst = (uint32_t)args->x[1];
    uint16_t src = (uint16_t)(src_dst >> 16);
    uint16_t dst = (uint16_t)(src_dst & 0xffffu);
    uint32_t x2 = (uint32_t)args->x[2];
    uint64_t cmd = 0;
    uint64_t status = 0, out0 = 0, out1 = 0, out2 = 0, out3 = 0;

    if (g_direct_req_log_budget != 0u) {
        g_direct_req_log_budget--;
        LOG_DEBUG(NULL, "direct_req32 src=0x%x dst=0x%x x2=0x%x cmd=0x%llx",
                  (unsigned)src, (unsigned)dst, (unsigned)x2,
                  (unsigned long long)args->x[3]);
    }

    /*
     * Handle SPMD framework messages (notably FFA_VERSION negotiation).
     * SPMD encodes: x2 = BIT31 | msg_id, x3 = payload.
     */
    if ((x2 & FFA_FWK_MSG_BIT) != 0U) {
        uint32_t msg_id = x2 & FFA_FWK_MSG_ID_MASK;

        if ((msg_id == SPMD_FWK_MSG_PSCI) &&
            ((uint32_t)args->x[3] == SPMD_PSCI_CPU_OFF)) {
            /* Local registers must be initialized again on the next CPU_ON. */
            g_spmc_inited[val_pe_index()] = 0U;
            ffa_direct_resp32(args, k_spmc_endpoint_id, src,
                              FFA_FWK_MSG_BIT | SPMD_FWK_MSG_PM_RESP,
                              VAL_SUCCESS, 0U, 0U, 0U, 0U);
            return;
        }

        if (msg_id == SPMD_FWK_MSG_FFA_VERSION_REQ) {
            /* Respond with SPMC supported version (v1.1). */
            ffa_direct_resp32(args, k_spmc_endpoint_id, src,
                              FFA_FWK_MSG_BIT | SPMD_FWK_MSG_FFA_VERSION_RESP,
                              FFA_MAKE_VERSION(1, 1), 0, 0, 0, 0);
            return;
        }

        /* Unknown framework message. */
        ffa_error(args, FFA_ERROR_NOT_SUPPORTED);
        return;
    }

    if (dst != k_acs_endpoint_id) {
        ffa_error(args, FFA_ERROR_INVALID_PARAMETER);
        return;
    }

    cmd = args->x[3];
    dispatch_cmd(cmd, args->x[4], args->x[5], args->x[6], args->x[7],
                 &status, &out0, &out1, &out2, &out3);

    ffa_direct_resp32(args, k_acs_endpoint_id, src, /* flags */ 0,
                      status, out0, out1, out2, out3);
}

/**
 *   @brief    - Secure payload SMC entrypoint called by TF-A SPMD.
 *   @param    - args : In/out SMC argument frame (x0..x7).
 *   @return   - void
**/
void spmc_handle_smc(smc_args_t *args)
{
    uint32_t fid = 0;

    spmc_init_once();

    fid = (uint32_t)args->x[0];
    LOG_DEBUG(NULL, "smc fid=%x", (unsigned)fid);

    /*
     * Initial synchronous entry from SPMD (SPMC init) passes a manifest pointer
     * in x0, not a direct-messaging FID. Return FFA_MSG_WAIT so SPMD marks init complete.
     */
    if ((fid & 0xFF000000U) != 0x84000000U && (fid & 0xFF000000U) != 0xC4000000U) {
        args->x[0] = (uint64_t)(uint32_t)FFA_MSG_WAIT;
        args->x[1] = 0;
        args->x[2] = 0;
        args->x[3] = 0;
        args->x[4] = 0;
        args->x[5] = 0;
        args->x[6] = 0;
        args->x[7] = 0;
        return;
    }

    switch (fid) {
    case FFA_VERSION:
        /* Return supported version (v1.1). */
        args->x[0] = (uint64_t)FFA_MAKE_VERSION(1, 1);
        args->x[1] = 0;
        args->x[2] = 0;
        args->x[3] = 0;
        args->x[4] = 0;
        args->x[5] = 0;
        args->x[6] = 0;
        args->x[7] = 0;
        return;
    case FFA_FEATURES:
        /*
         * Minimal: advertise direct messaging support.
         * Input: x1 = feature ID.
         */
        if ((uint32_t)args->x[1] == (uint32_t)FFA_MSG_SEND_DIRECT_REQ_SMC64 ||
            (uint32_t)args->x[1] == (uint32_t)FFA_MSG_SEND_DIRECT_REQ_SMC32) {
            ffa_success32(args, 0);
        } else {
            ffa_error(args, FFA_ERROR_NOT_SUPPORTED);
        }
        return;
    case FFA_ID_GET:
        /* Return caller's endpoint id (NS physical endpoint for this experiment). */
        ffa_success32(args, (uint64_t)k_ns_endpoint_id);
        return;
    case FFA_SPM_ID_GET:
        /* Return SPMC endpoint id. */
        ffa_success32(args, (uint64_t)k_spmc_endpoint_id);
        return;
    case FFA_MSG_SEND_DIRECT_REQ_SMC32:
        handle_direct_req32(args);
        return;
    case FFA_MSG_SEND_DIRECT_REQ_SMC64:
        handle_direct_req64(args);
        return;
    default:
        /* Allow boot-time entry with unknown x0 by returning NOT_SUPPORTED. */
        ffa_error(args, FFA_ERROR_NOT_SUPPORTED);
        return;
    }
}
