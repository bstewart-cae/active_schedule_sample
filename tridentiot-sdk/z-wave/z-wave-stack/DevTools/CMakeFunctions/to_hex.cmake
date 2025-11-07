##
# @b Syntax
#
# &emsp; @c  to_hex(\b IN_NAME, \b OUT_NAME )
#
# Function for converting an .elf file to hex format.
#
# @param[in] IN_NAME  name of elf file to be converted
# @param[in] OUT_NAME name of hex file which is being written
#
function(to_hex IN_NAME OUT_NAME)
  add_custom_command (TARGET ${IN_NAME} POST_BUILD COMMAND ${CMAKE_OBJCOPY} ARGS -O ihex $<TARGET_FILE:${IN_NAME}> ${OUT_NAME} COMMENT "Converting to Intel hex format.")
endfunction(to_hex IN_NAME OUT_NAME)
