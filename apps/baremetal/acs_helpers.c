/** @file
 * Copyright (c) 2025-2026, Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/pal_interface.h"
#include "val/include/val_interface.h"
#include "val/include/acs_pe.h"
#include "val/include/acs_val.h"
#include "val/include/acs_memory.h"
#include "val/include/acs_dma.h"
#include "acs.h"

void
createIoVirtInfoTable(
)
{
  uint64_t   *IoVirtInfoTable;
  IoVirtInfoTable = val_aligned_alloc(SIZE_4K, (sizeof(IOVIRT_INFO_TABLE)
                    + ((IOVIRT_ITS_COUNT + IOVIRT_SMMUV3_COUNT + IOVIRT_RC_COUNT
                    + IOVIRT_SMMUV2_COUNT + IOVIRT_NAMED_COMPONENT_COUNT + IOVIRT_PMCG_COUNT)
                    * sizeof(IOVIRT_BLOCK)) + (IOVIRT_MAX_NUM_MAP * sizeof(ID_MAP))));
  val_iovirt_create_info_table(IoVirtInfoTable);
}


void
createPcieInfoTable(
)
{
  uint64_t   *PcieInfoTable;
  PcieInfoTable = val_aligned_alloc(SIZE_4K, (sizeof(PCIE_INFO_TABLE)
                  + (PLATFORM_OVERRIDE_NUM_ECAM * sizeof(PCIE_INFO_BLOCK))));
  val_pcie_create_info_table(PcieInfoTable);
}

void createMemoryInfoTable(
)
{
  uint64_t   *MemoryInfoTable;
  MemoryInfoTable = val_aligned_alloc(SIZE_4K, sizeof(MEMORY_INFO_TABLE)
                    + (PLATFORM_OVERRIDE_MEMORY_ENTRY_COUNT * sizeof(MEM_INFO_BLOCK)));
  val_memory_create_info_table(MemoryInfoTable);
}

void
createSmbiosInfoTable(
)
{
    uint64_t *SmbiosInfoTable;

    SmbiosInfoTable = val_aligned_alloc(SIZE_4K, sizeof(PE_SMBIOS_PROCESSOR_INFO_TABLE) +
                            (PLATFORM_OVERRIDE_SMBIOS_SLOT_COUNT * sizeof(PE_SMBIOS_TYPE4_INFO)));

    val_smbios_create_info_table(SmbiosInfoTable);
}

uint32_t
createPeInfoTable(
)
{
    uint32_t Status;
    uint64_t *PeInfoTable;

    PeInfoTable = val_aligned_alloc(SIZE_4K, sizeof(PE_INFO_TABLE) +
                                    (PLATFORM_OVERRIDE_PE_CNT * sizeof(PE_INFO_ENTRY)));

    Status = val_pe_create_info_table(PeInfoTable);

    return Status;
}

uint32_t
createGicInfoTable(
)
{
    uint32_t Status;
    uint64_t *GicInfoTable;
    uint32_t gic_info_end_index = 1; //Additional index for mem alloc to store the end value(0xff)

    GicInfoTable = val_aligned_alloc(SIZE_4K, sizeof(GIC_INFO_TABLE)
                    + ((PLATFORM_OVERRIDE_GICITS_COUNT
                    + PLATFORM_OVERRIDE_GICC_GICRD_COUNT + PLATFORM_OVERRIDE_GICR_GICRD_COUNT
                    + PLATFORM_OVERRIDE_GICC_COUNT + PLATFORM_OVERRIDE_GICD_COUNT
                    + gic_info_end_index) * sizeof(GIC_INFO_ENTRY)));

    Status = val_gic_create_info_table(GicInfoTable);

    return Status;
}

void
createTimerInfoTable(
)
{
    uint64_t   *TimerInfoTable;

    TimerInfoTable = val_aligned_alloc(SIZE_4K, sizeof(TIMER_INFO_TABLE)
                    + (PLATFORM_OVERRIDE_TIMER_COUNT * sizeof(TIMER_INFO_GTBLOCK)));

    val_timer_create_info_table(TimerInfoTable);
}

void
createWatchdogInfoTable(
)
{
    uint64_t *WdInfoTable;

    WdInfoTable = val_aligned_alloc(SIZE_4K, sizeof(WD_INFO_TABLE)
                    + (PLATFORM_OVERRIDE_WD_TIMER_COUNT * sizeof(WD_INFO_BLOCK)));

    val_wd_create_info_table(WdInfoTable);
}

void
createPeripheralInfoTable(
)
{
    uint64_t   *PeripheralInfoTable;
    uint64_t   *MemoryInfoTable;
    uint32_t   per_info_end_index = 1; //Additional index for mem alloc to store the end value(0xff)

    PeripheralInfoTable = val_aligned_alloc(SIZE_4K, sizeof(PERIPHERAL_INFO_TABLE)
                            + ((PLATFORM_OVERRIDE_PERIPHERAL_COUNT + per_info_end_index)
                                * sizeof(PERIPHERAL_INFO_BLOCK)));
    val_peripheral_create_info_table(PeripheralInfoTable);

    MemoryInfoTable = val_aligned_alloc(SIZE_4K, sizeof(MEMORY_INFO_TABLE)
                        + (PLATFORM_OVERRIDE_MEMORY_ENTRY_COUNT * sizeof(MEM_INFO_BLOCK)));
    val_memory_create_info_table(MemoryInfoTable);
}

void
createDmaInfoTable(
)
{
    uint64_t  *DmaInfoTable;

    DmaInfoTable = val_aligned_alloc(SIZE_4K, sizeof(DMA_INFO_TABLE)
                    + (PLATFORM_OVERRIDE_DMA_CNT * sizeof(DMA_INFO_BLOCK)));
    val_dma_create_info_table(DmaInfoTable);
}

void
createPmuInfoTable(
)
{
    uint64_t   *PmuInfoTable;

    PmuInfoTable = val_aligned_alloc(SIZE_4K, sizeof(PMU_INFO_TABLE)
                    + PLATFORM_OVERRIDE_PMU_NODE_CNT * sizeof(PMU_INFO_BLOCK));
    val_pmu_create_info_table(PmuInfoTable);
}

void
createRasInfoTable(
)
{
    uint64_t   *RasInfoTable;

    RasInfoTable = val_aligned_alloc(SIZE_4K, sizeof(RAS_INFO_TABLE)
                    + (PLATFORM_OVERRIDE_NUM_PE_RAS_NODES + PLATFORM_OVERRIDE_NUM_MC_RAS_NODES)
                    * sizeof(RAS_NODE_INFO) + PLATFORM_OVERRIDE_NUM_RAS_NODES
                    * sizeof(RAS_INTERFACE_INFO)
                    + PLATFORM_OVERRIDE_NUM_RAS_NODES * sizeof(RAS_INTERRUPT_INFO));
    val_ras_create_info_table(RasInfoTable);
}

void
createCacheInfoTable(
)
{
    uint64_t   *CacheInfoTable;

    CacheInfoTable = val_aligned_alloc(SIZE_4K, sizeof(CACHE_INFO_TABLE)
                    + PLATFORM_OVERRIDE_CACHE_CNT * sizeof(CACHE_INFO_ENTRY));
    val_cache_create_info_table(CacheInfoTable);
}

void
createMpamInfoTable(
)
{
    uint64_t *MpamInfoTable;

    MpamInfoTable = val_aligned_alloc(SIZE_4K, sizeof(MPAM_INFO_TABLE)
                                        + PLATFORM_MPAM_MSC_COUNT * sizeof(MPAM_MSC_NODE)
                                        + PLATFORM_MPAM_MSC_COUNT * sizeof(MPAM_RESOURCE_NODE));
    val_mpam_create_info_table(MpamInfoTable);
}

void
createHmatInfoTable(
)
{
    uint64_t      *HmatInfoTable;

    HmatInfoTable = val_aligned_alloc(SIZE_4K, sizeof(HMAT_INFO_TABLE)
                                        + PLATFORM_OVERRIDE_HMAT_MEM_ENTRIES
                                          * sizeof(HMAT_BW_ENTRY));
    val_hmat_create_info_table(HmatInfoTable);
}

void
createSratInfoTable(
)
{
    uint64_t      *SratInfoTable;

    SratInfoTable = val_aligned_alloc(SIZE_4K,
                                        PLATFORM_OVERRIDE_NUM_SRAT_ENTRIES * sizeof(SRAT_INFO_ENTRY)
                                        + PLATFORM_OVERRIDE_MEM_AFF_CNT * sizeof(SRAT_MEM_AFF_ENTRY)
                                        + PLATFORM_OVERRIDE_GICC_AFF_CNT
                                          * sizeof(SRAT_GICC_AFF_ENTRY));
    val_srat_create_info_table(SratInfoTable);
}

void
createPccInfoTable(
)
{
    uint64_t      *PccInfoTable;

    PccInfoTable = val_aligned_alloc(SIZE_4K,
                                        PLATFORM_PCC_SUBSPACE_COUNT * sizeof(PCC_INFO));
    val_pcc_create_info_table(PccInfoTable);
}

/**
  @brief  This API allocates memory for info table and
          calls create info table function passed as parameter.

  @param  create_info_tbl_func  - function pointer to val_*_create_info_table
  @param  info_table_size       - memory size to be allocated.

  @return  None
**/

void
createInfoTable(
  void(*create_info_tbl_func)(uint64_t *),
  uint64_t info_table_size,
  char8_t *table_name
  )
{
    uint64_t      *InfoTable;

    val_print(ACS_PRINT_DEBUG, "\n Allocating memory for ", 0);
    val_print(ACS_PRINT_DEBUG, table_name, 0);
    val_print(ACS_PRINT_DEBUG, " info table", 0);

    InfoTable = val_aligned_alloc(SIZE_4K, info_table_size);


    (*create_info_tbl_func)(InfoTable);
}

void
createRas2InfoTable(
)
{
    uint64_t ras2_size = sizeof(RAS2_INFO_TABLE)
                        + PLATFORM_OVERRIDE_NUM_RAS2_BLOCK * sizeof(RAS2_BLOCK)
                        + PLATFORM_OVERRIDE_NUM_RAS2_MEM_BLOCK * sizeof(RAS2_MEM_INFO);
    createInfoTable(val_ras2_create_info_table, ras2_size, "RAS2");
}

void
createTpm2InfoTable(
)
{
  uint64_t *Tpm2InfoTable;

  Tpm2InfoTable = val_aligned_alloc(SIZE_4K, sizeof(TPM2_INFO_TABLE));
  val_tpm2_create_info_table(Tpm2InfoTable);
}
