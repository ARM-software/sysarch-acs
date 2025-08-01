## @file
 # Copyright (c) 2016-2021, 2025, Arm Limited or its affiliates. All rights reserved.
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

.PHONY: all bsa clean modules_install

export CROSS_COMPILE ?= aarch64-linux-gnu-
export ARCH ?= arm64

#since we have copied the files locally
ACS_DIR ?= .

VAL_SRC = $(ACS_DIR)/src

ifeq ($(ACS), bsa)
obj-m += bsa_acs_val.o
bsa_acs_val-objs += $(VAL_SRC)/acs_status.o      $(VAL_SRC)/acs_memory.o \
    $(VAL_SRC)/acs_peripherals.o $(VAL_SRC)/acs_dma.o  $(VAL_SRC)/acs_smmu.o \
    $(VAL_SRC)/acs_test_infra.o  $(VAL_SRC)/acs_pcie.o  $(VAL_SRC)/acs_pe_infra.o \
    $(VAL_SRC)/acs_iovirt.o $(VAL_SRC)/bsa_execute_test.o\
    $(VAL_SRC)/../driver/smmu_v3/smmu_v3.o $(VAL_SRC)/../driver/pcie/pcie.o

else ifeq ($(ACS), sbsa)
obj-m += sbsa_acs_val.o
sbsa_acs_val-objs += $(VAL_SRC)/acs_status.o      $(VAL_SRC)/acs_memory.o \
    $(VAL_SRC)/acs_peripherals.o  $(VAL_SRC)/acs_smmu.o $(VAL_SRC)/acs_dma.o \
    $(VAL_SRC)/acs_test_infra.o  $(VAL_SRC)/acs_pcie.o $(VAL_SRC)/acs_pe_infra.o \
    $(VAL_SRC)/acs_iovirt.o $(VAL_SRC)/../driver/smmu_v3/smmu_v3.o \
    $(VAL_SRC)/sbsa_execute_test.o $(VAL_SRC)/../driver/pcie/pcie.o

else  ifeq ($(ACS), pcbsa)
obj-m += pcbsa_acs_val.o
pcbsa_acs_val-objs += $(VAL_SRC)/acs_status.o      $(VAL_SRC)/acs_memory.o \
    $(VAL_SRC)/acs_peripherals.o  $(VAL_SRC)/acs_smmu.o $(VAL_SRC)/acs_dma.o\
    $(VAL_SRC)/acs_test_infra.o  $(VAL_SRC)/acs_pcie.o $(VAL_SRC)/acs_pe_infra.o \
    $(VAL_SRC)/acs_iovirt.o    $(VAL_SRC)/../driver/smmu_v3/smmu_v3.o \
    $(VAL_SRC)/pc_bsa_execute_test.o $(VAL_SRC)/../driver/pcie/pcie.o
endif

ccflags-y=-I$(PWD)/$(ACS_DIR)/include -I$(PWD)/$(ACS_DIR)/include -I$(PWD)/$(ACS_DIR) -DTARGET_LINUX -Wall -Werror

all:
ifeq ($(KERNEL_SRC),)
	echo "	KERNEL_SRC variable should be set to kernel path "
	exit 1
else
	echo "Kernel source is set to $(KERNEL_SRC)"
endif

bsa:
	make -C $(KERNEL_SRC) M=$(PWD) ACS=bsa modules

sbsa:
	make -C $(KERNEL_SRC) M=$(PWD) ACS=sbsa modules
pcbsa:
	make -C $(KERNEL_SRC) M=$(PWD) ACS=pcbsa modules

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules_install

clean:
	make -C $(KERNEL_SRC) M=$(PWD) clean
