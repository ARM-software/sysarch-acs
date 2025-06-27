/** @file
 * Copyright (c) 2016-2025, Arm Limited or its affiliates. All rights reserved.
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

#ifndef __VAL_INTERFACE_H__
#define __VAL_INTERFACE_H__

#include "pal_interface.h"
#include "acs_drtm.h"
#include "acs_pfdi.h"

/* set G_PRINT_LEVEL to one of the below values in your application entry
  to control the verbosity of the prints */
#define ACS_PRINT_ERR   5      /* Only Errors. use this to de-clutter the terminal and focus only on specifics */
#define ACS_PRINT_WARN  4      /* Only warnings & errors. use this to de-clutter the terminal and focus only on specifics */
#define ACS_PRINT_TEST  3      /* Test description and result descriptions. THIS is DEFAULT */
#define ACS_PRINT_DEBUG 2      /* For Debug statements. contains register dumps etc */
#define ACS_PRINT_INFO  1      /* Print all statements. Do not use unless really needed */


#define ACS_STATUS_FAIL      0x90000000
#define ACS_STATUS_ERR       0xEDCB1234  //some impropable value?
#define ACS_STATUS_SKIP      0x10000000
#define ACS_STATUS_PASS      0x0
#define ACS_STATUS_NIST_PASS 0x1
#define ACS_INVALID_INDEX    0xFFFFFFFF

#define NOT_IMPLEMENTED         0x4B1D  /* Feature or API not implemented */

#define VAL_EXTRACT_BITS(data, start, end) ((data >> start) & ((1ul << (end-start+1))-1))

/* the following macros are defined by edk2 headers in case of UEFI env. Required only for BM */
#ifdef TARGET_BAREMETAL
#define BIT0  (1)
#define BIT1  (1 << 1)
#define BIT4  (1 << 4)
#define BIT6  (1 << 6)
#define BIT14 (1 << 14)
#define BIT29 (1 << 29)
#endif /* TARGET_BAREMETAL */

#define SIZE_4KB   0x00001000

#define INVALID_NAMED_COMP_INFO 0xFFFFFFFFFFFFFFFFULL

typedef char char8_t;

/* GENERIC VAL APIs */
void val_allocate_shared_mem(void);
void val_free_shared_mem(void);
void val_print(uint32_t level, char8_t *string, uint64_t data);
void val_print_raw(uint64_t uart_addr, uint32_t level, char8_t *string, uint64_t data);
void val_print_primary_pe(uint32_t level, char8_t *string, uint64_t data, uint32_t index);
void val_print_test_start(char8_t *string);
void val_print_test_end(uint32_t status, char8_t *string);
void val_set_test_data(uint32_t index, uint64_t addr, uint64_t test_data);
void val_get_test_data(uint32_t index, uint64_t *data0, uint64_t *data1);
void *val_memcpy(void *dest_buffer, void *src_buffer, uint32_t len);
void val_dump_dtb(void);
void view_print_info(uint32_t view);

uint32_t execute_tests(void);
uint32_t val_strncmp(char8_t *str1, char8_t *str2, uint32_t len);
uint64_t val_time_delay_ms(uint64_t time_ms);

/* VAL PE APIs */
typedef enum {
  PE_FEAT_MPAM,
  PE_FEAT_PMU,
  PE_FEAT_RAS
} PE_FEAT_NAME;

void     val_pe_cache_clean_invalidate_range(uint64_t start_addr, uint64_t length);
void     val_pe_cache_invalidate_range(uint64_t start_addr, uint64_t length);
void     val_pe_free_info_table(void);
void     val_execute_on_pe(uint32_t index, void (*payload)(void), uint64_t args);
void     val_smbios_create_info_table(uint64_t *smbios_info_table);
void     val_smbios_free_info_table(void);

int      val_suspend_pe(uint64_t entry, uint32_t context_id);

uint32_t val_pe_create_info_table(uint64_t *pe_info_table);
uint32_t val_pe_get_num(void);
uint32_t val_pe_get_pmu_gsiv(uint32_t index);
uint32_t val_pe_get_gmain_gsiv(uint32_t index);
uint32_t val_pe_get_gicc_trbe_interrupt(uint32_t index);
uint32_t val_pe_get_index_mpid(uint64_t mpid);
uint32_t val_pe_install_esr(uint32_t exception_type, void (*esr)(uint64_t, void *));
uint32_t val_pe_get_primary_index(void);
uint32_t val_get_pe_architecture(uint32_t index);
uint32_t val_get_num_smbios_slots(void);

uint64_t val_pe_get_mpid(void);
uint64_t val_get_primary_mpidr(void);
uint64_t val_pe_get_mpid_index(uint32_t index);

uint32_t val_bsa_pe_execute_tests(uint32_t num_pe, uint32_t *g_sw_view);

uint32_t val_pe_get_index_uid(uint32_t uid);
uint32_t val_pe_get_uid(uint64_t mpidr);
uint32_t val_pe_feat_check(PE_FEAT_NAME pe_feature);

uint32_t val_get_device_path(const char *hid, char hid_path[][MAX_NAMED_COMP_LENGTH]);
uint32_t val_smmu_is_etr_behind_catu(char *etr_path);

/* GIC VAL APIs */
uint32_t    val_gic_create_info_table(uint64_t *gic_info_table);

typedef enum {
  GIC_INFO_VERSION=1,
  GIC_INFO_SEC_STATES,
  GIC_INFO_AFFINITY_NS,
  GIC_INFO_ENABLE_GROUP1NS,
  GIC_INFO_SGI_NON_SECURE,
  GIC_INFO_SGI_NON_SECURE_LEGACY,
  GIC_INFO_DIST_BASE,
  GIC_INFO_CITF_BASE,
  GIC_INFO_NUM_RDIST,
  GIC_INFO_RDIST_BASE,
  GIC_INFO_NUM_ITS,
  GIC_INFO_ITS_BASE,
  GIC_INFO_NUM_MSI_FRAME,
  GIC_INFO_NUM_GICR_GICRD
}GIC_INFO_e;

/* GICv2m APIs */
typedef enum {
  V2M_MSI_FRAME_ID = 1,
  V2M_MSI_SPI_BASE,
  V2M_MSI_SPI_NUM,
  V2M_MSI_FRAME_BASE,
  V2M_MSI_FLAGS
} V2M_MSI_INFO_e;

uint32_t val_gic_v2m_parse_info(void);
uint64_t val_gic_v2m_get_info(V2M_MSI_INFO_e type, uint32_t instance);
void     val_gic_free_info_table(void);
void     val_gic_cpuif_init(void);
void     val_gic_free_irq(uint32_t irq_num, uint32_t mapped_irq_num);
void     val_gic_free_msi(uint32_t bdf, uint32_t device_id, uint32_t its_id,
                          uint32_t int_id, uint32_t msi_index);
uint32_t val_gic_get_info(GIC_INFO_e type);
uint32_t val_gic_install_isr(uint32_t int_id, void (*isr)(void));
uint32_t val_gic_end_of_interrupt(uint32_t int_id);
uint32_t val_gic_request_irq(uint32_t irq_num, uint32_t mapped_irq_num, void *isr);
uint32_t val_gic_get_intr_trigger_type(uint32_t int_id, INTR_TRIGGER_INFO_TYPE_e *trigger_type);
uint32_t val_gic_its_configure(void);
uint32_t val_gic_its_get_base(uint32_t its_id, uint64_t *its_base);
uint32_t val_gic_request_msi(uint32_t bdf, uint32_t device_id, uint32_t its_id,
                             uint32_t int_id, uint32_t msi_index);

uint32_t val_bsa_gic_execute_tests(uint32_t num_pe, uint32_t *g_sw_view);
uint32_t val_gic_route_interrupt_to_pe(uint32_t int_id, uint64_t mpidr);
uint32_t val_gic_get_interrupt_state(uint32_t int_id);
void val_gic_clear_interrupt(uint32_t int_id);
void val_gic_set_intr_trigger(uint32_t int_id, INTR_TRIGGER_INFO_TYPE_e trigger_type);

uint32_t val_gic_get_espi_intr_trigger_type(uint32_t int_id,
                                                          INTR_TRIGGER_INFO_TYPE_e *trigger_type);
uint32_t val_get_num_nongic_ctrl(void);

/*TIMER VAL APIs */
typedef enum {
  TIMER_INFO_CNTFREQ = 1,
  TIMER_INFO_PHY_EL1_INTID,
  TIMER_INFO_PHY_EL1_FLAGS,
  TIMER_INFO_VIR_EL1_INTID,
  TIMER_INFO_VIR_EL1_FLAGS,
  TIMER_INFO_PHY_EL2_INTID,
  TIMER_INFO_PHY_EL2_FLAGS,
  TIMER_INFO_VIR_EL2_INTID,
  TIMER_INFO_VIR_EL2_FLAGS,
  TIMER_INFO_NUM_PLATFORM_TIMERS,
  TIMER_INFO_IS_PLATFORM_TIMER_SECURE,
  TIMER_INFO_SYS_CNTL_BASE,
  TIMER_INFO_SYS_CNT_BASE_N,
  TIMER_INFO_FRAME_NUM,
  TIMER_INFO_SYS_INTID,
  TIMER_INFO_SYS_TIMER_STATUS
}TIMER_INFO_e;

#define BSA_TIMER_FLAG_ALWAYS_ON 0x4
void     val_timer_create_info_table(uint64_t *timer_info_table);
void     val_timer_free_info_table(void);
void     val_timer_set_phy_el1(uint64_t timeout);
void     val_timer_set_vir_el1(uint64_t timeout);
void     val_platform_timer_get_entry_index(uint64_t instance, uint32_t *block, uint32_t *index);
uint64_t val_timer_get_info(TIMER_INFO_e info_type, uint64_t instance);
uint64_t val_get_phy_el2_timer_count(void);
uint32_t val_bsa_timer_execute_tests(uint32_t num_pe, uint32_t *g_sw_view);
void     val_timer_set_phy_el2(uint64_t timeout);
void     val_timer_set_vir_el2(uint64_t timeout);
void     val_timer_set_system_timer(addr_t cnt_base_n, uint32_t timeout);
void     val_timer_disable_system_timer(addr_t cnt_base_n);
uint32_t val_timer_skip_if_cntbase_access_not_allowed(uint64_t index);
uint64_t val_get_phy_el1_timer_count(void);

/* Watchdog VAL APIs */
typedef enum {
  WD_INFO_COUNT = 1,
  WD_INFO_CTRL_BASE,
  WD_INFO_REFRESH_BASE,
  WD_INFO_GSIV,
  WD_INFO_ISSECURE,
  WD_INFO_IS_EDGE
}WD_INFO_TYPE_e;

void     val_wd_create_info_table(uint64_t *wd_info_table);
void     val_wd_free_info_table(void);
uint64_t val_wd_get_info(uint32_t index, WD_INFO_TYPE_e info_type);
uint32_t val_bsa_wd_execute_tests(uint32_t num_pe, uint32_t *g_sw_view);
uint32_t val_wd_set_ws0(uint32_t index, uint32_t timeout);
uint64_t val_get_counter_frequency(void);


/* PCIE VAL APIs */

/* pcie_bdf_list_t is generic structure to carry list of device BDFs */
typedef struct {
  uint32_t count;
  uint32_t dev_bdfs[];
} pcie_bdf_list_t;

void     val_pcie_enumerate(void);
void     val_pcie_create_info_table(uint64_t *pcie_info_table);
void     *val_pcie_bdf_table_ptr(void);
void     val_pcie_free_info_table(void);
void     val_pcie_disable_bme(uint32_t bdf);
void     val_pcie_enable_bme(uint32_t bdf);
void     val_pcie_disable_msa(uint32_t bdf);
void     val_pcie_enable_msa(uint32_t bdf);
void     val_pcie_clear_urd(uint32_t bdf);
void     val_pcie_enable_eru(uint32_t bdf);
void     val_pcie_disable_eru(uint32_t bdf);
void     val_pcie_get_mmio_bar(uint32_t bdf, void *base);
void     val_pcie_read_acsctrl(uint32_t arr[][1]);
void     val_pcie_write_acsctrl(uint32_t arr[][1]);
void     val_pcie_read_ext_cap_word(uint32_t bdf, uint32_t ext_cap_id, uint8_t offset,
                                    uint16_t *val);

addr_t   val_pcie_get_ecam_base(uint32_t rp_bdf);
uint32_t val_pcie_get_pcie_type(uint32_t bdf);
uint32_t val_pcie_create_device_bdf_table(void);
uint32_t val_pcie_p2p_support(void);
uint32_t val_pcie_dev_p2p_support(uint32_t bdf);
uint32_t val_pcie_is_onchip_peripheral(uint32_t bdf);
uint32_t val_pcie_device_port_type(uint32_t bdf);
uint32_t val_pcie_find_capability(uint32_t bdf, uint32_t cid_type,
                                           uint32_t cid, uint32_t *cid_offset);
uint32_t val_pcie_is_msa_enabled(uint32_t bdf);
uint32_t val_pcie_is_urd(uint32_t bdf);
uint32_t val_pcie_bitfield_check(uint32_t bdf, uint64_t *bf_entry);
uint32_t val_pcie_register_bitfields_check(uint64_t *bf_info_table, uint32_t table_size);
uint32_t val_pcie_function_header_type(uint32_t bdf);
uint32_t val_pcie_get_downstream_function(uint32_t bdf, uint32_t *dsf_bdf);
uint8_t  val_pcie_is_host_bridge(uint32_t bdf);
uint32_t val_pcie_mem_get_offset(uint32_t bdf, PCIE_MEM_TYPE_INFO_e mem_type);

uint32_t val_bsa_pcie_execute_tests(uint32_t num_pe, uint32_t *g_sw_view);
uint32_t val_pcie_is_devicedma_64bit(uint32_t bdf);
uint32_t val_pcie_device_driver_present(uint32_t bdf);
uint8_t val_pcie_parent_is_rootport(uint32_t dsf_bdf, uint32_t *rp_bdf);
void val_pcie_clear_device_status_error(uint32_t bdf);
uint32_t val_pcie_is_device_status_error(uint32_t bdf);
uint32_t val_pcie_is_sig_target_abort(uint32_t bdf);
void val_pcie_clear_sig_target_abort(uint32_t bdf);
void val_pcie_enable_ordering(uint32_t bdf);
void val_pcie_disable_ordering(uint32_t bdf);
uint32_t val_pcie_dsm_ste_tags(void);
pcie_bdf_list_t *val_pcie_get_pcie_peripheral_bdf_list(void);


/* IO-VIRT APIs */

typedef enum {
  SMMU_NUM_CTRL = 1,
  SMMU_CTRL_BASE,
  SMMU_CTRL_ARCH_MAJOR_REV,
  SMMU_IOVIRT_BLOCK,
  SMMU_SSID_BITS,
  SMMU_IN_ADDR_SIZE,
  SMMU_OUT_ADDR_SIZE
}SMMU_INFO_e;

typedef enum {
  SMMU_CAPABLE     = 1,
  SMMU_CHECK_DEVICE_IOVA,
  SMMU_START_MONITOR_DEV,
  SMMU_STOP_MONITOR_DEV,
  SMMU_CREATE_MAP,
  SMMU_UNMAP,
  SMMU_IOVA_PHYS,
  SMMU_DEV_DOMAIN,
  SMMU_GET_ATTR,
  SMMU_SET_ATTR,
}SMMU_OPS_e;

typedef enum {
  NUM_PCIE_RC = 1,
  RC_SEGMENT_NUM,
  RC_ATS_ATTRIBUTE,
  RC_MEM_ATTRIBUTE,
  RC_IOVIRT_BLOCK,
  RC_SMMU_BASE
}PCIE_RC_INFO_e;

typedef enum {
  ITS_NUM_GROUPS = 1,
  ITS_GROUP_NUM_BLOCKS,
  ITS_GET_ID_FOR_BLK_INDEX,
  ITS_GET_GRP_INDEX_FOR_ID,
  ITS_GET_BLK_INDEX_FOR_ID
} ITS_INFO_e;

void     val_iovirt_create_info_table(uint64_t *iovirt_info_table);
void     val_iovirt_free_info_table(void);
uint32_t val_iovirt_get_rc_smmu_index(uint32_t rc_seg_num, uint32_t rid);
uint64_t val_smmu_get_info(SMMU_INFO_e, uint32_t index);
uint64_t val_iovirt_get_smmu_info(SMMU_INFO_e type, uint32_t index);
uint32_t val_bsa_smmu_execute_tests(uint32_t num_pe, uint32_t *g_sw_view);

/* DMA APIs */
void     val_dma_device_get_dma_addr(uint32_t ctrl_index, uint64_t *dma_addr, uint32_t *cpu_len);
int      val_dma_mem_get_attrs(void *buf, uint32_t *attr, uint32_t *sh);


typedef enum {
    DMA_NUM_CTRL = 1,
    DMA_HOST_INFO,
    DMA_PORT_INFO,
    DMA_TARGET_INFO,
    DMA_HOST_COHERENT,
    DMA_HOST_IOMMU_ATTACHED,
    DMA_HOST_PCI
} DMA_INFO_e;

void     val_dma_create_info_table(uint64_t *dma_info_ptr);
uint64_t val_dma_get_info(DMA_INFO_e type, uint32_t index);
uint32_t val_dma_start_from_device(void *buffer, uint32_t length, uint32_t index);
uint32_t val_dma_iommu_check_iova(uint32_t ctrl_index, addr_t dma_addr, addr_t cpu_addr);


/* POWER and WAKEUP APIs */
typedef enum {
    BSA_POWER_SEM_B = 1,
    BSA_POWER_SEM_c,
    BSA_POWER_SEM_D,
    BSA_POWER_SEM_E,
    BSA_POWER_SEM_F,
    BSA_POWER_SEM_G,
    BSA_POWER_SEM_H,
    BSA_POWER_SEM_I
} BSA_POWER_SEM_e;

void     val_debug_brk(uint32_t data);
uint32_t val_power_enter_semantic(BSA_POWER_SEM_e semantic);
uint32_t val_bsa_wakeup_execute_tests(uint32_t num_pe, uint32_t *g_sw_view);

typedef enum {
    PER_FLAG_MSI_ENABLED = 0x2
}PERIPHERAL_FLAGS_e;

/* Peripheral Tests APIs */
typedef enum {
  NUM_USB,
  NUM_SATA,
  NUM_UART,
  NUM_ALL,
  USB_BASE0,
  USB_FLAGS,
  USB_GSIV,
  USB_BDF,
  USB_INTERFACE_TYPE,
  USB_PLATFORM_TYPE,
  SATA_BASE0,
  SATA_BASE1,
  SATA_FLAGS,
  SATA_GSIV,
  SATA_BDF,
  SATA_INTERFACE_TYPE,
  SATA_PLATFORM_TYPE,
  UART_BASE0,
  UART_BASE1,
  UART_WIDTH,
  UART_GSIV,
  UART_FLAGS,
  UART_BAUDRATE,
  UART_INTERFACE_TYPE,
  ANY_BASE0,
  ANY_FLAGS,
  ANY_GSIV,
  ANY_BDF,
  MAX_PASIDS
}PERIPHERAL_INFO_e;

void     val_peripheral_create_info_table(uint64_t *peripheral_info_table);
void     val_peripheral_free_info_table(void);
void     val_peripheral_dump_info(void);
uint64_t val_peripheral_get_info(PERIPHERAL_INFO_e info_type, uint32_t index);
uint32_t val_peripheral_is_pcie(uint32_t bdf);
void     val_peripheral_uart_setup(void);
uint32_t val_bsa_peripheral_execute_tests(uint32_t num_pe, uint32_t *g_sw_view);

/* Memory Tests APIs */
typedef enum {
  MEM_TYPE_DEVICE = 0x1000,
  MEM_TYPE_NORMAL,
  MEM_TYPE_RESERVED,
  MEM_TYPE_NOT_POPULATED,
  MEM_TYPE_LAST_ENTRY
} MEMORY_INFO_e;

#define MEM_ATTR_UNCACHED  0x2000
#define MEM_ATTR_CACHED    0x1000
#define MEM_ALIGN_4K       0x1000
#define MEM_ALIGN_8K       0x2000
#define MEM_ALIGN_16K      0x4000
#define MEM_ALIGN_32K      0x8000
#define MEM_ALIGN_64K      0x10000

/* Mem Map APIs */
void     val_mmap_add_region(uint64_t va_base, uint64_t pa_base,
                             uint64_t length, uint64_t attributes);
uint32_t val_setup_mmu(void);
uint32_t val_enable_mmu(void);

/* Identify memory type using MAIR attribute, refer to ARM ARM VMSA for details */

#define MEM_NORMAL_WB_IN_OUT(attr) (((attr & 0xcc) == 0xcc) || (((attr & 0x7) >= 5) && (((attr >> 4) & 0x7) >= 5)))
#define MEM_NORMAL_NC_IN_OUT(attr) (attr == 0x44)
#define MEM_DEVICE(attr) ((attr & 0xf0) == 0)
#define MEM_SH_INNER(sh) (sh == 0x3)

void     val_memory_create_info_table(uint64_t *memory_info_table);
void     val_memory_free_info_table(void);
uint64_t val_memory_get_info(addr_t addr, uint64_t *attr);
uint32_t val_memory_get_entry_index(uint32_t type, uint32_t instance);
uint32_t val_bsa_memory_execute_tests(uint32_t num_pe, uint32_t *g_sw_view);
uint64_t val_memory_get_unpopulated_addr(addr_t *addr, uint32_t instance);
uint64_t val_get_max_memory(void);

/* PCIe Exerciser tests */
uint32_t val_bsa_exerciser_execute_tests(uint32_t *g_sw_view);

/* NIST VAL APIs */
uint32_t val_nist_generate_rng(uint32_t *rng_buffer);

/* PMU test related APIS*/
void     val_pmu_create_info_table(uint64_t *pmu_info_table);
void     val_pmu_free_info_table(void);

/*Cache related info APIs*/
#define INVALID_CACHE_INFO 0xFFFFFFFFFFFFFFFF
#define CACHE_TABLE_EMPTY 0xFFFFFFFF
#define DEFAULT_CACHE_IDX 0xFFFFFFFF

typedef enum {
  CACHE_TYPE,
  CACHE_SIZE,
  CACHE_ID,
  CACHE_NEXT_LEVEL_IDX,
  CACHE_PRIVATE_FLAG
} CACHE_INFO_e;

void val_cache_create_info_table(uint64_t *cache_info_table);
void val_cache_free_info_table(void);
uint64_t val_cache_get_info(CACHE_INFO_e type, uint32_t cache_index);
uint32_t val_cache_get_llc_index(void);
uint32_t val_cache_get_pe_l1_cache_res(uint32_t res_index);
uint64_t val_get_primary_mpidr(void);

/* MPAM tests APIs */
#define MPAM_INVALID_INFO 0xFFFFFFFF
#define SRAT_INVALID_INFO 0xFFFFFFFF
#define HMAT_INVALID_INFO 0xFFFFFFFF

void val_mpam_create_info_table(uint64_t *mpam_info_table);
void val_mpam_free_info_table(void);

typedef enum {
  MPAM_RSRC_TYPE_PE_CACHE,
  MPAM_RSRC_TYPE_MEMORY,
  MPAM_RSRC_TYPE_SMMU,
  MPAM_RSRC_TYPE_MEM_SIDE_CACHE,
  MPAM_RSRC_TYPE_ACPI_DEVICE,
  MPAM_RSRC_TYPE_UNKNOWN = 0xFF  /* 0x05-0xFE Reserved for future use */
} MPAM_RSRC_LOCATOR_TYPE;

/* MPAM info request types*/
typedef enum {
  MPAM_MSC_RSRC_COUNT,
  MPAM_MSC_RSRC_RIS,
  MPAM_MSC_RSRC_TYPE,
  MPAM_MSC_BASE_ADDR,
  MPAM_MSC_ADDR_LEN,
  MPAM_MSC_RSRC_DESC1,
  MPAM_MSC_RSRC_DESC2,
  MPAM_MSC_OF_INTR,
  MPAM_MSC_OF_INTR_FLAGS,
  MPAM_MSC_ERR_INTR,
  MPAM_MSC_ERR_INTR_FLAGS,
  MPAM_MSC_NRDY,
  MPAM_MSC_ID,
  MPAM_MSC_INTERFACE_TYPE
} MPAM_INFO_e;

/* RAS APIs */
#define INVALID_RAS2_INFO 0xFFFFFFFFFFFFFFFFULL
#define INVALID_RAS_REG_VAL 0xDEADDEADDEADDEADULL
#define RAS2_FEATURE_TYPE_MEMORY 0x0

typedef enum {
  RAS2_NUM_MEM_BLOCK,
  RAS2_PROX_DOMAIN,
  RAS2_SCRUB_SUPPORT
} RAS2_MEM_INFO_e;

uint32_t val_ras_create_info_table(uint64_t *ras_info_table);
uint32_t val_ras_get_info(uint32_t info_type, uint32_t param1, uint64_t *ret_data);
void val_ras2_create_info_table(uint64_t *ras2_info_table);
void val_ras2_free_info_table(void);
uint64_t val_ras2_get_mem_info(RAS2_MEM_INFO_e type, uint32_t index);

/* ETE */
uint32_t val_sbsa_ete_execute_tests(uint32_t level, uint32_t num_pe);

/* HMAT APIs */
void val_hmat_create_info_table(uint64_t *hmat_info_table);
void val_hmat_free_info_table(void);

/* SRAT APIs */
typedef enum {
  SRAT_MEM_NUM_MEM_RANGE,
  SRAT_MEM_BASE_ADDR,
  SRAT_MEM_ADDR_LEN,
  SRAT_GICC_PROX_DOMAIN,
  SRAT_GICC_PROC_UID,
  SRAT_GICC_REMOTE_PROX_DOMAIN
} SRAT_INFO_e;

void val_srat_create_info_table(uint64_t *srat_info_table);
void val_srat_free_info_table(void);
uint64_t val_srat_get_info(SRAT_INFO_e type, uint64_t prox_domain);

#define PMU_INVALID_INFO 0xFFFFFFFFFFFFFFFF
#define PMU_INVALID_INDEX 0xFFFFFFFF

/* PMU info request types */
typedef enum {
  PMU_NODE_TYPE,       /* PMU Node type               */
  PMU_NODE_BASE0,      /* Page 0 Base address         */
  PMU_NODE_BASE1,      /* Page 1 Base address         */
  PMU_NODE_PRI_INST,   /* Primary instance            */
  PMU_NODE_SEC_INST,   /* Secondary instance          */
  PMU_NODE_COUNT,      /* PMU Node count              */
  PMU_NODE_DP_EXTN,    /* Dual page extension support */
  PMU_NODE_CS_COM,     /* Node is Coresight arch complaint */
} PMU_INFO_e;

uint32_t val_sbsa_pe_execute_tests(uint32_t level, uint32_t num_pe);
uint32_t val_sbsa_gic_execute_tests(uint32_t level, uint32_t num_pe);
uint32_t val_sbsa_pcie_execute_tests(uint32_t level, uint32_t num_pe);
uint32_t val_sbsa_wd_execute_tests(uint32_t level, uint32_t num_pe);
uint32_t val_sbsa_timer_execute_tests(uint32_t level, uint32_t num_pe);
uint32_t val_sbsa_memory_execute_tests(uint32_t level, uint32_t num_pe);
uint32_t val_sbsa_smmu_execute_tests(uint32_t level, uint32_t num_pe);
uint32_t val_sbsa_exerciser_execute_tests(uint32_t level);
uint32_t val_sbsa_pmu_execute_tests(uint32_t level, uint32_t num_pe);
uint32_t val_sbsa_mpam_execute_tests(uint32_t level, uint32_t num_pe);
uint32_t val_sbsa_ras_execute_tests(uint32_t level, uint32_t num_pe);
uint32_t val_sbsa_nist_execute_tests(uint32_t level, uint32_t num_pe);

uint32_t val_bsa_execute_tests(uint32_t *g_sw_view);
uint32_t val_sbsa_execute_tests(uint32_t g_sbsa_level);

/* TPM2 API */

typedef enum {
  TPM2_INFO_IS_PRESENT = 1,
  TPM2_INFO_BASE_ADDR,
  TPM2_INFO_INTERFACE_TYPE
} TPM2_INFO_e;

void val_tpm2_create_info_table(uint64_t *tpm2_info_table);
void val_tpm2_free_info_table(void);
uint64_t val_tpm2_get_info(TPM2_INFO_e info_type);
uint64_t val_tpm2_get_version(void);

/* PC-BSA Related API's */
uint32_t val_pcbsa_execute_tests(uint32_t g_pcbsa_level);
uint32_t val_pcbsa_pe_execute_tests(uint32_t level, uint32_t num_pe);
uint32_t val_pcbsa_gic_execute_tests(uint32_t level, uint32_t num_pe);
uint32_t val_pcbsa_smmu_execute_tests(uint32_t level, uint32_t num_pe);
uint32_t val_pcbsa_memory_execute_tests(uint32_t level, uint32_t num_pe);
uint32_t val_pcbsa_pcie_execute_tests(uint32_t level, uint32_t num_pe);
uint32_t val_pcbsa_wd_execute_tests(uint32_t level, uint32_t num_pe);
uint32_t val_pcbsa_tpm2_execute_tests(uint32_t level, uint32_t num_pe);

/* PCC related APIs */
void val_pcc_create_info_table(uint64_t *pcc_info_table);
void *val_pcc_cmd_response(uint32_t subspace_id, uint32_t command, void *data, uint32_t data_size);
uint32_t val_pcc_get_ss_info_idx(uint32_t subspace_id);
void val_pcc_free_info_table(void);

typedef enum {
    INTERFACE_MODULE,
    DYNAMIC_LAUNCH_MODULE
} DRTM_MODULE_ID_e;

typedef struct __attribute__((packed)) {
    /* Callee-saved registers */
    uint64_t x19;
    uint64_t x20;
    uint64_t x21;
    uint64_t x22;
    uint64_t x23;
    uint64_t x24;
    uint64_t x25;
    uint64_t x26;
    uint64_t x27;
    uint64_t x28;
    /* Frame register */
    uint64_t x29;
    /* Link register */
    uint64_t x30;
    /* Stack pointer */
    uint64_t sp;
    /* Stack Pointer when SPSel=0 */
    uint64_t sp_el0;
    /* SCTLR_EL2 */
    uint64_t sctlr_el2;
} DRTM_ACS_DL_SAVED_STATE;

typedef struct __attribute__((packed)) {
    uint64_t x0;
    uint64_t x1;
} DRTM_ACS_DL_RESULT;

extern DRTM_ACS_DL_RESULT      *g_drtm_acs_dl_result;
extern DRTM_ACS_DL_SAVED_STATE *g_drtm_acs_dl_saved_state;

int64_t val_drtm_features(uint64_t fid, uint64_t *feat1, uint64_t *feat2);
uint32_t val_drtm_get_version(void);
int64_t val_drtm_simulate_dl(DRTM_PARAMETERS *drtm_params);
int64_t val_drtm_dynamic_launch(DRTM_PARAMETERS *drtm_params);
int64_t val_drtm_close_locality(uint32_t locality);
int64_t val_drtm_unprotect_memory(void);
int64_t val_drtm_get_error(uint64_t *feat1);
int64_t val_drtm_set_tcb_hash(uint64_t tcb_hash_table_addr);
int64_t val_drtm_lock_tcb_hashes(void);
uint32_t val_drtm_reserved_bits_check_is_zero(uint32_t reserved_bits);
uint32_t val_drtm_get_psci_ver(void);
uint32_t val_drtm_get_smccc_ver(void);

uint32_t val_drtm_create_info_table(void);
int64_t val_drtm_check_dl_result(uint64_t dlme_base_addr, uint64_t dlme_data_offset);
int64_t val_drtm_init_drtm_params(DRTM_PARAMETERS *drtm_params);
uint64_t val_drtm_get_feature(uint64_t feature_type);

uint32_t val_drtm_execute_interface_tests(uint32_t num_pe);
uint32_t val_drtm_execute_dl_tests(uint32_t num_pe);

uint32_t interface001_entry(uint32_t num_pe);
uint32_t interface002_entry(uint32_t num_pe);
uint32_t interface003_entry(uint32_t num_pe);
uint32_t interface004_entry(uint32_t num_pe);
uint32_t interface005_entry(uint32_t num_pe);
uint32_t interface006_entry(uint32_t num_pe);
uint32_t interface007_entry(uint32_t num_pe);
uint32_t interface008_entry(uint32_t num_pe);
uint32_t interface009_entry(uint32_t num_pe);
uint32_t interface010_entry(uint32_t num_pe);
uint32_t interface011_entry(uint32_t num_pe);
uint32_t interface012_entry(uint32_t num_pe);
uint32_t interface013_entry(uint32_t num_pe);
uint32_t interface014_entry(uint32_t num_pe);
uint32_t interface015_entry(uint32_t num_pe);

uint32_t dl001_entry(uint32_t num_pe);
uint32_t dl002_entry(uint32_t num_pe);
uint32_t dl003_entry(uint32_t num_pe);
uint32_t dl004_entry(uint32_t num_pe);
uint32_t dl005_entry(uint32_t num_pe);
uint32_t dl006_entry(uint32_t num_pe);
uint32_t dl007_entry(uint32_t num_pe);
uint32_t dl008_entry(uint32_t num_pe);
uint32_t dl009_entry(uint32_t num_pe);
uint32_t dl010_entry(uint32_t num_pe);
uint32_t dl011_entry(uint32_t num_pe);
uint32_t dl012_entry(uint32_t num_pe);


#define ACS_MPAM_REGISTER_TEST_NUM_BASE     0
#define ACS_MPAM_CACHE_TEST_NUM_BASE        100
#define ACS_MPAM_ERROR_TEST_NUM_BASE        200
#define ACS_MPAM_MEMORY_TEST_NUM_BASE       300

#define GET_MAX_VALUE(ax, ay) (((ax) > (ay)) ? (ax) : (ay))
#define GET_MIN_VALUE(ax, ay) (((ax) > (ay)) ? (ay) : (ax))


#define SIZE_1K    1024ULL
#define SIZE_16K   4 * SIZE_4K
#define SIZE_1M    SIZE_1K * SIZE_1K
#define SIZE_1G    SIZE_1M * SIZE_1K

#define SOFTLIMIT_DIS 0x0
#define SOFTLIMIT_EN 0x1
#define HARDLIMIT_DIS 0x0
#define HARDLIMIT_EN 0x1

typedef enum {
    REGISTER_MODULE,
    CACHE_MODULE,
    ERROR_MODULE,
    MEMORY_MODULE
} MPAM_MODULE_ID_e;

// Module entry functions.
uint32_t val_mpam_execute_register_tests(uint32_t num_pe);
uint32_t val_mpam_execute_error_tests(uint32_t num_pe);
uint32_t val_mpam_execute_cache_tests(uint32_t num_pe);
uint32_t val_mpam_execute_membw_tests(uint32_t num_pe);

// VAL API prototypes
uint32_t val_mpam_msc_reset_errcode(uint32_t msc_index);
uint32_t val_mpam_msc_get_errcode(uint32_t msc_index);
void     val_mpam_msc_generate_psr_error(uint32_t msc_index);
void     val_mpam_msc_generate_msr_error(uint32_t msc_index, uint16_t mon_count);
uint32_t val_mpam_msc_generate_por_error(uint32_t msc_index);
uint32_t val_mpam_msc_generate_pmgor_error(uint32_t msc_index);
void     val_mpam_msc_generate_msmon_config_error(uint32_t msc_index, uint16_t mon_count);
void     val_mpam_msc_generate_msmon_oflow_error(uint32_t msc_index, uint16_t mon_count);
void     val_mpam_msc_trigger_intr(uint32_t msc_index);

// Register tests entry calls
uint32_t reg001_entry(void);
uint32_t reg002_entry(void);
uint32_t reg003_entry(void);

// Memory Bandwidth partitioning tests entry calls
uint32_t mem001_entry(void);
uint32_t mem002_entry(void);
uint32_t mem003_entry(void);

// Error and Interrupt tests entry calls
uint32_t error001_entry(void);
uint32_t error002_entry(void);
uint32_t error003_entry(void);
uint32_t error004_entry(void);
uint32_t error005_entry(void);
uint32_t error006_entry(void);
uint32_t error007_entry(void);
uint32_t error008_entry(void);
uint32_t error009_entry(void);
uint32_t error010_entry(void);
uint32_t error011_entry(void);
uint32_t intr001_entry(void);
uint32_t intr002_entry(void);
uint32_t intr003_entry(void);

/* Cache Tests */
uint32_t partition001_entry(void);
uint32_t partition002_entry(void);
uint32_t partition003_entry(void);

uint32_t monitor001_entry(void);
uint32_t monitor002_entry(void);
uint32_t monitor003_entry(void);
uint32_t monitor004_entry(void);

// Accessing system registers from .S -> can be moved to respective .h
uint64_t arm64_write_sp(uint64_t write_data);
uint64_t arm64_read_sp(void);


typedef enum {
    PFDI_MODULE,
} PFDI_MODULE_ID_e;

typedef struct {
  int64_t test_count;
} pfdi_pe_test_support_info;

uint32_t val_pfdi_reserved_bits_check_is_zero(uint32_t reserved_bits);
int64_t val_pfdi_version(void);
int64_t val_pfdi_features(uint32_t function_id);
int64_t val_pfdi_pe_test_id(int64_t *test_id);
int64_t val_pfdi_pe_test_part_count(void);
int64_t val_pfdi_pe_test_run(int64_t start, int64_t end, int64_t *fault_test_id);
int64_t val_pfdi_pe_test_result(int64_t *fault_test_part_id);
int64_t val_pfdi_fw_check(void);
int64_t val_pfdi_force_error(uint32_t function_id, int64_t error_value);
uint32_t val_pfdi_allocate_pe_mem(void);
void val_pfdi_free_pe_mem(void);

uint32_t val_pfdi_execute_pfdi_tests(uint32_t num_pe);

uint32_t pfdi001_entry(uint32_t num_pe);
uint32_t pfdi002_entry(uint32_t num_pe);
uint32_t pfdi003_entry(uint32_t num_pe);
uint32_t pfdi004_entry(uint32_t num_pe);

#endif
