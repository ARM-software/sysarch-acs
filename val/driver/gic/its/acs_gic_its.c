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


#include "acs_gic_its.h"
#include "acs_gic_support.h"
#include "acs_memory.h"
#include "acs_pe.h"
#include "val_sysreg_pe.h"

extern GIC_ITS_INFO    *g_gic_its_info;
static uint32_t        *g_cwriter_ptr;
static uint32_t        g_its_setup_done;

typedef struct {
  uint64_t *l1_base;
  uint64_t id_count;
  uint32_t l1_entries;
  uint32_t l2_entries;
  uint32_t page_size;
} ITS_DEVICE_TABLE;

typedef ITS_DEVICE_TABLE ITS_DEVICE_TABLES[ARM_NUM_GITS_BASER];
static ITS_DEVICE_TABLES *g_device_tables;

#define ITS_L2_PA_MASK 0x000FFFFFFFFFF000ULL

static uint64_t ItsAllocateL2(uint32_t page_size)
{
  void *page = val_aligned_alloc(page_size, page_size);
  uint64_t pa;

  if (page == NULL)
    return 0;

  pa = (uint64_t)val_memory_virt_to_phys(page);
  if ((pa == 0) || (pa & ~ITS_L2_PA_MASK) || (pa & (page_size - 1))) {
    val_memory_free_aligned(page);
    return 0;
  }

  val_memory_set(page, page_size, 0);
  val_pe_cache_clean_range((uint64_t)page, page_size);
  dsbsy();

  return pa | ARM_GITS_BASER_VALID;
}

static uint32_t ItsEnsureDeviceTable(uint32_t its_index, uint32_t device_id,
                                     uint32_t allocate_l2)
{
  uint32_t baser_index, l1_index;
  uint32_t device_table_found = 0;
  uint64_t descriptor, typer;
  ITS_DEVICE_TABLE *table;
  uint64_t *entry;

  if ((g_gic_its_info == NULL) || (g_device_tables == NULL) ||
      (its_index >= g_gic_its_info->GicNumIts))
    return ACS_STATUS_ERR;

  typer = val_mmio_read64(g_gic_its_info->GicIts[its_index].Base + ARM_GITS_TYPER);
  if ((uint64_t)device_id >= (1ULL << (ARM_GITS_TYPER_DevBits(typer) + 1)))
    return ACS_STATUS_ERR;

  for (baser_index = 0; baser_index < ARM_NUM_GITS_BASER; baser_index++) {
    table = &g_device_tables[its_index][baser_index];
    if (table->id_count == 0)
      continue;

    device_table_found = 1;
    if ((uint64_t)device_id >= table->id_count)
      return ACS_STATUS_ERR;
    if (table->l1_base == NULL)
      continue;

    l1_index = device_id / table->l2_entries;
    if (l1_index >= table->l1_entries)
      return ACS_STATUS_ERR;

    entry = &table->l1_base[l1_index];
    if (*entry & ARM_GITS_BASER_VALID)
      continue;
    if (!allocate_l2)
      return ACS_STATUS_ERR;

    descriptor = ItsAllocateL2(table->page_size);
    if (descriptor == 0) {
      val_print(ERROR, "\nITS : L2 allocation failed for DeviceID 0x%x", device_id);
      return ACS_STATUS_ERR;
    }

    *entry = descriptor;
    val_pe_cache_clean_range((uint64_t)entry, sizeof(*entry));
    dsbsy();
  }

  return device_table_found ? ACS_STATUS_PASS : ACS_STATUS_ERR;
}

uint32_t GET_NUM_BITS(uint64_t value)
{
  uint64_t bit_pos = 0;

  while (!((value >> bit_pos) & 0x1)) {
    bit_pos++;
  }

  return bit_pos;
}

uint64_t val_its_get_curr_rdbase(uint64_t rd_base, uint32_t length)
{
  uint64_t     Mpidr;
  uint32_t     Affinity, CpuAffinity;
  uint32_t     rd_granularity;
  uint64_t     curr_rd_base; /* RD Base for Current CPU */
  uint32_t     typer;

  Mpidr = read_mpidr_el1();

  CpuAffinity = (Mpidr & (ARM_CORE_AFF0 | ARM_CORE_AFF1 | ARM_CORE_AFF2)) |
                ((Mpidr & ARM_CORE_AFF3) >> 8);

  rd_granularity = ARM_GICR_CTLR_FRAME_SIZE
                   + ARM_GICR_SGI_PPI_FRAME_SIZE;

  typer = val_mmio_read(rd_base + ARM_GICR_TYPER);

  /* Skip VLPI_base + reserved page */
  if (typer & ARM_GICR_TYPER_VLPIS)
      rd_granularity += ARM_GICR_VLPI_FRAME_SIZE + ARM_GICR_RESERVED_PAGE_SIZE;

  curr_rd_base = rd_base;

  /* If information is present in GICC Structure */
  if (length == 0)
  {
      Affinity = val_mmio_read(curr_rd_base + ARM_GICR_TYPER + NEXT_DW_OFFSET);
      if (Affinity == CpuAffinity) {
        return curr_rd_base;
      }
      return 0;
  }

  /* If information is present in GICR Structure */
  while (curr_rd_base < (rd_base + length))
  {
    Affinity = val_mmio_read(curr_rd_base + ARM_GICR_TYPER + NEXT_DW_OFFSET);

    if (Affinity == CpuAffinity) {
      return curr_rd_base;
    }

    /* Move to the next GIC Redistributor frame */
    curr_rd_base += rd_granularity;
  }

  return 0;
}

uint32_t val_its_gicd_lpi_support(uint64_t gicd_base)
{
  return (val_mmio_read(gicd_base + ARM_GICD_TYPER) & ARM_GICD_TYPER_LPIS);
}

uint32_t val_its_gicr_lpi_support(uint64_t rd_base)
{
  return (val_mmio_read(rd_base + ARM_GICR_TYPER) & ARM_GICR_TYPER_PLPIS);
}

static uint32_t
ArmGicSetItsCommandQueueBase(
    uint32_t     its_index
  )
{
  /* Allocate Memory for Command queue. Set command queue base in GITS_CBASER. */
  uint64_t                    Address;
  uint64_t                    write_value;
  uint64_t                    ItsBase;

  ItsBase = g_gic_its_info->GicIts[its_index].Base;

  Address = (uint64_t)val_aligned_alloc(SIZE_64KB, (NUM_PAGES_8 * SIZE_4KB));

  if (!Address) {
    val_print(ERROR, "\nITS : Could Not Allocate Memory CmdQ. Test may not pass.");
    return 1;
  }

  val_memory_set((void *)Address,  (NUM_PAGES_8*SIZE_4KB), 0);

  g_gic_its_info->GicIts[its_index].CommandQBase = Address;

  write_value = val_mmio_read64(ItsBase + ARM_GITS_CBASER) & (~ARM_GITS_CBASER_PA_MASK);
  write_value = write_value | (Address & ARM_GITS_CBASER_PA_MASK);
  write_value = write_value | ARM_GITS_CBASER_VALID | (NUM_PAGES_8 - 1);
  write_value = write_value | ((7ULL << 59) | (7ULL << 53) | (2ULL << 10));

  val_mmio_write64(ItsBase + ARM_GITS_CBASER, write_value);

  return 0;
}


static uint32_t ArmGicSetItsTables(uint32_t its_index)
{
  uint32_t                Pages;
  uint32_t                entry_size;
  uint64_t                TableSize;
  uint64_t                id_count, flat_table_capacity;
  uint64_t                its_baser, its_typer;
  uint8_t                 it, table_type;
  uint64_t                write_value, read_value;
  uint32_t                DevBits, CIDBits;
  uint64_t                Address;
  uint64_t                ItsBase;
  uint64_t                indirect_supported = 0, max_page_size = 0;
  uint64_t                lvl2_entries, lvl2_bits, lvl1_bits;
  uint64_t                baser_pgsz = 0x00, indirect_table;
  uint64_t                *lvl1_ptr = NULL;
  uint64_t                temp_val;
  ITS_DEVICE_TABLE        *device_table;

  ItsBase = g_gic_its_info->GicIts[its_index].Base;

  /* Allocate Memory for Table Depending on the Type of the table in GITS_BASER<n>. */
  for (it = 0; it < ARM_NUM_GITS_BASER; it++) {

    its_baser = val_mmio_read64(ItsBase + ARM_GITS_BASER(it));
    table_type = ARM_GITS_BASER_GET_TYPE(its_baser);
    entry_size = ARM_GITS_BASER_GET_ENTRY_SIZE(its_baser);

    if ((table_type != ARM_GITS_TBL_TYPE_DEVICE) &&
        (table_type != ARM_GITS_TBL_TYPE_CLCN))
      continue;
    indirect_supported = 0;
    max_page_size = 0;
    baser_pgsz = 0;

    its_typer = val_mmio_read64(ItsBase + ARM_GITS_TYPER);
    DevBits = ARM_GITS_TYPER_DevBits(its_typer);
    CIDBits = ARM_GITS_TYPER_CIDBits(its_typer);

    /* check if design supports indirect tables */
    write_value = its_baser;
    write_value |= 1ULL << 62;
    val_mmio_write64(ItsBase + ARM_GITS_BASER(it), write_value);
    read_value = val_mmio_read64(ItsBase + ARM_GITS_BASER(it));
    if ((read_value >> 62) & 0x1) {
      indirect_supported = 1;
    }

   /* reset the register to original value */
    val_mmio_write64(ItsBase + ARM_GITS_BASER(it), its_baser);

   /* Check the max page size supported */
    temp_val = 0x00;
    while (temp_val < ARM_GITS_BASER_MAX_PAGESZ) { // <= 64KB
      read_value = val_mmio_read64(ItsBase + ARM_GITS_BASER(it));
      write_value = read_value & ~(ARM_GITS_BASER_PAGE_MASK);
      write_value |= (temp_val << ARM_GITS_BASER_PAGE_SHIFT);
      val_mmio_write64(ItsBase + ARM_GITS_BASER(it), write_value);
      /* read back to check the actual value */
      read_value = val_mmio_read64(ItsBase + ARM_GITS_BASER(it));
      if (((read_value & ARM_GITS_BASER_PAGE_MASK) >> ARM_GITS_BASER_PAGE_SHIFT) == temp_val)
      {
        if (temp_val == ARM_GITS_BASER_PGSZ_4K) {
           max_page_size = PAGE_SIZE_4K;
           baser_pgsz = ARM_GITS_BASER_PGSZ_4K;
        } else if (temp_val == ARM_GITS_BASER_PGSZ_16K) {
           max_page_size = PAGE_SIZE_16K;
           baser_pgsz = ARM_GITS_BASER_PGSZ_16K;
        } else if (temp_val == ARM_GITS_BASER_PGSZ_64K) {
           max_page_size = PAGE_SIZE_64K;
           baser_pgsz = ARM_GITS_BASER_PGSZ_64K;
        }
      }
      temp_val = temp_val + 1;
    }

   /* reset the register to original value */
    val_mmio_write64(ItsBase + ARM_GITS_BASER(it), its_baser);

    if (max_page_size == 0) {
      val_print(ERROR, "\nITS : No supported BASER page size");
      return ACS_STATUS_ERR;
    }

    if (table_type == ARM_GITS_TBL_TYPE_DEVICE) {
      id_count = 1ULL << (DevBits + 1);
      TableSize = id_count * (entry_size + 1); // Assuming Single Level Table

    } else if (table_type == ARM_GITS_TBL_TYPE_CLCN) {
      id_count = 1ULL << (CIDBits + 1);
      TableSize = id_count * (entry_size + 1); // Assuming Single Level Table

    } else {
      continue;
    }

  lvl2_entries = 0;
  lvl2_bits = 0;
  indirect_table = 0;
  lvl1_bits = 0;

  if (TableSize > max_page_size*ARM_GITS_BASER_MAX_PAGES) {
    if (indirect_supported == 1) {
      indirect_table = 1;
      lvl2_entries = max_page_size/(entry_size+1);
      lvl2_bits = GET_NUM_BITS(lvl2_entries);
      if (table_type == ARM_GITS_TBL_TYPE_DEVICE) {
        lvl1_bits =  (DevBits+1)-lvl2_bits;
      } else if (table_type == ARM_GITS_TBL_TYPE_CLCN) {
        lvl1_bits =  (CIDBits+1)-lvl2_bits;
      }

      // level 1 needs 64 bits i.e 8 bytes
      TableSize = (1ULL << lvl1_bits)*ARM_GITS_BASER_INDIRECT_LVL1_ENTRY_SIZE;
      if (TableSize > max_page_size*ARM_GITS_BASER_MAX_PAGES) {
        val_print(WARN, "\nITS : Level 1 table size exceeded limit");
        val_print(WARN, "\nmax did size will not be supported..");
        TableSize = max_page_size*ARM_GITS_BASER_MAX_PAGES;
      }
    } else {
      val_print(WARN, "\nITS : Multilevel table not supported and single level table");
      val_print(WARN, "\nsize exceeded limit settings support only upto 24 bit ");
      val_print(WARN, "\n(if entry_size is 8 bytes)");
      TableSize = max_page_size*ARM_GITS_BASER_MAX_PAGES;
    }
  }

  Pages = TableSize/max_page_size;
  if (TableSize % max_page_size)
  {
    Pages = Pages+1;
  }

  TableSize = Pages*max_page_size;

  Address = (uint64_t)val_aligned_alloc(max_page_size, TableSize);

  if (!Address) {
      val_print(ERROR, "\nITS : Could Not Allocate Memory DT/CT. Test may not pass.");
      return 1;
  }

  val_memory_set((void *)Address,  TableSize, 0);

  if ((indirect_table == 1) && (table_type == ARM_GITS_TBL_TYPE_CLCN)) {
    lvl1_ptr = (uint64_t *)(Address);
    for (uint64_t i = 0; (i < (1ULL << lvl1_bits)) &&
                         (i < TableSize / sizeof(*lvl1_ptr)); i++) {
      temp_val = ItsAllocateL2((uint32_t)max_page_size);
      if (temp_val == 0) {
        val_print(ERROR, "\nITS : Could not allocate Collection Table L2 page");
        return ACS_STATUS_ERR;
      }
      lvl1_ptr[i] = temp_val;
    }
  }

  val_pe_cache_clean_range(Address, TableSize);
  dsbsy();

  write_value = val_mmio_read64(ItsBase + ARM_GITS_BASER(it));
  write_value &= ~(ARM_GITS_BASER_PA_MASK | ARM_GITS_BASER_INDIRECT |
                   ARM_GITS_BASER_PAGE_MASK | 0xFFULL);
  if (indirect_table ==  1) {
    write_value = write_value | ARM_GITS_BASER_INDIRECT;
  }
  write_value = write_value | (Address & ARM_GITS_BASER_PA_MASK);
  write_value = write_value | ARM_GITS_BASER_VALID;
  write_value = write_value | (baser_pgsz << ARM_GITS_BASER_PAGE_SHIFT);
  write_value = write_value | (Pages-1);
  write_value = write_value | ((7ULL << 59) | (7ULL << 53) | (2ULL << 10));
  val_mmio_write64(ItsBase + ARM_GITS_BASER(it), write_value);

  if (table_type == ARM_GITS_TBL_TYPE_DEVICE) {
    device_table = &g_device_tables[its_index][it];
    device_table->id_count = id_count;
    if (!indirect_table) {
      flat_table_capacity = TableSize / (entry_size + 1);
      if (flat_table_capacity < id_count)
        device_table->id_count = flat_table_capacity;
    }
    device_table->page_size = (uint32_t)max_page_size;
    if (indirect_table) {
      device_table->l1_base = (uint64_t *)Address;
      device_table->l1_entries = TableSize / sizeof(*lvl1_ptr);
      device_table->l2_entries = (uint32_t)lvl2_entries;
    }
  }

  }

  /* Allocate Memory for Interrupt Translation Table */
  Address = (uint64_t)val_aligned_alloc(SIZE_64KB, (NUM_PAGES_8 * SIZE_4KB));

  if (!Address) {
    val_print(ERROR, "\nITS : Could Not Allocate Memory For ITT. Test may not pass.");
    return 1;
  }

  val_memory_set((void *)Address, (NUM_PAGES_8*SIZE_4KB), 0);

  g_gic_its_info->GicIts[its_index].ITTBase = Address;

  return 0;
}


static void EnableITS(uint64_t GicItsBase)
{
  /* Set GITS_CTLR.Enable as 1 to enable the ITS */
  uint32_t    value;

  value = val_mmio_read(GicItsBase + ARM_GITS_CTLR);
  val_mmio_write(GicItsBase + ARM_GITS_CTLR, (value | ARM_GITS_CTLR_ENABLE));
}

static void
WriteCmdQMAPD(
   uint32_t     its_index,
   uint64_t     *CMDQ_BASE,
   uint64_t     device_id,
   uint64_t     ITT_BASE,
   uint32_t     Size,
   uint64_t     Valid
  )
{
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index]),
                     (uint64_t)((device_id << ITS_CMD_SHIFT_DEVID) | ARM_ITS_CMD_MAPD));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 1), (uint64_t)(Size));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 2),
                     (uint64_t)((Valid << ITS_CMD_SHIFT_VALID) | (ITT_BASE & ITT_PAR_MASK)));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 3), (uint64_t)(0x0));
    g_cwriter_ptr[its_index] = g_cwriter_ptr[its_index] + ITS_NEXT_CMD_PTR;
}

static void
WriteCmdQMAPC(
   uint32_t     its_index,
   uint64_t     *CMDQ_BASE,
   uint32_t     Clctn_ID,
   uint32_t     RDBase,
   uint64_t     Valid
  )
{
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index]),
                     (uint64_t)(ARM_ITS_CMD_MAPC));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 1), (uint64_t)(0x0));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 2),
                     (uint64_t)((Valid << ITS_CMD_SHIFT_VALID) | RDBase | Clctn_ID));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 3), (uint64_t)(0x0));
    g_cwriter_ptr[its_index] = g_cwriter_ptr[its_index] + ITS_NEXT_CMD_PTR;
}

static void
WriteCmdQMAPTI(
   uint32_t     its_index,
   uint64_t     *CMDQ_BASE,
   uint64_t     device_id,
   uint32_t     int_id,
   uint32_t     Clctn_ID
  )
{
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index]),
                     (uint64_t)((device_id << ITS_CMD_SHIFT_DEVID) | ARM_ITS_CMD_MAPTI));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 1),
                     ((uint64_t)(int_id-ARM_LPI_MINID) | ((uint64_t)int_id << 32)));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 2), (uint64_t)(Clctn_ID));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 3), (uint64_t)(0));
    g_cwriter_ptr[its_index] = g_cwriter_ptr[its_index] + ITS_NEXT_CMD_PTR;
}

static void
WriteCmdQINV(
   uint32_t     its_index,
   uint64_t     *CMDQ_BASE,
   uint64_t     device_id,
   uint32_t     int_id
  )
{
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index]),
                     (uint64_t)((device_id << ITS_CMD_SHIFT_DEVID) | ARM_ITS_CMD_INV));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 1),
                     (uint64_t)(int_id-ARM_LPI_MINID));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 2), (uint64_t)(0x0));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 3), (uint64_t)(0x0));
    g_cwriter_ptr[its_index] = g_cwriter_ptr[its_index] + ITS_NEXT_CMD_PTR;
}

static void
WriteCmdQDISCARD(
   uint32_t     its_index,
   uint64_t     *CMDQ_BASE,
   uint64_t     device_id,
   uint32_t     int_id
  )
{
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index]),
                     (uint64_t)((device_id << ITS_CMD_SHIFT_DEVID) | ARM_ITS_CMD_DISCARD));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 1),
                     (uint64_t)(int_id-ARM_LPI_MINID));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 2), (uint64_t)(0x0));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 3), (uint64_t)(0x0));
    g_cwriter_ptr[its_index] = g_cwriter_ptr[its_index] + ITS_NEXT_CMD_PTR;
}


static void
WriteCmdQSYNC(
   uint32_t     its_index,
   uint64_t     *CMDQ_BASE,
   uint32_t     RDBase
  )
{
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index]),
                     (uint64_t)(ARM_ITS_CMD_SYNC));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 1), (uint64_t)(0x0));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 2), (uint64_t)(RDBase));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 3), (uint64_t)(0x0));
    g_cwriter_ptr[its_index] = g_cwriter_ptr[its_index] + ITS_NEXT_CMD_PTR;
}

static void PollTillCommandQueueDone(uint32_t its_index)
{
  uint32_t    count;
  uint64_t    creadr_value;
  uint64_t    stall_value;
  uint64_t    cwriter_value;
  uint64_t    ItsBase;

  count = 0;
  ItsBase = g_gic_its_info->GicIts[its_index].Base;
  cwriter_value = val_mmio_read64(ItsBase + ARM_GITS_CWRITER);
  creadr_value = val_mmio_read64(ItsBase + ARM_GITS_CREADR);

  while (creadr_value != cwriter_value) {
    /* Check Stall Value */
    stall_value = creadr_value & ARM_GITS_CREADR_STALL;

    if (stall_value) {
      /* Retry */
      val_mmio_write64((ItsBase + ARM_GITS_CWRITER),
                  (cwriter_value | ARM_GITS_CWRITER_RETRY)
                 );
    }

    count++;
    if (count > WAIT_ITS_COMMAND_DONE) {
      val_print(ERROR,
                "\n       ITS : Command Queue READR not moving, Test may not pass");
      break;
    }

    creadr_value = val_mmio_read64(ItsBase + ARM_GITS_CREADR);
  }

}

static uint64_t GetRDBaseFormat(uint32_t its_index)
{
  uint32_t    value;
  uint64_t    pe_num;
  uint64_t    ItsBase;

  ItsBase = g_gic_its_info->GicIts[its_index].Base;

  /* Check GITS_TYPER.PTA.
     If PTA = 1 then RDBase = Physical Address,
     Else RDBase = GICR_TYPER.Processor_Number
  */
  value = val_mmio_read64(ItsBase + ARM_GITS_TYPER);
  if (value & ARM_GITS_TYPER_PTA) {
    return g_gic_its_info->GicRdBase;
  } else {
    value = val_mmio_read64(g_gic_its_info->GicRdBase + ARM_GICR_TYPER);
    pe_num = (value & ARM_GICR_TYPER_PN_MASK) >> ARM_GICR_TYPER_PN_SHIFT;

    /* RDBase is made 64KB aligned */
    return (pe_num << RD_BASE_SHIFT);
  }
}


void val_its_clear_lpi_map(uint32_t its_index, uint32_t device_id, uint32_t int_id)
{
  uint64_t    value;
  uint64_t    RDBase;
  uint64_t    ItsBase;
  uint64_t    ItsCommandBase;

  if (!g_its_setup_done)
    return;

  if (ItsEnsureDeviceTable(its_index, device_id, 0) != ACS_STATUS_PASS)
    return;

  ItsBase        = g_gic_its_info->GicIts[its_index].Base;
  ItsCommandBase = g_gic_its_info->GicIts[its_index].CommandQBase;

  /* Clear Config table for LPI=int_id */
  ClearConfigTable(int_id);

  /* Get RDBase Depending on GITS_TYPER.PTA */
  RDBase = GetRDBaseFormat(its_index);

  /* Discard Mappings */
  WriteCmdQDISCARD(its_index, (uint64_t *)(ItsCommandBase), device_id, int_id);
  /* Un Map Device using MAPD */
  WriteCmdQMAPD(its_index, (uint64_t *)(ItsCommandBase), device_id,
                g_gic_its_info->GicIts[its_index].ITTBase,
                0, 0 /*InValid*/);
  /* ITS SYNC Command */
  WriteCmdQSYNC(its_index, (uint64_t *)(ItsCommandBase), RDBase);

  dsbsy();
  /* Update the CWRITER Register so that all the commands from Command queue gets executed.*/
  value = ((g_cwriter_ptr[its_index] * NUM_BYTES_IN_DW));
  val_mmio_write64((ItsBase + ARM_GITS_CWRITER), value);

  /* Check CREADR value which ensures Command Queue is processed */
  PollTillCommandQueueDone(its_index);
  dsbsy();

}

uint32_t val_its_create_lpi_map(uint32_t its_index, uint32_t device_id,
                                uint32_t int_id, uint32_t Priority)
{
  uint64_t    value;
  uint64_t    RDBase;
  uint64_t    ItsBase;
  uint64_t    ItsCommandBase;

  if (!g_its_setup_done)
    return ACS_STATUS_ERR;

  if (ItsEnsureDeviceTable(its_index, device_id, 1) != ACS_STATUS_PASS) {
    val_print(ERROR, "\nITS : Device Table unavailable for DeviceID 0x%x", device_id);
    return ACS_STATUS_ERR;
  }

  ItsBase        = g_gic_its_info->GicIts[its_index].Base;
  ItsCommandBase = g_gic_its_info->GicIts[its_index].CommandQBase;

  /* Set Config table with enable the LPI = int_id, Priority. */
  SetConfigTable(int_id, Priority);

  /* Enable Redistributor */
  EnableLPIsRD(g_gic_its_info->GicRdBase);

  /* Enable ITS */
  EnableITS(ItsBase);

  /* Get RDBase Depending on GITS_TYPER.PTA */
  RDBase = GetRDBaseFormat(its_index);

  /* Map Device using MAPD */
  WriteCmdQMAPD(its_index, (uint64_t *)(ItsCommandBase), device_id,
                g_gic_its_info->GicIts[its_index].ITTBase,
                g_gic_its_info->GicIts[its_index].IDBits, 0x1 /*Valid*/);
  /* Map Collection using MAPC */
  WriteCmdQMAPC(its_index, (uint64_t *)(ItsCommandBase),
                0x1 /*Clctn_ID*/, RDBase, 0x1 /*Valid*/);
  /* Map Interrupt using MAPI */
  WriteCmdQMAPTI(its_index, (uint64_t *)(ItsCommandBase), device_id, int_id, 0x1 /*Clctn_ID*/);
  /* Invalid Entry */
  WriteCmdQINV(its_index, (uint64_t *)(ItsCommandBase), device_id, int_id);
  /* ITS SYNC Command */
  WriteCmdQSYNC(its_index, (uint64_t *)(ItsCommandBase), RDBase);

  dsbsy();

  /* Update the CWRITER Register so that all the commands from Command queue gets executed.*/
  value = ((g_cwriter_ptr[its_index] * NUM_BYTES_IN_DW));
  val_mmio_write64((ItsBase + ARM_GITS_CWRITER), value);

  /* Check CREADR value which ensures Command Queue is processed */
  PollTillCommandQueueDone(its_index);
  dsbsy();

  return ACS_STATUS_PASS;

}


uint32_t val_its_get_max_lpi(void)
{
  uint32_t    index;
  uint32_t    min_idbits = ARM_LPI_MAX_IDBITS;

  if ((g_gic_its_info == NULL) || (g_gic_its_info->GicNumIts == 0))
    return 0;

  if (!g_its_setup_done)
    return 0;

  /* Return The Minimum IDBits supported in ITS */
  for (index = 0; index < g_gic_its_info->GicNumIts; index++)
  {
    min_idbits = (min_idbits < (g_gic_its_info->GicIts[index].IDBits + 14)) ?
                 (min_idbits) :
                 (g_gic_its_info->GicIts[index].IDBits + 14);
  }
  return ((1 << (min_idbits+1)) - 1);
}


uint64_t val_its_get_translater_addr(uint32_t its_index)
{
  return (g_gic_its_info->GicIts[its_index].Base + ARM_GITS_TRANSLATER);
}


static uint32_t
SetInitialConfiguration(
  uint32_t     its_index
  )
{
  /* Program GIC Redistributor with the Min ID bits supported. */
  uint32_t    gicd_typer_idbits, gits_typer_bits;
  uint64_t    write_value;
  uint64_t    ItsBase;

  ItsBase = g_gic_its_info->GicIts[its_index].Base;

  gicd_typer_idbits = ARM_GICD_TYPER_IDbits(val_mmio_read(
                                            g_gic_its_info->GicDBase + ARM_GICD_TYPER));
  gits_typer_bits = ARM_GITS_TYPER_IDbits(val_mmio_read64(ItsBase + ARM_GITS_TYPER));

  /* Check least bits implemented is 14 if LPIs are supported. */
  if (gicd_typer_idbits < ARM_LPI_MIN_IDBITS) {
    return 1;
  }

  write_value = val_mmio_read64(g_gic_its_info->GicRdBase + ARM_GICR_PROPBASER);
  write_value |= gicd_typer_idbits;
  g_gic_its_info->GicIts[its_index].IDBits = gits_typer_bits;

  val_mmio_write64((g_gic_its_info->GicRdBase + ARM_GICR_PROPBASER), write_value);

  return 0;
}


uint32_t val_its_init(void)
{
  uint32_t    Status;
  uint32_t    index;
  uint64_t    state_size;

  if (g_its_setup_done)
    return ACS_STATUS_PASS;
  if ((g_gic_its_info == NULL) || (g_gic_its_info->GicNumIts == 0))
    return ACS_STATUS_ERR;

  state_size = (uint64_t)g_gic_its_info->GicNumIts * sizeof(*g_device_tables);
  if (state_size > 0xFFFFFFFFULL)
    return ACS_STATUS_ERR;

  g_device_tables = val_aligned_alloc(MEM_ALIGN_4K, (uint32_t)state_size);
  if (g_device_tables == NULL) {
    val_print(ERROR, "\nITS : Could Not Allocate Device Table state");
    return ACS_STATUS_ERR;
  }
  val_memory_set(g_device_tables, (uint32_t)state_size, 0);

  g_cwriter_ptr = (uint32_t *)pal_aligned_alloc(MEM_ALIGN_4K,
                                                sizeof(uint32_t) * (g_gic_its_info->GicNumIts));

  if (g_cwriter_ptr == NULL) {
    val_print(ERROR, "\nITS : Could Not Allocate Memory CWriteR. Test may not pass.");
    val_memory_free_aligned(g_device_tables);
    g_device_tables = NULL;
    return ACS_STATUS_ERR;
  }

  for (index = 0; index < g_gic_its_info->GicNumIts; index++)
    g_cwriter_ptr[index] = 0;

  for (index = 0; index < g_gic_its_info->GicNumIts; index++)
  {
    /* Set Initial configuration */   // DONE
    Status = SetInitialConfiguration(index);
    if (Status)
      return Status;
  }

  /* Configure Redistributor For LPIs */
  Status = ArmGicRedistributorConfigurationForLPI(g_gic_its_info->GicRdBase);
  if (Status)
    return Status;

  for (index = 0; index < g_gic_its_info->GicNumIts; index++)
  {
    /* Set Command Queue Base */
    Status = ArmGicSetItsCommandQueueBase(index);
    if (Status)
      return Status;

    /* Set Up the ITS tables */
    Status = ArmGicSetItsTables(index);
    if (Status)
      return Status;
  }

  g_its_setup_done = 1;

  val_print(TRACE, "\nITS : Info Block");
  for (index = 0; index < g_gic_its_info->GicNumIts; index++)
  {
      val_print(TRACE, "\nGIC ITS Index: %x", g_gic_its_info->GicIts[index].its_index);
      val_print(TRACE, " ID: %x", g_gic_its_info->GicIts[index].ID);
      val_print(TRACE, " Base: %x", g_gic_its_info->GicIts[index].Base);
  }

  return 0;
}
