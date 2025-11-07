# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

"""Proper Intel HEX file merger that maintains format integrity."""

import argparse
from intel_hex_utils import IntelHexFile


def main(hex1_filename, hex2_filename, output_filename):
    """Main function to merge two Intel HEX files."""
    # Load first HEX file
    hex1 = IntelHexFile()
    hex1.load_from_file(hex1_filename)

    # Load second HEX file
    hex2 = IntelHexFile()
    hex2.load_from_file(hex2_filename)

    # Merge hex2 into hex1
    hex1.merge_with(hex2)

    # Save the result
    hex1.save_to_file(output_filename)


if __name__ == "__main__":
    # Set up argument parser
    parser = argparse.ArgumentParser(description="Merge two Intel HEX files")
    parser.add_argument("--hex1", required=True, help="Path to the first hex file")
    parser.add_argument("--hex2", required=True, help="Path to the second hex file")
    parser.add_argument("--output", required=True, help="Path to the output file where the result will be saved")

    # Parse arguments
    args = parser.parse_args()

    # Call the main function with the parsed arguments
    main(args.hex1, args.hex2, args.output)
