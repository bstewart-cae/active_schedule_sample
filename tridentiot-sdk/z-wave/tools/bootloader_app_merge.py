# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

import lzma
import sys
import getopt
import zlib
import os
import pathlib
import subprocess


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



def main(argv):

     bootloader_file = None
     bootloader_data = None

     app_file = None
     app_data = None

     output_file = None

     try:
       opts, args = getopt.getopt(argv,"hb:a:o:",["help","bootloader=","app=","output="])
     except getopt.GetoptError:
       print("bootloader_app_merge.py -b <bootloader_bin_file> -a <app_bin_file> -o <output_bin_file>")
       print("merge bootloader binary file and application binary file into onefile")
       print("<bootloader_bin_file> bootloader binary file")
       print("<output_bin_file> The putput binary filer conagtinas both bootloader and application binaryies")
       sys.exit(2)

     for opt, arg in opts:
       if opt == '-h':
         print("bootloader_app_merge.py -b <bootloader_bin_file> -a <app_bin_file> -o <output_bin_file>")
         print("merge bootloader binary file and application binary file into onefile")
         print("<bootloader_bin_file> bootloader binary file")
         print("<output_bin_file> The putput binary filer conagtinas both bootloader and application binaryies")
         sys.exit()
       elif opt in ("-b", "--bootloader"):
         bootloader_file = arg
       elif opt in ("-a", "--app"):
         app_file = arg
       elif opt in ("-o", "--output"):
         output_file = arg

     try:
    # Open the bootloader file for reading in binary mode
       bootloaderF = open(bootloader_file,"rb")
       bootloader_data = bootloaderF.read()
       bootloaderF.close()
       if (bootloader_data == None) or (len(bootloader_data) == 0):
         MyCustomError("Invalid bootlaoder_binary file")
         sys.exit(1)
     except FileNotFoundError:
      print(f"Error: The file bootloader file ({bootloader_file}) does not exist.")
      sys.exit(1)
     except Exception as e:
       print(f"An error occurred: {str(e)}")
       sys.exit(1)


     try:
    # Open the application file for reading in binary mode
       appF = open(app_file,"rb")
       app_data = appF.read()
       appF.close()

       if (app_data == None) or (len(app_data) == 0):
         MyCustomError("Invalid bootlaoder_binary file")
     except FileNotFoundError:
      print(f"Error: The application file ({app_file}) does not exist.")
      sys.exit(1)
     except Exception as e:
       print(f"An error occurred: {str(e)}")
       sys.exit(1)

     try:
       writef = open(output_file,"wb")
       # padd the bootlaoder file up to 32k
       output_data = align_to_32k_bytes(bootloader_data)
       # add the bootloader data to the app data
       output_data = output_data + app_data

       writef.write(output_data)
       writef.close()


     except FileNotFoundError:
      print(f"Error: The file '{output_file}' does not exist.")
      sys.exit(1)
     except Exception as e:
       print(f"An error occurred: {str(e)}")
       sys.exit(1)

if __name__== "__main__":
  main(sys.argv[1:])


