/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#define PAL_FIRME_PAT_BUFFER_SIZE UINT64_C(8192)
#define PAL_FIRME_PAT_ALIGNMENT UINT64_C(4096)

/**
 * @brief Verify a backed NS host-image range uses the live coherent EL2 boot map.
 * @param base Physical/virtual identity address in the NS host image.
 * @param size Entire range to validate.
 * @return VAL_TRUE for the verified caller preconditions, otherwise VAL_FALSE.
 * This is not EL3 mapping or independent GPT evidence for R0070/R0071.
 */
uint32_t pal_firme_shared_buffer_ns_mapping_ready(uint64_t base, uint64_t size);

/** @brief Verify the Realm boot shared-buffer range uses the live EL2 boot map. */
uint32_t pal_firme_shared_buffer_realm_mapping_ready(uint64_t base, uint64_t size);

/** @brief Read the current EL2 translation-table base for the PAT lease check. */
uint64_t pal_firme_shared_buffer_read_ttbr0_el2(void);

/** @brief Read the current EL2 memory attributes for the PAT lease check. */
uint64_t pal_firme_shared_buffer_read_mair_el2(void);

/** @brief Read HCR_EL2 to reject VHE translation regimes for the boot-map check. */
uint64_t pal_firme_shared_buffer_read_hcr_el2(void);
