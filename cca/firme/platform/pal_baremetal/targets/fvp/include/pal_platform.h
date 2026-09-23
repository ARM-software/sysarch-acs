/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

/*
 * Default platform configuration for the Arm FVP reference platform.
 *
 * Partners porting to other platforms should override these values (see
 * `cca/firme/docs/porting_guide.md`).
 */

#ifndef PLAT_NS_UART_BASE
/*
 * TF-A FVP uses:
 * - UART0 (boot console): 0x1c090000
 * - UART1 (runtime console): 0x1c0a0000
 *
 * For FIRME bring-up, use UART0 so our payload logs appear alongside
 * TF-A early boot logs on terminal_0.
 */
#define PLAT_NS_UART_BASE 0x1c090000UL
#endif

#ifndef PLAT_GICD_BASE
#define PLAT_GICD_BASE 0x2f000000UL
#endif

#ifndef PLAT_GICR_BASE
#define PLAT_GICR_BASE 0x2f100000UL
#endif

/* BL33-style image base (NS-EL2 dispatcher). */
#ifndef PLAT_NSEL2_IMAGE_BASE
#define PLAT_NSEL2_IMAGE_BASE 0x88000000UL
#endif

/*
 * BL32-style secure image base.
 *
 * On TF-A FVP, BL32/SPMC is loaded at `PLAT_ARM_TRUSTED_DRAM_BASE` which is
 * `0x0600_0000` (see TF-A `plat/arm/board/fvp/include/platform_def.h`).
 *
 * Note: the initial GPT map for FVP marks the first 1GiB region as
 * `GPT_GPI_ANY` (see `plat/arm/board/fvp/include/fvp_pas_def.h`).
 */
#ifndef PLAT_SECURE_IMAGE_BASE
#define PLAT_SECURE_IMAGE_BASE 0x06000000UL
#endif

/* For identity mapping. */
#ifndef PLAT_DRAM_BASE
#define PLAT_DRAM_BASE 0x80000000UL
#endif

#ifndef PLAT_DRAM_SIZE
#define PLAT_DRAM_SIZE 0x80000000UL /* 2 GiB */
#endif

/* Realm-only alternate-MECID alias and ACS-reserved private test MECID. */
#ifndef PLAT_FIRME_MEC_TEST_VA
#define PLAT_FIRME_MEC_TEST_VA 0x4000000000ULL
#endif

#ifndef PLAT_FIRME_MEC_TEST_MECID
#define PLAT_FIRME_MEC_TEST_MECID 3ULL
#endif

/*
 * Direct-messaging endpoint IDs (FVP default).
 *
 * The underlying transport is SPMD direct messaging (FF-A direct messages).
 * - The NS physical endpoint ID is defined by the FF-A specification as 0.
 * - The SPMC endpoint ID is chosen by the platform manifest (FVP uses 0x8000).
 */
#ifndef PLAT_FFA_NS_ENDPOINT_ID
#define PLAT_FFA_NS_ENDPOINT_ID 0x0000U
#endif

#ifndef PLAT_FFA_SPMC_ENDPOINT_ID
#define PLAT_FFA_SPMC_ENDPOINT_ID 0x8000U
#endif

/*
 * Endpoint ID used by the FIRME Secure payload for direct messaging.
 *
 * Normal world direct messages are typically addressed to a Secure Partition
 * endpoint (not to the SPMC endpoint itself). FIRME-ACS uses a single BL32
 * image and implements this as a small "logical endpoint" hosted by the SPMC
 * shim.
 */
#ifndef PLAT_FFA_ACS_SECURE_ENDPOINT_ID
#define PLAT_FFA_ACS_SECURE_ENDPOINT_ID 0x8001U
#endif

/*
 * FIRME_GM_GPI_SET test window.
 *
 * Use an NS DRAM window derived from TF-A FVP GPT PAS map:
 * `/plat/arm/board/fvp/include/fvp_pas_def.h` maps NS DRAM at:
 *   0x8000_0000 .. 0xFC00_0000
 *
 * Pick a small window close to the top of NS DRAM, away from typical BL33 use.
 * Keep this consistent across all worlds so caller-state is the only variable.
 */
#ifndef PLAT_FIRME_GPT_BASE_REALM
#define PLAT_FIRME_GPT_BASE_REALM 0xFB000000ULL
#endif

#ifndef PLAT_FIRME_GPT_BASE_SECURE
#define PLAT_FIRME_GPT_BASE_SECURE 0xFB000000ULL
#endif

#ifndef PLAT_FIRME_GPT_BASE_NS
#define PLAT_FIRME_GPT_BASE_NS 0xFB000000ULL
#endif

#ifndef PLAT_FIRME_GPT_GRANULE_SIZE
#define PLAT_FIRME_GPT_GRANULE_SIZE 0x1000ULL
#endif


/*
 * PCIe ECAM window used by FIRME IDE key management tests.
 *
 * TF-A FVP identifies the PCIe Root Complex by the ECAM base address
 * (`PCIE_EXP_BASE`, 0x4000_0000). Partners should override these values for
 * their target platform in the PAL target header.
 */
#ifndef PLAT_FIRME_PCIE_ECAM_BASE
#define PLAT_FIRME_PCIE_ECAM_BASE 0x40000000ULL
#endif

#ifndef PLAT_FIRME_PCIE_ECAM_SIZE
#define PLAT_FIRME_PCIE_ECAM_SIZE 0x10000000ULL
#endif

#ifndef PLAT_FIRME_PCIE_SEGMENT
#define PLAT_FIRME_PCIE_SEGMENT 0U
#endif

#ifndef PLAT_FIRME_PCIE_START_BUS
#define PLAT_FIRME_PCIE_START_BUS 0U
#endif

#ifndef PLAT_FIRME_PCIE_END_BUS
#define PLAT_FIRME_PCIE_END_BUS 0x8U
#endif

#ifndef PLAT_FIRME_PCIE_MAX_DEV
#define PLAT_FIRME_PCIE_MAX_DEV 32U
#endif

#ifndef PLAT_FIRME_PCIE_MAX_FUNC
#define PLAT_FIRME_PCIE_MAX_FUNC 8U
#endif

#ifndef PLAT_FIRME_PCIE_ROOT_PORT_ID
#define PLAT_FIRME_PCIE_ROOT_PORT_ID 0x8U
#endif

#ifndef PLAT_FIRME_PCIE_ENDPOINT_ID
#define PLAT_FIRME_PCIE_ENDPOINT_ID 0xffffffffU
#endif

#ifndef PLAT_FIRME_PCIE_STREAM_ID
#define PLAT_FIRME_PCIE_STREAM_ID 1U
#endif

#ifndef PLAT_FIRME_PCIE_SUBSTREAM_ID
#define PLAT_FIRME_PCIE_SUBSTREAM_ID 0U
#endif

#ifndef PLAT_FIRME_PCIE_DIRECTION_RX
#define PLAT_FIRME_PCIE_DIRECTION_RX 0U
#endif

#ifndef PLAT_FIRME_PCIE_KEY_SET_0
#define PLAT_FIRME_PCIE_KEY_SET_0 0U
#endif
