#!/usr/bin/env bash
#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
set -euo pipefail
ulimit -c 0

firme_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
unit_dir="$firme_dir/test/ide/unit"
build_dir=$(mktemp -d "$firme_dir/build-ide-native.XXXXXX")
trap 'rm -rf -- "$build_dir"' EXIT

compiler=${CC:-cc}
flags=(
    -std=c11 -O1 -g -Wall -Wextra -Werror -ffunction-sections -fdata-sections
    -I"$firme_dir/val/include" -I"$firme_dir/val_common/include"
    -I"$firme_dir/platform/pal_baremetal/targets/fvp/include"
)
link_flags=(-Wl,--gc-sections)
spdm_flags=()
if [[ ${SANITIZE:-0} == 1 ]]; then
    flags+=(
        -fsanitize=address,undefined -fno-omit-frame-pointer
        -fno-sanitize-recover=all
    )
    spdm_flags+=(-fno-sanitize=shift)
fi

"$compiler" "${flags[@]}" "$unit_dir/pcie_bounds_test.c" \
    "$firme_dir/val/src/val_pcie.c" "${link_flags[@]}" \
    -o "$build_dir/pcie_bounds_test"
"$compiler" "${flags[@]}" "$unit_dir/ide_completion_test.c" \
    "$firme_dir/val/src/val_firme_ide.c" \
    "$firme_dir/platform/pal_baremetal/targets/fvp/src/pal_firme_ide_abi.c" \
    "${link_flags[@]}" -o "$build_dir/ide_completion_test"
"$compiler" "${flags[@]}" "${spdm_flags[@]}" -DFIRME_ENABLE_CXL_SPDM=1 \
    '-DLIBSPDM_CONFIG=<val_host_spdm_config.h>' \
    -I"$unit_dir/include" -I"$firme_dir/tools/config" \
    -I"$firme_dir/ext/libspdm/include" -I"$firme_dir/ext/spdm-emu/include" \
    "$unit_dir/spdm_buffer_test.c" "${link_flags[@]}" \
    -o "$build_dir/spdm_buffer_test"

status=0
"$build_dir/pcie_bounds_test" || status=1
"$build_dir/ide_completion_test" || status=1
"$build_dir/spdm_buffer_test" || status=1
exit "$status"
