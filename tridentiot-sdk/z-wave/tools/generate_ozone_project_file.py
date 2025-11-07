# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

import sys
import argparse
import re
from jinja2 import Template
from datetime import datetime
from pathlib import Path

def path_to_parent_refs(path: Path) -> str:
    # if path is just ".", return empty string
    if str(path) == ".":
        return ""

    # count the number of path parts (excluding the root if it's an absolute path)
    parts_count = len(path.parts)
    if path.is_absolute():
        parts_count -= 1  # Don't count the root directory for absolute paths

    # create a string with "../" for each part
    parent_refs = "/".join([".."] * parts_count)

    # make sure the string ends with a slash unless it's empty
    if parent_refs and not parent_refs.endswith("/"):
        parent_refs += "/"

    return parent_refs

def main(argv):
    parser = argparse.ArgumentParser(
                    prog='generate_ozone_project_file',
                    description='Generates an Ozone debugger project file',
                    epilog='')

    parser.add_argument('-e', '--elf-file', required=True, help="Path to the ELF file to use in the Ozone debugger project file.")
    parser.add_argument('-o', '--output-dir', required=True, help="Path to the directory where the generated Ozone debugger project file will be located.")
    parser.add_argument('-t', '--template', required=True, help="Path to the jinja template that will be used to generate the .jdebug file.")
    parser.add_argument('-d', '--device',     required=True, help="The TridentIot device's name.")
    parser.add_argument('-c', '--core',       required=True, help="The device's core name.")
    parser.add_argument('-f', '--freertos',   required=True, help="The FreeRTOS plugin.")
    args = parser.parse_args()

    _device = args.device
    _core = args.core
    freertos_plugin = args.freertos

    output_dir: Path = Path(args.output_dir)
    elf_path: Path = Path(args.elf_file).relative_to(output_dir)

    # get root dir for setting up jdebug paths
    container_root = Path.cwd().parts[1]

    # get relative project root - enables debug file to work inside and outside of sdk
    project_root = path_to_parent_refs(output_dir.relative_to(Path(f"/{container_root}/")))

    # get template file to hydrate
    templatefile = Path(args.template)

    # set output file name based on the elf file name
    outputfile_name = f"{str(elf_path.name).split('.elf')[0]}"
    outputfile = output_dir / Path(outputfile_name)
    outputfile = f"{str(outputfile) + '.jdebug'}"
    outputfile = re.sub('_[0-9a-fA-F]{8}_', '_', outputfile)

    # set the file generation date time similarly to how Segger does it
    current_datetime = datetime.now().strftime("%d %b %Y %H:%M")

    with open(templatefile) as file_:
        template = Template(file_.read())
        output = template.render(
            elf_file_path=elf_path,
            project_root=project_root,
            ozone_file_name=outputfile_name,
            container_root=container_root,
            device=_device,
            core=_core,
            freertos_plugin=freertos_plugin,
            date_time=current_datetime)

        print(f"Generating: {outputfile}")
        with open(outputfile, "w") as fh:
            fh.write(output)


if __name__ == "__main__":
    try:
         main(sys.argv[1:])
    except Exception as e:
        print(f"Failed to generate .jdebug file due to error: {e}")
