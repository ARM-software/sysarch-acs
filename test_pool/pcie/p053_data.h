/** @file
 * Copyright (c) 2019,2023-2025, Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/acs_pcie.h"

/**
* The test table covers bit-field entries for device capabilities, capabilities 2,
* control, control 2 registers belonging to capability id 10h (PCIe capability structure)
**/

pcie_cfgreg_bitfield_entry bf_info_table53[] = {

    // Bit-field entry 1: Device Control Register, bit[9] Phantom Functions Enable
    // WARNING
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x08,                                    // Offset from capability id base
        RCEC,                                    // Applicable to RCEC funtions
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
        RCEC,                                    // Applicable to RCEC funtions
        10,                                      // Start bit position
        10,                                      // End bit position
        0,                                       // Hardwired to 0b
        STICKY_RW,                               // Attribute is Read-Write
        "WARNING APPE value mismatch",           // APPE invalid configured value
        "WARNING APPE attribute mismatch"        // APPE invalid attribute
    },

    // Bit-field entry 3: Device Control Register, bit[11] Enable No Snoop
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x08,                                    // Offset from capability id base
        RCEC,                                    // Applicable to RCEC functions
        11,                                      // Start bit position
        11,                                      // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-Only
        "ENS value mismatch",                    // ENS invalid configured value
        "ENS attribute mismatch"                 // ENS invalid attribute
    },

    // Bit-field entry 1: Device Capabilities Register, bit[3:4] Phantom Functions Supported
    // WARNING
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x04,                                    // Offset from capability id base
        RCEC,                                    // Applicable to RCEC funtions
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
        RCEC,                                    // Applicable to RCEC funtions
        5,                                       // Start bit position
        5,                                       // End bit position
        1,                                       // Hardwired to 1b
        READ_ONLY,                               // Attribute is Read-only
        "ETFS value mismatch",                   // ETFS invalid configured value
        "ETFS attribute mismatch"                // ETFS invalid attribute
    },

    // Bit-field entry 5: Device Capabilities Register, bit[15] Role based error reporting
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x04,                                    // Offset from capability id base
        RCEC,                                    // Applicable to RCEC funtions
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
        RCEC,                                    // Applicable to RCEC funtions
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
        RCEC,                                    // Applicable to RCEC funtions
        26,                                      // Start bit position
        27,                                      // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "CSPLS value mismatch",                  // CSPLS invalid configured value
        "CSPLS attribute mismatch"               // CSPLS invalid attribute
    },

    // Bit-field entry 1: Device Capabilities Register 2, bit[5] ARI Forwarding Support
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x24,                                    // Offset from capability id base
        RCEC,                                    // Applicable to RCEC funtions
        5,                                       // Start bit position
        5,                                       // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "AFS value mismatch",                    // AFS invalid configured value
        "AFS attribute mismatch"                 // AFS invalid attribute
    },

    // Bit-field entry 3: Device Capabilities Register 2, bit[6] AtomicOp Routing Supported
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x24,                                    // Offset from capability id base
        RCEC,                                    // Applicable to RCEC funtions
        6,                                       // Start bit position
        6,                                       // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is Read-only
        "ARS value mismatch",                    // ARS invalid configured value
        "ARS attribute mismatch"                 // ARS invalid attribute
    },

    // Bit-field entry 5: Device Capabilities Register 2, bit[10] No RO-enabled PR-PR passing
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x24,                                    // Offset from capability id base
        RCEC,                                    // Applicable to RCEC funtions
        10,                                      // Start bit position
        10,                                      // End bit position
        0,                                       // Hardwired to 0b
        HW_INIT,                                 // Attribute is HW INIT
        "NREPP value mismatch",                  // NREPP invalid configured value
        "NREPP attribute mismatch"               // NREPP invalid attribute
    },

    // Bit-field entry 10: Device Capabilities Register 2, bit[20] Extended Fmt Field Supported
    // WARNING
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x24,                                    // Offset from capability id base
        RCEC,                                    // Applicable to RCEC functions
        20,                                      // Start bit position
        20,                                      // End bit position
        1,                                       // Hardwired to 1b
        READ_ONLY,                               // Attribute is READ_ONLY
        "WARNING EFFS value mismatch",           // EFFS invalid configured value
        "WARNING EFFS attribute mismatch"        // EFFS invalid attribute
    },

    // Bit-field entry 11: Device Capabilities Register 2, bit[21] End-End TLP Prefix Supported
    // WARNING
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x24,                                    // Offset from capability id base
        RCEC,                                    // Applicable to RCEC functions
        21,                                      // Start bit position
        21,                                      // End bit position
        0,                                       // Hardwired to 0b
        HW_INIT,                                 // Attribute is HW_INIT
        "WARNING ETPS value mismatch",           // ETPS invalid configured value
        "WARNING ETPS attribute mismatch"        // ETPS invalid attribute
    },

    // Bit-field entry 12: Device Capabilities Register 2, bit[22:23] Max End-End TLP Prefixes
    // WARNING
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x24,                                    // Offset from capability id base
        RCEC,                                    // Applicable to RCEC functions
        22,                                      // Start bit position
        23,                                      // End bit position
        0,                                       // Hardwired to 0b
        RSVDP_RO,                                // Attribute is RSVDP
        "WARNING METP value mismatch",           // METP invalid configured value
        "WARNING METP attribute mismatch"        // METP invalid attribute
    },

    // Bit-field entry 1: Device Control Register 2, bit[5] ARI Forwarding Enable
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x28,                                    // Offset from capability id base
        RCEC,                                    // Applicable to RCEC funtions
        5,                                       // Start bit position
        5,                                       // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is READ_ONLY
        "AFI value mismatch",                    // AFE invalid configured value
        "AFI attribute mismatch"                 // AFE invalid attribute
    },

    // Bit-field entry 2: Device Control Register 2, bit[7] Atomicop Egress Blocking
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x28,                                    // Offset from capability id base
        RCEC,                                    // Applicable to RCEC funtions
        7,                                       // Start bit position
        7,                                       // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is READ_ONLY
        "AEB value mismatch",                    // AEB invalid configured value
        "AEB attribute mismatch"                 // AEB invalid attribute
    },

    // Bit-field entry 3: Device Control Register 2, bit[8] IDO request Enable
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x28,                                    // Offset from capability id base
        RCEC,                                    // Applicable to RCEC functions
        8,                                       // Start bit position
        8,                                       // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is READ_ONLY
        "IRE value mismatch",                    // IRE invalid configured value
        "IRE attribute mismatch"                 // IRE invalid attribute
    },

    // Bit-field entry 4: Device Control Register 2, bit[9] IDO Completer Enable
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x28,                                    // Offset from capability id base
        RCEC,                                    // Applicable to RCEC functions
        9,                                       // Start bit position
        9,                                       // End bit position
        0,                                       // Hardwired to 0b
        READ_ONLY,                               // Attribute is READ_ONLY
        "ICE value mismatch",                    // ICE invalid configured value
        "ICE attribute mismatch"                 // ICE invalid attribute
    },

    // Bit-field entry 5: Device Control Register 2, bit[11] Emergency Power Reduction request
    {
        PCIE_CAP,                                // Part of PCIe capability register
        0x10,                                    // Capability id
        0,                                       // Not applicable
        0x28,                                    // Offset from capability id base
        RCEC,                                    // Applicable to RCEC funtions
        11,                                      // Start bit position
        11,                                      // End bit position
        0,                                       // Hardwired to 0b
        RSVDP_RO,                                // Attribute is RSDVP
        "EPPR value mismatch",                   // EPPR invalid configured value
        "EPPR attribute mismatch"                // EPPR invalid attribute
    }
};
