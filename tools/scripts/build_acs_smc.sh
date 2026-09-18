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

#!/usr/bin/env bash

set -euo pipefail

usage()
{
    cat <<EOF
Build TF-A with the ACS SMC service

Usage:
  $(basename "$0") TFA_PATH=<path> PLAT=<name> [--all] [NAME=value ...]

Required:
  TFA_PATH       TF-A source tree
  PLAT           Platform supported by TF-A and ACS

Optional:
  ACS_PATH       ACS source tree (default: detected)
  CC             AArch64 GCC executable, directory, or prefix (default: detected)
  MBEDTLS_DIR    Mbed TLS source tree (default: derived when required)
  STACK_PATH     RD stack root; enables copy and packaging

Options:
  -a, --all      Build all TF-A targets (default: bl31 only)
  -h, --help     Show this help

Other NAME=value arguments are passed directly to TF-A.
EOF
}

case "${1:-}" in
    -h|--help)
        usage
        exit 0
        ;;
esac

VALIDATION_ERRORS=()

add_error()
{
    VALIDATION_ERRORS+=("$*")
}

validate_file()
{
    if [ ! -f "$1" ]; then
        add_error "File not found: $1"
    fi
}

validate_dir()
{
    if [ ! -d "$1" ]; then
        add_error "Directory not found: $1"
    fi
}

report_validation_errors()
{
    if [ "${#VALIDATION_ERRORS[@]}" -eq 0 ]; then
        return
    fi

    echo >&2
    echo "Configuration validation failed:" >&2
    echo >&2

    for msg in "${VALIDATION_ERRORS[@]}"; do
        echo "  - $msg" >&2
    done

    echo >&2
    exit 1
}

###############################################################################
# Command-line arguments
###############################################################################

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TFA_PATH="${TFA_PATH:-}"
PLAT="${PLAT:-}"
ACS_PATH="${ACS_PATH:-${SCRIPT_DIR}/../..}"
CC_INPUT="${CC:-}"
MBEDTLS_DIR="${MBEDTLS_DIR:-}"
STACK_PATH="${STACK_PATH:-}"
OUTDIR="${OUTDIR:-}"
PACKAGE_SCRIPT="${PACKAGE_SCRIPT:-}"
BUILD_TARGET="bl31"
TF_A_BUILD_FLAGS=()

while [ "$#" -gt 0 ]; do
    case "$1" in
        -h|--help)
            usage
            exit 0
            ;;
        -a|--all)      BUILD_TARGET="all" ;;
        TFA_PATH=*)    TFA_PATH="${1#*=}" ;;
        PLAT=*)        PLAT="${1#*=}" ;;
        ACS_PATH=*)    ACS_PATH="${1#*=}" ;;
        CC=*)          CC_INPUT="${1#*=}" ;;
        MBEDTLS_DIR=*) MBEDTLS_DIR="${1#*=}" ;;
        STACK_PATH=*)  STACK_PATH="${1#*=}" ;;
        TARGET=*)
            add_error "TARGET is not supported; use --all or omit it for bl31"
            ;;
        CROSS_COMPILE=*|ENABLE_ACS_SMC=*|BUILD_BASE=*|BUILD_PLAT=*)
            add_error "'$1' is managed by this script"
            ;;
        DEBUG=*)
            if [ "${1#*=}" != "0" ] && [ "${1#*=}" != "1" ]; then
                add_error "DEBUG must be 0 or 1"
            fi
            TF_A_BUILD_FLAGS+=("$1")
            ;;
        NRD_PLATFORM_VARIANT=*)
            if [[ ! "${1#*=}" =~ ^[A-Za-z0-9_-]+$ ]]; then
                add_error "Invalid NRD_PLATFORM_VARIANT '${1#*=}'"
            fi
            TF_A_BUILD_FLAGS+=("$1")
            ;;
        *=*)           TF_A_BUILD_FLAGS+=("$1") ;;
        *)
            add_error "Invalid argument '$1'; expected NAME=value"
            ;;
    esac
    shift
done

if [ -z "$TFA_PATH" ]; then
    add_error "TFA_PATH is required"
elif [ ! -d "$TFA_PATH" ]; then
    add_error "Invalid TFA_PATH: $TFA_PATH"
else
    TFA_PATH="$(realpath "$TFA_PATH")"
fi

if [ -z "$PLAT" ]; then
    add_error "PLAT is required"
elif [[ ! "$PLAT" =~ ^[A-Za-z0-9_.-]+$ ]]; then
    add_error "Invalid PLAT '$PLAT'"
fi

if [ ! -d "$ACS_PATH" ]; then
    add_error "Invalid ACS directory: $ACS_PATH"
else
    ACS_PATH="$(realpath "$ACS_PATH")"
fi

###############################################################################
# Stack and Mbed TLS paths
###############################################################################

if [ -n "$STACK_PATH" ]; then
    if [ ! -d "$STACK_PATH" ]; then
        add_error "Invalid STACK_PATH: $STACK_PATH"
    else
        STACK_PATH="$(realpath "$STACK_PATH")"
        OUTDIR="${OUTDIR:-${STACK_PATH}/output}"
        PACKAGE_SCRIPT="${PACKAGE_SCRIPT:-${STACK_PATH}/build-scripts/rdinfra/build-test-acs.sh}"
        if [ -z "$MBEDTLS_DIR" ] && [ -d "${STACK_PATH}/mbedtls" ]; then
            MBEDTLS_DIR="${STACK_PATH}/mbedtls"
        fi
    fi
else
    OUTDIR=""
    PACKAGE_SCRIPT=""
fi

###############################################################################
# ACS SMC service paths
###############################################################################

ACS_SMC_DIR="${ACS_PATH}/services/acs_smc"
ACS_SMC_MK="${ACS_SMC_DIR}/acs_smc.mk"
ACS_SMC_INCLUDE_DIR="${ACS_SMC_DIR}/include"
ACS_SMC_SOURCE_DIR="${ACS_SMC_DIR}/src"
ACS_SMC_ASM_DIR="${ACS_SMC_SOURCE_DIR}/AArch64"
ACS_SMC_PLATFORM_DIR="${ACS_SMC_DIR}/platform/${PLAT}"
ACS_SMC_PLATFORM_HEADER="${ACS_SMC_PLATFORM_DIR}/platform_el3.h"

###############################################################################
# Validate input paths
###############################################################################

validate_dir "$ACS_SMC_DIR"
validate_dir "$ACS_SMC_INCLUDE_DIR"
validate_dir "$ACS_SMC_SOURCE_DIR"
validate_dir "$ACS_SMC_ASM_DIR"
validate_file "$ACS_SMC_MK"

if [ -n "$PLAT" ]; then
    if [ ! -d "$ACS_SMC_PLATFORM_DIR" ]; then
        add_error "Platform '$PLAT' is not supported by ACS SMC; expected directory: $ACS_SMC_PLATFORM_DIR"
    elif [ ! -f "$ACS_SMC_PLATFORM_HEADER" ]; then
        add_error "ACS SMC platform header missing for '$PLAT': $ACS_SMC_PLATFORM_HEADER"
    fi
fi

if [ -d "$TFA_PATH" ] && [ -n "$PLAT" ]; then
    validate_file "${TFA_PATH}/Makefile"
    validate_dir "${TFA_PATH}/plat"
    validate_dir "${TFA_PATH}/include"

    TFA_PLATFORM_MK="$(find "${TFA_PATH}/plat" -type f \
        -path "*/${PLAT}/platform.mk" -print -quit 2>/dev/null || true)"
    if [ -z "$TFA_PLATFORM_MK" ]; then
        add_error "Platform '$PLAT' is not supported by TF-A"
    fi
fi

echo "Checking dependencies..."

for tool in make nproc python3 dtc openssl find grep awk realpath cp rm mkdir; do
    if command -v "$tool" >/dev/null 2>&1; then
        echo "  [OK] $tool"
    else
        add_error "Required tool not found: $tool"
    fi
done

COMPILER=""
if [ -n "$CC_INPUT" ]; then
    if [ -d "$CC_INPUT" ]; then
        for candidate in aarch64-none-elf-gcc aarch64-linux-gnu-gcc gcc; do
            if [ -x "${CC_INPUT}/${candidate}" ]; then
                COMPILER="${CC_INPUT}/${candidate}"
                break
            fi
        done
    elif [[ "$CC_INPUT" == *gcc ]]; then
        COMPILER="$CC_INPUT"
    else
        COMPILER="${CC_INPUT}gcc"
    fi
else
    case "$(uname -m)" in
        aarch64|arm64)
            COMPILER="$(command -v gcc 2>/dev/null || true)"
            ;;
        *)
            for candidate in aarch64-none-elf-gcc aarch64-linux-gnu-gcc; do
                if command -v "$candidate" >/dev/null 2>&1; then
                    COMPILER="$(command -v "$candidate")"
                    break
                fi
            done
            ;;
    esac
fi

if [ -n "$COMPILER" ] && command -v "$COMPILER" >/dev/null 2>&1; then
    COMPILER="$(command -v "$COMPILER")"
    COMPILER_TARGET="$($COMPILER -dumpmachine 2>/dev/null || true)"
    if [[ "$COMPILER_TARGET" != aarch64* ]]; then
        add_error "Compiler does not target AArch64: $COMPILER ($COMPILER_TARGET)"
    fi
    CROSS_COMPILE="${COMPILER%gcc}"
    echo "  [OK] compiler: $COMPILER"
    for tool in objcopy objdump gcc-ar; do
        if [ -x "${CROSS_COMPILE}${tool}" ]; then
            echo "  [OK] ${CROSS_COMPILE}${tool}"
        else
            add_error "Required compiler tool not found: ${CROSS_COMPILE}${tool}"
        fi
    done
else
    CROSS_COMPILE=""
    add_error "AArch64 GCC not found; set CC=<compiler, directory, or prefix>"
fi

if [ -z "$MBEDTLS_DIR" ] && [ -n "${TFA_PLATFORM_MK:-}" ] && \
   grep -qi mbedtls "$TFA_PLATFORM_MK"; then

    DEFAULT_MBEDTLS_DIR="${TFA_PATH}/contrib/mbed-tls"

    if [ -d "${DEFAULT_MBEDTLS_DIR}/library" ] && \
       [ -f "${DEFAULT_MBEDTLS_DIR}/include/mbedtls/build_info.h" ]; then
        MBEDTLS_DIR="$DEFAULT_MBEDTLS_DIR"
    else
        add_error "Platform '$PLAT' requires Mbed TLS; set MBEDTLS_DIR=<path>"
    fi
fi

if [ -n "$MBEDTLS_DIR" ]; then
    if [ ! -d "${MBEDTLS_DIR}/library" ] || \
       [ ! -f "${MBEDTLS_DIR}/include/mbedtls/build_info.h" ]; then
        add_error "Invalid Mbed TLS source tree: $MBEDTLS_DIR"
    else
        MBEDTLS_DIR="$(realpath "$MBEDTLS_DIR")"
        MBEDTLS_MAJOR="$(awk '/MBEDTLS_VERSION_MAJOR/ {print $3; exit}' \
            "${MBEDTLS_DIR}/include/mbedtls/build_info.h")"
        MBEDTLS_VERSION="$(awk '$2 == "MBEDTLS_VERSION_STRING" {gsub(/"/, "", $3); print $3; exit}' \
            "${MBEDTLS_DIR}/include/mbedtls/build_info.h")"
        MBEDTLS_VERSION_NUMBER="$(awk '$2 == "MBEDTLS_VERSION_NUMBER" {print $3; exit}' \
            "${MBEDTLS_DIR}/include/mbedtls/build_info.h")"
        TFA_MBEDTLS_CONFIG="${TFA_PATH}/include/drivers/auth/mbedtls/default_mbedtls_config.h"
        TFA_MBEDTLS_MIN=""
        if [ -f "$TFA_MBEDTLS_CONFIG" ]; then
            TFA_MBEDTLS_MIN="$(awk '$2 == "MBEDTLS_CONFIG_VERSION" {print $3; exit}' \
                "$TFA_MBEDTLS_CONFIG")"
        fi
        if [ "$MBEDTLS_MAJOR" != "3" ]; then
            add_error "Unsupported Mbed TLS version ${MBEDTLS_VERSION:-$MBEDTLS_MAJOR.x}; TF-A requires 3.x"
        elif [ -n "$TFA_MBEDTLS_MIN" ] && [ -n "$MBEDTLS_VERSION_NUMBER" ] &&
             (( MBEDTLS_VERSION_NUMBER < TFA_MBEDTLS_MIN )); then
            TFA_MBEDTLS_MIN_NUMBER=$((TFA_MBEDTLS_MIN))
            printf -v TFA_MBEDTLS_MIN_VERSION '%d.%d.%d' \
                "$(((TFA_MBEDTLS_MIN_NUMBER >> 24) & 255))" \
                "$(((TFA_MBEDTLS_MIN_NUMBER >> 16) & 255))" \
                "$(((TFA_MBEDTLS_MIN_NUMBER >> 8) & 255))"
            add_error "Mbed TLS ${MBEDTLS_VERSION} is too old; this TF-A requires >= ${TFA_MBEDTLS_MIN_VERSION}"
        else
            echo "  [OK] Mbed TLS: ${MBEDTLS_VERSION}"
        fi
    fi
fi

echo
echo "============================================================"
echo "ACS-enabled TF-A build"
echo "Build target      : $BUILD_TARGET"
echo "============================================================"
echo "ACS path          : $ACS_PATH"
echo "TF-A path         : $TFA_PATH"
echo "Platform          : $PLAT"
echo "Compiler          : ${COMPILER:-not found}"
if [ -n "$MBEDTLS_DIR" ]; then
    echo "Mbed TLS path     : $MBEDTLS_DIR"
else
    echo "Mbed TLS path     : not specified"
fi
if [ -n "$STACK_PATH" ]; then
    echo "Stack path        : $STACK_PATH"
    echo "Output directory  : $OUTDIR"
else
    echo "Build mode        : standalone"
fi
echo "============================================================"
echo

if [ -n "$STACK_PATH" ] && [ -n "$PACKAGE_SCRIPT" ]; then
    if [ ! -x "$PACKAGE_SCRIPT" ]; then
        add_error "Stack packaging script not found: $PACKAGE_SCRIPT"
    fi
fi

report_validation_errors
echo "Validation passed."

###############################################################################
# Step 1: Copy ACS SMC services into TF-A
###############################################################################

TFA_ACS_SMC_DIR="${TFA_PATH}/plat/arm/common/acs_smc"

echo
echo "Checking ACS SMC services in TF-A"

if [ -d "$TFA_ACS_SMC_DIR" ]; then
    echo "ACS SMC services already exist in TF-A:"
    echo "  $TFA_ACS_SMC_DIR"
    echo "Skipping copy"
else
    echo "Copying ACS SMC services into TF-A"
    echo "  Source      : $ACS_SMC_DIR"
    echo "  Destination : $TFA_ACS_SMC_DIR"

    cp -a "$ACS_SMC_DIR" "$TFA_ACS_SMC_DIR"

    echo "ACS SMC services copied successfully"
fi

###############################################################################
# Step 2: Determine the TF-A output configuration
###############################################################################

BUILD_MODE="release"
PLATFORM_VARIANT=""

for flag in "${TF_A_BUILD_FLAGS[@]}"; do
    case "$flag" in
        DEBUG=1)
            BUILD_MODE="debug"
            ;;

        DEBUG=0)
            BUILD_MODE="release"
            ;;

        NRD_PLATFORM_VARIANT=*)
            PLATFORM_VARIANT="${flag#*=}"
            ;;
    esac
done

if [ -n "$PLATFORM_VARIANT" ]; then
    BL31_BUILD_DIR="${TFA_PATH}/build/${PLAT}/${PLATFORM_VARIANT}/${BUILD_MODE}"
else
    BL31_BUILD_DIR="${TFA_PATH}/build/${PLAT}/${BUILD_MODE}"
fi

echo
echo "TF-A output configuration:"
echo "  Build mode      : $BUILD_MODE"
echo "  Platform variant: ${PLATFORM_VARIANT:-none}"
echo "  BL31 directory  : $BL31_BUILD_DIR"

###############################################################################
# Step 3: Remove stale BL31 objects
###############################################################################

echo
echo "Removing stale BL31 objects"

rm -rf "${BL31_BUILD_DIR}/bl31"
rm -f "${BL31_BUILD_DIR}/bl31.bin"

###############################################################################
# Step 4: Build TF-A
###############################################################################

TF_A_COMMON_FLAGS=(
    "-j$(nproc)"
    "PLAT=${PLAT}"
    "ENABLE_ACS_SMC=1"
)

if [ -n "$MBEDTLS_DIR" ]; then
    TF_A_COMMON_FLAGS+=("MBEDTLS_DIR=${MBEDTLS_DIR}")
fi

if [ -n "$CROSS_COMPILE" ]; then
    TF_A_COMMON_FLAGS+=("CROSS_COMPILE=${CROSS_COMPILE}")
fi

echo
echo "Common TF-A build flags:"
printf '  %q\n' "${TF_A_COMMON_FLAGS[@]}"

echo
echo "Additional TF-A build flags:"

if [ "${#TF_A_BUILD_FLAGS[@]}" -gt 0 ]; then
    printf '  %q\n' "${TF_A_BUILD_FLAGS[@]}"
else
    echo "  None"
fi

echo "ACS-enabled TF-A build"
echo "Build target      : $BUILD_TARGET"

printf 'Command: make -C %q' "$TFA_PATH"
printf ' -f %q' "$ACS_SMC_MK"
printf ' -f Makefile'
printf ' %q' "${TF_A_COMMON_FLAGS[@]}"
if [ "${#TF_A_BUILD_FLAGS[@]}" -gt 0 ]; then
    printf ' %q' "${TF_A_BUILD_FLAGS[@]}"
fi
printf ' %q\n\n' "$BUILD_TARGET"

make -C "$TFA_PATH" \
    -f "$ACS_SMC_MK" \
    -f Makefile \
    "${TF_A_COMMON_FLAGS[@]}" \
    "${TF_A_BUILD_FLAGS[@]}" \
    "$BUILD_TARGET"

###############################################################################
# Step 5: Validate the generated BL31 output
###############################################################################

BL31_BIN="${BL31_BUILD_DIR}/bl31.bin"
BL31_ELF="${BL31_BUILD_DIR}/bl31/bl31.elf"

if [ ! -f "$BL31_BIN" ]; then
    echo "ERROR: BL31 binary not generated: $BL31_BIN" >&2
    exit 1
fi

echo
echo "============================================================"
echo "TF-A build completed successfully"
echo "============================================================"
echo "BL31 binary:"
echo "  $BL31_BIN"

if [ -f "$BL31_ELF" ]; then
    echo "BL31 ELF:"
    echo "  $BL31_ELF"
fi

###############################################################################
# Step 6: Optional stack packaging
###############################################################################

if [ -n "$STACK_PATH" ]; then
    STACK_OUTPUT_DIR="${OUTDIR}/${PLAT}"

    echo
    echo "Copying BL31 into the stack output"
    mkdir -p "$STACK_OUTPUT_DIR"
    cp -f "$BL31_BIN" "${STACK_OUTPUT_DIR}/tf-bl31.bin"

    if [ "${ACS_BL31_REPACKAGE_ACTIVE:-0}" != "1" ]; then
        echo "Packaging the stack"
        (
            cd "$STACK_PATH"
            ACS_BL31_REPACKAGE_ACTIVE=1 \
                "$PACKAGE_SCRIPT" -p "$PLAT" package
        )
    fi
fi

echo
echo "============================================================"
echo "ACS TF-A build completed successfully"
echo "============================================================"
