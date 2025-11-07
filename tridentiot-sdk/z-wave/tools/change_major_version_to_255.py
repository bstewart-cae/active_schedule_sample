# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

"""
This script modifies the major version of the Z-Wave application.
"""
import sys

def modify_binary(file_path: str):
    """
    Opens the application binary file, finds the version structure,
    and then changes the major version number to 255.

    Args:
        file_path (string): The path to the application binary file.

    Returns:
       Nothing.
    """
    with open(file_path, "r+b") as bin_file:
        file_content = bin_file.read()
        # Search for the 'M_425' marker in the file
        marker = b"M_425"
        offset = file_content.find(marker)
        if offset == -1:
            print("Error: 'M_425' magic string not found in the file.")
            return

        print("Update the major version to 255")
        # Read the major version value after the marker
        value_offset = offset + len(marker)
        if value_offset + 1 > len(file_content):
            print("Error: File does not contain complete structure.")
            return

        # Modify the value
        temp_array = bytearray(file_content)
        temp_array[value_offset] = 0xFF
        # Write the modified value back to the file
        file_content = temp_array
        bin_file.seek(0)
        bin_file.write(file_content)
        print("File updated successfully.")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <ota file>")
    else:
        modify_binary(sys.argv[1])
