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

ROOT_DIR := $(abspath $(CURDIR)/../../..)
VAL_ROOT := $(ROOT_DIR)/val
PAL_ROOT := $(ROOT_DIR)/pal
APP_INCLUDE_DIR := $(CURDIR)/include
VAL_DRIVER_DIR := $(VAL_ROOT)/driver

program_NAME := pc_bsa
BUILD_ROOT ?= $(ROOT_DIR)/build/apps
BUILD_DIR := $(BUILD_ROOT)/$(program_NAME)
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin

program_BIN := $(BIN_DIR)/$(program_NAME)
program_C_SRCS := $(abspath $(wildcard *.c))
program_CXX_SRCS := $(abspath $(wildcard *.cpp))
program_C_OBJS := $(patsubst $(ROOT_DIR)/%.c,$(OBJ_DIR)/%.o,$(program_C_SRCS))
program_CXX_OBJS := $(patsubst $(ROOT_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(program_CXX_SRCS))
program_OBJS := $(program_C_OBJS) $(program_CXX_OBJS)
legacy_OBJS := $(program_C_SRCS:.c=.o) $(program_CXX_SRCS:.cpp=.o)

program_INCLUDE_DIRS := \
	$(APP_INCLUDE_DIR) \
	$(ROOT_DIR) \
	$(VAL_ROOT) \
	$(VAL_ROOT)/include \
	$(PAL_ROOT)/include \
	$(VAL_DRIVER_DIR)/gic \
	$(VAL_DRIVER_DIR)/gic/its \
	$(VAL_DRIVER_DIR)/gic/v2 \
	$(VAL_DRIVER_DIR)/gic/v3 \
	$(VAL_DRIVER_DIR)/pcie
program_LIBRARY_DIRS :=
program_LIBRARIES :=
CC := $(CROSS_COMPILE)gcc
CXX := $(CROSS_COMPILE)g++

CPPFLAGS += $(foreach includedir,$(program_INCLUDE_DIRS),-I$(includedir)) -DTARGET_LINUX -g -Werror
LDFLAGS += $(foreach librarydir,$(program_LIBRARY_DIRS),-L$(librarydir))
LDFLAGS += $(foreach library,$(program_LIBRARIES),-l$(library))

.PHONY: all clean distclean $(program_NAME)

all: $(program_BIN)

$(program_NAME): $(program_BIN)

$(program_BIN): $(program_OBJS)
	@mkdir -p $(dir $@)
	$(CC) -static $(program_OBJS) -o $@

$(OBJ_DIR)/%.o: $(ROOT_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(ROOT_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) -c $< -o $@

clean:
	@- $(RM) -r $(BUILD_DIR)
	@- $(RM) $(program_NAME)
	@- $(RM) $(legacy_OBJS)

distclean: clean
