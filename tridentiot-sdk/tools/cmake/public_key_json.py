# SPDX-License-Identifier: LicenseRef-TridentMSLA
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
""" Generates json file containing the public key x, y tokens """
import sys
import getopt
import json
from pathlib import Path
from enum import Enum

def extract_key(key_data, is_x):
    """ Extracts the key component. """
    if is_x:
        key_part = key_data[27:59]
    else:
        key_part = key_data[59:]
    return key_part[::-1]

def bytes_to_hex(arr: bytes) -> str:
    """ Convert byte array to hex string. """
    return " ".join(f"0x{arr[0]:02X}" if i == 0 else f"{byte:02X}" for i, byte in enumerate(arr))

def write_to_json(file_path, key_x, key_y, encryption_key: bytes) -> None:
    """ Writes the x and y components of the public key to the JSON file. """
    data = {
        "TR_MFG_TOKEN_SIGNED_BOOTLOADER_KEY_X": {
            "value": bytes_to_hex(key_x),
            "value_type": "hex"
        },
        "TR_MFG_TOKEN_SIGNED_BOOTLOADER_KEY_Y": {
            "value": bytes_to_hex(key_y),
            "value_type": "hex"
        }
    }

    if len(encryption_key) > 0:
        data["TR_MFG_TOKEN_BOOTLOAD_AES_KEY"] = {
            "value": bytes_to_hex(encryption_key),
            "value_type": "hex"
        }

    with open(file_path, "w", encoding="utf-8") as json_file:
        json.dump(data, json_file, indent=4)

class EncryptionKeyParsingResult(Enum):
    """ Defines custom errors for parse_encryption_key() """
    SUCCESS = 0
    FILE_NOT_FOUND = 1
    INVALID_LENGTH = 2

def parse_encryption_key(encryption_key_file: Path) -> tuple[EncryptionKeyParsingResult, bytes]:
    """ Parses and validates the given encryption key. """
    hex_string = ""
    if encryption_key_file.is_file():
        with encryption_key_file.open(mode="r", encoding="utf-8") as file:
            hex_string = file.read()
    else:
        return EncryptionKeyParsingResult.FILE_NOT_FOUND, bytes()

    hex_string_stripped = hex_string.strip()
    if len(hex_string_stripped) != 32:
        return EncryptionKeyParsingResult.INVALID_LENGTH, bytes()

    encryption_key = bytearray.fromhex(hex_string_stripped)

    return EncryptionKeyParsingResult.SUCCESS, encryption_key


def print_help() -> None:
    """ Prints a help message. """
    print("public_key_json.py -i <public_key_file> -o <json_file> [-e <encryption_key_file>]")
    print("") # Newline
    print("Writes given key(s) to a JSON file in a format supported by elcap.")
    print("") # Newline
    print("<public_signing_key_file> the public signing key file in DER format")
    print("<encryption_key_file> the encryption key file in HEX format")
    print("<json_file> the output JSON file")

def main(argv):
    """ Main function that parses options and performs the conversion. """
    input_file = None
    encryption_key_file: Path = Path()
    output_file = None
    try:
        opts, _ = getopt.getopt(argv,"hb:i:o:e:",["help","input=","output=", "encryption-key="])
    except getopt.GetoptError:
        print_help()
        sys.exit(2)

    if len(opts) == 0:
        print("Error! No options given.")
        print("") # Newline
        print_help()
        sys.exit(3)

    for opt, arg in opts:
        if opt == '-h':
            print_help()
            sys.exit()
        elif opt in ("-i", "--input"):
            input_file = arg
        elif opt in ("-e", "--encryption-key"):
            encryption_key_file = Path(arg)
        elif opt in ("-o", "--output"):
            output_file = arg

    # Open the binary file for reading in binary mode
    with open(input_file,"rb") as readf:
        binary_data = readf.read()

    public_key_x = extract_key(binary_data, True)
    public_key_y = extract_key(binary_data, False)
    print("public_key x :" + bytes_to_hex(public_key_x))
    print("public_key y :" + bytes_to_hex(public_key_y))

    result, encryption_key = parse_encryption_key(encryption_key_file)

    if result == EncryptionKeyParsingResult.FILE_NOT_FOUND:
        print(
            f"Given encryption key ({encryption_key_file.absolute()}) is not a file."
            f" Ignore this if no encryption key was given."
        )
    elif result == EncryptionKeyParsingResult.INVALID_LENGTH:
        print(f"Given encryption key ({encryption_key_file.absolute()}) is invalid.")
        sys.exit(1)

    # Write to JSON file
    write_to_json(output_file, public_key_x, public_key_y, encryption_key)


if __name__== "__main__":
    main(sys.argv[1:])
