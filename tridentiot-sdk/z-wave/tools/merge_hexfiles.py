# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

# pylint: disable=line-too-long
"""This script merges 2 hex files."""

import argparse

def read_hex_file(filename):
    """Function to read a hex file."""
    with open(filename, 'rt', encoding="utf-8") as file:
        lines = file.readlines()
        file.close()
        return lines

def write_hex_file(filename, lines):
    """Function to write a hex file."""
    with open(filename, 'wt', encoding="utf-8") as file:
        file.writelines(lines)

def insert_hex2_into_hex1(hex1_lines, hex2_lines):
    """Function to merge hex2 into hex1."""
    # Remove the last 2 lines from hex2
    modified_hex2 = hex2_lines[:-2]

    # Insert the modified hex2 into hex1 before the last 2 lines
    hex1_lines[-2:-2] = modified_hex2

    return hex1_lines

def main(hex1_filename, hex2_filename, output_filename):
    """Main function."""
    # Read the contents of both hex files
    hex1_lines = read_hex_file(hex1_filename)
    hex2_lines = read_hex_file(hex2_filename)

    # Insert modified hex2 into hex1
    modified_hex1 = insert_hex2_into_hex1(hex1_lines, hex2_lines)

    # Write the modified content to the output file
    write_hex_file(output_filename, modified_hex1)

if __name__ == "__main__":
    # Set up argument parser
    parser = argparse.ArgumentParser(description="Merge two hex files by inserting the hex2 into hex1 before the last two lines.")
    parser.add_argument("--hex1", required=True, help="Path to the first hex file.")
    parser.add_argument("--hex2", required=True, help="Path to the second hex file.")
    parser.add_argument("--output", required=True, help="Path to the output file where the result will be saved.")

    # Parse arguments
    args = parser.parse_args()

    # Call the main function with the parsed arguments
    main(args.hex1, args.hex2, args.output)
