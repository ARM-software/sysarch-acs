#!/usr/bin/env bash
#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
set -euo pipefail

firme_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
build_dir=$(mktemp -d "$firme_dir/build-ide-rules.XXXXXX")
trap 'rm -rf -- "$build_dir"' EXIT
flags=(-std=c11 -O1 -g -Wall -Wextra -Werror -ffunction-sections -fdata-sections)
if [[ ${SANITIZE:-0} == 1 ]]; then
    flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi
"${CC:-cc}" "${flags[@]}" \
    -I"$firme_dir/val/include" -I"$firme_dir/val_common/include" \
    -I"$firme_dir/platform/pal_baremetal/targets/fvp/include" \
    "$firme_dir/test/ide/unit/pcie_ide_stream_test.c" \
    "$firme_dir/val/src/val_pcie.c" "$firme_dir/val/src/val_pcie_ide.c" \
    -Wl,--gc-sections -o "$build_dir/pcie_ide_stream_test"
"$build_dir/pcie_ide_stream_test"
"${CC:-cc}" "${flags[@]}" \
    -I"$firme_dir/val/include" -I"$firme_dir/val_common/include" \
    -I"$firme_dir/platform/pal_baremetal/targets/fvp/include" \
    "$firme_dir/test/ide/unit/ide_rule_test.c" \
    "$firme_dir/val/src/val_firme_ide.c" \
    "$firme_dir/val/src/val_firme_ide_observation.c" \
    -Wl,--gc-sections -o "$build_dir/ide_rule_test"
"$build_dir/ide_rule_test"
