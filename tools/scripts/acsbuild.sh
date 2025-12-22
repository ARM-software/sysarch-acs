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

if [ "$1" != "mem_test" ] && [ $(uname -m) != "aarch64" ] && [ -v $GCC_AARCH64_PREFIX ]
then
    echo "GCC_AARCH64_PREFIX is not set"
    echo "set using export GCC_AARCH64_PREFIX=<lib_path>/bin/aarch64-linux-gnu-"
    return 0
fi

#GGC49 prefix check for mem_test build
if [ "$1" == "mem_test" ] && [ $(uname -m) != "aarch64" ] && [ -v $GCCNOLTO_AARCH64_PREFIX ]
then
    echo "GCCNOLTO_AARCH64_PREFIX is not set"
    echo "set using export GCCNOLTO_AARCH64_PREFIX=<lib_path>/bin/aarch64-linux-gnu-"
    return 0
fi

# Get the path of the current shell script. Based on the script path navigate to sysarch-acs path
script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

acs_path="$(dirname "$(dirname "$script_dir")")"
echo "sysarch-acs path is: $(realpath "$acs_path")"

export ACS_PATH=$(realpath "$acs_path")

ACS_TYPE="$1"
VENDOR="$2"

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    echo "Usage: source ShellPkg/Application/sysarch-acs/tools/scripts/acsbuild.sh <acs_type> [vendor]"
    echo ""
    echo "Public builds (no vendor):"
    echo "  acsbuild.sh xbsa_acpi"
    echo "  acsbuild.sh bsa"
    echo "  acsbuild.sh sbsa"
    echo "  acsbuild.sh bsa_dt"
    echo "  acsbuild.sh nist"
    echo "  acsbuild.sh mpam"
    echo "  acsbuild.sh drtm"
    echo "  acsbuild.sh mem_test"
    echo "  acsbuild.sh pfdi"
    echo ""
    echo "Vendor builds:"
    echo "  acsbuild.sh xbsa_acpi nvidia"
    echo "  acsbuild.sh bsa nvidia"
    echo "  acsbuild.sh sbsa nvidia"
    return 1;
fi

# Helper function to build with vendor support
build_with_vendor() {
    local acs_type="$1"
    local vendor="$2"
    local patch_file="$3"
    local public_inf="$4"

    if [ -n "$vendor" ]; then
        # Vendor build - verify sync first
        echo "========================================"
        echo "Building $acs_type with $vendor vendor tests"
        echo "========================================"

        local verify_script="ShellPkg/Application/sysarch-acs/apps/uefi/vendor/verify_vendor_inf.sh"
        if [ ! -f "$verify_script" ]; then
            echo "ERROR: Vendor verification script not found: $verify_script"
            return 1
        fi

        bash "$verify_script" "$vendor"
        if [ $? -ne 0 ]; then
            echo "ERROR: Vendor INF sync check failed. Please update vendor INFs."
            return 1
        fi

        # Determine vendor INF path
        local vendor_inf_name
        case "$acs_type" in
            xbsa_acpi) vendor_inf_name="XbsaAcpi${vendor^}.inf" ;;
            bsa)       vendor_inf_name="Bsa${vendor^}.inf" ;;
            sbsa)      vendor_inf_name="Sbsa${vendor^}.inf" ;;
            *)
                echo "ERROR: Vendor builds not supported for $acs_type"
                return 1
                ;;
        esac

        local vendor_inf="ShellPkg/Application/sysarch-acs/apps/uefi/vendor/$vendor/$vendor_inf_name"
        if [ ! -f "$vendor_inf" ]; then
            echo "ERROR: Vendor INF not found: $vendor_inf"
            return 1
        fi

        git checkout ShellPkg/ShellPkg.dsc
        git apply "ShellPkg/Application/sysarch-acs/patches/$patch_file"
        build -a AARCH64 -t GCC -p ShellPkg/ShellPkg.dsc -m "$vendor_inf"
    else
        # Public build
        echo "========================================"
        echo "Building $acs_type (public)"
        echo "========================================"

        git checkout ShellPkg/ShellPkg.dsc
        git apply "ShellPkg/Application/sysarch-acs/patches/$patch_file"
        build -a AARCH64 -t GCC -p ShellPkg/ShellPkg.dsc -m "ShellPkg/Application/sysarch-acs/apps/uefi/$public_inf"
    fi
}

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


    git checkout ShellPkg/ShellPkg.dsc
    cd edk2-libc/
    git checkout StdLib/LibC/Main/Arm/flt_rounds.c
    git checkout StdLib/LibC/Main/Main.c
    cd ../
    git apply ShellPkg/Application/sysarch-acs/patches/edk2_sbsa_nist.patch
    build -a AARCH64 -t GCC -p ShellPkg/ShellPkg.dsc -m ShellPkg/Application/sysarch-acs/apps/uefi/SbsaNist.inf -D ENABLE_NIST -D SBSA
    status=$?
    if [ $status -ne 0 ]; then
        echo "Build failed for NIST."
        return $status
    fi

    return $status
}


if [ "$ACS_TYPE" == "nist" ]; then
    build_with_NIST
    NISTStatus=$?
    return 0;
fi

if [ "$ACS_TYPE" == "bsa" ]; then
    build_with_vendor "bsa" "$VENDOR" "edk2_bsa_acpi.patch" "Bsa.inf"
    return 0;
fi

if [ "$ACS_TYPE" == "bsa_dt" ]; then
    git checkout ShellPkg/ShellPkg.dsc
    git checkout MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.c
    git apply ShellPkg/Application/sysarch-acs/patches/edk2_bsa_dt.patch
    build -a AARCH64 -t GCC -p ShellPkg/ShellPkg.dsc -m ShellPkg/Application/sysarch-acs/apps/uefi/Bsa.inf
    return 0;
fi

if [ "$ACS_TYPE" == "sbsa" ]; then
    build_with_vendor "sbsa" "$VENDOR" "edk2_sbsa.patch" "Sbsa.inf"
    return 0;
fi


if [ "$ACS_TYPE" == "pc_bsa" ]; then
    git checkout ShellPkg/ShellPkg.dsc
    git apply ShellPkg/Application/sysarch-acs/patches/edk2_pcbsa.patch
    build -a AARCH64 -t GCC -p ShellPkg/ShellPkg.dsc -m ShellPkg/Application/sysarch-acs/apps/uefi/pc_bsa.inf
    return 0;
fi

if [ "$ACS_TYPE" == "vbsa" ]; then
    git checkout ShellPkg/ShellPkg.dsc
    git apply ShellPkg/Application/sysarch-acs/patches/edk2_vbsa.patch
    build -a AARCH64 -t GCC -p ShellPkg/ShellPkg.dsc -m ShellPkg/Application/sysarch-acs/apps/uefi/vbsa.inf
    return 0;
fi

if [ "$ACS_TYPE" == "drtm" ]; then
    git checkout ShellPkg/ShellPkg.dsc
    git apply ShellPkg/Application/sysarch-acs/patches/edk2_drtm.patch
    build -a AARCH64 -t GCC -p ShellPkg/ShellPkg.dsc -m ShellPkg/Application/sysarch-acs/apps/uefi/Drtm.inf
    return 0;
fi

if [ "$ACS_TYPE" == "pfdi" ]; then
    git checkout ShellPkg/ShellPkg.dsc
    git checkout MdePkg/Library/UefiMemoryAllocationLib/MemoryAllocationLib.c
    git checkout MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.c
    git apply ShellPkg/Application/sysarch-acs/patches/edk2_pfdi.patch
    build -a AARCH64 -t GCC -p ShellPkg/ShellPkg.dsc -m ShellPkg/Application/sysarch-acs/apps/uefi/Pfdi.inf
    return 0;
fi


if [ "$ACS_TYPE" == "mem_test" ]; then
    git checkout ShellPkg/ShellPkg.dsc
    git apply ShellPkg/Application/sysarch-acs/mem_test/patches/mem_test_edk2.patch
    build -a AARCH64 -t GCCNOLTO -n 1 -p ShellPkg/ShellPkg.dsc -m ShellPkg/Application/sysarch-acs/apps/uefi/Mem.inf
    return 0;
fi

if [ "$ACS_TYPE" == "mpam" ]; then
    git checkout ShellPkg/ShellPkg.dsc
    git apply ShellPkg/Application/sysarch-acs/patches/edk2_mpam.patch
    build -a AARCH64 -t GCC -p ShellPkg/ShellPkg.dsc -m ShellPkg/Application/sysarch-acs/apps/uefi/Mpam.inf
    return 0;
fi

if [ "$ACS_TYPE" == "xbsa_acpi" ]; then
    build_with_vendor "xbsa_acpi" "$VENDOR" "edk2_xbsa_acpi.patch" "xbsa_acpi.inf"
    return 0;
fi

echo "ERROR: Unknown acs_type: $ACS_TYPE"
return 1;
