#!/usr/bin/env python3
#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

"""Check standalone firmware-build defaults without a compiler or model."""

import os
from pathlib import Path
import subprocess


CAPTURE_EXIT_STATUS = 97
CAPTURE_PREFIX = "FIRME_BUILD_CONFIG="
BUILD_SCRIPT = Path(__file__).resolve().with_name("build_tfa_fvp.sh")
CAPTURE_COMMAND = r'''
firme_check_gcc() { return 1; }
firme_check_ld() { return 1; }
firme_check_objcopy() { return 1; }
cmake() {
    printf 'FIRME_BUILD_CONFIG=%s|%s|%s|%s\n' \
        "$SUITE" "$FVP_ENABLE_IDE" "$FVP_IDE_KM_PENDING_OPS_MAX" \
        "$FVP_SIMULATE_IDE_KM_UNIT"
    return 97
}
source "$1"
'''


def capture_config(overrides):
    """Stop at CMake after observing the real script's resolved options."""
    environment = {key: os.environ[key] for key in ("PATH", "HOME")
                   if key in os.environ}
    environment["CROSS_COMPILE"] = "firme_check_"
    environment.update(overrides)
    result = subprocess.run(
        ["bash", "-c", CAPTURE_COMMAND, "firme-build-check", str(BUILD_SCRIPT)],
        env=environment, text=True, capture_output=True, check=False,
        timeout=15,
    )
    captures = [line[len(CAPTURE_PREFIX):]
                for line in result.stdout.splitlines()
                if line.startswith(CAPTURE_PREFIX)]
    return result, captures


def main():
    """Check suite defaults, explicit overrides and invalid-suite rejection."""
    cases = [
        ({}, "all|1|1|1"),
        ({"SUITE": "all"}, "all|1|1|1"),
        ({"SUITE": "ide"}, "ide|1|1|1"),
        ({"SUITE": "IDE"}, "ide|1|1|1"),
        ({"SUITE": "base"}, "base|0|0|0"),
        ({"SUITE": "gpt"}, "gpt|0|0|0"),
        ({"SUITE": "mec"}, "mec|0|0|0"),
        ({"SUITE": "attest"}, "attest|0|0|0"),
        ({"SUITE": "all", "FVP_ENABLE_IDE": "0"}, "all|0|0|0"),
        ({"SUITE": "base", "FVP_ENABLE_IDE": "1"}, "base|1|1|1"),
        ({"SUITE": "ide", "FVP_IDE_KM_PENDING_OPS_MAX": "4",
          "FVP_SIMULATE_IDE_KM_UNIT": "0"}, "ide|1|4|0"),
    ]
    for overrides, expected in cases:
        result, captures = capture_config(overrides)
        if result.returncode != CAPTURE_EXIT_STATUS or captures != [expected]:
            raise SystemExit(
                f"FAIL: {overrides}: expected {expected}, got {captures}\n"
                f"{result.stdout}{result.stderr}"
            )

    result, captures = capture_config({"SUITE": "invalid"})
    if (result.returncode == 0 or captures or
            "unknown SUITE" not in result.stderr):
        raise SystemExit("FAIL: invalid SUITE was not rejected before CMake")
    print("PASS: standalone build suite defaults and explicit IDE overrides")


if __name__ == "__main__":
    main()
