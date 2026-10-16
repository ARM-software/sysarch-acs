#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

"""Native Realm PAT setup/crypto/lease checks; not live FIRME qualification."""

import os
from pathlib import Path
import shlex
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
MBEDTLS = ROOT / "ext/libspdm/os_stub/mbedtlslib/mbedtls"
CRYPTO_SOURCES = (
    "bignum", "bignum_core", "bignum_mod", "bignum_mod_raw", "constant_time",
    "ecp", "ecp_curves", "ecp_curves_new", "memory_buffer_alloc", "platform",
    "platform_util", "sha256",
)


def main():
    """Build real pinned crypto and inject only setup SMC/entropy/mapping responses."""
    compiler = shlex.split(os.environ.get("CC", "cc"))
    flags = shlex.split(os.environ.get("CFLAGS", ""))
    includes = [
        ROOT / "val/realm", ROOT / "val/include", ROOT / "val_common/include",
        ROOT / "platform/pal_baremetal/targets/fvp/include", ROOT / "tools/config",
        MBEDTLS / "include",
    ]
    with tempfile.TemporaryDirectory(prefix="firme-realm-pat-") as directory:
        for mode, defines, scenarios in (
            ("disabled", [], ["disabled"]),
            ("enabled", ["-DFIRME_REALM_PAT_DELEGATED_LEGACY=1"],
             ["normal", "pending", "positive-status", "unexpected-negative"]),
            ("tiny-heap", ["-DFIRME_REALM_PAT_DELEGATED_LEGACY=1",
                           "-DVAL_REALM_PAT_HEAP_BYTES=256"], ["tiny-heap"]),
        ):
            binary = Path(directory) / mode
            sources = [ROOT / "tools/scripts/check_realm_pat_binding.c",
                       ROOT / "val/realm/val_realm_shared_buffer.c"]
            if defines:
                sources.extend(MBEDTLS / "library" / (name + ".c")
                               for name in CRYPTO_SOURCES)
            command = compiler + flags + ["-std=c11", "-Wall", "-Wextra", "-Werror",
                                          "-ffunction-sections", "-fdata-sections",
                                          "-Wl,--gc-sections"]
            command += ['-DMBEDTLS_CONFIG_FILE="val_realm_pat_mbedtls_config.h"']
            command += defines + [f"-I{path}" for path in includes]
            command += [str(path) for path in sources] + ["-o", str(binary)]
            subprocess.run(command, check=True)
            for scenario in scenarios:
                subprocess.run([str(binary), scenario], check=True)
                print(f"Realm PAT binding: {mode}/{scenario} PASS")


if __name__ == "__main__":
    main()
