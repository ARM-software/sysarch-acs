#!/usr/bin/env bash
#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
set -euo pipefail

ACS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

: "${MODEL_BASE:?MODEL_BASE must point to the FVP binary (e.g. FVP_Base_RevC-2xAEMv8A)}"

TFA_DIR="${TFA_DIR:-}"
PLAT="${PLAT:-fvp}"
BUILD_BASE="${BUILD_BASE:-${ACS_DIR}/build-tfa}"
BUILD_TYPE="${BUILD_TYPE:-debug}"

UART_LOG="${UART_LOG:-${ACS_DIR}/build-fvp-uart.log}"
UART1_LOG="${UART1_LOG:-${ACS_DIR}/build-fvp-uart1.log}"
UART2_LOG="${UART2_LOG:-${ACS_DIR}/build-fvp-uart2.log}"
UART3_LOG="${UART3_LOG:-${ACS_DIR}/build-fvp-uart3.log}"
MODEL_LOG="${MODEL_LOG:-${ACS_DIR}/build-fvp-model.log}"
FVP_TIMEOUT_SECS="${FVP_TIMEOUT_SECS:-0}"
FVP_ENABLE_TELNET="${FVP_ENABLE_TELNET:-1}"
FVP_TELNET_PORTS="${FVP_TELNET_PORTS:-5000,5001,5002,5003}"
FVP_TELNET_LOG_DIR="${FVP_TELNET_LOG_DIR:-${ACS_DIR}/build-fvp-telnet}"
FVP_TELNET_MERGED_LOG="${FVP_TELNET_MERGED_LOG:-${FVP_TELNET_LOG_DIR}/merged.log}"
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

FVP_ENABLE_RME="${FVP_ENABLE_RME:-1}"
FVP_RME_SUPPORT_LEVEL="${FVP_RME_SUPPORT_LEVEL:-2}"
if [[ -z "${FVP_ENABLE_MEC+x}" ]]; then
    case "${SUITE}" in
        all|mec)
            FVP_ENABLE_MEC=1
            ;;
        *)
            FVP_ENABLE_MEC=0
            ;;
    esac
fi
FVP_MEC_SUPPORT_LEVEL="${FVP_MEC_SUPPORT_LEVEL:-2}"
FVP_MECID_WIDTH="${FVP_MECID_WIDTH:-16}"
FVP_ENABLE_SMMU_MEC="${FVP_ENABLE_SMMU_MEC:-0}"
if [[ -z "${FVP_CACHE_STATE_MODELLED+x}" ]]; then
    if [[ "${FVP_ENABLE_MEC}" == "1" ]]; then
        FVP_CACHE_STATE_MODELLED=0
    else
        FVP_CACHE_STATE_MODELLED=1
    fi
fi
BUILD_MODE="${BUILD_MODE:-auto}" # auto|always|never

BUILD_PLAT="${BUILD_BASE}/${PLAT}/${BUILD_TYPE}"
BUILD_STAMP="${BUILD_PLAT}/firme-build.stamp"

need_build_auto() {
    python3 - "$ACS_DIR" "$BUILD_PLAT/fip.bin" "$BUILD_STAMP" "$SUITE" <<'PY'
import os, sys

acs_dir = sys.argv[1]
fip = sys.argv[2]
stamp = sys.argv[3]
suite = sys.argv[4]

SOURCE_SUFFIXES = {
    ".c", ".h", ".S", ".s", ".ld", ".mk", ".make", ".json", ".dts", ".dtsi",
    ".sh", ".py", ".md", ".txt", ".cmake",
}

def is_ignored(path: str) -> bool:
    rel = os.path.relpath(path, acs_dir)
    if rel == ".":
        return True
    parts = rel.split(os.sep)
    if parts[0] in ("build", "build-tfa", ".git", ".venv", "__pycache__"):
        return True
    _, ext = os.path.splitext(rel)
    if ext and ext not in SOURCE_SUFFIXES:
        return True
    return False

latest = 0.0
for root, dirs, files in os.walk(acs_dir):
    # prune ignored dirs
    dirs[:] = [d for d in dirs if not is_ignored(os.path.join(root, d))]
    for name in files:
        p = os.path.join(root, name)
        if is_ignored(p):
            continue
        try:
            st = os.stat(p)
        except FileNotFoundError:
            continue
        latest = max(latest, st.st_mtime)

try:
    fip_mtime = os.stat(fip).st_mtime
except FileNotFoundError:
    print("1")
    sys.exit(0)

try:
    with open(stamp, "r", encoding="utf-8") as stamp_file:
        stamp_lines = stamp_file.read().splitlines()
except FileNotFoundError:
    print("1")
    sys.exit(0)

if f"SUITE={suite}" not in stamp_lines:
    print("1")
    sys.exit(0)

print("1" if latest > fip_mtime else "0")
PY
}

case "${BUILD_MODE}" in
    always)
        DO_BUILD=1
        ;;
    never)
        DO_BUILD=0
        ;;
    auto)
        DO_BUILD="$(need_build_auto)"
        ;;
    *)
        echo "ERROR: unknown BUILD_MODE='${BUILD_MODE}' (expected auto|always|never)" >&2
        exit 1
        ;;
esac

if [[ "${DO_BUILD}" == "1" ]]; then
    echo "[run] Building TF-A + FIRME payloads (BUILD_MODE=${BUILD_MODE})..."
    if [[ -z "${TFA_DIR}" ]]; then
        echo "ERROR: TFA_DIR is not set (path to your trusted-firmware-a checkout)." >&2
        exit 1
    fi
    TFA_DIR="${TFA_DIR}" PLAT="${PLAT}" BUILD_BASE="${BUILD_BASE}" BUILD_TYPE="${BUILD_TYPE}" \
        SUITE="${SUITE}" \
        BUILD_TFA="${BUILD_TFA:-auto}" \
        bash "${ACS_DIR}/tools/scripts/build_tfa_fvp.sh"
    printf 'SUITE=%s\n' "${SUITE}" > "${BUILD_STAMP}"
else
    echo "[run] Skipping build (BUILD_MODE=${BUILD_MODE})"
fi

BL1_BIN="${BUILD_PLAT}/bl1.bin"
FIP_BIN="${BUILD_PLAT}/fip.bin"

if [[ ! -f "${BL1_BIN}" ]]; then
    echo "ERROR: BL1 not found: ${BL1_BIN}" >&2
    exit 1
fi
if [[ ! -f "${FIP_BIN}" ]]; then
    echo "ERROR: FIP not found: ${FIP_BIN}" >&2
    exit 1
fi

echo "[run] FVP:"
echo "  MODEL_BASE : ${MODEL_BASE}"
echo "  BL1        : ${BL1_BIN}"
echo "  FIP        : ${FIP_BIN}"
echo "  UART_LOG   : ${UART_LOG}"
echo "  UART1_LOG  : ${UART1_LOG}"
echo "  UART2_LOG  : ${UART2_LOG}"
echo "  UART3_LOG  : ${UART3_LOG}"
echo "  MODEL_LOG  : ${MODEL_LOG}"
echo "  SUITE      : ${SUITE}"
echo "  TELNET     : ${FVP_ENABLE_TELNET} (ports ${FVP_TELNET_PORTS})"
echo "  TELNET_DIR : ${FVP_TELNET_LOG_DIR}"
echo "  RME        : ${FVP_ENABLE_RME} (support level ${FVP_RME_SUPPORT_LEVEL})"
echo "  MEC        : ${FVP_ENABLE_MEC} (support level ${FVP_MEC_SUPPORT_LEVEL}, MECID width ${FVP_MECID_WIDTH})"
echo "  SMMU MEC   : ${FVP_ENABLE_SMMU_MEC}"

mkdir -p "$(dirname "${UART_LOG}")"
mkdir -p "$(dirname "${UART1_LOG}")"
mkdir -p "$(dirname "${UART2_LOG}")"
mkdir -p "$(dirname "${UART3_LOG}")"
mkdir -p "$(dirname "${MODEL_LOG}")"
mkdir -p "${FVP_TELNET_LOG_DIR}"

FVP_ARGS=(
    -C pctl.startup=0.0.0.0
    -C bp.vis.disable_visualisation=1
    -C bp.secure_memory=1
    -C bp.tzc_400.diagnostics=1
    -C cluster0.NUM_CORES=4
    -C cluster1.NUM_CORES=4
    -C cache_state_modelled="${FVP_CACHE_STATE_MODELLED}"
    -C bp.secureflashloader.fname="${BL1_BIN}"
    -C bp.flashloader0.fname="${FIP_BIN}"
    -C bp.pl011_uart0.uart_enable=1
    -C bp.pl011_uart0.out_file="${UART_LOG}"
    -C bp.pl011_uart0.unbuffered_output=1
    -C bp.pl011_uart1.uart_enable=1
    -C bp.pl011_uart1.out_file="${UART1_LOG}"
    -C bp.pl011_uart1.unbuffered_output=1
    -C bp.pl011_uart2.uart_enable=1
    -C bp.pl011_uart2.out_file="${UART2_LOG}"
    -C bp.pl011_uart2.unbuffered_output=1
    -C bp.pl011_uart3.uart_enable=1
    -C bp.pl011_uart3.out_file="${UART3_LOG}"
    -C bp.pl011_uart3.unbuffered_output=1
    -C bp.terminal_0.start_telnet=0
    -C bp.terminal_1.start_telnet=0
    -C bp.terminal_2.start_telnet=0
    -C bp.terminal_3.start_telnet=0
    -C bp.terminal_0.terminal_command=/bin/true
    -C bp.terminal_1.terminal_command=/bin/true
    -C bp.terminal_2.terminal_command=/bin/true
    -C bp.terminal_3.terminal_command=/bin/true
)

if [[ "${FVP_ENABLE_RME}" == "1" ]]; then
    # Match the RME enablement knobs used by shrinkwrap configs:
    # -C bp.has_rme=1
    # -C clusterX.rme_support_level=2
    # Also enable FEAT_CSV2_2 knobs that some TF-A builds require with ENABLE_RME=1.
    FVP_ARGS+=(
        -C bp.has_rme=1
        -C cluster0.rme_support_level="${FVP_RME_SUPPORT_LEVEL}"
        -C cluster1.rme_support_level="${FVP_RME_SUPPORT_LEVEL}"
        -C cluster0.restriction_on_speculative_execution=2
        -C cluster1.restriction_on_speculative_execution=2
        -C cluster0.restriction_on_speculative_execution_aarch32=2
        -C cluster1.restriction_on_speculative_execution_aarch32=2
    )
fi

if [[ "${FVP_ENABLE_MEC}" == "1" ]]; then
    mec_output_attrs="ExtendedID[62:55]=MPAM_PMG,ExtendedID[38]=MPAM_SP[0],ExtendedID[37]=MPAM_SP[1],UserFlags[31:16]=MECID"
    FVP_ARGS+=(
        -C cluster0.mec_support_level="${FVP_MEC_SUPPORT_LEVEL}"
        -C cluster1.mec_support_level="${FVP_MEC_SUPPORT_LEVEL}"
        -C bp.mpe.block_size_in_bytes=4096
        -C bp.mpe.corruption_strategy=0
        -C bp.mpe.enable=1
        -C bp.mpe.ignore_mecid=0
        -C bp.mpe.non_secure_pas_enc_key=34
        -C bp.mpe.realm_pas_enc_key=136
        -C bp.mpe.root_pas_enc_key=68
        -C bp.mpe.secure_pas_enc_key=17
        -C "bp.mpe.output_attributes_parameter_of_core=${mec_output_attrs}"
        -C "cluster0.output_attributes=${mec_output_attrs}"
        -C "cluster1.output_attributes=${mec_output_attrs}"
        -C "gic_distributor.output_attributes=${mec_output_attrs}"
        -C "elfloader.output_attributes_parameter_of_core=${mec_output_attrs}"
        -C cluster0.rme_mecid_width="${FVP_MECID_WIDTH}"
        -C cluster1.rme_mecid_width="${FVP_MECID_WIDTH}"
    )

    if [[ "${FVP_ENABLE_SMMU_MEC}" == "1" ]]; then
        FVP_ARGS+=(
            # Device-side MECID plumbing is optional for the current
            # FIRME_MEC_REFRESH ABI tests. Some FVP revisions reject these
            # parameters unless the PCI SMMU Realm interface is also enabled.
            # Keep it opt-in for future IDE/MEC memory-correlation scenarios.
            -C pci.pci_smmuv3.mmu.SMMU_R_IDR3=0x18000
            -C pci.pci_smmuv3.mmu.SMMU_R_MECIDR=15
            -C 'pci.pci_smmuv3.mmu.mec_attribute_transform=UserFlags[31:16]=MECID[15:0]'
        )
    fi
fi

echo "[run] Launching FVP (Ctrl-C to stop)."

if [[ "${FVP_ENABLE_TELNET}" == "1" ]]; then
    : > "${MODEL_LOG}"
    : > "${UART_LOG}"
    : > "${UART1_LOG}"
    : > "${UART2_LOG}"
    : > "${UART3_LOG}"
    : > "${FVP_TELNET_MERGED_LOG}"

    # Start each run with clean per-port logs.
    for p in ${FVP_TELNET_PORTS//,/ }; do
        : > "${FVP_TELNET_LOG_DIR}/uart_${p}.log"
    done

    # Ensure the model exposes UARTs over telnet (and doesn't try to spawn xterms).
    IFS=',' read -r -a _ports <<<"${FVP_TELNET_PORTS}"
    for i in 0 1 2 3; do
        if [[ $i -lt ${#_ports[@]} ]]; then
            FVP_ARGS+=(
                -C "bp.terminal_${i}.start_telnet=1"
                -C "bp.terminal_${i}.start_port=${_ports[$i]}"
                -C "bp.terminal_${i}.terminal_command=/bin/true"
            )
        else
            FVP_ARGS+=(
                -C "bp.terminal_${i}.start_telnet=1"
                -C "bp.terminal_${i}.terminal_command=/bin/true"
            )
        fi
    done

    "${MODEL_BASE}" "${FVP_ARGS[@]}" "$@" >"${MODEL_LOG}" 2>&1 &
    fvp_pid="$!"

    cleanup() {
        if kill -0 "${fvp_pid}" >/dev/null 2>&1; then
            kill "${fvp_pid}" >/dev/null 2>&1 || true
        fi
    }
    trap cleanup EXIT INT TERM

    if [[ "${FVP_TIMEOUT_SECS}" != "0" ]]; then
        (
            sleep "${FVP_TIMEOUT_SECS}"
            if kill -0 "${fvp_pid}" >/dev/null 2>&1; then
                echo "[run] Timeout reached; terminating FVP..." >&2
                kill "${fvp_pid}" >/dev/null 2>&1 || true
                sleep 2
                kill -9 "${fvp_pid}" >/dev/null 2>&1 || true
            fi
        ) &
    fi

    echo "[run] Capturing telnet UART ports -> ${FVP_TELNET_LOG_DIR} (and merged ${FVP_TELNET_MERGED_LOG})"
    python3 - "${FVP_TELNET_PORTS}" "${FVP_TELNET_LOG_DIR}" "${FVP_TELNET_MERGED_LOG}" "${fvp_pid}" <<'PY'
import os
import sys
import threading
import time
import telnetlib

ports_csv = sys.argv[1]
out_dir = sys.argv[2]
merged_path = sys.argv[3]
fvp_pid = int(sys.argv[4])

ports = []
for p in ports_csv.split(","):
    p = p.strip()
    if p:
        ports.append(int(p))
if not ports:
    raise SystemExit("No ports provided")

os.makedirs(out_dir, exist_ok=True)
merged = open(merged_path, "ab", buffering=0)
lock = threading.Lock()

def fvp_running() -> bool:
    try:
        os.kill(fvp_pid, 0)
        return True
    except OSError:
        return False

def capture_port(port: int):
    per_path = os.path.join(out_dir, f"uart_{port}.log")
    while fvp_running():
        try:
            tn = telnetlib.Telnet("127.0.0.1", port, timeout=2)
        except OSError:
            time.sleep(0.1)
            continue
        try:
            with open(per_path, "ab", buffering=0) as per:
                while fvp_running():
                    data = tn.read_eager()
                    if not data:
                        time.sleep(0.05)
                        continue
                    per.write(data)
                    with lock:
                        merged.write(data)
                        sys.stdout.buffer.write(data)
                        sys.stdout.buffer.flush()
        except EOFError:
            time.sleep(0.1)
        finally:
            try:
                tn.close()
            except Exception:
                pass

threads = [threading.Thread(target=capture_port, args=(p,), daemon=True) for p in ports]
for t in threads:
    t.start()
for t in threads:
    t.join()
PY

    wait "${fvp_pid}" || true
else
    # Non-telnet mode: rely on UART out_file capture (stdout is model diagnostics).
    : > "${MODEL_LOG}"
    : > "${UART_LOG}"
    : > "${UART1_LOG}"
    : > "${UART2_LOG}"
    : > "${UART3_LOG}"
    if [[ "${FVP_TIMEOUT_SECS}" != "0" ]] && command -v timeout >/dev/null 2>&1; then
        timeout -k 5 "${FVP_TIMEOUT_SECS}" "${MODEL_BASE}" "${FVP_ARGS[@]}" "$@" >"${MODEL_LOG}" 2>&1 || true
    else
        "${MODEL_BASE}" "${FVP_ARGS[@]}" "$@" >"${MODEL_LOG}" 2>&1 || true
    fi
fi
