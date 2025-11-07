// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestAsserts_ZW_slave_network_info_files_sizes.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_slave_network_info_storage.c"
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

#define FILE_SIZE_RETURNROUTEINFO_EXPECTED              76
#define FILE_SIZE_ZW_VERSION_EXPECTED                   4
#define FILE_SIZE_RETURNROUTESDESTINATIONS_EXPECTED     5
#define FILE_SIZE_SLAVEINFO_EXPECTED                    8
#define FILE_SIZE_SLAVE_FILE_MAP_EXPECTED               30
#define FILE_SIZE_S2_KEYS_EXPECTED                      64
#define FILE_SIZE_S2_KEYCLASSES_ASSIGNED_EXPECTED       1
#define FILE_SIZE_S2_MPAN_EXPECTED                      24
#define FILE_SIZE_S2_SPAN_EXPECTED                      48

const uint16_t expectedFileSize[] = {FILE_SIZE_RETURNROUTEINFO_EXPECTED,
                                     FILE_SIZE_ZW_VERSION_EXPECTED,
                                     FILE_SIZE_RETURNROUTESDESTINATIONS_EXPECTED,
                                     FILE_SIZE_SLAVEINFO_EXPECTED,
                                     FILE_SIZE_SLAVE_FILE_MAP_EXPECTED,
                                     FILE_SIZE_S2_KEYS_EXPECTED,
                                     FILE_SIZE_S2_KEYCLASSES_ASSIGNED_EXPECTED,
                                     FILE_SIZE_S2_MPAN_EXPECTED,
                                     FILE_SIZE_S2_SPAN_EXPECTED};

const uint16_t curFileSize[] =      {FILE_SIZE_RETURNROUTEINFO,
                                     FILE_SIZE_ZW_VERSION,
                                     FILE_SIZE_RETURNROUTESDESTINATIONS,
                                     FILE_SIZE_SLAVEINFO,
                                     FILE_SIZE_SLAVE_FILE_MAP,
                                     FILE_SIZE_S2_KEYS,
                                     FILE_SIZE_S2_KEYCLASSES_ASSIGNED,
                                     FILE_SIZE_S2_MPAN,
                                     FILE_SIZE_S2_SPAN};

const char*    fileNameStr[] =      {"FILE_SIZE_RETURNROUTEINFO",
                                     "FILE_SIZE_ZW_VERSION",
                                     "FILE_SIZE_RETURNROUTESDESTINATIONS",
                                     "FILE_SIZE_SLAVEINFO",
                                     "FILE_SIZE_SLAVE_FILE_MAP",
                                     "FILE_SIZE_S2_KEYS",
                                     "FILE_SIZE_S2_KEYCLASSES_ASSIGNED",
                                     "FILE_SIZE_S2_MPAN",
                                     "FILE_SIZE_S2_SPAN"};

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

