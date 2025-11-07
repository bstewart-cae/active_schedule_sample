# SPDX-FileCopyrightText: 2023 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

import lzma
import sys
import getopt
import zlib
import os
import pathlib
import subprocess
import secrets
import string


def generate_random_string(length):
    # Define the character set for the random string
    character_set = string.ascii_letters + string.digits  # You can customize this to include other characters

    # Generate a random string of the specified length
    random_string = ''.join(secrets.choice(character_set) for _ in range(length))

    return random_string

def align_to_256_bytes(byte_array, array_len):
    # Calculate the padding size to reach the desired alignment
    padding_size = 256 - array_len % 256
    if (padding_size == 256):
      padding_size = 0

    # Create a byte array with the padding
    padding = bytes([0xff] * padding_size)
    # Concatenate the original byte array with the padding
    aligned_byte_array = byte_array + padding
    return aligned_byte_array

def extract_key(key_data):

  if key_data[3] == 32:
    key1_start_offset = 4
    key1_end_offset = 32 + 4
  else:
    key1_start_offset = 5
    key1_end_offset = 32 + 5

  if key_data[key1_end_offset + 1] == 32:
    key2_start_offset = key1_end_offset + 2
    key2_end_offset = 32 + key2_start_offset
  else:
    key2_start_offset = key1_end_offset + 3
    key2_end_offset = 32 + key2_start_offset

  sig_data1 = key_data[key1_start_offset:key1_end_offset]
  sig_data2 = key_data[key2_start_offset:key2_end_offset]
  sig_data = sig_data1[::-1] + sig_data2[::-1]

  return sig_data



def main(argv):
    sig_id = int(hex(0x534EBF58),16)
    input_file = None
    output_file = None
    output_data = None
    crc32_val = None
    private_key_file = None
    compressed_der_file = None
    plaintext_der_file = None
    encryption_key_file = None
    iv_file = None
    only_signed_bin = False
    fw_len = None
    use_comp = False
    fw_option = 0
    binary_size = None
    systeminfo = None
    # ota_header_size MUST be equal to sizeof(ota_header_t)
    ota_header_size = 48
    try:
        opts, args = getopt.getopt(argv,"hbcs:e:n:i:o:",["help","bin_only","compress","sign=","encrypt=", "nonce=", "input=","output="])
    except getopt.GetoptError:
        print("ota_image_generate.py -i <bin_file> -o <bin_ota_file> [-b] [-c] [-s <private_key>] [-e <encryption_key>] [-n <nonce>]")
        print("<bin_file> the input binary file to generate ota image or signed binary file for")
        print("<bin_ota_file> the output binary file")
        print("<private_key> the private key to use to generate the signed binary file")
        print("<encryption_key> the AES key used for encrypting the image")
        print("<nonce> the AES-CTR nonce (Initial vector) used when encrypting the image")
        print("-b only generate signed binary files")
        print("-c compress the binary file before generated the ota/signed binary file")
        sys.exit(2)

    for opt, arg in opts:
        if opt in ("-h", "--help"):
            print("ota_image_generate.py -i <bin_file> -o <bin_ota_file> [-b] [-c] [-s <private_key>]")
            sys.exit()
        elif opt in ("-i", "--input"):
            input_file = arg
        elif opt in ("-o", "--output"):
            output_file = arg
        elif opt in ("-c", "--compress"):
            use_comp = True
        elif opt in ("-s", "--sign"):
            private_key_file = arg
        elif opt in ("-b", "--bin_only"):
            only_signed_bin = True
        elif opt in ("-e", "--encrypt"):
            encryption_key_file = arg
        elif opt in ("-n", "--nonce"):
            iv_file = arg

    random_string = generate_random_string(5)
    # Open the binary file for reading in binary mode
    try:
        with open(input_file,"rb") as file:
            binary_data = file.read()
    except Exception:
        print(f"Unable to open {input_file} passed to --input :(")
        sys.exit(1)
    if only_signed_bin == False:
         #define MAGIC_WORD_STR    "M_425"  // in hex 0x4D5F343235
         index = binary_data.find(b'M_425')
         systeminfo = binary_data[index:index+16]
         index = index + len('M_425')

    input_basename = pathlib.Path(input_file).stem

    if private_key_file != None:
         """
         to  sign the image we do thw following
         add the length of the binary file into bytes 41 to 43
         run the openSSL toot to generate the signature bits in the file name orignal_bin_name.der
         then read the signature file and add the 64 bytes the original binary file as follow:
         original_bin_file + padding bytes (alligned to 256 bytes) + 0x53 0x4e 0xbf 0x85 + original bin length +
         64 signature bytes + 0xff * 184 bytes

         """

         fw_len = len(binary_data)
         #print("binary length :" + hex(fw_len))
         binary_data = binary_data[0:40] + fw_len.to_bytes(4,"little") + binary_data[44:]
         input_file = input_basename + random_string + ".bin"
         try:
            with open(input_file, "wb") as temp_file:
              temp_file.write(binary_data)
         except:
            print(f"Got exception when handling temporary file ({input_file}) :(")
            sys.exit(1)

         fw_option |= 0x02
         plaintext_der_file = input_basename  + random_string +  ".der"
         subprocess.run(
          f"openssl dgst -sha256 -sign {private_key_file} -binary -out {plaintext_der_file} {input_file}",
          shell=True,
          check=True,
         )

         try:
           with open(plaintext_der_file, "rb") as plaintext_der:
             sig_data = plaintext_der.read()
         except:
           print(f"Got exception when handling plaintext DER file ({plaintext_der_file}) :(")
           sys.exit(1)

         try:
           os.remove(plaintext_der_file)
         except:
           print(f"Got exception when removing plaintext DER file ({plaintext_der_file}) :(")
           sys.exit(1)
         binary_size = len(binary_data)
         binary_data = align_to_256_bytes(binary_data, binary_size)

         binary_data = binary_data + sig_id.to_bytes(4,'big') + binary_size.to_bytes(4,'little')
         """
         In the DER file consist of tag,length,bytes
         In the signature DER file has 3 tags first tag is 0x30 (sequence) length bytes string
         The seconed is 0x02 (integer) lenght(32 or 33) bytes string third and lst tag is
         0x02 , lenght (32 or 33), bytes string.
         when the 0x02 tag length is 33 the first byte is a padding
         """
         sig_data = extract_key(sig_data)

         binary_data = align_to_256_bytes(binary_data + sig_data, len(binary_data + sig_data))
         #print("binary + sig length :" + hex(len(binary_data)))
         try:
           with open(input_file,"wb") as temp_file:
             temp_file.truncate()
             temp_file.write(binary_data)
           os.remove(input_file)
         except:
            print(f"Got exception when opening/writing/removing temporary file ({input_file}) :(")
            sys.exit(1)
    """
        to compress the image do the following
        compress usiong lzma with dic size of 64k
        the lzma binary file format missing the length of the uncompressed file, we generate it our self
    """
    if (use_comp == True) and (only_signed_bin == False):
         dict_size = 2**16
         my_filters = [
           {"id": lzma.FILTER_LZMA1, "mode": lzma.MODE_NORMAL, "dict_size": dict_size} ,
         ]

         output_data = lzma.compress(binary_data,format=lzma.FORMAT_ALONE,filters=my_filters)
         binary_size = len(binary_data)
         uncompressed_binary_size = binary_size

         #print("compressed binary + sig length :" + str(hex(len(output_data))))
         # the lzma binary file format missing the length of the uncompressed file, we generate it our self
         output_data = output_data[:5] + binary_size.to_bytes(8, 'little') + output_data[13:]
         fw_option |= 0x01
         """
         if the compressed binary need to be signe do the following
         write the compress imaged to new file orignal_bin_file_name_compressed.bin
         run the openSSL tool to generate signature file orignal_bin_file_name_compressed.der
         then read the signature file and add the 64 bytes to the original binary file as follow:
         compressed_file + padding bytes (alligned to 256 bytes) + 0x53 0x4e 0xbf 0x85 + compressed file length +
         64 signature bytes + 0xff * 184 bytes

         """

         if (encryption_key_file != None and iv_file != None):
           kfile = open(encryption_key_file,"rt")
           encryption_key = kfile.read()
           encryption_key = encryption_key.strip()
           kfile.close()
           ifile = open(iv_file,"rt")
           encryption_iv = ifile.read()
           encryption_iv = encryption_iv.strip()
           ifile.close()
           enc_bin_file_name = input_basename + random_string + "_before_encryption.bin"
           bfile = open(enc_bin_file_name,"wb")
           bfile.write(output_data)
           bfile.close()
           encrypted_file = input_basename + random_string + "_encrypted.bin"
           subprocess.run(
            f"openssl enc -aes-128-ctr -in {enc_bin_file_name} -out {encrypted_file} -K {encryption_key} -iv {encryption_iv}",
            shell=True,
            check=True,
           )
           os.remove(enc_bin_file_name)

           readf = open(encrypted_file, "rb")
           output_data = readf.read()
           readf.close()
           os.remove(encrypted_file)

           fw_option |= 0x04

         if private_key_file != None:

           sig_bin_file_name = input_basename  + random_string +  "_compressed.bin"
           bfile = open(sig_bin_file_name,"wb")
           bfile.write(output_data)
           bfile.close()
           compressed_der_file = input_basename  + random_string +  "_compressed.der"
           subprocess.run(
            f"openssl dgst -sha256 -sign {private_key_file} -binary -out {compressed_der_file} {sig_bin_file_name}",
            shell=True,
            check=True,
           )
           os.remove(sig_bin_file_name)
           readf = open(compressed_der_file, "rb")
           sig_data = readf.read()
           readf.close()
           os.remove(compressed_der_file)
           binary_size = len(output_data)
           output_data = align_to_256_bytes(output_data, (binary_size + ota_header_size))

           output_data = output_data + sig_id.to_bytes(4,'big') + binary_size.to_bytes(4,'little')
           sig_data = extract_key(sig_data)
           output_data = align_to_256_bytes(output_data + sig_data, (len(output_data + sig_data) + ota_header_size))

           #print("compressed binary + sig + sig  length: " + hex(len(output_data)))

    else:
         output_data = binary_data

    fw_len = len(output_data)

    crc32_val = zlib.crc32(output_data) & 0xFFFFFFFF

    try:
    # Write compressed/uncompressed data to file.
       writef = open(output_file,"wb")
       if only_signed_bin == False:
         writef.write(systeminfo)
         endianess = "little"
         writef.write(crc32_val.to_bytes(4,endianess))
         fw_addr = int(hex(0x8000),16)
         writef.write(fw_addr.to_bytes(4,endianess))

         fw_addr = fw_addr + fw_len
         writef.write(fw_addr.to_bytes(4,endianess))

         if (private_key_file != None) and (use_comp == True):
           writef.write(fw_option.to_bytes(1,endianess))
           writef.write(uncompressed_binary_size.to_bytes(3,endianess))
         else:
           writef.write(fw_option.to_bytes(4,endianess))

         # Add IV to header
         if (iv_file == None):
           encryption_iv = "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
         encryption_iv_binary = bytes.fromhex(encryption_iv)
         print("Initial Vector =", end = " ")
         print(encryption_iv_binary)
         writef.write(encryption_iv_binary)
         writef.write(output_data)

         print(f'ota image generated as {output_file}')
       else:
         writef.write(output_data)
       writef.close()
    except Exception as e:
       print(f"An error occurred: {str(e)}")
       sys.exit(1)

if __name__== "__main__":
  main(sys.argv[1:])


