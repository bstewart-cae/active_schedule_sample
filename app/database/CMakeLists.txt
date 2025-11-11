# SPDX-FileCopyrightText: Card Access Engineering, LLC <https://www.caengineering.com/>
# SPDX-License-Identifier: BSD-3-Clause

list(APPEND ZW_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/app_database.h
    ${CMAKE_CURRENT_LIST_DIR}/database_properties.h
)

list(APPEND APP_SOURCES 
    ${CMAKE_CURRENT_LIST_DIR}/app_database.c
    ${CMAKE_CURRENT_LIST_DIR}/app_database_u3c.c
    ${CMAKE_CURRENT_LIST_DIR}/app_database_ascc.c
)
