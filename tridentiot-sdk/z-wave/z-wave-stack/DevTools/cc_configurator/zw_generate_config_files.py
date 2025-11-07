#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: BSD-3-Clause
""" Generates C config files based on a YAML input file and templates. """
import os
import argparse
import sys
from pathlib import Path
import yaml
from jinja2 import Environment, FileSystemLoader

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='Z-Wave Command Class configuration to C converter.'
    )

    parser.add_argument(
        '--output-dir',
        required=True,
        help='Output directory to populate with generated source/header files.'
    )
    parser.add_argument(
        '--config-file',
        required=True,
        help='Absolute path to .yaml file.'
    )
    parser.add_argument(
        '--template-dir',
        required=True,
        help='Template directory.'
    )
    parser.add_argument(
        '--key',
        required=True,
        help='Configuration key'
    )

    args = parser.parse_args()

    config_file = Path(args.config_file)
    TEMPLATE_DIR = Path(args.template_dir)
    key = args.key

    # Create the output directory.
    output_dir = Path(args.output_dir)
    output_dir.mkdir(exist_ok=True)

    # Load the YAML file
    with config_file.open(encoding='utf-8') as fd:
        configuration = yaml.load(fd, Loader=yaml.SafeLoader)

    # Prepare the template environment
    file_loader = FileSystemLoader(TEMPLATE_DIR)
    env = Environment(loader=file_loader,
                        lstrip_blocks=True, trim_blocks=True)

    for template_file in os.listdir(TEMPLATE_DIR):
        print(template_file)

        template = env.get_template(template_file)

        if key in configuration:
            sub_key = list(configuration[key].keys())[0]
            print("sub_key: " + sub_key)
            config_data = template.render({
                sub_key: configuration[key][sub_key]
            })
        else:
            config_data = template.render()

        c_file = template_file.replace('.jinja', '')

        output_file = Path(args.output_dir + '/' + c_file)

        output_file.write_text(config_data, encoding='utf-8')

    sys.exit(0)
