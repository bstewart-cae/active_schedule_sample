# The object library will be compiled once for all applications.
# This library must contain files that are generic for all applications.
zw_add_object_cc(NAME CC_UserCredential
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/config
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_UserCredential.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_user_credential_tx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_user_credential_io_weak.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_user_credential_manufacturer_validation_weak.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_user_credential_validation.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_user_credential_operations.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_user_credential_learn.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_user_credential_handlers_admin_code.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_user_credential_handlers_capabilities.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_user_credential_handlers_checksum.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_user_credential_handlers_database.c
  DEPENDS
    CC_Association
    CC_Notification
    CC_Supervision
    ZAF_EventDistributor_soc
    CRC
)

# FIXME: The CRC component is only needed for the test build. 
IF(CMAKE_BUILD_TYPE MATCHES Test)
  target_sources(CC_UserCredential PRIVATE ${COMPONENTS_DIR}/CRC/CRC.c)
ENDIF(CMAKE_BUILD_TYPE MATCHES Test)

# Source files listed in the interface library will be compiled together with
# the application source files. Hence, configuration files using definitions
# that are specific to an application must be listed here.
zw_add_interface_cc(NAME CC_UserCredential_config
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_user_credential_config_api.c
)

# This library is relevant for applications that are designed to store the
# credential data in the Z-Wave chip. In the case of products with an additional
# chip that stores the credential information, this library must not be used.
zw_add_interface_cc(NAME CC_UserCredential_nvm
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_user_credential_nvm.c
)

# Replaces the database implementation of the User Code Command Class with a
# wrapper accessing the PIN Codes stored by the User Credential Command Class.
zw_add_interface_cc(NAME CC_UserCode_UserCredential
  SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_user_code_database_user_credential.c
)

add_test_subdirectory(mocks)
add_test_subdirectory(tests)
