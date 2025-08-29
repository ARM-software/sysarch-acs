## @file
#  Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
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

#!/bin/bash

WORK_DIR=${PWD}/workspace
HOME_DIR=${PWD}

if [ ! -d workspace ]
then
    mkdir workspace
fi
cd ${WORK_DIR}
if [ ! -d arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-linux-gnu ]
then
    rm -f arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-linux-gnu.tar.xz
    wget https://developer.arm.com/-/media/Files/downloads/gnu/14.3.rel1/binrel/arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-linux-gnu.tar.xz
    tar -xf arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-linux-gnu.tar.xz
fi

if [ ! -d output ]
then
    mkdir output
fi

echo "Building MPAM ACS for UEFI"
if [ ! -d edk2 ]
then
    git clone https://github.com/tianocore/edk2.git
    pushd edk2
    git checkout 836942fbadb629050b866a8052e6af755bcdf623
    git submodule update --init --recursive
    popd
    git clone https://github.com/tianocore/edk2-libc edk2/edk2-libc
fi
cd ${WORK_DIR}/edk2
make -C BaseTools

rm -f ShellPkg/Application/sysarch-acs
rm -rf Build/Shell/DEBUG_GCC/*

ln -s ${HOME_DIR} ShellPkg/Application/sysarch-acs
export GCC_AARCH64_PREFIX=${WORK_DIR}/arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-
export PACKAGES_PATH=${WORK_DIR}/edk2/edk2-libc

source edksetup.sh
git apply ShellPkg/Application/sysarch-acs/patches/edk2_mpam.patch
source ShellPkg/Application/sysarch-acs/tools/scripts/acsbuild.sh mpam
cp Build/Shell/DEBUG_GCC/AARCH64/Mpam.efi ${WORK_DIR}/output

cd ${HOME_DIR}

echo "Mpam EFI Application is available here : ${WORK_DIR}/output/Mpam.efi"
