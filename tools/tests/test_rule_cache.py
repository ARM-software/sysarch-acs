# Copyright (c) 2026, Arm Limited and Contributors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
"""Host regressions: python3 -m unittest discover -s tools/tests -v."""

import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]


class RuleCacheTests(unittest.TestCase):
    def test_orchestrator(self):
        sources = [
            "tools/tests/rule_cache_harness.c",
            "val/src/rule_based_orchestrator.c",
            "val/src/rule_based_execution_helpers.c",
            "val/src/acs_execution_policy.c",
            "apps/baremetal/acs_runtime_init.c",
        ]
        with tempfile.TemporaryDirectory() as output:
            for optimization, enabled in (("-O0", 0), ("-O0", 1), ("-O2", 1)):
                with self.subTest(optimization=optimization, enabled=enabled):
                    binary = Path(output) / (optimization + str(enabled))
                    command = [
                        *shlex.split(os.environ.get("CC", "cc")),
                        "-std=gnu99", optimization, "-Wall", "-Wextra", "-Werror",
                        "-ffunction-sections", "-fdata-sections", "-DTARGET_BAREMETAL",
                        "-DCOMPILE_RB_EXE", "-DSBSA", f"-DACS_RULE_CACHE={enabled}",
                        "-I" + str(ROOT),
                        "-I" + str(ROOT / "apps/baremetal"),
                        "-I" + str(ROOT / "val/include"),
                        "-I" + str(ROOT / "pal/baremetal/base/include"),
                        "-I" + str(ROOT / "pal/baremetal/target/RDN2/include"),
                        "-Wl,--gc-sections",
                        "-Wl,--wrap=print_rule_test_start",
                        "-Wl,--wrap=print_rule_test_status",
                        *[str(ROOT / source) for source in sources],
                        "-o", str(binary),
                    ]
                    subprocess.run(command, check=True)
                    result = subprocess.run(
                        [str(binary)], check=True, stdout=subprocess.PIPE, text=True
                    )
                    self.assertIn("18 rule-cache regressions passed", result.stdout)


if __name__ == "__main__":
    unittest.main()
