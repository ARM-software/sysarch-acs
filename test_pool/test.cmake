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

# get testsuite directory list
set(TEST_DIR_PATH ${ROOT_DIR}/test)
if(SUITE STREQUAL "all")
    # Get all the test pool components
    _get_sub_dir_list(TEST_SUITE_LIST ${TEST_DIR_PATH})
else()
    set(TEST_SUITE_LIST ${SUITE})
endif()

set(TEST_INCLUDE ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND TEST_INCLUDE
    ${ROOT_DIR}/
    ${ROOT_DIR}/val/include/
    ${ROOT_DIR}/val/src/
    ${ROOT_DIR}/pal/baremetal/base/include/
    ${ROOT_DIR}/pal/baremetal/base/src/
    ${ROOT_DIR}/pal/baremetal/target/${TARGET}/include/
    ${ROOT_DIR}/pal/baremetal/target/${TARGET}/src/
)

set(TEST_LIB ${EXE_NAME}_test_lib)

file(GLOB SUBFOLDERS "${ROOT_DIR}/test_pool/*/")

if(ACS STREQUAL "bsa")
    set(LIST_FILES "${ROOT_DIR}/tools/cmake/infra/bsa_test.txt")
elseif(ACS STREQUAL "sbsa")
    # For SBSA, include both SBSA and BSA tests
    set(LIST_FILES
        "${ROOT_DIR}/tools/cmake/infra/sbsa_test.txt"
        "${ROOT_DIR}/tools/cmake/infra/bsa_test.txt"
    )
elseif(ACS STREQUAL "pc_bsa")
    set(LIST_FILES "${ROOT_DIR}/tools/cmake/infra/pc_bsa_test.txt")
endif()

# Aggregate file names from one or more list files
set(FILE_NAMES "")
foreach(LF IN LISTS LIST_FILES)
    if(EXISTS ${LF})
        file(STRINGS ${LF} TMP_FILE_NAMES)
        list(APPEND FILE_NAMES ${TMP_FILE_NAMES})
    else()
        message(STATUS "List file not found: ${LF}")
    endif()
endforeach()

# Remove duplicates if any
list(REMOVE_DUPLICATES FILE_NAMES)

foreach(FILE_NAME ${FILE_NAMES})
    string(STRIP "${FILE_NAME}" FILE_NAME) # Remove extra spaces or newlines
    set(FILE_FOUND FALSE)
    foreach(SUBFOLDER IN LISTS SUBFOLDERS)
	    set(FILE_PATH "${SUBFOLDER}/${FILE_NAME}")
	    if(EXISTS ${FILE_PATH})
		list(APPEND TEST_SRC "${FILE_PATH}")
                set(FILE_FOUND TRUE)
		break()
	    endif()
    endforeach()
    if(NOT FILE_FOUND)
	    message(STATUS "File doesnt exists: ${FILE_PATH}")
    endif()
endforeach()

        message(dir ="${SUBFOLDER}")

 # Compile all .c/.S files from test directory
 #file(GLOB TEST_SRC
 #    "${ROOT_DIR}/test_pool/*/*.c"
 #)

 # Create TEST library
 add_library(${TEST_LIB} STATIC ${TEST_SRC})

 #Create compile list files
 list(APPEND COMPILE_LIST ${TEST_SRC})
 set(COMPILE_LIST ${COMPILE_LIST} PARENT_SCOPE)

 target_include_directories(${TEST_LIB} PRIVATE ${TEST_INCLUDE}
 )

 create_executable(${EXE_NAME} ${BUILD}/output/ "")
 unset(TEST_SRC)

 # target_include_directories(${VAL_LIB} PRIVATE
 #     ${CMAKE_CURRENT_BINARY_DIR}
 #     ${ROOT_DIR}/val/
 # )


 # test source directory
 set(TEST_SOURCE_DIR ${ROOT_DIR}/test)
 # Get all the test pool components
 _get_sub_dir_list(SUITE_LIST ${TEST_SOURCE_DIR})
