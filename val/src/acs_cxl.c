/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0
 *
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
#include "acs_val.h"
#include "acs_common.h"
#include "acs_cxl.h"
#include "val_interface.h"
#include "acs_pcie.h"
#include "acs_memory.h"
#include "acs_mmu.h"
#include "acs_pgt.h"
#include "acs_pe.h"

static CXL_INFO_TABLE *g_cxl_info_table;
static CXL_COMPONENT_TABLE *g_cxl_component_table;
extern pcie_device_bdf_table *g_pcie_bdf_table;

static inline uint64_t
val_align_down(uint64_t value, uint64_t align)
{
  return value & ~(align - 1u);
}

static inline uint64_t
val_align_up(uint64_t value, uint64_t align)
{
  return (value + align - 1u) & ~(align - 1u);
}

/**
  @brief   Retrieve a pointer to a component table entry.

  @param  component_index  Index of the component within the component table.

  @return Pointer to the component entry on success; NULL on error.
**/
CXL_COMPONENT_ENTRY *
val_cxl_get_component_entry(uint32_t component_index)
{
  if (g_cxl_component_table == NULL) {
    val_print(ERROR, " CXL: component table not initialised");
    return NULL;
  }

  if (component_index >= g_cxl_component_table->num_entries) {
    val_print(ERROR, " CXL: invalid component index %u", component_index);
    return NULL;
  }

  return &g_cxl_component_table->component[component_index];
}

/**
  @brief   Locate a capability structure within the register block.

  @param  index      Index of the device for which the cap to be identified.
  @param  cap_id     Capability ID to search for.

  @return 0 if found; else 1.
**/
uint32_t
val_cxl_find_comp_capability(uint32_t index, uint32_t cap_id)
{
  uint64_t base;
  uint32_t arr_hdr;
  uint32_t entries;
  uint32_t idx;
  uint32_t cap_hdr;
  uint32_t found_id;

  base = val_cxl_get_info(CXL_INFO_COMPONENT_BASE, index);
  if (base == 0)
    return 1;

  base = base + CXL_CACHEMEM_PRIMARY_OFFSET;
  arr_hdr = val_mmio_read(base + CXL_COMPONENT_CAP_ARRAY_OFFSET);
  entries = CXL_CAP_ARRAY_ENTRIES(arr_hdr);

  for (idx = 0; idx <= entries; ++idx) {
    cap_hdr = val_mmio_read(base + (uint64_t)idx * CXL_CAP_HDR_SIZE);
    val_print(TRACE, "\n       cap_hdr %llx", cap_hdr);
    if ((cap_hdr == 0u) || (cap_hdr == PCIE_UNKNOWN_RESPONSE))
      continue;

    found_id = CXL_CAP_HDR_CAPID(cap_hdr);
    val_print(TRACE, "\n       Found id %llx", found_id);
    if (found_id == cap_id)
        return 0;


  }

  return 1;
}

/**
  @brief  Convert a component role enumeration value to a printable label.
  @param  role  CXL component role value.
  @return Pointer to a static string describing the role.
**/
static const char *
val_cxl_role_name(uint32_t role)
{

  switch (role) {
  case CXL_COMPONENT_ROLE_ROOT_PORT:        return "Root Port";
  case CXL_COMPONENT_ROLE_SWITCH_UPSTREAM:  return "Switch Upstream";
  case CXL_COMPONENT_ROLE_SWITCH_DOWNSTREAM:return "Switch Downstream";
  case CXL_COMPONENT_ROLE_ENDPOINT:         return "Endpoint";
  default:                                  return "Unknown";
  }

}

/**
  @brief   Convert a CXL device type value to a printable label.
  @param  type  Enumerated device type value.
  @return Pointer to a static string describing the device type.
**/
static const char *
val_cxl_device_type_name(uint32_t type)
{

  switch (type) {
  case CXL_DEVICE_TYPE_TYPE1: return "Type1";
  case CXL_DEVICE_TYPE_TYPE2: return "Type2";
  case CXL_DEVICE_TYPE_TYPE3: return "Type3";
  default:                    return "Unknown";
  }
}

/**
  @brief   Print a diagnostic summary of the discovered CXL components.
**/
void
val_cxl_print_component_summary(void)
{

  uint32_t total            = 0;
  uint32_t role_root_port   = 0;
  uint32_t role_endpoint    = 0;
  uint32_t type_unknown     = 0;
  uint32_t type1_devices    = 0;
  uint32_t type2_devices    = 0;
  uint32_t type3_devices    = 0;
  uint32_t idx;

  if (g_cxl_component_table == NULL) {
    val_print(TRACE, " CXL_COMPONENT: Discovered 0 components");
    return;
  }

  total = g_cxl_component_table->num_entries;

  for (idx = 0; idx < total; idx++) {
    const CXL_COMPONENT_ENTRY *entry = &g_cxl_component_table->component[idx];

    switch (entry->role) {
    case CXL_COMPONENT_ROLE_ROOT_PORT:
      role_root_port++;
      break;
    case CXL_COMPONENT_ROLE_ENDPOINT:
      role_endpoint++;
      break;
    default:
      break;
    }

    switch (entry->device_type) {
    case CXL_DEVICE_TYPE_TYPE1:
      type1_devices++;
      break;
    case CXL_DEVICE_TYPE_TYPE2:
      type2_devices++;
      break;
    case CXL_DEVICE_TYPE_TYPE3:
      type3_devices++;
      break;
    default:
      type_unknown++;
      break;
    }
  }

  val_print(INFO, " CXL_INFO: Number of host bridges     :    %u\n",
             g_cxl_info_table->num_entries);
  val_print(INFO,
            " CXL_INFO: Number of components       : %4u\n", (uint64_t)total);
  val_print(INFO,
            " CXL_INFO: Root Ports                 : %4u\n", (uint64_t)role_root_port);
  val_print(INFO,
            " CXL_INFO: Devices (Endpoints)        : %4u\n", (uint64_t)role_endpoint);
  val_print(INFO,
            " CXL_INFO: Type1 Devices              : %4u\n", (uint64_t)type1_devices);
  val_print(INFO,
            " CXL_INFO: Type2 Devices              : %4u\n", (uint64_t)type2_devices);
  val_print(INFO,
            " CXL_INFO: Type3 Devices              : %4u\n", (uint64_t)type3_devices);

  val_print(TRACE, " CXL_COMPONENT: Discovered %u components", (uint64_t)total);

  for (idx = 0; idx < g_cxl_component_table->num_entries; idx++) {
    const CXL_COMPONENT_ENTRY *entry = &g_cxl_component_table->component[idx];
    const char *role_str = val_cxl_role_name(entry->role);
    const char *dtype_str = val_cxl_device_type_name(entry->device_type);

    val_print(TRACE, "\n   Component Index : %u", (uint64_t)idx);
    val_print(TRACE, "     BDF           : 0x%x", (uint64_t)entry->bdf);
    val_print(TRACE, "     Role          : %a", (uint64_t)role_str);
    val_print(TRACE, "     Device Type   : %a", (uint64_t)dtype_str);

    if (entry->component_reg_base) {
      val_print(TRACE, "     CompReg Base  : 0x%llx", entry->component_reg_base);
      val_print(TRACE, "     CompReg Len   : 0x%llx", entry->component_reg_length);
    }


    val_print(TRACE, "   HDM Decoder Count  : %u", entry->hdm_decoder_count);
  }
}

uint32_t
val_cxl_find_capability(uint32_t bdf, uint32_t cid, uint32_t *cid_offset)
{

  uint32_t hdr0, hdr1;
  uint32_t hdr2;
  uint32_t next_cap_offset;
  uint16_t dvsec_id;

  next_cap_offset = PCIE_ECAP_START;

  while (next_cap_offset)
  {
    val_pcie_read_cfg(bdf, next_cap_offset, &hdr0);
    val_pcie_read_cfg(bdf, next_cap_offset + CXL_DVSEC_HDR1_OFFSET, &hdr1);

    if (((hdr0 & PCIE_ECAP_CIDR_MASK) == ECID_DVSEC) &&
        ((hdr1 & CXL_DVSEC_HDR1_VENDOR_ID_MASK) == CXL_DVSEC_VENDOR_ID))
    {
      val_pcie_read_cfg(bdf, next_cap_offset + CXL_DVSEC_HDR2_OFFSET, &hdr2);
      dvsec_id = hdr2 & CXL_DVSEC_HDR2_ID_MASK;
      if (dvsec_id == cid)
      {
          val_print(TRACE, " \nFound CXL DVSEC 0x%lx", dvsec_id);
          *cid_offset = next_cap_offset;
          return 0;
      }
    }
    next_cap_offset = (hdr0 >> PCIE_ECAP_NCPR_SHIFT) & PCIE_ECAP_NCPR_MASK;
  }
  return 1;
}

/**
  @brief   Retrieve or allocate a component entry for the given PCIe function.
  @param  bdf  PCIe identifier of the device to look up.
  @return Pointer to the component entry or NULL if no slot is available.
**/
static CXL_COMPONENT_ENTRY *
val_cxl_get_or_create_component(uint32_t bdf)
{

  uint32_t idx;
  CXL_COMPONENT_ENTRY *entry;

  if (g_cxl_component_table == NULL)
    return NULL;

  for (idx = 0; idx < g_cxl_component_table->num_entries; idx++) {
    entry = &g_cxl_component_table->component[idx];
    if (entry->bdf == bdf)
      return entry;
  }

  if (g_cxl_component_table->num_entries >= CXL_COMPONENT_TABLE_MAX_ENTRIES)
    return NULL;

  entry = &g_cxl_component_table->component[g_cxl_component_table->num_entries++];

  entry->bdf                  = bdf;
  entry->host_bridge_index    = CXL_COMPONENT_INVALID_INDEX;
  entry->role                 = CXL_COMPONENT_ROLE_UNKNOWN;
  entry->device_type          = CXL_DEVICE_TYPE_UNKNOWN;
  entry->component_reg_base   = 0;
  entry->component_reg_length = 0;
  entry->device_reg_base      = 0;
  entry->device_reg_length    = 0;
  entry->hdm_decoder_count    = 0;

  return entry;

}

/**
  @brief   Allocate and initialise the global CXL component table.

  @return ACS_STATUS_PASS on success, ACS_STATUS_ERR otherwise.
**/
uint32_t
val_cxl_create_default_component_table(void)
{
  uint32_t idx;
  uint32_t i;

  if (g_cxl_component_table != NULL)
    return ACS_STATUS_PASS;

  g_cxl_component_table =
    (CXL_COMPONENT_TABLE *)val_aligned_alloc(MEM_ALIGN_4K, CXL_COMPONENT_TABLE_SZ);

  if (g_cxl_component_table == NULL)
    return ACS_STATUS_ERR;

  for (i = 0; i < CXL_COMPONENT_TABLE_SZ; i++)
    ((uint8_t *)g_cxl_component_table)[i] = 0;

  g_cxl_component_table->num_entries = 0;

  for (idx = 0; idx < CXL_COMPONENT_TABLE_MAX_ENTRIES; idx++) {
    g_cxl_component_table->component[idx].host_bridge_index = CXL_COMPONENT_INVALID_INDEX;
    g_cxl_component_table->component[idx].role              = CXL_COMPONENT_ROLE_UNKNOWN;
    g_cxl_component_table->component[idx].device_type       = CXL_DEVICE_TYPE_UNKNOWN;
  }

  return ACS_STATUS_PASS;
}

/**
  @brief   Reset component discovery table for subsequent scans.
**/
void
val_cxl_free_component_table(void)
{
    if (g_cxl_component_table != NULL) {
        val_memory_free_aligned((void *)g_cxl_component_table);
        g_cxl_component_table = NULL;
    }
}

/**
  @brief   Return the active CXL component table pointer, if any.
  @return  Pointer to the global component table, or NULL if not created.
**/
CXL_COMPONENT_TABLE *
val_cxl_component_table_ptr(void)
{
  return g_cxl_component_table;
}

/**
  @brief  Query a property from the discovered CXL component table.
  @param  type   Selector describing which field to return.
  @param  index  Component index when the selector is per-entry.
  @return Requested value or 0 on error.
**/
uint64_t
val_cxl_get_component_info(CXL_COMPONENT_INFO_e type, uint32_t index)
{

  if (g_cxl_component_table == NULL) {
    val_print(ERROR, " GET_CXL_COMPONENT_INFO: component table not created");
    return 0;
  }

  if (type == CXL_COMPONENT_INFO_COUNT)
    return g_cxl_component_table->num_entries;

  if (index >= g_cxl_component_table->num_entries) {
    val_print(ERROR, " GET_CXL_COMPONENT_INFO: Invalid index %u", index);
    return 0;
  }

  const CXL_COMPONENT_ENTRY *entry = &g_cxl_component_table->component[index];
  switch (type) {
  case CXL_COMPONENT_INFO_ROLE:
    return entry->role;
  case CXL_COMPONENT_INFO_DEVICE_TYPE:
    return entry->device_type;
  case CXL_COMPONENT_INFO_HOST_BRIDGE_INDEX:
    return entry->host_bridge_index;
  case CXL_COMPONENT_INFO_BDF_INDEX:
    return entry->bdf;
  case CXL_COMPONENT_INFO_COMPONENT_BASE:
    return entry->component_reg_base;
  case CXL_COMPONENT_INFO_COMPONENT_LENGTH:
    return entry->component_reg_length;
  case CXL_COMPONENT_INFO_HDM_COUNT:
    return entry->hdm_decoder_count;
  default:
    val_print(ERROR, " GET_CXL_COMPONENT_INFO: Unsupported type %u", type);
    break;
  }

  return 0;
}

/**
  @brief  Decode the HDM decoder count field from the capability register.
  @param  field  Raw field extracted from the HDM capability register.
  @return Number of HDM decoders reported, or 0 when encoding is reserved.
**/
static uint32_t
val_cxl_decode_hdm_count(uint32_t field)
{

  switch (field & CXL_HDM_DECODER_COUNT_MASK) {
  case 0x0: return 1;
  case 0x1: return 2;
  case 0x2: return 4;
  case 0x3: return 6;
  case 0x4: return 8;
  case 0x5: return 10;
  case 0x6: return 12;
  case 0x7: return 14;
  case 0x8: return 16;
  case 0x9: return 20;
  case 0xA: return 24;
  case 0xB: return 28;
  case 0xC: return 32;
  default:  return 0;
  }
}

/**
  @brief  Populate decoder metadata for a component from its HDM capability.
  @param  component  Component entry to update.
  @param  cap_base   Base address of the HDM capability structure.
**/
static void
val_cxl_parse_hdm_capability(CXL_COMPONENT_ENTRY *component,
                             uint64_t cap_base)
{

  uint32_t cap_reg;
  uint32_t dec_count_encoded;
  uint32_t decoder_count;

  if ((component == NULL) || (cap_base == 0))
    return;

  cap_reg = val_mmio_read(cap_base + CXL_HDM_CAP_REG_OFFSET);
  dec_count_encoded = (cap_reg >> CXL_HDM_DECODER_COUNT_SHIFT) & CXL_HDM_DECODER_COUNT_MASK;
  decoder_count = val_cxl_decode_hdm_count(dec_count_encoded);

  component->hdm_decoder_count = decoder_count;
}

/**
  @brief   Get host physical address for a given PCIe BAR on a CXL device.
  @param bdf           - Segment/Bus/Device/Function identifier.
  @param bir           - BAR indicator (BAR number) from CXL Register Locator.
  @param bar_base_out  - Output pointer for resolved BAR base host PA.
  @param bar_is64      - Output flag set to 1 if BAR is 64-bit, else 0.
  @return  VAL_CXL_BAR_SUCCESS on success; specific error code otherwise.
**/
int val_cxl_get_mmio_bar_host_pa(uint32_t bdf, uint8_t bir,
                                        uint64_t *bar_base_out, uint32_t *bar_is64)
{

    uint32_t off, lo, hi = 0, header_type, mit, mdt;
    uint64_t bar_base64;
    uint32_t bar_is64_local = 0;

    if (!bar_base_out)
        return VAL_CXL_BAR_ERR_CFG_READ;

    /* Guard BAR index against header type limits */
    header_type = val_pcie_function_header_type(bdf);
    if ((header_type == TYPE0_HEADER && bir >= TYPE0_MAX_BARS) ||
        (header_type == TYPE1_HEADER && bir >= TYPE1_MAX_BARS)) {
        *bar_base_out = 0;
        if (bar_is64) *bar_is64 = 0;
        return VAL_CXL_BAR_ERR_INVALID_INDEX;
    }

    off = TYPE01_BAR + (bir * 4);
    if (val_pcie_read_cfg(bdf, off, &lo))
        return VAL_CXL_BAR_ERR_CFG_READ;

    mit = (lo >> BAR_MIT_SHIFT) & BAR_MIT_MASK;
    if (mit != MMIO)
        return VAL_CXL_BAR_ERR_NOT_MMIO;

    mdt = (lo >> BAR_MDT_SHIFT) & BAR_MDT_MASK;
    if (mdt == BITS_64) {
        if (val_pcie_read_cfg(bdf, off + 4, &hi))
            return VAL_CXL_BAR_ERR_CFG_READ;
        /* Reconstruct base using BAR field macros to avoid magic masks */
        bar_base64 = (((uint64_t)hi) << 32) |
                     (((uint64_t)((lo >> BAR_BASE_SHIFT) & BAR_BASE_MASK)) << BAR_BASE_SHIFT);
        bar_is64_local = 1;
    } else {
        bar_base64 = ((uint64_t)((lo >> BAR_BASE_SHIFT) & BAR_BASE_MASK)) << BAR_BASE_SHIFT;
        bar_is64_local = 0;
    }

    /* Enable Bus Master Enable and Memory Space Access */
    val_pcie_enable_bme(bdf);
    val_pcie_enable_msa(bdf);
    *bar_base_out = bar_base64;
    if (bar_is64) *bar_is64 = bar_is64_local;
    return (*bar_base_out) ? PCIE_SUCCESS : VAL_CXL_BAR_ERR_ZERO;

}

/**
  @brief   Return a human-readable name for a CXL component capability ID.
  @param id    - CXL component capability ID.
  @return  Pointer to a static string describing the capability.
**/
const char *
val_cxl_cap_name(uint16_t id)
{
    switch (id) {
    case CXL_CAPID_COMPONENT_CAP:     return "CXL Capability";
    case CXL_CAPID_RAS:               return "CXL RAS Capability";
    case CXL_CAPID_SECURITY:          return "CXL Security Capability";
    case CXL_CAPID_LINK:              return "CXL Link Capability";
    case CXL_CAPID_HDM_DECODER:       return "CXL HDM Decoder Capability";
    case CXL_CAPID_EXT_SECURITY:      return "CXL Extended Security Capability";
    case CXL_CAPID_IDE:               return "CXL IDE Capability";
    case CXL_CAPID_SNOOP_FILTER:      return "CXL Snoop Filter Capability";
    case CXL_CAPID_TIMEOUT_ISOLATION: return "CXL Timeout Isolation Capability";
    case CXL_CAPID_BI_DECODER:        return "CXL BI Decoder Capability";
    case CXL_CAPID_CACHE_ID_DECODER:  return "CXL CACHE ID Decoder Capability";
    /* Device Register Cap IDs overlap numerically with Component Cap IDs.
       Avoid duplicate case values; treat others as unknown here. */
    default:
     return "Unknown CXL Capability";
    }
}

/**
  @brief   Return a human-readable name for a CXL device register capability ID.
  @param id    - CXL device capability ID.
  @return  Pointer to a static string describing the device capability.
**/
static const char *val_cxl_dev_cap_name(uint16_t id)
{
    switch (id) {
    case CXL_DEVCAPID_DEVICE_STATUS:     return "Device Status Registers";
    case CXL_DEVCAPID_PRI_MAILBOX:       return "Primary Mailbox Registers";
    case CXL_DEVCAPID_SEC_MAILBOX:       return "Secondary Mailbox Registers";
    case CXL_DEVCAPID_MEMORY_DEVICE_STS: return "Memory Device Status Registers";
    default: return "Unknown CXL Device Capability";
    }
}

/* Read one Device Capability header element from the Device Register block
   array (8.2.8.2.1). Each element is 16 bytes, but ID/Version/Offset are
   contained in the first 64-bit word:
     - w0[15:0]  = Capability ID
     - w0[23:16] = Version
     - w0[63:32] = Offset from start of Device Register block
   Returns 0 on success, non-zero on invalid/terminator element.
*/
/**
  @brief Read one capability header from a CXL Device Capabilities array.
  @param arr_base  - Base address of the Device Capabilities array (MMIO).
  @param index     - Element index within the array to read.
  @param id_out    - Output pointer for the capability ID.
  @param ver_out   - Output pointer for the capability version.
  @param off_out   - Output pointer for the capability offset from block base.
  @return  0 on success; non-zero if the entry is invalid or terminator.
**/
static int val_cxl_dev_cap_hdr_read(uint64_t arr_base, uint32_t index,
                                    uint16_t *id_out, uint8_t *ver_out, uint32_t *off_out)
{
    uint64_t w0 = val_mmio_read64(arr_base + (uint64_t)index * CXL_DEV_CAP_ELEM_SIZE);
    uint32_t w0_lo = (uint32_t)(w0 & CXL_DEV_CAP_ELEM_W0_OFF_MASK);
    if (w0_lo == 0 || w0_lo == (uint32_t)PCIE_UNKNOWN_RESPONSE)
        return PCIE_CAP_NOT_FOUND;

    if (id_out)  *id_out  = (uint16_t)(w0_lo & CXL_DEV_CAP_ELEM_W0_ID_MASK);
    if (ver_out) *ver_out = (uint8_t)((w0_lo >> CXL_DEV_CAP_ELEM_W0_VER_SHIFT)
                            & CXL_DEV_CAP_ELEM_W0_VER_MASK);
    if (off_out) *off_out = (uint32_t)((w0 >> CXL_DEV_CAP_ELEM_W0_OFF_SHIFT)
                            & CXL_DEV_CAP_ELEM_W0_OFF_MASK);
    return PCIE_SUCCESS;
}

/* Dump a CXL register block (Component or Device), printing all caps it contains. */
/**
  @brief   Walk a CXL register block and print discovered capabilities.
  @param base_pa     - Base physical address of the register block.
  @param block_len   - Length of the block in bytes (0 if unknown/unbounded).
  @param block_id    - CXL Register Block ID (e.g., Component or Device).
  @return  None.
**/
void val_cxl_dump_reg_block(uint64_t base_pa,
                                   uint32_t block_len,
                                   uint16_t block_id,
                                   CXL_COMPONENT_ENTRY *component)
{

    uint16_t id;
    uint8_t  ver;
    const char *hdr_fmt;
    uint64_t hdr64;
    uint32_t hdr_cnt;
    uint64_t hdr_base;
    const uint32_t elem_sz = CXL_DEV_CAP_ELEM_SIZE; /* bytes per element */
    uint32_t max_cnt_by_len;
    uint32_t i;
    uint16_t id_local;
    uint8_t  ver_local;
    uint32_t cap_off;
    uint32_t arr_hdr;
    uint16_t cap_id;
    uint8_t  cap_ver;
    uint8_t  cachemem_ver;
    uint8_t  arr_sz;
    uint32_t idx;
    uint32_t cap_hdr;
    uint32_t cap_ptr;
    uint64_t cap_base;

    /* Per CXL 3.1 Table 8-21, Component registers reside in the
       CXL.cachemem Primary range: BAR base + 4KB. Adjust here so that
       the capability walk lands in the correct window. */
    if (block_id == CXL_REG_BLOCK_COMPONENT)
        base_pa += CXL_CACHEMEM_PRIMARY_OFFSET;

    if (block_id == CXL_REG_BLOCK_COMPONENT)
        hdr_fmt = " \n[CXL Component Registers] base=0x%lx";
    else if (block_id == CXL_REG_BLOCK_DEVICE)
        hdr_fmt = " \n[CXL Device Registers] base=0x%lx";
    else
        hdr_fmt = " \n[CXL Register Block] base=0x%lx";
    val_print(TRACE, (char8_t *)hdr_fmt, base_pa);

    if (block_len && block_len < CXL_CAP_HDR_SIZE) {
        val_print(TRACE, " ERROR in CXL Summary :: Block length too small\n");
        return;
    }

    if (block_id == CXL_REG_BLOCK_DEVICE) {
        /* Device Register Block: starts with Device Capabilities Array (8.2.8) */
        hdr64 = val_mmio_read64(base_pa + CXL_DEV_CAP_ARR_HDR_OFFSET);
        hdr_cnt = (uint32_t)((hdr64 >> CXL_DEV_CAP_ARR_COUNT_SHIFT) & CXL_DEV_CAP_ARR_COUNT_MASK);
        hdr_base = base_pa + CXL_DEV_CAP_ARR_BASE_OFFSET;
        max_cnt_by_len = (block_len > CXL_DEV_CAP_ARR_HDR_SIZE) ?
                         ((block_len - CXL_DEV_CAP_ARR_HDR_SIZE) / elem_sz) : 0;
        if (block_len == 0) {
            if (hdr_cnt > CXL_DEV_CAP_MAX_GUARD) hdr_cnt = CXL_DEV_CAP_MAX_GUARD; /* guard */
        } else if (hdr_cnt > max_cnt_by_len) {
            hdr_cnt = max_cnt_by_len;
        }

        val_print(TRACE, "   \nDevCap Array count=%ld", (uint64_t)hdr_cnt);
        for (i = 0; i < hdr_cnt; ++i) {
        if (val_cxl_dev_cap_hdr_read(hdr_base, i, &id_local, &ver_local, &cap_off))
                break;
            id = id_local; ver = ver_local;
            val_print(TRACE, "   \nDevCap[%ld]: ", (uint64_t)i);
            val_print(TRACE, "   ID=0x%x ", (uint64_t)id);
            val_print(TRACE, "   (%a) ", (uint64_t)val_cxl_dev_cap_name(id));
            val_print(TRACE, "   Ver=%d ", (uint64_t)ver);
            val_print(TRACE, "   Off=0x%x", (uint64_t)cap_off);
            /* Bounds check for capability structure */
            if (block_len && cap_off >= block_len) {
                val_print(TRACE, " ERROR in CXL Summary:: -> Cap offset out of range");
            }
        }
        return;
    }
    /* Component Register Block (8.2.4): capability array within 4KB primary region */
    arr_hdr     = val_mmio_read(base_pa + CXL_COMPONENT_CAP_ARRAY_OFFSET);
    cap_id       = CXL_CAP_HDR_CAPID(arr_hdr);
    cap_ver      = CXL_CAP_HDR_VER(arr_hdr);
    cachemem_ver = CXL_CAP_HDR_CACHEMEM_VER(arr_hdr);
    arr_sz       = CXL_CAP_ARRAY_ENTRIES(arr_hdr);
    val_print(TRACE,  "   \nCXL_Capability_Header: ID=0x%x", cap_id);
    val_print(TRACE,  "   \nPrimary Array: CXL.cachemem v%ld",
                (uint64_t)cachemem_ver);
    val_print(TRACE,  "   CXL Cap v%ld", cap_ver);
    val_print(TRACE,  "   entries=%ld", arr_sz);
    for (idx = 0; idx <= arr_sz; ++idx) {
        cap_hdr = val_mmio_read(base_pa + (uint64_t)idx * CXL_CAP_HDR_SIZE);
        if (cap_hdr == PCIE_UNKNOWN_RESPONSE || cap_hdr == 0x00000000)
            continue;
        id  = CXL_CAP_HDR_CAPID(cap_hdr);
        ver = CXL_CAP_HDR_VER(cap_hdr);
        cap_ptr = CXL_CAP_HDR_POINTER(cap_hdr);
        cap_base = base_pa + cap_ptr;
        val_print(TRACE, "   \nCapID=0x%x ", id);
        val_print(TRACE, "    (%a) ", (uint64_t)val_cxl_cap_name(id));
        val_print(TRACE, "    Ver=%d ", ver);
        val_print(TRACE, "    @+0x%llx", (idx * CXL_CAP_HDR_SIZE));
        if ((component != NULL) && (block_id == CXL_REG_BLOCK_COMPONENT) &&
             (id == CXL_CAPID_HDM_DECODER))
                val_cxl_parse_hdm_capability(component, cap_base);
    }
}

/**
  @brief   Assign a component role based on the PCIe device/port type.

  @param  component  Pointer to component entry to update.
  @param  dp_type    PCIe device/port type value.

  @return  None.
**/
static void
val_cxl_assign_component_role(CXL_COMPONENT_ENTRY *component, uint32_t dp_type)
{
  if (component == NULL)
    return;
  switch (dp_type) {
  case RP:
  case iEP_RP:
    component->role = CXL_COMPONENT_ROLE_ROOT_PORT;
    break;
  case UP:
    component->role = CXL_COMPONENT_ROLE_SWITCH_UPSTREAM;
    break;
  case DP:
    component->role = CXL_COMPONENT_ROLE_SWITCH_DOWNSTREAM;
    break;
  case EP:
  case iEP_EP:
  case RCiEP:
    component->role = CXL_COMPONENT_ROLE_ENDPOINT;
    break;
  default:
    component->role = CXL_COMPONENT_ROLE_UNKNOWN;
    break;
  }
}

/**
  @brief   Parse a CXL Register Locator DVSEC and walk exposed register blocks.

  @param bdf        - BDF of the device.
  @param ecap_off   - Offset to the Register Locator DVSEC in config space.

  @return  None.
**/
static void val_cxl_parse_register_locator(uint32_t bdf,
                                           uint32_t ecap_off,
                                           uint32_t dp_type)
{
    uint32_t dvsec_hdr1;
    uint16_t dvsec_vendor;
    uint32_t dvsec_rev, dvsec_len, num_entries, ent_off_cfg, i;
    uint32_t reg0, off;
    uint16_t block_id;
    uint8_t  bir;
    uint8_t  bar_num;
    uint32_t bar_st;
    uint64_t reg_off, bar_base, block_pa;
    uint32_t      bar_is64;
    const char *blkname;
    CXL_COMPONENT_ENTRY *component;

    component = val_cxl_get_or_create_component(bdf);

    if (component)
        val_cxl_assign_component_role(component, dp_type);

    if (val_pcie_read_cfg(bdf, ecap_off + CXL_DVSEC_HDR1_OFFSET, &dvsec_hdr1)) {
        val_print(TRACE, " ERROR in CXL Summary :: DVSEC Header1 read failed");
        return;
    }

    dvsec_vendor = dvsec_hdr1 & CXL_DVSEC_HDR1_VENDOR_ID_MASK;
    dvsec_rev    = (dvsec_hdr1 >> CXL_DVSEC_HDR1_REV_SHIFT) & CXL_DVSEC_HDR1_REV_MASK;
    dvsec_len    = (dvsec_hdr1 >> CXL_DVSEC_HDR1_LEN_SHIFT) & CXL_DVSEC_HDR1_LEN_MASK;

    val_print(TRACE, " \nDVSEC Header 1 : 0x%lx", dvsec_hdr1);
    val_print(TRACE, "    \nVendor ID    : 0x%lx", dvsec_vendor);
    val_print(TRACE, "    Revision     : 0x%lx", dvsec_rev);
    val_print(TRACE, "    Length (B)   : 0x%lx", dvsec_len);

    if (dvsec_len < CXL_RL_REG_BLK_ENTRIES ||
        ((dvsec_len - CXL_RL_REG_BLK_ENTRIES) % CXL_RL_ENTRY_SIZE) != 0) {
        val_print(TRACE, " ERROR in CXL Summary :: RL: Bad DVSEC Length 0x%lx",
                  (uint64_t)dvsec_len);
        return;
    }

    num_entries = (dvsec_len - CXL_RL_REG_BLK_ENTRIES) / CXL_RL_ENTRY_SIZE;
    val_print(TRACE, " \nRegister Locator: %ld entries", num_entries);

    ent_off_cfg = ecap_off + CXL_RL_REG_BLK_ENTRIES;
    for (i = 0; i < num_entries; i++, ent_off_cfg += CXL_RL_ENTRY_SIZE) {
        if (val_pcie_read_cfg(bdf, ent_off_cfg + CXL_RL_REG_OFF_LOW, &reg0) ||
            val_pcie_read_cfg(bdf, ent_off_cfg + CXL_RL_REG_OFF_HIGH, &off)) {
            val_print(TRACE, " ERROR in CXL Summary :: RL[%ld]: entry read failed", i);
            continue;
        }

        /* DW0: [2:0]=BIR, [15:8]=BlockID */
        bir      = (reg0 >> CXL_RL_ENTRY_BIR_SHIFT)    & CXL_RL_ENTRY_BIR_MASK;
        block_id = (reg0 >> CXL_RL_ENTRY_BLOCKID_SHIFT) & CXL_RL_ENTRY_BLOCKID_MASK;
        bar_num = CXL_RL_BAR_NUM(bir);
        reg_off = off; /* RL entry is 8B: 32b offset */
        {
            bar_st = val_cxl_get_mmio_bar_host_pa(bdf, bar_num, &bar_base, &bar_is64);
            if (bar_st != PCIE_SUCCESS) {
                val_print(TRACE, " ERROR in CXL Summary :: RL[%ld]: ", i);
                val_print(TRACE, " ERROR in CXL Summary :: BAR%ld not usable ",
                            (uint64_t)bar_num);
                switch (bar_st) {
                case VAL_CXL_BAR_ERR_INVALID_INDEX:
                    val_print(TRACE, " ERROR in CXL Summary :: Invalid BAR index");
                    break;
                case VAL_CXL_BAR_ERR_CFG_READ:
                    val_print(TRACE, " ERROR in CXL Summary :: CFG read failed");
                    break;
                case VAL_CXL_BAR_ERR_NOT_MMIO:
                    val_print(TRACE, " ERROR in CXL Summary :: BAR not MMIO type");
                    break;
                case VAL_CXL_BAR_ERR_ZERO:
                    val_print(TRACE, " ERROR in CXL Summary :: BAR is zero");
                    break;
                default:
                    break;
                }
                continue;
            }
        }
        block_pa = bar_base + reg_off;
        val_print(TRACE, "  \nRL reg0=0x%lx ", reg0);
        val_print(TRACE, "  off=0x%lx", off);
        val_print(TRACE, "  BlockID=0x%x ", block_id);
        val_print(TRACE, "  BIR=%d", CXL_RL_BAR_NUM(bir));

        blkname =
            (block_id == CXL_REG_BLOCK_COMPONENT) ? "\nCXL Component Registers" :
            (block_id == CXL_REG_BLOCK_DEVICE)    ? "CXL Device Registers" :
            (block_id == CXL_REG_BLOCK_VENDOR_SPECIFIC) ? "Vendor-Specific Reg Block" :
                                                    "CXL Register Block";

        val_print(TRACE, "  \nRL[%ld]: ", i);
        val_print(TRACE, "  BlockID=0x%x ", block_id);
        val_print(TRACE, "  (%a) ", (uint64_t)blkname);
        val_print(TRACE, "  BIR=%ld ", CXL_RL_BAR_NUM(bir));
        val_print(TRACE, "  off=0x%lx ", reg_off);

        if (component && block_id == CXL_REG_BLOCK_COMPONENT) {
            component->component_reg_base   = block_pa + CXL_CACHEMEM_PRIMARY_OFFSET;
            component->component_reg_length = CXL_CACHEMEM_PRIMARY_SIZE;
        } else if (component && block_id == CXL_REG_BLOCK_DEVICE) {
            component->device_reg_base    = block_pa;
            component->device_reg_length  = CXL_CACHEMEM_PRIMARY_SIZE;
        }

        /* Walk capabilities; length unknown in RL entry, pass 0 */
        val_cxl_dump_reg_block(block_pa, 0, block_id, component);
    }
}

/**
  @brief   Parse the PCIe DVSEC for CXL Devices and update component flags.

  @param bdf        - BDF of the device exposing the DVSEC.
  @param ecap_off   - Offset to the DVSEC in PCIe config space.
  @param dp_type    - PCIe device/port type.

  @return  None.
**/
static void val_cxl_parse_device_dvsec(uint32_t bdf,
                                       uint32_t ecap_off,
                                       uint32_t dp_type)
{
    CXL_COMPONENT_ENTRY *component;
    uint32_t hdr2;
    uint32_t cache_capable;
    uint32_t io_capable;
    uint32_t mem_capable;
    uint16_t cxl_cap;

    component = val_cxl_get_or_create_component(bdf);
    if (component == NULL)
        return;

    val_cxl_assign_component_role(component, dp_type);

    if (val_pcie_read_cfg(bdf, ecap_off + CXL_DVSEC_HDR2_OFFSET, &hdr2))
        return;

    cxl_cap = (uint16_t)((hdr2 >> CXL_DVSEC_CXL_CAPABILITY_SHIFT) &
                         CXL_DVSEC_CXL_CAPABILITY_MASK);

    cache_capable = ((cxl_cap & CXL_DVSEC_CXL_CAP_CACHE_CAPABLE) != 0) ? 1 : 0;
    io_capable = ((cxl_cap & CXL_DVSEC_CXL_CAP_IO_CAPABLE) != 0) ? 1 : 0;
    mem_capable = ((cxl_cap & CXL_DVSEC_CXL_CAP_MEM_CAPABLE) != 0) ? 1 : 0;

    if (io_capable == 1) {
        if (cache_capable == 1) {
            if (mem_capable == 1)
                component->device_type = CXL_DEVICE_TYPE_TYPE2;
            else
                component->device_type = CXL_DEVICE_TYPE_TYPE1;
        } else if (mem_capable == 1) {
            component->device_type = CXL_DEVICE_TYPE_TYPE3;
        }
    }
}

static uint32_t
val_cxl_find_host_index_by_uid(uint32_t uid, uint32_t *index_out)
{
  if ((g_cxl_info_table == NULL) || (index_out == NULL))
    return ACS_STATUS_ERR;

  for (uint32_t idx = 0; idx < g_cxl_info_table->num_entries; idx++) {
    if (g_cxl_info_table->device[idx].uid == uid) {
      *index_out = idx;
      return ACS_STATUS_PASS;
    }
  }

  return ACS_STATUS_SKIP;
}


static void
val_cxl_assign_host_bridge_indices(void)
{
  uint32_t uid = 0;
  uint32_t host_index, comp_index;
  if ((g_cxl_info_table == NULL) || (g_cxl_component_table == NULL))
    return;

  for (comp_index = 0; comp_index < g_cxl_component_table->num_entries;
       comp_index++) {
    CXL_COMPONENT_ENTRY *entry = &g_cxl_component_table->component[comp_index];

    if (entry->role != CXL_COMPONENT_ROLE_ROOT_PORT)
      continue;

    if (entry->host_bridge_index != CXL_COMPONENT_INVALID_INDEX)
      continue;

    if (pal_cxl_get_host_bridge_uid(entry->bdf, &uid) != 0) {
      continue;
    }

    if (val_cxl_find_host_index_by_uid(uid, &host_index) != ACS_STATUS_PASS) {
      continue;
    }

    entry->host_bridge_index = host_index;
    val_print(TRACE, "  \n host_index:%llx ", host_index);
  }
}

/**
  @brief   Quickly determine whether a PCIe function advertises any CXL DVSECs.

  @param  bdf  PCIe identifier of the device to probe.

  @return ACS_STATUS_PASS when a CXL DVSEC is present.
          PCIE_UNKNOWN_RESPONSE when none are found.
          ACS_STATUS_ERR on config space access failures.
**/
uint32_t
val_cxl_device_is_cxl(uint32_t bdf)
{
  uint32_t next_cap_offset = PCIE_ECAP_START;
  uint32_t prev_off = PCIE_UNKNOWN_RESPONSE;
  uint32_t hdr0;
  uint32_t hdr1;

  while (next_cap_offset) {
    if (next_cap_offset == prev_off)
      break;

    prev_off = next_cap_offset;

    if (val_pcie_read_cfg(bdf, next_cap_offset, &hdr0))
      return ACS_STATUS_ERR;

    if ((hdr0 == 0u) || (hdr0 == PCIE_UNKNOWN_RESPONSE))
      break;

    if ((hdr0 & PCIE_ECAP_CIDR_MASK) == ECID_DVSEC) {

      if (val_pcie_read_cfg(bdf, next_cap_offset + CXL_DVSEC_HDR1_OFFSET, &hdr1))
        return ACS_STATUS_ERR;

      if ((hdr1 & CXL_DVSEC_HDR1_VENDOR_ID_MASK) == CXL_DVSEC_VENDOR_ID)
        return ACS_STATUS_PASS;
    }

    next_cap_offset = (hdr0 >> PCIE_ECAP_NCPR_SHIFT) & PCIE_ECAP_NCPR_MASK;
  }

  return ACS_STATUS_SKIP;
}

/**
  @brief   Populate the global CXL component table for a discovered CXL function.

  @param  bdf  PCIe identifier of the device to register.

  @return ACS_STATUS_PASS when the component entry is created/populated.
          ACS_STATUS_SKIP when no CXL capabilities are detected.
          ACS_STATUS_ERR on allocation or config space errors.
**/
uint32_t
val_cxl_create_table()
{
  uint32_t status;
  uint32_t bdf;
  uint32_t next_cap_offset;
  uint32_t prev_off;
  uint32_t hdr0;
  uint32_t dp_type;
  uint32_t hdr1;
  uint32_t hdr2;
  uint32_t next_ptr;
  uint16_t dvsec_id;
  uint32_t found = 0;
  uint32_t tbl_index = 0;
  pcie_device_bdf_table *bdf_tbl_ptr;
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  for (tbl_index = 0; tbl_index < bdf_tbl_ptr->num_entries; tbl_index++)
  {
      bdf = bdf_tbl_ptr->device[tbl_index].bdf;
      if (val_cxl_device_is_cxl(bdf) != ACS_STATUS_PASS)
          continue;

      val_print(TRACE, "\n CXL device is: 0x%lx   ", bdf);
      status = val_cxl_create_default_component_table();
      if (status != ACS_STATUS_PASS)
        return ACS_STATUS_ERR;

      next_cap_offset = PCIE_ECAP_START;
      prev_off = PCIE_UNKNOWN_RESPONSE;
      dp_type = val_pcie_device_port_type(bdf);

      while (next_cap_offset) {

        if (next_cap_offset == prev_off)
          break;

        prev_off = next_cap_offset;

        if (val_pcie_read_cfg(bdf, next_cap_offset, &hdr0))
          return ACS_STATUS_ERR;

        next_ptr = (hdr0 >> PCIE_ECAP_NCPR_SHIFT) & PCIE_ECAP_NCPR_MASK;

        if ((hdr0 == 0u) || (hdr0 == PCIE_UNKNOWN_RESPONSE)) {
          next_cap_offset = next_ptr;
          continue;
        }

        if ((hdr0 & PCIE_ECAP_CIDR_MASK) != ECID_DVSEC) {
          next_cap_offset = next_ptr;
          continue;
        }

        if (val_pcie_read_cfg(bdf, next_cap_offset + CXL_DVSEC_HDR1_OFFSET, &hdr1))
          return ACS_STATUS_ERR;

        if ((hdr1 & CXL_DVSEC_HDR1_VENDOR_ID_MASK) != CXL_DVSEC_VENDOR_ID) {
          next_cap_offset = next_ptr;
          continue;
        }

        if (val_pcie_read_cfg(bdf, next_cap_offset + CXL_DVSEC_HDR2_OFFSET, &hdr2))
          return ACS_STATUS_ERR;

        dvsec_id = (uint16_t)(hdr2 & CXL_DVSEC_HDR2_ID_MASK);
        found = 1;

        if (val_cxl_get_or_create_component(bdf) == NULL)
          return ACS_STATUS_ERR;

        val_print(TRACE, "\n BDF: 0x%lx  :: ", bdf);
        val_print(TRACE, " Device type : 0x%lx", dp_type);
        val_print(TRACE, " Found CXL DVSEC (ID=0x%x)", dvsec_id);

        switch (dvsec_id) {
        case CXL_DVSEC_ID_DEVICE:
          val_cxl_parse_device_dvsec(bdf, next_cap_offset, dp_type);
          break;
        case CXL_DVSEC_ID_REGISTER_LOCATOR:
          val_cxl_parse_register_locator(bdf, next_cap_offset, dp_type);
          break;
        default:
          break;
        }

        next_cap_offset = next_ptr;
      }

      if (!found)
        return ACS_STATUS_SKIP;

      if (g_cxl_info_table != NULL)
        val_cxl_assign_host_bridge_indices();

  }
  return ACS_STATUS_PASS;
}

static uint32_t
val_cxl_host_discover_capabilities(CXL_INFO_BLOCK *entry)
{
  if (entry == NULL)
    return ACS_STATUS_ERR;

  if ((entry->component_reg_base == 0u) || (entry->component_reg_length == 0u)) {
    entry->hdm_decoder_count = 0;
    return ACS_STATUS_PASS;
  }

  if (val_mmu_update_entry(entry->component_reg_base,
                                   entry->component_reg_length, DEVICE_nGnRnE) != ACS_STATUS_PASS)
    return ACS_STATUS_ERR;
  return ACS_STATUS_PASS;
}



/**
  @brief   Create the VAL CXL info table by delegating to the platform PAL.
  @param  cxl_info_table  Caller-provided buffer to populate.
**/
void
val_cxl_create_info_table(uint64_t *cxl_info_table)
{

  uint32_t num_cxl_hb = 0;
  uint32_t index;
  uint32_t window;

  if (cxl_info_table == NULL) {
    val_print(ERROR, " CXL_INFO: Input table pointer is NULL ");
    return;
  }
  g_cxl_info_table = (CXL_INFO_TABLE *)cxl_info_table;

  pal_cxl_create_info_table(g_cxl_info_table);

  /* Print parsed informationi*/
  num_cxl_hb = g_cxl_info_table->num_entries;
  val_print(TRACE, "\n CXL_INFO: Number of host bridges :    %u\n", num_cxl_hb);

  if (num_cxl_hb == 0)
    return;

  for (index = 0; index < num_cxl_hb; index++) {
    CXL_INFO_BLOCK *entry = &g_cxl_info_table->device[index];

    if (val_cxl_host_discover_capabilities(entry) != ACS_STATUS_PASS) {
      val_print(ERROR,
                " CXL_INFO: Failed to map host bridge component window (UID 0x%x)",
                entry->uid);
    }

    val_print(TRACE, " \nCXL_INFO: Host Bridge[%u]\n", index);
    val_print(TRACE, "   UID                : 0x%x\n", entry->uid);
    val_print(TRACE, "   Structure Type     : 0x%x\n", entry->cxl_struct_type);
    val_print(TRACE, "   Component Type     : 0x%x\n", entry->component_reg_type);
    val_print(TRACE, "   Component Base     : 0x%llx\n", entry->component_reg_base);
    val_print(TRACE, "   Component Length   : 0x%llx\n", entry->component_reg_length);
    val_print(TRACE, "   Revision           : 0x%x\n", entry->cxl_version);

    val_print(TRACE, "   CFMWS Count        : %u", entry->cfmws_count);

    for (window = 0; window < entry->cfmws_count && window < CXL_MAX_CFMWS_WINDOWS; window++) {
      val_print(TRACE, "     CFMWS Index       : %u\n", window);
      val_print(TRACE, "       Base            : 0x%llx\n", entry->cfmws_base[window]);
      val_print(TRACE, "       Length          : 0x%llx\n", entry->cfmws_length[window]);
    }
  }

  val_cxl_create_table();
  val_cxl_print_component_summary();
}

/**
  @brief   Release VAL-side state associated with the CXL info table.
**/
void
val_cxl_free_info_table(void)
{
    val_cxl_free_component_table();

    if (g_cxl_info_table != NULL) {
        val_memory_free_aligned((void *)g_cxl_info_table);
        g_cxl_info_table = NULL;
    }
}

/**
  @brief   Fetch a value from the cached CXL info table.
  @param  type   Selector from CXL_INFO_e.
  @param  index  Host bridge index for per-entry selectors.
  @return Requested value or 0 on failure.
**/
uint64_t
val_cxl_get_info(CXL_INFO_e type, uint32_t index)
{

  if (g_cxl_info_table == NULL) {
    val_print(ERROR, " GET_CXL_INFO: CXL info table is not created ");
    return 0;
  }

  if ((type != CXL_INFO_NUM_DEVICES) && (index >= g_cxl_info_table->num_entries)) {
    val_print(ERROR, " GET_CXL_INFO: Invalid index %d ", index);
    return 0;
  }

  switch (type) {
  case CXL_INFO_NUM_DEVICES:
    return g_cxl_info_table->num_entries;
  case CXL_INFO_COMPONENT_BASE:
    return g_cxl_info_table->device[index].component_reg_base;
  case CXL_INFO_COMPONENT_LENGTH:
    return g_cxl_info_table->device[index].component_reg_length;
  case CXL_INFO_COMPONENT_TYPE:
    return g_cxl_info_table->device[index].component_reg_type;
  case CXL_INFO_DEVICE_TYPE:
    return g_cxl_info_table->device[index].cxl_struct_type;
  case CXL_INFO_HDM_COUNT:
    return g_cxl_info_table->device[index].hdm_decoder_count;
  case CXL_INFO_UID:
    return g_cxl_info_table->device[index].uid;
  case CXL_INFO_VERSION:
    return g_cxl_info_table->device[index].cxl_version;
  default:
    val_print(ERROR, " GET_CXL_INFO: Unsupported info type %d ", type);
    break;
  }

  return 0;
}

/**
  @brief  Return number of CFMWS windows associated with a host bridge.
  @param  host_index  Host bridge index in CXL info table.
  @return CFMWS window count or 0 on error.
**/
uint32_t
val_cxl_get_cfmws_count(uint32_t host_index)
{
  if (g_cxl_info_table == NULL)
    return 0;

  if (host_index >= g_cxl_info_table->num_entries)
    return 0;

  CXL_INFO_BLOCK *entry = &g_cxl_info_table->device[host_index];
  uint32_t count = entry->cfmws_count;
  uint32_t max_windows = (uint32_t)(sizeof(entry->cfmws_base) /
                                    sizeof(entry->cfmws_base[0]));

  if (count > max_windows)
    count = max_windows;

  return count;
}

uint32_t
val_cxl_get_cfmws(uint32_t index,
                  uint32_t window_index,
                  uint64_t *base,
                  uint64_t *length)
{
  if ((g_cxl_info_table == NULL) || (base == NULL) || (length == NULL))
    return 1;

  if (index >= g_cxl_info_table->num_entries)
    return 1;

  CXL_INFO_BLOCK *entry = &g_cxl_info_table->device[index];

  if (window_index >= entry->cfmws_count)
    return 1;

  if (window_index >= (uint32_t)(sizeof(entry->cfmws_base) /
                                 sizeof(entry->cfmws_base[0])))
    return 1;

  *base   = entry->cfmws_base[window_index];
  *length = entry->cfmws_length[window_index];
  return 0;
}

/**
  @brief  Return base and length of a CFMWS window for a host bridge.
  @param  host_index    Host bridge index in CXL info table.
  @param  window_index  CFMWS slot index.
  @param  base          Output base HPA.
  @param  length        Output window length.
  @return ACS_STATUS_PASS on success.
**/
uint32_t
val_cxl_get_cfmws_window(uint32_t host_index, uint64_t *base, uint64_t *size)
{
  uint32_t window_count, idx;
  uint64_t candidate_base;
  uint64_t candidate_size;

  window_count = val_cxl_get_cfmws_count(host_index);
  for (idx = 0; idx < window_count; ++idx)
  {
    candidate_base = 0;
    candidate_size = 0;

    if (val_cxl_get_cfmws(host_index, idx, &candidate_base, &candidate_size) != 0u)
      continue;

    if ((candidate_base == 0) || (candidate_size == 0))
      continue;

    if (((candidate_base | candidate_size) & CXL_HDM_ALIGNMENT_MASK) != 0u)
      continue;

    val_print(TRACE, "\n      CFMWS Base address is : 0x%llx", candidate_base);
    val_print(TRACE, "\n      CFMWS address size is : 0x%llx", candidate_size);
    *base = candidate_base;
    *size = candidate_size;
    return ACS_STATUS_PASS;
  }

  return ACS_STATUS_SKIP;

}

/**
  @brief  Return if a CXL device is cache capable or not

  @param  bdf    BDf of CXL device.

  @return 1 if cache capable. else return 0
**/
uint32_t
val_cxl_device_cache_capable(uint32_t bdf)
{
  uint32_t dvsec_off;
  uint32_t reg_value;
  uint32_t cxl_caps;

  if (val_cxl_find_capability(bdf, CXL_DVSEC_ID_DEVICE, &dvsec_off)) {
      val_print(DEBUG, "DVSEC Capability not found for bdf 0x%x", bdf);
      return 0;
  }

  val_pcie_read_cfg(bdf, dvsec_off + CXL_DVSEC_HDR2_OFFSET, &reg_value);
  cxl_caps = (reg_value >> CXL_DVSEC_CXL_CAPABILITY_SHIFT) & CXL_DVSEC_CXL_CAPABILITY_MASK;

  return ((cxl_caps & CXL_DVSEC_CXL_CAP_CACHE_CAPABLE) != 0);
}

uint32_t
val_cxl_check_persistent_memory(uint32_t index)
{
  uint32_t window_count, idx;
  uint32_t memory_type;

  if (g_cxl_info_table == NULL)
    return 1;

  if (index >= g_cxl_info_table->num_entries)
    return 1;

  CXL_INFO_BLOCK *entry = &g_cxl_info_table->device[index];

  window_count = val_cxl_get_cfmws_count(index);
  for (idx = 0; idx < window_count; ++idx)
  {
      memory_type = entry->cfmws_window[idx];
      if (((memory_type & PERSISTENT_MASK) >> PERSISTENT_SHIFT) == 1)
          return 0;
  }
  return 1;
}

uint32_t
val_cxl_map_hdm_address(uint64_t base, uint64_t length, volatile uint64_t **virt_out)
{
  memory_region_descriptor_t mem_desc[2];
  pgt_descriptor_t pgt_desc;
  uint64_t page_size;
  uint64_t range_end;
  uint64_t aligned_base;
  uint64_t aligned_length;
  uint64_t *aligned_va;
  uint64_t ttbr;

  if ((base == 0) || (length == 0) || (virt_out == NULL))
    return ACS_STATUS_ERR;

  page_size = val_memory_page_size();
  if (page_size == 0)
    return ACS_STATUS_ERR;

  range_end = base + length;
  if (range_end < base)
    return ACS_STATUS_ERR;

  aligned_base = base & ~(page_size - 1);

  {
    uint64_t aligned_end = range_end + page_size - 1;
    if (aligned_end < range_end)
      return ACS_STATUS_ERR;
    aligned_length = (aligned_end & ~(page_size - 1u)) - aligned_base;
  }

  val_memory_set(mem_desc, sizeof(mem_desc), 0);

  aligned_va = val_aligned_alloc(MEM_ALIGN_4K, page_size);
  if (!aligned_va)
    return ACS_STATUS_ERR;

  mem_desc[0].virtual_address  = (uint64_t)aligned_va;
  mem_desc[0].physical_address = aligned_base;
  mem_desc[0].length           = aligned_length;
  mem_desc[0].attributes       = PGT_WB;

  if (val_pe_reg_read_tcr(0, &pgt_desc.tcr))
    return ACS_STATUS_ERR;

  if (val_pe_reg_read_ttbr(0, &ttbr))
    return ACS_STATUS_ERR;

  pgt_desc.pgt_base = (ttbr & AARCH64_TTBR_ADDR_MASK);
  pgt_desc.mair     = val_pe_reg_read(MAIR_ELx);
  pgt_desc.stage    = PGT_STAGE1;
  pgt_desc.ias      = 48;
  pgt_desc.oas      = 48;

  if (val_pgt_create(mem_desc, &pgt_desc))
    return ACS_STATUS_ERR;

  *virt_out = (volatile uint64_t *)(aligned_va + (base - aligned_base));
  return ACS_STATUS_PASS;
}
