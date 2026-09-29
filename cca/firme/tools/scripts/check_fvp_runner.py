#!/usr/bin/env python3
#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

"""Check launcher completion independently of test verdicts using a fake FVP."""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


RUNNER = Path(__file__).resolve().with_name("run_fvp_regression.sh")
SUMMARY = "Total Tests run = 3; Tests Passed = 1; Tests Failed = 2\n"
COMPLETE = "FIRME tests complete.\n"
MODEL = r'''#!/usr/bin/env python3
import os
from pathlib import Path
import resource
import signal
import sys
import time

resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
shutdown = os.environ.get("FAKE_SHUTDOWN")
if shutdown == "abort":
    signal.signal(signal.SIGTERM, lambda *_: os.abort())
elif shutdown == "error":
    signal.signal(signal.SIGTERM, lambda *_: sys.exit(1))
elif shutdown == "clean":
    signal.signal(signal.SIGTERM, lambda *_: sys.exit(0))
parameters = dict(argument.split("=", 1) for argument in sys.argv[1:] if "=" in argument)
if parameters.get("bp.ve_sysregs.exit_on_shutdown") != "1":
    raise SystemExit("PSCI system-off must terminate the model")
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
            (firmware / "bl1.bin").write_bytes(b"BL1 fixture")
            (firmware / "fip.bin").write_bytes(b"FIP fixture")
            (firmware / ".firme_build_tfa.stamp").write_text(
                "FVP_FIRME_MEC_REFRESH=0\nENABLE_FEAT_RME_GDI=0\n"
                "FVP_FIRME_CXL_IDE=0\n", encoding="utf-8")
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

    def test_quarantined_execution(self):
        """A GPT safety abort is incomplete even when a summary is printed."""
        uart = ("GPT state unconfirmed; stopping before memory reuse\n" +
                SUMMARY + COMPLETE)
        for telnet in ("0", "1"):
            with self.subTest(telnet=telnet):
                result, log = self.run_model(uart, telnet=telnet)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("did not complete FIRME test execution", result.stderr)
                self.assertTrue(log.endswith(uart), log)

    def test_shutdown_failure(self):
        """A timeout must not replace shutdown crashes with an acceptable timeout status."""
        for telnet in ("0", "1"):
            for shutdown in ("abort", "error", "clean"):
                with self.subTest(telnet=telnet, shutdown=shutdown):
                    result, _ = self.run_model(
                        SUMMARY + COMPLETE, telnet=telnet, limit="1",
                        overrides={"FAKE_DELAY": "10", "FAKE_SHUTDOWN": shutdown})
                    self.assertEqual(result.returncode == 0, shutdown == "clean", result.stderr)
                    if shutdown != "clean":
                        self.assertIn("FVP exited abnormally", result.stderr)

    def test_payload_shutdown_failure(self):
        """Post-summary cleanup or an unexpected SYSTEM_OFF return is not completion."""
        for failure in ("FIRME platform shutdown returned unexpectedly",
                        "CXL Type-2 endpoint DOE cleanup failed"):
            for telnet in ("0", "1"):
                with self.subTest(failure=failure, telnet=telnet):
                    result, _ = self.run_model(SUMMARY + COMPLETE + failure + "\n",
                                               telnet=telnet)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn("did not complete FIRME test execution", result.stderr)

    def test_completed_with_blocked_gpt(self):
        """Completed accounting retains a cleanup failure and explicit blocked SKIPs."""
        uart = ("R0127: GPT state unconfirmed; quarantining GPT scratch\n"
                "Blocked: GPT scratch quarantined after R0127; body not executed\n"
                "Total Tests run = 3; Tests Passed = 1; Tests Failed = 1\n"
                "Tests Skipped = 1\n" + COMPLETE)
        for telnet in ("0", "1"):
            with self.subTest(telnet=telnet):
                result, log = self.run_model(uart, telnet=telnet)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertTrue(log.endswith(uart), log)

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


class ArtifactReuseTests(unittest.TestCase):
    """Keep build-stage artifacts authoritative across isolated CI workspaces."""

    def setUp(self):
        """Use real launch/build scripts with fake build tools and FVP in a private tree."""
        temporary = tempfile.TemporaryDirectory(prefix="firme-artifact-reuse-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.scripts = self.root / "acs/tools/scripts"
        self.scripts.mkdir(parents=True)
        for name in ("run_fvp_regression.sh", "build_tfa_fvp.sh", "fvp_gpt_config.sh",
                     "fvp_tfa_source.sh"):
            shutil.copyfile(RUNNER.with_name(name), self.scripts / name)
        config = self.root / "acs/tools/config"
        config.mkdir()
        (config / "sp_layout.json").write_text("{}", encoding="utf-8")
        (config / "fvp_spmc_manifest.dts").write_text("fixture", encoding="utf-8")
        toolchain = self.root / "toolchain.cmake"
        toolchain.touch()
        self.firmware = self.root / "firmware/fvp/debug"
        self.firmware.mkdir(parents=True)
        self.source = self.root / "build-checkout"
        (self.source / ".git").mkdir(parents=True)
        (self.source / "local.c").write_text("original source", encoding="utf-8")
        self.bin = self.root / "bin"
        self.bin.mkdir()
        for tool in ("fixture-gcc", "fixture-ld", "fixture-objcopy"):
            self.write_tool(self.bin / tool, "#!/bin/bash\nexit 0\n")
        self.write_tool(self.bin / "git", '#!/bin/bash\n'
                        'if [[ "${FORBID_GIT:-0}" == 1 ]]; then exit 99; fi\n'
                        'case "$3" in\n'
                        'rev-parse) printf "%s\\n" "${FAKE_REV:-build-revision}" ;;\n'
                        'diff) printf "%s" "${FAKE_DIFF:-}" ;;\n'
                        'ls-files) printf "local.c\\0" ;;\nesac\n')
        self.write_tool(self.bin / "cmake", '#!/bin/bash\nset -eu\n'
                        'echo cmake >> "$BUILD_CAPTURE"\n'
                        'mkdir -p "$FIRME_ARTIFACT_DIR"\n'
                        'for image in secure nsel2 rel2; do\n'
                        '    echo "$image" > "$FIRME_ARTIFACT_DIR/$image.bin"\n'
                        'done\n')
        self.write_tool(self.bin / "make", '#!/bin/bash\nset -eu\n'
                        'echo make >> "$BUILD_CAPTURE"\n'
                        'for image in bl1 bl2 bl31; do\n'
                        '    echo "$image" > "$BUILD_BASE/fvp/debug/$image.bin"\n'
                        'done\n')
        fiptool = self.firmware / "tools/fiptool/fiptool"
        fiptool.parent.mkdir(parents=True)
        self.write_tool(fiptool, '#!/bin/bash\nset -eu\n'
                        'if [[ "$1" == create ]]; then echo fip > "${!#}"; fi\n')
        self.write_tool(self.root / "model", MODEL)
        self.environment = {key: os.environ[key] for key in ("PATH", "HOME")
                            if key in os.environ}
        self.environment.update({
            "PATH": f"{self.bin}:{os.environ['PATH']}", "TFA_DIR": str(self.source),
            "CROSS_COMPILE": "fixture-", "FIRME_TOOLCHAIN_FILE": str(toolchain),
            "FIRME_ARTIFACT_DIR": str(self.root / "payloads"),
            "FIRME_BUILD_DIR": str(self.root / "cmake"), "SUITE": "base",
            "BUILD_BASE": str(self.root / "firmware"),
            "BUILD_CAPTURE": str(self.root / "build-calls"),
            "MODEL_BASE": str(self.root / "model"), "MODEL_LOG": str(self.root / "model.log"),
            "FVP_ENABLE_TELNET": "0", "FVP_TIMEOUT_SECS": "0", "FAKE_UART": SUMMARY + COMPLETE,
        })
        for suffix in ("", "1", "2", "3"):
            self.environment[f"UART{suffix}_LOG"] = str(self.root / f"uart{suffix}.log")
        self.build()
        (self.root / "build-calls").unlink()

    def write_tool(self, path, content):
        """Install a deterministic executable fixture without modifying host tools."""
        path.write_text(content, encoding="utf-8")
        path.chmod(0o700)

    def build(self):
        """Produce artifacts and manifests through the standalone build entry point."""
        result = subprocess.run(["bash", str(self.scripts / "build_tfa_fvp.sh")],
                                env=self.environment, capture_output=True, text=True,
                                check=False, timeout=15)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertTrue((self.firmware / "firme-build.stamp").is_file())

    def launch(self, overrides=None):
        """Launch or explicitly rebuild using only the temporary checkout and artifacts."""
        return subprocess.run(["bash", str(self.scripts / "run_fvp_regression.sh")],
                              env=self.environment | (overrides or {}), capture_output=True,
                              text=True, check=False, timeout=15)

    def test_ci_artifacts_are_authoritative(self):
        """Default/never reuse copied artifacts despite checkout path, revision or mtime."""
        copied = self.root / "run-artifacts"
        shutil.copytree(self.root / "firmware", copied, copy_function=shutil.copyfile)
        other_source = self.root / "run-checkout"
        shutil.copytree(self.source, other_source)
        (other_source / "local.c").write_text("different source", encoding="utf-8")
        for image in ("bl1.bin", "fip.bin"):
            os.utime(copied / "fvp/debug" / image, (1, 1))
        for mode in ("auto", "never"):
            for source in ("", str(self.root / "missing"), str(other_source)):
                with self.subTest(mode=mode, source=source):
                    result = self.launch({"BUILD_MODE": mode, "BUILD_BASE": str(copied),
                                          "TFA_DIR": source, "FAKE_REV": "run-revision",
                                          "FORBID_GIT": "1"})
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    self.assertIn("TFA_GIT_REV=build-revision", result.stdout)
                    self.assertNotIn("TFA_GIT_REV=run-revision", result.stdout)
                    self.assertFalse((self.root / "build-calls").exists())

    def test_legacy_ci_artifacts(self):
        """Existing CI packages without a payload manifest remain runnable."""
        (self.firmware / "firme-build.stamp").unlink()
        for mode in ("auto", "never"):
            result = self.launch({"BUILD_MODE": mode, "TFA_DIR": "/missing-checkout"})
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("Legacy artifacts", result.stdout)
            self.assertFalse((self.root / "build-calls").exists())

    def test_experimental_artifact_reuse(self):
        """Opt-in backends also reuse their exact artifacts without a run-stage rebuild."""
        platform = self.source / "plat/arm/board/fvp/platform.mk"
        platform.parent.mkdir(parents=True)
        platform.write_text("FVP_FIRME_CXL_IDE := 0\nFVP_FIRME_MEC_REFRESH := 0\n",
                            encoding="utf-8")
        hierarchy = self.root / "hierarchy.json"
        hierarchy.write_text('{"fixture": true}', encoding="utf-8")
        for backend in ("FVP_FIRME_CXL_IDE", "FVP_FIRME_MEC_REFRESH"):
            self.environment.update({"FVP_FIRME_CXL_IDE": "0", "FVP_FIRME_MEC_REFRESH": "0",
                                     "FVP_ENABLE_IDE": "1", "FVP_ENABLE_MEC": "1",
                                     "FVP_CACHE_STATE_MODELLED": "1",
                                     "FVP_PCIE_HIERARCHY": str(hierarchy), backend: "1"})
            self.build()
            (self.root / "build-calls").unlink()
            result = self.launch({"BUILD_MODE": "auto", "TFA_DIR": "/missing-checkout"})
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertFalse((self.root / "build-calls").exists())

    def test_legacy_profile_guards(self):
        """Legacy packages retain CXL/MEC/GDI checks even without a source checkout."""
        (self.firmware / "firme-build.stamp").unlink()
        stamp = self.firmware / ".firme_build_tfa.stamp"
        original = stamp.read_text(encoding="utf-8")
        for key in ("FVP_FIRME_CXL_IDE", "FVP_FIRME_MEC_REFRESH", "ENABLE_FEAT_RME_GDI"):
            for value in ("1", "invalid"):
                stamp.write_text(original.replace(f"{key}=0\n", f"{key}={value}\n"),
                                 encoding="utf-8")
                result = self.launch({"TFA_DIR": "/missing-checkout"})
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse((self.root / "build-calls").exists())
                self.assertFalse((self.root / "uart.log").exists())
        stamp.unlink()
        result = self.launch()
        self.assertNotEqual(result.returncode, 0)

    def test_legacy_runner_manifest(self):
        """Older runner-produced profile stamps remain valid without new hashes."""
        manifest = self.firmware / "firme-build.stamp"
        lines = manifest.read_text(encoding="utf-8").splitlines()
        manifest.write_text("\n".join(line for line in lines
                                      if not line.startswith("FIRME_ARTIFACT_MANIFEST=")
                                      and "_SHA256=" not in line) + "\n", encoding="utf-8")
        result = self.launch({"TFA_DIR": "/missing-checkout"})
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertFalse((self.root / "build-calls").exists())

    def test_new_manifest_guards(self):
        """Reject corruption, missing provenance, duplicate keys and profile mismatch."""
        manifest = self.firmware / "firme-build.stamp"
        original = manifest.read_text(encoding="utf-8")
        for invalid in (original + "SUITE=base\n", original + "invalid\n",
                        original.replace("FIRME_ARTIFACT_MANIFEST=1", "FIRME_ARTIFACT_MANIFEST=2"),
                        original.replace("BL1_SHA256=", "MISSING_HASH="),
                        original.replace("SUITE=base", "SUITE=ide")):
            manifest.write_text(invalid, encoding="utf-8")
            result = self.launch()
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("invalid FIRME build manifest", result.stderr)
            self.assertFalse((self.root / "build-calls").exists())
        manifest.write_text(original, encoding="utf-8")
        for name in ("bl1.bin", "fip.bin", ".firme_build_tfa.stamp"):
            artifact = self.firmware / name
            original_bytes = artifact.read_bytes()
            artifact.write_bytes(original_bytes + b"corruption\n")
            result = self.launch()
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("artifact does not match", result.stderr)
            artifact.write_bytes(original_bytes)
        self.assertFalse((self.root / "build-calls").exists())

    def test_explicit_source_mode(self):
        """Developer source mode rebuilds for revision/diff changes, not checkout location."""
        copied = self.root / "identical-source"
        shutil.copytree(self.source, copied)
        future = 4102444800
        os.utime(self.firmware / "fip.bin", (future, future))
        result = self.launch({"BUILD_MODE": "source", "TFA_DIR": str(copied)})
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertFalse((self.root / "build-calls").exists())
        for changes in ({"FAKE_REV": "new-revision"}, {"FAKE_DIFF": "edited source"}):
            with self.subTest(changes=changes):
                result = self.launch({"BUILD_MODE": "source"} | changes)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertTrue((self.root / "build-calls").is_file())
                (self.root / "build-calls").unlink()
        (self.source / "local.c").write_text("untracked source edited", encoding="utf-8")
        result = self.launch({"BUILD_MODE": "source"})
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertTrue((self.root / "build-calls").is_file())
        (self.root / "build-calls").unlink()
        os.utime(self.scripts / "run_fvp_regression.sh", (future, future))
        result = self.launch({"BUILD_MODE": "source"})
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertTrue((self.root / "build-calls").is_file())

    def test_explicit_rebuild_and_missing_artifacts(self):
        """Always rebuilds; auto builds missing images; source mode requires a checkout."""
        for mode in ("always", "auto"):
            if mode == "auto":
                (self.firmware / "fip.bin").unlink()
            result = self.launch({"BUILD_MODE": mode})
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertTrue((self.root / "build-calls").is_file())
            (self.root / "build-calls").unlink()
        result = self.launch({"BUILD_MODE": "source", "TFA_DIR": "/missing-checkout"})
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("requires an available TFA_DIR", result.stderr)
        self.assertFalse((self.root / "build-calls").exists())
        (self.firmware / "fip.bin").write_bytes(b"")
        result = self.launch()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("FIP not found or empty", result.stderr)
        self.assertFalse((self.root / "build-calls").exists())


if __name__ == "__main__":
    unittest.main()
