# SPDX-License-Identifier: LicenseRef-TridentMSLA
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
import sys
import argparse
from datetime import datetime
from jinja2 import Template
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

def main():
    parser = argparse.ArgumentParser(
                    prog='gen_ozone_project',
                    description='Generates an Ozone debugger project file',
                    epilog='')

    parser.add_argument('-e', '--elf-file', required=True, help="Path to the ELF file to use in the Ozone debugger project file.")
    parser.add_argument('-o', '--output-dir', required=True, help="Path to the directory where the generated Ozone debugger project file will be located.")
    parser.add_argument('-t', '--template', required=True, help="Path to the jinja template that will be used to generate the .jdebug file.")
    args = parser.parse_args()

    output_dir: Path = Path(args.output_dir)
    elf_path: Path = Path(args.elf_file).relative_to(output_dir)

    # get root dir for setting up jdebug paths
    container_root = Path.cwd().parts[1]

    # get relative project root - enables debug file to work inside and outside of sdk
    project_root = path_to_parent_refs(output_dir.relative_to(Path(f"/{container_root}/")))

    # get template file to hydrate
    templatefile = Path(args.template)

    # set output file name based on the elf file name
    outputfile_name = f"{str(elf_path.name).split(".")[0]}.jdebug"
    outputfile = output_dir / Path(outputfile_name)
    
    # set the file generation date time similarly to how Segger does it
    current_datetime = datetime.now().strftime("%d %b %Y %H:%M")

    with open(templatefile) as file_:
        template = Template(file_.read())
        output = template.render(
            elf_file_path=elf_path,
            project_root=project_root,
            ozone_file_name=outputfile_name,
            container_root=container_root,
            date_time=current_datetime)

        print(f"Generating: {outputfile}")
        with open(outputfile, "w") as fh:
            fh.write(output)


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"Failed to generate .jdebug file due to error: {e}")
