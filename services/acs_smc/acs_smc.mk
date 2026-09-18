## @file
#  Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#  SPDX-License-Identifier : Apache-2.0
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
##

ACS_SMC_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ACS_SMC_PLAT_DIR := $(ACS_SMC_DIR)/platform/$(PLAT)
ACS_SMC_PLAT_HEADER := $(ACS_SMC_PLAT_DIR)/platform_el3.h

# Add ACS EL3 sources to the TF-A BL31 build.
VENDOR_EL3_SRCS += \
        $(wildcard $(ACS_SMC_DIR)/src/*.c) \
        $(wildcard $(ACS_SMC_DIR)/src/AArch64/*.S)

BL31_CPPFLAGS += \
        -I$(ACS_SMC_DIR)/include \
        -I$(ACS_SMC_PLAT_DIR) \
        -include $(ACS_SMC_PLAT_HEADER)

# Redirect the TF-A ACS SMC handler to the ACS implementation.
BL31_LDFLAGS += \
        -Wl,--wrap=plat_arm_acs_smc_handler