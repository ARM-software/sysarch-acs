## @file
#  Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
#  SPDX-License-Identifier : Apache-2.0
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
##

if [ $(uname -m) != "aarch64" ] && [ -v $GCC49_AARCH64_PREFIX ]
then
    echo "GCC49_AARCH64_PREFIX is not set"
    echo "set using export GCC49_AARCH64_PREFIX=<lib_path>/bin/aarch64-linux-gnu-"
    return 0
fi

# Get the path of the current shell script. Based on the script path navigate to sysarch-acs path
script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

acs_path="$(dirname "$(dirname "$script_dir")")"
echo "sysarch-acs path is: $(realpath "$acs_path")"

export ACS_PATH=$(realpath "$acs_path")

if [ "$#" -ne 1 ]; then
    echo "Usage: source ShellPkg/Application/sysarch-acs/tools/scripts/acsbuild.sh <acs_type>"
    echo "where acs_type is unified, bsa, bsa_dt, sbsa, nist, mpam, drtm, mem_test"
    return 1;
fi

NISTStatus=1;

function build_with_NIST()
{
    if [ ! -f "sts-2_1_2.zip" ]; then
        wget https://csrc.nist.gov/CSRC/media/Projects/Random-Bit-Generation/documents/sts-2_1_2.zip
        status=$?
        if [ $status -ne 0 ]; then
            echo "wget failed for NIST."
            return $status
        fi
    fi

    if [ ! -d "ShellPkg/Application/sysarch-acs/test_pool/nist_sts/sts-2.1.2/" ]; then
        /usr/bin/unzip sts-2_1_2.zip -d ShellPkg/Application/sysarch-acs/test_pool/nist_sts/.
	status=$?
        if [ $status -ne 0 ]; then
            echo "unzip failed for NIST."
            return $status
        fi
    fi

    cd ShellPkg/Application/sysarch-acs/test_pool/nist_sts/sts-2.1.2/
    if ! patch -R -p1 -s -f --dry-run < ../../../patches/nist_sbsa_sts.diff; then
        patch -p1 < ../../../patches/nist_sbsa_sts.diff
        status=$?
	if [ $status -ne 0 ]; then
            echo "patch failed for NIST."
            return $status
        fi
    fi
    cd -

    git apply ShellPkg/Application/sysarch-acs/patches/edk2_sbsa_nist.patch
    build -a AARCH64 -t GCC49 -p ShellPkg/ShellPkg.dsc -m ShellPkg/Application/sysarch-acs/apps/uefi/SbsaNist.inf -D ENABLE_NIST -D SBSA
    status=$?
    if [ $status -ne 0 ]; then
        echo "Build failed for NIST."
        return $status
    fi

    return $status
}


if [ "$1" == "nist" ]; then
    build_with_NIST
    NISTStatus=$?
    return 0;
fi

if [ "$1" == "bsa" ]; then
    git checkout ShellPkg/ShellPkg.dsc 
    git apply ShellPkg/Application/sysarch-acs/patches/edk2_bsa_acpi.patch
    build -a AARCH64 -t GCC49 -p ShellPkg/ShellPkg.dsc -m ShellPkg/Application/sysarch-acs/apps/uefi/Bsa.inf
    return 0;
fi

if [ "$1" == "bsa_dt" ]; then
    git checkout ShellPkg/ShellPkg.dsc 
    git apply ShellPkg/Application/sysarch-acs/patches/edk2_bsa_dt.patch
    build -a AARCH64 -t GCC49 -p ShellPkg/ShellPkg.dsc -m ShellPkg/Application/sysarch-acs/apps/uefi/Bsa.inf
    return 0;
fi

if [ "$1" == "sbsa" ]; then
    git checkout ShellPkg/ShellPkg.dsc 
    git apply ShellPkg/Application/sysarch-acs/patches/edk2_sbsa.patch
    build -a AARCH64 -t GCC49 -p ShellPkg/ShellPkg.dsc -m ShellPkg/Application/sysarch-acs/apps/uefi/Sbsa.inf
    return 0;
fi

if [ "$1" == "drtm" ]; then
    git checkout ShellPkg/ShellPkg.dsc 
    git apply ShellPkg/Application/sysarch-acs/patches/edk2_drtm.patch
    build -a AARCH64 -t GCC49 -p ShellPkg/ShellPkg.dsc -m ShellPkg/Application/sysarch-acs/apps/uefi/Drtm.inf
    return 0;
fi

if [ "$1" == "mem_test" ]; then
    git checkout ShellPkg/ShellPkg.dsc 
    git apply ShellPkg/Application/sysarch-acs/mem_test/patches/mem_test_edk2.patch
    build -a AARCH64 -t GCC49 -n 1 -p ShellPkg/ShellPkg.dsc -m ShellPkg/Application/sysarch-acs/apps/uefi/Mem.inf
    return 0;
fi

if [ "$1" == "mpam" ]; then
    git checkout ShellPkg/ShellPkg.dsc 
    git apply ShellPkg/Application/sysarch-acs/patches/edk2_mpam.patch
    build -a AARCH64 -t GCC49 -p ShellPkg/ShellPkg.dsc -m ShellPkg/Application/sysarch-acs/apps/uefi/Mpam.inf
    return 0;
fi

if [ "$1" == "unified" ]; then
    git checkout ShellPkg/ShellPkg.dsc 
    git apply ShellPkg/Application/sysarch-acs/patches/edk2_unified_acpi.patch
    build -a AARCH64 -t GCC49 -p ShellPkg/ShellPkg.dsc -m ShellPkg/Application/sysarch-acs/apps/uefi/Unified.inf
    return 0;
fi
