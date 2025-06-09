import shutil
import json
import sys
import os

def func_config(functions, output_file):
    """Generate a .c file with stubs for a list of functions."""
    with open(output_file, "w") as out_file:
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
    with open(pal_path, "r") as f:
        data = json.load(f)

    common = data.get("common", [])
    bsa = data.get("bsa", [])
    sbsa = data.get("sbsa", [])

    # Generate pal_bsa.c from common + bsa
    bsa_path = os.path.join(src_dir, "pal_bsa.c")
    func_config(common + bsa, bsa_path)

    # Generate pal_sbsa.c from common + sbsa
    sbsa_path = os.path.join(src_dir, "pal_sbsa.c")
    func_config(sbsa, sbsa_path)

    print(f"Generated {bsa_path} and {sbsa_path}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: ./generate.py <platform_name>")
        sys.exit(1)

    platform = sys.argv[1]
    generate(platform)

