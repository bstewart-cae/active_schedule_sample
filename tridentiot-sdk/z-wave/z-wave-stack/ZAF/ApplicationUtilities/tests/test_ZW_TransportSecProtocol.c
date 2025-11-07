// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file test_ZW_TransportSecProtocol.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stdint.h>
#include <ZW_TransportSecProtocol.c>
#include <ZW_TransportEndpoint.c>
#include <unity.h>
#include <mock_control.h>
#include <ZAF_command_class_utils.c>

#define GENERIC_TYPE            GENERIC_TYPE_SWITCH_BINARY
#define SPECIFIC_TYPE           SPECIFIC_TYPE_POWER_SWITCH_BINARY
#define ZAF_CONFIG_DEVICE_OPTION_MASK   APPLICATION_NODEINFO_LISTENING

void test_TO6513_ApplicationSecureCommandsSupported(void)
{
  mock_t* pMock = NULL;

  uint8_t cmdClassListSecure[] =
  {
    COMMAND_CLASS_POWERLEVEL,
    COMMAND_CLASS_BATTERY
  };

  mock_calls_clear();

  /**
   * Structure includes application node information list's and device type.
   */
  app_node_information_t m_AppNIF =
  {
    NULL, 0,
    NULL, 0,
    cmdClassListSecure, sizeof(cmdClassListSecure)
  };

  uint8_t *pCmdListOut;
  uint8_t cmdListOutLength = 0;

  uint8_t node_id = 2;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = SECURITY_KEY_S2_UNAUTHENTICATED_BIT;

  Transport_OnApplicationInitSW(&m_AppNIF, NULL);

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = SECURITY_KEY_S2_UNAUTHENTICATED_BIT;

  ApplicationSecureCommandsSupported(SECURITY_KEY_S2_UNAUTHENTICATED_BIT, &pCmdListOut, &cmdListOutLength );
  UNITY_TEST_ASSERT_NOT_NULL(pCmdListOut, __LINE__, "ptr NULL!");
  UNITY_TEST_ASSERT_EQUAL_UINT8_ARRAY(m_AppNIF.cmdClassListSecure, pCmdListOut,  m_AppNIF.cmdClassListSecureCount, __LINE__, "list diff!" );
  UNITY_TEST_ASSERT_EQUAL_UINT8(m_AppNIF.cmdClassListSecureCount, cmdListOutLength,  __LINE__, "length diff!" );
}


/**
 * Unsecure node information list.
 * CHANGE THIS - Add all supported non-secure command classes here
 **/
static uint8_t cmdClassListNonSecureNotIncluded[] =
{
  COMMAND_CLASS_ZWAVEPLUS_INFO,
  COMMAND_CLASS_SECURITY,
  COMMAND_CLASS_SECURITY_2,
  COMMAND_CLASS_SUPERVISION,
  COMMAND_CLASS_MARK,
  COMMAND_CLASS_SWITCH_BINARY_V2
};

/**
 * Unsecure node information list Secure included.
 * CHANGE THIS - Add all supported non-secure command classes here
 **/
static uint8_t cmdClassListNonSecureIncludedSecure[] =
{
  COMMAND_CLASS_ZWAVEPLUS_INFO,
  COMMAND_CLASS_TRANSPORT_SERVICE_V2,
  COMMAND_CLASS_SECURITY,
  COMMAND_CLASS_SECURITY_2
};

/**
 * Secure node inforamtion list.
 * CHANGE THIS - Add all supported secure command classes here
 **/
uint8_t cmdClassListSecure[] =
{
    COMMAND_CLASS_SUPERVISION,
    COMMAND_CLASS_MARK,
    COMMAND_CLASS_SWITCH_BINARY_V2,
    COMMAND_CLASS_DOOR_LOCK
};

/**
 * Structure includes application node information list's and device type.
 */
app_node_information_t m_AppNIF =
{
  cmdClassListNonSecureNotIncluded, sizeof(cmdClassListNonSecureNotIncluded),
  cmdClassListNonSecureIncludedSecure, sizeof(cmdClassListNonSecureIncludedSecure),
  cmdClassListSecure, sizeof(cmdClassListSecure),
  ZAF_CONFIG_DEVICE_OPTION_MASK
};

 uint8_t ep1_cmdClassListNonSecureNotIncluded[] =
{
  COMMAND_CLASS_ZWAVEPLUS_INFO,
  COMMAND_CLASS_SECURITY,
  COMMAND_CLASS_SECURITY_2,
  COMMAND_CLASS_MARK,
  COMMAND_CLASS_SWITCH_BINARY_V2
};


uint8_t ep1_cmdClassListNonSecureIncludedSecure[] =
{
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_ASSOCIATION_V2,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2
};

code uint8_t ep1_cmdClassListSecure[] =
{
  COMMAND_CLASS_SUPERVISION,
  COMMAND_CLASS_TRANSPORT_SERVICE_V2,
  COMMAND_CLASS_MARK,
  COMMAND_CLASS_SWITCH_BINARY_V2
};

EP_NIF endpointsNIF[1] =
{  /*Endpoint 1*/
  { GENERIC_TYPE, SPECIFIC_TYPE,
    {{ep1_cmdClassListNonSecureNotIncluded, sizeof(ep1_cmdClassListNonSecureNotIncluded)},
       {{ep1_cmdClassListNonSecureIncludedSecure, sizeof(ep1_cmdClassListNonSecureIncludedSecure)},
        {ep1_cmdClassListSecure, sizeof(ep1_cmdClassListSecure)}
    }}
  }
};

void test_CommandsSuppported_nonsecure_notIncluded_root(void)
{
  mock_t* pMock = NULL;
  bTransportNID = 0;
  uint8_t ep = 0;

  mock_calls_clear();

  CMD_CLASS_LIST* pCmdClassList = NULL;

  uint8_t node_id = 0;

  Transport_OnApplicationInitSW(&m_AppNIF, NULL );

  pCmdClassList = GetCommandClassList((0 !=node_id), SECURITY_KEY_NONE, ep);

  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList, __LINE__, "pCmdClassList NULL!");
  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList->pList, __LINE__, "pCmdClassList->pList NULL!");
  UNITY_TEST_ASSERT_EQUAL_UINT8_ARRAY(pCmdClassList->pList, m_AppNIF.cmdClassListNonSecure,  m_AppNIF.cmdClassListNonSecureCount, __LINE__, "list diff!" );
  UNITY_TEST_ASSERT_EQUAL_UINT8(pCmdClassList->size, m_AppNIF.cmdClassListNonSecureCount,  __LINE__, "length diff!" );

  mock_calls_verify();
}


void test_CommandsSuppported_nonsecure_included_root(void)
{
  mock_t* pMock = NULL;
  bTransportNID = 0;
  uint8_t ep = 0;
  uint8_t secureMask = SECURITY_KEY_NONE_MASK;

  static uint8_t ccNonSecIncluded[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_SUPERVISION,
    COMMAND_CLASS_MARK,
    COMMAND_CLASS_SWITCH_BINARY_V2
  };

  mock_calls_clear();

  CMD_CLASS_LIST* pCmdClassList = NULL;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  uint8_t node_id = 2;

  Transport_OnApplicationInitSW(&m_AppNIF, NULL );

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  pCmdClassList = GetCommandClassList((0 !=node_id), SECURITY_KEY_NONE, ep);

  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList, __LINE__, "pCmdClassList NULL!");
  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList->pList, __LINE__, "pCmdClassList->pList NULL!");
  UNITY_TEST_ASSERT_EQUAL_UINT8_ARRAY(pCmdClassList->pList, m_AppNIF.cmdClassListNonSecure,  sizeof(ccNonSecIncluded), __LINE__, "list diff!" );
  UNITY_TEST_ASSERT_EQUAL_UINT8(pCmdClassList->size,  m_AppNIF.cmdClassListNonSecureCount,  __LINE__, "length diff!" );

  mock_calls_verify();
}



void test_CommandsSuppported_nosecure_S0S2_included_root(void)
{
  mock_t* pMock = NULL;
  bTransportNID = 0;
  security_key_t eKey = SECURITY_KEY_NONE;
  uint8_t ep = 0;
  uint8_t secureMask = SECURITY_KEY_S2_UNAUTHENTICATED_BIT | SECURITY_KEY_S0_BIT;

  mock_calls_clear();

  CMD_CLASS_LIST* pCmdClassList = NULL;

  uint8_t node_id = 2;
  
  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  Transport_OnApplicationInitSW(&m_AppNIF, NULL );

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  pCmdClassList = GetCommandClassList((0 !=node_id), eKey, ep);

  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList, __LINE__, "pCmdClassList NULL!");
  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList->pList, __LINE__, "pCmdClassList->pList NULL!");
  UNITY_TEST_ASSERT_EQUAL_UINT8_ARRAY(pCmdClassList->pList, m_AppNIF.cmdClassListNonSecureIncludedSecure,  m_AppNIF.cmdClassListNonSecureIncludedSecureCount, __LINE__, "list diff!" );
  UNITY_TEST_ASSERT_EQUAL_UINT8(pCmdClassList->size, m_AppNIF.cmdClassListNonSecureIncludedSecureCount,  __LINE__, "length diff!" );

  mock_calls_verify();
}

void test_CommandsSuppported_nonsecure_S2_included_root(void)
{
  mock_t* pMock = NULL;
  bTransportNID = 0;
  security_key_t eKey = SECURITY_KEY_NONE;
  uint8_t ep = 0;
  uint8_t secureMask = SECURITY_KEY_S2_UNAUTHENTICATED_BIT;

  mock_calls_clear();

  CMD_CLASS_LIST* pCmdClassList = NULL;

  uint8_t node_id = 2;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  Transport_OnApplicationInitSW(&m_AppNIF, NULL );

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  pCmdClassList = GetCommandClassList((0 !=node_id), eKey, ep);

  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList, __LINE__, "pCmdClassList NULL!");
  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList->pList, __LINE__, "pCmdClassList->pList NULL!");
  UNITY_TEST_ASSERT_EQUAL_UINT8_ARRAY(pCmdClassList->pList, m_AppNIF.cmdClassListNonSecureIncludedSecure,  m_AppNIF.cmdClassListNonSecureIncludedSecureCount, __LINE__, "list diff!" );
  UNITY_TEST_ASSERT_EQUAL_UINT8(pCmdClassList->size, m_AppNIF.cmdClassListNonSecureIncludedSecureCount,  __LINE__, "length diff!" );

  mock_calls_verify();
}


void test_CommandsSuppported_S0secure_S0S2_included_root(void)
{
  mock_t* pMock = NULL;
  bTransportNID = 0;
  security_key_t eKey = SECURITY_KEY_S0;
  uint8_t ep = 0;
  uint8_t secureMask = SECURITY_KEY_S2_UNAUTHENTICATED_BIT | SECURITY_KEY_S0_BIT;

  mock_calls_clear();

  CMD_CLASS_LIST* pCmdClassList = NULL;

  uint8_t node_id = 2;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  Transport_OnApplicationInitSW(&m_AppNIF, NULL );

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;
  pCmdClassList = GetCommandClassList((0 !=node_id), eKey, ep);

  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList, __LINE__, "pCmdClassList NULL!");
  UNITY_TEST_ASSERT_NULL(pCmdClassList->pList, __LINE__, "pCmdClassList->pList not NULL!");

  mock_calls_verify();
}


void test_CommandsSuppported_S2secure_S0S2_included_root(void)
{
  mock_t* pMock = NULL;
  bTransportNID = 0;
  security_key_t eKey = SECURITY_KEY_S2_UNAUTHENTICATED;
  uint8_t ep = 0;
  uint8_t secureMask = SECURITY_KEY_S2_UNAUTHENTICATED_BIT | SECURITY_KEY_S0_BIT;

  uint8_t cmdClassListSecure_s2[] =
  {
      COMMAND_CLASS_SUPERVISION
  };

  mock_calls_clear();

  CMD_CLASS_LIST* pCmdClassList = NULL;

  uint8_t node_id = 2;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  Transport_OnApplicationInitSW(&m_AppNIF, NULL );

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;
  pCmdClassList = GetCommandClassList((0 !=node_id), eKey, ep);

  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList, __LINE__, "pCmdClassList NULL!");
  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList->pList, __LINE__, "pCmdClassList->pList NULL!");
  UNITY_TEST_ASSERT_EQUAL_UINT8_ARRAY(pCmdClassList->pList, cmdClassListSecure_s2,  sizeof(cmdClassListSecure_s2), __LINE__, "list diff!" );
  UNITY_TEST_ASSERT_EQUAL_UINT8(pCmdClassList->size, sizeof(cmdClassListSecure_s2),  __LINE__, "length diff!" );

  mock_calls_verify();
}


void test_CommandsSuppported_S2UNAUTHENTICATED_secure_S2_ACCESS_included_root(void)
{
  mock_t* pMock = NULL;
  bTransportNID = 0;
  security_key_t eKey = SECURITY_KEY_S2_UNAUTHENTICATED;
  uint8_t secureMask = SECURITY_KEY_S2_ACCESS_BIT;
  uint8_t ep = 0;

  mock_calls_clear();

  CMD_CLASS_LIST* pCmdClassList = NULL;

  uint8_t node_id = 2;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  Transport_OnApplicationInitSW(&m_AppNIF, NULL );

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;
  pCmdClassList = GetCommandClassList((0 !=node_id), eKey, ep);

  UNITY_TEST_ASSERT_NULL(pCmdClassList->pList, __LINE__, "pCmdClassList->pList is NULL!");
  UNITY_TEST_ASSERT_EQUAL_UINT8(pCmdClassList->size, 0,  __LINE__, "length diff!" );

  mock_calls_verify();
}

/** Endpoint test*/

void test_CommandsSuppported_nonsecure_included_EP1(void)
{
  mock_t* pMock = NULL;
  bTransportNID = 0;
  uint8_t ep = 1;
  security_key_t eKey = SECURITY_KEY_NONE;
  uint8_t secureMask = SECURITY_KEY_NONE_MASK;

  EP_NIF endpointsNIF[1] =
  {  /*Endpoint 1*/
    { GENERIC_TYPE, SPECIFIC_TYPE,
      {{ep1_cmdClassListNonSecureNotIncluded, sizeof(ep1_cmdClassListNonSecureNotIncluded)},
         {{ep1_cmdClassListNonSecureIncludedSecure, sizeof(ep1_cmdClassListNonSecureIncludedSecure)},
          {ep1_cmdClassListSecure, sizeof(ep1_cmdClassListSecure)}
      }}
    }
  };

  mock_calls_clear();

  CMD_CLASS_LIST* pCmdClassList = NULL;

  uint8_t node_id = 2;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  Transport_OnApplicationInitSW(&m_AppNIF, NULL );
  Transport_AddEndpointSupport(NULL, endpointsNIF, 1);

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;
  //EP mock call
  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  pCmdClassList = GetCommandClassList((0 !=node_id), eKey, ep);

  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList, __LINE__, "pCmdClassList NULL!");
  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList->pList, __LINE__, "pCmdClassList->pList NULL!");
  UNITY_TEST_ASSERT_EQUAL_UINT8_ARRAY(pCmdClassList->pList, endpointsNIF[0].CmdClass3List.unsecList.pList,  endpointsNIF[0].CmdClass3List.unsecList.size, __LINE__, "list diff!" );
  UNITY_TEST_ASSERT_EQUAL_UINT8(pCmdClassList->size,  endpointsNIF[0].CmdClass3List.unsecList.size,  __LINE__, "length diff!" );

  mock_calls_verify();
}

void test_CommandsSuppported_nosecure_S0S2_included_EP1(void)
{
  mock_t* pMock = NULL;
  bTransportNID = 0;
  security_key_t eKey = SECURITY_KEY_NONE;
  uint8_t ep = 1;
  uint8_t secureMask = SECURITY_KEY_S2_UNAUTHENTICATED_BIT | SECURITY_KEY_S0_BIT;

  mock_calls_clear();

  CMD_CLASS_LIST* pCmdClassList = NULL;

  uint8_t node_id = 2;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  Transport_OnApplicationInitSW(&m_AppNIF, NULL );
  Transport_AddEndpointSupport(NULL, endpointsNIF, 1);

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  pCmdClassList = GetCommandClassList((0 !=node_id), eKey, ep);

  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList, __LINE__, "pCmdClassList NULL!");
  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList->pList, __LINE__, "pCmdClassList->pList NULL!");
  UNITY_TEST_ASSERT_EQUAL_UINT8_ARRAY(pCmdClassList->pList, endpointsNIF[0].CmdClass3List.sec.unsecList.pList,  endpointsNIF[0].CmdClass3List.sec.unsecList.size, __LINE__, "list diff!" );
  UNITY_TEST_ASSERT_EQUAL_UINT8(pCmdClassList->size, endpointsNIF[0].CmdClass3List.sec.unsecList.size,  __LINE__, "length diff!" );

  mock_calls_verify();
}

void test_CommandsSuppported_nonsecure_S2_included_EP1(void)
{
  mock_t* pMock = NULL;
  bTransportNID = 0;
  security_key_t eKey = SECURITY_KEY_NONE;
  uint8_t ep = 1;
  uint8_t secureMask = SECURITY_KEY_S2_UNAUTHENTICATED_BIT;

  mock_calls_clear();

  CMD_CLASS_LIST* pCmdClassList = NULL;

  uint8_t node_id = 2;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  Transport_OnApplicationInitSW(&m_AppNIF, NULL );
  Transport_AddEndpointSupport(NULL, endpointsNIF, 1);

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  pCmdClassList = GetCommandClassList((0 !=node_id), eKey, ep);

  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList, __LINE__, "pCmdClassList NULL!");
  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList->pList, __LINE__, "pCmdClassList->pList NULL!");
  UNITY_TEST_ASSERT_EQUAL_UINT8_ARRAY(pCmdClassList->pList, endpointsNIF[0].CmdClass3List.sec.unsecList.pList, endpointsNIF[0].CmdClass3List.sec.unsecList.size, __LINE__, "list diff!" );
  UNITY_TEST_ASSERT_EQUAL_UINT8(pCmdClassList->size, endpointsNIF[0].CmdClass3List.sec.unsecList.size,  __LINE__, "length diff!" );

  mock_calls_verify();
}

void test_CommandsSuppported_nonsecure_S0_included_EP1(void)
{
  mock_t* pMock = NULL;
  bTransportNID = 0;
  security_key_t eKey = SECURITY_KEY_NONE;
  uint8_t ep = 1;
  uint8_t secureMask = SECURITY_KEY_S0_BIT;

  mock_calls_clear();

  CMD_CLASS_LIST* pCmdClassList = NULL;

  uint8_t node_id = 2;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  Transport_OnApplicationInitSW(&m_AppNIF, NULL );
  Transport_AddEndpointSupport(NULL, endpointsNIF, 1);

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  pCmdClassList = GetCommandClassList((0 !=node_id), eKey, ep);

  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList, __LINE__, "pCmdClassList NULL!");
  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList->pList, __LINE__, "pCmdClassList->pList NULL!");
  UNITY_TEST_ASSERT_EQUAL_UINT8_ARRAY(pCmdClassList->pList, endpointsNIF[0].CmdClass3List.sec.unsecList.pList,  endpointsNIF[0].CmdClass3List.sec.unsecList.size, __LINE__, "list diff!" );
  UNITY_TEST_ASSERT_EQUAL_UINT8(pCmdClassList->size, endpointsNIF[0].CmdClass3List.sec.unsecList.size,  __LINE__, "length diff!" );

  mock_calls_verify();
}

void test_CommandsSuppported_S0secure_S0S2_included_EP1(void)
{
  mock_t* pMock = NULL;
  bTransportNID = 0;
  security_key_t eKey = SECURITY_KEY_S0;
  uint8_t ep = 1;
  uint8_t secureMask = SECURITY_KEY_S2_UNAUTHENTICATED_BIT | SECURITY_KEY_S0_BIT;

  mock_calls_clear();

  CMD_CLASS_LIST* pCmdClassList = NULL;

  uint8_t node_id = 2;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  Transport_OnApplicationInitSW(&m_AppNIF, NULL );
  Transport_AddEndpointSupport(NULL, endpointsNIF, 1);

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  pCmdClassList = GetCommandClassList((0 !=node_id), eKey, ep);

  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList, __LINE__, "pCmdClassList NULL!");
  UNITY_TEST_ASSERT_NULL(pCmdClassList->pList, __LINE__, "pCmdClassList->pList not NULL!");

  mock_calls_verify();
}


void test_CommandsSuppported_S2secure_S0S2_included_EP1(void)
{
  mock_t* pMock = NULL;
  bTransportNID = 0;
  security_key_t eKey = SECURITY_KEY_S2_UNAUTHENTICATED;
  uint8_t ep = 1;
  uint8_t secureMask = SECURITY_KEY_S2_UNAUTHENTICATED_BIT | SECURITY_KEY_S0_BIT;

  uint8_t cmdClassListSecure_s2[] =
  {
      COMMAND_CLASS_SUPERVISION,
      COMMAND_CLASS_TRANSPORT_SERVICE_V2
  };

  mock_calls_clear();

  CMD_CLASS_LIST* pCmdClassList = NULL;

  uint8_t node_id = 2;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  Transport_OnApplicationInitSW(&m_AppNIF, NULL );
  Transport_AddEndpointSupport(NULL, endpointsNIF, 1);

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;
  pCmdClassList = GetCommandClassList((0 !=node_id), eKey, ep);

  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList, __LINE__, "pCmdClassList NULL!");
  UNITY_TEST_ASSERT_NOT_NULL(pCmdClassList->pList, __LINE__, "pCmdClassList->pList NULL!");
  UNITY_TEST_ASSERT_EQUAL_UINT8_ARRAY(pCmdClassList->pList, cmdClassListSecure_s2,  sizeof(cmdClassListSecure_s2), __LINE__, "list diff!" );
  UNITY_TEST_ASSERT_EQUAL_UINT8(pCmdClassList->size, sizeof(cmdClassListSecure_s2),  __LINE__, "length diff!" );

  mock_calls_verify();
}



uint8_t CommandclassSupported_nosecure_cmdclass_nonsecureIncl_root(uint8_t checkCmdClass)
{
  mock_t* pMock = NULL;
  bTransportNID = 1;
  security_key_t eKey = SECURITY_KEY_NONE;
  uint8_t secureMask = SECURITY_KEY_NONE_MASK;
  //uint8_t checkCmdClass = COMMAND_CLASS_ZWAVEPLUS_INFO;
  mock_calls_clear();

  uint8_t node_id = 2;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  Transport_OnApplicationInitSW(&m_AppNIF, NULL );

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  uint8_t cmdLegal = CmdClassSupported(eKey,
                                checkCmdClass,
                                m_AppInfo.u.pNifs->cmdClassListSecure,
                                m_AppInfo.u.pNifs->cmdClassListSecureCount,
                                m_AppInfo.activeNonsecureList.pList,
                                m_AppInfo.activeNonsecureList.size);



  mock_calls_verify();
  return cmdLegal;
}


void test_CommandclassSupported_nosecure_cmdclass_nonsecureIncl_root(void)
{
  /** COMMAND_CLASS_ZWAVEPLUS_INFO supported none secure included*/
  uint8_t cmdLegal = CommandclassSupported_nosecure_cmdclass_nonsecureIncl_root( COMMAND_CLASS_ZWAVEPLUS_INFO);
  UNITY_TEST_ASSERT_EQUAL_UINT8(true, cmdLegal,  __LINE__, "nonsecureIncl_root, COMMAND_CLASS_ZWAVEPLUS_INFO should not legal!");

  /** COMMAND_CLASS_SWITCH_BINARY_V2 supported none secure included*/
  cmdLegal = CommandclassSupported_nosecure_cmdclass_nonsecureIncl_root( COMMAND_CLASS_SWITCH_BINARY_V2);
  UNITY_TEST_ASSERT_EQUAL_UINT8(true, cmdLegal,  __LINE__, "nonsecureIncl_root, COMMAND_CLASS_SWITCH_BINARY_V2 should not legal!");

  /** COMMAND_CLASS_TRANSPORT_SERVICE_V2 supported secure included*/
  cmdLegal = CommandclassSupported_nosecure_cmdclass_nonsecureIncl_root( COMMAND_CLASS_TRANSPORT_SERVICE_V2);
  UNITY_TEST_ASSERT_EQUAL_UINT8(false, cmdLegal,  __LINE__, "nonsecureIncl_root, COMMAND_CLASS_TRANSPORT_SERVICE_V2 should not legal!");

  /** COMMAND_CLASS_DOOR_LOCK supported secure included and should fail*/
  cmdLegal = CommandclassSupported_nosecure_cmdclass_nonsecureIncl_root( COMMAND_CLASS_DOOR_LOCK);
  UNITY_TEST_ASSERT_EQUAL_UINT8(false, cmdLegal,  __LINE__, "nonsecureIncl_root, COMMAND_CLASS_DOOR_LOCK should not legal!");
}


uint8_t CommandclassSupported_nosecure_cmdclass_notInclroot(uint8_t checkCmdClass)
{
  mock_t* pMock = NULL;
  bTransportNID = 0;
  security_key_t eKey = SECURITY_KEY_NONE;
  uint8_t secureMask = SECURITY_KEY_S2_MASK;
  //uint8_t checkCmdClass = COMMAND_CLASS_ZWAVEPLUS_INFO;
  mock_calls_clear();

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  uint8_t node_id = 0;

  Transport_OnApplicationInitSW(&m_AppNIF, NULL );


  uint8_t cmdLegal = CmdClassSupported(eKey,
                                checkCmdClass,
                                m_AppInfo.u.pNifs->cmdClassListSecure,
                                m_AppInfo.u.pNifs->cmdClassListSecureCount,
                                m_AppInfo.activeNonsecureList.pList,
                                m_AppInfo.activeNonsecureList.size);

  mock_calls_verify();
  return cmdLegal;
}

void test_CommandclassSupported_nosecure_cmdclass_notInclroot(void)
{
  /** COMMAND_CLASS_ZWAVEPLUS_INFO supported none secure included*/
  uint8_t cmdLegal = CommandclassSupported_nosecure_cmdclass_notInclroot( COMMAND_CLASS_ZWAVEPLUS_INFO);
  UNITY_TEST_ASSERT_EQUAL_UINT8(true, cmdLegal,  __LINE__, "notInclroot, COMMAND_CLASS_ZWAVEPLUS_INFO should not legal!");

  /** COMMAND_CLASS_SWITCH_BINARY_V2 supported none secure included*/
  cmdLegal = CommandclassSupported_nosecure_cmdclass_notInclroot( COMMAND_CLASS_SWITCH_BINARY_V2);
  UNITY_TEST_ASSERT_EQUAL_UINT8(true, cmdLegal,  __LINE__, "notInclroot, COMMAND_CLASS_SWITCH_BINARY_V2 should not legal!");

  /** COMMAND_CLASS_TRANSPORT_SERVICE_V2 supported secure included*/
  cmdLegal = CommandclassSupported_nosecure_cmdclass_notInclroot( COMMAND_CLASS_TRANSPORT_SERVICE_V2);
  UNITY_TEST_ASSERT_EQUAL_UINT8(false, cmdLegal,  __LINE__, "notInclroot, COMMAND_CLASS_TRANSPORT_SERVICE_V2 should not legal!");

  /** COMMAND_CLASS_DOOR_LOCK supported secure included and should fail*/
  cmdLegal = CommandclassSupported_nosecure_cmdclass_notInclroot( COMMAND_CLASS_DOOR_LOCK);
  UNITY_TEST_ASSERT_EQUAL_UINT8(false, cmdLegal,  __LINE__, "notInclroot, COMMAND_CLASS_DOOR_LOCK should not legal!");

}

void test_CommandclassSupported_secure_cmdclass_secureIncl_root(void)
{
  mock_t* pMock = NULL;
  bTransportNID = 1;
  security_key_t eKey = SECURITY_KEY_S0;
  uint8_t secureMask = SECURITY_KEY_S0_BIT;

  uint8_t checkCmdClass = COMMAND_CLASS_TRANSPORT_SERVICE_V2;
  mock_calls_clear();

  uint8_t node_id = 2;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  Transport_OnApplicationInitSW(&m_AppNIF, NULL );


  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  uint8_t cmdLegal = CmdClassSupported(eKey,
                                checkCmdClass,
                                m_AppInfo.u.pNifs->cmdClassListSecure,
                                m_AppInfo.u.pNifs->cmdClassListSecureCount,
                                m_AppInfo.activeNonsecureList.pList,
                                m_AppInfo.activeNonsecureList.size);

  TEST_ASSERT_TRUE_MESSAGE( cmdLegal, "COMMAND_CLASS_TRANSPORT_SERVICE_V2 should be legal!");

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  //Check not legal cmdclass
  checkCmdClass = COMMAND_CLASS_SWITCH_BINARY_V2;
  cmdLegal = CmdClassSupported(eKey,
                                checkCmdClass,
                                m_AppInfo.u.pNifs->cmdClassListSecure,
                                m_AppInfo.u.pNifs->cmdClassListSecureCount,
                                m_AppInfo.activeNonsecureList.pList,
                                m_AppInfo.activeNonsecureList.size);

  TEST_ASSERT_TRUE_MESSAGE( cmdLegal, "COMMAND_CLASS_SWITCH_BINARY_V2 should not be legal!");

  mock_calls_verify();
}

void test_CommandclassSupported_nosecure_cmdclass_secureIncl_root(void)
{
  mock_t* pMock = NULL;
  bTransportNID = 1;
  security_key_t eKey = SECURITY_KEY_NONE;
  uint8_t secureMask = SECURITY_KEY_S0_BIT;

  uint8_t checkCmdClass = COMMAND_CLASS_TRANSPORT_SERVICE_V2;
  mock_calls_clear();

  uint8_t node_id = 2;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  Transport_OnApplicationInitSW(&m_AppNIF, NULL );


  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  uint8_t cmdLegal = CmdClassSupported(eKey,
                                checkCmdClass,
                                m_AppInfo.u.pNifs->cmdClassListSecure,
                                m_AppInfo.u.pNifs->cmdClassListSecureCount,
                                m_AppInfo.activeNonsecureList.pList,
                                m_AppInfo.activeNonsecureList.size);

  TEST_ASSERT_TRUE_MESSAGE( cmdLegal, "COMMAND_CLASS_TRANSPORT_SERVICE_V2 should be legal!");

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  //Check not legal cmdclass
  checkCmdClass = COMMAND_CLASS_SWITCH_BINARY_V2;
  cmdLegal = CmdClassSupported(eKey,
                                checkCmdClass,
                                m_AppInfo.u.pNifs->cmdClassListSecure,
                                m_AppInfo.u.pNifs->cmdClassListSecureCount,
                                m_AppInfo.activeNonsecureList.pList,
                                m_AppInfo.activeNonsecureList.size);

  TEST_ASSERT_FALSE_MESSAGE( cmdLegal, "COMMAND_CLASS_SWITCH_BINARY_V2 should not be legal!");

  mock_calls_verify();
}


void test_CommandclassSupported_S2_UNAUTHENTICATED_cmdclass_secureIncl_S2_ACCESS_root(void)
{
  mock_t* pMock = NULL;
  bTransportNID = 1;
  security_key_t eKey = SECURITY_KEY_S2_UNAUTHENTICATED;
  uint8_t secureMask = SECURITY_KEY_S2_ACCESS_BIT;

  uint8_t checkCmdClass = COMMAND_CLASS_TRANSPORT_SERVICE_V2;
  mock_calls_clear();

  uint8_t node_id = 2;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  Transport_OnApplicationInitSW(&m_AppNIF, NULL );


  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  uint8_t cmdLegal = CmdClassSupported(eKey,
                                checkCmdClass,
                                m_AppInfo.u.pNifs->cmdClassListSecure,
                                m_AppInfo.u.pNifs->cmdClassListSecureCount,
                                m_AppInfo.activeNonsecureList.pList,
                                m_AppInfo.activeNonsecureList.size);

  TEST_ASSERT_TRUE_MESSAGE(cmdLegal, "COMMAND_CLASS_TRANSPORT_SERVICE_V2 should be legal!");

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  //Check not legal cmdclass
  checkCmdClass = COMMAND_CLASS_SWITCH_BINARY_V2;
  cmdLegal = CmdClassSupported(eKey,
                                checkCmdClass,
                                m_AppInfo.u.pNifs->cmdClassListSecure,
                                m_AppInfo.u.pNifs->cmdClassListSecureCount,
                                m_AppInfo.activeNonsecureList.pList,
                                m_AppInfo.activeNonsecureList.size);

  TEST_ASSERT_FALSE_MESSAGE(cmdLegal, "COMMAND_CLASS_SWITCH_BINARY_V2 should no be legal, frame secure level SECURITY_KEY_S2_UNAUTHENTICATED and device SECURITY_KEY_S2_ACCESS");

  mock_calls_verify();
}

void test_CommandclassSupported_S0_cmdclass_secureIncl_S2_ACCESS_root(void)
{
  mock_t* pMock = NULL;
  bTransportNID = 1;
  security_key_t eKey = SECURITY_KEY_S0;
  uint8_t secureMask = SECURITY_KEY_S2_ACCESS_BIT;
  uint8_t checkCmdClass = COMMAND_CLASS_TRANSPORT_SERVICE_V2;
  mock_calls_clear();

  uint8_t node_id = 2;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  Transport_OnApplicationInitSW(&m_AppNIF, NULL );



  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  uint8_t cmdLegal = CmdClassSupported(eKey,
                                checkCmdClass,
                                m_AppInfo.u.pNifs->cmdClassListSecure,
                                m_AppInfo.u.pNifs->cmdClassListSecureCount,
                                m_AppInfo.activeNonsecureList.pList,
                                m_AppInfo.activeNonsecureList.size);

  TEST_ASSERT_TRUE_MESSAGE(cmdLegal, "COMMAND_CLASS_TRANSPORT_SERVICE_V2 should be legal!");

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  //Check not legal cmdclass
  checkCmdClass = COMMAND_CLASS_SWITCH_BINARY_V2;
  cmdLegal = CmdClassSupported(eKey,
                                checkCmdClass,
                                m_AppInfo.u.pNifs->cmdClassListSecure,
                                m_AppInfo.u.pNifs->cmdClassListSecureCount,
                                m_AppInfo.activeNonsecureList.pList,
                                m_AppInfo.activeNonsecureList.size);

  TEST_ASSERT_FALSE_MESSAGE(cmdLegal, "COMMAND_CLASS_SWITCH_BINARY_V2 should no be legal, frame secure level SECURITY_KEY_S2_UNAUTHENTICATED and device SECURITY_KEY_S2_ACCESS");

  mock_calls_verify();
}


void test_CommandclassSupported_S2_ACCESS_cmdclass_secureIncl_S2_UNAUTHENTICATED_root(void)
{
  mock_t* pMock = NULL;
  bTransportNID = 1;
  security_key_t eKey = SECURITY_KEY_S2_ACCESS;
  uint8_t secureMask = SECURITY_KEY_S2_UNAUTHENTICATED_BIT;
  uint8_t checkCmdClass = COMMAND_CLASS_TRANSPORT_SERVICE_V2;
  mock_calls_clear();

  uint8_t node_id = 2;

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  Transport_OnApplicationInitSW(&m_AppNIF, NULL );


  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  uint8_t cmdLegal = CmdClassSupported(eKey,
                                checkCmdClass,
                                m_AppInfo.u.pNifs->cmdClassListSecure,
                                m_AppInfo.u.pNifs->cmdClassListSecureCount,
                                m_AppInfo.activeNonsecureList.pList,
                                m_AppInfo.activeNonsecureList.size);

  TEST_ASSERT_TRUE_MESSAGE(cmdLegal, "COMMAND_CLASS_TRANSPORT_SERVICE_V2 should be legal!");

  mock_call_expect(TO_STR(ZW_GetSecurityKeys), &pMock);
  pMock->return_code.v = secureMask;

  //Check not legal cmdclass
  checkCmdClass = COMMAND_CLASS_SWITCH_BINARY_V2;
  cmdLegal = CmdClassSupported(eKey,
                                checkCmdClass,
                                m_AppInfo.u.pNifs->cmdClassListSecure,
                                m_AppInfo.u.pNifs->cmdClassListSecureCount,
                                m_AppInfo.activeNonsecureList.pList,
                                m_AppInfo.activeNonsecureList.size);

  TEST_ASSERT_FALSE_MESSAGE(cmdLegal, "COMMAND_CLASS_SWITCH_BINARY_V2 should no be legal, frame secure level SECURITY_KEY_S2_UNAUTHENTICATED and device SECURITY_KEY_S2_ACCESS");

  mock_calls_verify();
}

