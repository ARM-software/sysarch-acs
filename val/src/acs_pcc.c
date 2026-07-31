/** @file
 * Copyright (c) 2024-2026, Arm Limited or its affiliates. All rights reserved.
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

/* This file contains APIs used by other SBSA modules/components */

#include "acs_val.h"
#include "acs_common.h"
#include "acs_memory.h"
#include "acs_mmu.h"
#include "val_interface.h"

static PCC_INFO_TABLE *g_pcc_info_table;

/* Doorbell registers, Command complete update/check registers are represented in GAS format.
   Each GAS format describes it's own access_size. Perform read/write based on the access_size */
static uint64_t
val_pcc_read_gas_register(const ACPI_GENERIC_ADDRESS_STRUCTURE *gas)
{
  if (gas->access_size == 1U)
      return val_mmio_read8(gas->addr);

  if (gas->access_size == 2U)
      return val_mmio_read16(gas->addr);

  if ((gas->access_size == 4U) ||
      ((gas->access_size == 0U) &&
       ((gas->reg_bit_offset + gas->reg_bit_width) > 32U)))
      return val_mmio_read64(gas->addr);

  return val_mmio_read(gas->addr);
}

static void
val_pcc_write_gas_register(const ACPI_GENERIC_ADDRESS_STRUCTURE *gas, uint64_t value)
{
  if (gas->access_size == 1U) {
      val_mmio_write8(gas->addr, (uint8_t)value);
      return;
  }

  if (gas->access_size == 2U) {
      val_mmio_write16(gas->addr, (uint16_t)value);
      return;
  }

  if ((gas->access_size == 4U) ||
      ((gas->access_size == 0U) &&
       ((gas->reg_bit_offset + gas->reg_bit_width) > 32U))) {
      val_mmio_write64(gas->addr, value);
      return;
  }

  val_mmio_write(gas->addr, (uint32_t)value);
}

/**
  @brief  Map the pages containing a PCC Generic Address Structure register.

  @param  name  Register name used in diagnostic messages.
  @param  gas   Generic Address Structure describing the register.

  @return PCC_STATUS_SUCCESS on success, RETURN_FAILURE on failure.
**/
static uint32_t
val_pcc_map_gas_register(const char *name, const ACPI_GENERIC_ADDRESS_STRUCTURE *gas)
{
  uint32_t access_bytes;
  uint32_t field_bits;
  uint32_t map_size;
  uint64_t page_base;
  uint64_t page_offset;

  if (gas->addr_space_id != PCC_GAS_SYSTEM_MEMORY) {
      val_print(ERROR, "\n    PCC: %a GAS uses unsupported address space 0x%x",
                name, gas->addr_space_id);
      return RETURN_FAILURE;
  }

  if (gas->addr == 0U) {
      val_print(ERROR, "\n    PCC: %a GAS has a null address", name);
      return RETURN_FAILURE;
  }

  /* GAS AccessSize values 1 through 4 encode byte, word, dword, and qword. */
  field_bits = gas->reg_bit_offset + gas->reg_bit_width;
  if ((gas->access_size >= PCC_GAS_ACCESS_SIZE_MIN) &&
      (gas->access_size <= PCC_GAS_ACCESS_SIZE_MAX))
      access_bytes = 1U << (gas->access_size - PCC_GAS_ACCESS_SIZE_MIN);
  else if (gas->access_size == 0U)
      access_bytes = (field_bits > 32U) ? sizeof(uint64_t) : sizeof(uint32_t);
  else {
      val_print(ERROR, "\n    PCC: %a GAS has unsupported AccessSize 0x%x",
                name, gas->access_size);
      return RETURN_FAILURE;
  }

  if (field_bits > (access_bytes * PCC_BITS_PER_BYTE)) {
      val_print(ERROR, "\n    PCC: %a GAS field exceeds its access width", name);
      return RETURN_FAILURE;
  }

  page_base = gas->addr & ~((uint64_t)SIZE_4KB - 1U);
  page_offset = gas->addr - page_base;

  /* A GAS register is at most eight bytes, so it can span no more than two pages. */
  map_size = (page_offset + access_bytes > SIZE_4KB) ? (2U * SIZE_4KB) : SIZE_4KB;

  val_print(DEBUG, "\n    PCC: mapping %a GAS page=0x%llx size=0x%x address=0x%llx",
            name, page_base, map_size, gas->addr);
  return val_mmu_update_entry(page_base, map_size, DEVICE_nGnRnE);
}

/**
  @brief  Map the PCC shared communication region as Device memory.

  @param  subspace  PCC Type 3 subspace describing the communication region.

  @return PCC_STATUS_SUCCESS on success, RETURN_FAILURE on failure.
**/
static uint32_t
val_pcc_map_shared_memory(const PCC_SUBSPACE_TYPE_3 *subspace)
{
  uint64_t last_addr;
  uint64_t page_addr;
  uint64_t last_page;

  if ((subspace->base_addr == 0U) || (subspace->memory_length == 0U)) {
      val_print(ERROR, "\n    PCC: shared memory has an invalid base or length");
      return RETURN_FAILURE;
  }

  /* Check the inclusive end-address calculation before aligning the range. */
  if (subspace->base_addr > (~0ULL - ((uint64_t)subspace->memory_length - 1U))) {
      val_print(ERROR, "\n    PCC: shared memory range overflows the address space");
      return RETURN_FAILURE;
  }

  last_addr = subspace->base_addr + subspace->memory_length - 1U;
  page_addr = subspace->base_addr & ~((uint64_t)SIZE_4KB - 1U);
  last_page = last_addr & ~((uint64_t)SIZE_4KB - 1U);

  val_print(DEBUG,
            "\n    PCC: mapping shared memory base=0x%llx length=0x%x",
            subspace->base_addr, subspace->memory_length);

  /* Check every page because an earlier mapping may cover only part of the region. */
  while (page_addr <= last_page) {
      if (val_mmu_update_entry(page_addr, SIZE_4KB, DEVICE_nGnRnE) !=
          PCC_STATUS_SUCCESS) {
          val_print(ERROR, "\n    PCC: failed to map shared memory page 0x%llx",
                    page_addr);
          return RETURN_FAILURE;
      }

      if (page_addr == last_page)
          break;

      page_addr += SIZE_4KB;
  }

  return PCC_STATUS_SUCCESS;
}

/**
  @brief  Wait for the platform to return ownership of a PCC channel.

  @param  subspace           PCC subspace containing the completion register.
  @param  phase              Text used to identify the poll in debug output.
  @param  initial_delay_usec Delay before the first completion check.

  @return PCC_STATUS_SUCCESS when command complete is set, or
          RETURN_FAILURE after the timeout.
**/
static uint32_t
val_pcc_wait_for_completion(const PCC_SUBSPACE_TYPE_3 *subspace, const char *phase,
                            uint32_t delay_usec)
{
  uint64_t cmd_complete;
  uint64_t cmd_complete_raw;
  uint32_t remaining_usec;
  uint32_t waited_usec = 0U;

  /* Keep the advisory initial delay within the overall polling timeout. */
  if (delay_usec > PCC_COMMAND_TIMEOUT_USEC) {
      delay_usec = PCC_COMMAND_TIMEOUT_USEC;
  }

  /* Initial delay before the agent can probe for command completion. */
  if (delay_usec != 0U) {
      val_time_delay_ms(delay_usec);
      waited_usec = delay_usec;
  }

  /* Probe the command complete check register to verify that the platform has processed the command
     and agent can read the response back */
  while (waited_usec <= PCC_COMMAND_TIMEOUT_USEC) {
      cmd_complete_raw = val_pcc_read_gas_register(&subspace->cmd_complete_chk_reg);
      cmd_complete = cmd_complete_raw & subspace->cmd_complete_chk_mask;
      val_print(DEBUG,
                "\n    PCC: %a poll raw=0x%llx masked=0x%llx waited=%u us",
                phase, cmd_complete_raw, cmd_complete, waited_usec);

      if (cmd_complete != 0U) {
          return PCC_STATUS_SUCCESS;
      }

      if (waited_usec >= PCC_COMMAND_TIMEOUT_USEC) {
          break;
      }

      /* Poll at a fixed interval without extending the overall timeout. */
      remaining_usec = PCC_COMMAND_TIMEOUT_USEC - waited_usec;
      delay_usec = (remaining_usec < PCC_POLL_INTERVAL_USEC) ?
                   remaining_usec : PCC_POLL_INTERVAL_USEC;
      val_time_delay_ms(delay_usec);
      waited_usec += delay_usec;
  }

  return RETURN_FAILURE;
}

/**
  @brief  Initialize the PCC information table through PAL.
  @param  pcc_info_table  Caller-allocated storage for the PCC information.
**/
void
val_pcc_create_info_table(uint64_t *pcc_info_table)
{
  if (pcc_info_table == NULL) {
      val_print(ERROR, "\n    PCC: cannot create info table from a NULL buffer");
      return;
  }

  /* PAL populates the caller-owned buffer with the platform's PCCT data. */
  g_pcc_info_table = (PCC_INFO_TABLE *)pcc_info_table;
  pal_pcc_create_info_table(g_pcc_info_table);

  val_print(DEBUG, "\n    PCC: info table=0x%llx subspace count=%u",
            (uint64_t)g_pcc_info_table, g_pcc_info_table->subspace_cnt);
}

/**
  @brief  Find the PCC information-table entry for a subspace ID.
  @param  subspace_id  Subspace ID from the PCCT.
  @return Information-table index, or RETURN_FAILURE when not found.
**/
uint32_t
val_pcc_get_ss_info_idx(uint32_t subspace_id)
{
  uint32_t i;

  if (g_pcc_info_table == NULL) {
      val_print(ERROR, "\n    PCC: info table is NULL while looking up subspace %u", subspace_id);
      return RETURN_FAILURE;
  }

  /* looking up PCC subspace in available subspace entries */
  for (i = 0; i < g_pcc_info_table->subspace_cnt; i++) {
      if (g_pcc_info_table->pcc_info[i].subspace_idx == subspace_id) {
          val_print(DEBUG, "\n    PCC: subspace=%u found at table index=%u type=%u",
                    subspace_id, i, g_pcc_info_table->pcc_info[i].subspace_type);
          return i;
      }
  }

  val_print(ERROR, "\n    PCC: subspace=%u is not present in the info table", subspace_id);
  return RETURN_FAILURE;
}

/**
  @brief  Map the shared memory and registers for a PCC subspace.
  @param  subspace_id  Subspace ID used to index the PCCT array.
  @return  PCC_STATUS_SUCCESS on success, RETURN_FAILURE on failure.
**/
uint32_t
val_pcc_map_registers(uint32_t subspace_id)
{
  uint32_t pcc_idx;
  PCC_SUBSPACE_TYPE_3 *subspace;

  pcc_idx = val_pcc_get_ss_info_idx(subspace_id);
  if (pcc_idx == RETURN_FAILURE) {
      val_print(ERROR, "\n    PCC: cannot map registers for unknown subspace %u", subspace_id);
      return RETURN_FAILURE;
  }

  subspace = &g_pcc_info_table->pcc_info[pcc_idx].type_spec_info.pcc_ss_type_3;

  if (val_pcc_map_shared_memory(subspace) != PCC_STATUS_SUCCESS)
      return RETURN_FAILURE;

  if (val_pcc_map_gas_register("doorbell", &subspace->doorbell_reg) != PCC_STATUS_SUCCESS)
      return RETURN_FAILURE;

  if (val_pcc_map_gas_register("command complete check",
                               &subspace->cmd_complete_chk_reg) != PCC_STATUS_SUCCESS)
      return RETURN_FAILURE;

  if (val_pcc_map_gas_register("command complete update",
                               &subspace->cmd_complete_update_reg) != PCC_STATUS_SUCCESS)
      return RETURN_FAILURE;

  return PCC_STATUS_SUCCESS;
}

/**
  @brief  Submit a command using the ACPI PCC doorbell protocol.

  @param  subspace_id  Subspace ID from the PCCT.
  @param  command      Protocol message header.
  @param  data         Payload copied into the shared communication region.
  @param  data_size    Payload size in bytes.

  @return Pointer to the response payload in shared memory, or NULL on failure.
**/
void
*val_pcc_cmd_response(uint32_t subspace_id, uint32_t command, void *data, uint32_t data_size)
{
  uint32_t pcc_idx;
  uint64_t shared_mem_addr;
  uint64_t response_addr;
  uint64_t cmd_complete_update_val;
  uint64_t doorbell_val;
  PCC_SUBSPACE_TYPE_3 *subspace;

  if ((data == NULL) && (data_size != 0U)) {
      val_print(ERROR, "\n    PCC: command payload is NULL but its size is %u", data_size);
      return NULL;
  }

  /* Resolve the PCCT subspace ID to its cached Type 3 descriptor. */
  pcc_idx = val_pcc_get_ss_info_idx(subspace_id);
  if (pcc_idx == RETURN_FAILURE) {
      return NULL;
  }

  subspace = &g_pcc_info_table->pcc_info[pcc_idx].type_spec_info.pcc_ss_type_3;

  /* Verify that the channel can hold the PCC header and command payload. */
  if ((subspace->memory_length < PCC_TY3_COMM_SPACE) ||
      (data_size > (subspace->memory_length - PCC_TY3_COMM_SPACE))) {
      val_print(ERROR,
                "\n    PCC: payload size %u exceeds channel memory length %u for subspace %u",
                data_size, subspace->memory_length, subspace_id);
      return NULL;
  }

  /* Note : For information on Doorbell Protocol refer ACPI 6.5 specification; section 14.5 */
  /* OSPM checks that there is no command pending completion and the subspace is free to use.
     A set completion bit means OSPM owns the shared-memory channel. */
  if (val_pcc_wait_for_completion(subspace, "pre-command",
                                  subspace->min_req_turnaround_usec) != PCC_STATUS_SUCCESS) {
      val_print(ERROR,
                "\n    PCC: channel remained busy for %u us for subspace id : 0x%x",
                PCC_COMMAND_TIMEOUT_USEC, subspace_id);
      return NULL;
  }

  shared_mem_addr = subspace->base_addr;

  /* The OSPM places a command into the shared memory of the subspace
     to update the flags, length, command and payload fields */
  /* Flags: 0 = Disable interrupt on command completion */
  val_mmio_write(shared_mem_addr + PCC_TY3_FLAGS_OFFSET, PCC_SCMI_TRANSPORT_FLAGS);

  /* Length: Length of the payload being transmitted including the command */
  val_mmio_write(shared_mem_addr + PCC_TY3_LENGTH_OFFSET,
                 PCC_SCMI_HEADER_SIZE + data_size);

  /* Command: Command being sent over the subspace. SCMI Header in case of MPAM Fb profile */
  val_mmio_write(shared_mem_addr + PCC_TY3_CMD_OFFSET, command);

  /* Payload: Copy the payload to the shared memory */
  if (data_size != 0U) {
      val_memcpy((void *)(shared_mem_addr + PCC_TY3_COMM_SPACE), data, data_size);
  }

  /* Make the complete request visible before transferring ownership. */
  val_mem_issue_dsb();

  /* Clear completion with the platform-provided preserve and set masks. */
  cmd_complete_update_val = val_pcc_read_gas_register(&subspace->cmd_complete_update_reg);
  cmd_complete_update_val =
      (cmd_complete_update_val & subspace->cmd_complete_update_preserve) |
      subspace->cmd_complete_update_set;
  val_pcc_write_gas_register(&subspace->cmd_complete_update_reg, cmd_complete_update_val);

  val_print(DEBUG, "\n    PCC: complete update write value=0x%llx",
            cmd_complete_update_val);

  /* Ring the doorbell with its independent preserve and write masks. */
  doorbell_val = val_pcc_read_gas_register(&subspace->doorbell_reg);
  doorbell_val = (doorbell_val & subspace->doorbell_preserve) |
                 subspace->doorbell_write;
  val_pcc_write_gas_register(&subspace->doorbell_reg, doorbell_val);

  val_print(DEBUG, "\n    PCC: doorbell write value=0x%llx", doorbell_val);

  /* NominalLatency delays the first check; the ACS timeout remains the limit. */
  if (val_pcc_wait_for_completion(subspace, "post-command",
                                  subspace->nominal_latency_usec) != PCC_STATUS_SUCCESS) {
      val_print(ERROR,
          "\n    PCC: command did not complete within %u us for subspace id : 0x%x",
          PCC_COMMAND_TIMEOUT_USEC, subspace_id);
      return NULL;
  }

  /* Completion transfers ownership back to OSPM; order response reads after it. */
  val_mem_issue_dsb();

  response_addr = shared_mem_addr + PCC_TY3_COMM_SPACE;
  val_print(DEBUG, "\n    PCC: command complete response=0x%llx", response_addr);
  return (void *)response_addr;
}

/**
  @brief  Free the memory allocated for the PCC information table.
**/
void
val_pcc_free_info_table(void)
{
  if (g_pcc_info_table != NULL) {
      pal_mem_free_aligned((void *)g_pcc_info_table);
      g_pcc_info_table = NULL;
  } else {
      val_print(ERROR,
                "\n WARNING: g_pcc_info_table pointer is already NULL");
  }
}
