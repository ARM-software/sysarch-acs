#!/usr/bin/env python3
#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

"""Check generator consistency, not DUT architectural compliance."""

import json
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


FIRME_ROOT = Path(__file__).resolve().parents[2]
GENERATOR = Path("tools/scripts/generate_firme_abi_catalog.py")
CATALOG = Path("tools/config/abi/firme_1_0_alp4.json")
IDE_ABIS = {
    "IDE_KEYSET_PROG", "IDE_KEYSET_GO", "IDE_KEYSET_STOP", "IDE_KEYSET_POLL",
}


class CatalogGenerationTests(unittest.TestCase):
    """Exercise the generator using only artifacts in this source revision."""

    def test_checked_in_artifacts(self):
        """Every default generator output must exist and be current."""
        subprocess.run(
            [sys.executable, str(FIRME_ROOT / GENERATOR), "--check"],
            check=True,
        )

    def test_clean_regeneration(self):
        """Default regeneration must reproduce this revision's artifacts."""
        with tempfile.TemporaryDirectory(prefix="firme-abi-generator-") as temp:
            root = Path(temp)
            (root / GENERATOR).parent.mkdir(parents=True)
            shutil.copy2(FIRME_ROOT / GENERATOR, root / GENERATOR)
            shutil.copytree(
                FIRME_ROOT / "tools/config/abi", root / "tools/config/abi",
            )
            inputs = {
                path.relative_to(root) for path in root.rglob("*")
                if path.is_file()
            }
            command = [sys.executable, str(root / GENERATOR)]
            subprocess.run(command, check=True)
            outputs = {
                path.relative_to(root) for path in root.rglob("*")
                if path.is_file()
            } - inputs
            self.assertTrue(outputs)
            for relative in sorted(outputs):
                with self.subTest(artifact=str(relative)):
                    original = FIRME_ROOT / relative
                    self.assertTrue(original.is_file(), str(relative))
                    self.assertEqual(
                        original.read_bytes(), (root / relative).read_bytes(),
                    )
            subprocess.run(command + ["--check"], check=True)

    def test_ide_keyset_reserved_bits(self):
        """ALP4 R0205/R0013 leave only x3 bits [37:0] usable as input."""
        catalog = json.loads((FIRME_ROOT / CATALOG).read_text(encoding="utf-8"))
        descriptors = {
            abi["id"]: abi for abi in catalog["abis"]
            if abi["id"] in IDE_ABIS
        }
        self.assertEqual(set(descriptors), IDE_ABIS)
        for identifier, abi in descriptors.items():
            with self.subTest(abi=identifier):
                mask = int(abi["input_masks"]["x3"], 0)
                self.assertEqual(mask, (1 << 38) - 1)
                for bit in range(64):
                    self.assertEqual(bool(mask & (1 << bit)), bit < 38)


if __name__ == "__main__":
    unittest.main()
