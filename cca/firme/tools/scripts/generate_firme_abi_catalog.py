#!/usr/bin/env python3
#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

"""Validate FIRME ABI scenarios and generate the Alpha1 test framework."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any


STATUS_VALUES = {
    "SUCCESS": 0,
    "NOT_SUPPORTED": -1,
    "INVALID_PARAMETERS": -2,
    "ABORTED": -3,
    "INCOMPLETE": -4,
    "DENIED": -5,
    "BUSY": -6,
    "OP_CONFLICT": -7,
    "ALREADY_EXISTS": -8,
    "NOT_FOUND": -9,
    "NO_MEMORY": -10,
    "BAD_DATA": -11,
}

EXECUTION_MODELS = {
    "synchronous": "VAL_FIRME_ABI_EXEC_SYNCHRONOUS",
    "retryable": "VAL_FIRME_ABI_EXEC_RETRYABLE",
    "continuation": "VAL_FIRME_ABI_EXEC_CONTINUATION",
    "stateful_reinvoke": "VAL_FIRME_ABI_EXEC_STATEFUL_REINVOKE",
    "asynchronous": "VAL_FIRME_ABI_EXEC_ASYNCHRONOUS",
    "poll": "VAL_FIRME_ABI_EXEC_POLL",
    "partial_retrieval": "VAL_FIRME_ABI_EXEC_PARTIAL_RETRIEVAL",
}

RESULT_MODELS = {
    "status": "VAL_FIRME_ABI_RESULT_STATUS",
    "version": "VAL_FIRME_ABI_RESULT_VERSION",
}

WORLD_POLICIES = {
    "mandatory": "VAL_FIRME_ABI_WORLD_MANDATORY",
    "optional": "VAL_FIRME_ABI_WORLD_OPTIONAL",
    "not_supported": "VAL_FIRME_ABI_WORLD_NOT_SUPPORTED",
}

WORLD_MASKS = {
    "ns": "VAL_FIRME_ABI_WORLD_MASK_NS",
    "secure": "VAL_FIRME_ABI_WORLD_MASK_SECURE",
    "realm": "VAL_FIRME_ABI_WORLD_MASK_REALM",
}

WORLD_SUFFIXES = {
    "ns": "ns",
    "secure": "secure",
    "realm": "realm",
}

WORLD_ENUMS = {
    "ns": "FIRME_ACS_WORLD_NSEL2",
    "secure": "FIRME_ACS_WORLD_SECURE",
    "realm": "FIRME_ACS_WORLD_REALM",
}

BASELINE_POLICIES = {
    "positive_case": "VAL_FIRME_BASELINE_POSITIVE_CASE",
    "adapter_validated": "VAL_FIRME_BASELINE_ADAPTER_VALIDATED",
}

MODULE_ENABLES = {
    "base": "FIRME_ENABLE_BASE",
    "gpt": "FIRME_ENABLE_GPT",
    "ide": "FIRME_ENABLE_IDE",
    "mec": "FIRME_ENABLE_MEC",
}

MODULE_CHAPTERS = {
    "base": "FIRME_CHAPTER_BASE",
    "gpt": "FIRME_CHAPTER_GPT",
    "ide": "FIRME_CHAPTER_IDE",
    "mec": "FIRME_CHAPTER_MEC",
}

REGISTER_PATTERN = re.compile(r"x([0-9]|1[0-7])$")
IDENTIFIER_PATTERN = re.compile(r"[A-Z][A-Z0-9_]*$")
PLAN_NAME_PATTERN = re.compile(r"[a-z][a-z0-9_]*$")
RULE_PATTERN = re.compile(r"R[0-9]{4}$")


def parse_u64(value: Any, field: str) -> int:
    """Parse a JSON integer or base-prefixed string as an unsigned value."""
    parsed = int(value, 0) if isinstance(value, str) else int(value)
    if parsed < 0 or parsed > 0xFFFFFFFFFFFFFFFF:
        raise ValueError(f"{field}: value is outside uint64 range")
    return parsed


def register_index(name: str) -> int:
    """Return an architectural register index after validating its name."""
    match = REGISTER_PATTERN.fullmatch(name)
    if match is None:
        raise ValueError(f"invalid register name: {name}")
    return int(match.group(1))


def status_mask(statuses: list[str]) -> int:
    """Encode architected status values as bit N for status -N."""
    mask = 0
    for name in statuses:
        if name not in STATUS_VALUES:
            raise ValueError(f"unknown FIRME status: {name}")
        mask |= 1 << -STATUS_VALUES[name]
    return mask


def register_mask(registers: list[str]) -> int:
    """Encode a list of x0-x17 register names as a bit mask."""
    mask = 0
    for name in registers:
        mask |= 1 << register_index(name)
    return mask


def world_mask(worlds: list[str]) -> int:
    """Encode a list of caller worlds as a numeric bit mask."""
    mask = 0
    for world in worlds:
        if world not in WORLD_MASKS:
            raise ValueError(f"invalid caller world: {world}")
        mask |= 1 << list(WORLD_MASKS).index(world)
    return mask


def enum_name(prefix: str, value: str) -> str:
    """Convert a reviewed identifier into a generated C enum name."""
    normalized = re.sub(r"[^A-Z0-9]+", "_", value.upper()).strip("_")
    return f"{prefix}_{normalized}"


def c_string(value: str) -> str:
    """Escape a reviewed string for a generated C string literal."""
    return value.replace("\\", "\\\\").replace('"', '\\"')


def load_scenarios(directory: Path) -> dict[str, dict[str, Any]]:
    """Load one deterministic scenario document for every release ABI."""
    scenarios: dict[str, dict[str, Any]] = {}
    paths = sorted(directory.glob("*.json"))
    if not paths:
        raise ValueError(f"no scenario files found under {directory}")

    for path in paths:
        scenario = json.loads(path.read_text(encoding="utf-8"))
        abi_id = scenario.get("abi", "")
        if abi_id in scenarios:
            raise ValueError(f"duplicate scenario for ABI {abi_id}")
        scenario["_source"] = path.name
        scenarios[abi_id] = scenario

    return scenarios


def validate_abi(abi: dict[str, Any], identifiers: set[str], names: set[str],
                 fids: set[int], feature_keys: set[tuple[int, int]]) -> None:
    """Validate one ABI descriptor from the reviewed catalog."""
    identifier = abi.get("id", "")
    name = abi.get("name", "")
    if IDENTIFIER_PATTERN.fullmatch(identifier) is None:
        raise ValueError(f"invalid ABI id: {identifier}")
    if identifier in identifiers:
        raise ValueError(f"duplicate ABI id: {identifier}")
    identifiers.add(identifier)

    if not name.startswith("FIRME_") or name in names:
        raise ValueError(f"invalid or duplicate ABI name: {name}")
    names.add(name)

    service_id = int(abi.get("service_id", -1))
    if service_id < 0 or service_id > 0xFF:
        raise ValueError(f"{identifier}: service_id is not an 8-bit value")
    if service_id > 5:
        raise ValueError(f"{identifier}: service_id is not allocated by ALP4")

    fid = parse_u64(abi.get("fid"), f"{identifier}.fid")
    if fid in fids:
        raise ValueError(f"{identifier}: duplicate FID 0x{fid:x}")
    if (fid & (1 << 30)) == 0:
        raise ValueError(f"{identifier}: FID is not SMC64")
    fids.add(fid)

    feature_bit = int(abi.get("feature_bit", -1))
    if feature_bit < 0 or feature_bit > 63:
        raise ValueError(f"{identifier}: invalid feature bit")
    feature_key = (service_id, feature_bit)
    if feature_key in feature_keys:
        raise ValueError(f"{identifier}: duplicate service feature bit")
    feature_keys.add(feature_key)

    if abi.get("execution_model") not in EXECUTION_MODELS:
        raise ValueError(f"{identifier}: invalid execution model")
    if abi.get("result_model") not in RESULT_MODELS:
        raise ValueError(f"{identifier}: invalid result model")
    if RULE_PATTERN.fullmatch(abi.get("input_rule", "")) is None:
        raise ValueError(f"{identifier}: invalid input rule")
    if RULE_PATTERN.fullmatch(abi.get("output_rule", "")) is None:
        raise ValueError(f"{identifier}: invalid output rule")

    input_masks = abi.get("input_masks")
    if not isinstance(input_masks, dict) or "x0" not in input_masks:
        raise ValueError(f"{identifier}: input_masks must include x0")
    for register, value in input_masks.items():
        register_index(register)
        parse_u64(value, f"{identifier}.{register}")

    outputs = abi.get("output_registers")
    if not isinstance(outputs, list) or "x0" not in outputs:
        raise ValueError(f"{identifier}: output_registers must include x0")
    register_mask(outputs)
    status_mask(abi.get("allowed_statuses", []))

    policies = abi.get("world_policy")
    if not isinstance(policies, dict):
        raise ValueError(f"{identifier}: world_policy must be an object")
    if set(policies) != set(WORLD_MASKS):
        raise ValueError(
            f"{identifier}: world_policy must define ns, secure and realm"
        )
    for policy in policies.values():
        if policy not in WORLD_POLICIES:
            raise ValueError(f"{identifier}: invalid world policy {policy}")


def validate_feature_registers(catalog: dict[str, Any]) -> list[dict[str, int]]:
    """Validate the reviewed feature-register metadata used by Base rules."""
    registers = catalog.get("feature_registers")
    if not isinstance(registers, list) or not registers:
        raise ValueError("feature_registers must be a non-empty array")

    validated: list[dict[str, int]] = []
    keys: set[tuple[int, int]] = set()
    for register in registers:
        service_id = int(register.get("service_id", -1))
        index = int(register.get("index", -1))
        size_bits = int(register.get("size_bits", 0))
        key = (service_id, index)

        if service_id < 0 or service_id > 0xFF:
            raise ValueError(
                f"feature register service_id {service_id} is not an 8-bit value"
            )
        if service_id > 5:
            raise ValueError(
                f"feature register service_id {service_id} is not allocated by ALP4"
            )
        if index < 0 or index > 0xFF:
            raise ValueError(
                f"service {service_id}: feature register index is not 8-bit"
            )
        if key in keys:
            raise ValueError(
                f"service {service_id}: duplicate feature register {index}"
            )
        keys.add(key)

        if size_bits <= 0 or (size_bits % 64) != 0:
            raise ValueError(
                f"service {service_id} register {index}: size is not a "
                "non-zero multiple of 64 bits"
            )

        field = register.get("max_concurrent_op_count")
        shift = 0
        width = 0
        if field is not None:
            if not isinstance(field, dict):
                raise ValueError(
                    f"service {service_id} register {index}: invalid "
                    "MAX_CONCURRENT_OP_CNT descriptor"
                )
            shift = int(field.get("shift", -1))
            width = int(field.get("width", 0))
            if shift < 0 or width != 16 or (shift + width) > size_bits:
                raise ValueError(
                    f"service {service_id} register {index}: "
                    "MAX_CONCURRENT_OP_CNT must be a 16-bit in-range field"
                )

        validated.append({
            "service_id": service_id,
            "index": index,
            "size_bits": size_bits,
            "max_concurrent_op_shift": shift,
            "max_concurrent_op_width": width,
        })

    for service_id in range(6):
        if (service_id, 0) not in keys or (service_id, 1) not in keys:
            raise ValueError(
                f"service {service_id}: ALP4 feature registers 0 and 1 are required"
            )

        register1 = next(
            register for register in validated
            if register["service_id"] == service_id and register["index"] == 1
        )
        if service_id != 0 and register1["max_concurrent_op_width"] != 16:
            raise ValueError(
                f"service {service_id}: feature register 1 needs a 16-bit "
                "MAX_CONCURRENT_OP_CNT field"
            )

    return sorted(
        validated,
        key=lambda register: (register["service_id"], register["index"]),
    )


def validate_plan(plan: dict[str, Any], abi: dict[str, Any],
                  plan_ids: set[str], plan_names: set[str],
                  rules: set[str], mutations: set[str]) -> None:
    """Validate one input- or output-rule scenario."""
    plan_id = plan.get("id", "")
    plan_name = plan.get("name", "")
    kind = plan.get("kind", "")
    rule = plan.get("rule", "")

    if IDENTIFIER_PATTERN.fullmatch(plan_id) is None or plan_id in plan_ids:
        raise ValueError(f"invalid or duplicate test plan id: {plan_id}")
    plan_ids.add(plan_id)
    if PLAN_NAME_PATTERN.fullmatch(plan_name) is None or plan_name in plan_names:
        raise ValueError(f"invalid or duplicate test plan name: {plan_name}")
    plan_names.add(plan_name)
    if RULE_PATTERN.fullmatch(rule) is None or rule in rules:
        raise ValueError(f"invalid or duplicate interface rule: {rule}")
    rules.add(rule)
    if kind not in {"input", "output"}:
        raise ValueError(f"{plan_id}: kind must be input or output")
    expected_rule = abi[f"{kind}_rule"]
    if rule != expected_rule:
        raise ValueError(
            f"{plan_id}: {kind} plan must cover {expected_rule}, not {rule}"
        )

    description = plan.get("description")
    if not isinstance(description, str) or not description:
        raise ValueError(f"{plan_id}: description must be non-empty")
    if not isinstance(plan.get("enabled"), bool):
        raise ValueError(f"{plan_id}: enabled must be boolean")
    if not plan["enabled"] and not plan.get("disabled_reason"):
        raise ValueError(f"{plan_id}: disabled plan needs disabled_reason")
    if plan["enabled"] and plan.get("disabled_reason"):
        raise ValueError(f"{plan_id}: enabled plan cannot have disabled_reason")

    required = plan.get("required_worlds")
    optional = plan.get("optional_worlds")
    if not isinstance(required, list) or not isinstance(optional, list):
        raise ValueError(f"{plan_id}: world lists must be arrays")
    if set(required) & set(optional):
        raise ValueError(f"{plan_id}: required and optional worlds overlap")
    world_mask(required + optional)
    if not required and not optional:
        raise ValueError(f"{plan_id}: plan has no execution world")
    for world in required + optional:
        if abi["world_policy"][world] == "not_supported":
            raise ValueError(f"{plan_id}: {world} is not supported for this ABI")

    if plan.get("baseline_policy") not in BASELINE_POLICIES:
        raise ValueError(f"{plan_id}: invalid baseline policy")
    if not isinstance(plan.get("require_advertised"), bool):
        raise ValueError(f"{plan_id}: require_advertised must be boolean")
    if not isinstance(plan.get("require_output_validation"), bool):
        raise ValueError(f"{plan_id}: require_output_validation must be boolean")

    cases = plan.get("cases")
    if not isinstance(cases, list) or not cases:
        raise ValueError(f"{plan_id}: cases must be a non-empty array")
    case_names: set[str] = set()
    has_positive = False
    for case in cases:
        case_name = case.get("name", "")
        if not case_name or case_name in case_names:
            raise ValueError(f"{plan_id}: invalid or duplicate case {case_name}")
        case_names.add(case_name)
        mutation = case.get("mutation", "")
        if not mutation:
            raise ValueError(f"{plan_id}.{case_name}: missing mutation")
        mutations.add(mutation)
        expected = case.get("expected")
        if expected == "positive":
            if abi["result_model"] != "version":
                raise ValueError(
                    f"{plan_id}.{case_name}: positive requires a version ABI"
                )
            has_positive = True
        elif expected == "architected_status":
            if abi["result_model"] != "status":
                raise ValueError(
                    f"{plan_id}.{case_name}: status requires a status ABI"
                )
        elif expected not in STATUS_VALUES:
            raise ValueError(f"{plan_id}.{case_name}: invalid result {expected}")
        elif expected not in abi["allowed_statuses"]:
            raise ValueError(
                f"{plan_id}.{case_name}: {expected} is not valid for the ABI"
            )
        if expected == "SUCCESS":
            has_positive = True
        if int(case.get("step", 0)) <= 0:
            raise ValueError(f"{plan_id}.{case_name}: step must be positive")
        if int(case.get("priority", -1)) < 0:
            raise ValueError(f"{plan_id}.{case_name}: priority must be non-negative")
        if not isinstance(case.get("verify_no_effect", False), bool):
            raise ValueError(
                f"{plan_id}.{case_name}: verify_no_effect must be boolean"
            )

    if plan["baseline_policy"] == "positive_case" and not has_positive:
        raise ValueError(f"{plan_id}: positive_case has no successful case")


def validate_model(catalog: dict[str, Any],
                   scenarios: dict[str, dict[str, Any]]) -> dict[str, Any]:
    """Validate catalog/scenarios and return the deterministic release model."""
    if catalog.get("schema_version") != 2:
        raise ValueError("catalog schema_version must be 2")
    if catalog.get("release_scope") != "Alpha1":
        raise ValueError("release_scope must be Alpha1")
    if catalog.get("register_count") != 18:
        raise ValueError("register_count must be 18")
    if catalog.get("service_id_width") != 8:
        raise ValueError("service_id_width must be 8")

    feature_registers = validate_feature_registers(catalog)

    abis = catalog.get("abis")
    release_ids = catalog.get("release_abis")
    if not isinstance(abis, list) or not abis:
        raise ValueError("abis must be a non-empty array")
    if not isinstance(release_ids, list) or not release_ids:
        raise ValueError("release_abis must be a non-empty array")
    if len(set(release_ids)) != len(release_ids):
        raise ValueError("release_abis contains duplicates")

    identifiers: set[str] = set()
    names: set[str] = set()
    fids: set[int] = set()
    feature_keys: set[tuple[int, int]] = set()
    abi_by_id: dict[str, dict[str, Any]] = {}
    for abi in abis:
        validate_abi(abi, identifiers, names, fids, feature_keys)
        abi_by_id[abi["id"]] = abi

    for identifier in release_ids:
        if identifier not in abi_by_id:
            raise ValueError(f"release ABI is not in catalog: {identifier}")
        if abi_by_id[identifier]["module"] not in MODULE_ENABLES:
            raise ValueError(f"{identifier}: module is outside Alpha1 scope")

    if set(scenarios) != set(release_ids):
        missing = sorted(set(release_ids) - set(scenarios))
        extra = sorted(set(scenarios) - set(release_ids))
        raise ValueError(
            f"scenario/release mismatch: missing={missing}, extra={extra}"
        )

    plans: list[dict[str, Any]] = []
    plan_ids: set[str] = set()
    plan_names: set[str] = set()
    rules: set[str] = set()
    mutations = {"none"}
    scoped_abis = [abi_by_id[identifier] for identifier in release_ids]
    for abi in scoped_abis:
        scenario = scenarios[abi["id"]]
        if scenario.get("schema_version") != 1:
            raise ValueError(
                f"{scenario['_source']}: scenario schema_version must be 1"
            )
        scenario_plans = scenario.get("plans")
        if not isinstance(scenario_plans, list) or len(scenario_plans) != 2:
            raise ValueError(f"{abi['id']}: exactly two plans are required")
        kinds = {plan.get("kind") for plan in scenario_plans}
        if kinds != {"input", "output"}:
            raise ValueError(f"{abi['id']}: input and output plans are required")
        for plan in scenario_plans:
            validate_plan(
                plan, abi, plan_ids, plan_names, rules, mutations
            )
            release_plan = dict(plan)
            release_plan["abi"] = abi["id"]
            plans.append(release_plan)

    return {
        "specification": catalog["specification"],
        "release_scope": catalog["release_scope"],
        "service_id_width": catalog["service_id_width"],
        "feature_registers": feature_registers,
        "abis": scoped_abis,
        "test_plans": plans,
        "mutations": ["none"] + sorted(mutations - {"none"}),
    }


def format_input_masks(abi: dict[str, Any]) -> list[str]:
    """Format one fixed-size input-bit-mask initializer."""
    values = [0] * 18
    for register, value in abi["input_masks"].items():
        values[register_index(register)] = parse_u64(value, register)
    return [f"UINT64_C(0x{value:016X})" for value in values]


def generate_header(model: dict[str, Any]) -> str:
    """Generate catalog, scenario, and stable identifier declarations."""
    abis = model["abis"]
    plans = model["test_plans"]
    lines = [
        "/*",
        " * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.",
        " *",
        " * SPDX-License-Identifier: BSD-3-Clause",
        " *",
        " * Generated by tools/scripts/generate_firme_abi_catalog.py. Do not edit.",
        " */",
        "",
        "#pragma once",
        "",
        "#include <stdint.h>",
        "",
        "#define VAL_FIRME_ABI_REGISTER_COUNT 18U",
        "#define VAL_FIRME_ABI_WORLD_COUNT 3U",
        f"#define VAL_FIRME_SERVICE_ID_WIDTH {model['service_id_width']}U",
        "#define VAL_FIRME_ABI_WORLD_MASK_NS (UINT32_C(1) << 0)",
        "#define VAL_FIRME_ABI_WORLD_MASK_SECURE (UINT32_C(1) << 1)",
        "#define VAL_FIRME_ABI_WORLD_MASK_REALM (UINT32_C(1) << 2)",
        "",
        "typedef enum {",
    ]
    for index, abi in enumerate(abis):
        lines.append(f"    VAL_FIRME_ABI_ID_{abi['id']} = {index},")
    lines.extend([
        f"    VAL_FIRME_ABI_ID_COUNT = {len(abis)},",
        "} val_firme_abi_id_t;",
        "",
        "typedef enum {",
        "    VAL_FIRME_ABI_WORLD_NS = 0,",
        "    VAL_FIRME_ABI_WORLD_SECURE = 1,",
        "    VAL_FIRME_ABI_WORLD_REALM = 2,",
        "} val_firme_abi_world_t;",
        "",
        "typedef enum {",
        "    VAL_FIRME_ABI_WORLD_MANDATORY = 0,",
        "    VAL_FIRME_ABI_WORLD_OPTIONAL = 1,",
        "    VAL_FIRME_ABI_WORLD_NOT_SUPPORTED = 2,",
        "} val_firme_abi_world_policy_t;",
        "",
        "typedef enum {",
        "    VAL_FIRME_ABI_EXEC_SYNCHRONOUS = 0,",
        "    VAL_FIRME_ABI_EXEC_RETRYABLE,",
        "    VAL_FIRME_ABI_EXEC_CONTINUATION,",
        "    VAL_FIRME_ABI_EXEC_STATEFUL_REINVOKE,",
        "    VAL_FIRME_ABI_EXEC_ASYNCHRONOUS,",
        "    VAL_FIRME_ABI_EXEC_POLL,",
        "    VAL_FIRME_ABI_EXEC_PARTIAL_RETRIEVAL,",
        "} val_firme_abi_execution_t;",
        "",
        "typedef enum {",
        "    VAL_FIRME_ABI_RESULT_STATUS = 0,",
        "    VAL_FIRME_ABI_RESULT_VERSION,",
        "} val_firme_abi_result_model_t;",
        "",
        "typedef enum {",
        "    VAL_FIRME_EXPECT_STATUS = 0,",
        "    VAL_FIRME_EXPECT_POSITIVE,",
        "    VAL_FIRME_EXPECT_ARCHITECTED_STATUS,",
        "} val_firme_abi_expectation_t;",
        "",
        "typedef enum {",
        "    VAL_FIRME_BASELINE_POSITIVE_CASE = 0,",
        "    VAL_FIRME_BASELINE_ADAPTER_VALIDATED,",
        "} val_firme_abi_baseline_t;",
        "",
        "typedef enum {",
    ])
    for index, mutation in enumerate(model["mutations"]):
        lines.append(
            f"    {enum_name('VAL_FIRME_MUTATION', mutation)} = {index},"
        )
    lines.extend([
        "} val_firme_abi_mutation_t;",
        "",
        "typedef enum {",
    ])
    for index, plan in enumerate(plans):
        lines.append(f"    VAL_FIRME_TEST_PLAN_{plan['id']} = {index},")
    lines.extend([
        f"    VAL_FIRME_TEST_PLAN_COUNT = {len(plans)},",
        "} val_firme_abi_test_plan_id_t;",
        "",
        "typedef struct {",
        "    val_firme_abi_id_t id;",
        "    const char *name;",
        "    const char *module;",
        "    const char *input_rule;",
        "    const char *output_rule;",
        "    uint64_t fid;",
        "    uint64_t feature_bit;",
        "    uint64_t input_bit_masks[VAL_FIRME_ABI_REGISTER_COUNT];",
        "    uint8_t service_id;",
        "    uint32_t input_register_mask;",
        "    uint32_t output_register_mask;",
        "    uint32_t allowed_status_mask;",
        "    val_firme_abi_execution_t execution_model;",
        "    val_firme_abi_result_model_t result_model;",
        "    val_firme_abi_world_policy_t world_policy[VAL_FIRME_ABI_WORLD_COUNT];",
        "} val_firme_abi_descriptor_t;",
        "",
        "typedef struct {",
        "    uint16_t size_bits;",
        "    uint8_t service_id;",
        "    uint8_t index;",
        "    uint8_t max_concurrent_op_shift;",
        "    uint8_t max_concurrent_op_width;",
        "} val_firme_feature_register_descriptor_t;",
        "",
        "typedef struct {",
        "    const char *name;",
        "    val_firme_abi_mutation_t mutation;",
        "    val_firme_abi_expectation_t expectation;",
        "    int32_t expected_status;",
        "    uint32_t diagnostic_step;",
        "    uint32_t priority;",
        "    uint32_t verify_no_effect;",
        "} val_firme_abi_case_t;",
        "",
        "typedef struct {",
        "    val_firme_abi_test_plan_id_t id;",
        "    const char *name;",
        "    const char *rule;",
        "    const char *description;",
        "    const char *disabled_reason;",
        "    val_firme_abi_id_t abi_id;",
        "    val_firme_abi_baseline_t baseline_policy;",
        "    uint32_t required_worlds_mask;",
        "    uint32_t optional_worlds_mask;",
        "    uint32_t enabled;",
        "    uint32_t require_advertised;",
        "    uint32_t require_output_validation;",
        "    const val_firme_abi_case_t *cases;",
        "    uint32_t case_count;",
        "} val_firme_abi_test_plan_t;",
        "",
        "extern const val_firme_abi_descriptor_t g_val_firme_abi_catalog[];",
        "extern const uint32_t g_val_firme_abi_catalog_count;",
        "extern const val_firme_feature_register_descriptor_t",
        "    g_val_firme_feature_register_catalog[];",
        "extern const uint32_t g_val_firme_feature_register_catalog_count;",
        "extern const val_firme_abi_test_plan_t g_val_firme_abi_test_plans[];",
        "extern const uint32_t g_val_firme_abi_test_plan_count;",
        "",
        "/** Return an ABI descriptor by stable identifier, or NULL. */",
        "const val_firme_abi_descriptor_t *val_firme_abi_catalog_get(",
        "    val_firme_abi_id_t id);",
        "",
        "/** Return an ABI descriptor by function identifier, or NULL. */",
        "const val_firme_abi_descriptor_t *val_firme_abi_catalog_find_fid(",
        "    uint64_t fid);",
        "",
        "/** Return feature-register metadata by service and index, or NULL. */",
        "const val_firme_feature_register_descriptor_t *",
        "val_firme_feature_register_catalog_get(uint32_t service_id,",
        "                                       uint32_t index);",
        "",
        "/** Return a generated test plan by stable identifier, or NULL. */",
        "const val_firme_abi_test_plan_t *val_firme_abi_test_plan_get(",
        "    val_firme_abi_test_plan_id_t id);",
        "",
    ])
    return "\n".join(lines)


def generate_source(model: dict[str, Any]) -> str:
    """Generate ABI descriptors, scenario cases, and lookup helpers."""
    abis = model["abis"]
    plans = model["test_plans"]
    lines = [
        "/*",
        " * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.",
        " *",
        " * SPDX-License-Identifier: BSD-3-Clause",
        " *",
        " * Generated by tools/scripts/generate_firme_abi_catalog.py. Do not edit.",
        " */",
        "",
        "#include \"val_firme_abi_catalog.h\"",
        "",
        "#include <stddef.h>",
        "",
        "const val_firme_feature_register_descriptor_t",
        "    g_val_firme_feature_register_catalog[] = {",
    ]

    for register in model["feature_registers"]:
        lines.extend([
            "    {",
            f"        .size_bits = {register['size_bits']}U,",
            f"        .service_id = {register['service_id']}U,",
            f"        .index = {register['index']}U,",
            "        .max_concurrent_op_shift = "
            f"{register['max_concurrent_op_shift']}U,",
            "        .max_concurrent_op_width = "
            f"{register['max_concurrent_op_width']}U,",
            "    },",
        ])

    lines.extend([
        "};",
        "",
        "const uint32_t g_val_firme_feature_register_catalog_count =",
        "    (uint32_t)(sizeof(g_val_firme_feature_register_catalog) /",
        "               sizeof(g_val_firme_feature_register_catalog[0]));",
        "",
        "const val_firme_abi_descriptor_t g_val_firme_abi_catalog[] = {",
    ])

    for abi in abis:
        input_masks = format_input_masks(abi)
        policies = abi["world_policy"]
        input_register_mask = register_mask(list(abi["input_masks"]))
        output_register_mask = register_mask(abi["output_registers"])
        allowed_status_mask = status_mask(abi["allowed_statuses"])
        lines.extend([
            "    {",
            f"        .id = VAL_FIRME_ABI_ID_{abi['id']},",
            f"        .name = \"{c_string(abi['name'])}\",",
            f"        .module = \"{c_string(abi['module'])}\",",
            f"        .input_rule = \"{abi['input_rule']}\",",
            f"        .output_rule = \"{abi['output_rule']}\",",
            f"        .fid = UINT64_C(0x{parse_u64(abi['fid'], 'fid'):08X}),",
            f"        .feature_bit = UINT64_C(0x{1 << int(abi['feature_bit']):X}),",
            "        .input_bit_masks = {",
        ])
        for offset in range(0, 18, 2):
            values = ", ".join(input_masks[offset:offset + 2])
            lines.append(f"            {values},")
        lines.extend([
            "        },",
            f"        .service_id = {int(abi['service_id'])}U,",
            f"        .input_register_mask = UINT32_C(0x{input_register_mask:X}),",
            f"        .output_register_mask = UINT32_C(0x{output_register_mask:X}),",
            f"        .allowed_status_mask = UINT32_C(0x{allowed_status_mask:X}),",
            f"        .execution_model = {EXECUTION_MODELS[abi['execution_model']]},",
            f"        .result_model = {RESULT_MODELS[abi['result_model']]},",
            "        .world_policy = {",
            f"            {WORLD_POLICIES[policies['ns']]},",
            f"            {WORLD_POLICIES[policies['secure']]},",
            f"            {WORLD_POLICIES[policies['realm']]},",
            "        },",
            "    },",
        ])

    lines.extend([
        "};",
        "",
        "const uint32_t g_val_firme_abi_catalog_count =",
        "    (uint32_t)(sizeof(g_val_firme_abi_catalog) /",
        "               sizeof(g_val_firme_abi_catalog[0]));",
        "",
    ])

    for plan in plans:
        lines.append(
            f"static const val_firme_abi_case_t g_{plan['name']}_cases[] = {{"
        )
        for case in plan["cases"]:
            expected = case["expected"]
            if expected == "positive":
                expectation = "VAL_FIRME_EXPECT_POSITIVE"
                expected_status = 0
            elif expected == "architected_status":
                expectation = "VAL_FIRME_EXPECT_ARCHITECTED_STATUS"
                expected_status = 0
            else:
                expectation = "VAL_FIRME_EXPECT_STATUS"
                expected_status = STATUS_VALUES[expected]
            lines.extend([
                "    {",
                f"        .name = \"{c_string(case['name'])}\",",
                "        .mutation = "
                f"{enum_name('VAL_FIRME_MUTATION', case['mutation'])},",
                f"        .expectation = {expectation},",
                f"        .expected_status = {expected_status},",
                f"        .diagnostic_step = {int(case['step'])}U,",
                f"        .priority = {int(case['priority'])}U,",
                "        .verify_no_effect = "
                f"{1 if case.get('verify_no_effect') else 0}U,",
                "    },",
            ])
        lines.extend(["};", ""])

    lines.append(
        "const val_firme_abi_test_plan_t g_val_firme_abi_test_plans[] = {"
    )
    for plan in plans:
        disabled_reason = c_string(plan.get("disabled_reason", ""))
        required = world_mask(plan["required_worlds"])
        optional = world_mask(plan["optional_worlds"])
        lines.extend([
            "    {",
            f"        .id = VAL_FIRME_TEST_PLAN_{plan['id']},",
            f"        .name = \"{c_string(plan['name'])}\",",
            f"        .rule = \"{plan['rule']}\",",
            "        .description =",
            f"            \"{c_string(plan['description'])}\",",
            "        .disabled_reason =",
            f"            \"{disabled_reason}\",",
            f"        .abi_id = VAL_FIRME_ABI_ID_{plan['abi']},",
            "        .baseline_policy = "
            f"{BASELINE_POLICIES[plan['baseline_policy']]},",
            f"        .required_worlds_mask = UINT32_C(0x{required:X}),",
            f"        .optional_worlds_mask = UINT32_C(0x{optional:X}),",
            f"        .enabled = {1 if plan['enabled'] else 0}U,",
            "        .require_advertised = "
            f"{1 if plan['require_advertised'] else 0}U,",
            "        .require_output_validation = "
            f"{1 if plan['require_output_validation'] else 0}U,",
            f"        .cases = g_{plan['name']}_cases,",
            "        .case_count = "
            f"(uint32_t)(sizeof(g_{plan['name']}_cases) /",
            f"                                 sizeof(g_{plan['name']}_cases[0])),",
            "    },",
        ])
    lines.extend([
        "};",
        "",
        "const uint32_t g_val_firme_abi_test_plan_count =",
        "    (uint32_t)(sizeof(g_val_firme_abi_test_plans) /",
        "               sizeof(g_val_firme_abi_test_plans[0]));",
        "",
        "_Static_assert(VAL_FIRME_ABI_ID_COUNT ==",
        "               (sizeof(g_val_firme_abi_catalog) /",
        "                sizeof(g_val_firme_abi_catalog[0])),",
        "               \"FIRME ABI catalog count mismatch\");",
        "_Static_assert(VAL_FIRME_TEST_PLAN_COUNT ==",
        "               (sizeof(g_val_firme_abi_test_plans) /",
        "                sizeof(g_val_firme_abi_test_plans[0])),",
        "               \"FIRME ABI test plan count mismatch\");",
        "",
        "/**",
        " * @brief Return feature-register metadata by service and index.",
        " * @param service_id FIRME service identifier.",
        " * @param index Feature-register index.",
        " * @return Feature-register descriptor, or NULL when unallocated.",
        " */",
        "const val_firme_feature_register_descriptor_t *",
        "val_firme_feature_register_catalog_get(uint32_t service_id,",
        "                                       uint32_t index)",
        "{",
        "    uint32_t descriptor_index;",
        "",
        "    for (descriptor_index = 0U;",
        "         descriptor_index < g_val_firme_feature_register_catalog_count;",
        "         descriptor_index++) {",
        "        const val_firme_feature_register_descriptor_t *descriptor =",
        "            &g_val_firme_feature_register_catalog[descriptor_index];",
        "",
        "        if (((uint32_t)descriptor->service_id == service_id) &&",
        "            ((uint32_t)descriptor->index == index))",
        "            return descriptor;",
        "    }",
        "",
        "    return NULL;",
        "}",
        "",
        "/**",
        " * @brief Return an ABI descriptor by stable identifier.",
        " * @param id Stable ABI identifier.",
        " * @return ABI descriptor, or NULL if the identifier is invalid.",
        " */",
        "const val_firme_abi_descriptor_t *val_firme_abi_catalog_get(",
        "    val_firme_abi_id_t id)",
        "{",
        "    if ((uint32_t)id >= g_val_firme_abi_catalog_count)",
        "        return NULL;",
        "",
        "    return &g_val_firme_abi_catalog[(uint32_t)id];",
        "}",
        "",
        "/**",
        " * @brief Return an ABI descriptor by function identifier.",
        " * @param fid FIRME SMC function identifier.",
        " * @return ABI descriptor, or NULL when the FID is not allocated.",
        " */",
        "const val_firme_abi_descriptor_t *val_firme_abi_catalog_find_fid(",
        "    uint64_t fid)",
        "{",
        "    uint32_t index;",
        "",
        "    for (index = 0U; index < g_val_firme_abi_catalog_count; index++) {",
        "        if (g_val_firme_abi_catalog[index].fid == fid)",
        "            return &g_val_firme_abi_catalog[index];",
        "    }",
        "",
        "    return NULL;",
        "}",
        "",
        "/**",
        " * @brief Return a generated test plan by stable identifier.",
        " * @param id Stable test plan identifier.",
        " * @return Test plan, or NULL if the identifier is invalid.",
        " */",
        "const val_firme_abi_test_plan_t *val_firme_abi_test_plan_get(",
        "    val_firme_abi_test_plan_id_t id)",
        "{",
        "    if ((uint32_t)id >= g_val_firme_abi_test_plan_count)",
        "        return NULL;",
        "",
        "    return &g_val_firme_abi_test_plans[(uint32_t)id];",
        "}",
        "",
    ])
    return "\n".join(lines)


def generated_function_name(plan: dict[str, Any], world: str) -> str:
    """Return the stable generated rule wrapper name."""
    return f"firme_abi_{plan['rule'].lower()}_run_{WORLD_SUFFIXES[world]}"


def registry_world_mask_lines(field: str, worlds: list[str]) -> list[str]:
    """Format a registry world mask without producing long generated lines."""
    if not worlds:
        return [f"        .{field} = 0U,"]

    lines: list[str] = []
    for index, world in enumerate(worlds):
        prefix = f"        .{field} = " if index == 0 else " " * 32
        suffix = " |" if index + 1 < len(worlds) else ","
        lines.append(f"{prefix}{WORLD_MASKS[world]}{suffix}")
    return lines


def generate_wrapper_header(model: dict[str, Any]) -> str:
    """Generate declarations for all rule/world test entry points."""
    lines = [
        "/*",
        " * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.",
        " *",
        " * SPDX-License-Identifier: BSD-3-Clause",
        " *",
        " * Generated by tools/scripts/generate_firme_abi_catalog.py. Do not edit.",
        " */",
        "",
        "#pragma once",
        "",
        "#include <stdint.h>",
        "",
        "#include \"val_firme_rules.h\"",
        "#include \"val_firme_abi_catalog.h\"",
        "#include \"val_firme_dispatch.h\"",
        "",
        "/** Execute one generated ABI plan in the selected caller world. */",
        "int val_firme_abi_generated_execute(",
        "    val_firme_abi_test_plan_id_t plan_id, firme_acs_world_t world,",
        "    uint64_t base_pa, firme_rule_res_t *out);",
        "",
    ]
    for plan in model["test_plans"]:
        for world in plan["required_worlds"] + plan["optional_worlds"]:
            function = generated_function_name(plan, world)
            lines.extend([
                f"int {function}(uint64_t base_pa,",
                "                         firme_rule_res_t *out);",
            ])
    lines.append("")
    return "\n".join(lines)


def generate_wrapper_source(model: dict[str, Any]) -> str:
    """Generate the shared runner used by per-rule test sources."""
    lines = [
        "/*",
        " * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.",
        " *",
        " * SPDX-License-Identifier: BSD-3-Clause",
        " *",
        " * Generated by tools/scripts/generate_firme_abi_catalog.py. Do not edit.",
        " */",
        "",
        "#include \"val_firme_abi_generated.h\"",
        "",
        "#include <stddef.h>",
        "",
        "#include \"val_common/val_status.h\"",
        "#include \"val_firme_abi_adapter.h\"",
        "#include \"val_firme_abi_test.h\"",
        "#include \"val_log.h\"",
        "",
        "/**",
        " * @brief Execute one generated plan in the selected caller world.",
        " * @param plan_id Generated plan identifier.",
        " * @param world Caller world.",
        " * @param base_pa Platform test address supplied by the dispatcher.",
        " * @param out Rule result populated by the common executor.",
        " * @return VAL_SUCCESS for an architectural result, otherwise VAL_ERROR.",
        " */",
        "int val_firme_abi_generated_execute(",
        "    val_firme_abi_test_plan_id_t plan_id, firme_acs_world_t world,",
        "    uint64_t base_pa, firme_rule_res_t *out)",
        "{",
        "    const val_firme_abi_test_plan_t *plan;",
        "    const val_firme_abi_test_ops_t *ops;",
        "    val_firme_abi_test_request_t request = {0};",
        "",
        "    if (!out)",
        "        return VAL_ERROR;",
        "",
        "    plan = val_firme_abi_test_plan_get(plan_id);",
        "    if (!plan || (plan->enabled == 0U))",
        "        return VAL_ERROR;",
        "",
        "    ops = val_firme_abi_adapter_get(plan->abi_id);",
        "    if (!ops) {",
        "        LOG_ERROR(NULL, \"%s: no adapter for generated ABI plan\",",
        "                  plan->name);",
        "        out->status = (uint64_t)ACS_STATUS_FAIL;",
        "        out->fail_step = 0ULL;",
        "        out->last_x0 = 0ULL;",
        "        out->last_x1 = 0ULL;",
        "        return VAL_ERROR;",
        "    }",
        "",
        "    request.plan_id = plan_id;",
        "    request.world = world;",
        "    request.base_pa = base_pa;",
        "    request.adapter_context = NULL;",
        "    request.ops = ops;",
        "",
        "    return val_firme_abi_test_execute(&request, out);",
        "}",
        "",
    ]
    return "\n".join(lines)


def generated_test_filename(plan: dict[str, Any]) -> str:
    """Return the deterministic filename for one generated rule test."""
    return f"generated_abi_{plan['name']}.c"


def generate_test_source(plan: dict[str, Any]) -> str:
    """Generate one visible test source for one interface rule."""
    lines = [
        "/*",
        " * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.",
        " *",
        " * SPDX-License-Identifier: BSD-3-Clause",
        " *",
        " * Generated by tools/scripts/generate_firme_abi_catalog.py. Do not edit.",
        " */",
        "",
        "#include \"val_firme_abi_generated.h\"",
        "",
    ]
    for world in plan["required_worlds"] + plan["optional_worlds"]:
        function = generated_function_name(plan, world)
        lines.extend([
            "/**",
            f" * @brief Execute {plan['rule']} from the {world} caller.",
            " * @param base_pa Platform test address supplied by the dispatcher.",
            " * @param out Rule result populated by the common executor.",
            " * @return VAL_SUCCESS for an architectural result, otherwise VAL_ERROR.",
            " */",
            f"int {function}(uint64_t base_pa,",
            "                         firme_rule_res_t *out)",
            "{",
            "    return val_firme_abi_generated_execute(",
            f"        VAL_FIRME_TEST_PLAN_{plan['id']},",
            f"        {WORLD_ENUMS[world]}, base_pa, out);",
            "}",
            "",
        ])
    return "\n".join(lines)


def generate_registry(model: dict[str, Any]) -> str:
    """Generate enabled Alpha1 entries for the global test registry."""
    abi_by_id = {abi["id"]: abi for abi in model["abis"]}
    lines = [
        "/* Generated by tools/scripts/generate_firme_abi_catalog.py. */",
    ]
    for plan in model["test_plans"]:
        if not plan["enabled"]:
            continue
        abi = abi_by_id[plan["abi"]]
        module = abi["module"]
        worlds = plan["required_worlds"] + plan["optional_worlds"]
        lines.extend([
            f"#if {MODULE_ENABLES[module]}",
            "    {",
            f"        .chapter = {MODULE_CHAPTERS[module]},",
            f"        .rule_id = FIRME_RULE_{plan['rule']},",
            f"        .rule_str = \"{plan['rule']}\",",
            "        .suite = \"firme\",",
            f"        .test_name = \"{c_string(plan['name'])}\",",
            f"        .desc = \"{c_string(plan['description'])}\",",
        ])
        lines.extend(
            registry_world_mask_lines(
                "required_worlds_mask", plan["required_worlds"]
            )
        )
        lines.extend(
            registry_world_mask_lines(
                "optional_worlds_mask", plan["optional_worlds"]
            )
        )
        for world in WORLD_MASKS:
            label = (
                f'\"{plan["rule"]} from {world} world\"'
                if world in worlds else "0"
            )
            lines.append(f"        .check_{world} = {label},")
        for world in WORLD_MASKS:
            function = (
                generated_function_name(plan, world)
                if world in worlds else "0"
            )
            lines.append(f"        .run_{world} = {function},")
        lines.extend([
            "    },",
            "#endif",
        ])
    lines.append("")
    return "\n".join(lines)


def generate_coverage(model: dict[str, Any]) -> str:
    """Generate the reviewable Alpha1 ABI interface coverage table."""
    abi_by_id = {abi["id"]: abi for abi in model["abis"]}
    lines = [
        "# FIRME Alpha1 ABI Interface Coverage",
        "",
        f"Specification: `{model['specification']}`",
        "",
        "This generated table is limited to the Alpha1 Base, GPT, IDE and MEC",
        "ABIs. Attestation and Integrated Device Management are intentionally",
        "outside this generator scope for this release.",
        "",
        "| Rule | ABI | Direction | State | Worlds | Rationale |",
        "|---|---|---|---|---|---|",
    ]
    for plan in model["test_plans"]:
        abi = abi_by_id[plan["abi"]]
        worlds = plan["required_worlds"] + plan["optional_worlds"]
        state = "Active" if plan["enabled"] else "Gated"
        rationale = (
            plan["description"] if plan["enabled"]
            else plan["disabled_reason"]
        )
        lines.append(
            f"| {plan['rule']} | `{abi['name']}` | {plan['kind']} | "
            f"{state} | {', '.join(worlds) or '-'} | {rationale} |"
        )
    lines.extend([
        "",
        "Every plan has a generated one-rule test entry point. Gated plans are",
        "not added to the runtime registry until their adapters and platform",
        "prerequisites support the stated rule coverage.",
        "",
    ])
    return "\n".join(lines)


def update_or_check(path: Path, content: str, check: bool) -> bool:
    """Write one generated file, or report whether it is current."""
    content = content.rstrip() + "\n"
    if check:
        try:
            current = path.read_text(encoding="utf-8")
        except FileNotFoundError:
            print(f"missing generated file: {path}", file=sys.stderr)
            return False
        if current != content:
            print(f"stale generated file: {path}", file=sys.stderr)
            return False
        return True

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")
    return True


def main() -> int:
    """Generate or verify all Alpha1 ABI framework artifacts."""
    firme_root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--catalog",
        type=Path,
        default=firme_root / "tools/config/abi/firme_1_0_alp4.json",
    )
    parser.add_argument(
        "--scenarios",
        type=Path,
        default=firme_root / "tools/config/abi/scenarios/alpha1",
    )
    parser.add_argument(
        "--header",
        type=Path,
        default=firme_root / "val/include/val_firme_abi_catalog.h",
    )
    parser.add_argument(
        "--source",
        type=Path,
        default=firme_root / "val/src/val_firme_abi_catalog.c",
    )
    parser.add_argument(
        "--wrapper-header",
        type=Path,
        default=firme_root / "val/include/val_firme_abi_generated.h",
    )
    parser.add_argument(
        "--wrapper-source",
        type=Path,
        default=firme_root / "val/src/val_firme_abi_generated.c",
    )
    parser.add_argument(
        "--registry",
        type=Path,
        default=firme_root / "val/include/val_firme_abi_registry.inc",
    )
    parser.add_argument(
        "--coverage",
        type=Path,
        default=firme_root / "docs/FIRME_Alpha1_ABI_Interface_Coverage.md",
    )
    parser.add_argument(
        "--test-root",
        type=Path,
        default=firme_root / "test",
    )
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    try:
        catalog = json.loads(args.catalog.read_text(encoding="utf-8"))
        scenarios = load_scenarios(args.scenarios)
        model = validate_model(catalog, scenarios)
        abi_by_id = {abi["id"]: abi for abi in model["abis"]}
        results = [
            update_or_check(args.header, generate_header(model), args.check),
            update_or_check(args.source, generate_source(model), args.check),
            update_or_check(
                args.wrapper_header,
                generate_wrapper_header(model),
                args.check,
            ),
            update_or_check(
                args.wrapper_source,
                generate_wrapper_source(model),
                args.check,
            ),
            update_or_check(
                args.registry, generate_registry(model), args.check
            ),
            update_or_check(
                args.coverage, generate_coverage(model), args.check
            ),
        ]
        for plan in model["test_plans"]:
            module = abi_by_id[plan["abi"]]["module"]
            test_path = (
                args.test_root / module / generated_test_filename(plan)
            )
            results.append(
                update_or_check(
                    test_path, generate_test_source(plan), args.check
                )
            )
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"FIRME ABI generator error: {error}", file=sys.stderr)
        return 1

    if not all(results):
        print(
            "run generate_firme_abi_catalog.py to refresh generated files",
            file=sys.stderr,
        )
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
