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
* The test table covers bit-field entries for PCIe capabilities register
* belonging to capability id 10h (PCIe capability structure)
**/

pcie_cfgreg_bitfield_entry bf_info_table51[] = {

    // Bit-field entry 1: PCI Express Capabilities Register, bit[8] Slot Implemented
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x02,                                    // Offset from capability id base
        iEP_RP,                                  // Applicable to Rootports
        8,                                       // Start bit position
        8,                                       // End bit position
        0,                                       // Hardwired to 0b
        HW_INIT,                                 // Attribute is HW_INIT
        "SI value mismatch",                     // SI invalid configured value
        "SI attribute mismatch"                  // SI invalid attribute
    },

    // Bit-field entry 1: Device Control Register, bit[9] Phantom Functions Enable
    // WARNING
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x08,                                    // Offset from capability id base
        iEP_RP,                                  // Applicable to integrated root ports
        9,                                       // Start bit position
        9,                                       // End bit position
        0,                                       // Hardwired to 0b
        READ_WRITE,                              // Attribute is Read-Write
        "WARNING PFE value mismatch",            // PFE invalid configured value
        "WARNING PFE attribute mismatch"         // PFE invalid attribute
    },

    // Bit-field entry 2: Device Control Register, bit[10] Aux power PM Enable
    // WARNING
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x08,                                    // Offset from capability id base
        iEP_RP,                                  // Applicable to integrated root ports
        10,                                      // Start bit position
        10,                                      // End bit position
        0,                                       // Hardwired to 0b
        STICKY_RW,                               // Attribute is Read-Write
        "WARNING APPE value mismatch",           // APPE invalid configured value
        "WARNING APPE attribute mismatch"        // APPE invalid attribute
    },

    // Bit-field entry 4: Device Control Register, bit[15] Initiate FLR
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x08,                                    // Offset from capability id base
        iEP_RP,                                  // Applicable to integrated root ports
        15,                                      // Start bit position
        15,                                      // End bit position
        0,                                       // Hardwired to 0b
        RSVDP_RO,                                // Attribute is rsvdp
        "IFLR value mismatch",                   // IFLR invalid configured value
        "IFLR attribute mismatch"                // IFLR invalid attribute
    },

    // Bit-field entry 1: Device Capabilities Register, bit[3:4] Phantom Functions Supported
    // WARNING
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x04,                                    // Offset from capability id base
        iEP_RP,                                  // Applicable to integrated root ports
        3,                                       // Start bit position
        4,                                       // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "WARNING : PFS value mismatch",          // PFS invalid configured value
        "WARNING : PFS attribute mismatch"       // PFS invalid attribute
    },

    // Bit-field entry 2: Device Capabilities Register, bit[5] Extended tag field support
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x04,                                    // Offset from capability id base
        iEP_RP,                                  // Applicable to integrated root ports
        5,                                       // Start bit position
        5,                                       // End bit position
        1,                                       // Hardwired to 1b
        READ_ONLY,                               // Attribute is Read-only
        "ETFS value mismatch",                   // ETFS invalid configured value
        "ETFS attribute mismatch"                // ETFS invalid attribute
    },

    // Bit-field entry 3: Device Capabilities Register, bit[6:8] Endpoint L0S Acceptable Latency
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x04,                                    // Offset from capability id base
        iEP_RP,                                  // Applicable to integrated root ports
        6,                                       // Start bit position
        8,                                       // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "ELAL value mismatch",                   // ELAL invalid configured value
        "ELAL attribute mismatch"                // ELAL invalid attribute
    },

    // Bit-field entry 4: Device Capabilities Register, bit[9:11] Endpoint L1 Acceptable Latency
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x04,                                    // Offset from capability id base
        iEP_RP,                                  // Applicable to integrated root ports
        9,                                       // Start bit position
        11,                                      // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "ELAL value mismatch",                   // ELAL invalid configured value
        "ELAL attribute mismatch"                // ELAL invalid attribute
    },

    // Bit-field entry 5: Device Capabilities Register, bit[15] Role based error reporting
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x04,                                    // Offset from capability id base
        iEP_RP,                                  // Applicable to integrated root ports
        15,                                      // Start bit position
        15,                                      // End bit position
        1,                                       // Hardwired to 1b
        READ_ONLY,                               // Attribute is Read-only
        "RBER value mismatch",                   // RBER invalid configured value
        "RBER attribute mismatch"                // RBER invalid attribute
    },

    // Bit-field entry 6: Device Capabilities Register, bit[18:25] Captured Slot Power Limit Value
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x04,                                    // Offset from capability id base
        iEP_RP,                                  // Applicable to integrated root ports
        18,                                      // Start bit position
        25,                                      // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "CSPLV value mismatch",                  // CSPLV invalid configured value
        "CSPLV attribute mismatch"               // CSPLV invalid attribute
    },

    // Bit-field entry 7: Device Capabilities Register, bit[26:27] Captured Slot Power Limit Scale
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x04,                                    // Offset from capability id base
        iEP_RP,                                  // Applicable to integrated root ports
        26,                                      // Start bit position
        27,                                      // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "CSPLS value mismatch",                  // CSPLS invalid configured value
        "CSPLS attribute mismatch"               // CSPLS invalid attribute
    },

    // Bit-field entry 8: Device Capabilities Register, bit[28] Functions Level Reset Capability
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x04,                                    // Offset from capability id base
        iEP_RP,                                  // Applicable to integrated root ports
        28,                                      // Start bit position
        28,                                      // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "FLRC value mismatch",                   // FLRC invalid configured value
        "FLRC attribute mismatch"                // FLRC invalid attribute
    },

    // Bit-field entry 2: Device Capabilities Register 2, bit[5] ARI Forwarding Support
    // WARNING
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x24,                                    // Offset from capability id base
        iEP_RP,                                  // Applicable to integrated root ports
        5,                                       // Start bit position
        5,                                       // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "WARNING AFS value mismatch",            // AFS invalid configured value
        "WARNING AFS attribute mismatch"         // AFS invalid attribute
    },

    // Bit-field entry 4: Device Capabilities Register 2, bit[6] AtomicOp Routing Supported
    // WARNING
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x24,                                    // Offset from capability id base
        iEP_RP,                                  // Applicable to integrated root ports
        6,                                       // Start bit position
        6,                                       // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "WARNING ARS value mismatch",            // ARS invalid configured value
        "WARNING ARS attribute mismatch"         // ARS invalid attribute
    },

    // Bit-field entry 7: Device Capabilities Register 2, bit[16] 10-bit tag completer supported
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x24,                                    // Offset from capability id base
        iEP_RP,                                  // Applicable to integrated root ports
        16,                                      // Start bit position
        16,                                      // End bit position
        1,                                       // Hardwired to 1b
        HW_INIT,                                 // Attribute is HW INIT
        "TCS value mismatch",                    // TCS invalid configured value
        "TCS attribute mismatch"                 // TCS invalid attribute
    },

    // Bit-field entry 8: Device Capabilities Register 2, bit[17] 10-bit tag requester supported
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x24,                                    // Offset from capability id base
        iEP_RP,                                  // Applicable to integrated root ports
        17,                                      // Start bit position
        17,                                      // End bit position
        1,                                       // Hardwired to 1b
        HW_INIT,                                 // Attribute is HW INIT
        "TRS value mismatch",                    // TRS invalid configured value
        "TRS attribute mismatch"                 // TRS invalid attribute
    },

    // Bit-field entry 9: Device Capabilities Register 2, bit[20] Extended Fmt Field Supported
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x24,                                    // Offset from capability id base
        iEP_RP,                                  // Applicable to integrated root ports
        20,                                      // Start bit position
        20,                                      // End bit position
        1,                                       // Hardwired to 1b
        READ_ONLY,                               // Attribute is READ_ONLY
        "EFFS value mismatch",                   // EFFS invalid configured value
        "EFFS attribute mismatch"                // EFFS invalid attribute
    },

    // Bit-field entry 13: Device Cap Register 2, bit[24:25] Emergency power reduction supported
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x24,                                    // Offset from capability id base
        iEP_RP,                                  // Applicable to integrated root ports
        24,                                      // Start bit position
        25,                                      // End bit position
        0,                                       // Hardwired to 0b
        RSVDP_RO,                                // Attribute is RSVDP
        "EPRS value mismatch",                   // EPRS invalid configured value
        "EPRS attribute mismatch"                // EPRS invalid attribute
    },

    // Bit-field entry 15: Device Cap Register 2, bit[26] Emergency power reduction init required
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x24,                                    // Offset from capability id base
        iEP_RP,                                  // Applicable to integrated root ports
        26,                                      // Start bit position
        26,                                      // End bit position
        0,                                       // Hardwired to 0b
        RSVDP_RO,                                // Attribute is RSVDP
        "EPRIR value mismatch",                  // EPRIR invalid configured value
        "EPRIR attribute mismatch"               // EPRIR invalid attribute
    },

    // Bit-field entry 5: Device Control Register 2, bit[11] Emergency Power Reduction request
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x28,                                    // Offset from capability id base
        iEP_RP,                                  // Applicable to integrated root ports
        11,                                      // Start bit position
        11,                                      // End bit position
        0,                                       // Hardwired to 0b
        RSVDP_RO,                                // Attribute is RSDVP
        "EPPR value mismatch",                   // EPPR invalid configured value
        "EPPR attribute mismatch"                // EPPR invalid attribute
    }
};
