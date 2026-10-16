#!/usr/bin/env bash
#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

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

tfa_source_stamp_matches() {
    local stamp_file="$1"
    local source_hash
    local revision
    [[ -n "${TFA_DIR}" && -e "${TFA_DIR}/.git" ]] || return 1
    [[ -f "${stamp_file}" ]] || return 1
    revision="$(tfa_git_rev)" || return 1
    source_hash="$(tfa_git_status_sha256)" || return 1
    [[ -n "${revision}" && -n "${source_hash}" ]] || return 1
    grep -Fqx "TFA_GIT_REV=${revision}" "${stamp_file}" &&
        grep -Fqx "TFA_GIT_STATUS_SHA256=${source_hash}" "${stamp_file}"
}

firme_run_config() {
    cat <<EOF
SUITE=${SUITE}
FVP_GPT_PROFILE=${FVP_GPT_PROFILE}
TFA_ENABLE_RME_GDI=${TFA_ENABLE_RME_GDI}
FIRME_ENABLE_SVE=$1
FIRME_ENABLE_SME=$2
FVP_ENABLE_IDE=${FVP_ENABLE_IDE}
FVP_IDE_KM_PENDING_OPS_MAX=${FVP_IDE_KM_PENDING_OPS_MAX}
FVP_SIMULATE_IDE_KM_UNIT=${FVP_SIMULATE_IDE_KM_UNIT}
FVP_FIRME_CXL_IDE=${FVP_FIRME_CXL_IDE}
FVP_FIRME_MEC_REFRESH=${FVP_FIRME_MEC_REFRESH}
FIRME_ENABLE_CXL_ENDPOINT_PROBE=${FIRME_ENABLE_CXL_ENDPOINT_PROBE}
FIRME_ENABLE_CXL_SPDM=${FIRME_ENABLE_CXL_SPDM}
FIRME_REALM_PAT_BINDING=${FIRME_REALM_PAT_BINDING:-none}
EOF
}

firme_run_stamp_write() {
    local build_dir="$1"
    local config
    config="$(firme_run_config "$2" "$3")" || return 1
    python3 - "${build_dir}" "${config}" <<'PY'
import hashlib
from pathlib import Path
import sys

build_dir = Path(sys.argv[1])
lines = [sys.argv[2], "FIRME_ARTIFACT_MANIFEST=1"]
for name, key in (("bl1.bin", "BL1_SHA256"), ("fip.bin", "FIP_SHA256"),
                  (".firme_build_tfa.stamp", "TFA_STAMP_SHA256")):
    data = (build_dir / name).read_bytes()
    if not data:
        raise SystemExit(f"ERROR: empty build artifact: {name}")
    lines.append(f"{key}={hashlib.sha256(data).hexdigest()}")
temporary = build_dir / "firme-build.stamp.tmp"
temporary.write_text("\n".join(lines) + "\n", encoding="utf-8")
temporary.replace(build_dir / "firme-build.stamp")
PY
}

firme_run_stamp_matches() {
    local stamp_file="$1"
    local config
    config="$(firme_run_config "$2" "$3")" || return 1
    python3 - "${stamp_file}" "${config}" <<'PY'
import hashlib
from pathlib import Path
import sys

stamp = Path(sys.argv[1])
try:
    actual = {}
    for line in stamp.read_text(encoding="utf-8").splitlines():
        key, separator, value = line.partition("=")
        if not separator or not key or key in actual:
            raise ValueError("malformed or duplicate entry")
        actual[key] = value
    expected = dict(line.split("=", 1) for line in sys.argv[2].splitlines())
    for key, value in expected.items():
        if actual.get(key) != value:
            raise ValueError(f"{key}: built={actual.get(key, 'missing')}, requested={value}")
    if "FIRME_ARTIFACT_MANIFEST" in actual:
        if actual["FIRME_ARTIFACT_MANIFEST"] != "1":
            raise ValueError("unsupported artifact manifest version")
        for name, key in (("bl1.bin", "BL1_SHA256"), ("fip.bin", "FIP_SHA256"),
                          (".firme_build_tfa.stamp", "TFA_STAMP_SHA256")):
            data = (stamp.parent / name).read_bytes()
            if not data or hashlib.sha256(data).hexdigest() != actual.get(key):
                raise ValueError(f"{name}: artifact does not match the build manifest")
except (OSError, UnicodeError, ValueError) as error:
    raise SystemExit(f"ERROR: invalid FIRME build manifest: {error}")
PY
}
