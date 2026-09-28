#!/usr/bin/env python3
#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

"""Check launcher completion independently of test verdicts using a fake FVP."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


RUNNER = Path(__file__).resolve().with_name("run_fvp_regression.sh")
SUMMARY = "Total Tests run = 3; Tests Passed = 1; Tests Failed = 2\n"
COMPLETE = "FIRME tests complete.\n"
MODEL = r'''#!/usr/bin/env python3
import os
from pathlib import Path
import signal
import sys
import time

parameters = dict(argument.split("=", 1) for argument in sys.argv[1:] if "=" in argument)
Path(parameters["bp.pl011_uart0.out_file"]).write_text(
    "GPT CPU features: RME=1 GPC2=0 GDI=0\n" + os.environ["FAKE_UART"],
    encoding="utf-8")
if os.environ.get("FAKE_CRASH"):
    os.kill(os.getpid(), signal.SIGTERM)
time.sleep(float(os.environ.get("FAKE_DELAY", "0")))
sys.exit(int(os.environ.get("FAKE_EXIT", "0")))
'''


class FvpRunnerTests(unittest.TestCase):
    """Keep CI result policy separate from launcher and completion errors."""

    def run_model(self, uart, *, telnet="0", limit="0", overrides=None):
        """Exercise the real runner without a compiler, license or firmware."""
        with tempfile.TemporaryDirectory(prefix="firme-runner-") as directory:
            root = Path(directory)
            firmware = root / "firmware/fvp/debug"
            firmware.mkdir(parents=True)
            (firmware / "bl1.bin").touch()
            (firmware / "fip.bin").touch()
            (firmware / ".firme_build_tfa.stamp").write_text(
                "FVP_FIRME_MEC_REFRESH=0\nENABLE_FEAT_RME_GDI=0\n", encoding="utf-8")
            model = root / "model"
            model.write_text(MODEL, encoding="utf-8")
            model.chmod(0o700)
            environment = {key: os.environ[key] for key in ("PATH", "HOME")
                           if key in os.environ}
            environment.update({
                "SUITE": "base", "BUILD_MODE": "never",
                "BUILD_BASE": str(root / "firmware"), "MODEL_BASE": str(model),
                "MODEL_LOG": str(root / "model.log"), "FAKE_UART": uart,
                "FVP_ENABLE_TELNET": telnet, "FVP_TIMEOUT_SECS": limit,
                "FVP_TELNET_PORTS": "0", "FVP_TELNET_LOG_DIR": str(root / "telnet"),
            })
            for suffix in ("", "1", "2", "3"):
                environment[f"UART{suffix}_LOG"] = str(root / f"uart{suffix}.log")
            environment.update(overrides or {})
            result = subprocess.run(["bash", str(RUNNER)], env=environment,
                                    capture_output=True, text=True, check=False, timeout=15)
            log = root / "uart.log"
            return result, log.read_text(encoding="utf-8") if log.exists() else ""

    def test_completed_verdicts(self):
        """PASS, FAIL and SKIP results reach CI unchanged in both capture modes."""
        for telnet in ("0", "1"):
            for passed, failed in ((3, 0), (1, 2), (0, 3), (0, 0)):
                uart = (f"Total Tests run = 3; Tests Passed = {passed}; "
                        f"Tests Failed = {failed}\n"
                        f"Tests Skipped = {3 - passed - failed}\n" + COMPLETE)
                with self.subTest(telnet=telnet, passed=passed, failed=failed):
                    result, log = self.run_model(uart, telnet=telnet)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertTrue(log.endswith(uart), log)

    def test_incomplete_execution(self):
        """Reject absent, truncated, empty-suite and malformed result logs."""
        for uart in ("", SUMMARY, COMPLETE, SUMMARY.replace("= 3;", "= 0;") + COMPLETE,
                     SUMMARY.replace("Failed = 2", "Failed = invalid") + COMPLETE):
            result, _ = self.run_model(uart)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("did not complete FIRME test execution", result.stderr)

    def test_timeout(self):
        """A model left idle after completion is safe to stop, not an early hang."""
        for telnet in ("0", "1"):
            for complete in (False, True):
                with self.subTest(telnet=telnet, complete=complete):
                    result, _ = self.run_model(
                        SUMMARY + (COMPLETE if complete else ""), telnet=telnet,
                        limit="1", overrides={"FAKE_DELAY": "10"})
                    self.assertEqual(result.returncode == 0, complete, result.stderr)

    def test_model_errors(self):
        """Do not hide startup errors or crashes behind a completed UART log."""
        for telnet in ("0", "1"):
            for limit in ("0", "1"):
                for overrides in ({"FAKE_EXIT": "1"}, {"FAKE_CRASH": "1"},
                                  {"MODEL_BASE": "/nonexistent/firme-model"}):
                    with self.subTest(telnet=telnet, limit=limit, overrides=overrides):
                        result, _ = self.run_model(SUMMARY + COMPLETE, telnet=telnet,
                                                   limit=limit, overrides=overrides)
                        self.assertNotEqual(result.returncode, 0)
                        self.assertIn("FVP exited abnormally", result.stderr)

    def test_build_precondition(self):
        """A missing build prerequisite must stop execution before launch."""
        result, log = self.run_model(SUMMARY + COMPLETE,
                                     overrides={"BUILD_MODE": "always", "TFA_DIR": ""})
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("TFA_DIR is not set", result.stderr)
        self.assertEqual(log, "")


if __name__ == "__main__":
    unittest.main()
