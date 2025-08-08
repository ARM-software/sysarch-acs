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

#include "include/acs_val.h"
#include "include/acs_exerciser.h"
#include "include/acs_pcie.h"
#include "include/acs_smmu.h"
#include "include/acs_iovirt.h"
#include "include/acs_memory.h"

EXERCISER_INFO_TABLE g_exerciser_info_table;

extern uint32_t pcie_bdf_table_list_flag;

/**
  @brief   Return a string representing the vendor name for a given vendor ID
**/
char8_t *lookup_vendor_name(uint32_t vendor_id)
{
    switch (vendor_id) {
    case 0x13B5:
        return "Arm Ltd";
    case 0x14AB:
        return "Siemens AG";
    case 0x16C3:
        return "Synopsys, Inc.";
    case 0x17CD:
        return "Cadence Design Systems";
    default:
        return "Unknown Vendor";
    }
}

/**
  @brief   This API popultaes information from all the PCIe stimulus generation IP available
           in the system into exerciser_info_table structure
  @param   exerciser_info_table - Table pointer to be filled by this API
  @return  exerciser_info_table - Contains info to communicate with stimulus generation hardware
**/
uint32_t val_exerciser_create_info_table(void)
{
  uint32_t Bdf;
  uint32_t reg_value;
  uint32_t num_bdf, num_ecam;
  uint32_t num_exerciser_info = 0;
  pcie_device_bdf_table *bdf_table;
  char8_t *vendor_name;
  uint32_t vendor_id;

  num_ecam = (uint32_t)val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0);
  if (num_ecam == 0)
  {
      val_print(ACS_PRINT_ERR, "       No ECAMs discovered\n ", 0);
      return 1;
  }

  bdf_table = val_pcie_bdf_table_ptr();
  /* if no bdf table ptr return error */
  if (bdf_table->num_entries == 0)
  {
      val_print(ACS_PRINT_DEBUG, "\n       No BDFs discovered            ", 0);
      return 1;
  }

  num_bdf = bdf_table->num_entries;
  while (num_bdf-- != 0)
  {

      Bdf = bdf_table->device[num_bdf].bdf;
      /* Probe pcie device Function with this bdf */
      if (val_pcie_read_cfg(Bdf, TYPE01_VIDR, &reg_value) == PCIE_NO_MAPPING)
      {
          /* Return if there is a bdf mapping issue */
          val_print(ACS_PRINT_ERR, "\n      BDF 0x%x mapping issue", Bdf);
          return 1;
      }

      /* Store the Function's BDF if there was a valid response */
      if (pal_is_bdf_exerciser(Bdf))
      {
          g_exerciser_info_table.e_info[num_exerciser_info].bdf = Bdf;
          g_exerciser_info_table.e_info[num_exerciser_info++].initialized = 0;
          vendor_id = (reg_value >> TYPE01_VIDR_SHIFT) & TYPE01_VIDR_MASK;
          vendor_name = lookup_vendor_name(vendor_id);
          val_print(ACS_PRINT_DEBUG, "    exerciser Bdf %x\n", Bdf);
          val_print(ACS_PRINT_DEBUG, "    Vendor ID: 0x%04x, ", vendor_id);
          val_print(ACS_PRINT_DEBUG, "Vendor Name: ", 0);
          val_print(ACS_PRINT_DEBUG, vendor_name, 0);
          val_print(ACS_PRINT_DEBUG, "\n\n", 0);
      }
  }
  g_exerciser_info_table.num_exerciser = num_exerciser_info;
  val_print(ACS_PRINT_TEST, " PCIE_INFO: Number of exerciser cards : %4d \n",
                                                             g_exerciser_info_table.num_exerciser);
  return 0;
}

/**
  @brief   This API returns the requested information about the PCIe stimulus hardware
  @param   type         - Information type required from the stimulus hadrware
  @return  value        - Information value for input type
**/
uint32_t val_exerciser_get_info(EXERCISER_INFO_TYPE type)
{
    switch (type) {
    case EXERCISER_NUM_CARDS:
         return g_exerciser_info_table.num_exerciser;
    default:
         return 0;
    }
}

/**
  @brief   This API writes the configuration parameters of the PCIe stimulus generation hardware
  @param   type         - Parameter type that needs to be set in the stimulus hadrware
  @param   value1       - Parameter 1 that needs to be set
  @param   value2       - Parameter 2 that needs to be set
  @param   instance     - Stimulus hardware instance number
  @return  status       - SUCCESS if the input paramter type is successfully written
**/
uint32_t val_exerciser_set_param(EXERCISER_PARAM_TYPE type, uint64_t value1, uint64_t value2,
                                 uint32_t instance)
{
    uint32_t status;

    status = pal_exerciser_set_param(type, value1, value2,
                                   g_exerciser_info_table.e_info[instance].bdf);
    val_mem_issue_dsb();
    return status;
}

/**
  @brief   This API returns the bdf of the PCIe Stimulus generation hardware
  @param   instance  - Stimulus hardware instance number
  @return  bdf       - BDF Number of the instance
**/
uint32_t val_exerciser_get_bdf(uint32_t instance)
{
    return g_exerciser_info_table.e_info[instance].bdf;
}
/**
  @brief   This API reads the configuration parameters of the PCIe stimulus generation hardware
  @param   type         - Parameter type that needs to be read from the stimulus hadrware
  @param   value1       - Parameter 1 that is read from hardware
  @param   value2       - Parameter 2 that is read from hardware
  @param   instance     - Stimulus hardware instance number
  @return  status       - SUCCESS if the requested paramter type is successfully read
**/
uint32_t val_exerciser_get_param(EXERCISER_PARAM_TYPE type, uint64_t *value1, uint64_t *value2,
                                 uint32_t instance)
{
    return pal_exerciser_get_param(type, value1, value2,
                                   g_exerciser_info_table.e_info[instance].bdf);

}

/**
  @brief   This API obtains the state of the PCIe stimulus generation hardware
  @param   state        - State that is read from the stimulus hadrware
  @param   instance     - Stimulus hardware instance number
  @return  status       - SUCCESS if the state is successfully read from hardware
**/
uint32_t val_exerciser_get_state(EXERCISER_STATE *state, uint32_t instance)
{
    return pal_exerciser_get_state(state, g_exerciser_info_table.e_info[instance].bdf);
}

/**
  @brief   This API obtains initializes
  @param   instance     - Stimulus hardware instance number
  @return  status       - SUCCESS if the state is successfully read from hardware
**/
uint32_t val_exerciser_init(uint32_t instance)
{
  uint32_t Bdf;
  uint64_t Ecam;
  uint64_t cfg_addr;
  EXERCISER_STATE state;

  if (!g_exerciser_info_table.e_info[instance].initialized)
  {
      Bdf = g_exerciser_info_table.e_info[instance].bdf;
      if (val_exerciser_get_state(&state, instance) || (state != EXERCISER_ON)) {
          val_print(ACS_PRINT_ERR, "\n   Exerciser Bdf %lx not ready", Bdf);
          return 1;
      }

      // setting command register for Memory Space Enable and Bus Master Enable
      Ecam = val_pcie_get_ecam_base(Bdf);

      /* There are 8 functions / device, 32 devices / Bus and each has a 4KB config space */
      cfg_addr = (PCIE_EXTRACT_BDF_BUS(Bdf) * PCIE_MAX_DEV * PCIE_MAX_FUNC * 4096) + \
                 (PCIE_EXTRACT_BDF_DEV(Bdf) * PCIE_MAX_FUNC * 4096) + \
                 (PCIE_EXTRACT_BDF_FUNC(Bdf) * 4096);

      pal_mmio_write((Ecam + cfg_addr + COMMAND_REG_OFFSET),
                  (pal_mmio_read((Ecam + cfg_addr) + COMMAND_REG_OFFSET) | BUS_MEM_EN_MASK));

      g_exerciser_info_table.e_info[instance].initialized = 1;
  }
  else
           val_print(ACS_PRINT_INFO, "\n       Already initialized %2d",
                                                                    instance);
  return 0;
}
/**
  @brief   This API performs the input operation using the PCIe stimulus generation hardware
  @param   ops          - Operation that needs to be performed with the stimulus hadrware
  @param   value        - Additional information to perform the operation
  @param   instance     - Stimulus hardware instance number
  @return  status       - SUCCESS if the operation is successfully performed using the hardware
**/
uint32_t val_exerciser_ops(EXERCISER_OPS ops, uint64_t param, uint32_t instance)
{
    uint32_t status;

    status = pal_exerciser_ops(ops, param, g_exerciser_info_table.e_info[instance].bdf);
    val_mem_issue_dsb();
    return status;
}

/**
  @brief   This API returns test specific data from the PCIe stimulus generation hardware
  @param   type         - data type for which the data needs to be returned
  @param   data         - test specific data to be be filled by pal layer
  @param   instance     - Stimulus hardware instance number
  @return  status       - SUCCESS if the requested data is successfully filled
**/
uint32_t val_exerciser_get_data(EXERCISER_DATA_TYPE type, exerciser_data_t *data,
                                uint32_t instance)
{
    uint32_t bdf = g_exerciser_info_table.e_info[instance].bdf;
    uint64_t ecam = val_pcie_get_ecam_base(bdf);
    return pal_exerciser_get_data(type, data, bdf, ecam);
}

uint32_t val_get_exerciser_err_info(EXERCISER_ERROR_CODE type)
{
    switch (type) {
    case CORR_RCVR_ERR:
         return CORR_RCVR_ERR_OFFSET;
    case CORR_BAD_TLP:
         return CORR_BAD_TLP_OFFSET;
    case CORR_BAD_DLLP:
         return CORR_BAD_DLLP_OFFSET;
    case CORR_RPL_NUM_ROLL:
         return CORR_RPL_NUM_ROLL_OFFSET;
    case CORR_RPL_TMR_TIMEOUT:
         return CORR_RPL_TMR_TIMEOUT_OFFSET;
    case CORR_ADV_NF_ERR:
         return CORR_ADV_NF_ERR_OFFSET;
    case CORR_INT_ERR:
         return CORR_INT_ERR_OFFSET;
    case CORR_HDR_LOG_OVRFL:
         return CORR_HDR_LOG_OVRFL_OFFSET;
    case UNCORR_DL_ERROR:
         return UNCORR_DL_ERROR_OFFSET;
    case UNCORR_SD_ERROR:
         return UNCORR_SD_ERROR_OFFSET;
    case UNCORR_PTLP_REC:
         return UNCORR_PTLP_REC_OFFSET;
    case UNCORR_FL_CTRL_ERR:
         return UNCORR_FL_CTRL_ERR_OFFSET;
    case UNCORR_CMPT_TO:
         return UNCORR_CMPT_TO_OFFSET;
    case UNCORR_AMPT_ABORT:
         return UNCORR_AMPT_ABORT_OFFSET;
    case UNCORR_UNEXP_CMPT:
         return UNCORR_UNEXP_CMPT_OFFSET;
    case UNCORR_RCVR_ERR:
         return UNCORR_RCVR_ERR_OFFSET;
    case UNCORR_MAL_TLP:
         return UNCORR_MAL_TLP_OFFSET;
    case UNCORR_ECRC_ERR:
         return UNCORR_ECRC_ERR_OFFSET;
    case UNCORR_UR:
         return UNCORR_UR_OFFSET;
    case UNCORR_ACS_VIOL:
         return UNCORR_ACS_VIOL_OFFSET;
    case UNCORR_INT_ERR:
         return UNCORR_INT_ERR_OFFSET;
    case UNCORR_MC_BLK_TLP:
         return UNCORR_MC_BLK_TLP_OFFSET;
    case UNCORR_ATOP_EGR_BLK:
         return UNCORR_ATOP_EGR_BLK_OFFSET;
    case UNCORR_TLP_PFX_EGR_BLK:
         return UNCORR_TLP_PFX_EGR_BLK_OFFSET;
    case UNCORR_PTLP_EGR_BLK:
         return UNCORR_PTLP_EGR_BLK_OFFSET;
    default:
         val_print(ACS_PRINT_ERR, "\n   Invalid error offset ", 0);
         return 0;
    }
}

/**
  @brief   This API disables the RP-PIO register support of the RP
  @param   type         - RP BDF of which the RP-PIO needs to be disabled
  @return  None
**/
void val_exerciser_disable_rp_pio_register(uint32_t bdf)
{

  pal_exerciser_disable_rp_pio_register(bdf);
  return;
}

uint32_t
val_exerciser_check_poison_data_forwarding_support(void)
{
  return pal_exerciser_check_poison_data_forwarding_support();
}

uint32_t
val_exerciser_get_pcie_ras_compliant_err_node(uint32_t bdf, uint32_t rp_bdf)
{
  return pal_exerciser_get_pcie_ras_compliant_err_node(bdf, rp_bdf);
}

uint64_t
val_exerciser_get_ras_status(uint32_t ras_node, uint32_t e_bdf, uint32_t erp_bdf)
{
  return pal_exerciser_get_ras_status(ras_node, e_bdf, erp_bdf);
}

uint32_t
val_exerciser_set_bar_response(uint32_t bdf)
{
  return pal_exerciser_set_bar_response(bdf);
}
