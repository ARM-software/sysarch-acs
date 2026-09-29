#!/usr/bin/env python3
#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

"""Exercise GPT build/run profiles without a compiler or licensed model."""

import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


SCRIPTS = Path(__file__).resolve().parent
CONFIG = SCRIPTS / "fvp_gpt_config.sh"
RUNNER = SCRIPTS / "run_fvp_regression.sh"
TFA_SOURCE = SCRIPTS / "fvp_tfa_source.sh"
PROFILES = {"baseline": (0, 0), "gpc2": (1, 0), "gdi": (1, 1)}
MODEL = r'''#!/usr/bin/env python3
import json
import os
from pathlib import Path
import sys

arguments = sys.argv[1:]
Path(os.environ["CAPTURE_ARGS"]).write_text(json.dumps(arguments))
parameters = dict(argument.split("=", 1) for argument in arguments if "=" in argument)
gpc2 = parameters["cluster0.has_rme_gpc2"]
gdi = str(int(parameters["cluster0.has_rme_gdi"]) // 2)
if os.environ.get("CORRUPT_FEATURES"):
    gdi = str(1 - int(gdi))
Path(parameters["bp.pl011_uart0.out_file"]).write_text(
    f"GPT CPU features: RME=1 GPC2={gpc2} GDI={gdi}\n"
    "Total Tests run = 1; Tests Passed = 1; Tests Failed = 0\n"
    "FIRME tests complete.\n")
'''


def environment(overrides):
    """Keep local toolchain/model settings out of deterministic checks."""
    result = {key: os.environ[key] for key in ("PATH", "HOME") if key in os.environ}
    result.update(overrides)
    return result


class GptConfigurationTests(unittest.TestCase):
    """Require consistent feature controls and real runtime advertisement."""

    def resolve(self, overrides):
        """Observe the shared configuration used by both build and run scripts."""
        return subprocess.run(
            ["bash", "-ec", 'source "$1"; printf "%s:%s:%s:%s" '
             '"$FVP_GPT_PROFILE" "$FVP_ENABLE_GPC2" "$FVP_ENABLE_GDI" '
             '"$TFA_ENABLE_RME_GDI"', "gpt-config-check", str(CONFIG)],
            env=environment(overrides), text=True, capture_output=True,
            check=False, timeout=15,
        )

    def test_dut_identity(self):
        """Compare source content for developer rebuilds, independently of checkout path."""
        with tempfile.TemporaryDirectory(prefix="firme-dut-identity-") as directory:
            root = Path(directory)
            (root / ".git").mkdir()
            source = root / "local.c"
            source.write_text("initial fixture\n", encoding="utf-8")
            git = root / "git"
            git.write_text(
                '#!/bin/bash\ncase "$3" in\n'
                'rev-parse) printf "%s\\n" "${FAKE_REV:-original}" ;;\n'
                'diff) printf "%s" "${FAKE_DIFF:-}" ;;\n'
                'ls-files) printf "local.c\\0" ;;\nesac\n', encoding="utf-8")
            git.chmod(0o700)
            env = environment({"PATH": f"{root}:{os.environ['PATH']}",
                               "TFA_DIR": str(root)})
            command = ['bash', '-ec', 'source "$1"; printf "TFA_DIR=%s\\n" "$TFA_DIR"; '
                       'printf "TFA_GIT_REV=%s\\n" "$(tfa_git_rev)"; '
                       'printf "TFA_GIT_STATUS_SHA256=%s\\n" "$(tfa_git_status_sha256)"',
                       'dut-identity', str(TFA_SOURCE)]
            stamp = root / "firmware.stamp"
            result = subprocess.run(command, env=env, capture_output=True, text=True,
                                    check=True, timeout=15)
            stamp.write_text(result.stdout, encoding="utf-8")
            command = ['bash', '-ec', 'source "$1"; tfa_source_stamp_matches "$2"',
                       'dut-identity', str(TFA_SOURCE), str(stamp)]
            for changes in ({}, {"FAKE_REV": "new"}, {"FAKE_DIFF": "changed"},
                            {"TFA_DIR": str(root / "other-checkout")}):
                result = subprocess.run(command, env=env | changes, capture_output=True,
                                        check=False, timeout=15)
                self.assertEqual(result.returncode == 0, not changes)
            source.write_text("modified fixture\n", encoding="utf-8")
            result = subprocess.run(command, env=env, capture_output=True,
                                    check=False, timeout=15)
            self.assertNotEqual(result.returncode, 0)

    def test_suite_defaults(self):
        """Include implemented GPT extensions in all/GPT and preserve smaller suites."""
        for suite in ("all", "gpt", "base", "mec", "ide", "attest"):
            result = self.resolve({"SUITE": suite})
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(result.stdout, "gdi:1:1:1" if suite in ("all", "gpt")
                             else "baseline:0:0:0")

    def test_overrides_and_invalid_settings(self):
        """Allow baseline negative testing and reject inconsistent firmware/model pairs."""
        for profile, (gpc2, gdi) in PROFILES.items():
            for firmware in (str(gdi), "2"):
                result = self.resolve({"SUITE": "all", "FVP_GPT_PROFILE": profile,
                                       "TFA_ENABLE_RME_GDI": firmware})
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout, f"{profile}:{gpc2}:{gdi}:{firmware}")
        for overrides in (
            {"FVP_GPT_PROFILE": "invalid"},
            {"FVP_GPT_PROFILE": "gdi", "TFA_ENABLE_RME_GDI": "0"},
            {"FVP_GPT_PROFILE": "gpc2", "TFA_ENABLE_RME_GDI": "1"},
            {"TFA_ENABLE_RME_GDI": "3"},
            {"FVP_ENABLE_RME": "0"},
            {"FVP_RME_SUPPORT_LEVEL": "1"},
        ):
            self.assertNotEqual(self.resolve(overrides).returncode, 0, overrides)

    def run_model(self, profile, *, stale=False, corrupt=False):
        """Run the real launcher against a UART/argument-only native model fixture."""
        with tempfile.TemporaryDirectory(prefix="firme-gpt-config-") as directory:
            root = Path(directory)
            firmware = root / "firmware/fvp/debug"
            firmware.mkdir(parents=True)
            (firmware / "bl1.bin").write_bytes(b"BL1 fixture")
            (firmware / "fip.bin").write_bytes(b"FIP fixture")
            gpc2, gdi = PROFILES[profile]
            (firmware / ".firme_build_tfa.stamp").write_text(
                "FVP_FIRME_MEC_REFRESH=0\nFVP_FIRME_CXL_IDE=0\n"
                ""
                f"ENABLE_FEAT_RME_GDI={1 - gdi if stale else gdi}\n",
                encoding="utf-8",
            )
            model = root / "model"
            model.write_text(MODEL, encoding="utf-8")
            model.chmod(0o700)
            overrides = {
                "SUITE": "gpt", "FVP_GPT_PROFILE": profile,
                "BUILD_MODE": "never", "BUILD_BASE": str(root / "firmware"),
                "MODEL_BASE": str(model), "CAPTURE_ARGS": str(root / "args.json"),
                "FVP_ENABLE_TELNET": "0", "FVP_TIMEOUT_SECS": "0",
                "FVP_TELNET_LOG_DIR": str(root / "telnet"),
                "MODEL_LOG": str(root / "model.log"),
            }
            for suffix in ("", "1", "2", "3"):
                overrides[f"UART{suffix}_LOG"] = str(root / f"uart{suffix}.log")
            if corrupt:
                overrides["CORRUPT_FEATURES"] = "1"
            result = subprocess.run(["bash", str(RUNNER)], env=environment(overrides),
                                    capture_output=True, text=True, check=False, timeout=15)
            arguments = (json.loads((root / "args.json").read_text())
                         if (root / "args.json").exists() else [])
            return result, arguments

    def test_launcher_and_runtime_features(self):
        """Both clusters receive controls and stale/misreported features fail closed."""
        for profile, (gpc2, gdi) in PROFILES.items():
            result, arguments = self.run_model(profile)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("bp.dram_size=8", arguments)
            self.assertIn("cache_state_modelled=1", arguments)
            for cluster in (0, 1):
                self.assertIn(f"cluster{cluster}.has_rme_gpc2={gpc2}", arguments)
                self.assertIn(f"cluster{cluster}.has_rme_gdi={gdi * 2}", arguments)
            result, arguments = self.run_model(profile, stale=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("rebuild firmware for the requested GPT GDI", result.stderr)
            self.assertEqual(arguments, [])
            result, _ = self.run_model(profile, corrupt=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("runtime GPT feature discovery does not match", result.stderr)


if __name__ == "__main__":
    unittest.main()
