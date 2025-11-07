// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestAsserts_ZW_controller_network_info_files_sizes.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_controller_network_info_storage.c"
#include "string.h"
#include <ZW_typedefs.h>
#include <stdio.h>
#include <string.h>
#include <SizeOf.h>
#include <unity.h>

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

#define FILE_SIZE_ZW_VERSION_EXPECTED               4
#define FILE_SIZE_PREFERREDREPEATERS_EXPECTED       0   // not used
#define FILE_SIZE_SUCNODELIST_LEGACY_v4_EXPECTED    1408
#define FILE_SIZE_SUCNODELIST_EXPECTED              176
#define FILE_SIZE_CONTROLLERINFO_EXPECTED           22
#define FILE_SIZE_NODE_STORAGE_EXIST_EXPECTED       29
#define FILE_SIZE_APP_ROUTE_LOCK_FLAG_EXPECTED      29
#define FILE_SIZE_ROUTE_SLAVE_SUC_FLAG_EXPECTED     29
#define FILE_SIZE_SUC_PENDING_UPDATE_FLAG_EXPECTED  29
#define FILE_SIZE_BRIDGE_NODE_FLAG_EXPECTED         29
#define FILE_SIZE_PENDING_DISCOVERY_FLAG_EXPECTED   29
#define FILE_SIZE_NODE_ROUTECACHE_EXIST_EXPECTED    29
#define FILE_SIZE_LRANGE_NODE_EXIST_EXPECTED        128
#define FILE_SIZE_S2_KEYS_EXPECTED                  64
#define FILE_SIZE_S2_KEYCLASSES_ASSIGNED_EXPECTED   1
#define FILE_SIZE_S2_MPAN_EXPECTED                  24
#define FILE_SIZE_S2_SPAN_EXPECTED                  48
#define FILE_SIZE_NODEINFO_EXPECTED                 140
#define FILE_SIZE_NODEINFO_LR_EXPECTED              150
#define FILE_SIZE_NODEROUTE_CACHE_EXPECTED          80

const uint16_t expectedFileSize[] = {FILE_SIZE_ZW_VERSION_EXPECTED,
                                     FILE_SIZE_SUCNODELIST_LEGACY_v4_EXPECTED,
                                     FILE_SIZE_SUCNODELIST_EXPECTED,
                                     FILE_SIZE_CONTROLLERINFO_EXPECTED,
                                     FILE_SIZE_NODE_STORAGE_EXIST_EXPECTED,
                                     FILE_SIZE_APP_ROUTE_LOCK_FLAG_EXPECTED,
                                     FILE_SIZE_ROUTE_SLAVE_SUC_FLAG_EXPECTED,
                                     FILE_SIZE_SUC_PENDING_UPDATE_FLAG_EXPECTED,
                                     FILE_SIZE_BRIDGE_NODE_FLAG_EXPECTED,
                                     FILE_SIZE_PENDING_DISCOVERY_FLAG_EXPECTED,
                                     FILE_SIZE_NODE_ROUTECACHE_EXIST_EXPECTED,
                                     FILE_SIZE_LRANGE_NODE_EXIST_EXPECTED,
                                     FILE_SIZE_NODEINFO_EXPECTED,
                                     FILE_SIZE_NODEINFO_LR_EXPECTED,
                                     FILE_SIZE_NODEROUTE_CACHE_EXPECTED
                                    };

const uint16_t curFileSize[] =      {FILE_SIZE_ZW_VERSION,
                                     FILE_SIZE_SUCNODELIST_LEGACY_v4,
                                     FILE_SIZE_SUCNODELIST,
                                     FILE_SIZE_CONTROLLERINFO,
                                     FILE_SIZE_NODE_STORAGE_EXIST,
                                     FILE_SIZE_APP_ROUTE_LOCK_FLAG,
                                     FILE_SIZE_ROUTE_SLAVE_SUC_FLAG,
                                     FILE_SIZE_SUC_PENDING_UPDATE_FLAG,
                                     FILE_SIZE_BRIDGE_NODE_FLAG,
                                     FILE_SIZE_PENDING_DISCOVERY_FLAG,
                                     FILE_SIZE_NODE_ROUTECACHE_EXIST,
                                     FILE_SIZE_LRANGE_NODE_EXIST,
                                     FILE_SIZE_NODEINFO,
                                     FILE_SIZE_NODEINFO_LR,
                                     FILE_SIZE_NODEROUTE_CACHE
                                    };

const char*  fileNameStr[] =        {"FILE_SIZE_ZW_VERSION",
                                     "FILE_SIZE_SUCNODELIST_LEGACY_v4",
                                     "FILE_SIZE_SUCNODELIST",
                                     "FILE_SIZE_CONTROLLERINFO",
                                     "FILE_SIZE_NODE_STORAGE_EXIST",
                                     "FILE_SIZE_APP_ROUTE_LOCK_FLAG",
                                     "FILE_SIZE_ROUTE_SLAVE_SUC_FLAG",
                                     "FILE_SIZE_SUC_PENDING_UPDATE_FLAG",
                                     "FILE_SIZE_BRIDGE_NODE_FLAG",
                                     "FILE_SIZE_PENDING_DISCOVERY_FLAG",
                                     "FILE_SIZE_NODE_ROUTECACHE_EXIST",
                                     "FILE_SIZE_LRANGE_NODE_EXIST",
                                     "FILE_SIZE_NODEINFO",
                                     "FILE_SIZE_NODEINFO_LR",
                                     "FILE_SIZE_NODEROUTE_CACHE"
                                    };

const char * const errMsg = "\r\n\r\nThe %s file sizes has changed.\r\n \
                             Expected file size = %d | Actual file size = %d\r\n \
                             If this is as intended, then this change must be inputted into the flash lifetime estimation spreadsheet\r\n \
                             found in location: https://confluence.silabs.com/pages/viewpage.action?spaceKey=ZWAVE&title=NVM3+lifetime+estimates\r\n \
                             - This file must be kept up to date and reviewed upon change.\r\n \
                               So please add the file to the Jira ticket for review and update the page above with the latest version of this file,\r\n \
                               and also increment the revision number. (The R value in filename). Thanks. Contact person: Saeed Ghasemi.\r\n\r\n";

/*
*  Monitors the NMV3 File sizes for change and trigger warning that the flash lifetime estimation sheets need to be updated with
*  the latest file sizes so that these sheets are maintained as the code develops.
*  The test loops through the expected sizes of the NVM3 files and compare it with the sizes of the files compiled in the code.
*  If the size of a file mismatch a message will be printed with expected file size value and the current value
*  Please refer to JIRA ticket https://jira.silabs.com/browse/SWPROT-6046 for more details
*/
void test_SlaveStorage_files_sizes(void)
{
  bool testFailed = false;
  for (uint8_t i=0; i < sizeof_array(expectedFileSize); i++) {
    if (expectedFileSize[i] != curFileSize[i]) {
      testFailed = true;
      printf(errMsg, fileNameStr[i], expectedFileSize[i], curFileSize[i]);
    }
  }
  TEST_ASSERT_MESSAGE(testFailed == 0, "Test failed, check the comment above");
}

