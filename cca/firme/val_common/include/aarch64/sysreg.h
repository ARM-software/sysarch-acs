/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

/*
 * AArch64 system register / barrier helpers.
 *
 * ACS style: avoid inline assembly in C. These helpers are implemented in
 * `val_common/src/val_sysreg.S` and exposed as plain functions.
 */

uint64_t read_currentel(void);
uint64_t read_mpidr_el1(void);
uint64_t read_id_aa64pfr0_el1(void);
uint64_t read_id_aa64pfr1_el1(void);
uint64_t read_id_aa64mmfr4_el1(void);
uint64_t read_mecidr_el2(void);
uint64_t val_read_id_aa64mmfr0_el1(void);
uint64_t val_read_tcr_el2(void);
void val_tlbi_all_el2(void);
void val_cache_clean_poc(uintptr_t start, size_t size);
void val_mp_relax(void);

uint64_t read_cptr_el2(void);
void write_cptr_el2(uint64_t v);
uint64_t read_cpacr_el1(void);
void write_cpacr_el1(uint64_t v);

void write_vbar_el2(uint64_t v);
void write_vbar_el1(uint64_t v);

void write_elr_el2(uint64_t v);
void write_elr_el1(uint64_t v);
void write_spsr_el2(uint64_t v);

uint64_t read_sctlr_el2(void);
void write_sctlr_el2(uint64_t v);
uint64_t read_sctlr_el1(void);
void write_sctlr_el1(uint64_t v);
void write_tcr_el2(uint64_t v);
void write_tcr_el1(uint64_t v);
void write_ttbr0_el2(uint64_t v);
void write_ttbr0_el1(uint64_t v);
void write_mair_el2(uint64_t v);
void write_mair_el1(uint64_t v);

void dsb_sy(void);
void isb_sy(void);

void enable_irq(void);
void disable_irq(void);
void mask_fiq(void);
void unmask_fiq(void);
void mask_serror(void);
void unmask_serror(void);

/* GICv3 system register interface helpers (EL depends on caller). */
void write_icc_sre_el2(uint64_t v);
void write_icc_sre_el1(uint64_t v);
void write_icc_pmr_el1(uint64_t v);
void write_icc_igrpen1_el1(uint64_t v);
uint64_t read_icc_iar1_el1(void);
void write_icc_eoir1_el1(uint64_t v);

/* Low-level wait/trap helpers. */
void val_wfe(void);
void val_hvc0(void);
