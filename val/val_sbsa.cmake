## @file
 # Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
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

 file(GLOB VAL_SRC
 "${ROOT_DIR}/val/src/AArch64/*.S"
 "${ROOT_DIR}/val/src/AArch64/*.s"
 "${ROOT_DIR}/val/src/AArch64/*.h"
 "${ROOT_DIR}/val/include/*.h"
 "${ROOT_DIR}/val/src/*.c"
 "${ROOT_DIR}/val/driver/pcie/*.h"
 "${ROOT_DIR}/val/driver/pcie/*.c"
 "${ROOT_DIR}/val/driver/smmu_v3/*.h"
 "${ROOT_DIR}/val/driver/smmu_v3/*.c"
 "${ROOT_DIR}/val/driver/gic/*.h"
 "${ROOT_DIR}/val/driver/gic/*.c"
 "${ROOT_DIR}/val/driver/gic/AArch64/*.S"
 "${ROOT_DIR}/val/driver/gic/its/*.h"
 "${ROOT_DIR}/val/driver/gic/its/*.c"
 "${ROOT_DIR}/val/driver/gic/v2/*.h"
 "${ROOT_DIR}/val/driver/gic/v2/*.c"
 "${ROOT_DIR}/val/driver/gic/v3/*.h"
 "${ROOT_DIR}/val/driver/gic/v3/*.c"
 "${ROOT_DIR}/val/driver/gic/v3/AArch64/*.S"
 "${ROOT_DIR}/apps/baremetal/*.h"
 "${ROOT_DIR}/apps/baremetal/*.c"

)

list(REMOVE_ITEM VAL_SRC
 "${ROOT_DIR}/val/src/AArch64/Drtm.S"
 "${ROOT_DIR}/apps/baremetal/bsa_main.c"
 "${ROOT_DIR}/apps/baremetal/pc_bsa_main.c"
 "${ROOT_DIR}/val/src/bsa_execute_test.c"
 "${ROOT_DIR}/val/src/mpam_execute_test.c"
)

#Create compile list files
list(APPEND COMPILE_LIST ${VAL_SRC})
set(COMPILE_LIST ${COMPILE_LIST} PARENT_SCOPE)

# Create VAL library
add_library(${VAL_LIB} STATIC ${VAL_SRC})

# Define COMPILE_RB_EXE for gating few compilations in val for rule based
# execution infra.
target_compile_definitions(${VAL_LIB} PRIVATE COMPILE_RB_EXE)

target_include_directories(${VAL_LIB} PRIVATE
 ${CMAKE_CURRENT_BINARY_DIR}
 ${ROOT_DIR}/
 ${ROOT_DIR}/val
 ${ROOT_DIR}/val/include/
 ${ROOT_DIR}/val/src/AArch64/
 ${ROOT_DIR}/val/driver/smmu_v3/
 ${ROOT_DIR}/val/driver/gic/
 ${ROOT_DIR}/val/driver/gic/its/
 ${ROOT_DIR}/val/driver/gic/v2/
 ${ROOT_DIR}/val/driver/gic/v3/
 ${ROOT_DIR}/pal/baremetal/
 ${ROOT_DIR}/pal/baremetal/include/
 ${ROOT_DIR}/pal/baremetal/target/${TARGET}/include/
 ${ROOT_DIR}/pal/baremetal/src/AArch64/
 ${ROOT_DIR}/apps/baremetal/
)

unset(VAL_SRC)
