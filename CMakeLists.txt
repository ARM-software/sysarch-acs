## @file
 # Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
 # SPDX-License-Identifier : Apache-2.0
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 #  http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
 ##

# Set the minimum required version of CMake for the project
cmake_minimum_required(VERSION 3.21)

project(bsa-acs LANGUAGES)

# cmake_policy
cmake_policy(SET CMP0001 NEW)

### Tool dependency check - end ###

get_filename_component(ROOT_DIR . ABSOLUTE)

# Set internal build directory variable
set(BUILD ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "Setting build directory to ${BUILD}" FORCE)
file(MAKE_DIRECTORY ${BUILD}/output/)

# Set global compile list variable
set(COMPILE_LIST "")

#### Include cmake support module ###
include(${ROOT_DIR}/tools/cmake/toolchain/utils.cmake)
include(${ROOT_DIR}/tools/cmake/toolchain/default.cmake)
####


### Valid value range for command line argument ###

list(APPEND ARM_ARCH_MAJOR_LIST 8 9)
list(APPEND ACS_LIST "bsa" "sbsa" "pc_bsa")

###
# Prefer the toolchain file to validate and set `CROSS_COMPILE`.
# If no toolchain file is provided, require `CROSS_COMPILE` from the
# environment to support local cross-configures.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(NOT DEFINED CROSS_COMPILE)
        set(CROSS_COMPILE $ENV{CROSS_COMPILE})
    endif()

    if(NOT DEFINED CROSS_COMPILE OR "${CROSS_COMPILE}" STREQUAL "")
        message(FATAL_ERROR "CROSS_COMPILE not defined. Please set CROSS_COMPILE or use -DCMAKE_TOOLCHAIN_FILE=<path>.")
    endif()
endif()

# NOTE: cross-toolchain selection is handled by an explicit toolchain file
# (`tools/cmake/toolchain/arm-none-elf-toolchain.cmake`). Users should
# configure with `-DCMAKE_TOOLCHAIN_FILE=...` to avoid host-specific flags.

# Check for valid targets
_get_sub_dir_list(TARGET_LIST ${ROOT_DIR}/pal/baremetal/target/)
if(NOT DEFINED TARGET)
    set(TARGET ${TARGET_DFLT} CACHE INTERNAL "Defaulting target to ${TARGET}" FORCE)
else()
    set(TARGET ${TARGET} CACHE INTERNAL "TARGET is set to ${TARGET}" FORCE)
endif()

if(NOT ${TARGET} IN_LIST TARGET_LIST)
    message(FATAL_ERROR "[ACS] : Error: Unspported value for -DTARGET=, supported targets are : ${TARGET_LIST}")
endif()

if(NOT DEFINED ACS)
    set(ACS "${ACS_DFLT}" CACHE INTERNAL "Defaulting ACS to ${ACS}" FORCE)
else()
    set(ACS "${ACS}" CACHE INTERNAL "ACS is set to ${ACS}" FORCE)
endif()

if(NOT ${ACS} IN_LIST ACS_LIST)
    message(FATAL_ERROR "[ACS] : Error: Unspported value for -DACS=, supported acs are : ${ACS_LIST}")
endif()

# Check for ARM_ARCH_MAJOR
if(NOT DEFINED ARM_ARCH_MAJOR)
    set(ARM_ARCH_MAJOR "${ARM_ARCH_MAJOR_DFLT}" CACHE INTERNAL "Default ARM_ARCH_MAJOR value" FORCE)
        message(STATUS "[ACS] : Defaulting ARM_ARCH_MAJOR to ${ARM_ARCH_MAJOR}")
else()
    if(NOT ${ARM_ARCH_MAJOR} IN_LIST ARM_ARCH_MAJOR_LIST)
        message(FATAL_ERROR "[ACS] : Error: Unspported value for -DARM_ARCH_MAJOR=, supported values are : ${ARM_ARCH_MAJOR_LIST}")
    endif()
    message(STATUS "[ACS] : ARM_ARCH_MAJOR is set to ${ARM_ARCH_MAJOR}")
endif()

# Check for ARM_ARCH_MINOR
if(NOT DEFINED ARM_ARCH_MINOR)
    set(ARM_ARCH_MINOR "${ARM_ARCH_MINOR_DFLT}" CACHE INTERNAL "Default ARM_ARCH_MINOR value" FORCE)
        message(STATUS "[ACS] : Defaulting ARM_ARCH_MINOR to ${ARM_ARCH_MINOR}")
else()
    message(STATUS "[ACS] : ARM_ARCH_MINOR is set to ${ARM_ARCH_MINOR}")
endif()

# Setup toolchain parameters for compilation and link
include(${ROOT_DIR}/tools/cmake/toolchain/common.cmake)

# Propagate ACS_ENABLED_MODULE_LIST into compiler definitions for all sources
if(DEFINED ACS_ENABLED_MODULE_LIST)
    message(STATUS "[ACS] : ACS_ENABLED_MODULE_LIST (compile defs) = ${ACS_ENABLED_MODULE_LIST}")
    add_compile_definitions(ACS_ENABLED_MODULE_LIST=${ACS_ENABLED_MODULE_LIST})
endif()

### Cmake clean target ###
list(APPEND CLEAN_LIST
        ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_HEADER}
        ${CMAKE_CURRENT_BINARY_DIR}/output
)

# Optional forwarding of ACS_ENABLED_MODULE_LIST to inner configure
set(DEFAULT_ENABLED_MODULES "")
if(DEFINED ACS_ENABLED_MODULE_LIST)
    message(STATUS "[ACS] : ACS_ENABLED_MODULE_LIST (top-level) = ${ACS_ENABLED_MODULE_LIST}")
    list(APPEND DEFAULT_ENABLED_MODULES -DACS_ENABLED_MODULE_LIST=${ACS_ENABLED_MODULE_LIST})
endif()

# Enable fast-path optimizations for simulation/emulation builds.
# Use:
#   cmake -DTARGET_SIMULATION=ON ...
option(TARGET_SIMULATION "Enable simulation/emulation optimizations (defines TARGET_SIMULATION)" OFF)
if(TARGET_SIMULATION)
    message(STATUS "[ACS] : TARGET_SIMULATION is enabled (defining TARGET_SIMULATION)")
    add_compile_definitions(TARGET_SIMULATION)
 else()
    message(STATUS "[ACS] : TARGET_SIMULATION is disabled")
 endif()

# Include the files for make clean
foreach(clean_item ${CLEAN_LIST})
        set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${clean_item})
endforeach()
###

# Reuse the caller-provided toolchain for nested configure steps, so
# convenience targets do not override user choices.
set(_acs_toolchain_arg "")
if(DEFINED CMAKE_TOOLCHAIN_FILE AND NOT "${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
    set(_acs_toolchain_arg "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
else()
    set(_acs_toolchain_arg "-DCMAKE_TOOLCHAIN_FILE=${ROOT_DIR}/tools/cmake/toolchain/arm-none-elf-toolchain.cmake")
endif()

# Generate per-ACS custom targets and an aggregate target
foreach(_acs IN LISTS ACS_LIST)
    add_custom_target(${_acs}
        COMMAND ${CMAKE_COMMAND} -DACS=${_acs} -DTARGET=${TARGET} -DTARGET_SIMULATION=${TARGET_SIMULATION} ${DEFAULT_ENABLED_MODULES}
        -DCMAKE_OSX_ARCHITECTURES= ${_acs_toolchain_arg} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/${_acs}_build
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/${_acs}_build
    )
endforeach()

add_custom_target(acs_all
    DEPENDS ${ACS_LIST}
)

message(STATUS "[ACS] : ACS is set to ${ACS}, and TAREGT to ${TARGET} based on configuration")
message(STATUS "[ACS] : You can change overide target using -DTARGET=<value>")
message(STATUS "[ACS] : Supported values for TARGET: ${TARGET_LIST}")
message(STATUS "[ACS] : Example - cmake --preset bsa -DTARGET=RDV3")
message(STATUS "[ACS] : Check available presets by - `cmake --list-presets`")
message(STATUS "[ACS] : To build using cmake - `cmake --build --preset bsa`")
message(STATUS "[ACS] : To build using make - `cmake --preset acs_all; cd build; make bsa` (to build all baremetal acs - `make acs_all`)")
add_subdirectory(${ROOT_DIR}/tools/cmake/infra)


### Throw waring for the files which is not compiled ###

# list(REMOVE_DUPLICATES COMPILE_LIST)
# execute_process(COMMAND python ${ROOT_DIR}/tools/scripts/compile_check.py "${COMPILE_LIST}" "${ROOT_DIR}" "${TARGET}" OUTPUT_VARIABLE NOT_COMPILED_FILES)
# if(NOT ${NOT_COMPILED_FILES} MATCHES NULL)
#     message(WARNING "Following files are not compiled ${NOT_COMPILED_FILES}")
# endif()
