#-------------------------------------------------------------------------------
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

set(FIRME_REALM_PAT_BINDING "none" CACHE STRING
    "Established Realm PAT fixture: none or delegated-rmm-el3 (legacy private P-384 RAK, requires RNDR)")
set_property(CACHE FIRME_REALM_PAT_BINDING PROPERTY STRINGS none delegated-rmm-el3)

if(FIRME_REALM_PAT_BINDING STREQUAL "none")
  return()
endif()
if(NOT FIRME_REALM_PAT_BINDING STREQUAL "delegated-rmm-el3")
  message(FATAL_ERROR "Unknown FIRME_REALM_PAT_BINDING='${FIRME_REALM_PAT_BINDING}'")
endif()

set(FIRME_REALM_MBEDTLS_DIR
    "${PROJECT_SOURCE_DIR}/ext/libspdm/os_stub/mbedtlslib/mbedtls" CACHE PATH
    "Pinned mbedTLS for the Realm PAT binding fixture")
if(NOT EXISTS "${FIRME_REALM_MBEDTLS_DIR}/library/ecp.c")
  message(FATAL_ERROR
    "Realm PAT binding requires pinned mbedTLS. Run git submodule update --init --recursive.")
endif()

set(_firme_realm_crypto_sources bignum.c bignum_core.c bignum_mod.c bignum_mod_raw.c
    constant_time.c ecp.c ecp_curves.c ecp_curves_new.c memory_buffer_alloc.c
    platform.c platform_util.c sha256.c)
list(TRANSFORM _firme_realm_crypto_sources PREPEND "${FIRME_REALM_MBEDTLS_DIR}/library/")
add_library(firme_realm_pat_crypto STATIC ${_firme_realm_crypto_sources})
target_include_directories(firme_realm_pat_crypto PUBLIC
    "${FIRME_REALM_MBEDTLS_DIR}/include"
    "${PROJECT_SOURCE_DIR}/val/realm"
    "${PROJECT_SOURCE_DIR}/tools/config")
target_compile_definitions(firme_realm_pat_crypto PUBLIC
    "MBEDTLS_CONFIG_FILE=\"val_realm_pat_mbedtls_config.h\"")
target_compile_options(firme_realm_pat_crypto PRIVATE
    -ffreestanding -fno-builtin -fno-stack-protector -mgeneral-regs-only
    -mstrict-align -mno-outline-atomics -ffunction-sections -fdata-sections -O2)
target_compile_definitions(rel2.elf PRIVATE FIRME_REALM_PAT_DELEGATED_LEGACY=1)
target_link_libraries(rel2.elf PRIVATE firme_realm_pat_crypto)
target_link_options(rel2.elf PRIVATE -Wl,--gc-sections)
message(STATUS
  "Realm PAT: externally configured Delegated model; legacy RAK is setup only, PAT remains ALP4")
