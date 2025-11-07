# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

import sys
from pathlib import Path
import subprocess
import merge_hexfiles
import argparse

"""
This script takes 2 binary files as inputs and combines them together. The resulting
combination will create a single .bin file of the pass in name and it will also create
a combined hex file of the same name if a start address is defined.

For example, passing in:
    -b bootload.bin -a app.bin -o combo.bin -s 0x0 -f 0x9000 -j path/to/objcopy
will create an output of: combo.bin AND combo.hex, but passing in:
    -b bootload.bin -a app.bin -o combo.bin
will only create an output of: combo.bin
"""

class MyCustomError(Exception):
    def __init__(self, message):
        super().__init__(message)

def align_to_32k_bytes(byte_array):
    # Calculate the padding size to reach the desired alignment
    padding_size = (32* 1024) - len(byte_array) % (32*2014)
    # Create a byte array with the padding
    padding = bytes([0xff] * padding_size)
    # Concatenate the original byte array with the padding
    aligned_byte_array = byte_array + padding
    return aligned_byte_array


def create_hex_file_from_bin(bin_file, hex_file, address, objcopy):
    cmd = [objcopy, '-I', 'binary', '-O', 'ihex', '--adjust-vma', address, bin_file, hex_file]

    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError:
        print("Failed to execute objcopy command")
        sys.exit(1)


def check_for_hex_file_gen(flash_start_address, app_start_address, objcopy_path):
    if flash_start_address is None or app_start_address is None:
        print("Cannot create hex file without --flash_start_address or --app_start_address being defined")
        sys.exit(1)
    if objcopy_path is None:
        print("Cannot create hex file without --objcopy being defined")
        sys.exit(1)
    elif not Path(objcopy_path).exists():
        print(f"Unable to locate {objcopy_path}, failed to create combo hex file")
        sys.exit(1)


def main(bootloader_file = None,
         app_file = None,
         output_file = None,
         flash_start_address = None,
         app_start_address = None,
         objcopy_path = None
         ):

    bootloader_data = None
    app_data = None

    # fail gracefully if missing an input file, this is important for zigbee apps
    if not Path(bootloader_file).exists():
        print(f"{Path(bootloader_file).name} does not exist, skipping combination step")
        return

    if not Path(app_file).exists():
        print(f"{Path(app_file).name} does not exist, skipping combination step")
        return

    # if hex file options are included make sure we can execute hex file creation
    check_for_hex_file_gen(flash_start_address, app_start_address, objcopy_path)

    try:
        # Open the bootloader file for reading in binary mode
        bootloader_f = open(bootloader_file,"rb")
        bootloader_data = bootloader_f.read()
        bootloader_f.close()
        if (bootloader_data == None) or (len(bootloader_data) == 0):
            raise MyCustomError("Invalid bootlaoder_binary file")
    except Exception as e:
        print(f"An error occurred: {str(e)}")

    try:
    # Open the application file for reading in binary mode
        app_f = open(app_file,"rb")
        app_data = app_f.read()
        app_f.close()

        if (app_data == None) or (len(app_data) == 0):
            raise MyCustomError("Invalid bootlaoder_binary file")
    except FileNotFoundError:
        print(f"Error: The file '{app_file}' does not exist.")
    except Exception as e:
        print(f"An error occurred: {str(e)}")

    try:
        writef = open(output_file,"wb")
        # padd the bootlaoder file up to 32k
        output_data = align_to_32k_bytes(bootloader_data)
        # add the bootloader data to the app data
        output_data = output_data + app_data

        writef.write(output_data)
        writef.close()

        # if hex file options exist, create the hex file as well
        if flash_start_address and app_start_address and objcopy_path:
            # create each hex file separately first
            bootload_hex = Path(bootloader_file).parent / Path(f"{Path(bootloader_file).stem}.hex")
            create_hex_file_from_bin(bootloader_file, bootload_hex, flash_start_address, objcopy_path)

            app_hex = Path(app_file).parent / Path(f"{Path(app_file).stem}.hex")
            create_hex_file_from_bin(app_file, app_hex, app_start_address, objcopy_path)

            # then combine them
            output_hex = Path(output_file).parent / Path(f"{Path(output_file).stem}.hex")
            merge_hexfiles.main(bootload_hex, app_hex, output_hex)
            print(f"{Path(output_hex).name} created successfully")

    except FileNotFoundError:
        print(f"Error: The file '{output_file}' does not exist.")
    except Exception as e:
        print(f"An error occurred: {str(e)}")

if __name__== "__main__":
    parser = argparse.ArgumentParser(description='Combine 2 binary files with optional hex output')

    parser.add_argument('-b', '--bootloader', help='bootloader binary file')
    parser.add_argument('-a', '--app', help='application binary file')
    parser.add_argument('-o', '--output', help='combined output binary file ')
    parser.add_argument('-f', '--flash_start_address', help='start address of the flash bank. Must define to enable hex file generation')
    parser.add_argument('-s', '--app_start_address', help='start address of the application. Must define to enable hex file generation')
    parser.add_argument('-j', '--objcopy', help='path/to/objcopy tool used to create a hex file. Must define to enable hex file generation')

    args = parser.parse_args()

    main(bootloader_file=args.bootloader,
         app_file=args.app,
         output_file=args.output,
         flash_start_address=args.flash_start_address,
         app_start_address=args.app_start_address,
         objcopy_path=args.objcopy)
