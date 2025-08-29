
/** @file
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
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

#include "pal_common_support.h"
#include "pal_pcie_enum.h"
#include "platform_image_def.h"
#include "platform_override_struct.h"
#include "platform_override_fvp.h"

/**
  Conduits for service calls (SMC vs HVC).
**/
#define CONDUIT_SMC       0
#define CONDUIT_HVC       1
#define CONDUIT_NONE     -2

extern PLATFORM_OVERRIDE_GIC_INFO_TABLE     platform_gic_cfg;
extern PLATFORM_OVERRIDE_TIMER_INFO_TABLE   platform_timer_cfg;
extern PLATFORM_OVERRIDE_IOVIRT_INFO_TABLE  platform_iovirt_cfg;
extern PLATFORM_OVERRIDE_NODE_DATA          platform_node_type;
extern PLATFORM_OVERRIDE_UART_INFO_TABLE    platform_uart_cfg;
extern PLATFORM_OVERRIDE_MEMORY_INFO_TABLE  platform_mem_cfg;
extern PCIE_INFO_TABLE                      platform_pcie_cfg;
extern WD_INFO_TABLE                        platform_wd_cfg;
extern DMA_INFO_TABLE                       platform_dma_cfg;

extern addr_t __TEXT_START__, __TEXT_END__;
#define TEXT_START    ((addr_t)&__TEXT_START__)
#define TEXT_END      ((addr_t)&__TEXT_END__)

extern addr_t __RODATA_START__, __RODATA_END__;
#define RODATA_START  ((addr_t)&__RODATA_START__)
#define RODATA_END    ((addr_t)&__RODATA_END__)

extern addr_t __DATA_START__, __DATA_END__;
#define DATA_START    ((addr_t)&__DATA_START__)
#define DATA_END      ((addr_t)&__DATA_END__)

extern addr_t __BSS_START__, __BSS_END__;
#define BSS_START  ((addr_t)&__BSS_START__)
#define BSS_END    ((addr_t)&__BSS_END__)

#define MAX_MMAP_REGION_COUNT 75
memory_region_descriptor_t mmap_region_list[MAX_MMAP_REGION_COUNT];


/** GIC PAL API's **/
/**
  @brief  Indicate that processing of interrupt is complete by writing to
          End of interrupt register in the GIC CPU Interface

  @param  int_id  Interrupt ID which needs to be acknowledged that it is complete

  @return Status of the operation
**/
uint32_t
pal_gic_set_intr_trigger(uint32_t int_id, INTR_TRIGGER_INFO_TYPE_e trigger_type)
{
  (void) int_id;
  (void) trigger_type;
  return 0;
}

// Note that while creating a list of mem map, the size of mappings will vary across platforms.
// The below mapping size is specific to FVP RDV3.


static uint64_t device_mem_region_attr       = ATTR_DEVICE_RW;
static uint64_t map_length                   = MEM_SIZE_64K;

static uint64_t image_size                = PLATFORM_NORMAL_WORLD_IMAGE_SIZE;
static uint64_t image_base                = PLATFORM_NORMAL_WORLD_IMAGE_BASE;
static uint64_t mem_pool_size             = PLATFORM_MEMORY_POOL_SIZE;

static uint32_t mmap_list_curr_index;
static uint32_t is_mem_pool_mapped;
static uint32_t is_uart_region_mapped;
static uint32_t is_watchdog_region_mapped;
static uint32_t is_timer_region_mapped;
static uint32_t is_gic_region_mapped;
static uint32_t is_smmu_region_mapped;
static uint32_t is_pcie_region_mapped;
static uint32_t is_platform_mem_mapped;

static
void map_text_region(void)
{
    mmap_region_list[mmap_list_curr_index].virtual_address  = TEXT_START;
    mmap_region_list[mmap_list_curr_index].physical_address = TEXT_START;
    mmap_region_list[mmap_list_curr_index].length           = TEXT_END - TEXT_START;
    mmap_region_list[mmap_list_curr_index].attributes       = ATTR_CODE;
    mmap_list_curr_index++;
}

static
void map_rodata_region(void)
{
    mmap_region_list[mmap_list_curr_index].virtual_address  = RODATA_START;
    mmap_region_list[mmap_list_curr_index].physical_address = RODATA_START;
    mmap_region_list[mmap_list_curr_index].length           = RODATA_END - RODATA_START;
    mmap_region_list[mmap_list_curr_index].attributes       = ATTR_RO_DATA;
    mmap_list_curr_index++;
}

static
void map_data_region(void)
{
    mmap_region_list[mmap_list_curr_index].virtual_address  = DATA_START;
    mmap_region_list[mmap_list_curr_index].physical_address = DATA_START;
    mmap_region_list[mmap_list_curr_index].length           = DATA_END - DATA_START;
    mmap_region_list[mmap_list_curr_index].attributes       = ATTR_RW_DATA;
    mmap_list_curr_index++;
}

static
void map_bss_region(void)
{
    mmap_region_list[mmap_list_curr_index].virtual_address  = BSS_START;
    mmap_region_list[mmap_list_curr_index].physical_address = BSS_START;
    mmap_region_list[mmap_list_curr_index].length           = BSS_END - BSS_START;
    mmap_region_list[mmap_list_curr_index].attributes       = ATTR_RW_DATA;
    mmap_list_curr_index++;
}

static
void map_mem_pool_region(void)
{
    mmap_region_list[mmap_list_curr_index].virtual_address  = image_base + image_size;
    mmap_region_list[mmap_list_curr_index].physical_address = image_base + image_size;
    mmap_region_list[mmap_list_curr_index].length           = mem_pool_size;
    mmap_region_list[mmap_list_curr_index].attributes       = ATTR_RW_DATA;
    mmap_list_curr_index++;
}

static
void map_uart_device_region(uint64_t length, uint64_t attr)
{
    uint64_t uart_base;
    uart_base = platform_uart_cfg.BaseAddress.Address;
    mmap_region_list[mmap_list_curr_index].virtual_address  = uart_base;
    mmap_region_list[mmap_list_curr_index].physical_address = uart_base;
    mmap_region_list[mmap_list_curr_index].length           = length;
    mmap_region_list[mmap_list_curr_index].attributes       = attr;
    mmap_list_curr_index++;
}

static
void map_watchdog_device_region(uint64_t num_wd, uint64_t length, uint64_t attr)
{
    uint8_t i;
    uint64_t wd_ctrl_base, wd_refresh_base;
    for (i = 0; i < num_wd; i++)
    {
        wd_ctrl_base    = platform_wd_cfg.wd_info[i].wd_ctrl_base;
        mmap_region_list[mmap_list_curr_index].virtual_address  = wd_ctrl_base;
        mmap_region_list[mmap_list_curr_index].physical_address = wd_ctrl_base;
        mmap_region_list[mmap_list_curr_index].length           = length;
        mmap_region_list[mmap_list_curr_index].attributes       = attr;
        mmap_list_curr_index++;

        wd_refresh_base = platform_wd_cfg.wd_info[i].wd_refresh_base;
        mmap_region_list[mmap_list_curr_index].virtual_address  = wd_refresh_base;
        mmap_region_list[mmap_list_curr_index].physical_address = wd_refresh_base;
        mmap_region_list[mmap_list_curr_index].length           = length;
        mmap_region_list[mmap_list_curr_index].attributes       = attr;
        mmap_list_curr_index++;
    }
}

static
void map_timer_device_region(uint64_t num_timer, uint64_t length, uint64_t attr)
{
    uint8_t i = 0;
    uint64_t gen_timer_base;
    uint64_t block_cntl_base;

    block_cntl_base = platform_timer_cfg.gt_info.block_cntl_base;
    mmap_region_list[mmap_list_curr_index].virtual_address  = block_cntl_base;
    mmap_region_list[mmap_list_curr_index].physical_address = block_cntl_base;
    mmap_region_list[mmap_list_curr_index].length           = length;
    mmap_region_list[mmap_list_curr_index].attributes       = attr;
    mmap_list_curr_index++;

    for (i = 0; i < num_timer; i++)
    {
        gen_timer_base    = platform_timer_cfg.gt_info.GtCntBase[i];
        mmap_region_list[mmap_list_curr_index].virtual_address  = gen_timer_base;
        mmap_region_list[mmap_list_curr_index].physical_address = gen_timer_base;
        mmap_region_list[mmap_list_curr_index].length           = length;
        mmap_region_list[mmap_list_curr_index].attributes       = attr;
        mmap_list_curr_index++;
    }
}

static
void map_gic_device_region(uint32_t gicc_count, uint32_t gicd_count,
                           uint32_t gicc_rd_count, uint32_t gicr_rd_count,
                           uint32_t gich_count, uint32_t gic_its_count,
                           uint64_t length, uint64_t attr)
{
    uint8_t i;
    uint64_t base_address;
    for (i = 0; i < gicc_count; i++)
    {
        base_address = platform_gic_cfg.gicc_base[i];
        mmap_region_list[mmap_list_curr_index].virtual_address  = base_address;
        mmap_region_list[mmap_list_curr_index].physical_address = base_address;
        mmap_region_list[mmap_list_curr_index].length           = 0x4 * length;//0x40000 map size
        mmap_region_list[mmap_list_curr_index].attributes       = attr;
        mmap_list_curr_index++;
    }

    for (i = 0; i < gicd_count; i++)
    {
        base_address = platform_gic_cfg.gicd_base[i];
        mmap_region_list[mmap_list_curr_index].virtual_address  = base_address;
        mmap_region_list[mmap_list_curr_index].physical_address = base_address;
        mmap_region_list[mmap_list_curr_index].length           = 20 * length;
        mmap_region_list[mmap_list_curr_index].attributes       = attr;
        mmap_list_curr_index++;
    }

    for (i = 0; i < gicc_rd_count; i++)
    {
        base_address = platform_gic_cfg.gicc_rd_base[i];
        mmap_region_list[mmap_list_curr_index].virtual_address  = base_address;
        mmap_region_list[mmap_list_curr_index].physical_address = base_address;
        mmap_region_list[mmap_list_curr_index].length           = 0x4 * length;
        mmap_region_list[mmap_list_curr_index].attributes       = attr;
        mmap_list_curr_index++;
    }

    for (i = 0; i < gicr_rd_count; i++)
    {
        base_address = platform_gic_cfg.gicr_rd_base[i];
        mmap_region_list[mmap_list_curr_index].virtual_address  = base_address;
        mmap_region_list[mmap_list_curr_index].physical_address = base_address;
        mmap_region_list[mmap_list_curr_index].length           = 0x4 * length;
        mmap_region_list[mmap_list_curr_index].attributes       = attr;
        mmap_list_curr_index++;
    }

    for (i = 0; i < gich_count; i++)
    {
        base_address = platform_gic_cfg.gich_base[i];
        mmap_region_list[mmap_list_curr_index].virtual_address  = base_address;
        mmap_region_list[mmap_list_curr_index].physical_address = base_address;
        mmap_region_list[mmap_list_curr_index].length           = 0x4 * length;//0x40000 map length
        mmap_region_list[mmap_list_curr_index].attributes       = attr;
        mmap_list_curr_index++;
    }

    for (i = 0; i < gic_its_count; i++)
    {
        base_address = platform_gic_cfg.gicits_base[i];
        mmap_region_list[mmap_list_curr_index].virtual_address  = base_address;
        mmap_region_list[mmap_list_curr_index].physical_address = base_address;
        mmap_region_list[mmap_list_curr_index].length           = 0x2 * length;//0x20000 map length
        mmap_region_list[mmap_list_curr_index].attributes       = attr;
        mmap_list_curr_index++;
    }
}

static
void map_smmu_device_region(uint64_t num_smmu, uint64_t length, uint64_t attr)
{
    uint8_t i = 0;
    uint64_t smmu_base;

    for (i = 0; i < num_smmu; i++)
    {
        smmu_base    = platform_node_type.smmu[i].base;
        mmap_region_list[mmap_list_curr_index].virtual_address  = smmu_base;
        mmap_region_list[mmap_list_curr_index].physical_address = smmu_base;
        mmap_region_list[mmap_list_curr_index].length           = 0x5 * length;//0x50000 map length
        mmap_region_list[mmap_list_curr_index].attributes       = attr;
        mmap_list_curr_index++;
    }
}

static
void map_pcie_ecam_bar_region(uint32_t num_ecam, uint64_t length, uint64_t attr)
{
    uint8_t i = 0;
    uint64_t ecam_base;

    // Map ECAM region to memory
    for (i = 0; i < num_ecam; i++)
    {
        ecam_base    = platform_pcie_cfg.block[i].ecam_base;
        mmap_region_list[mmap_list_curr_index].virtual_address  = ecam_base;
        mmap_region_list[mmap_list_curr_index].physical_address = ecam_base;
        mmap_region_list[mmap_list_curr_index].length           = 0x1000 * length;
        mmap_region_list[mmap_list_curr_index].attributes       = attr;
        mmap_list_curr_index++;
    }

    // Map BAR region to memory only if ECAM is present
    if (num_ecam)
    {
        mmap_region_list[mmap_list_curr_index].virtual_address  =
                                                            PLATFORM_OVERRIDE_PCIE_ECAM0_EP_BAR64;
        mmap_region_list[mmap_list_curr_index].physical_address =
                                                            PLATFORM_OVERRIDE_PCIE_ECAM0_EP_BAR64;
        mmap_region_list[mmap_list_curr_index].length           = 0x10 * length;
        mmap_region_list[mmap_list_curr_index].attributes       = attr;
        mmap_list_curr_index++;

        mmap_region_list[mmap_list_curr_index].virtual_address  =
                                                            PLATFORM_OVERRIDE_PCIE_ECAM0_RP_BAR64;
        mmap_region_list[mmap_list_curr_index].physical_address =
                                                            PLATFORM_OVERRIDE_PCIE_ECAM0_RP_BAR64;
        mmap_region_list[mmap_list_curr_index].length           = 0x10 * length;
        mmap_region_list[mmap_list_curr_index].attributes       = attr;
        mmap_list_curr_index++;

        mmap_region_list[mmap_list_curr_index].virtual_address  =
                                                            PLATFORM_OVERRIDE_PCIE_ECAM0_EP_NPBAR32;
        mmap_region_list[mmap_list_curr_index].physical_address =
                                                            PLATFORM_OVERRIDE_PCIE_ECAM0_EP_NPBAR32;
        mmap_region_list[mmap_list_curr_index].length           = 0x60 * length;
        mmap_region_list[mmap_list_curr_index].attributes       = attr;
        mmap_list_curr_index++;

        mmap_region_list[mmap_list_curr_index].virtual_address  =
                                                            PLATFORM_OVERRIDE_PCIE_ECAM0_EP_PBAR32;
        mmap_region_list[mmap_list_curr_index].physical_address =
                                                            PLATFORM_OVERRIDE_PCIE_ECAM0_EP_PBAR32;
        mmap_region_list[mmap_list_curr_index].length           = 0x100 * length;
        mmap_region_list[mmap_list_curr_index].attributes       = attr;
        mmap_list_curr_index++;

        mmap_region_list[mmap_list_curr_index].virtual_address  =
                                                            PLATFORM_OVERRIDE_PCIE_ECAM0_RP_BAR32;
        mmap_region_list[mmap_list_curr_index].physical_address =
                                                            PLATFORM_OVERRIDE_PCIE_ECAM0_RP_BAR32;
        mmap_region_list[mmap_list_curr_index].length           = 0x20 * length;
        mmap_region_list[mmap_list_curr_index].attributes       = attr;
        mmap_list_curr_index++;
    }
}

uint64_t get_mem_attr(uint32_t mem_type)
{
    uint64_t mem_attr;
    switch (mem_type)
    {
        case(MEMORY_TYPE_DEVICE):
            mem_attr = ATTR_DEVICE_RW;
            break;
        case(MEMORY_TYPE_NORMAL):
            mem_attr = ATTR_RW_DATA;
            break;
        case(MEMORY_TYPE_NOT_POPULATED):
            mem_attr = ATTR_USER_RW | ATTR_AF | ATTR_NS; // Do not categorize mem as Dev or Normal
            break;
        case(MEMORY_TYPE_RESERVED):
            mem_attr = ATTR_PRIV_RO | ATTR_AF | ATTR_NS;
            break;
    }
    return mem_attr;
}

static
void map_system_mem_region(uint64_t num_regions)
{
    uint8_t i;
    uint64_t mem_phy_base, mem_virt_base, mem_size, mem_attr;
    uint32_t mem_type;

    for (i = 0; i < num_regions; i++)
    {
        mem_phy_base    = platform_mem_cfg.info[i].phy_addr;
        mem_virt_base   = platform_mem_cfg.info[i].virt_addr;
        mem_size        = platform_mem_cfg.info[i].size;
        mem_type        = platform_mem_cfg.info[i].type;
        mem_attr        = get_mem_attr(mem_type);

        if (mem_type == MEMORY_TYPE_RESERVED)
            continue;

        mmap_region_list[mmap_list_curr_index].virtual_address  = mem_phy_base;
        mmap_region_list[mmap_list_curr_index].physical_address = mem_virt_base;
        mmap_region_list[mmap_list_curr_index].length           = mem_size;
        mmap_region_list[mmap_list_curr_index].attributes       = mem_attr;
        mmap_list_curr_index++;
    }
}

void pal_mmu_add_mmap(void)
{
    uint32_t wd_count, timer_count;
    uint32_t smmu_count, ecam_count, sys_mem_count;
    uint32_t gicc_count, gicd_count, gicc_rd_count, gicr_rd_count, gich_count, gic_its_count;
    mmap_list_curr_index = 0;
    is_mem_pool_mapped        = 0;
    is_uart_region_mapped     = 0;
    is_watchdog_region_mapped = 0;
    is_timer_region_mapped    = 0;
    is_gic_region_mapped      = 0;
    is_smmu_region_mapped     = 0;
    is_pcie_region_mapped     = 0;
    is_platform_mem_mapped    = 0;

    /* Map Image regions */
    // Text region - Get the start and end addressed from linker script and map with ATTR_CODE
    map_text_region();

    // RODATA region - Get the start and end addressed from linker script and map with ATTR_DATA_RO
    map_rodata_region();

    // Data region - Get the start and end addressed from linker script and map with ATTR_DATA_RW
    map_data_region();

    // BSS region - Get the start and end addressed from linker script and map with ATTR_DATA_RW
    map_bss_region();

    // Mem Pool region - Get the start and end addressed from platform_override.h
    // and map with ATTR_DATA_RW
    map_mem_pool_region();

    // Map UART device region
    map_uart_device_region(map_length, device_mem_region_attr);

    // Iterate through number of WD and map the device region
    wd_count = platform_wd_cfg.header.num_wd;
    map_watchdog_device_region(wd_count, map_length, device_mem_region_attr);

    // Iterate through number of System timers and map the device region
    timer_count = platform_timer_cfg.gt_info.timer_count;
    map_timer_device_region(timer_count, map_length, device_mem_region_attr);

    // Map GIC device region to memory. This includes GICC, GICD, GICRD, GICH and GIC_ITS
    gicc_count    = platform_gic_cfg.num_gicc;
    gicd_count    = platform_gic_cfg.num_gicd;
    gicc_rd_count = platform_gic_cfg.num_gicc_rd;
    gicr_rd_count = platform_gic_cfg.num_gicr_rd;
    gich_count    = platform_gic_cfg.num_gich;
    gic_its_count = platform_gic_cfg.num_gicits;
    map_gic_device_region(gicc_count, gicd_count, gicc_rd_count, gicr_rd_count, gich_count,
                                gic_its_count, map_length, device_mem_region_attr);

    // Map SMMU device region to memory. Iterate through number of SMMUs
    smmu_count = IOVIRT_SMMUV3_COUNT;
    map_smmu_device_region(smmu_count, map_length, device_mem_region_attr);

    // Map PCIe ECAM region and BAR regions to memory.
    ecam_count = platform_pcie_cfg.num_entries;
    map_pcie_ecam_bar_region(ecam_count, map_length, device_mem_region_attr);

    // Map System Mem region. Address and attributes to get from override.h file.
    sys_mem_count = platform_mem_cfg.count;
    map_system_mem_region(sys_mem_count);
}

void *pal_mmu_get_mmap_list(void)
{
    return (memory_region_descriptor_t *)mmap_region_list;
}

uint32_t pal_mmu_get_mapping_count(void)
{
    return mmap_list_curr_index;
}


/** PE PAL API's **/

/* Populate phy_mpid_array with mpidr value of CPUs available
 * in the system. */
static const uint64_t phy_mpidr_array[PLATFORM_OVERRIDE_PE_CNT] = {
    PLATFORM_OVERRIDE_PE0_MPIDR,
#if (PLATFORM_OVERRIDE_PE_CNT > 1)
    PLATFORM_OVERRIDE_PE1_MPIDR,
#endif
#if (PLATFORM_OVERRIDE_PE_CNT > 2)
    PLATFORM_OVERRIDE_PE2_MPIDR,
#endif
#if (PLATFORM_OVERRIDE_PE_CNT > 3)
    PLATFORM_OVERRIDE_PE3_MPIDR,
#endif
#if (PLATFORM_OVERRIDE_PE_CNT > 4)
    PLATFORM_OVERRIDE_PE4_MPIDR,
#endif
#if (PLATFORM_OVERRIDE_PE_CNT > 5)
    PLATFORM_OVERRIDE_PE5_MPIDR,
#endif
#if (PLATFORM_OVERRIDE_PE_CNT > 6)
    PLATFORM_OVERRIDE_PE6_MPIDR,
#endif
#if (PLATFORM_OVERRIDE_PE_CNT > 7)
    PLATFORM_OVERRIDE_PE7_MPIDR,
#endif
#if (PLATFORM_OVERRIDE_PE_CNT > 8)
    PLATFORM_OVERRIDE_PE8_MPIDR,
#endif
#if (PLATFORM_OVERRIDE_PE_CNT > 9)
    PLATFORM_OVERRIDE_PE9_MPIDR,
#endif
#if (PLATFORM_OVERRIDE_PE_CNT > 10)
    PLATFORM_OVERRIDE_PE10_MPIDR,
#endif
#if (PLATFORM_OVERRIDE_PE_CNT > 11)
    PLATFORM_OVERRIDE_PE11_MPIDR,
#endif
#if (PLATFORM_OVERRIDE_PE_CNT > 12)
    PLATFORM_OVERRIDE_PE12_MPIDR,
#endif
#if (PLATFORM_OVERRIDE_PE_CNT > 13)
    PLATFORM_OVERRIDE_PE13_MPIDR,
#endif
#if (PLATFORM_OVERRIDE_PE_CNT > 14)
    PLATFORM_OVERRIDE_PE14_MPIDR,
#endif
#if (PLATFORM_OVERRIDE_PE_CNT > 15)
    PLATFORM_OVERRIDE_PE15_MPIDR,
#endif

};

uint32_t pal_get_pe_count(void)
{
    return PLATFORM_OVERRIDE_PE_CNT;
}

uint64_t *pal_get_phy_mpidr_list_base(void)
{
    return (uint64_t *)&phy_mpidr_array[0];
}

/**
  @brief  Install Exception Handler through BAREMETAL Interrupt registration

  @param  ExceptionType  - AARCH64 Exception type
  @param  esr            - Function pointer of the exception handler

  @return status of the API
**/
uint32_t
pal_pe_install_esr(uint32_t ExceptionType,  void (*esr)(uint64_t, void *))
{

  (void) ExceptionType;
  (void) esr;

  return 1;
}

/**
  @brief Update the ELR to return from exception handler to a desired address

  @param  context - exception context structure
  @param  offset - address with which ELR should be updated

  @return  None
**/
void
pal_pe_update_elr(void *context, uint64_t offset)
{
  (void) context;
  (void) offset;

}

/**
  @brief Get the Exception syndrome from Baremetal exception handler

  @param  context - exception context structure

  @return  ESR
**/
uint64_t
pal_pe_get_esr(void *context)
{
  /*TO DO - Baremetal
   * Place holder to return ESR from context saving structure
   */
  (void) context;
  return 0;
}

/**
  @brief Get the FAR from Baremetal exception handler

  @param  context - exception context structure

  @return  FAR
**/
uint64_t
pal_pe_get_far(void *context)
{
  /* TO DO - Baremetal
   * Place holder to return FAR from context saving structure
   */
  (void) context;
  return 0;
}

/**
  @brief   Check whether PSCI is implemented and,
           if so, using which conduit (HVC or SMC).

  @param

  @retval  CONDUIT_NONE:          PSCI is not implemented
  @retval  CONDUIT_SMC:           PSCI is implemented and uses SMC as
                                  the conduit.
  @retval  CONDUIT_HVC:           PSCI is implemented and uses HVC as
                                  the conduit.
**/
uint32_t
pal_psci_get_conduit(void)
{
  return CONDUIT_NONE;
}

/** GIC PAL PAI's **/

/**
  @brief  Installs an Interrupt Service Routine for int_id.
          Enable the interrupt in the GIC Distributor and GIC CPU Interface and hook
          the interrupt service routine for the Interrupt ID.

  @param  int_id  Interrupt ID which needs to be enabled and service routine installed for
  @param  isr     Function pointer of the Interrupt service routine

  @return Status of the operation
**/
uint32_t
pal_gic_install_isr(uint32_t int_id,  void (*isr)())
{
  /* This API installs the interrupt service routine for interrupt int_id.
   * For SPIs, PPIs & SGIs
   * Configure interrupt Trigger edge/level.
   * Program Interrupt routing.
   * Enable Interrupt.
   * Install isr for int_id.
   */

  (void) int_id;
  (void) isr;
  return 0;
}

/**
  @brief  Indicate that processing of interrupt is complete by writing to
          End of interrupt register in the GIC CPU Interface

  @param  int_id  Interrupt ID which needs to be acknowledged that it is complete

  @return Status of the operation
**/
uint32_t
pal_gic_end_of_interrupt(uint32_t int_id)
{

  (void) int_id;
  return 0;
}


/**
 @Registers the interrupt handler for a given IRQ.

 @param irq_num: hardware IRQ number
 @param mapped_irq_num: mapped IRQ number
 @param isr: interrupt service routine that returns the status

**/
uint32_t
pal_gic_request_irq (
  uint32_t IrqNum,
  uint32_t MappedIrqNum,
  void *Isr
  )
{
  (void) IrqNum;
  (void) MappedIrqNum;
  (void) Isr;
  return 0;
}

/**
 @Frees the registered interrupt handler for agiven IRQ.

 @param Irq_Num: hardware IRQ number
 @param MappedIrqNum: mapped IRQ number

**/
void
pal_gic_free_irq (
  uint32_t IrqNum,
  uint32_t MappedIrqNum
  )
{
  (void) IrqNum;
  (void) MappedIrqNum;
  return;
}

/** SMMU PAL PAI's **/
#define SMMU_V3_IDR1 0x4
#define SMMU_V3_IDR1_PASID_SHIFT 6
#define SMMU_V3_IDR1_PASID_MASK  0x1f

/**
  @brief   This API prepares the smmu page tables to support input PasId
  @param   SmmuBase - Physical addr of the SMMU for which PasId support is needed
  @param   PasId    - Process Address Space identifier
  @return  zero for success, one for failure
**/
uint32_t
pal_smmu_create_pasid_entry (uint64_t SmmuBase, uint32_t PasId)
{
  (void) SmmuBase;
  (void) PasId;

  return 1;
}

/**
  @brief   This API globally disables the SMMU based on input base address
  @param   SmmuBase - Physical addr of the SMMU that needs to be globally disabled
  @return  zero for success, one for failure
**/
uint32_t
pal_smmu_disable (uint64_t SmmuBase)
{
  (void) SmmuBase;

  return 0;
}


/**
  @brief   This API converts physical address to IO virtual address
  @param   SmmuBase - Physical addr of the SMMU for pa to iova conversion
  @param   Pa       - Physical address to use in conversion
  @return  zero for success, one for failure
*/
uint64_t
pal_smmu_pa2iova (uint64_t SmmuBase, uint64_t Pa)
{
  (void) SmmuBase;
  (void) Pa;

  return 0;
}

/**
  @brief   Check if input address is within the IOVA translation range for the device
  @param   port - Pointer to the DMA port
  @param   dma_addr   - The input address to be checked
  @return  Success if the input address is found in the range
**/
uint32_t pal_smmu_check_device_iova(void *port, uint64_t dma_addr)
{
  (void) port;
  (void) dma_addr;

  return 0;
}

/**
  @brief   Start monitoring an IO virtual address coming from DMA port
  @param   port - Pointer to the DMA port
  @return  None
**/
void pal_smmu_device_start_monitor_iova(void *port)
{
  (void) port;

  return;
}

/**
  @brief   Stop monitoring an IO virtual address coming from DMA port
  @param   port - Pointer to the DMA port
  @return  None
**/
void pal_smmu_device_stop_monitor_iova(void *port)
{
  (void) port;

  return;
}

/** PCIe PAL API's */

/**
  @brief   This API checks the PCIe hierarchy fo P2P support
           1. Caller       -  Test Suite
  @return  1 - P2P feature not supported 0 - P2P feature supported
**/
uint32_t
pal_pcie_p2p_support(void)
{
  // This API checks the PCIe hierarchy for P2P support as defined
  // in the PCIe platform configuration

  return 0;

}

/**
    @brief   Return if driver present for pcie device
    @param   bus        PCI bus address
    @param   dev        PCI device address
    @param   fn         PCI function number
    @return  Driver present : 0 or 1
**/
uint32_t
pal_pcie_device_driver_present(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn)
{
  (void) seg;
  (void) bus;
  (void) dev;
  (void) fn;

  return 1;

}

/**
    @brief   Create a list of MSI(X) vectors for a device

    @param   bus        PCI bus address
    @param   dev        PCI device address
    @param   fn         PCI function number
    @param   mvector    pointer to a MSI(X) list address

    @return  mvector    list of MSI(X) vectors
    @return  number of MSI(X) vectors
**/
uint32_t
pal_get_msi_vectors(uint32_t Seg, uint32_t Bus, uint32_t Dev, uint32_t Fn,
                                                                PERIPHERAL_VECTOR_LIST **MVector)
{
  (void) Seg;
  (void) Bus;
  (void) Dev;
  (void) MVector;
  (void) Fn;

  return 0;
}

/**
    @brief   Gets RP support of transaction forwarding.

    @param   bus        PCI bus address
    @param   dev        PCI device address
    @param   fn         PCI function number
    @param   seg        PCI segment number

    @return  0 if rp not involved in transaction forwarding
             1 if rp is involved in transaction forwarding
**/
uint32_t
pal_pcie_get_rp_transaction_frwd_support(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn)
{
  (void) seg;
  (void) bus;
  (void) dev;
  (void) fn;

  return 1;
}

/**
  @brief  Returns whether a PCIe Function is an on-chip peripheral or not

  @param  bdf        - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return Returns TRUE if the Function is on-chip peripheral, FALSE if it is
          not an on-chip peripheral
**/
uint32_t
pal_pcie_is_onchip_peripheral(uint32_t bdf)
{
  (void) bdf;
  return 0;
}

/**
  @brief  Returns the memory offset that can be accesed safely.
          This offset is platform-specific. It needs to
          be modified according to the requirement.

  @param  bdf      - PCIe BUS/Device/Function
  @param  mem_type - If the memory is Pre-fetchable or Non-prefetchable memory
  @return memory offset
**/
uint32_t
pal_pcie_mem_get_offset(uint32_t bdf, PCIE_MEM_TYPE_INFO_e mem_type)
{

  (void) bdf;
  (void) mem_type;
  return MEM_OFFSET_SMALL;
}

/* Peripheral PAL API's */

uint64_t
pal_memory_ioremap(void *ptr, uint32_t size, uint32_t attr)
{
  (void) size;
  (void) attr;

  return (uint64_t)ptr;
}

void
pal_memory_unmap(void *ptr)
{
  (void) ptr;

  return;
}

/**
  @brief  Platform specific code for UART initialisation

  @param   None
  @return  None
**/
void
pal_peripheral_uart_setup(void)
{
  return;
}

/** MISC PAL API's */

#define __ADDR_ALIGN_MASK(a, mask)    (((a) + (mask)) & ~(mask))
#define ADDR_ALIGN(a, b)              __ADDR_ALIGN_MASK(a, (typeof(a))(b) - 1)

void *mem_alloc(size_t alignment, size_t size);
void mem_free(void *ptr);

typedef struct {
    uint64_t base;
    uint64_t size;
} val_host_alloc_region_ts;

static uint64_t heap_base;
static uint64_t heap_top;
static uint8_t  heap_init_done;

/**
  @brief  Sends a formatted string to the output console

  @param  string  An ASCII string
  @param  data    data for the formatted output

  @return None
**/
void
pal_print(char *string, uint64_t data)
{

  print(ACS_PRINT_ERR, string, data);
  return;
}

/**
  @brief  Returns the physical address of the input virtual address.

  @param Va virtual address of the memory to be converted

  Returns the physical address.
**/
void *
pal_mem_virt_to_phys(void *Va)
{
  /* Place holder function. Need to be
   * implemented if needed in later releases
   */
  return Va;
}

/**
  @brief  Returns the virtual address of the input physical address.

  @param Pa physical address of the memory to be converted

  Returns the virtual address.
**/
void *
pal_mem_phys_to_virt (
  uint64_t Pa
  )
{
  /* Place holder function*/
  return (void *)Pa;
}


/**
  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return 0 - Success

**/
uint64_t
pal_time_delay_ms(uint64_t MicroSeconds)
{
  (void) MicroSeconds;
  return 0;
}

/**
  @brief  page size being used in current translation regime.

  @return page size being used
**/
uint32_t
pal_mem_page_size(void)
{
  return PLATFORM_PAGE_SIZE;
}

/**
  @brief  allocates contiguous numpages of size
          returned by pal_mem_page_size()

  @return Start address of base page
**/
void *
pal_mem_alloc_pages(uint32_t NumPages)
{
  return (void *)mem_alloc(MEM_ALIGN_4K, NumPages * PLATFORM_PAGE_SIZE);
}

/**
  @brief  frees continguous numpages starting from page
          at address PageBase

**/
void
pal_mem_free_pages(void *PageBase, uint32_t NumPages)
{
  (void) PageBase;
  (void) NumPages;
}

/**
  @brief  Allocates memory with the given alignement.

  @param  Alignment   Specifies the alignment.
  @param  Size        Requested memory allocation size.

  @return Pointer to the allocated memory with requested alignment.
**/
void
*pal_aligned_alloc(uint32_t alignment, uint32_t size)
{
  return (void *)mem_alloc(alignment, size);
}

/**
  @brief  Free the Aligned memory allocated by UEFI Framework APIs

  @param  Buffer        the base address of the aligned memory range

  @return None
*/

void
pal_mem_free_aligned(void *Buffer)
{
    mem_free(Buffer);
    return;
}

/**
  @brief   Creates a buffer with length equal to size within the
           address range (mem_base, mem_base + mem_size)

  @param   mem_base    - Base address of the memory range
  @param   mem_size    - Size of the memory range of interest
  @param   size        - Buffer size to be created

  @return  Buffer address if SUCCESSFUL, else NULL
**/
void *
pal_mem_alloc_at_address(
  uint64_t mem_base,
  uint64_t Size
  )
{
  (void) mem_base;
  (void) Size;
  return (void *) NULL;
}

/**
  @brief  Free the memory allocated by UEFI Framework APIs
  @param  Buffer the base address of the memory range to be freed

  @return None
**/
void
pal_mem_free_at_address(uint64_t mem_base,
  uint64_t Size
)
{
  (void) mem_base;
  (void) Size;
}

/* Functions implemented below are used to allocate memory from heap. Baremetal implementation
   of memory allocation.
*/

static int is_power_of_2(uint32_t n)
{
    return n && !(n & (n - 1));
}

/**
 * @brief Allocates contiguous memory of requested size(no_of_bytes) and alignment.
 * @param alignment - alignment for the address. It must be in power of 2.
 * @param Size - Size of the region. It must not be zero.
 * @return - Returns allocated memory base address if allocation is successful.
 *           Otherwise returns NULL.
 **/
void *heap_alloc(size_t alignment, size_t size)
{
    uint64_t addr;

    addr = ADDR_ALIGN(heap_base, alignment);
    size += addr - heap_base;

    if ((heap_top - heap_base) < size)
    {
       return NULL;
    }

    heap_base += size;

    return (void *)addr;
}

/**
 * @brief  Initialisation of allocation data structure
 * @param  void
 * @return Void
 **/
void mem_alloc_init(void)
{
    heap_base = PLATFORM_HEAP_REGION_BASE;
    heap_top = PLATFORM_HEAP_REGION_BASE + PLATFORM_HEAP_REGION_SIZE;
    heap_init_done = HEAP_INITIALISED;
}

/**
 * @brief Allocates contiguous memory of requested size(no_of_bytes) and alignment.
 * @param alignment - alignment for the address. It must be in power of 2.
 * @param Size - Size of the region. It must not be zero.
 * @return - Returns allocated memory base address if allocation is successful.
 *           Otherwise returns NULL.
 **/
void *mem_alloc(size_t alignment, size_t size)
{
  void *addr = NULL;

  if (heap_init_done != HEAP_INITIALISED)
    mem_alloc_init();

  if (size <= 0)
  {
    return NULL;
  }

  if (!is_power_of_2((uint32_t)alignment))
  {
    return NULL;
  }

  size += alignment - 1;
  addr = heap_alloc(alignment, size);

  return addr;
}

/**
 * TODO: Free the memory for given memory address
 * Currently acs code is initialisazing from base for every test,
 * the regions data structure is internal and below code only setting to zero
 * not actually freeing memory.
 * If require can revisit in future.
 **/
void mem_free(void *ptr)
{
  if (!ptr)
    return;

  return;
}

/**
  @brief  Allocates memory of the requested size.

  @param  Bdf:  BDF of the requesting PCIe device
  @param  Size: size of the memory region to be allocated
  @param  Pa:   physical address of the allocated memory
**/
void *
pal_mem_alloc_cacheable(uint32_t Bdf, uint32_t Size, void **Pa)
{

  void *address;
  uint32_t alignment = 0x08;
  (void) Bdf;
  address = (void *)mem_alloc(alignment, Size);
  *Pa = (void *)address;
  return (void *)address;
}

/**
  @brief  Frees the memory allocated

  @param  Bdf:  BDF of the requesting PCIe device
  @param  Size: size of the memory region to be freed
  @param  Va:   virtual address of the memory to be freed
  @param  Pa:   physical address of the memory to be freed
**/
void
pal_mem_free_cacheable(uint32_t Bdf, uint32_t Size, void *Va, void *Pa)
{

  (void) Bdf;
  (void) Size;
  (void) Va;
  (void) Pa;

}

/** DMA PAL PAI's **/
#define MEM_ALIGN_4K       0x1000

/**
  @brief   Allocate DMAable memory: (Aligned to 4K by default)

  @param   buffer - Pointer to return the buffer address
  @param   length - Number of bytes to allocate
  @param   dev    - Pointer to the device structure
  @param   flag   - Allocation flags

  @return  DMA address of memory allocated
**/
uint64_t
pal_dma_mem_alloc(void **buffer, uint32_t length, void *dev, uint32_t flag)
{

  (void) dev;
  (void) flag;
  *buffer = (void *)pal_aligned_alloc(MEM_ALIGN_4K, length);

  return 0;
}

/**
  @brief   Free the memory allocated by pal_dma_mem_alloc.

  @param  buffer: memory mapped to the DMA that is tobe freed
  @param  mem_dma: DMA address with respect to device
  @param  length: size of the memory
  @param  port: ATA port structure
  @param  flags: Allocation flags
**/
void pal_dma_mem_free(void *buffer, uint64_t mem_dma, unsigned int length, void *port,
                                                                                unsigned int flags)
{

  (void) mem_dma;
  (void) length;
  (void) port;
  (void) flags;

  pal_mem_free_aligned(buffer);
  return;
}

/**
  @brief  Abstracts the functionality of performing a DMA
          operation from the device to DDR memory

  @param  dma_target_buf is the target physical addressing
          the memory where the DMA data is to bewritten.
  @return 0 on success.
          IMPLEMENTATION DEFINED on error.
**/
unsigned int
pal_dma_start_from_device(void *dma_target_buf, unsigned int length,
                          void *host, void *dev)
{
  (void) dma_target_buf;
  (void) length;
  (void) host;
  (void) dev;

  return 0;
}

/**
  @brief   Get the DMA address used by the queried DMA controller port

  @param   port : DMA port information
  @param   dma_address : Pointer where the DMA address needs to be returned
  @param   dma_len : Length of the DMA address mapping

  @return  None
**/
void
pal_dma_scsi_get_dma_addr(void *port, void *dma_addr, unsigned int *dma_len)
{

    /* *dma_addr = dma_address;
     * *dma_len  = dma_length;
    */
  (void) dma_addr;
  (void) port;
  (void) dma_len;

}

/**
  @brief   Get atributes of DMA memory

  @param   buf   - Pointer to return the buffer
  @param   attr  - Variable to return the attributes
  @param   sh    - Inner sharable domain or not

  @return  0 on SUCCESS or 1 for FAIL
**/
int
pal_dma_mem_get_attrs(void *buf, uint32_t *attr, uint32_t *sh)
{

  /* Pointer to return: the attributes (attr)
   * and shareable domain (sh)
  */
  (void) buf;
  (void) attr;
  (void) sh;

  return 1;
}
