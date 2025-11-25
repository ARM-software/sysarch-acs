/** @file test_wrappers.h
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
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

/* @brief files declares test wrappers, which wraps multiple test entry functions required by
   single base rule into one single test entry function */


#ifndef TEST_WRAPPERS_H
#define TEST_WRAPPERS_H

uint32_t b_ppi_00_entry(uint32_t num_pe);
uint32_t b_wak_03_07_entry(uint32_t num_pe);
uint32_t s_l7mp_03_entry(uint32_t num_pe);
uint32_t sys_ras_2_entry(uint32_t num_pe);
uint32_t appendix_i_6_entry(uint32_t num_pe);
uint32_t p_l1pe_01_entry(uint32_t num_pe);
uint32_t ie_reg_1_entry(uint32_t num_pe);
uint32_t pci_ic_11_entry(uint32_t num_pe);
uint32_t pci_in_04_entry(uint32_t num_pe);
uint32_t pci_li_02_entry(uint32_t num_pe);
uint32_t pci_li_03_entry(uint32_t num_pe);
uint32_t pci_msi_2_entry(uint32_t num_pe);
uint32_t pci_pp_04_entry(uint32_t num_pe);
uint32_t pci_pp_05_entry(uint32_t num_pe);
uint32_t re_rec_1_entry(uint32_t num_pe);
uint32_t re_reg_1_entry(uint32_t num_pe);
uint32_t ie_reg_3_entry(uint32_t num_pe);
uint32_t pci_in_19_entry(uint32_t num_pe);
uint32_t pci_li_01_entry(uint32_t num_pe);
uint32_t pci_mm_01_entry(uint32_t num_pe);
uint32_t pci_mm_03_entry(uint32_t num_pe);
uint32_t ri_smu_1_entry(uint32_t num_pe);
uint32_t ie_reg_2_entry(uint32_t num_pe);
uint32_t ie_reg_4_entry(uint32_t num_pe);
uint32_t pci_in_13_entry(uint32_t num_pe);
uint32_t pci_in_17_entry(uint32_t num_pe);
uint32_t pci_in_05_entry(uint32_t num_pe);
uint32_t gpu_04_entry(uint32_t num_pe);

#endif // TEST_WRAPPERS_H
