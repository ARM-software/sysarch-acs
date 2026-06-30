#-------------------------------------------------------------------------------
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)

project(firme_acs C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
    -O2 -g
)

set(FIRME_COMMON_LDFLAGS
    -nostdlib -nostartfiles
)

add_library(firme_common_obj OBJECT
    "${FIRME_PAL_DIR}/src/pal_uart_pl011.c"

    val_common/src/val_log.c
    val_common/src/val_arch_init.c
    val_common/src/val_fault.c
    val_common/src/val_mmu.c
    val_common/src/val_gicv3.c
    val_common/src/val_mem.c
    val_common/src/val_smccc.S
    val_common/src/val_sysreg.S

    val/src/val_firme_abi.c
    val/src/val_firme_transport.c
    val/src/val_firme_gpt.c
    val/src/val_firme_service.c
    val/src/val_platform.c
    val/src/val_firme_suite.c
    val/src/val_firme_diag.c
    val/src/val_firme_rules_dispatch.c
    val/val_firme_test.c
)

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

if(FIRME_ENABLE_BASE)
  target_sources(firme_common_obj PRIVATE
    test/base/service_version.c
    test/base/service_feature_registers.c
    test/base/service_features.c
    test/base/base_service_mandatory.c
  )
endif()

if(FIRME_ENABLE_GPT)
  target_sources(firme_common_obj PRIVATE
    test/gpt/gpt_transition_feat_rme.c
    test/gpt/gpt_transition_feat_gpc2.c
    test/gpt/gpt_transition_feat_gdi.c
  )
endif()

# Future chapters (MEC/IDE/ATTEST) should follow the same pattern:
# - add rule sources under test/<module>/
# - add them here behind FIRME_ENABLE_* gates

function(add_firme_payload target_name out_base link_script)
  add_executable(${target_name}.elf ${ARGN} $<TARGET_OBJECTS:firme_common_obj>)
  target_include_directories(${target_name}.elf PRIVATE ${FIRME_COMMON_INCLUDES})
  target_compile_options(${target_name}.elf PRIVATE ${FIRME_COMMON_CFLAGS})
  target_compile_definitions(${target_name}.elf PRIVATE ACS_PRINT_LEVEL=${ACS_PRINT_LEVEL})
  target_compile_definitions(${target_name}.elf PRIVATE
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

add_firme_payload(
  nsel2
  nsel2
  "tools/linker/host.ld"
  val/host/val_host_main.c
  val/host/val_host_dispatcher.c
  val/host/val_host_entry.S
  val/host/val_host_vectors.S
)

add_firme_payload(
  secure
  secure
  "tools/linker/secure.ld"
  val/secure/val_secure_spmc.c
  val/secure/val_secure_entry.S
  val/secure/val_secure_vectors.S
  val/secure/val_secure_sel1_entry.S
)

add_firme_payload(
  rel2
  rel2
  "tools/linker/realm.ld"
  val/realm/val_realm_main.c
  val/realm/val_realm_rmi_dispatch.c
  val/realm/val_realm_entry.S
  val/realm/val_realm_vectors.S
)

add_custom_target(firme_payloads ALL DEPENDS nsel2.elf secure.elf rel2.elf)
