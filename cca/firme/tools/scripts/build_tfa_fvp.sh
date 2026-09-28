#!/usr/bin/env bash
#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
set -euo pipefail

ACS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

TFA_DIR="${TFA_DIR:-}"
PLAT="${PLAT:-fvp}"
BUILD_BASE="${BUILD_BASE:-${ACS_DIR}/build-tfa}"
BUILD_TYPE="${BUILD_TYPE:-debug}"
BUILD_TFA="${BUILD_TFA:-auto}" # auto|always|never
TFA_LOG_LEVEL="${TFA_LOG_LEVEL:-40}" # 10..50 (ERROR..VERBOSE)
BL2_ENABLE_SP_LOAD="${BL2_ENABLE_SP_LOAD:-0}"
FIRME_SUPPORT="${FIRME_SUPPORT:-1}"
SPMD_SPM_AT_SEL2="${SPMD_SPM_AT_SEL2:-1}" # 1: SPMC at S-EL2, 0: SPMC at S-EL1
CTX_INCLUDE_EL2_REGS="${CTX_INCLUDE_EL2_REGS:-}" # TF-A internal knob; set only when needed
TFA_E="${TFA_E:-}" # TF-A warnings-as-errors (E=1 enables -Werror)
FVP_FIRME_CXL_IDE="${FVP_FIRME_CXL_IDE:-0}"
SUITE="${SUITE:-all}"
SUITE="$(printf '%s' "${SUITE}" | tr '[:upper:]' '[:lower:]')"

case "${SUITE}" in
    all|base|gpt|mec|ide|attest)
        ;;
    *)
        echo "ERROR: unknown SUITE='${SUITE}' (expected all|base|gpt|mec|ide|attest)" >&2
        exit 1
        ;;
esac

if [[ -z "${FVP_ENABLE_IDE+x}" ]]; then
    case "${SUITE}" in
        all|ide)
            FVP_ENABLE_IDE=1
            ;;
        *)
            FVP_ENABLE_IDE=0
            ;;
    esac
fi
if [[ -z "${FVP_IDE_KM_PENDING_OPS_MAX+x}" ]]; then
    FVP_IDE_KM_PENDING_OPS_MAX="${FVP_ENABLE_IDE}"
fi
if [[ -z "${FVP_SIMULATE_IDE_KM_UNIT+x}" ]]; then
    FVP_SIMULATE_IDE_KM_UNIT="${FVP_ENABLE_IDE}"
fi

case "${FVP_FIRME_CXL_IDE}" in
    0|1) ;;
    *)
        echo "ERROR: FVP_FIRME_CXL_IDE must be 0 or 1." >&2
        exit 1
        ;;
esac
if [[ "${FVP_FIRME_CXL_IDE}" == "1" ]]; then
    if [[ "${FVP_ENABLE_IDE}" != "1" || "${PLAT}" != "fvp" ]]; then
        echo "ERROR: the experimental CXL backend requires FVP_ENABLE_IDE=1 and PLAT=fvp." >&2
        exit 1
    fi
    if ! grep -q 'FVP_FIRME_CXL_IDE' "${TFA_DIR}/plat/arm/board/fvp/platform.mk"; then
        echo "ERROR: this TF-A checkout does not provide the experimental CXL backend." >&2
        exit 1
    fi
fi

echo "Building FIRME payloads..."
FIRME_BUILD_DIR="${FIRME_BUILD_DIR:-${ACS_DIR}/build-cmake}"
FIRME_ARTIFACT_DIR="${FIRME_ARTIFACT_DIR:-${ACS_DIR}/build}"
FIRME_ENABLE_CXL_ENDPOINT_PROBE="${FIRME_ENABLE_CXL_ENDPOINT_PROBE:-0}"
FIRME_ENABLE_CXL_SPDM="${FIRME_ENABLE_CXL_SPDM:-0}"
ACS_PRINT_LEVEL="${ACS_PRINT_LEVEL:-3}"
FIRME_MP_SELF_TEST="${FIRME_MP_SELF_TEST:-ON}"
FIRME_TOOLCHAIN="${FIRME_TOOLCHAIN:-gcc}" # gcc|clang
if [[ -z "${FIRME_ENABLE_SVE+x}" ]]; then
    case "${SUITE}" in
        all|base)
            FIRME_ENABLE_SVE=1
            ;;
        *)
            FIRME_ENABLE_SVE=0
            ;;
    esac
fi
if [[ -z "${FIRME_ENABLE_SME+x}" ]]; then
    case "${SUITE}" in
        all|base)
            FIRME_ENABLE_SME=1
            ;;
        *)
            FIRME_ENABLE_SME=0
            ;;
    esac
fi
case "${FIRME_ENABLE_SVE}:${FIRME_ENABLE_SME}" in
    0:0|1:0|1:1)
        ;;
    *)
        echo "ERROR: FIRME_ENABLE_SVE/SME must be 0 or 1, and SME requires SVE." >&2
        exit 1
        ;;
esac

if [[ "${FIRME_ENABLE_SVE}" == "1" ]]; then
    TFA_ENABLE_SVE_FOR_NS="${TFA_ENABLE_SVE_FOR_NS:-2}"
    TFA_ENABLE_SVE_FOR_SWD="${TFA_ENABLE_SVE_FOR_SWD:-1}"
else
    TFA_ENABLE_SVE_FOR_NS="${TFA_ENABLE_SVE_FOR_NS:-0}"
    TFA_ENABLE_SVE_FOR_SWD="${TFA_ENABLE_SVE_FOR_SWD:-0}"
fi
if [[ "${FIRME_ENABLE_SME}" == "1" ]]; then
    TFA_ENABLE_SME_FOR_NS="${TFA_ENABLE_SME_FOR_NS:-2}"
    TFA_ENABLE_SME_FOR_SWD="${TFA_ENABLE_SME_FOR_SWD:-1}"
    TFA_ENABLE_SME2_FOR_NS="${TFA_ENABLE_SME2_FOR_NS:-2}"
else
    TFA_ENABLE_SME_FOR_NS="${TFA_ENABLE_SME_FOR_NS:-0}"
    TFA_ENABLE_SME_FOR_SWD="${TFA_ENABLE_SME_FOR_SWD:-0}"
    TFA_ENABLE_SME2_FOR_NS="${TFA_ENABLE_SME2_FOR_NS:-0}"
fi

if [[ -z "${FIRME_TOOLCHAIN_FILE:-}" ]]; then
    case "${FIRME_TOOLCHAIN}" in
        gcc)
            FIRME_TOOLCHAIN_FILE="${ACS_DIR}/tools/cmake/toolchains/aarch64-none-elf-gcc.cmake"
            ;;
        clang)
            FIRME_TOOLCHAIN_FILE="${ACS_DIR}/tools/cmake/toolchains/aarch64-clang.cmake"
            ;;
        *)
            echo "ERROR: unknown FIRME_TOOLCHAIN='${FIRME_TOOLCHAIN}' (expected gcc|clang)" >&2
            exit 1
            ;;
    esac
fi

need_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "ERROR: required tool not found in PATH: $1" >&2
        return 1
    fi
}

if ! command -v cmake >/dev/null 2>&1; then
    echo "ERROR: cmake not found in PATH (required to build FIRME-ACS payloads)." >&2
    exit 1
fi
if [[ ! -f "${FIRME_TOOLCHAIN_FILE}" ]]; then
    echo "ERROR: FIRME_TOOLCHAIN_FILE not found: ${FIRME_TOOLCHAIN_FILE}" >&2
    exit 1
fi

cross="${CROSS_COMPILE:-}"
if [[ -z "${cross}" ]]; then
    cat <<'EOF'
ERROR: CROSS_COMPILE is not set.

Set CROSS_COMPILE to your AArch64 toolchain prefix (it may include a full path), for example:
  export CROSS_COMPILE=/opt/toolchains/bin/aarch64-none-elf-

EOF
    exit 1
fi
case "${FIRME_TOOLCHAIN}" in
    gcc)
        need_cmd "${cross}gcc" || exit 1
        need_cmd "${cross}ld" || exit 1
        need_cmd "${cross}objcopy" || exit 1
        ;;
    clang)
        need_cmd clang || exit 1
        need_cmd "${cross}ld" || exit 1
        if command -v llvm-objcopy >/dev/null 2>&1; then
            :
        else
            need_cmd "${cross}objcopy" || exit 1
        fi
        ;;
esac

cmake -S "${ACS_DIR}" -B "${FIRME_BUILD_DIR}" \
    -DCMAKE_TOOLCHAIN_FILE="${FIRME_TOOLCHAIN_FILE}" \
    -DFIRME_ARTIFACT_DIR="${FIRME_ARTIFACT_DIR}" \
    -DFIRME_ENABLE_CXL_ENDPOINT_PROBE="${FIRME_ENABLE_CXL_ENDPOINT_PROBE}" \
    -DFIRME_ENABLE_CXL_SPDM="${FIRME_ENABLE_CXL_SPDM}" \
    -DFIRME_LIBSPDM_DIR="${FIRME_LIBSPDM_DIR:-${ACS_DIR}/ext/libspdm}" \
    -DFIRME_SPDM_EMU_DIR="${FIRME_SPDM_EMU_DIR:-${ACS_DIR}/ext/spdm-emu}" \
    -DFIRME_SPDM_ROOT_CERT="${FIRME_SPDM_ROOT_CERT:-}" \
    -DACS_PRINT_LEVEL="${ACS_PRINT_LEVEL}" \
    -DFIRME_MP_SELF_TEST="${FIRME_MP_SELF_TEST}" \
    ${SUITE:+-DSUITE=${SUITE}}
cmake --build "${FIRME_BUILD_DIR}"

if [[ -z "${TFA_DIR}" ]]; then
    echo "ERROR: TFA_DIR is not set (path to your trusted-firmware-a checkout)." >&2
    exit 1
fi

BL32_BIN="${FIRME_ARTIFACT_DIR}/secure.bin"
BL33_BIN="${FIRME_ARTIFACT_DIR}/nsel2.bin"
REL2_BIN="${FIRME_ARTIFACT_DIR}/rel2.bin"

echo "Building TF-A:"
echo "  TFA_DIR   : ${TFA_DIR}"
echo "  PLAT      : ${PLAT}"
echo "  BUILD_BASE: ${BUILD_BASE}"
echo "  BUILD_TYPE: ${BUILD_TYPE}"
echo "  LOG_LEVEL : ${TFA_LOG_LEVEL}"
echo "  BL32      : ${BL32_BIN}"
echo "  BL33      : ${BL33_BIN}"
echo "  REL2      : ${REL2_BIN}"
echo "  BL2_ENABLE_SP_LOAD: ${BL2_ENABLE_SP_LOAD}"
echo "  FIRME_SUPPORT      : ${FIRME_SUPPORT}"
echo "  SPMD_SPM_AT_SEL2  : ${SPMD_SPM_AT_SEL2}"
echo "  ENABLE_SVE_FOR_NS : ${TFA_ENABLE_SVE_FOR_NS}"
echo "  ENABLE_SVE_FOR_SWD: ${TFA_ENABLE_SVE_FOR_SWD}"
echo "  ENABLE_SME_FOR_NS : ${TFA_ENABLE_SME_FOR_NS}"
echo "  ENABLE_SME_FOR_SWD: ${TFA_ENABLE_SME_FOR_SWD}"
echo "  ENABLE_SME2_FOR_NS: ${TFA_ENABLE_SME2_FOR_NS}"
echo "  FIRME IDE KM      : ${FVP_ENABLE_IDE}"
echo "  IDE pending ops   : ${FVP_IDE_KM_PENDING_OPS_MAX}"
echo "  IDE KM simulation : ${FVP_SIMULATE_IDE_KM_UNIT}"
echo "  CXL model backend : ${FVP_FIRME_CXL_IDE} (experimental)"

if [[ "${SPMD_SPM_AT_SEL2}" == "0" ]]; then
    cat <<'EOF'
ERROR: SPMC-at-S-EL1 build requested (SPMD_SPM_AT_SEL2=0), but this TF-A configuration
does not currently link cleanly when ENABLE_RME=1.

Workaround for FIRME:
- Keep SPMC at S-EL2 (default), and use FIRME_ACS_CMD_*_SEL1 to execute Secure bodies
  from S-EL1 via an internal EL2->EL1 trampoline (still exercises SEL1-origin FIRME calls).
EOF
    exit 1
fi

# With ENABLE_RME=1, TF-A needs EL2 sysreg context types for Realm context
# setup; keep CTX_INCLUDE_EL2_REGS enabled. When building SPMC at S-EL1,
# TF-A's SPMD path calls cm_el1_sysregs_context_{save,restore} but the
# prototypes are hidden behind CTX_INCLUDE_EL2_REGS. Build with E=0 (no
# -Werror) to avoid failing on those warnings in this configuration.
if [[ -z "${CTX_INCLUDE_EL2_REGS}" ]]; then
    CTX_INCLUDE_EL2_REGS=1
fi
if [[ -z "${TFA_E}" ]]; then
    if [[ "${SPMD_SPM_AT_SEL2}" == "0" ]]; then
        TFA_E=0
    else
        TFA_E=1
    fi
fi
echo "  CTX_INCLUDE_EL2_REGS: ${CTX_INCLUDE_EL2_REGS}"
echo "  TFA_E              : ${TFA_E}"

#
# TF-A requires SP_LAYOUT_FILE when SPD=spmd and SPMD_SPM_AT_SEL2=1.
# The upstream SP mk generator depends on a Python 'fdt' module which may not
# be installed in minimal dev environments. For this experiment we keep the
# SP layout empty and:
# - Prefer TF-A's generator when the Python dependency is available.
# - Fall back to a minimal pre-created sp_gen.mk only when NUM_SP=0.
#
BUILD_PLAT="${BUILD_BASE}/${PLAT}/${BUILD_TYPE}"
BUILD_PLAT_ABS="$(
    python3 - "${BUILD_PLAT}" <<'PY'
import os
import sys

print(os.path.abspath(sys.argv[1]))
PY
)"
mkdir -p "${BUILD_PLAT}"

BL1_BIN="${BUILD_PLAT_ABS}/bl1.bin"
BL2_BIN="${BUILD_PLAT_ABS}/bl2.bin"
BL31_BIN="${BUILD_PLAT_ABS}/bl31.bin"
FIPTOOL_BIN="${BUILD_PLAT_ABS}/tools/fiptool/fiptool"
FIP_BIN="${BUILD_PLAT_ABS}/fip.bin"
STAMP_FILE="${BUILD_PLAT_ABS}/.firme_build_tfa.stamp"

have_tfa_artifacts() {
    [[ -f "${BL1_BIN}" && -f "${BL2_BIN}" && -f "${BL31_BIN}" ]]
}

sp_layout_sha256() {
    python3 - "$1" <<'PY'
import hashlib, sys
path = sys.argv[1]
h = hashlib.sha256()
with open(path, "rb") as f:
    h.update(f.read())
print(h.hexdigest())
PY
}

tfa_git_rev() {
    if command -v git >/dev/null 2>&1 && [[ -e "${TFA_DIR}/.git" ]]; then
        git -C "${TFA_DIR}" rev-parse HEAD 2>/dev/null || true
    fi
}

tfa_git_status_sha256() {
    if command -v git >/dev/null 2>&1 && [[ -e "${TFA_DIR}/.git" ]]; then
        python3 - "${TFA_DIR}" <<'PY'
import hashlib
import os
import subprocess
import sys

tfa_dir = sys.argv[1]
digest = hashlib.sha256(subprocess.check_output(
    ["git", "-C", tfa_dir, "diff", "--binary", "HEAD", "--"],
    stderr=subprocess.DEVNULL,
))
untracked = subprocess.check_output(
    ["git", "-C", tfa_dir, "ls-files", "--others", "--exclude-standard", "-z"],
    stderr=subprocess.DEVNULL,
)
for name in sorted(filter(None, untracked.split(b"\0"))):
    digest.update(name + b"\0")
    path = os.path.join(os.fsencode(tfa_dir), name)
    if os.path.islink(path):
        digest.update(os.readlink(path))
    else:
        with open(path, "rb") as source:
            for chunk in iter(lambda: source.read(65536), b""):
                digest.update(chunk)
    digest.update(b"\0")
print(digest.hexdigest())
PY
    fi
}

expected_stamp() {
    local rev
    local source_hash
    rev="$(tfa_git_rev)"
    source_hash="$(tfa_git_status_sha256)" || return 1
    cat <<EOF
TFA_DIR=${TFA_DIR}
TFA_GIT_REV=${rev}
TFA_GIT_STATUS_SHA256=${source_hash}
PLAT=${PLAT}
BUILD_TYPE=${BUILD_TYPE}
LOG_LEVEL=${TFA_LOG_LEVEL}
SPD=spmd
ENABLE_RME=1
SPMD_SPM_AT_SEL2=${SPMD_SPM_AT_SEL2}
CTX_INCLUDE_EL2_REGS=${CTX_INCLUDE_EL2_REGS}
ENABLE_SVE_FOR_NS=${TFA_ENABLE_SVE_FOR_NS}
ENABLE_SVE_FOR_SWD=${TFA_ENABLE_SVE_FOR_SWD}
ENABLE_SME_FOR_NS=${TFA_ENABLE_SME_FOR_NS}
ENABLE_SME_FOR_SWD=${TFA_ENABLE_SME_FOR_SWD}
ENABLE_SME2_FOR_NS=${TFA_ENABLE_SME2_FOR_NS}
E=${TFA_E}
BL2_ENABLE_SP_LOAD=${BL2_ENABLE_SP_LOAD}
FIRME_SUPPORT=${FIRME_SUPPORT}
FIRME_SUPPORT_IDE_KM=${FVP_ENABLE_IDE}
PLAT_IDE_KM_PENDING_OPS_MAX=${FVP_IDE_KM_PENDING_OPS_MAX}
FVP_SIMULATE_IDE_KM_UNIT=${FVP_SIMULATE_IDE_KM_UNIT}
FVP_FIRME_CXL_IDE=${FVP_FIRME_CXL_IDE}
SP_LAYOUT_FILE=${ACS_DIR}/tools/config/sp_layout.json
SP_LAYOUT_SHA256=$(sp_layout_sha256 "${ACS_DIR}/tools/config/sp_layout.json")
SPMC_MANIFEST_SHA256=$(sp_layout_sha256 "${ACS_DIR}/tools/config/fvp_spmc_manifest.dts")
EOF
}

stamp_matches() {
    [[ -f "${STAMP_FILE}" ]] || return 1
    diff -q <(expected_stamp) "${STAMP_FILE}" >/dev/null 2>&1
}

sp_layout_has_entries() {
    python3 - "$1" <<'PY'
import json, sys
path = sys.argv[1]
with open(path, "r", encoding="utf-8") as f:
    data = json.load(f)
print(1 if (isinstance(data, dict) and len(data) > 0) else 0)
PY
}

case "${BUILD_TFA}" in
    always)
        DO_TFA_BUILD=1
        ;;
    never)
        DO_TFA_BUILD=0
        ;;
    auto)
        if have_tfa_artifacts && stamp_matches; then
            DO_TFA_BUILD=0
        else
            DO_TFA_BUILD=1
        fi
        ;;
    *)
        echo "ERROR: unknown BUILD_TFA='${BUILD_TFA}' (expected auto|always|never)" >&2
        exit 1
        ;;
esac

if [[ "${DO_TFA_BUILD}" == "1" ]]; then
    if python3 -c 'import fdt' >/dev/null 2>&1; then
        rm -f "${BUILD_PLAT}/sp_gen.mk"
        echo "Python module 'fdt' found: TF-A will generate sp_gen.mk (if needed)."
    else
        if [[ "$(sp_layout_has_entries "${ACS_DIR}/tools/config/sp_layout.json")" != "0" ]]; then
            echo "ERROR: SP layout is non-empty but Python module 'fdt' is missing."
            echo "Fix: use a venv and 'pip install fdt', or provide fdt via system Python."
            exit 1
        fi

        cat > "${BUILD_PLAT}/sp_gen.mk" <<'EOF'
# Auto-generated by FIRME fallback: empty Secure Partition package list.
$(eval $(call add_define_val,NUM_SP,0))
SP_PKGS :=
EOF
    fi

    make -C "${TFA_DIR}" \
        PLAT="${PLAT}" \
        DEBUG=1 \
        LOG_LEVEL="${TFA_LOG_LEVEL}" \
        SPD=spmd \
        ENABLE_RME=1 \
        SPMD_SPM_AT_SEL2="${SPMD_SPM_AT_SEL2}" \
        CTX_INCLUDE_EL2_REGS="${CTX_INCLUDE_EL2_REGS}" \
        ENABLE_SVE_FOR_NS="${TFA_ENABLE_SVE_FOR_NS}" \
        ENABLE_SVE_FOR_SWD="${TFA_ENABLE_SVE_FOR_SWD}" \
        ENABLE_SME_FOR_NS="${TFA_ENABLE_SME_FOR_NS}" \
        ENABLE_SME_FOR_SWD="${TFA_ENABLE_SME_FOR_SWD}" \
        ENABLE_SME2_FOR_NS="${TFA_ENABLE_SME2_FOR_NS}" \
        E="${TFA_E}" \
        BL2_ENABLE_SP_LOAD="${BL2_ENABLE_SP_LOAD}" \
        FIRME_SUPPORT="${FIRME_SUPPORT}" \
        FIRME_SUPPORT_IDE_KM="${FVP_ENABLE_IDE}" \
        PLAT_IDE_KM_PENDING_OPS_MAX="${FVP_IDE_KM_PENDING_OPS_MAX}" \
        FVP_SIMULATE_IDE_KM_UNIT="${FVP_SIMULATE_IDE_KM_UNIT}" \
        FVP_FIRME_CXL_IDE="${FVP_FIRME_CXL_IDE}" \
        SP_LAYOUT_FILE="${ACS_DIR}/tools/config/sp_layout.json" \
        ARM_SPMC_MANIFEST_DTS="${ACS_DIR}/tools/config/fvp_spmc_manifest.dts" \
        BL32="${BL32_BIN}" \
        BL33="${BL33_BIN}" \
        RMM="${REL2_BIN}" \
        BUILD_BASE="${BUILD_BASE}" \
        all dtbs

    expected_stamp > "${STAMP_FILE}"
else
    if ! have_tfa_artifacts; then
        echo "ERROR: TF-A artifacts missing under ${BUILD_PLAT_ABS} but BUILD_TFA=${BUILD_TFA}" >&2
        exit 1
    fi
    echo "Skipping TF-A build (BUILD_TFA=${BUILD_TFA}); repacking fip.bin with updated payloads."
fi

if [[ ! -f "${FIPTOOL_BIN}" ]]; then
    echo "Building fiptool (out-of-tree, no symlink into TF-A source tree)..."
    OPENSSL_DIR="${OPENSSL_DIR:-/usr}"
    make -C "${TFA_DIR}/tools/fiptool" \
        PLAT="${PLAT}" \
        BUILD_PLAT="${BUILD_PLAT_ABS}" \
        OPENSSL_DIR="${OPENSSL_DIR}" \
        DEBUG=1 \
        all
fi

echo "Creating ${FIP_BIN}..."
"${FIPTOOL_BIN}" create \
    --soc-fw-config "${BUILD_PLAT_ABS}/fdts/fvp_soc_fw_config.dtb" \
    --nt-fw-config "${BUILD_PLAT_ABS}/fdts/fvp_nt_fw_config.dtb" \
    --tos-fw-config "${BUILD_PLAT_ABS}/fdts/fvp_spmc_manifest.dtb" \
    --hw-config "${BUILD_PLAT_ABS}/fdts/fvp-base-gicv3-psci.dtb" \
    --fw-config "${BUILD_PLAT_ABS}/fdts/fvp_fw_config.dtb" \
    --tb-fw-config "${BUILD_PLAT_ABS}/fdts/fvp_tb_fw_config.dtb" \
    --tb-fw "${BL2_BIN}" \
    --soc-fw "${BL31_BIN}" \
    --tos-fw "${BL32_BIN}" \
    --rmm-fw "${REL2_BIN}" \
    --nt-fw "${BL33_BIN}" \
    "${FIP_BIN}"

"${FIPTOOL_BIN}" info "${FIP_BIN}"

echo "Done."
echo "Artifacts:"
echo "  ${FIP_BIN}"
echo "  ${BUILD_PLAT_ABS}/bl31.bin"
