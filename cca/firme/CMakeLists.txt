#-------------------------------------------------------------------------------
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)

project(firme_acs C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(FIRME_VERIFY_ABI_CATALOG
       "Verify generated FIRME ABI catalog files during every build" ON)
option(FIRME_MP_SELF_TEST "Validate secondary PE workers before running rules" ON)
option(FIRME_ENABLE_CXL_ENDPOINT_PROBE
       "Probe a Type-2 endpoint and its SPDM DOE protocols before FIRME tests" OFF)
option(FIRME_ENABLE_CXL_SPDM
       "Exercise authenticated CXL IDE key management over endpoint DOE" OFF)

if(FIRME_ENABLE_CXL_SPDM)
  if(NOT FIRME_ENABLE_CXL_ENDPOINT_PROBE)
    message(FATAL_ERROR "FIRME_ENABLE_CXL_SPDM requires FIRME_ENABLE_CXL_ENDPOINT_PROBE")
  endif()
  include("${CMAKE_CURRENT_LIST_DIR}/tools/cmake/firme_spdm.cmake")
endif()

if(FIRME_VERIFY_ABI_CATALOG)
  find_program(FIRME_PYTHON3_EXECUTABLE NAMES python3 REQUIRED)
  set(FIRME_ABI_CATALOG
      "${CMAKE_CURRENT_LIST_DIR}/tools/config/abi/firme_1_0_alp4.json")
  set(FIRME_ABI_GENERATOR
      "${CMAKE_CURRENT_LIST_DIR}/tools/scripts/generate_firme_abi_catalog.py")
  file(GLOB FIRME_ABI_SCENARIOS CONFIGURE_DEPENDS
      "${CMAKE_CURRENT_LIST_DIR}/tools/config/abi/scenarios/alpha1/*.json")
  set(FIRME_ABI_GENERATED_HEADER
      "${CMAKE_CURRENT_LIST_DIR}/val/include/val_firme_abi_catalog.h")
  set(FIRME_ABI_GENERATED_SOURCE
      "${CMAKE_CURRENT_LIST_DIR}/val/src/val_firme_abi_catalog.c")
  set(FIRME_ABI_GENERATED_WRAPPER_HEADER
      "${CMAKE_CURRENT_LIST_DIR}/val/include/val_firme_abi_generated.h")
  set(FIRME_ABI_GENERATED_WRAPPER_SOURCE
      "${CMAKE_CURRENT_LIST_DIR}/val/src/val_firme_abi_generated.c")
  set(FIRME_ABI_GENERATED_REGISTRY
      "${CMAKE_CURRENT_LIST_DIR}/val/include/val_firme_abi_registry.inc")
  set(FIRME_ABI_GENERATED_COVERAGE
      "${CMAKE_CURRENT_LIST_DIR}/docs/FIRME_Alpha1_ABI_Interface_Coverage.md")
  file(GLOB FIRME_ABI_GENERATED_TESTS CONFIGURE_DEPENDS
      "${CMAKE_CURRENT_LIST_DIR}/test/base/generated_abi_*.c"
      "${CMAKE_CURRENT_LIST_DIR}/test/gpt/generated_abi_*.c"
      "${CMAKE_CURRENT_LIST_DIR}/test/ide/generated_abi_*.c"
      "${CMAKE_CURRENT_LIST_DIR}/test/mec/generated_abi_*.c")

  add_custom_target(firme_abi_catalog_check ALL
    COMMAND "${FIRME_PYTHON3_EXECUTABLE}" "${FIRME_ABI_GENERATOR}" --check
    DEPENDS
      "${FIRME_ABI_CATALOG}"
      "${FIRME_ABI_GENERATOR}"
      ${FIRME_ABI_SCENARIOS}
      "${FIRME_ABI_GENERATED_HEADER}"
      "${FIRME_ABI_GENERATED_SOURCE}"
      "${FIRME_ABI_GENERATED_WRAPPER_HEADER}"
      "${FIRME_ABI_GENERATED_WRAPPER_SOURCE}"
      "${FIRME_ABI_GENERATED_REGISTRY}"
      "${FIRME_ABI_GENERATED_COVERAGE}"
      ${FIRME_ABI_GENERATED_TESTS}
    COMMENT "Checking generated FIRME ABI catalog"
    VERBATIM
  )
endif()

# Suite selection (build-time).
# - Default is "all": build all available FIRME chapters/tests.
# - To build a specific chapter only: -DSUITE=base|gpt|mec|ide|attest
set(SUITE "all" CACHE STRING "FIRME suite to build: all|base|gpt|mec|ide|attest")
string(TOLOWER "${SUITE}" _suite)

# Internal enable switches derived from SUITE.
set(FIRME_ENABLE_BASE OFF)
set(FIRME_ENABLE_GPT OFF)
set(FIRME_ENABLE_MEC OFF)
set(FIRME_ENABLE_IDE OFF)
set(FIRME_ENABLE_ATTEST OFF)

if(_suite STREQUAL "all")
  set(FIRME_ENABLE_BASE ON)
  set(FIRME_ENABLE_GPT ON)
  set(FIRME_ENABLE_MEC ON)
  set(FIRME_ENABLE_IDE ON)
  set(FIRME_ENABLE_ATTEST ON)
elseif(_suite STREQUAL "base")
  set(FIRME_ENABLE_BASE ON)
elseif(_suite STREQUAL "gpt")
  set(FIRME_ENABLE_GPT ON)
elseif(_suite STREQUAL "mec")
  set(FIRME_ENABLE_MEC ON)
elseif(_suite STREQUAL "ide")
  set(FIRME_ENABLE_IDE ON)
elseif(_suite STREQUAL "attest")
  set(FIRME_ENABLE_ATTEST ON)
else()
  message(FATAL_ERROR "Invalid SUITE='${SUITE}' (expected all|base|gpt|mec|ide|attest).")
endif()

# Default artifact output directory matches the existing scripts.
set(FIRME_ARTIFACT_DIR "${CMAKE_CURRENT_LIST_DIR}/build" CACHE PATH
    "Directory to write final .bin/.elf payload artifacts")

# Logging verbosity (ACS-style): 1=TRACE, 2=DEBUG, 3=TEST(default), 4=WARN, 5=ERROR.
set(ACS_PRINT_LEVEL 3 CACHE STRING "ACS print level (1..5)")

#
# PAL selection (environment + target platform).
# - Environment: execution substrate (bare-metal for FIRME-ACS payloads).
# - Target: platform instance (default: fvp). Partners can add a new target
#   under platform/pal_<env>/targets/<target>/ and switch this variable.
#
set(PAL_ENV "baremetal" CACHE STRING "PAL environment: baremetal")
set(PAL_TARGET "fvp" CACHE STRING "PAL target platform: fvp")
string(TOLOWER "${PAL_ENV}" _pal_env)
string(TOLOWER "${PAL_TARGET}" _pal_target)

set(FIRME_PAL_DIR
    "${CMAKE_CURRENT_LIST_DIR}/platform/pal_${_pal_env}/targets/${_pal_target}"
)

set(FIRME_COMMON_INCLUDES
    "${FIRME_PAL_DIR}/include"
    "${CMAKE_CURRENT_LIST_DIR}/val/include"
    "${CMAKE_CURRENT_LIST_DIR}/val_common/include"
)

set(FIRME_COMMON_CFLAGS
    -Wall -Wextra -Werror
    -Wdeclaration-after-statement
    -ffreestanding -fno-builtin -fno-stack-protector -fno-omit-frame-pointer
    $<$<COMPILE_LANGUAGE:C>:-mstrict-align>
    -O2 -g
    -mstrict-align
)

set_source_files_properties(
    "${CMAKE_CURRENT_LIST_DIR}/val_common/src/val_fault.c"
    "${CMAKE_CURRENT_LIST_DIR}/test/base/reserved_input_bits.c"
    "${CMAKE_CURRENT_LIST_DIR}/test/base/reserved_input_registers.c"
    PROPERTIES COMPILE_OPTIONS "-mgeneral-regs-only"
)

set(FIRME_COMMON_LDFLAGS
    -nostdlib -nostartfiles
)

file(GLOB FIRME_PAL_SRC CONFIGURE_DEPENDS
    "${FIRME_PAL_DIR}/src/*.c"
    "${FIRME_PAL_DIR}/src/*.S"
)

file(GLOB FIRME_COMMON_VAL_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/val_common/src/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/val_common/src/*.S"
)

file(GLOB FIRME_VAL_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/val/src/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/val/src/*.S"
    "${CMAKE_CURRENT_LIST_DIR}/val/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/val/*.S"
)

add_library(firme_common_obj OBJECT
    ${FIRME_PAL_SRC}
    ${FIRME_COMMON_VAL_SRC}
    ${FIRME_VAL_SRC}
)

if(FIRME_VERIFY_ABI_CATALOG)
  add_dependencies(firme_common_obj firme_abi_catalog_check)
endif()

target_include_directories(firme_common_obj PRIVATE ${FIRME_COMMON_INCLUDES})
target_compile_options(firme_common_obj PRIVATE ${FIRME_COMMON_CFLAGS})
target_compile_definitions(firme_common_obj PRIVATE ACS_PRINT_LEVEL=${ACS_PRINT_LEVEL})

target_compile_definitions(firme_common_obj PRIVATE
  FIRME_ENABLE_BASE=$<BOOL:${FIRME_ENABLE_BASE}>
  FIRME_ENABLE_GPT=$<BOOL:${FIRME_ENABLE_GPT}>
  FIRME_ENABLE_MEC=$<BOOL:${FIRME_ENABLE_MEC}>
  FIRME_ENABLE_IDE=$<BOOL:${FIRME_ENABLE_IDE}>
  FIRME_ENABLE_ATTEST=$<BOOL:${FIRME_ENABLE_ATTEST}>
)

file(GLOB FIRME_BASE_TEST_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/test/base/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/test/base/*.S"
)

file(GLOB FIRME_GPT_TEST_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/test/gpt/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/test/gpt/*.S"
)

file(GLOB FIRME_MEC_TEST_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/test/mec/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/test/mec/*.S"
)

file(GLOB FIRME_IDE_TEST_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/test/ide/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/test/ide/*.S"
)

if(FIRME_ENABLE_BASE)
  target_sources(firme_common_obj PRIVATE ${FIRME_BASE_TEST_SRC})
endif()

if(FIRME_ENABLE_GPT)
  target_sources(firme_common_obj PRIVATE ${FIRME_GPT_TEST_SRC})
endif()

if(FIRME_ENABLE_MEC)
  target_sources(firme_common_obj PRIVATE ${FIRME_MEC_TEST_SRC})
endif()

if(FIRME_ENABLE_IDE)
  target_sources(firme_common_obj PRIVATE ${FIRME_IDE_TEST_SRC})
endif()

# Future chapters should follow the same pattern:
# - add rule sources under test/<module>/
# - add a module glob and include it behind the SUITE-derived enable switch

function(add_firme_payload target_name out_base link_script)
  add_executable(${target_name}.elf ${ARGN} $<TARGET_OBJECTS:firme_common_obj>)
  target_include_directories(${target_name}.elf PRIVATE ${FIRME_COMMON_INCLUDES})
  target_compile_options(${target_name}.elf PRIVATE ${FIRME_COMMON_CFLAGS})
  target_compile_definitions(${target_name}.elf PRIVATE ACS_PRINT_LEVEL=${ACS_PRINT_LEVEL})
  target_compile_definitions(${target_name}.elf PRIVATE
    FIRME_MP_SELF_TEST=$<BOOL:${FIRME_MP_SELF_TEST}>
    FIRME_ENABLE_BASE=$<BOOL:${FIRME_ENABLE_BASE}>
    FIRME_ENABLE_GPT=$<BOOL:${FIRME_ENABLE_GPT}>
    FIRME_ENABLE_MEC=$<BOOL:${FIRME_ENABLE_MEC}>
    FIRME_ENABLE_IDE=$<BOOL:${FIRME_ENABLE_IDE}>
    FIRME_ENABLE_ATTEST=$<BOOL:${FIRME_ENABLE_ATTEST}>
  )

  # Linker script must live under tools/ (portable, centralized).
  set(_ld "${CMAKE_CURRENT_LIST_DIR}/${link_script}")
  set_property(TARGET ${target_name}.elf PROPERTY LINK_DEPENDS "${_ld}")
  target_link_options(${target_name}.elf PRIVATE ${FIRME_COMMON_LDFLAGS} "-Wl,-T,${_ld}")

  add_custom_command(TARGET ${target_name}.elf POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${FIRME_ARTIFACT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${target_name}.elf>"
            "${FIRME_ARTIFACT_DIR}/${out_base}.elf"
    COMMAND ${CMAKE_OBJCOPY} -O binary
            "$<TARGET_FILE:${target_name}.elf>"
            "${FIRME_ARTIFACT_DIR}/${out_base}.bin"
    BYPRODUCTS
            "${FIRME_ARTIFACT_DIR}/${out_base}.elf"
            "${FIRME_ARTIFACT_DIR}/${out_base}.bin"
    VERBATIM
  )
endfunction()

file(GLOB FIRME_HOST_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/val/host/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/val/host/*.S"
)

file(GLOB FIRME_SECURE_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/val/secure/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/val/secure/*.S"
)

file(GLOB FIRME_REALM_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/val/realm/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/val/realm/*.S"
)

add_firme_payload(
  nsel2
  nsel2
  "tools/linker/host.ld"
  ${FIRME_HOST_SRC}
)
target_compile_definitions(nsel2.elf PRIVATE
  FIRME_ENABLE_CXL_ENDPOINT_PROBE=$<BOOL:${FIRME_ENABLE_CXL_ENDPOINT_PROBE}>
  FIRME_ENABLE_CXL_SPDM=$<BOOL:${FIRME_ENABLE_CXL_SPDM}>
)
if(FIRME_ENABLE_CXL_SPDM)
  target_include_directories(nsel2.elf PRIVATE
    "${FIRME_LIBSPDM_DIR}/include"
    "${FIRME_SPDM_EMU_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}"
  )
  target_link_libraries(nsel2.elf PRIVATE ${FIRME_SPDM_LINK_LIBRARIES})
  target_link_options(nsel2.elf PRIVATE -Wl,--gc-sections)
endif()

add_firme_payload(
  secure
  secure
  "tools/linker/secure.ld"
  ${FIRME_SECURE_SRC}
)

add_firme_payload(
  rel2
  rel2
  "tools/linker/realm.ld"
  ${FIRME_REALM_SRC}
)

add_custom_target(firme_payloads ALL DEPENDS nsel2.elf secure.elf rel2.elf)
