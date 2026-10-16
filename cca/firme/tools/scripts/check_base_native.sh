#!/usr/bin/env bash
#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
set -euo pipefail

firme_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
build_dir=$(mktemp -d "$firme_dir/build-base-native.XXXXXX")
trap 'rm -rf -- "$build_dir"' EXIT
flags=(-std=c11 -O1 -g -Wall -Wextra -Werror -Wdeclaration-after-statement
    -ffunction-sections -fdata-sections)
if [[ ${SANITIZE:-0} == 1 ]]; then
    flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi
"${CC:-cc}" "${flags[@]}" \
    -I"$firme_dir/val/include" -I"$firme_dir/val_common/include" \
    -I"$firme_dir/platform/pal_baremetal/targets/fvp/include" \
    "$firme_dir/test/base/native/base_coverage_test.c" \
    "$firme_dir/val/src/val_firme_abi.c" \
    "$firme_dir/val/src/val_firme_service.c" \
    "$firme_dir/val/src/val_firme_ide.c" \
    -Wl,--gc-sections -o "$build_dir/base_coverage_test"
"$build_dir/base_coverage_test"
