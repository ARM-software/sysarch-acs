#!/usr/bin/env bash
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
set -euo pipefail

firme_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
test_binary=$(mktemp "${TMPDIR:-/tmp}/firme-ide-namespace-native.XXXXXX")
trap 'rm -f -- "$test_binary"' EXIT

flags=(-std=c11 -O1 -g -Wall -Wextra -Werror -ffunction-sections -fdata-sections)
if [[ ${SANITIZE:-0} == 1 ]]; then
    flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi

"${CC:-cc}" "${flags[@]}" -Wl,--gc-sections \
    -I"$firme_dir/test/ide/unit/include" \
    -I"$firme_dir/val/include" -I"$firme_dir/val_common/include" \
    -I"$firme_dir/platform/pal_baremetal/targets/fvp/include" \
    "$firme_dir/test/ide/unit/ide_namespace_probe_test.c" -o "$test_binary"
"$test_binary"
