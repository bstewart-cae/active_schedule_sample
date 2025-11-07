# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

"""Shared Intel HEX file utilities and classes."""

from typing import Dict


class IntelHexRecord:
    """Class to represent an Intel HEX record."""

    def __init__(self, byte_count: int, address: int, record_type: int, data: bytes):
        """Initialize Intel HEX record."""
        self.byte_count = byte_count
        self.address = address
        self.record_type = record_type
        self.data = data

    @classmethod
    def from_line(cls, line: str) -> 'IntelHexRecord':
        """Parse an Intel HEX record line."""
        line = line.strip()
        if not line.startswith(':'):
            raise ValueError(f"Invalid HEX record: {line}")

        try:
            byte_count = int(line[1:3], 16)
            address = int(line[3:7], 16)
            record_type = int(line[7:9], 16)
            data_hex = line[9:9+byte_count*2]
            data = bytes.fromhex(data_hex) if data_hex else b''
            checksum = int(line[9+byte_count*2:9+byte_count*2+2], 16)

            # Verify checksum
            record = cls(byte_count, address, record_type, data)
            calculated_checksum = record.calculate_checksum()
            if checksum != calculated_checksum:
                raise ValueError(f"Checksum mismatch in line: {line}")

            return record
        except (ValueError, IndexError) as e:
            raise ValueError(f"Failed to parse HEX record '{line}': {e}")

    def calculate_checksum(self) -> int:
        """Calculate Intel HEX checksum."""
        checksum = self.byte_count + (self.address >> 8) + (self.address & 0xFF) + self.record_type
        checksum += sum(self.data)
        return (0x100 - (checksum & 0xFF)) & 0xFF

    def to_hex_line(self) -> str:
        """Convert record to HEX file line format."""
        checksum = self.calculate_checksum()
        data_hex = ''.join(f'{b:02X}' for b in self.data)
        return f':{self.byte_count:02X}{self.address:04X}{self.record_type:02X}{data_hex}{checksum:02X}'

    def __str__(self):
        """Return the HEX line representation."""
        return self.to_hex_line()


class IntelHexFile:
    """Class to represent and manipulate Intel HEX files."""

    def __init__(self):
        """Initialize empty HEX file."""
        self.data: Dict[int, int] = {}  # address -> byte value
        self.memory_blocks: Dict[int, bytes] = {}  # address -> byte block
        self.start_address: int = 0

    def load_from_file(self, filename: str):
        """Load Intel HEX file."""
        with open(filename, 'r', encoding='utf-8') as f:
            lines = f.readlines()

        current_extended_addr = 0

        for _, line in enumerate(lines, 1):
            line = line.strip()
            if not line or not line.startswith(':'):
                continue

            try:
                record = IntelHexRecord.from_line(line)

                if record.record_type == 0:  # Data record
                    base_addr = current_extended_addr + record.address

                    # Store individual bytes for merge compatibility
                    for i, byte_val in enumerate(record.data):
                        addr = base_addr + i
                        self.data[addr] = byte_val

                    # Store as memory block for patch compatibility
                    if record.data:
                        self.memory_blocks[base_addr] = record.data

                elif record.record_type == 1:  # EOF record
                    break

                elif record.record_type == 2:  # Extended Segment Address
                    segment = int.from_bytes(record.data, 'big')
                    current_extended_addr = segment * 16

                elif record.record_type == 4:  # Extended Linear Address
                    linear = int.from_bytes(record.data, 'big')
                    current_extended_addr = linear * 65536

                elif record.record_type == 5:  # Start Linear Address
                    self.start_address = int.from_bytes(record.data, 'big')

            except ValueError:
                # Skip invalid records
                continue

    def merge_with(self, other: 'IntelHexFile'):
        """Merge another HEX file into this one."""
        # Merge individual byte data
        for addr, byte_val in other.data.items():
            self.data[addr] = byte_val  # Last write wins

        # Merge memory blocks
        for addr, block_data in other.memory_blocks.items():
            self.memory_blocks[addr] = block_data

    def add_data_at_address(self, address: int, data: bytes):
        """Add data at a specific address (useful for patching)."""
        # Add to individual byte storage
        for i, byte_val in enumerate(data):
            self.data[address + i] = byte_val

        # Add to memory block storage
        self.memory_blocks[address] = data

    def get_memory_block_data_record(self):
        lines = []
        current_extended_addr = None

        addresses = sorted(self.memory_blocks.keys())

        for addr in addresses:
            data = self.memory_blocks[addr]

            # Check if we need an extended address record
            extended_addr = addr & 0xFFFF0000
            if extended_addr != current_extended_addr:
                if extended_addr > 0xFFFF:
                    # Extended Linear Address record (type 04)
                    extended_linear = extended_addr >> 16
                    ext_data = extended_linear.to_bytes(2, 'big')
                    ext_record = IntelHexRecord(2, 0, 4, ext_data)
                    lines.append(ext_record.to_hex_line())
                current_extended_addr = extended_addr

            # Create data records for this memory block (max 16 bytes per record)
            offset = 0
            while offset < len(data):
                chunk_size = min(16, len(data) - offset)
                chunk_data = data[offset:offset + chunk_size]
                record_addr = (addr + offset) & 0xFFFF

                record = IntelHexRecord(chunk_size, record_addr, 0, chunk_data)
                lines.append(record.to_hex_line())
                offset += chunk_size

        return lines

    def get_individual_byte_data_record(self):
        lines = []
        current_extended_addr = None
        addresses = sorted(self.data.keys())

        # Group consecutive addresses into chunks of max 16 bytes
        i = 0
        while i < len(addresses):
            start_addr = addresses[i]

            # Check if we need an extended address record
            extended_addr = start_addr & 0xFFFF0000
            if extended_addr != current_extended_addr:
                if extended_addr > 0xFFFF:
                    # Extended Linear Address record (type 04)
                    extended_linear = extended_addr >> 16
                    ext_data = extended_linear.to_bytes(2, 'big')
                    ext_record = IntelHexRecord(2, 0, 4, ext_data)
                    lines.append(ext_record.to_hex_line())
                current_extended_addr = extended_addr

            # Collect consecutive bytes for this record
            record_addr = start_addr & 0xFFFF
            data_bytes = []
            byte_count = 0

            while (i < len(addresses) and
                    byte_count < 16 and
                    addresses[i] == start_addr + byte_count and
                    (addresses[i] & 0xFFFF0000) == extended_addr):

                data_bytes.append(self.data[addresses[i]])
                byte_count += 1
                i += 1

            # Create data record
            record = IntelHexRecord(byte_count, record_addr, 0, bytes(data_bytes))
            lines.append(record.to_hex_line())

        return lines

    def save_to_file(self, filename: str):
        """Save as Intel HEX file using memory blocks if available, otherwise individual bytes."""
        if not self.data and not self.memory_blocks:
            return

        lines = []

        # Use memory blocks if available for more efficient output
        if self.memory_blocks:
            lines = self.get_memory_block_data_record()
        else:
            # Fall back to individual byte method
            lines = self.get_individual_byte_data_record()

        # Add EOF record
        eof_record = IntelHexRecord(0, 0, 1, b'')
        lines.append(eof_record.to_hex_line())

        # Write to file
        with open(filename, 'w', encoding='utf-8') as f:
            for line in lines:
                f.write(line + '\n')

    def get_memory_dict(self) -> Dict[int, bytes]:
        """Get memory as address -> bytes mapping"""
        return self.memory_blocks.copy()

    def set_memory_dict(self, memory: Dict[int, bytes]):
        """Set memory from address -> bytes mapping"""
        self.memory_blocks = memory.copy()
        # Also update individual byte storage
        self.data.clear()
        for addr, block_data in memory.items():
            for i, byte_val in enumerate(block_data):
                self.data[addr + i] = byte_val
