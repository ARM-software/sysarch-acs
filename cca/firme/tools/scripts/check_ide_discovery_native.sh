#!/usr/bin/env bash
#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
set -euo pipefail

firme_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
build_dir=$(mktemp -d "$firme_dir/build-ide-discovery.XXXXXX")
trap 'rm -rf -- "$build_dir"' EXIT
flags=(-std=c11 -O1 -g -Wall -Wextra -Werror -ffunction-sections -fdata-sections)
if [[ ${SANITIZE:-0} == 1 ]]; then
    flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi

"${CC:-cc}" "${flags[@]}" -DFIRME_ENABLE_CXL_ENDPOINT_PROBE=1 \
    -I"$firme_dir/val/include" -I"$firme_dir/val_common/include" \
    -I"$firme_dir/platform/pal_baremetal/targets/fvp/include" \
    "$firme_dir/test/ide/unit/pcie_namespace_test.c" \
    "$firme_dir/val/src/val_pcie_namespace.c" \
    "$firme_dir/val/src/val_pcie.c" "$firme_dir/val/host/val_host_doe.c" \
    -Wl,--gc-sections -o "$build_dir/pcie_namespace_test"
"$build_dir/pcie_namespace_test"

"${CC:-cc}" "${flags[@]}" \
    -I"$firme_dir/val/include" -I"$firme_dir/val_common/include" \
    -I"$firme_dir/platform/pal_baremetal/targets/fvp/include" \
    "$firme_dir/test/ide/unit/pcie_ecam_address_test.c" \
    "$firme_dir/val/src/val_pcie.c" \
    "$firme_dir/platform/pal_baremetal/targets/fvp/src/pal_pcie_ecam.c" \
    -Wl,--gc-sections -o "$build_dir/pcie_ecam_address_test"
"$build_dir/pcie_ecam_address_test"

"${CC:-cc}" "${flags[@]}" \
    -I"$firme_dir/test/ide/unit/include" \
    -I"$firme_dir/val/include" -I"$firme_dir/val_common/include" \
    -I"$firme_dir/platform/pal_baremetal/targets/fvp/include" \
    "$firme_dir/test/ide/unit/pal_cxl_cleanup_test.c" \
    -Wl,--gc-sections -o "$build_dir/pal_cxl_cleanup_test"
"$build_dir/pal_cxl_cleanup_test"
