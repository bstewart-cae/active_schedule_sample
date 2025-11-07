#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA
import argparse
import struct
import subprocess
import sys
import logging
from pathlib import Path
from ecdsa import SigningKey
from hashlib import sha256

logger = logging.getLogger()
logger.setLevel(logging.DEBUG)

# Console handler
stream_handler = logging.StreamHandler()
stream_handler.setLevel(logging.DEBUG)

# Formatter
formatter = logging.Formatter('%(levelname)s - %(message)s')
stream_handler.setFormatter(formatter)

# Add handler
logger.addHandler(stream_handler)

MAGIC_CPK = b"@$PK"               # 4 bytes : first root key
#MAGIC_CPK = b"A$PK"               # 4 bytes : second root key
#MAGIC_CPK = b"C$PK"               # 4 bytes : Third root key
MAGIC_SSR = b"SFSR"               # 4 bytes
MAX_OPTION_BYTES = 48


def align_to_256_bytes(byte_array: bytes) -> bytes:
    # Calculate the padding size to reach the desired alignment
    padding_size = 256 - len(byte_array) % 256
    if (padding_size == 256):
      padding_size = 0

    # Create a byte array with the padding
    padding = bytes([0xff] * padding_size)
    # Concatenate the original byte array with the padding
    aligned_byte_array = byte_array + padding
    return aligned_byte_array

def run_openssl(cmd: list, input_data: bytes = None) -> bytes:
    result = subprocess.run(
        cmd,
        input=input_data,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=True
    )
    return result.stdout


def derive_raw_pubkey_from_pem(pem_path: Path) -> bytes:
    # Extract public key coordinates using OpenSSL
    text = run_openssl(["openssl", "ec", "-pubin", "-in", str(pem_path), "-text", "-noout"])
    lines = text.decode().splitlines()
    hex_bytes = []
    capture = False
    for l in lines:
        l = l.strip()
        if l.startswith("pub:"):
            capture = True
            continue
        if capture:
            if l.startswith("ASN1 OID") or l.startswith("NIST CURVE"):
                break
            parts = l.split(":")
            for p in parts:
                if p:
                    hex_bytes.append(p)
    raw = bytes.fromhex("".join(hex_bytes))
    if raw[0] != 0x04 or len(raw) != 65:
        raise RuntimeError("Unexpected public key format from OpenSSL")

    key = raw[1:] # drop 0x04, keep X(32)||Y(32)

    x = key[0:32]
    y = key[32:]

    return x[::-1] + y[::-1]

def parse_asn1_text(asn1_text: str) -> bytes:
    rs = []
    for line in asn1_text.splitlines():
        if "INTEGER" in line and ":" in line:
            hex_str = line.split(":")[-1].strip()
            val = bytes.fromhex(hex_str)
            # Strip leading 0x00 if length > 32
            if len(val) > 32:
                val = val.lstrip(b"\x00")
            # Pad left to 32 bytes
            val = val.rjust(32, b"\x00")
            rs.append(val)
    if len(rs) != 2:
        raise RuntimeError("Failed to parse r and s from ASN.1 text")
    return rs[0] + rs[1]

class Certificate():
    def __init__(self, certificate: bytes, option_bytes: bytes) -> None:
        self.certificate = certificate
        logger.debug("Certificate length: %u", len(self.certificate))
        self.option_bytes = option_bytes
        logger.debug("Option byte count: %u", len(self.option_bytes))
        pass

    def __str__(self) -> str:
        cstr = ""
        cstr += f"Certificate:\n"
        cstr += f"Magic word: {self.certificate[0:4].hex()}\n"
        cstr += f"Length: {self.certificate[4:8].hex()}\n"
        cstr += f"Bootloader signature public key:\n"

        offset = 8
        for _ in range(0, 8):
            #cstr += f"Offset: {offset}\n"
            cstr += f"{self.certificate[offset:offset+8].hex()}\n"
            offset += 8

        cstr += f"Option bytes:\n"
        for _ in range(0, int(len(self.option_bytes)/8)):
            #cstr += f"Offset: {offset}\n"
            cstr += f"{self.certificate[offset:offset+8].hex()}\n"
            offset += 8

        cstr += f"Signature:\n"
        width = 16
        for _ in range(0, int(64/width)):
            #cstr += f"Offset: {offset}\n"
            cstr += f"{self.certificate[offset:offset+width].hex()}\n"
            offset += width

        return cstr

def build_certificate_1(sig_pubkey_raw: bytes, option_bytes: bytes, root_privkey_path: Path, endian='little') -> bytes:
    if len(option_bytes) > MAX_OPTION_BYTES:
        raise ValueError(f"option_bytes too long: {len(option_bytes)} > {MAX_OPTION_BYTES}")

    length_x = 4 + 4 + 64 + len(option_bytes)
    if endian == 'little':
        length_x_bytes = struct.pack('<I', length_x)
    else:
        length_x_bytes = struct.pack('>I', length_x)

    cpk_header = MAGIC_CPK + length_x_bytes
    cpk_body = sig_pubkey_raw + option_bytes

    message = cpk_header + cpk_body

    with root_privkey_path.open('rb') as file:
        sk = SigningKey.from_pem(file.read(), hashfunc=sha256)

    vk = sk.get_verifying_key()
    public_key_bytes = vk.to_string()
    logger.debug("public key from private key: %s", public_key_bytes.hex())

    hash_msg = sha256(message).digest()
    logger.debug("hash_msg: %s", hash_msg.hex())

    logger.debug("Signing with:")
    logger.debug("* Private key: %s", sk.to_string().hex())
    logger.debug("* Hash msg: %s", hash_msg.hex())

    signature = sk.sign_digest(
        hash_msg,
        sigencode=lambda r, s, order: (r, s),
    )

    r, s = signature

    # Convert r and s to bytes individually
    r_bytes: bytes = r.to_bytes(32, 'little')
    s_bytes: bytes = s.to_bytes(32, 'little')

    print(f"r (bytes): {r_bytes.hex()}")
    print(f"s (bytes): {s_bytes.hex()}")
    logger.debug("r len: %s", len(r_bytes))
    logger.debug("s len: %s", len(s_bytes))

    sig_raw = r_bytes + s_bytes

    certificate = message + sig_raw

    cert = Certificate(certificate, option_bytes)

    logger.debug(cert)

    return certificate


def build_signed_bootloader(bootloader: bytes, certificate_1: bytes, sig_privkey_path: Path, endian='little') -> bytes:
    bootloader_aligned = align_to_256_bytes(bootloader)
    len_before = len(bootloader_aligned) + len(certificate_1) + 4 + 4
    if endian == 'little':
        length_y_bytes = struct.pack('<I', len_before)
    else:
        length_y_bytes = struct.pack('>I', len_before)

    tail = MAGIC_SSR + length_y_bytes

    message = bootloader_aligned + certificate_1 + tail

    with sig_privkey_path.open('rb') as file:
        sk = SigningKey.from_pem(file.read(), hashfunc=sha256)

    hash_msg = sha256(message).digest()

    signature = sk.sign_digest(
        hash_msg,
        sigencode=lambda r, s, order: (r, s),
    )

    r, s = signature

    # Convert r and s to bytes individually
    r_bytes: bytes = r.to_bytes(32, 'little')
    s_bytes: bytes = s.to_bytes(32, 'little')

    print(f"r (bytes): {r_bytes.hex()}")
    print(f"s (bytes): {s_bytes.hex()}")
    logger.debug("r len: %s", len(r_bytes))
    logger.debug("s len: %s", len(s_bytes))

    sig_raw = r_bytes + s_bytes

    return message + sig_raw


def main():
    ap = argparse.ArgumentParser(description="Generate signed bootloader image for RT584 secure boot (OpenSSL version)")
    ap.add_argument('--bootloader', required=True, type=Path, help='Path to bootloader binary')
    ap.add_argument('--root-priv', required=True, type=Path, help='Path to root private key (PEM)')
    ap.add_argument('--btl-priv', required=True, type=Path, help='Path to signature private key (PEM)')
    ap.add_argument('--btl-pub', required=True, type=Path, help='Path to signature public key (DER)')
    ap.add_argument('--option-bytes', required=False, help='Option bytes as hex string (e.g. 0A1B2C)')
    ap.add_argument('--endian', choices=['little', 'big'], default='little', help='Endianness for length fields (default: little)')
    ap.add_argument('--out', required=True, type=Path, help='Output signed bootloader path')
    ap.add_argument('--verbose', '-v', action="store_true")
    args = ap.parse_args()

    bootloader = args.bootloader.read_bytes()

    logger.info("*******************************")
    logger.info("* Bootloader Image Signer      ")
    logger.info("*******************************")
    logger.info("")

    root_private_key_file = args.root_priv
    btl_private_key_file = args.btl_priv
    btl_public_key_file = args.btl_pub

    if args.verbose is True:
        logger.debug("Signing %s with", args.bootloader.absolute())
        logger.debug("")
        logger.debug("* Root private key: %s", root_private_key_file.absolute())
        logger.debug("* Bootloader private key: %s", btl_private_key_file.absolute())
        logger.debug("* Bootloader public key: %s", btl_public_key_file.absolute())
        logger.debug("")
        logger.debug("to %s", args.out.absolute())

    sig_pub_raw = derive_raw_pubkey_from_pem(btl_public_key_file)

    option_bytes = bytes([0x00] * 16)
    if args.option_bytes:
        hb = args.option_bytes.strip()
        if len(hb) % 2:
            hb = '0' + hb
        option_bytes = bytes.fromhex(hb)

    if len(option_bytes) > MAX_OPTION_BYTES:
        print(f"Error: option-bytes length {len(option_bytes)} exceeds {MAX_OPTION_BYTES}")
        sys.exit(2)

    certificate_1 = build_certificate_1(sig_pub_raw, option_bytes, root_private_key_file, endian=args.endian)
    final = build_signed_bootloader(bootloader, certificate_1, btl_private_key_file, endian=args.endian)

    args.out.write_bytes(final)
    print(f"Signed bootloader written to: {args.out}")


if __name__ == '__main__':
    main()
