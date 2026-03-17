/** @file
 * Copyright (c) 2019,2023-2026, Arm Limited or its affiliates. All rights reserved.
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

#include "acs_pcie.h"

/**
* The test table covers bit-field entries for registers
* which are applicable for only type0 header
**/

pcie_cfgreg_bitfield_entry bf_info_table49[] = {

    {
        HEADER,                                  // Part of Header type register
        0,                                       // Not applicable
        0,                                       // Not applicable
        0x04,                                    // Offset from ECAM base
        iEP_EP,                                  // Applicable to integrated endpoints
        3,                                       // Start bit position
        3,                                       // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "CR SCE value mismatch",                 // SCE invalid configured value
        "CR SCE attribute mismatch"              // SCE invalid attribute
     },

    // Bit-field entry 2: Command register, bit[4] = Memory Write and Invalidate
    {
        HEADER,                                  // Part of Header type register
        0,                                       // Not applicable
        0,                                       // Not applicable
        0x04,                                    // Offset from ECAM base
        iEP_EP,                                  // Applicable to integrated endpoints
        4,                                       // Start bit position
        4,                                       // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "CR MWI value mismatch",                 // MWI invalid configured value
        "CR MWI attribute mismatch"              // MWI invalid attribute
    },

    // Bit-field entry 3: Command register, bit[5] = VGA Palette Snoop
    {
        HEADER,                                  // Part of Header type register
        0,                                       // Not applicable
        0,                                       // Not applicable
        0x04,                                    // Offset from ECAM base
        iEP_EP,                                  // Applicable to integrated endpoints
        5,                                       // Start bit position
        5,                                       // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "CR VPS cfg value mismatch",             // VPS invalid configured value
        "CR VPS attribute mismatch"              // VPS invalid attribute
    },

    // Bit-field entry 4: Command register, bit[7] = IDSEL Stepping/Wait Cycle Control
    {
        HEADER,                                  // Part of Header type register
        0,                                       // Not applicable
        0,                                       // Not applicable
        0x04,                                    // Offset from ECAM base
        iEP_EP,                                  // Applicable to integrated endpoints
        7,                                       // Start bit position
        7,                                       // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "CR IDSEL value mismatch",               // IDSEL invalid configured value
        "CR IDSEL attribute mismatch"            // IDSEL invalid attribute
    },

    // Bit-field entry 5: Command register, bit[9] = Fast Back-to-Back Transaction Enable
    {
        HEADER,                                  // Part of Header type register
        0,                                       // Not applicable
        0,                                       // Not applicable
        0x04,                                    // Offset from ECAM base
        iEP_EP,                                  // Applicable to integrated endpoints
        9,                                       // Start bit position
        9,                                       // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "CR FBBTE value mismatch",               // FBBTE invalid configured value
        "CR FBBTE attribute mismatch"            // FBBTE invalid attribute
    },

    // Bit-field entry 6: Command register, bit[10] = interrupt disable
    {
        HEADER,                                  // Part of Header type register
        0,                                       // Not applicable
        0,                                       // Not applicable
        0x04,                                    // Offset from ECAM base
        iEP_EP,                                  // Applicable to integrated endpoints
        10,                                      // Start bit position
        10,                                      // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "CR ID value mismatch",                  // Interrupt disable invalid configured value
        "CR ID attribute mismatch"               // Interrupt disable invalid attribute
    },

    // Bit-field entry 7: Status register, bit[3] = Interrupt Status
    {
        HEADER,                                  // Part of Header type register
        0,                                       // Not applicable
        0,                                       // Not applicable
        0x06,                                    // Offset from ECAM base
        iEP_EP,                                  // Applicable to integrated endpoints
        3,                                       // Start bit position
        3,                                       // End bit position
        0,                                       // Hardwired to 1b
        READ_ONLY,                               // Attribute is Read-only
        "SR IS value mismatch",                  // Interrupt Status invalid configured value
        "SR IS attribute mismatch"               // Interrupt Status invalid attribute
    },

    // Bit-field entry 8: Status register, bit[4] = Capabilities List
    {
        HEADER,                                  // Part of Header type register
        0,                                       // Not applicable
        0,                                       // Not applicable
        0x06,                                    // Offset from ECAM base
        iEP_EP,                                  // Applicable to integrated endpoints
        4,                                       // Start bit position
        4,                                       // End bit position
        1,                                       // Hardwired to 1b
        READ_ONLY,                               // Attribute is Read-only
        "SR CL value mismatch",                  // Capabilities List invalid configured value
        "SR CL attribute mismatch"               // Capabilities List invalid attribute
    },

    // Bit-field entry 9: Status register, bit[5] = 66 MHz Capable
    {
        HEADER,                                  // Part of Header type register
        0,                                       // Not applicable
        0,                                       // Not applicable
        0x06,                                    // Offset from ECAM base
        iEP_EP,                                  // Applicable to integrated endpoints
        5,                                       // Start bit position
        5,                                       // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "SR 66MHz capable value mismatch",       // 66MHz Capable invalid configured value
        "SR 66MHz capable attribute mismatch"    // 66MHz Capable invalid attribute
    },

    // Bit-field entry 10: Status register, bit[7] = Fast Back-to-Back Transactions Capable
    {
        HEADER,                                  // Part of Header type register
        0,                                       // Not applicable
        0,                                       // Not applicable
        0x06,                                    // Offset from ECAM base
        iEP_EP,                                  // Applicable to integrated endpoints
        7,                                       // Start bit position
        7,                                       // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "SR FBBTC value mismatch",               // FBBTC invalid configured value
        "SR FBBTC attribute mismatch"            // FBBTC invalid attribute
    },

    // Bit-field entry 11: Status register, bit[9:10] = DEVSEL timing
    {
        HEADER,                                  // Part of Header type register
        0,                                       // Not applicable
        0,                                       // Not applicable
        0x06,                                    // Offset from ECAM base
        iEP_EP,                                  // Applicable to integrated endpoints
        9,                                       // Start bit position
        10,                                      // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "SR DT value mismatch",                  // DEVSEL Timing invalid configured value
        "SR DT attribute mismatch"               // DEVSEL Timing invalid attribute
    },

    // Bit-field entry 12: Latency Timer register, bit[0:7] = latency timer register
    {
        HEADER,                                  // Part of Header type register
        0,                                       // Not applicable
        0,                                       // Not applicable
        0x0D,                                    // Offset from ECAM base
        iEP_EP,                                  // Applicable to integrated endpoints
        0,                                       // Start bit position
        7,                                       // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "LTR value mismatch",                    // Latency Timer invalid configured value
        "LTR attribute mismatch"                 // Latency Timer invalid attribute
    },

    // Bit-field entry 1: CardBus CIS Pointer, bit[0:31] = cardbus cis pointer
    {
        HEADER,                                  // Part of Header type register
        0,                                       // Not applicable
        0,                                       // Not applicable
        0x28,                                    // Offset from ECAM base
        iEP_EP,                                  // Applicable to Endpoints
        0,                                       // Start bit position
        31,                                      // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "CCP value mismatch",                    // CardBus CIS pointer invalid configured value
        "CCP attribute mismatch"                 // CardBus CIS pointer invalid attribute
    },

    // Bit-field entry 2: Min Grant, bit[0:7] = Min Gnt
    {
        HEADER,                                  // Part of Header type register
        0,                                       // Not applicable
        0,                                       // Not applicable
        0x3E,                                    // Offset from ECAM base
        iEP_EP,                                  // Applicable to Endpoints
        0,                                       // Start bit position
        7,                                       // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "MinGnt value mismatch",                 // MinGnt invalid configured value
        "MinGnt attribute mismatch"              // MinGnt invalid attribute
    },

    // Bit-field entry 3: Max Latency, bit[0:7] = Max latency
    {
        HEADER,                                  // Part of Header type register
        0,                                       // Not applicable
        0,                                       // Not applicable
        0x3F,                                    // Offset from ECAM base
        iEP_EP,                                  // Applicable to Endpoints
        0,                                       // Start bit position
        7,                                       // End bit position
        0,                                       // Hardwired to 0b
        RSVDP_RO,                                // Attribute is Read-only
        "Max latency value mismatch",            // Max latency invalid configured value
        "Max latency attribute mismatch"         // Max latency invalid attribute
    }
};
