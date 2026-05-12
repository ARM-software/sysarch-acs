/** @file
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
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

#include "acs_val.h"
#include "acs_common.h"
#include "val_interface.h"
#include "acs_memory.h"
#include "acs_pe.h"
#include "acs_ras.h"

#define TEST_NUM   (ACS_RAS_TEST_NUM_BASE + 3)
#define TEST_RULE  "RAS_03"
#define TEST_DESC  "Check FHI in Error Record Group       "

static
void
payload()
{

  uint32_t status;
  uint32_t fail_cnt = 0, compare_cnt = 0, sr_node_cnt = 0, first_sr_node = 0;
  uint64_t num_node;
  uint32_t node_index, sec_node;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint64_t intf_type;
  uint64_t fhi_id = 0, fhi_id_sec;
  uint64_t base_addr, base_addr_sec;
  uint32_t fhi_valid = 0;

  /* Get Number of nodes with RAS Functionality */
  status = val_ras_get_info(RAS_INFO_NUM_NODES, 0, &num_node);
  if (status || (num_node == 0)) {
    val_print(DEBUG, "\n       No RAS Nodes found in AEST table.");
    val_print(DEBUG, "\n       The test must be considered fail if system \
                                        components supports RAS nodes");
    val_set_status(index, RESULT_WARNING(01));
    return;
  }
  /*
   * SR nodes do not have an MMIO base address that can be used for grouping.
   * Count them up front so the test can distinguish between:
   *   - zero/one SR node: no SR-to-SR comparison is possible, so this is PASS.
   *   - two or more SR nodes: all SR nodes must report the same FHI.
   */
  for (node_index = 0; node_index < num_node; node_index++) {
    status = val_ras_get_info(RAS_INFO_INTF_TYPE, node_index, &intf_type);
    if (status) {
      val_print(ERROR, "\n       Could not get interface type for index %d", node_index);
      fail_cnt++;
      continue;
    }

    if (intf_type == RAS_INTERFACE_SR)
      sr_node_cnt++;
  }

  if (fail_cnt) {
    val_set_status(index, RESULT_FAIL(01));
    return;
  }

  /*
   * Validate the SR-node rule. The first SR node with an FHI becomes the
   * reference value; every subsequent SR node must expose the same FHI.
   * MMIO nodes are deliberately ignored in this loop because they are checked
   * separately below using the original same-error-record-group logic.
   */
  for (node_index = 0; node_index < num_node; node_index++) {
    status = val_ras_get_info(RAS_INFO_INTF_TYPE, node_index, &intf_type);
    if (status) {
      val_print(ERROR, "\n       Could not get interface type for index %d", node_index);
      fail_cnt++;
      continue;
    }

    if (intf_type != RAS_INTERFACE_SR)
      continue;

    /*
     * A single SR node, or one SR node mixed with MMIO nodes, has no second SR
     * node to compare against. That is compliant and must pass, not skip.
     */
    if (sr_node_cnt < 2)
      continue;

    /* Get FHI number for this SR Node */
    status = val_ras_get_info(RAS_INFO_FHI_ID, node_index, &fhi_id_sec);
    if (status) {
      val_print(ERROR, "\n       FHI not supported for SR node index %d", node_index);
      fail_cnt++;
      continue;
    }

    if (!fhi_valid) {
      fhi_id = fhi_id_sec;
      first_sr_node = node_index;
      fhi_valid = 1;
      continue;
    }

    /* Check if FHI is same for all SR nodes otherwise fail the test */
    compare_cnt++;
    if (fhi_id != fhi_id_sec) {
      val_print(ERROR, "\n       FHI different for SR node index %d", first_sr_node);
      val_print(ERROR, " : %d", node_index);
      fail_cnt++;
      continue;
    }
  }

  /*
   * Preserve the existing MMIO behavior: MMIO RAS nodes that belong to the
   * same Error Record Group share the same base address, and nodes in the same
   * group must report the same FHI. SR nodes are skipped here because they do
   * not provide RAS_INFO_BASE_ADDR.
   */
  for (node_index = 0; node_index < (num_node - 1); node_index++) {
    status = val_ras_get_info(RAS_INFO_INTF_TYPE, node_index, &intf_type);
    if (status) {
      val_print(ERROR, "\n       Could not get interface type for index %d", node_index);
      fail_cnt++;
      continue;
    }

    if (intf_type == RAS_INTERFACE_SR)
      continue;

    /* Get Current Node Base Address */
    status = val_ras_get_info(RAS_INFO_BASE_ADDR, node_index, &base_addr);
    if (status) {
      val_print(ERROR, "\n       Could not get base address for index %d", node_index);
      fail_cnt++;
      continue;
    }

    /* Get FHI number for this Node, If Not Skip the Node */
    status = val_ras_get_info(RAS_INFO_FHI_ID, node_index, &fhi_id);
    if (status) {
      val_print(DEBUG, "\n       FHI not supported for index %d", node_index);
      continue;
    }

    /* Compare with Rest of the node having same Base Address */
    for (sec_node = node_index + 1; sec_node < num_node; sec_node++) {
      status = val_ras_get_info(RAS_INFO_INTF_TYPE, sec_node, &intf_type);
      if (status) {
        val_print(ERROR, "\n       Could not get interface type for index %d", sec_node);
        fail_cnt++;
        continue;
      }

      if (intf_type == RAS_INTERFACE_SR)
        continue;

      /* Get Second Node Base Address */
      status = val_ras_get_info(RAS_INFO_BASE_ADDR, sec_node, &base_addr_sec);
      if (status) {
        val_print(ERROR, "\n       Could not get base address for index %d", sec_node);
        fail_cnt++;
        continue;
      }

      /* If not same base address then skip this node */
      if (base_addr != base_addr_sec)
        continue;

      /* Get FHI number for this Node */
      status = val_ras_get_info(RAS_INFO_FHI_ID, sec_node, &fhi_id_sec);
      if (status) {
        val_print(DEBUG, "\n       FHI not supported for index %d", sec_node);
        continue;
      }

      /* Check if FHI is same otherwise fail the test */
      compare_cnt++;
      if (fhi_id != fhi_id_sec) {
        val_print(ERROR, "\n       FHI different for Same Group index %d", node_index);
        val_print(ERROR, " : %d", sec_node);
        fail_cnt++;
        continue;
      }
    }
  }

  if (fail_cnt) {
    val_set_status(index, RESULT_FAIL(01));
    return;
  }

  if (!compare_cnt)
    val_print(DEBUG, "\n       No FHI comparison required.");

  val_set_status(index, RESULT_PASS);
}

uint32_t
ras003_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
