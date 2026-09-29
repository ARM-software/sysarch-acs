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
import tempfile


CAPTURE_EXIT_STATUS = 97
CAPTURE_PREFIX = "FIRME_BUILD_CONFIG="
BUILD_SCRIPT = Path(__file__).resolve().with_name("build_tfa_fvp.sh")
RUN_SCRIPT = BUILD_SCRIPT.with_name("run_fvp_regression.sh")
CAPTURE_COMMAND = r'''
firme_check_gcc() { return 1; }
firme_check_ld() { return 1; }
firme_check_objcopy() { return 1; }
cmake() {
    printf 'FIRME_BUILD_CONFIG=%s|%s|%s|%s|%s\n' \
        "$SUITE" "$FVP_ENABLE_IDE" "$FVP_IDE_KM_PENDING_OPS_MAX" \
        "$FVP_SIMULATE_IDE_KM_UNIT" "$FVP_FIRME_MEC_REFRESH"
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


def check_refresh_config():
    """Reject incompatible opt-in firmware/model settings before execution."""
    build_cases = [
        ({"FVP_FIRME_MEC_REFRESH": "invalid"}, "must be 0 or 1"),
        ({"FVP_FIRME_MEC_REFRESH": "1", "PLAT": "other"},
         "requires FIRME_SUPPORT=1 and PLAT=fvp"),
        ({"FVP_FIRME_MEC_REFRESH": "1", "FIRME_SUPPORT": "0"},
         "requires FIRME_SUPPORT=1 and PLAT=fvp"),
        ({"FVP_FIRME_MEC_REFRESH": "1", "TFA_DIR": "/nonexistent"},
         "no experimental MEC refresh backend"),
    ]
    for overrides, expected in build_cases:
        result, captures = capture_config(overrides)
        if result.returncode == 0 or captures or expected not in result.stderr:
            raise SystemExit(f"FAIL: build {overrides}: {result.stderr}")

    with tempfile.TemporaryDirectory(prefix="firme-build-config-") as root:
        platform = Path(root) / "plat/arm/board/fvp/platform.mk"
        platform.parent.mkdir(parents=True)
        platform.write_text("FVP_FIRME_MEC_REFRESH := 0\n", encoding="utf-8")
        result, captures = capture_config({
            "FVP_FIRME_MEC_REFRESH": "1", "TFA_DIR": root,
        })
        if (result.returncode != CAPTURE_EXIT_STATUS or
                captures != ["all|1|1|1|1"]):
            raise SystemExit(f"FAIL: opt-in build: {result.stderr}")

        firmware_stamp = Path(root) / "fvp/debug/.firme_build_tfa.stamp"
        firmware_stamp.parent.mkdir(parents=True)
        firmware_stamp.write_text("FVP_FIRME_CXL_IDE=0\n",
                                  encoding="utf-8")
        run_cases = [
            ({"FVP_FIRME_MEC_REFRESH": "invalid"}, "must be 0 or 1"),
            ({"FVP_FIRME_MEC_REFRESH": "1", "FVP_ENABLE_MEC": "0"},
             "requires FVP MEC/RME and modeled caches"),
            ({"FVP_FIRME_MEC_REFRESH": "1", "FVP_ENABLE_RME": "0"},
             "requires FVP MEC/RME and modeled caches"),
            ({"FVP_FIRME_MEC_REFRESH": "1", "FVP_CACHE_STATE_MODELLED": "0"},
             "requires FVP MEC/RME and modeled caches"),
            ({"FVP_FIRME_MEC_REFRESH": "1", "PLAT": "other"},
             "requires FVP MEC/RME and modeled caches"),
            ({"FVP_FIRME_MEC_REFRESH": "1"},
             "rebuild firmware for the requested MEC refresh"),
            ({"FVP_FIRME_MEC_REFRESH": "0"},
             "rebuild firmware for the requested MEC refresh"),
        ]
        for overrides, expected in run_cases:
            environment = {key: os.environ[key] for key in ("PATH", "HOME")
                           if key in os.environ}
            environment.update({"MODEL_BASE": "/nonexistent", "BUILD_MODE": "never",
                                "BUILD_BASE": root})
            environment.update(overrides)
            result = subprocess.run(["bash", str(RUN_SCRIPT)], env=environment,
                                    text=True, capture_output=True, check=False,
                                    timeout=15)
            if result.returncode == 0 or expected not in result.stderr:
                raise SystemExit(f"FAIL: run {overrides}: {result.stderr}")

        for built in ("0", "1"):
            firmware_stamp.write_text(f"FVP_FIRME_MEC_REFRESH={built}\n"
                                      "FVP_FIRME_CXL_IDE=0\n"
                                      "ENABLE_FEAT_RME_GDI=1\n"
                                      "",
                                      encoding="utf-8")
            for requested in ("0", "1"):
                environment.update({"PLAT": "fvp", "FVP_ENABLE_MEC": "1",
                                    "FVP_ENABLE_RME": "1",
                                    "FVP_CACHE_STATE_MODELLED": "1",
                                    "FVP_FIRME_MEC_REFRESH": requested})
                result = subprocess.run(["bash", str(RUN_SCRIPT)], env=environment,
                                        text=True, capture_output=True, check=False,
                                        timeout=15)
                expected = ("BL1 not found" if built == requested else
                            "rebuild firmware for the requested MEC refresh")
                if result.returncode == 0 or expected not in result.stderr:
                    raise SystemExit(
                        f"FAIL: built={built}, requested={requested}: {result.stderr}"
                    )


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
        if (result.returncode != CAPTURE_EXIT_STATUS or
                captures != [expected + "|0"]):
            raise SystemExit(
                f"FAIL: {overrides}: expected {expected}, got {captures}\n"
                f"{result.stdout}{result.stderr}"
            )

    result, captures = capture_config({"SUITE": "invalid"})
    if (result.returncode == 0 or captures or
            "unknown SUITE" not in result.stderr):
        raise SystemExit("FAIL: invalid SUITE was not rejected before CMake")
    check_refresh_config()
    print("PASS: suite defaults, IDE overrides and MEC refresh configuration")


if __name__ == "__main__":
    main()
