## @file
 # Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
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

# Minimal toolchain for aarch64-none-elf cross builds
# Set system name early to avoid passing -arch compiler options on macOS
set(CMAKE_SYSTEM_NAME Generic CACHE STRING "Cross compile system" FORCE)

# Disable -arch flags on macOS
set(CMAKE_OSX_ARCHITECTURES "" CACHE STRING "Disable -arch flags for cross compile" FORCE)

# Pull CROSS_COMPILE from environment if not provided
if(NOT DEFINED CROSS_COMPILE)
    if(DEFINED ENV{CROSS_COMPILE})
        set(CROSS_COMPILE $ENV{CROSS_COMPILE})
    endif()
endif()

if(NOT DEFINED CROSS_COMPILE OR "${CROSS_COMPILE}" STREQUAL "")
    message(FATAL_ERROR "CROSS_COMPILE not defined in environment. Please set CROSS_COMPILE to your toolchain prefix.")
endif()

set(CMAKE_C_COMPILER "${CROSS_COMPILE}gcc" CACHE FILEPATH "C compiler" FORCE)
set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}" CACHE FILEPATH "ASM compiler" FORCE)
set(CMAKE_AR "${CROSS_COMPILE}ar" CACHE FILEPATH "Archiver" FORCE)
set(CMAKE_OBJCOPY "${CROSS_COMPILE}objcopy" CACHE FILEPATH "Objcopy" FORCE)
set(CMAKE_OBJDUMP "${CROSS_COMPILE}objdump" CACHE FILEPATH "Objdump" FORCE)
set(CMAKE_RANLIB "${CROSS_COMPILE}ranlib" CACHE FILEPATH "Ranlib" FORCE)

# Ensure baremetal sources get appropriate platform macro
add_definitions(-DTARGET_BAREMETAL)
