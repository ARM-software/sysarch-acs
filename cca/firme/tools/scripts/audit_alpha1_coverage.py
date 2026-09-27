#!/usr/bin/env python3
#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

"""Audit the FIRME Alpha1 rule inventory and full-suite runtime readiness."""

from __future__ import annotations

import argparse
from collections import Counter
from pathlib import Path
import re
import subprocess
import sys
import textwrap


EXPECTED_RULE_COUNT = 123
STATIC_RULES = {"R0023", "R0041"}
RULE_ENUM_PATTERN = re.compile(r"^\s*FIRME_RULE_(R\d{4})\s*=", re.MULTILINE)
REGISTRY_PATTERN = re.compile(r"\.rule_id\s*=\s*FIRME_RULE_(R\d{4})")
SCENARIO_ROW_PATTERN = re.compile(r"^  \* -[ \t]*$", re.MULTILINE)
SCENARIO_RULE_PATTERN = re.compile(r"\b(R\d{4}):")
SCENARIO_LINK_PATTERN = re.compile(r"<(\.\./test/[^>]+)>")
C_COMMENT_PATTERN = re.compile(r"/\*.*?\*/|//[^\n]*", re.DOTALL)
LOG_RULE_PATTERN = re.compile(r"^Rule:\s*(R\d{4})\s*$")
LOG_RESULT_PATTERN = re.compile(r"^Result:\s*(PASS|FAIL|SKIPPED)\s*$")
LOG_SUMMARY_PATTERN = re.compile(
    r"Total Tests run\s*=\s*(\d+)\s*;\s*"
    r"Tests Passed\s*=\s*(\d+)\s*;\s*"
    r"Tests Failed\s*=\s*(\d+)"
)
LOG_SKIPPED_PATTERN = re.compile(r"Tests Skipped\s*=\s*(\d+)")


class AuditError(Exception):
    """Represent a rule-inventory or regression-log inconsistency."""


def read_text(path: Path) -> str:
    """Read a UTF-8 source or log file."""
    return path.read_text(encoding="utf-8", errors="replace")


def strip_c_comments(content: str) -> str:
    """Exclude commented-out registry entries from the active inventory."""
    return C_COMMENT_PATTERN.sub("", content)


def unique_rules(label: str, rules: list[str]) -> set[str]:
    """Reject duplicate rule identifiers before returning an inventory set."""
    duplicates = sorted(
        rule for rule, count in Counter(rules).items() if count != 1
    )
    if duplicates:
        raise AuditError(f"{label} has duplicate rules: {', '.join(duplicates)}")
    return set(rules)


def format_rules(rules: set[str]) -> str:
    """Format rule identifiers for a readable diagnostic."""
    return textwrap.fill(", ".join(sorted(rules)), width=79,
                         subsequent_indent="  ")


def check_same_rules(label: str, actual: set[str], expected: set[str]) -> None:
    """Require two rule inventories to contain exactly the same identifiers."""
    missing = expected - actual
    extra = actual - expected
    if missing or extra:
        raise AuditError(
            f"{label} mismatch: missing [{format_rules(missing)}], "
            f"extra [{format_rules(extra)}]"
        )


def read_scenario_rules(root: Path) -> set[str]:
    """Check each handwritten scenario has a unique rule and a live test link."""
    document = root / "docs/FIRME_Architecture_Compliance_Suite_Scenario_Document.rst"
    content = read_text(document)
    if ".. list-table::" not in content:
        raise AuditError("scenario document has no rule table")

    rows = SCENARIO_ROW_PATTERN.split(content.split(".. list-table::", 1)[1])
    rules = []
    for row in rows[1:]:
        row_rules = SCENARIO_RULE_PATTERN.findall(row)
        if len(row_rules) != 1:
            raise AuditError(f"scenario row has {len(row_rules)} rule IDs")
        rule = row_rules[0]
        links = SCENARIO_LINK_PATTERN.findall(row)
        if rule in STATIC_RULES:
            if links:
                raise AuditError(f"{rule} is static but links to a test")
        elif len(links) != 1:
            raise AuditError(f"{rule} needs exactly one test source link")
        else:
            source = (document.parent / links[0]).resolve()
            try:
                source.relative_to((root / "test").resolve())
            except ValueError as error:
                raise AuditError(f"{rule} test link escapes test/") from error
            if not source.is_file():
                raise AuditError(f"{rule} test source is missing: {source}")
        rules.append(rule)

    return unique_rules("scenario document", rules)


def read_generated_states(root: Path) -> dict[str, str]:
    """Read generated ABI-rule states after checking catalog freshness."""
    generator = root / "tools/scripts/generate_firme_abi_catalog.py"
    check = subprocess.run(
        [sys.executable, str(generator), "--check"],
        capture_output=True, text=True, check=False
    )
    if check.returncode != 0:
        raise AuditError(
            "generated ABI catalog is stale or invalid: "
            f"{check.stderr.strip()}"
        )

    coverage = read_text(root / "docs/FIRME_Alpha1_ABI_Interface_Coverage.md")
    states = {}
    for line in coverage.splitlines():
        if not re.match(r"^\|\s*R\d{4}\s*\|", line):
            continue
        columns = [column.strip() for column in line.strip().strip("|").split("|")]
        if len(columns) != 6 or columns[3] not in {"Active", "Gated"}:
            raise AuditError(f"malformed generated coverage row: {line}")
        rule = columns[0]
        if rule in states:
            raise AuditError(f"duplicate generated coverage rule: {rule}")
        states[rule] = columns[3]
    if len(states) != 24:
        raise AuditError(f"expected 24 generated ABI rules, found {len(states)}")
    return states


def audit_inventory(root: Path) -> tuple[set[str], set[str]]:
    """Reconcile ALP4 rules, scenarios, generated plans and registry entries."""
    header = read_text(root / "val/include/val_firme_rules.h")
    declared = unique_rules(
        "rule enum", RULE_ENUM_PATTERN.findall(header)
    )
    if len(declared) != EXPECTED_RULE_COUNT:
        raise AuditError(
            f"expected {EXPECTED_RULE_COUNT} Alpha1 rules, found {len(declared)}"
        )
    if not STATIC_RULES <= declared:
        raise AuditError("static-rule identifiers are absent from the catalog")

    handwritten = read_scenario_rules(root)
    generated = read_generated_states(root)
    check_same_rules("documented scenarios", handwritten | set(generated), declared)

    registry = strip_c_comments(
        read_text(root / "val/src/val_firme_suite.c") + "\n" +
        read_text(root / "val/include/val_firme_abi_registry.inc")
    )
    active = unique_rules("runtime registry", REGISTRY_PATTERN.findall(registry))
    if active - declared:
        raise AuditError(
            f"registry has unscoped rules: {format_rules(active - declared)}"
        )
    if active & STATIC_RULES:
        raise AuditError("static rules must not enter the runtime registry")

    for rule, state in generated.items():
        expected_state = "Active" if rule in active else "Gated"
        if state != expected_state:
            raise AuditError(
                f"{rule}: generated state {state}, registry {expected_state}"
            )

    gated = declared - active - STATIC_RULES
    print(
        f"Alpha1 inventory: {len(declared)} scoped, {len(active)} active, "
        f"{len(gated)} gated, {len(STATIC_RULES)} static"
    )
    print(f"Gated rules: {format_rules(gated)}")
    return active, gated


def audit_runtime(log_path: Path, active: set[str], gated: set[str]) -> bool:
    """Require complete, passing UART results with no deferred rule coverage."""
    content = read_text(log_path)
    results = {}
    current_rule = None
    for line in content.splitlines():
        rule_match = LOG_RULE_PATTERN.fullmatch(line.strip())
        result_match = LOG_RESULT_PATTERN.fullmatch(line.strip())
        if rule_match:
            if current_rule is not None:
                raise AuditError(f"{current_rule} has no result before next rule")
            current_rule = rule_match.group(1)
        elif result_match:
            if current_rule is None:
                raise AuditError("result has no preceding rule")
            if current_rule in results:
                raise AuditError(f"duplicate runtime result for {current_rule}")
            results[current_rule] = result_match.group(1)
            current_rule = None
    if current_rule is not None:
        raise AuditError(f"{current_rule} has no final result")
    check_same_rules("full-suite UART results", set(results), active)

    summary = LOG_SUMMARY_PATTERN.search(content)
    skipped_summary = LOG_SKIPPED_PATTERN.search(content)
    if summary is None or skipped_summary is None:
        raise AuditError("UART log has no complete suite summary")
    total, passed, failed = (int(value) for value in summary.groups())
    skipped = int(skipped_summary.group(1))
    counts = Counter(results.values())
    if (total != len(results) or passed != counts["PASS"] or
            failed != counts["FAIL"] or skipped != counts["SKIPPED"] or
            total != passed + failed + skipped):
        raise AuditError("UART summary does not match per-rule results")

    runtime_skips = {rule for rule, status in results.items()
                     if status == "SKIPPED"}
    runtime_failures = {rule for rule, status in results.items()
                        if status == "FAIL"}
    print(f"UART results: {passed} PASS, {failed} FAIL, {skipped} SKIPPED")
    if runtime_skips:
        print(f"Runtime skips: {format_rules(runtime_skips)}")
    if runtime_failures:
        print(f"Runtime failures: {format_rules(runtime_failures)}")
    if gated or runtime_skips or runtime_failures:
        print("NOT RELEASE READY: gated, skipped or failing rules remain")
        return False
    print("RELEASE CHECK PASSED")
    return True


def main() -> int:
    """Run an inventory-only check or audit a complete SUITE=all UART log."""
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--inventory-only", action="store_true")
    mode.add_argument("--uart-log", type=Path)
    args = parser.parse_args()

    try:
        root = Path(__file__).resolve().parents[2]
        active, gated = audit_inventory(root)
        if args.inventory_only:
            print("Inventory is consistent; release readiness was not evaluated")
            return 0
        return 0 if audit_runtime(args.uart_log, active, gated) else 1
    except (AuditError, OSError) as error:
        print(f"FIRME Alpha1 audit error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
