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

project(bsa LANGUAGES C ASM)

if(ACS STREQUAL "bsa")
set(EXE_NAME bsa)
set(VAL_LIB ${EXE_NAME}_val_lib)
set(PAL_LIB ${EXE_NAME}_pal_lib)
include(${ROOT_DIR}/val/val_bsa.cmake)
endif()

if(ACS STREQUAL "sbsa")
set(EXE_NAME sbsa)
set(VAL_LIB ${EXE_NAME}_val_lib)
set(PAL_LIB ${EXE_NAME}_pal_lib)
include(${ROOT_DIR}/val/val_sbsa.cmake)
endif()

if(ACS STREQUAL "pc_bsa")
set(EXE_NAME pc_bsa)
set(VAL_LIB ${EXE_NAME}_val_lib)
set(PAL_LIB ${EXE_NAME}_pal_lib)
include(${ROOT_DIR}/val/val_pcbsa.cmake)
endif()

include(${ROOT_DIR}/pal/baremetal/pal.cmake)
include(${ROOT_DIR}/test_pool/test.cmake)
