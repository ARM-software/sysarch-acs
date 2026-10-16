#!/usr/bin/env bash
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
set -euo pipefail

firme_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
test_binary=$(mktemp "${TMPDIR:-/tmp}/firme-gpt-audit.XXXXXX")
trap 'rm -f "$test_binary"' EXIT

for fixture in gpt_fixture_audit_test gpt_not_supported_test; do
    if [[ "$fixture" == gpt_fixture_audit_test ]]; then
        support_source="$firme_root/test/gpt/native/gpt_frame_mock.c"
    else
        support_source="$firme_root/val/src/val_firme_gpt.c"
    fi
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter \
        -ffunction-sections -fdata-sections -Wl,--gc-sections \
        ${CFLAGS:-} \
        -I"$firme_root/val/include" \
        -I"$firme_root/val_common/include" \
        -I"$firme_root/platform/pal_baremetal/targets/fvp/include" \
        "$firme_root/test/gpt/native/$fixture.c" "$support_source" \
        -o "$test_binary"
    "$test_binary"
done
