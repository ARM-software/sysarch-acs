#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Convenience wrapper around the CMake build.
# CMake is the single source of truth for payload build wiring.

ACS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

# Toolchain prefix (partners may provide a full path prefix).
CROSS_COMPILE ?= aarch64-linux-gnu-

# CMake build output (intermediate) and final artifacts directory.
FIRME_BUILD_DIR ?= $(ACS_DIR)/build-cmake
FIRME_ARTIFACT_DIR ?= $(ACS_DIR)/build

# Logging verbosity: 1=TRACE, 2=DEBUG, 3=TEST(default), 4=WARN, 5=ERROR.
ACS_PRINT_LEVEL ?= 3

# Suite selection: all (default) | base | gpt | mec | ide | attest
SUITE ?= all

# PAL selection (default: baremetal+fvp).
PAL_ENV ?= baremetal
PAL_TARGET ?= fvp

# Toolchain file selection.
FIRME_TOOLCHAIN ?= gcc # gcc|clang
FIRME_TOOLCHAIN_FILE ?=

ifeq ($(FIRME_TOOLCHAIN_FILE),)
  ifeq ($(FIRME_TOOLCHAIN),gcc)
    FIRME_TOOLCHAIN_FILE := $(ACS_DIR)/tools/cmake/toolchains/aarch64-linux-gnu-gcc.cmake
  else ifeq ($(FIRME_TOOLCHAIN),clang)
    FIRME_TOOLCHAIN_FILE := $(ACS_DIR)/tools/cmake/toolchains/aarch64-clang.cmake
  else
    $(error Invalid FIRME_TOOLCHAIN='$(FIRME_TOOLCHAIN)' (expected gcc|clang))
  endif
endif

.PHONY: all clean configure build payloads

all: payloads

payloads: configure
	@cmake --build "$(FIRME_BUILD_DIR)" --target firme_payloads

configure:
	@CROSS_COMPILE="$(CROSS_COMPILE)" cmake -S "$(ACS_DIR)" -B "$(FIRME_BUILD_DIR)" \
		-DCMAKE_TOOLCHAIN_FILE="$(FIRME_TOOLCHAIN_FILE)" \
		-DFIRME_ARTIFACT_DIR="$(FIRME_ARTIFACT_DIR)" \
		-DACS_PRINT_LEVEL="$(ACS_PRINT_LEVEL)" \
		-DSUITE="$(SUITE)" \
		-DPAL_ENV="$(PAL_ENV)" \
		-DPAL_TARGET="$(PAL_TARGET)"

clean:
	@rm -rf "$(ACS_DIR)/build" "$(ACS_DIR)/build-cmake"
