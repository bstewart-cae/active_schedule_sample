# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

# pylint: disable=line-too-long
import argparse
from collections import defaultdict
from elftools.elf.elffile import ELFFile

def parse_region(region_str):
    """
    Parses a region string in the format NAME:START-END.
    Example: 'FLASH:0x10008000-0x10099000'
    Returns a tuple: (name, start_address, end_address)
    """
    try:
        name, address_range = region_str.split(":")
        start_str, end_str = address_range.split("-")
        start = int(start_str, 0)  # Auto-detect hex or decimal
        end = int(end_str, 0)
        return (name, start, end)
    except Exception:
        raise argparse.ArgumentTypeError(
            f"Invalid region format: '{region_str}'. Use NAME:START-END (e.g. FLASH:0x10008000-0x10099000)"
        )

def analyze_elf(elf_path, output_path, regions):
    """
    Reads the ELF file and calculates memory usage for each region.
    Outputs section details and a usage summary, sorted by region name.
    """
    # Dictionary: {region_name: (start, end)}
    region_ranges = {}
    # Dictionary: {region_name: total_bytes_used}
    region_usage = {}
    # Dictionary: {region_name: [section_info_lines]}
    region_sections = defaultdict(list)

    # Store input regions
    for name, start, end in regions:
        region_ranges[name] = (start, end)
        region_usage[name] = 0

    # Open ELF file
    with open(elf_path, 'rb') as file:
        elf = ELFFile(file)

        # Iterate over ELF sections
        for section in elf.iter_sections():
            section_address = section['sh_addr']
            section_size = section['sh_size']
            section_name = section.name

            # Match section to a region
            for region_name in region_ranges:
                start, end = region_ranges[region_name]
                if start <= section_address < end:
                    # Update region usage
                    region_usage[region_name] += section_size

                    # Prepare section info line
                    line = f"  Section: {section_name:<42} Addr: 0x{section_address:08X}  Size: {section_size:<6} bytes"
                    region_sections[region_name].append(line)
                    break  # Only match the first region

    # Build output lines
    output_lines = ["Section Usage Details:\n"]

    # Sort by region name
    for region_name in sorted(region_sections.keys()):
        output_lines.append(f"[{region_name}]")
        output_lines.extend(region_sections[region_name])
        output_lines.append("")  # blank line between regions

    # Summary
    output_lines.append("Summary:\n")
    for region_name in sorted(region_usage.keys()):
        used = region_usage[region_name]
        start, end = region_ranges[region_name]
        total_size = end - start
        output_lines.append("")  # blank line between regions
        output_lines.append(f"{region_name} usage: {used} / {total_size} bytes")
        output_lines.append(f"{region_name} remaining: {total_size - used} bytes")


    # Write to output file and print
    with open(output_path, 'w') as out_file:
        for line in output_lines:
            print(line)
            out_file.write(line + '\n')

def main():
    """
    Handles argument parsing and runs the memory analyzer.
    """
    parser = argparse.ArgumentParser(
        description="Analyze ELF file memory usage and print RAM/Flash usage by region."
    )
    parser.add_argument('--elf', required=True, help="Path to ELF file")
    parser.add_argument('--out', required=True, help="Path to output report file")
    parser.add_argument('--region', action='append', required=True,
                        help="Region in format NAME:START-END (e.g. RAM:0x30000000-0x30030000)")

    args = parser.parse_args()

    try:
        regions = [parse_region(r) for r in args.region]
    except argparse.ArgumentTypeError as e:
        parser.error(str(e))

    analyze_elf(args.elf, args.out, regions)

if __name__ == '__main__':
    main()
