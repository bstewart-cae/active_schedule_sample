add_library(test_common test_common.c)

target_include_directories(test_common 
  PUBLIC 
    "." 
    ${ZWAVE_API_DIR}
    ${ZAF_UTILDIR} 
)

target_link_libraries(test_common
  zpal
)
