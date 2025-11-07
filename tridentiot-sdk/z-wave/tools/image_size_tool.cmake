# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

##
# @b Syntax
#
# &emsp; @c  size_file_generate(\b ELF_FILENAME )
#
# Function for generating a size file from an .elf file
#
# @param[in] ELF_FILENAME  name of elf file to be analyzed for size
#
function(size_file_generate ELF_FILENAME)
  add_custom_command(
    TARGET ${ELF_FILENAME}.elf
    POST_BUILD
    # Make size file
    COMMAND ${Python3_EXECUTABLE} ${TR_TEST_TOOLS_DIR}/mem_usage.py --elf ${ELF_FILENAME}.elf --out ${ELF_FILENAME}.txt --region FLASH:0x10008000-0x10099000 --region RAM:0x30000000-0x30030000
    COMMENT "Generating ${ELF_FILENAME} Flash and SRAM size file"
  )

endfunction()
