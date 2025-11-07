# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

# pylint: disable=missing-function-docstring
""" CLI application for the ISP interface """
import sys
import time
import os
from hashlib import sha256

from argparse import ArgumentParser
from pathlib import Path
from typing import Optional

import logging
import serial
from ecdsa import SigningKey

EXIT_CODE_OK = 0
EXIT_CODE_ARGS = 2
EXIT_CODE_KEY_FILE_MISSING = 3
EXIT_CODE_SECURE_ERASE = 4
EXIT_CODE_SECURE_WRITE = 5

magic_string = bytes([0x50, 0x4B, 0xA3, 0x72, 0x00, 0x00, 0x05, 0xFF])

ISP_HEADER = 0x52

ISP_END = 0xC0
ISP_ESC = 0xDB
isp_end_encode = bytes([0xDB, 0xDC])
isp_esc_encode = bytes([0xDB, 0xDD])

HANDSHAKE_START = b'\xAA'
HANDSHAKE_ACK = b'\x55'
HANDSHAKE_ISP_START = b'\x69'
HANDSHAKE_ISP_READY = b'\x6A'

HANDSHAKE_MAX_SECONDS = 5
HANDSHAKE_ATTEMPT_RX_TIMEOUT_SECONDS = 0.02
HANDSHAKE_MAX_ATTEMPTS = HANDSHAKE_MAX_SECONDS * int(1000 / HANDSHAKE_ATTEMPT_RX_TIMEOUT_SECONDS)


def bytes_to_hex(byte_array):
    hex_string = byte_array.hex()
    # Add the "0x" prefix to each pair of hexadecimal digits
    hex_with_prefix = " ".join(
        [f"0x{hex_string[i : i + 2]}" for i in range(0, len(hex_string), 2)]
    )
    return hex_with_prefix


def send_and_wait(ser_port, tx_byte, rx_byte) -> bool:
    ser_port.write(tx_byte)
    response = ser_port.read(1)  # Wait for the response
    return bool(response == rx_byte)


def calculate_checksum(data):
    checksum = 0
    for byte in data:
        checksum += byte  # XOR the CRC with the byte
    return ~checksum & 0xFF


def calculate_crc8(data):
    crc = 0  # Initialize the CRC to 0
    poly = 0xD5
    for byte in data:
        crc ^= byte  # XOR the CRC with the byte
        for _ in range(8):
            if crc & 0x80:  # Check if the leftmost bit is 1
                crc = (
                    (crc << 1) ^ poly
                ) & 0xFF  # If it is, perform a polynomial division
            else:
                crc = crc << 1  # If not, just shift the CRC to the left
    return crc


def calculate_crc16_ccitt(data):
    crc = 0xFFFF  # Initial CRC value
    poly = 0x1021  # CRC-16/CCITT-FALSE polynomial
    for byte in data:
        crc ^= (byte << 8) & 0xFFFF
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ poly) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def encode_isp_cmd(isp_cmd):
    encoded_isp_cmd = isp_cmd[0].to_bytes(1, "little")  # first byte is 0x52

    for i in range(1, len(isp_cmd)):
        byte = isp_cmd[i]
        if byte == 0xC0:
            encoded_isp_cmd = encoded_isp_cmd + isp_end_encode
        elif byte == 0xDB:
            encoded_isp_cmd = encoded_isp_cmd + isp_esc_encode
        else:
            encoded_isp_cmd = encoded_isp_cmd + byte.to_bytes(1, "little")

    encoded_isp_cmd = (
        ISP_END.to_bytes(1, "little") + encoded_isp_cmd + ISP_END.to_bytes(1, "little")
    )
    return encoded_isp_cmd


def creat_isp_frame(seq, cmd):
    cmd_len = len(cmd)
    isp_cmd = bytes([ISP_HEADER, seq]) + cmd_len.to_bytes(2, "big")
    isp_checksum = calculate_checksum(isp_cmd[1:])
    isp_cmd = isp_cmd + isp_checksum.to_bytes(1, "little") + cmd
    isp_cmd_crc16 = calculate_crc16_ccitt(isp_cmd)
    isp_cmd = isp_cmd + isp_cmd_crc16.to_bytes(2, "big")
    #  print("isp_cmd :" + bytes_to_hex(isp_cmd))
    isp_cmd = encode_isp_cmd(isp_cmd)
    return isp_cmd


def create_write_key_cmd(seq, public_key):
    cmd = bytes([0x09, 0x01, 0xEF, 0x00, 0x10, 0x00, 0x00])
    cmd_data = magic_string + public_key + bytes([0xFF] * 184)
    crc8 = calculate_crc8(cmd_data)
    cmd = cmd + cmd_data + crc8.to_bytes(1, "little")
    cmd_frame = creat_isp_frame(seq, cmd)
    return cmd_frame


def create_earse_secure_page(seq):
    cmd = bytes([0x08, 0x05, 0x00, 0x10, 0x00, 0x00])
    cmd_frame = creat_isp_frame(seq, cmd)
    return cmd_frame


def read_register(seq):
    cmd = bytes([0x01, 0x00, 0x50, 0x04, 0x47, 0x00]) # OTP2_192 register
    cmd_frame = creat_isp_frame(seq, cmd)
    return cmd_frame


def request_debug_secure_debug(seq):
    cmd = bytes([0x0F, 0x00])
    cmd_frame = creat_isp_frame(seq, cmd)
    return cmd_frame


def send_challenge_response(seq, response):
    cmd = bytearray([0x10, 0x00, 0x00])
    cmd.extend(response)
    cmd_frame = creat_isp_frame(seq, cmd)
    return cmd_frame

class ISP():
    """ Implements ISP commands. """
    PERIPH_BASE = 0x40000000
    PERIPH_SECURE_OFFSET = 0x10000000
    SEC_CTRL_BASE = PERIPH_BASE+PERIPH_SECURE_OFFSET+0x3000
    OTP2_OFFSET = 0x44000
    OTP2_BASE_ADDRESS = 0x50000000 + OTP2_OFFSET

    ROOT_PUBLIC_KEY_OFFSET = 0x00
    ADAC_PUBLIC_KEY_OFFSET = 0x40
    ROOT2_PUBLIC_KEY_OFFSET = 0x80
    ROOT3_PUBLIC_KEY_OFFSET = 0xC0


    def __init__(self, ser, confirm: bool) -> None:
        self.sequence_number = 1
        self.ser: serial.Serial = ser
        self.confirm: bool = confirm
        self._logger = logging.getLogger(f'{self.__class__.__name__}')
        self._logger.setLevel(logging.DEBUG)

        # Console handler
        stream_handler = logging.StreamHandler()
        stream_handler.setLevel(logging.DEBUG)

        # Formatter
        formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
        stream_handler.setFormatter(formatter)

        # Add handler
        self._logger.addHandler(stream_handler)

    def send_cmd(self, cmd_string: bytes, rx_len: int, rx_timeout_ms: int) -> bytes:
        org_timeout = self.ser.timeout
        self.ser.timeout = rx_timeout_ms / 1000
        self.ser.write(cmd_string)
        self.ser.flush()
        self._logger.debug("BUM!")
        rx_data = self.ser.read(rx_len)
        self.ser.timeout = org_timeout
        return rx_data

    def execute_cmd(self, cmd, status, timeout, rx_count: Optional[int] = 128) -> tuple[bool, bytes]:
        for _ in range(3):
            print("cmd_tx :" + bytes_to_hex(cmd))
            cmd_rx: bytes = self.send_cmd(cmd, rx_count, timeout)
            print("cmd_rx :" + bytes_to_hex(cmd_rx))
            if status is not None:
                if cmd_rx[7] == status:
                    return True, cmd_rx
            else:
                self.ser.reset_input_buffer()
                self.ser.reset_output_buffer()
                return True, cmd_rx
            #time.sleep(0.25)
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
        return False, bytes()

    def get_flash_id(self) -> int:
        cmd = bytes([0x04, 0x00])
        cmd_frame = creat_isp_frame(self.sequence_number, cmd)
        self.sequence_number += 1
        result, response = self.execute_cmd(cmd_frame, None, 100)
        if result is not True:
            raise RuntimeError
        return int.from_bytes(response[8:12], byteorder="big")

    # pylint: disable=no-self-use
    def _address_to_bytes(self, address: int) -> bytes:
        return address.to_bytes(4, 'little')
    # pylint: enable=no-self-use

    def read_register(self, address: int) -> bytes:
        cmd: bytearray = bytearray([0x01, 0x00])
        cmd.extend(self._address_to_bytes(address))
        cmd_frame = creat_isp_frame(self.sequence_number, cmd)
        self.sequence_number += 1
        result, response = self.execute_cmd(cmd_frame, None, 100)
        if result is not True:
            raise RuntimeError
        return response[8:12]

    def verify_bootloader_signature(self) -> bool:
        cmd: bytearray = bytearray([0x0E, 0x00])
        frame = creat_isp_frame(self.sequence_number, cmd)
        self.sequence_number += 1
        if self.confirm is True:
            self._logger.debug("Executing command")
            _, _ = self.execute_cmd(frame, None, 100)

        return True

    def write_register(self, address: int, value: bytes) -> bool:
        byte_str = ' '.join(f"{b:02x}" for b in value[0:4])
        self._logger.debug("0x%02X: %s", address, byte_str)

        cmd: bytearray = bytearray([0x02, 0x00])
        cmd.extend(self._address_to_bytes(address))
        cmd.extend(value[0:4])

        frame = creat_isp_frame(self.sequence_number, cmd)
        self.sequence_number += 1
        if self.confirm is True:
            self._logger.debug("Executing command")
            _, _ = self.execute_cmd(frame, None, 100)

        return True

    def _write_otp_register(self, offset: int, value: bytes) -> bool:
        buffer: bytearray = bytearray([0x11, 0x00]) # Command + option
        offset_in_bytes: bytes = offset.to_bytes(4, "little")

        buffer.append(~(sum(offset_in_bytes) % 256) & 0xFF) # Address checksum
        buffer.extend(offset_in_bytes)
        buffer.extend(value)
        buffer.append(calculate_crc8(value))
        buffer_str = ' '.join(f"{b:02x}" for b in buffer)
        self._logger.debug(f"0x{offset:08x}: {buffer_str}")

        frame = creat_isp_frame(self.sequence_number, buffer)
        self.sequence_number += 1
        if self.confirm is True:
            self._logger.debug("Executing command")
            _, _ = self.execute_cmd(frame, None, 100)

    def _write_otp_key(self, offset: int, key: bytes) -> bool:
        buffer: bytearray = bytearray([0x12, 0x00]) # Command + option
        offset_in_bytes: bytes = offset.to_bytes(4, "little")

        key_reversed = key

        buffer.append(~(sum(offset_in_bytes) % 256) & 0xFF) # Address checksum
        buffer.extend(offset_in_bytes)
        buffer.extend(key_reversed)
        buffer_str = ' '.join(f"{b:02x}" for b in buffer)
        self._logger.debug(f"0x{offset:08x}: {buffer_str}")

        frame = creat_isp_frame(self.sequence_number, buffer)
        self.sequence_number += 1
        if self.confirm is True:
            self._logger.debug("Executing command")
            _, _ = self.execute_cmd(frame, None, 100)


    def write_root_public_key(self, key: bytes) -> bool:
        for index in range(0,16):
            offset = index * 4
            byte_str = ' '.join(f"{b:02x}" for b in key[offset:offset+4])
            self._logger.debug(f"0x{offset + ISP.ROOT_PUBLIC_KEY_OFFSET:08x}: {byte_str}")

        return self._write_otp_key(ISP.ROOT_PUBLIC_KEY_OFFSET, key)

    def write_adac_public_key(self, key: bytes) -> bool:
        for index in range(0,16):
            offset = index * 4
            byte_str = ' '.join(f"{b:02x}" for b in key[offset:offset+4])
            self._logger.debug(f"0x{offset + ISP.ADAC_PUBLIC_KEY_OFFSET:08x}: {byte_str}")

        return self._write_otp_key(ISP.ADAC_PUBLIC_KEY_OFFSET, key)

    def read_root_public_key(self) -> bytes:
        key: bytearray = bytearray()

        for index in range(0,16):
            offset = index * 4
            address = ISP.OTP2_BASE_ADDRESS + offset
            print(f"{index}: 0x{address:08x}")
            key.extend(self.read_register(address))
            print(len(key))

        return key

    def read_adac_public_key(self) -> bytes:
        key: bytearray = bytearray()

        for index in range(16,32):
            offset = index * 4
            address = ISP.OTP2_BASE_ADDRESS + offset
            print(f"{index}: 0x{address:08x}")
            key.extend(self.read_register(address))
            print(len(key))

        return key

    def test_adac(self, private_key: Path):
        with private_key.open('rb') as file:
            sk = SigningKey.from_pem(file.read(), hashfunc=sha256)

        cd = bytearray([0x52, 0x00, 0x00, 0x01])
        r = bytes([
            # TEST VALUES. DO NOT USE IN PRODUCTION!
            0xA0, 0x64, 0x8A, 0x35, 0xB2, 0xD8, 0x5B, 0xEA,
            0x19, 0x1F, 0xB0, 0xAE, 0x31, 0xAD, 0xE3, 0xAC,
            0xB7, 0x8C, 0x91, 0x2C, 0xF0, 0x60, 0x44, 0x90,
            0xB4, 0x80, 0xE3, 0xB0, 0x2A, 0xD0, 0xB2, 0x47,
            0xEE, 0x56, 0x91, 0x23, 0x93, 0x60, 0x0E, 0xBA,
            0xD7, 0xAF, 0xF0, 0xD4, 0x7B, 0xCB, 0xC6, 0xF9,
            0xB1, 0x86, 0xBE, 0x92, 0x17, 0x32, 0xBB, 0xCC,
            0x9A, 0xB2, 0xF5, 0x07, 0x91, 0x14, 0xE6, 0xC5
        ])
        self._logger.debug("R: %s", r.hex())

        message = cd + r

        hash_msg = sha256(message).digest()

        # A fixed k must be used in test only. DO NOT USE IN PRODUCTION.
        fixed_k = bytes([
            0xbc, 0xb3, 0x83, 0xbf, 0x38, 0x65, 0xfe, 0xb8,
            0x9b, 0x0a, 0xfa, 0xe2, 0xb9, 0x31, 0xe6, 0x86,
            0x67, 0x13, 0xdd, 0x52, 0x5e, 0x35, 0x2a, 0x8f,
            0x99, 0xa3, 0xde, 0x61, 0x82, 0x82, 0x21, 0x1f
        ])
        int_k = int.from_bytes(fixed_k,"little")

        self._logger.debug("* Private key: %s", sk.to_string().hex())
        self._logger.debug("* Hash msg: %s", hash_msg.hex())
        self._logger.debug("* k: %s", fixed_k.hex())

        signature = sk.sign_digest(
            hash_msg,
            sigencode=lambda r, s, order: (r, s),
            k=int_k
        )

        r, s = signature

        # Convert r and s to bytes individually
        r_bytes: bytes = r.to_bytes(32, 'little')
        s_bytes: bytes = s.to_bytes(32, 'little')

        signature_bytes: bytes = r_bytes + s_bytes

        self._logger.debug("Signature: %s", signature_bytes.hex())

    def enable_debug_interface(self, private_key: Path):
        self._logger.debug("Request secure debug")
        cmd = request_debug_secure_debug(self.sequence_number)
        self.sequence_number += 1
        _, response = self.execute_cmd(cmd, None, 500, rx_count=77)

        challenge = response[10 : 10 + 64]
        cd = bytearray([0x52, 0x00, 0x00, 0x01])
        r = challenge

        self._logger.debug("CD+R")
        self._logger.debug(bytes_to_hex(cd))
        self._logger.debug(f"Length: {len(cd)}")

        message = sha256(cd + r).digest()

        with private_key.open("rb") as file:
            pem = file.read()

        sk = SigningKey.from_pem(pem, hashfunc=sha256)

        vk = sk.verifying_key

        signature = sk.sign_digest(
            message,
            sigencode=lambda r, s, order: (r, s),
        )

        r, s = signature
        r_bytes: bytes = r.to_bytes(32, 'little')
        s_bytes: bytes = s.to_bytes(32, 'little')

        signature_bytes: bytes = r_bytes + s_bytes

        response = cd + signature_bytes

        cmd = send_challenge_response(self.sequence_number, response)
        self.sequence_number += 1

        _, response = self.execute_cmd(cmd, None, 500, rx_count=11)

def path_validator(path: str) -> Path:
    key_path = Path(path)
    if not key_path.is_file():
        msg = "The key does not exist."
        raise RuntimeError(msg)
    return key_path

# pylint: disable=too-many-locals,too-many-branches,too-many-statements
def main():
    baud_rate = 2000000
    write_key = False
    reset_key = False
    public_key_file = None

    parser = ArgumentParser(
        prog=os.path.basename(__file__),
        description="Offers different commands for the ISP interface."
    )

    parser.add_argument(
        '--port',
        help="Serial port to connect to.",
        required=True
    )

    parser.add_argument(
        '--confirm',
        action="store_true",
        help="Confirm writing"
    )

    command = parser.add_subparsers(dest="command")

    register_parser = command.add_parser("register")

    register_action = register_parser.add_subparsers(dest="register_action")

    register_action_parser = register_action.add_parser("read")

    register_action_parser.add_argument("--name", required=True)

    key_parser = command.add_parser("key")

    key_action = key_parser.add_subparsers(dest="key_action")

    key_action_read_parser = key_action.add_parser("read")

    key_action_read_parser.add_argument("--name", required=True)

    key_action_write_parser = key_action.add_parser("write")

    key_action_write_parser.add_argument("--name", required=True)
    key_action_write_parser.add_argument("--der", required=True, type=path_validator)

    _ = key_action.add_parser("verify")

    parser_debug = command.add_parser("debug")

    parser_debug.add_argument(
        '--pem-file',
        help="Private key to use for ADAC.",
        required=True
    )

    parser_test = command.add_parser("test")
    parser_test.add_argument("--pem", required=True, type=path_validator)

    args = parser.parse_args()

    try:
        # Open the COM port with an initial timeout in seconds
        print("press Ctrl+C to exit")
        print(f"Connecting to {args.port}")
        ser = serial.Serial(
            args.port, baud_rate, timeout=0.001
        )  # 1 milliseconds (0.002 seconds)
        if args.confirm is True:
            link_phase = 0
        else:
            link_phase = 2
        while True:
            # Establish connection by following the protocol
            if link_phase == 0:
                received_hand_shake = False
                for attempt in range(HANDSHAKE_MAX_ATTEMPTS):
                    if link_phase == 0:
                        print("Handshake attempt " + str(attempt))
                        ser.reset_input_buffer()
                        ser.write(HANDSHAKE_START)
                        ser.flush()
                        b = ser.read(1)
                        #print("b")
                        #print(b)
                        if HANDSHAKE_ACK == b:
                            link_phase = 1
                            #print("Got ack")
                    elif link_phase == 1:
                        ser.reset_input_buffer()
                        ser.write(HANDSHAKE_ISP_START)
                        ser.flush()
                        b = ser.read(1)
                        if HANDSHAKE_ISP_READY == b:
                            #print("ISP Ready")
                            received_hand_shake = True
                            break
                if not received_hand_shake:
                    print("BootROM Handshake failed...")
                    sys.exit(-100)

                link_phase = 2
                print("BootROM Handshake OK.")
                '''
                response = send_and_wait(ser, b"\xaa", b"\x55")
                if response is True:
                    ser.reset_input_buffer()
                    ser.reset_output_buffer()
                    link_phase = 1

            if link_phase == 1:
                response = send_and_wait(ser, b"\x69", b"\x6a")
                if response is True:
                    time.sleep(0.2)
                    ser.reset_input_buffer()
                    ser.reset_output_buffer()
                    link_phase = 2
                    print("link established")

                '''

            if link_phase == 2:
                print(f"Confirm: {args.confirm}")

                isp = ISP(ser, args.confirm)

                print("Get flash ID")
                flash_id = isp.get_flash_id()
                print(f"Flash ID: {flash_id:08x}")

                if args.command == "key":
                    if args.key_action == "verify":
                        isp.verify_bootloader_signature()
                    elif args.key_action == "write":
                        if args.name == "root":
                            with args.der.open('rb') as file:
                                binary_data = file.read()

                            public_key_x = binary_data[27:59]
                            public_key_y = binary_data[59:]
                            public_key_x_y_little_endian = public_key_x[::-1] + public_key_y[::-1]

                            result = isp.write_root_public_key(public_key_x_y_little_endian)
                        elif args.name == "adac":
                            with args.der.open('rb') as file:
                                binary_data = file.read()

                            public_key_x = binary_data[27:59]
                            public_key_y = binary_data[59:]
                            public_key_x_y_little_endian = public_key_x[::-1] + public_key_y[::-1]

                            result = isp.write_adac_public_key(public_key_x_y_little_endian)

                    elif args.key_action == "read":
                        if args.name == "root":
                            root_public_key = isp.read_root_public_key()
                            print("Root Public key: " + bytes_to_hex(root_public_key))
                        elif args.name == "adac":
                            adac_public_key = isp.read_adac_public_key()
                            print("ADAC Public key: " + bytes_to_hex(adac_public_key))
                        else:
                            print("Unknown key name")

                if args.command == "debug":
                    pem_file = Path(args.pem_file)
                    if not pem_file.is_file():
                        print(f"Error! Cannot open {pem_file}.")
                        sys.exit(1)
                    isp.enable_debug_interface(private_key = pem_file)

                if args.command == "test":
                    isp.test_adac(args.pem)

                break

        # Close the COM port
        ser.close()
    except serial.SerialTimeoutException:
        print("Timeout: No response received.")
    except serial.SerialException as e:
        print(f"Serial communication error: {e}")
    except KeyboardInterrupt:
        print("Program interrupted by the user")
# pylint: enable=too-many-locals,too-many-branches,too-many-statements


if __name__ == "__main__":
    main()
