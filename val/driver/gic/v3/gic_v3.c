/** @file
 * Copyright (c) 2021, 2023-2026, Arm Limited or its affiliates. All rights reserved.
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

#include "include/acs_val.h"
#include "include/acs_gic.h"
#include "include/acs_gic_support.h"
#include "include/acs_pe.h"
#include "gic_v3.h"
#include "gic.h"
#include "acs_exception.h"

/**
  @brief  checks if given int id is spi
  @param  int id
  @return true if SPI
**/
uint32_t
IsSpi(uint32_t int_id)
{
  if (int_id >= 32 && int_id < 1020)
      return 1;
  else
      return 0;
}

/**
  @brief  Retruns GICD_TYPER value
  @param  none
  @return gicd typer value
**/
uint32_t
v3_read_gicdTyper(void)
{
  return val_mmio_read(val_get_gicd_base() + GICD_TYPER);
}

/**
  @brief  Retruns GICR_TYPER value
  @param  none
  @return gicr typer value
**/
uint64_t
v3_read_gicr_typer(void)
{
  return val_mmio_read64(v3_get_pe_gicr_base() + GICR_TYPER);
}


/**
  @brief  derives current pe rd base
  @param  rd base
  @param  rd base length
  @return pe rd base
**/
static uint64_t
CurrentCpuRDBase(uint64_t mGicRedistributorBase, uint32_t length)
{
  uint64_t     Affinity, CpuAffinity;
  uint64_t     GicRedistributorGranularity;
  uint64_t     GicCpuRedistributorBase;
  uint64_t     Mpidr;

  Mpidr = ArmReadMpidr();

  CpuAffinity = (Mpidr & (PE_AFF0 | PE_AFF1 | PE_AFF2)) | ((Mpidr & PE_AFF3) >> 8);

  GicRedistributorGranularity = GICR_CTLR_FRAME_SIZE + GICR_SGI_PPI_FRAME_SIZE;

  GicCpuRedistributorBase = mGicRedistributorBase;

  /* If information is present in GICC Structure */
  if (length == 0)
  {
      Affinity = (val_mmio_read64(GicCpuRedistributorBase + GICR_TYPER) & GICR_TYPER_AFF) >> 32;
      if (Affinity == CpuAffinity) {
        return GicCpuRedistributorBase;
      }
      return 0;
  }

  /* If information is present in GICR Structure */
  while (GicCpuRedistributorBase < (mGicRedistributorBase + length))
  {
    Affinity = (val_mmio_read64(GicCpuRedistributorBase + GICR_TYPER) & GICR_TYPER_AFF) >> 32;
    if (Affinity == CpuAffinity) {
      return GicCpuRedistributorBase;
    }

    /* Move to the next GIC Redistributor frame */
    GicCpuRedistributorBase += GicRedistributorGranularity;
  }

  return 0;
}

/**
  @brief  Marks primary PE as online
  @param  none
  @return none
**/
static void
WakeUpRD(void)
{
  uint32_t                rdbase_len;
  uint64_t                rd_base;
  uint64_t                cpuRd_base;
  uint32_t                tmp;

  rd_base = val_get_gicr_base(&rdbase_len, 0);
  cpuRd_base = CurrentCpuRDBase(rd_base, rdbase_len);
  if (cpuRd_base == 0) {
    return;
  }

  tmp = val_mmio_read(cpuRd_base + GICR_WAKER);
  if ((tmp >> 1) & 0x01)
      val_mmio_write(cpuRd_base + GICR_WAKER, tmp & ~0x02);

#if defined(TARGET_SIMULATION)
    uint32_t timeout = 0x20000;
    uint32_t spin = 0;
    do {
        tmp = (val_mmio_read(cpuRd_base + GICR_WAKER) >> GICR_WAKER_CHILDREN_ASLEEP_SHIFT)
              & GICR_WAKER_BIT_MASK;
        /* Interface Active */
        if (!tmp) break;
        if ((++spin & GICR_WAKER_SPIN_DELAY_MASK) == 0) {
          /* Give sim time to advance without hammering MMIO */
          val_time_delay_ms(1);
        }
    } while (--timeout);
#else
  do {
      tmp = (val_mmio_read(cpuRd_base + GICR_WAKER)
            >> GICR_WAKER_CHILDREN_ASLEEP_SHIFT) & GICR_WAKER_BIT_MASK;
  } while (tmp);
#endif
}

/**
  @brief  derives current pe rd base
  @param  none
  @return pe rd base
**/
uint64_t v3_get_pe_gicr_base(void)
{
  uint32_t                rdbase_len;
  uint64_t                rd_base;

  rd_base = val_get_gicr_base(&rdbase_len, 0);

  return CurrentCpuRDBase(rd_base, rdbase_len);
}

/**
  @brief  Acknowledges the interrupt
  @param  none
  @return interrupt id
**/
uint32_t
v3_AcknowledgeInterrupt(void)
{
  return bsa_gic_ack_intr();
}

/**
  @brief  Ends the interrupt
  @param  interrupt id
  @return none
**/
void
v3_EndofInterrupt(uint32_t int_id)
{
  bsa_gic_end_intr(int_id);
}

/**
  @brief  Disables the interrupt source
  @param  interrupt id
  @return none
**/
void
v3_DisableInterruptSource(uint32_t int_id)
{
  uint32_t                regOffset;
  uint32_t                regShift;
  uint32_t                rdbase_len;
  uint64_t                rd_base;
  uint64_t                cpuRd_base;

  if (v3_is_extended_spi(int_id) || v3_is_extended_ppi(int_id)) {
      v3_disable_extended_interrupt_source(int_id);
      return;
  }

  /* Calculate register offset and bit position */
  regOffset = int_id / 32;
  regShift = int_id % 32;

  if (IsSpi(int_id)) {
      val_mmio_write(val_get_gicd_base() + GICD_ICENABLER + (4 * regOffset), 1 << regShift);
  } else {
    rd_base = val_get_gicr_base(&rdbase_len, 0);
    cpuRd_base = CurrentCpuRDBase(rd_base, rdbase_len);
    if (cpuRd_base == 0) {
      return;
    }
    val_mmio_write(cpuRd_base + GICR_CTLR_FRAME_SIZE + GICR_ICENABLER + (4 * regOffset),
                   1 << regShift);
  }
}

/**
  @brief  Enables the interrupt source
  @param  interrupt id
  @return none
**/
void
v3_EnableInterruptSource(uint32_t int_id)
{
  uint32_t                regOffset;
  uint32_t                regShift;
  uint32_t                rdbase_len;
  uint64_t                rd_base;
  uint64_t                cpuRd_base;

  if (v3_is_extended_spi(int_id) || v3_is_extended_ppi(int_id)) {
      v3_enable_extended_interrupt_source(int_id);
      return;
  }
  /* Calculate register offset and bit position */
  regOffset = int_id / 32;
  regShift = int_id % 32;

  if (IsSpi(int_id)) {
      val_mmio_write(val_get_gicd_base() + GICD_ISENABLER + (4 * regOffset), 1 << regShift);
  } else {
    rd_base = val_get_gicr_base(&rdbase_len, 0);
    cpuRd_base = CurrentCpuRDBase(rd_base, rdbase_len);
    if (cpuRd_base == 0) {
      return;
    }
    val_mmio_write(cpuRd_base + GICR_CTLR_FRAME_SIZE + GICR_ISENABLER + (4 * regOffset),
                   1 << regShift);
  }
}

#if defined(TARGET_SIMULATION)
/*
 * Fast simulation helpers:
 * - Avoid per-interrupt MMIO and RMW sequences.
 * - Use register granularity (ICENABLER: 32 IRQs, IPRIORITYR: 4 IRQs).
 */
static void v3_DisableAllInterrupts_fast(uint32_t max_num_interrupts)
{
  uint64_t gicd_base = val_get_gicd_base();
  uint64_t gicr_base = v3_get_pe_gicr_base();
  uint32_t reg;

  /* Disable SGI/PPI (0..31) for current PE */
  if (gicr_base) {
    val_mmio_write(gicr_base + GICR_CTLR_FRAME_SIZE + GICR_ICENABLER, GIC_ALL_INTERRUPTS_MASK);
  }

  /* Disable SPIs (32..max-1) in 32-IRQ chunks: start at reg index 1 (ID 32) */
  for (reg = 1; (reg * 32U) < max_num_interrupts; reg++) {
    val_mmio_write(gicd_base + GICD_ICENABLER + (4U * reg), GIC_ALL_INTERRUPTS_MASK);
  }
  return;
}

static void v3_SetDefaultPriority_fast(uint32_t max_num_interrupts, uint8_t prio)
{
  uint64_t gicd_base = val_get_gicd_base();
  uint64_t gicr_base = v3_get_pe_gicr_base();
  uint32_t words, w;
  uint32_t prio_word = (uint32_t)prio;

  /*enumerating priorty for 4 interrupts at a time*/
  prio_word |= (prio_word << 8);
  prio_word |= (prio_word << 16);

  /* SGI/PPI priorities (0..31) for current PE: 32 IRQs => 8 words */
  if (gicr_base) {
    for (w = 0; w < (MAX_SGI_PPI / 4U); w++) {
      val_mmio_write(gicr_base + GICR_IPRIORITYR + (4U * w), prio_word);
    }
  }

  /* SPI priorities (32..max-1). We can just program all supported IDs in distributor. */
  words = (max_num_interrupts + 3U) / 4U;
  for (w = 0; w < words; w++) {
    val_mmio_write(gicd_base + GICD_IPRIORITYR + (4U * w), prio_word);
  }
  return;
}
#else
/**
  @brief  Sets interrupt priority
  @param  interrupt id
  @param  priority
  @return none
**/
static void
v3_SetInterruptPriority(uint32_t int_id, uint32_t priority)
{
  uint32_t                regOffset;
  uint32_t                regShift;
  uint32_t                rdbase_len;
  uint64_t                rd_base;
  uint64_t                cpuRd_base;

  if (v3_is_extended_spi(int_id) || v3_is_extended_ppi(int_id)) {
      v3_set_extended_interrupt_priority(int_id, priority);
      return;
  }

  /* Calculate register offset and bit position */
  regOffset = int_id / 4;
  regShift = (int_id % 4) * 8;

  if (IsSpi(int_id)) {
      val_mmio_write(val_get_gicd_base() + GICD_IPRIORITYR + (4 * regOffset),
                    (val_mmio_read(val_get_gicd_base() + GICD_IPRIORITYR + (4 * regOffset)) &
                     ~(0xff << regShift)) | priority << regShift);
  } else {
    rd_base = val_get_gicr_base(&rdbase_len, 0);
    cpuRd_base = CurrentCpuRDBase(rd_base, rdbase_len);
    if (cpuRd_base == 0) {
      return;
    }
    val_mmio_write(cpuRd_base + GICR_IPRIORITYR + (4 * regOffset),
                  (val_mmio_read(cpuRd_base + GICR_IPRIORITYR + (4 * regOffset)) &
                   ~(0xff << regShift)) | priority << regShift);
  }
  return;
}
#endif

/**
  @brief  Initializes the GIC v3
  @param  none
  @return init success or failure
**/
void
v3_Init(void)
{
  uint32_t   max_num_interrupts;
  uint32_t   index;
  uint64_t   gicd_base;
  uint64_t   cpuTarget;
  uint64_t   Mpidr;
#if defined(TARGET_SIMULATION)
  uint32_t   route_max;
#endif

  if (val_gic_espi_support() || val_gic_eppi_support())
    v3_extended_init();

  /* Get the distributor base */
  gicd_base = val_get_gicd_base();

  /* Get the max interrupt */
  max_num_interrupts = val_get_max_intid();

  val_print(ACS_PRINT_DEBUG, "  GIC_INIT: D base %x\n", gicd_base);
  val_print(ACS_PRINT_DEBUG, "  GIC_INIT: Interrupts %d\n", max_num_interrupts);

#if defined(TARGET_SIMULATION)
  /* Fast-sim: disable in register chunks (32 IRQs per write) */
  v3_DisableAllInterrupts_fast(max_num_interrupts);
#else
  /* Disable all interrupts */
  for (index = 0; index < max_num_interrupts; index++) {
    v3_DisableInterruptSource(index);
  }
#endif

  /* Set vector table */
  bsa_gic_vector_table_init();

  if (val_pe_reg_read(CurrentEL) == AARCH64_EL2) {
    /* Route exception to EL2 */
    GicWriteHcr(1 << 27);
  }

  GicClearDaif();

  /* Wake up redistributor before programming SGI/PPI state */
  WakeUpRD();

  /* Set ARI bits for v3 mode */
  val_mmio_write(gicd_base + GICD_CTLR, val_mmio_read(gicd_base + GICD_CTLR) | GIC_ARE_ENABLE);
  val_mmio_write(gicd_base + GICD_CTLR, val_mmio_read(gicd_base + GICD_CTLR) | 0x2);
  val_print(ACS_PRINT_DEBUG, "  GIC_INIT: GICD_CTLR value 0x%08x\n",
                             val_mmio_read(gicd_base + GICD_CTLR));

#if defined(TARGET_SIMULATION)
  /* Fast-sim: program priorities in 4-IRQ chunks (no per-ID RMW) */
  v3_SetDefaultPriority_fast(max_num_interrupts, (uint8_t)GIC_DEFAULT_PRIORITY);
#else
  /* Set default priority */
  for (index = 0; index < max_num_interrupts; index++) {
    v3_SetInterruptPriority(index, GIC_DEFAULT_PRIORITY);
  }
#endif

  Mpidr = ArmReadMpidr();
  cpuTarget = Mpidr & (PE_AFF0 | PE_AFF1 | PE_AFF2 | PE_AFF3);

#if defined(TARGET_SIMULATION)
  /*
   * Fast-sim: routing every SPI can be very slow.
   * Cap to the first 256 SPIs (IDs 32..287) unless max is smaller.
   */
  route_max = max_num_interrupts;
  if (route_max > (MAX_SGI_PPI + GIC_SIM_MAX_SPI_ROUTE_COUNT))
    route_max = (MAX_SGI_PPI + GIC_SIM_MAX_SPI_ROUTE_COUNT);
  for (index = 0; index < (route_max - 32U); index++) {
    val_mmio_write64(gicd_base + GICD_IROUTERn + (index * 8U), cpuTarget);
  }
#else
  for (index = 0; index < (max_num_interrupts - 32); index++) {
    val_mmio_write64(gicd_base + GICD_IROUTERn + (index * 8), cpuTarget);
  }
#endif

  /* Initialize cpu interface */
  val_gic_cpuif_init();
}
