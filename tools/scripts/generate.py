## @file
 # Copyright (c) 2025-2026, Arm Limited or its affiliates. All rights reserved.
 # SPDX-License-Identifier : Apache-2.0
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 #  http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
 ##

import shutil
import json
import sys
import os

def func_config(functions, output_file):
    """Generate a .c file with stubs for a list of functions."""
    with open(output_file, "w", encoding="utf-8") as out_file:
        out_file.write("#include <stdio.h>\n\n")
        out_file.write("#include <stdint.h>\n\n")
        out_file.write("#include \"pal_common_support.h\"\n\n")
        out_file.write("#include \"pal_pcie_enum.h\"\n\n")
        out_file.write("#include \"platform_image_def.h\"\n\n")
        out_file.write("#include \"platform_override_struct.h\"\n\n")
        out_file.write("#include \"platform_override_fvp.h\"\n\n")

        for func in functions:
            ret_type = func["return_type"]
            name = func["function_name"]
            param = func.get("parameters", [])
            param_str = ", ".join(param)

            out_file.write(f"{ret_type} {name}({param_str})\n")
            out_file.write("{\n")
            out_file.write(f'    print(ACS_PRINT_ERR, "PAL API {name} not implemented \\n", 0);\n')
            if ret_type != "void":
                out_file.write("    return 0;\n")
            out_file.write("}\n\n")


def generate(platform_name):
    # Create folder with the <platform_name>
    acs_dir = os.getcwd()
    target_dir = os.path.join(acs_dir, "pal", "baremetal", "target")
    ref_dir = os.path.join(target_dir, "RDN2")
    platform_dir = os.path.join(target_dir, platform_name)
    src_dir = os.path.join(platform_dir, "src")
    os.makedirs(src_dir, exist_ok=True)

    # Copy the target.c and the header files
    src_inc_folder = os.path.join(ref_dir, "include")
    platform_inc_folder = os.path.join(platform_dir, "include")
    shutil.copytree(src_inc_folder, platform_inc_folder, dirs_exist_ok=True)
    config_file_path = os.path.join(ref_dir, "src", "platform_cfg_fvp.c")
    shutil.copy(config_file_path, src_dir)

    # Load functions.json
    pal_path = os.path.join(acs_dir, "tools", "scripts", "functions.json")
    with open(pal_path, "r", encoding="utf-8") as pal_funcs:
        data = json.load(pal_funcs)

    common = data.get("common", [])
    common_exerciser = data.get("common_exerciser", [])
    bsa = data.get("bsa", [])
    sbsa = data.get("sbsa", [])
    sbsa_exerciser = data.get("sbsa_exerciser", [])

    # Generate pal_bsa.c from common + bsa
    bsa_path = os.path.join(src_dir, "pal_bsa.c")
    func_config(common + bsa, bsa_path)

    # Generate pal_sbsa.c from sbsa
    sbsa_path = os.path.join(src_dir, "pal_sbsa.c")
    func_config(sbsa, sbsa_path)

    # Generate pal_exerciser.c from common_exerciser + sbsa_exerciser
    exer_path = os.path.join(src_dir, "pal_exerciser.c")
    func_config(common_exerciser + sbsa_exerciser, exer_path)
    print(f"Generated {bsa_path} and {sbsa_path} and {exer_path}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: ./generate.py <platform_name>")
        sys.exit(1)

    platform = sys.argv[1]
    generate(platform)

