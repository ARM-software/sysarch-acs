/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

    .section .text, "ax"

    /* Minimal AArch64 system register accessors used by FIRME-ACS payloads. */

/*
 * Toolchain portability:
 * Some AArch64 assemblers do not accept newer symbolic sysreg names,
 * so use explicit encodings via macros.
 *
 * ID_AA64PFR1_EL1  == S3_0_C0_C4_1.
 * ID_AA64MMFR4_EL1 == S3_0_C0_C7_4.
 * MECIDR_EL2       == S3_4_C10_C8_7.
 */
#define VAL_SYSREG_ID_AA64PFR1_EL1  S3_0_C0_C4_1
#define VAL_SYSREG_ID_AA64MMFR4_EL1 S3_0_C0_C7_4
#define VAL_SYSREG_MECIDR_EL2       S3_4_C10_C8_7

    .global read_currentel
read_currentel:
    mrs x0, CurrentEL
    ret

    .global read_mpidr_el1
read_mpidr_el1:
    mrs x0, MPIDR_EL1
    ret

    .global read_id_aa64pfr0_el1
read_id_aa64pfr0_el1:
    mrs x0, ID_AA64PFR0_EL1
    ret

    .global read_id_aa64pfr1_el1
read_id_aa64pfr1_el1:
    mrs x0, VAL_SYSREG_ID_AA64PFR1_EL1
    ret

    .global read_id_aa64mmfr4_el1
read_id_aa64mmfr4_el1:
    mrs x0, VAL_SYSREG_ID_AA64MMFR4_EL1
    ret

    .global read_mecidr_el2
read_mecidr_el2:
    mrs x0, VAL_SYSREG_MECIDR_EL2
    ret

    .global read_cptr_el2
read_cptr_el2:
    mrs x0, CPTR_EL2
    ret

    .global write_cptr_el2
write_cptr_el2:
    msr CPTR_EL2, x0
    isb
    ret

    .global read_cpacr_el1
read_cpacr_el1:
    mrs x0, CPACR_EL1
    ret

    .global write_cpacr_el1
write_cpacr_el1:
    msr CPACR_EL1, x0
    isb
    ret

    .global write_vbar_el2
write_vbar_el2:
    msr VBAR_EL2, x0
    isb
    ret

    .global write_vbar_el1
write_vbar_el1:
    msr VBAR_EL1, x0
    isb
    ret

    .global write_elr_el2
write_elr_el2:
    msr ELR_EL2, x0
    isb
    ret

    .global write_elr_el1
write_elr_el1:
    msr ELR_EL1, x0
    isb
    ret

    .global write_spsr_el2
write_spsr_el2:
    msr SPSR_EL2, x0
    isb
    ret

    .global read_sctlr_el2
read_sctlr_el2:
    mrs x0, SCTLR_EL2
    ret

    .global write_sctlr_el2
write_sctlr_el2:
    msr SCTLR_EL2, x0
    isb
    ret

    .global read_sctlr_el1
read_sctlr_el1:
    mrs x0, SCTLR_EL1
    ret

    .global write_sctlr_el1
write_sctlr_el1:
    msr SCTLR_EL1, x0
    isb
    ret

/** @brief Read the implemented physical-address range. */
    .global val_read_id_aa64mmfr0_el1
val_read_id_aa64mmfr0_el1:
    mrs x0, ID_AA64MMFR0_EL1
    ret

/** @brief Save EL2 translation controls for a temporary device mapping. */
    .global val_read_tcr_el2
val_read_tcr_el2:
    mrs x0, TCR_EL2
    ret

/** @brief Invalidate EL2 stage-1 translations after publishing descriptors. */
    .global val_tlbi_all_el2
val_tlbi_all_el2:
    dsb sy
    tlbi alle2is
    dsb sy
    isb
    ret

/** @brief Yield without waiting indefinitely for an event during an ABI retry. */
    .global val_mp_relax
val_mp_relax:
    yield
    ret

/** @brief Clean a bounded range to PoC using the current PE's cache-line size. */
    .global val_cache_clean_poc
val_cache_clean_poc:
    cbz x1, 2f
    add x1, x0, x1
    mrs x2, ctr_el0
    ubfx x2, x2, #16, #4
    mov x3, #4
    lsl x2, x3, x2
    sub x3, x2, #1
    bic x0, x0, x3
1:
    dc cvac, x0
    add x0, x0, x2
    cmp x0, x1
    b.lo 1b
2:
    dsb sy
    ret

    .global write_tcr_el2
write_tcr_el2:
    msr TCR_EL2, x0
    isb
    ret

    .global write_tcr_el1
write_tcr_el1:
    msr TCR_EL1, x0
    isb
    ret

    .global write_ttbr0_el2
write_ttbr0_el2:
    msr TTBR0_EL2, x0
    isb
    ret

    .global write_ttbr0_el1
write_ttbr0_el1:
    msr TTBR0_EL1, x0
    isb
    ret

    .global write_mair_el2
write_mair_el2:
    msr MAIR_EL2, x0
    isb
    ret

    .global write_mair_el1
write_mair_el1:
    msr MAIR_EL1, x0
    isb
    ret

    .global dsb_sy
dsb_sy:
    dsb sy
    ret

    .global isb_sy
isb_sy:
    isb
    ret

    .global enable_irq
enable_irq:
    msr DAIFClr, #2
    ret

    .global disable_irq
disable_irq:
    msr DAIFSet, #2
    ret

    .global mask_fiq
mask_fiq:
    msr DAIFSet, #1
    ret

    .global unmask_fiq
unmask_fiq:
    msr DAIFClr, #1
    ret

    .global mask_serror
mask_serror:
    msr DAIFSet, #4
    ret

    .global unmask_serror
unmask_serror:
    msr DAIFClr, #4
    ret

    .global write_icc_sre_el2
write_icc_sre_el2:
    msr ICC_SRE_EL2, x0
    isb
    ret

    .global write_icc_sre_el1
write_icc_sre_el1:
    msr ICC_SRE_EL1, x0
    isb
    ret

    .global write_icc_pmr_el1
write_icc_pmr_el1:
    msr ICC_PMR_EL1, x0
    isb
    ret

    .global write_icc_igrpen1_el1
write_icc_igrpen1_el1:
    msr ICC_IGRPEN1_EL1, x0
    isb
    ret

    .global read_icc_iar1_el1
read_icc_iar1_el1:
    mrs x0, ICC_IAR1_EL1
    ret

    .global write_icc_eoir1_el1
write_icc_eoir1_el1:
    msr ICC_EOIR1_EL1, x0
    isb
    ret

    .global val_wfe
val_wfe:
    wfe
    ret

    .global val_hvc0
val_hvc0:
    hvc #0
    ret
