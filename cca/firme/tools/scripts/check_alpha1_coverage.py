#!/usr/bin/env python3
#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

"""Exercise the Alpha1 release guard with complete and malformed UART logs."""

import contextlib
import io
from pathlib import Path
import tempfile
import unittest

from audit_alpha1_coverage import AuditError, audit_runtime


class RuntimeAuditTests(unittest.TestCase):
    """Keep runtime parsing separate from whether the release is complete."""

    def evaluate(self, status="PASS", skipped=None, gated=None, *, extra="",
                 complete=True, passed=None, pending_exclusions=None):
        """Run the production auditor on one synthetic rule result."""
        passed_count = int(status == "PASS") if passed is None else passed
        failed_count = int(status == "FAIL")
        content = (
            f"Rule: R0001\nResult: {status}\n{extra}"
            f"Total Tests run = 1; Tests Passed = {passed_count}; "
            f"Tests Failed = {failed_count}\n"
        )
        if skipped is not None:
            content += f"Tests Skipped = {skipped}\n"
        if complete:
            content += "********* FIRME tests complete. Reset the system *********\n"
        with tempfile.TemporaryDirectory(prefix="firme-runtime-audit-") as work:
            path = Path(work) / "uart.log"
            path.write_text(content, encoding="utf-8")
            with contextlib.redirect_stdout(io.StringIO()):
                return audit_runtime(
                    path, {"R0001"}, gated or set(), pending_exclusions
                )

    def test_zero_skips_implicit(self):
        """Accept the dispatcher's normal successful zero-SKIP summary."""
        self.assertTrue(self.evaluate())

    def test_zero_skips_explicit(self):
        """Also accept logs that explicitly report zero skips."""
        self.assertTrue(self.evaluate(skipped=0))

    def test_gated_rules_block_release(self):
        """Passing active tests cannot hide a registry-gated rule."""
        self.assertFalse(self.evaluate(gated={"R0002"}))

    def test_pending_exclusions_block_release(self):
        """A passing DUT subset cannot grant an architecture waiver."""
        self.assertFalse(self.evaluate(pending_exclusions={"R0002"}))

    def test_skips_block_release(self):
        """A complete but skipped runtime result is not release-ready."""
        self.assertFalse(self.evaluate(status="SKIPPED", skipped=1))

    def test_failed_rule_blocks_release(self):
        """A failed rule remains a failure with no optional skip line."""
        self.assertFalse(self.evaluate(status="FAIL"))

    def test_missing_skip_count(self):
        """Only an actual zero-SKIP result permits an omitted skip count."""
        with self.assertRaises(AuditError):
            self.evaluate(status="SKIPPED")

    def test_mismatched_summary(self):
        """Require counters to agree with individual rule results."""
        with self.assertRaises(AuditError):
            self.evaluate(passed=0)

    def test_duplicate_result(self):
        """Reject concatenated or duplicated rule results."""
        with self.assertRaises(AuditError):
            self.evaluate(extra="Rule: R0001\nResult: PASS\n")

    def test_unfinished_rule_is_invalid(self):
        """Never accept a started rule without its terminal result."""
        with self.assertRaises(AuditError):
            self.evaluate(extra="Rule: R0002\n")

    def test_missing_completion(self):
        """A summary without the completion banner can be truncated."""
        with self.assertRaises(AuditError):
            self.evaluate(complete=False)


if __name__ == "__main__":
    unittest.main()
