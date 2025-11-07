# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA
#!/usr/bin/env python3
"""
Intel HEX File Patcher with Token Support

This script patches an Intel HEX file by inserting data at a specified flash address
or at a token location defined in a JSON token definition file.
It properly handles the HEX file format including checksums and record types.
"""

import sys
import argparse
import tempfile
import shutil
import os
import json
from typing import Optional, Dict, Tuple
from pathlib import Path

from intel_hex_utils import IntelHexFile


def load_token_definitions(token_file: str) -> Dict[str, Dict[str, str]]:
    """Load token definitions from JSON file."""
    try:
        with open(token_file, 'r') as f:
            tokens = json.load(f)

        # Validate token format
        for token_name, token_info in tokens.items():
            if not isinstance(token_info, dict):
                raise ValueError(f"Token '{token_name}' must be a dictionary")
            if 'address' not in token_info:
                raise ValueError(f"Token '{token_name}' missing 'address' field")
            if 'size' not in token_info:
                raise ValueError(f"Token '{token_name}' missing 'size' field")

        return tokens
    except json.JSONDecodeError as e:
        raise ValueError(f"Invalid JSON in token definition file: {e}")
    except FileNotFoundError:
        raise FileNotFoundError(f"Token definition file not found: {token_file}")


def load_token_data(token_data_file: str) -> Dict[str, Dict[str, str]]:
    """Load token data from JSON file."""
    try:
        with open(token_data_file, 'r') as f:
            token_data = json.load(f)

        # Validate token data format
        for token_name, token_info in token_data.items():
            if not isinstance(token_info, dict):
                raise ValueError(f"Token data '{token_name}' must be a dictionary")
            if 'value' not in token_info:
                raise ValueError(f"Token data '{token_name}' missing 'value' field")
            if 'value_type' not in token_info:
                raise ValueError(f"Token data '{token_name}' missing 'value_type' field")

        return token_data
    except json.JSONDecodeError as e:
        raise ValueError(f"Invalid JSON in token data file: {e}")
    except FileNotFoundError:
        raise FileNotFoundError(f"Token data file not found: {token_data_file}")


def get_token_info(tokens: Dict[str, Dict[str, str]], token_name: str) -> Tuple[int, int]:
    """Get address and size for a token name."""
    if token_name not in tokens:
        raise ValueError(f"Token '{token_name}' not found.")

    token_info = tokens[token_name]

    # Parse address (supports hex format)
    address = int(token_info['address'], 16)
    size = int(token_info['size'], 16)

    return address, size


def parse_token_value(value: str, value_type: str, token_name: str) -> bytes:
    """Parse token value based on its type."""
    try:
        if value_type == "hex":
            # Remove spaces, 0x prefixes, and convert to bytes
            hex_string = value.replace(' ', '').replace('0x', '')
            if len(hex_string) % 2 != 0:
                raise ValueError("Hex string must have even number of characters")
            return bytes.fromhex(hex_string)

        elif value_type == "int":
            # Convert integer to single byte (assuming 8-bit values)
            int_val = int(value)
            if int_val < 0 or int_val > 255:
                raise ValueError("Integer value must be between 0 and 255")
            return bytes([int_val])

        elif value_type == "ascii":
            # Convert ASCII string to bytes (null-terminated if needed)
            return value.encode('ascii')

        else:
            raise ValueError(f"Unsupported value_type: {value_type}")

    except ValueError as e:
        raise ValueError(f"Error parsing token '{token_name}' value '{value}' as {value_type}: {e}")


def validate_data_size(data: bytes, token_size: int, token_name: str) -> None:
    """Validate that data does not exceed the token size."""
    if len(data) > token_size:
        raise ValueError(f"Data size mismatch for token '{token_name}': "
                        f"Max size is {token_size} bytes, got {len(data)} bytes")


def patch_hex_file(input_file: str, output_file: str, flash_address: int, patch_data: bytes,
                  token_name: Optional[str] = None, token_size: Optional[int] = None):
    """
    Patch a HEX file by inserting data at the specified flash address
    """
    # Check if input and output are the same file
    same_file = os.path.abspath(input_file) == os.path.abspath(output_file)

    # Read the original HEX file
    hex_file = IntelHexFile()
    hex_file.load_from_file(input_file)

    # Validate data size if we have expected size information
    if token_size is not None and token_name is not None:
        validate_data_size(patch_data, token_size, token_name)
        print(f"Patching token '{token_name}' ({len(patch_data)} bytes) at address 0x{flash_address:08X}")
    else:
        print(f"Patching {len(patch_data)} bytes at address 0x{flash_address:08X}")

    # Add the patch data
    hex_file.add_data_at_address(flash_address, patch_data)

    if same_file:
        # Use temporary file for safety when overwriting
        with tempfile.NamedTemporaryFile(mode='w', delete=False, suffix='.hex') as temp_file:
            temp_filename = temp_file.name

        try:
            # Write to temporary file first
            hex_file.save_to_file(temp_filename)
            # If successful, replace the original
            shutil.move(temp_filename, output_file)
        except Exception as e:
            # Clean up temporary file on error
            if os.path.exists(temp_filename):
                os.unlink(temp_filename)
            raise e
    else:
        # Write directly to output file
        hex_file.save_to_file(output_file)


def patch_multiple_tokens(input_file: str, output_file: str, token_definitions: dict, token_data: dict):
    """
    Patch a HEX file with multiple tokens from token data file
    """
    # Check if input and output are the same file
    same_file = os.path.abspath(input_file) == os.path.abspath(output_file)

    # Read the original HEX file
    hex_file = IntelHexFile()
    hex_file.load_from_file(input_file)

    print(f"Patching {len(token_data)} tokens:")

    # Process each token in the data file
    for token_name, token_info in token_data.items():
        if token_name not in token_definitions:
            print(f"Warning: Token '{token_name}' not found in definitions file, skipping")
            continue

        # Get token address and size from definitions
        flash_address, token_size = get_token_info(token_definitions, token_name)

        # Parse token value
        patch_data = parse_token_value(token_info['value'], token_info['value_type'], token_name)

        # Validate data size
        validate_data_size(patch_data, token_size, token_name)

        # Add the patch data
        hex_file.add_data_at_address(flash_address, patch_data)

        print(f"  {token_name}: {len(patch_data)} bytes at 0x{flash_address:08X}")

    # Write the patched HEX file
    if same_file:
        # Use temporary file for safety when overwriting
        with tempfile.NamedTemporaryFile(mode='w', delete=False, suffix='.hex') as temp_file:
            temp_filename = temp_file.name

        try:
            # Write to temporary file first
            hex_file.save_to_file(temp_filename)
            # If successful, replace the original
            shutil.move(temp_filename, output_file)
        except Exception as e:
            # Clean up temporary file on error
            if os.path.exists(temp_filename):
                os.unlink(temp_filename)
            raise e
    else:
        # Write directly to output file
        hex_file.save_to_file(output_file)


def process_multi_token_args(multi_token_file: str,
                             token_def_file: str,
                             input_file: str,
                             output_file: str):
    if not token_def_file:
        print("Error: --multi-token requires --token-file")
        sys.exit(1)

    # Load token definitions and data
    token_definitions = load_token_definitions(token_def_file)
    token_data = load_token_data(multi_token_file)

    # Patch multiple tokens
    patch_multiple_tokens(input_file, output_file, token_definitions, token_data)


def process_single_data_args(data: str,
                             token: str,
                             token_file: str,
                             address: str,
                             input_file: str,
                             output_file: str):
    if not data:
        print("Error: --data is required when using --address or --token")
        sys.exit(1)

    # Determine flash address
    if token:
        if not token_file:
            print("Error: --token requires --token-file")
            sys.exit(1)

        tokens = load_token_definitions(token_file)
        flash_address, token_size = get_token_info(tokens, token)
        token_name = token
    else:
        # Parse flash address directly
        flash_address = int(address, 16)
        token_size = None
        token_name = None

    # Parse data
    if data.startswith('@'):
        # Read data from file
        data_file = data[1:]
        with open(data_file, 'rb') as f:
            patch_data = f.read()
    else:
        # Parse hex string
        hex_string = data.replace(' ', '').replace('0x', '')
        if len(hex_string) % 2 != 0:
            raise ValueError("Hex string must have even number of characters")
        patch_data = bytes.fromhex(hex_string)

    # Perform the patch
    patch_hex_file(input_file, output_file, flash_address, patch_data, token_name, token_size)


def main():
    parser = argparse.ArgumentParser(description='Patch Intel HEX files with data at specified addresses or token locations')
    parser.add_argument('input_file', help='Input HEX file to patch')
    parser.add_argument('output_file', help='Output HEX file (patched)')

    # Create mutually exclusive groups for different modes
    mode_group = parser.add_mutually_exclusive_group(required=True)
    mode_group.add_argument('-a', '--address', help='Flash address to patch (hex format, e.g., 0x8000)')
    mode_group.add_argument('-t', '--token', help='Single token name to patch (requires --token-file)')
    mode_group.add_argument('-m', '--multi-token', help='JSON file containing multiple token data to patch (requires --token-file)')

    parser.add_argument('-f', '--token-file', help='JSON file containing token definitions')
    parser.add_argument('-d', '--data',
                       help='Data to insert (hex string, e.g., "DEADBEEF" or file path with @file.bin) - used with -a or -t')

    args = parser.parse_args()

    # fail gracefully if missing an input file, this is important for zigbee apps
    if not Path(args.input_file).exists():
        print(f"{Path(args.input_file).name} does not exist, skipping tokens patch")
        return

    if not Path(args.output_file).exists():
        print(f"{Path(args.output_file).name} does not exist, skipping tokens patch")
        return

    try:
        if args.multi_token:
            # Multi-token mode
            process_multi_token_args(args.multi_token,
                                     args.token_file,
                                     args.input_file,
                                     args.output_file)

        else:
            # Single token or address mode
            process_single_data_args(args.data,
                                     args.token,
                                     args.token_file,
                                     args.address,
                                     args.input_file,
                                     args.output_file)

        print("Patch completed successfully!")

    except FileNotFoundError as e:
        print(f"Error: File not found - {e}")
        sys.exit(1)
    except ValueError as e:
        print(f"Error: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Unexpected error: {e}")
        sys.exit(1)


if __name__ == '__main__':
    main()
