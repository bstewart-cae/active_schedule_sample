# SPDX-License-Identifier: LicenseRef-TridentMSLA
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
import argparse
import os
import re
import sys
import time
import ctypes
import subprocess
import json
from pathlib import Path

class TokenGenerator:
    _mfg_token_header_file: Path
    _app_token_header_file: Path
    _token_def_file: Path
    _target: str
    _flash_size: str
    _tok_type: str
    _tok_header: Path
    _platform_path: Path
    _output_dir: Path

    def __init__(
            self,
            type: str,
            util_path: Path,
            platform_path: Path,
            header: Path,
            target: str,
            flash_size: str,
            output: Path
        ):
        # TODO: figure out better way to handle when project is building stand alone versus inside the SDK
        self._mfg_token_header_file = Path(f"{util_path}/tr_mfg_tokens.h")
        self._app_token_header_file = Path(f"{util_path}/tr_app_tokens.h")
        self._token_def_file = Path(f"{util_path}/generator/token_def.c")
        self._target = target
        self._flash_size = flash_size
        self._tok_type = type
        self._tok_header = header
        self._platform_path = platform_path
        self._output_dir = output

    def _find_git_root(self, file_path):
        # Get the absolute path of the input file/directory
        file_path = os.path.abspath(file_path)

        # Traverse the directory tree upwards
        current_path = file_path
        while current_path != os.path.dirname(current_path):  # Stop when we reach the root (when current_path == /)
            git_folder = os.path.join(current_path, ".git")
            if os.path.exists(git_folder) and os.path.isdir(git_folder):
                # If we find a .git directory, return its path
                return current_path
            # Move one directory level up
            current_path = os.path.dirname(current_path)
            
        return None  # Return None if no .git folder was found

    def _generate_c_library(self):
        if self._tok_type in "MFG":
            # use SDK token header when building MFG tokens
            tok_header = self._mfg_token_header_file

            with open(tok_header, 'r') as file:
                c_header_content = file.read()

            # use a regular expression to scrape out the TR_MFG_TOKEN_ variable names
            pattern = r'TR_CREATE_MFG_TOKEN\((TR_MFG_TOKEN_[A-Z0-9_]+),\s*\w+\)'

            # find all TR_MFG_TOKEN_ variables
            tokens = re.findall(pattern, c_header_content)

            # create a new C file
            mapped_content = "#include <stdio.h>\n"
            mapped_content += "#include <stdint.h>\n"
            mapped_content += "#include <stdbool.h>\n"
            mapped_content += "#include <string.h>\n"
            mapped_content += f"#include \"{str(self._mfg_token_header_file.name)}\"\n"

            # get start offset
            mapped_content += "uint32_t get_mfg_token_start_offset(void) {\n"
            mapped_content += "    return TR_MFG_TOKEN_BASE;\n"
            mapped_content += "}\n\n"

            # generate function for getting offset
            mapped_content += "uint32_t get_mfg_token_offset(const char* token_name) {\n"
            mapped_content += "    if (token_name == NULL) return 0;\n"
            for token in tokens:
                mapped_content += f"    if (strcmp(token_name, \"{token}\") == 0) return tr_get_mfg_token_offset({token});\n"
            mapped_content += "    return 0;\n"
            mapped_content += "}\n\n"

            # generate function for getting size
            mapped_content += "uint32_t get_mfg_token_size(const char* token_name) {\n"
            mapped_content += "    if (token_name == NULL) return 0;\n"
            for token in tokens:
                mapped_content += f"    if (strcmp(token_name, \"{token}\") == 0) return tr_get_mfg_token_len({token});\n"
            mapped_content += "    return 0;\n"
            mapped_content += "}\n\n"

            # check if token exists
            mapped_content += "bool check_mfg_token_exists(const char* token_name) {\n"
            mapped_content += "    if (token_name == NULL) return false;\n"
            for token in tokens:
                mapped_content += f"    if (strcmp(token_name, \"{token}\") == 0) return true;\n"
            mapped_content += "    return false;\n"
            mapped_content += "}\n\n"

            # save the new C file
            with open(self._token_def_file, 'w') as output_file:
                output_file.write(mapped_content)

        elif self._tok_type in "APP":
            tok_header = self._tok_header

            with open(tok_header, 'r') as file:
                c_header_content = file.read()

            # use a regular expression to scrape out the app token variable names
            pattern = r'TR_CREATE_APP_TOKEN\(\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*,'

            # find all app token variables
            tokens = re.findall(pattern, c_header_content)

            # create a new C file
            mapped_content = "#include <stdio.h>\n"
            mapped_content += "#include <stdint.h>\n"
            mapped_content += "#include <stdbool.h>\n"
            mapped_content += "#include <string.h>\n"
            mapped_content += f"#include \"{str(self._app_token_header_file.name)}\"\n"
            mapped_content += f"#include \"{str(self._tok_header.name)}\"\n"

            # get start offset
            mapped_content += "uint32_t get_app_token_start_offset(void) {\n"
            mapped_content += "    return TR_APP_TOKEN_BASE;\n"
            mapped_content += "}\n\n"

            # generate function for getting offset
            mapped_content += "uint32_t get_app_token_offset(const char* token_name) {\n"
            mapped_content += "    if (token_name == NULL) return 0;\n"
            for token in tokens:
                mapped_content += f"    if (strcmp(token_name, \"{token}\") == 0) return tr_get_app_token_offset({token});\n"
            mapped_content += "    return 0;\n"
            mapped_content += "}\n\n"

            # generate function for getting size
            mapped_content += "uint32_t get_app_token_size(const char* token_name) {\n"
            mapped_content += "    if (token_name == NULL) return 0;\n"
            for token in tokens:
                mapped_content += f"    if (strcmp(token_name, \"{token}\") == 0) return tr_get_app_token_len({token});\n"
            mapped_content += "    return 0;\n"
            mapped_content += "}\n\n"

            # check if token exists
            mapped_content += "bool check_app_token_exists(const char* token_name) {\n"
            mapped_content += "    if (token_name == NULL) return false;\n"
            for token in tokens:
                mapped_content += f"    if (strcmp(token_name, \"{token}\") == 0) return true;\n"
            mapped_content += "    return false;\n"
            mapped_content += "}\n\n"

            # Save the new C file
            with open(self._token_def_file, 'w') as output_file:
                output_file.write(mapped_content)


    def _build_c_library(self):
        # delete build folder if it exists
        if Path(f"{str(self._token_def_file.parent)}/build").is_dir():
            subprocess.run(["rm", "-rf", f"{str(self._token_def_file.parent)}/build"], check=True)

        if self._tok_type in "MFG":
            # prepare to build C library
            try:
                command = ["cmake",
                        "-S", f"{str(self._token_def_file.parent)}",
                        "-B", f"{str(self._token_def_file.parent)}/build",
                        "-DCMAKE_C_FLAGS=-fPIC -Wno-pointer-to-int-cast",
                        f"-DPLATFORM_PATH={str(self._platform_path)}",
                        f"-DTR_PLATFORM={self._target}{self._flash_size}"]

                # Run the command
                result = subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

                # Capture the output and error messages
                output = result.stdout.decode('utf-8')
                error = result.stderr.decode('utf-8')

                #print("Output:\n", output)
                if error:
                    print("Error:\n", error)

            except subprocess.CalledProcessError as e:
                print(f"An error occurred while running the command: {e}")

        elif self._tok_type in "APP":
            # prepare to build C library
            try:
                command = ["cmake",
                        "-S", f"{str(self._token_def_file.parent)}",
                        "-B", f"{str(self._token_def_file.parent)}/build",
                        "-DCMAKE_C_FLAGS=-fPIC -Wno-pointer-to-int-cast",
                        f"-DAPP_INCLUDE_PATH={str(self._tok_header.parent)}",
                        f"-DAPP_INCLUDE_FILE={str(self._tok_header.name)}",
                        f"-DPLATFORM_PATH={str(self._platform_path)}",
                        f"-DTR_PLATFORM={self._target}{self._flash_size}"]

                # Run the command
                result = subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

                # Capture the output and error messages
                output = result.stdout.decode('utf-8')
                error = result.stderr.decode('utf-8')

                #print("Output:\n", output)
                if error:
                    print("Error:\n", error)

            except subprocess.CalledProcessError as e:
                print(f"An error occurred while running the command: {e}")

        # build C library
        try:
            command = ["cmake", "--build", f"{str(self._token_def_file.parent)}/build"]

            # Run the command
            result = subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

            # Capture the output and error messages
            output = result.stdout.decode('utf-8')
            error = result.stderr.decode('utf-8')

            #print("Output:\n", output)
            if error:
                print("Error:\n", error)

        except subprocess.CalledProcessError as e:
            print(f"An error occurred while running the command: {e}")


    def _generate_tokens_json(self):
        c_lib = f"{str(self._token_def_file.parent)}/build/libtoken_def.so"
        tokens_c_lib = ctypes.CDLL(str(c_lib))
        json_path = Path(f"{str(self._output_dir)}/{self._target}{self._flash_size}_{str(self._tok_type).lower()}_token_def.json")

        if self._tok_type in "MFG":
            #print(f"Token Start Offset: {str(hex(tokens_c_lib.get_mfg_token_start_offset()))}")

            # use SDK token header when building MFG tokens
            tok_header = self._mfg_token_header_file

            with open(tok_header, 'r') as file:
                c_header_content = file.read()

            # use a regular expression to scrape out the TR_MFG_TOKEN_ variable names
            pattern = r'TR_CREATE_MFG_TOKEN\((TR_MFG_TOKEN_[A-Z0-9_]+),\s*\w+\)'

            # find all TR_MFG_TOKEN_ variables
            tokens = re.findall(pattern, c_header_content)

            tokens_dict = {}

            #tokens_c_lib.get_mfg_token_start_offset.argtypes = [ctypes.c_uint32]
            start_offset = int(tokens_c_lib.get_mfg_token_start_offset())

            for token in tokens:
                # get token address
                tokens_c_lib.get_mfg_token_offset.argtypes = [ctypes.c_char_p]
                address = hex(int(tokens_c_lib.get_mfg_token_offset(token.encode('utf-8'))) + start_offset)

                # get token size
                tokens_c_lib.get_mfg_token_size.argtypes = [ctypes.c_char_p]
                size = hex(tokens_c_lib.get_mfg_token_size(token.encode('utf-8')))

                # add dictionary entry with the extracted information
                tokens_dict[token] = {
                    "address": address,
                    "size": size
                }

            with open(json_path, 'w') as json_file:
                json.dump(tokens_dict, json_file, indent=4)

        elif self._tok_type in "APP":
            #print(f"Token Start Offset: {str(hex(tokens_c_lib.get_app_token_start_offset()))}")

            tok_header = self._tok_header

            with open(tok_header, 'r') as file:
                c_header_content = file.read()

            # use a regular expression to scrape out the app token variable names
            pattern = r'TR_CREATE_APP_TOKEN\(\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*,'

            # find all app token variables
            tokens = re.findall(pattern, c_header_content)

            tokens_dict = {}

            #tokens_c_lib.get_mfg_token_start_offset.argtypes = [ctypes.c_uint32]
            start_offset = int(tokens_c_lib.get_app_token_start_offset())

            for token in tokens:
                # get token address
                tokens_c_lib.get_app_token_offset.argtypes = [ctypes.c_char_p]
                address = hex(int(tokens_c_lib.get_app_token_offset(token.encode('utf-8'))) + start_offset)

                # get token size
                tokens_c_lib.get_app_token_size.argtypes = [ctypes.c_char_p]
                size = hex(tokens_c_lib.get_app_token_size(token.encode('utf-8')))

                # add dictionary entry with the extracted information
                tokens_dict[token] = {
                    "address": address,
                    "size": size
                }

            with open(json_path, 'w') as json_file:
                json.dump(tokens_dict, json_file, indent=4)

        #print(f"{str(json_path)} generation complete\n")


    def _clean_up_build(self):
        # remove temporary files
        if Path(f"{str(self._token_def_file.parent)}/build").is_dir():
            subprocess.run(["rm", "-rf", f"{str(self._token_def_file.parent)}/build"], check=True)

        if os.path.exists(self._token_def_file):
            subprocess.run(["rm", str(self._token_def_file)], check=True)


    def generate(self):
        # generate C library file for extracting token information
        self._generate_c_library()

        # build C library for extracting token information
        self._build_c_library()

        # use C library to generate JSON file that shall be consumed by eclap
        self._generate_tokens_json()

        # clean up left over build artifacts
        self._clean_up_build()


def validate_type(value):
    if value not in ["APP", "MFG"]:
        raise argparse.ArgumentTypeError(f"Invalid type: {value}. Must be 'APP' or 'MFG'")
    return value

def validate_path(value):
    if not Path(value).exists():
        raise argparse.ArgumentTypeError(f"Invalid output path: {value} does not exist!")
    return value

def main():
    # Create argument parser
    parser = argparse.ArgumentParser(description="Process type, target, and header values.")

    # Add arguments
    parser.add_argument("-t", "--type", type=validate_type, required=True,
                        help="Specify the type. Must be either 'APP' or 'MFG'")
    parser.add_argument("-c", "--chip", type=str, required=True,
                        help="Specify the chip target string")
    parser.add_argument("-s", "--size", type=str, required=True,
                        help="Specify the chip target flash size character")
    parser.add_argument("-u", "--util", type=Path, required=True,
                        help="Specify the path to the token utility directory")
    parser.add_argument("-p", "--platform", type=validate_path, required=True,
                        help="Specify the path to the chip platform directory")
    parser.add_argument("-f", "--file", type=Path, required=False,
                        help="Specify the path to the token header file")
    parser.add_argument("-o", "--out", type=validate_path, required=True,
                        help="Specify the path to the output directory")

    # Parse arguments
    args = parser.parse_args()

    # Print the received arguments
    #print(f"Type: {args.type}")
    #print(f"Target: {args.chip}")

    # get relative path to token utility directory
    rel_util = Path(os.path.join(os.getcwd(), args.util))

    # get relative path to platform directory
    rel_platform = Path(os.path.join(os.getcwd(), args.platform))

    # get relative path to header file
    rel_header = ""
    if "APP" in args.type:
        rel_header = Path(os.path.join(os.getcwd(), args.file))
    
    tok = TokenGenerator(type=args.type, 
                         util_path=rel_util,
                         platform_path=rel_platform,
                         header=rel_header,
                         target=str(args.chip).upper(),
                         flash_size=str(args.size).upper(),
                         output=Path(args.out))
    tok.generate()


if __name__ == "__main__":
    main()

