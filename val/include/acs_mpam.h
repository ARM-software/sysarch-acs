/** @file
 * Copyright (c) 2024-2025, Arm Limited or its affiliates. All rights reserved.
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

#ifndef __ACS_MPAM_H__
#define __ACS_MPAM_H__

/* The return value is always 64-bit, type casting is needed by the caller and (m > n) */
#define CLEAR_BITS_M_TO_N(num, m, n)  (num & ((~0UL << (m+1)) | ((1UL << n) - 1)))

#define MPAM_VERSION_0_1           0x01
#define MPAM_VERSION_1_0           0x10
#define MPAM_VERSION_1_1           0x11
#define MPAM_MSC_REGISTER_SPACE    0x2FFF  // Last register access is 0x2000 + (127 * 32) + 31

/*******************************************************************************
 * MPAM system register bit definitions & constants
 ******************************************************************************/
/* MPAMn_ELx bit definitions */
#define MPAMn_ELx_PARTID_I_SHIFT    0
#define MPAMn_ELx_PARTID_D_SHIFT    16
#define MPAMn_ELx_PMG_I_SHIFT       32
#define MPAMn_ELx_PMG_D_SHIFT       40
#define MPAMn_ELx_MPAMEN_SHIFT      63
#define MPAMn_ELx_PARTID_I_MASK     0xffffULL
#define MPAMn_ELx_PARTID_D_MASK     0xffffULL
#define MPAMn_ELx_PMG_I_MASK        0xffULL
#define MPAMn_ELx_PMG_D_MASK        0xffULL
#define MPAMn_ELx_MPAMEN_MASK       0x1ULL
/* MPAMIDR_EL1 bit definitions */
#define MPAMIDR_PARTID_MAX_SHIFT    0
#define MPAMIDR_PMG_MAX_SHIFT       32
#define MPAMIDR_PARTID_MAX_MASK     0xffff
#define MPAMIDR_PMG_MAX_MASK        0xff

#define CPOR_BITMAP_DEF_VAL         0xFFFFFFFF

/* MPAM ACPI bit definitions and masks */
#define MPAM_ACPI_ERR_INTR_TYPE_MASK   0x01
#define MPAM_ACPI_ERR_INTR_TYPE_LEVEL  0x00
#define MPAM_ACPI_ERR_INTR_TYPE_EDGE   0x01

/* MPAM system registers */
typedef enum {
  MPAMIDR_EL1,
  MPAM2_EL2,
  MPAM1_EL1
} MPAM_SYS_REGS;

#define DEFAULT_PARTID 0ULL
#define DEFAULT_PARTID_MAX 65535 //(2^16 - 1)
#define DEFAULT_PMG 0ULL
#define DEFAULT_PMG_MAX 255 //(2^8 - 1)
#define MPAM_MON_NOT_READY -1

#define MAX_CPBM_WIDTH      32768
#define MAX_BWPBM_WIDTH     4096

void val_mpam_reg_write(MPAM_SYS_REGS reg_id, uint64_t write_data);
uint64_t val_mpam_reg_read(MPAM_SYS_REGS reg_id);
uint64_t AA64ReadMpamidr(void);
uint64_t AA64ReadMpam1(void);
uint64_t AA64ReadMpam2(void);
void AA64IssueDSB(void);
void AA64WriteMpam1(uint64_t write_data);
void AA64WriteMpam2(uint64_t write_data);

uint64_t val_mpam_get_info(MPAM_INFO_e type, uint32_t msc_index, uint32_t rsrc_index);
uint32_t val_mpam_msc_supports_mbwpart(uint32_t msc_index);
uint32_t val_mpam_msc_supports_mbwpbm(uint32_t msc_index);
uint32_t val_mpam_msc_supports_mbw_min(uint32_t msc_index);
uint32_t val_mpam_msc_supports_mbw_max(uint32_t msc_index);
uint32_t val_mpam_msc_supports_mon(uint32_t msc_index);
uint32_t val_mpam_msc_supports_mbwumon(uint32_t msc_index);
uint32_t val_mpam_msc_supports_partid_nrw(uint32_t msc_index);
uint32_t val_mpam_get_mbwumon_count(uint32_t msc_index);
void val_mpam_memory_configure_mbwumon(uint32_t msc_index);
void val_mpam_memory_mbwumon_enable(uint32_t msc_index);
void val_mpam_memory_mbwumon_disable(uint32_t msc_index);
void val_mpam_memory_configure_ris_sel(uint32_t msc_index, uint32_t rsrc_index);
uint32_t val_mpam_msc_supports_ext_idr(uint32_t msc_index);
uint32_t val_mpam_msc_supports_ris(uint32_t msc_index);
uint32_t val_mpam_msc_supports_extd_esr(uint32_t msc_index);
uint32_t val_mpam_msc_supports_esr(uint32_t msc_index);
uint64_t val_mpam_memory_mbwumon_read_count(uint32_t msc_index);
uint32_t val_mpam_get_msc_count(void);
uint32_t val_mpam_get_max_ris_count(uint32_t msc_index);
void val_mpam_memory_mbwumon_reset(uint32_t msc_index);
void *val_mem_alloc_at_address (uint64_t mem_base, uint64_t size);
void val_mem_free_at_address (uint64_t mem_base, uint64_t size);
uint32_t val_alloc_shared_memcpybuf(uint64_t mem_base, uint64_t buffer_size, uint32_t pe_count);
uint64_t val_get_shared_memcpybuf(uint32_t pe_index);
void val_mem_free_shared_memcpybuf(uint32_t num_pe);
uint32_t val_mpam_get_csumon_count(uint32_t msc_index);
uint32_t val_mpam_supports_csumon(uint32_t msc_index);
uint64_t val_mpam_memory_get_size(uint32_t msc_index, uint32_t rsrc_index);
uint64_t val_mpam_memory_get_base(uint32_t msc_index, uint32_t rsrc_index);
uint32_t val_mpam_supports_cpor(uint32_t msc_index);
uint32_t val_mpam_supports_ccap(uint32_t msc_index);
uint32_t val_mpam_get_cmax_wd(uint32_t msc_index);
uint32_t val_mpam_get_bwa_wd(uint32_t msc_index);
uint64_t val_mpam_msc_get_mscbw(uint32_t msc_index, uint32_t rsrc_index);
uint32_t val_mpam_mbwu_supports_long(uint32_t msc_index);
uint32_t val_mpam_mbwu_supports_lwd(uint32_t msc_index);
uint32_t val_mpam_get_max_partid(uint32_t msc_index);
uint16_t val_mpam_get_max_intpartid(uint32_t msc_index);
uint32_t val_mpam_msc_get_version(uint32_t msc_index);
uint32_t val_mpam_get_max_pmg(uint32_t msc_index);
void val_mpam_configure_cpor(uint32_t msc_index, uint16_t partid, uint32_t cpbm_percentage);
void val_mpam_configure_ccap(uint32_t msc_index, uint16_t partid,
                                                     uint8_t softlim, uint32_t ccap_percentage);
void val_mpam_configure_mbwpbm(uint32_t msc_index, uint16_t partid, uint32_t mbwpbm_percentage);
void val_mpam_msc_configure_mbwmin(uint32_t msc_index, uint16_t partid, uint32_t mbwmin_percentage);
void val_mpam_msc_configure_mbwmax(uint32_t msc_index, uint16_t partid,
                                                      uint8_t hardlim, uint32_t mbwmax_percentage);
uint32_t val_mpam_get_cpbm_width(uint32_t msc_index);
uint32_t val_mpam_get_mbwpbm_width(uint32_t msc_index);
void val_mpam_configure_csu_mon(uint32_t msc_index, uint16_t partid, uint8_t pmg, uint16_t mon_sel);
void val_mpam_csumon_enable(uint32_t msc_index);
void val_mpam_csumon_disable(uint32_t msc_index);
uint32_t val_mpam_read_csumon(uint32_t msc_index);
uint64_t val_srat_get_prox_domain(uint64_t mem_range_index);
uint32_t val_mpam_mmr_read(uint32_t msc_index, uint32_t reg_offset);
uint64_t val_mpam_mmr_read64(uint32_t msc_index, uint32_t reg_offset);
void     val_mpam_mmr_write(uint32_t msc_index, uint32_t reg_offset, uint32_t data);
void     val_mpam_mmr_write64(uint32_t msc_index, uint32_t reg_offset, uint64_t data);
uint32_t val_mpam_pcc_read(uint32_t msc_index, uint32_t reg_offset);
void     val_mpam_pcc_write(uint32_t msc_index, uint32_t reg_offset, uint32_t data);


uint32_t mpam001_entry(uint32_t num_pe);
uint32_t mpam002_entry(uint32_t num_pe);
uint32_t mpam003_entry(uint32_t num_pe);
uint32_t mpam005_entry(uint32_t num_pe);
uint32_t mpam006_entry(uint32_t num_pe);
uint32_t mpam007_entry(uint32_t num_pe);

#endif /*__ACS_MPAM_H__ */
