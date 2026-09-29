#!/usr/bin/env bash
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
set -euo pipefail

firme_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
test_binary=$(mktemp "${TMPDIR:-/tmp}/firme-dispatch-native.XXXXXX")
trap 'rm -f "$test_binary"' EXIT

for fixture in dispatch_contract_test abi_lifecycle_test; do
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter \
        -ffunction-sections -fdata-sections -Wl,--gc-sections \
        -DACS_PRINT_LEVEL=3 ${CFLAGS:-} \
        -I"$firme_root/val/include" \
        -I"$firme_root/val_common/include" \
        -I"$firme_root/platform/pal_baremetal/targets/fvp/include" \
        "$firme_root/test/framework/native/$fixture.c" \
        -o "$test_binary"
    "$test_binary"
done
