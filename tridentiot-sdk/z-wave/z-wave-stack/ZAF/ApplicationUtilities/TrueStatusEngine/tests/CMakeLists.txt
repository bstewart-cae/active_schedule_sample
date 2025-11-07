# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

include_directories( .. )

# Set the Test source files
set(test_ZAF_TSE_src
    test_ZAF_TSE.c
    ../ZAF_TSE.c
    ${ZAF_CCDIR}/Association/mocks/mock_association_plus.c
    ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
    ${ZAF_UTILDIR}/mocks/AppTimer_mock.c
)

# Add the test for ZAF TSE
add_unity_test(NAME test_ZAF_TSE
               FILES ${test_ZAF_TSE_src} 
               LIBRARIES mock
                         SwTimerMock
                         CC_CommonMock
                         DebugPrintMock
                         zaf_transport_layer_cmock
)

# Necessary includes
target_include_directories(test_ZAF_TSE PRIVATE
  ../
  ../config
  ${ZAF_UTILDIR}
  ${ZAF_UTILDIR}/TargetSpecificIncludes
  ${ZAF_CCDIR}/Association/inc
  ${ZAF_CCDIR}/Common
  ${ZAF_CONFIGDIR}/config
  ${ZAF_UNITTESTEXTERNALS}
  ${ZAF_CCDIR}/Association/config
)

# Required defines for the ZAF_TSE module (it uses association_plus.h)
target_compile_definitions(test_ZAF_TSE PUBLIC 
  ZAF_CONFIG_NUMBER_OF_END_POINTS=3
  CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT=3
  CC_ASSOCIATION_MAX_NODES_IN_GROUP=5
)