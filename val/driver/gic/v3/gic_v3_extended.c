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

#if defined(TARGET_SIMULATION)
/*
 * Fast simulation helpers:
 *  - Avoid per-interrupt MMIO for disable/priority.
 *  - Use register granularity:
 *      * ICENABLERE / ICENABLER : 32 interrupts per write
 *      * IPRIORITYRE / IPRIORITYR: 4 interrupts per word
 */
static void v3_disable_all_espi_fast(uint32_t max_espi_intid)
{
  uint64_t gicd_base = val_get_gicd_base();
  uint32_t last = max_espi_intid;
  uint32_t nregs, r;

  if (last < EXTENDED_SPI_START_INTID)
    return;

  nregs = ((last - EXTENDED_SPI_START_INTID) / 32U) + 1U;
  for (r = 0; r < nregs; r++) {
    val_mmio_write(gicd_base + GICD_ICENABLERE + (4U * r), GIC_ALL_INTERRUPTS_MASK);
  }
}

static void v3_disable_all_eppi_fast(uint32_t max_eppi_intid)
{
  uint64_t cpuRd_base = v3_get_pe_gicr_base();
  uint32_t last = max_eppi_intid;
  uint32_t nregs, r;

  if (cpuRd_base == 0)
    return;
  if (last < EXTENDED_PPI_START_INTID)
    return;

  /*
   * Matches the existing per-ID calculation:
   *   regOffset = (int_id - EXTENDED_PPI_REG_OFFSET) / 32
   */
  nregs = ((last - EXTENDED_PPI_REG_OFFSET) / 32U) + 1U;
  for (r = 0; r < nregs; r++) {
    val_mmio_write(cpuRd_base + GICR_CTLR_FRAME_SIZE + GICR_ICENABLER + (4U * r),
                   GIC_ALL_INTERRUPTS_MASK);
  }
  return;
}

static void v3_set_all_espi_prio_fast(uint32_t max_espi_intid, uint8_t prio)
{
  uint64_t gicd_base = val_get_gicd_base();
  uint32_t last = max_espi_intid;
  uint32_t nwords, w;
  uint32_t prio_word = (uint32_t)prio;

  if (last < EXTENDED_SPI_START_INTID)
    return;
  /*Enumerating priority for 4 interrupts*/
  prio_word |= (prio_word << 8);
  prio_word |= (prio_word << 16);

  nwords = ((last - EXTENDED_SPI_START_INTID) / 4U) + 1U;
  for (w = 0; w < nwords; w++) {
    val_mmio_write(gicd_base + GICD_IPRIORITYRE + (4U * w), prio_word);
  }
  return;
}

static void v3_set_all_eppi_prio_fast(uint32_t max_eppi_intid, uint8_t prio)
{
  uint64_t cpuRd_base = v3_get_pe_gicr_base();
  uint32_t last = max_eppi_intid;
  uint32_t nwords, w;
  uint32_t prio_word = (uint32_t)prio;

  if (cpuRd_base == 0)
    return;
  if (last < EXTENDED_PPI_START_INTID)
    return;
  /*Enumerating priority for 4 interrupts*/
  prio_word |= (prio_word << 8);
  prio_word |= (prio_word << 16);

  nwords = ((last - EXTENDED_PPI_REG_OFFSET) / 4U) + 1U;
  for (w = 0; w < nwords; w++) {
    val_mmio_write(cpuRd_base + GICR_IPRIORITYR + (4U * w), prio_word);
  }
  return;
}
#endif /* TARGET_SIMULATION */

/**
  @brief  API used to clear espi interrupt
  @param  interrupt
  @return none
**/
void v3_clear_extended_spi_interrupt(uint32_t int_id)
{
  uint32_t reg_offset = (int_id - EXTENDED_SPI_START_INTID) / 32;
  uint32_t reg_shift  = (int_id - EXTENDED_SPI_START_INTID) % 32;

  val_mmio_write(val_get_gicd_base() + GICD_ICPENDRE0 + (4 * reg_offset), (1 << reg_shift));
  val_mmio_write(val_get_gicd_base() + GICD_ICACTIVERE0 + (4 * reg_offset), (1 << reg_shift));
}

/**
  @brief  checks if given int id is espi
  @param  int_id
  @return true if ESPI
**/
uint32_t
v3_is_extended_spi(uint32_t int_id)
{
  if (int_id >= EXTENDED_SPI_START_INTID && int_id <= val_gic_max_espi_val())
      return 1;
  else
      return 0;
}

/**
  @brief  checks if given int id is eppi
  @param  int_id
  @return true if EPPI
**/
uint32_t
v3_is_extended_ppi(uint32_t int_id)
{
  if (int_id >= EXTENDED_PPI_START_INTID && int_id <= val_gic_max_eppi_val())
      return 1;
  else
      return 0;
}

/**
  @brief  Disables the interrupt source
  @param  interrupt id
  @return none
**/
void
v3_disable_extended_interrupt_source(uint32_t int_id)
{
  uint32_t                regOffset;
  uint32_t                regShift;
  uint64_t                cpuRd_base;

  if (v3_is_extended_spi(int_id)) {
      /* Calculate register offset and bit position */
      regOffset = (int_id - EXTENDED_SPI_START_INTID) / 32;
      regShift = (int_id - EXTENDED_SPI_START_INTID) % 32;
      val_mmio_write(val_get_gicd_base() + GICD_ICENABLERE + (4 * regOffset), 1 << regShift);
  } else {
      /* Calculate register offset and bit position */
      regOffset = (int_id - EXTENDED_PPI_REG_OFFSET) / 32;
      regShift = (int_id - EXTENDED_PPI_REG_OFFSET) % 32;
      cpuRd_base = v3_get_pe_gicr_base();
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
v3_enable_extended_interrupt_source(uint32_t int_id)
{
  uint32_t                regOffset;
  uint32_t                regShift;
  uint64_t                cpuRd_base;

  if (v3_is_extended_spi(int_id)) {
      /* Calculate register offset and bit position */
      regOffset = (int_id - EXTENDED_SPI_START_INTID) / 32;
      regShift = (int_id - EXTENDED_SPI_START_INTID) % 32;
      val_mmio_write(val_get_gicd_base() + GICD_ICENABLERE + (4 * regOffset), 1 << regShift);
  } else {
      /* Calculate register offset and bit position */
      regOffset = (int_id - EXTENDED_PPI_REG_OFFSET) / 32;
      regShift = (int_id - EXTENDED_PPI_REG_OFFSET) % 32;
      cpuRd_base = v3_get_pe_gicr_base();
      if (cpuRd_base == 0) {
        return;
      }
      val_mmio_write(cpuRd_base + GICR_CTLR_FRAME_SIZE + GICR_ISENABLER + (4 * regOffset),
                   1 << regShift);
  }
}

/**
  @brief  Sets interrupt priority
  @param  interrupt id
  @param  priority
  @return none
**/
void
v3_set_extended_interrupt_priority(uint32_t int_id, uint32_t priority)
{
  uint32_t                regOffset;
  uint32_t                regShift;
  uint64_t                cpuRd_base;

  if (v3_is_extended_spi(int_id)) {
      /* Calculate register offset and bit position */
      regOffset = (int_id - EXTENDED_SPI_START_INTID) / 4;
      regShift = ((int_id - EXTENDED_SPI_START_INTID) % 4) * 8;

      val_mmio_write(val_get_gicd_base() + GICD_IPRIORITYRE + (4 * regOffset),
                    (val_mmio_read(val_get_gicd_base() + GICD_IPRIORITYRE + (4 * regOffset)) &
                     ~(0xff << regShift)) | priority << regShift);
  } else {
     /* Calculate register offset and bit position */
    regOffset = (int_id - EXTENDED_PPI_REG_OFFSET) / 4;
    regShift = ((int_id - EXTENDED_PPI_REG_OFFSET) % 4) * 8;

    cpuRd_base = v3_get_pe_gicr_base();
    if (cpuRd_base == 0) {
      return;
    }
    val_mmio_write(cpuRd_base + GICR_IPRIORITYR + (4 * regOffset),
                  (val_mmio_read(cpuRd_base + GICR_IPRIORITYR + (4 * regOffset)) &
                   ~(0xff << regShift)) | priority << regShift);
  }
}

/**
  @brief  Route interrupt to primary PE
  @param  interrupt id
  @return none
**/
void
v3_route_extended_interrupt(uint32_t int_id)
{
  uint64_t   gicd_base;
  uint64_t   cpuTarget;
  uint64_t   Mpidr;

  /* Get the distributor base */
  gicd_base = val_get_gicd_base();

  Mpidr = ArmReadMpidr();
  cpuTarget = Mpidr & (PE_AFF0 | PE_AFF1 | PE_AFF2 | PE_AFF3);

  val_mmio_write64(gicd_base + GICD_IROUTERn + (int_id * 8), cpuTarget);
}

/**
  @brief  Initializes the GIC v3 Extended Interrupts
  @param  none
  @return init success or failure
**/
void
v3_extended_init(void)
{
  uint32_t   max_num_espi_interrupts;
  uint32_t   max_num_eppi_interrupts;
  uint32_t   index;

  /* Get the max interrupt */
  max_num_espi_interrupts = val_gic_max_espi_val();
  max_num_eppi_interrupts = val_gic_max_eppi_val();

  val_print(ACS_PRINT_DEBUG, "\n GIC_INIT: Extended SPI Interrupts %d\n", max_num_espi_interrupts);
  val_print(ACS_PRINT_DEBUG, "\n GIC_INIT: Extended PPI Interrupts %d\n", max_num_eppi_interrupts);

#if defined(TARGET_SIMULATION)
  /* Fast-sim: bulk disable in 32-interrupt chunks */
  v3_disable_all_espi_fast(max_num_espi_interrupts);
  v3_disable_all_eppi_fast(max_num_eppi_interrupts);

  /* Fast-sim: bulk priority programming in 4-interrupt chunks (no RMW) */
  v3_set_all_espi_prio_fast(max_num_espi_interrupts, (uint8_t)GIC_DEFAULT_PRIORITY);
  v3_set_all_eppi_prio_fast(max_num_eppi_interrupts, (uint8_t)GIC_DEFAULT_PRIORITY);
#else
  /* Disable all ESPI interrupt */
  for (index = EXTENDED_SPI_START_INTID; index <= max_num_espi_interrupts; index++) {
      v3_disable_extended_interrupt_source(index);
  }

  /* Disable all EPPI interrupt */
  for (index = EXTENDED_PPI_START_INTID; index <= max_num_eppi_interrupts; index++) {
      v3_disable_extended_interrupt_source(index);
  }

  /* Set default for ESPI priority */
  for (index = EXTENDED_SPI_START_INTID; index <= max_num_espi_interrupts; index++) {
      v3_set_extended_interrupt_priority(index, GIC_DEFAULT_PRIORITY);
  }

  /* Set default for EPPI priority */
  for (index = EXTENDED_PPI_START_INTID; index <= max_num_eppi_interrupts; index++) {
      v3_set_extended_interrupt_priority(index, GIC_DEFAULT_PRIORITY);
  }
#endif

#if defined(TARGET_SIMULATION)
  /*
   * Fast-sim: routing a very large ESPI range can be slow. Cap routing.
   * (Tune this cap if your sim actually uses more.)
   */
    uint32_t route_last = max_num_espi_interrupts;
    uint32_t cap = EXTENDED_SPI_START_INTID + 256U - 1U;
    if (route_last > cap) route_last = cap;
    for (index = EXTENDED_SPI_START_INTID; index <= route_last; index++) {
      v3_route_extended_interrupt(index);
    }
#else
  for (index = EXTENDED_SPI_START_INTID; index <= (max_num_espi_interrupts); index++) {
    v3_route_extended_interrupt(index);
  }
#endif

}
