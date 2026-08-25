#-------------------------------------------------------------------------------
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

set(FIRME_LIBSPDM_DIR "${PROJECT_SOURCE_DIR}/ext/libspdm" CACHE PATH
    "Path to the pinned libspdm submodule")
set(FIRME_SPDM_EMU_DIR "${PROJECT_SOURCE_DIR}/ext/spdm-emu" CACHE PATH
    "Path to the pinned spdm-emu submodule")
set(FIRME_SPDM_ROOT_CERT "" CACHE FILEPATH
    "DER trust anchor for the CXL endpoint SPDM responder")

if(NOT EXISTS "${FIRME_LIBSPDM_DIR}/library/spdm_requester_lib/CMakeLists.txt" OR
   NOT EXISTS "${FIRME_LIBSPDM_DIR}/os_stub/mbedtlslib/mbedtls/library/CMakeLists.txt")
  message(FATAL_ERROR
    "Missing libspdm/mbedTLS. Run git submodule update --init --recursive from the repository root.")
endif()
if(NOT EXISTS "${FIRME_SPDM_EMU_DIR}/library/cxl_ide_km_requester_lib/CMakeLists.txt")
  message(FATAL_ERROR
    "Missing spdm-emu. Run git submodule update --init --recursive from the repository root.")
endif()
if(NOT EXISTS "${FIRME_SPDM_ROOT_CERT}")
  message(FATAL_ERROR "FIRME_ENABLE_CXL_SPDM requires FIRME_SPDM_ROOT_CERT")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  set(_firme_spdm_runtime_compiler "${CMAKE_C_COMPILER}")
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang" AND
       NOT "$ENV{CROSS_COMPILE}" STREQUAL "")
  set(_firme_spdm_runtime_compiler "$ENV{CROSS_COMPILE}gcc")
else()
  message(FATAL_ERROR
    "SPDM requires a bare-metal C runtime. Set CROSS_COMPILE to an aarch64-none-elf- toolchain with newlib.")
endif()

execute_process(COMMAND "${_firme_spdm_runtime_compiler}" -dumpmachine
  RESULT_VARIABLE _firme_spdm_runtime_status
  OUTPUT_VARIABLE _firme_spdm_runtime_target
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT _firme_spdm_runtime_status EQUAL 0 OR
   NOT _firme_spdm_runtime_target MATCHES "^aarch64(-[A-Za-z0-9_]+)*-elf$")
  message(FATAL_ERROR
    "SPDM requires an AArch64 bare-metal C runtime, not a hosted Linux libc. Use an aarch64-none-elf- CROSS_COMPILE toolchain.")
endif()

execute_process(COMMAND "${_firme_spdm_runtime_compiler}" -print-file-name=libc.a
  RESULT_VARIABLE _firme_spdm_runtime_status
  OUTPUT_VARIABLE FIRME_SPDM_LIBC
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT _firme_spdm_runtime_status EQUAL 0 OR
   NOT IS_ABSOLUTE "${FIRME_SPDM_LIBC}" OR
   NOT EXISTS "${FIRME_SPDM_LIBC}" OR IS_DIRECTORY "${FIRME_SPDM_LIBC}")
  message(FATAL_ERROR
    "SPDM requires the toolchain's newlib libc.a. Install a complete AArch64 bare-metal GNU toolchain.")
endif()
message(STATUS "SPDM bare-metal C runtime: ${FIRME_SPDM_LIBC}")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
             "${FIRME_SPDM_ROOT_CERT}")

file(SIZE "${FIRME_SPDM_ROOT_CERT}" FIRME_SPDM_ROOT_CERT_SIZE)
if(FIRME_SPDM_ROOT_CERT_SIZE LESS 1 OR FIRME_SPDM_ROOT_CERT_SIZE GREATER 8192)
  message(FATAL_ERROR "FIRME_SPDM_ROOT_CERT must contain 1..8192 DER bytes")
endif()
file(READ "${FIRME_SPDM_ROOT_CERT}" _firme_cert_hex HEX)
set(_firme_cert_bytes "")
math(EXPR _firme_cert_last "${FIRME_SPDM_ROOT_CERT_SIZE} - 1")
foreach(_firme_cert_index RANGE 0 ${_firme_cert_last})
  math(EXPR _firme_cert_offset "${_firme_cert_index} * 2")
  string(SUBSTRING "${_firme_cert_hex}" ${_firme_cert_offset} 2 _firme_cert_octet)
  string(APPEND _firme_cert_bytes "0x${_firme_cert_octet},")
  math(EXPR _firme_cert_column "${_firme_cert_index} % 16")
  if(_firme_cert_column EQUAL 15)
    string(APPEND _firme_cert_bytes "\n")
  endif()
endforeach()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/firme_spdm_trust_anchor.h"
     "#pragma once\n#include <stddef.h>\n#include <stdint.h>\n"
     "static const uint8_t g_val_host_spdm_root_cert[] = {\n${_firme_cert_bytes}\n};\n")

set(LIBSPDM_DIR "${FIRME_LIBSPDM_DIR}")
set(SPDM_EMU_DIR "${FIRME_SPDM_EMU_DIR}")
set(ARCH aarch64)
set(TOOLCHAIN ARM_GNU_BARE_METAL)
set(MBEDTLS_CONFIG_FILE
    "${CMAKE_CURRENT_LIST_DIR}/../config/val_host_spdm_mbedtls_config.h")

set(_firme_spdm_components
    "${LIBSPDM_DIR}/library/spdm_common_lib"
    "${LIBSPDM_DIR}/library/spdm_requester_lib"
    "${LIBSPDM_DIR}/library/spdm_secured_message_lib"
    "${LIBSPDM_DIR}/library/spdm_transport_pcidoe_lib"
    "${LIBSPDM_DIR}/library/spdm_crypt_lib"
    "${LIBSPDM_DIR}/os_stub/memlib"
    "${LIBSPDM_DIR}/os_stub/malloclib"
    "${LIBSPDM_DIR}/os_stub/debuglib_null"
    "${LIBSPDM_DIR}/os_stub/spdm_crypt_ext_lib"
    "${LIBSPDM_DIR}/os_stub/spdm_device_secret_lib_null"
    "${LIBSPDM_DIR}/os_stub/cryptlib_mbedtls"
    "${LIBSPDM_DIR}/os_stub/mbedtlslib"
    "${SPDM_EMU_DIR}/library/pci_doe_requester_lib"
    "${SPDM_EMU_DIR}/library/cxl_ide_km_requester_lib")

foreach(_firme_spdm_component IN LISTS _firme_spdm_components)
  get_filename_component(_firme_spdm_name "${_firme_spdm_component}" NAME)
  add_subdirectory("${_firme_spdm_component}"
                   "${CMAKE_CURRENT_BINARY_DIR}/spdm/${_firme_spdm_name}")
  if(NOT _firme_spdm_name STREQUAL "mbedtlslib")
    target_compile_options(${_firme_spdm_name} PRIVATE
      -ffunction-sections -fdata-sections)
    target_include_directories(${_firme_spdm_name} PRIVATE
      "${LIBSPDM_DIR}/include"
      "${LIBSPDM_DIR}/os_stub"
      "${SPDM_EMU_DIR}/include"
      "${CMAKE_CURRENT_LIST_DIR}/../config")
    target_compile_definitions(${_firme_spdm_name} PRIVATE
      "LIBSPDM_CONFIG=<val_host_spdm_config.h>")
  endif()
endforeach()

foreach(_firme_mbedtls_target IN ITEMS cryptlib_mbedtls mbedtls mbedx509 mbedcrypto)
  target_compile_options(${_firme_mbedtls_target} PRIVATE
    -ffunction-sections -fdata-sections)
  target_compile_definitions(${_firme_mbedtls_target} PRIVATE
    "MBEDTLS_CONFIG_FILE=\"${MBEDTLS_CONFIG_FILE}\"")
endforeach()

set(FIRME_SPDM_LINK_LIBRARIES
    "-Wl,--start-group"
    cxl_ide_km_requester_lib
    pci_doe_requester_lib
    spdm_requester_lib
    spdm_common_lib
    spdm_secured_message_lib
    spdm_transport_pcidoe_lib
    spdm_crypt_lib
    spdm_crypt_ext_lib
    spdm_device_secret_lib_null
    cryptlib_mbedtls
    mbedtlslib
    mbedx509
    mbedcrypto
    memlib
    malloclib
    debuglib_null
    "${FIRME_SPDM_LIBC}"
    gcc
    "-Wl,--end-group")
