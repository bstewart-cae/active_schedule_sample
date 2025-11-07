/// ****************************************************************************
/// @file setup_common_cli.c
///
/// @brief Setting up common application test CLI
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include <SizeOf.h>
#include "zpal_uart.h"
#include "zpal_misc.h"
#include "zpal_misc_private.h"
#include "tr_cli.h"
#include "Assert.h" // NOSONAR
#include "ZAF_Common_interface.h"
#include "setup_common_cli.h"
#include "MfgTokens.h"
#include "events.h"
#include "zaf_event_distributor_soc.h"

//#define DEBUGPRINT // NOSONAR
#include "DebugPrint.h"

// Cli funtion prototypes
void tr_cli_common_print(const char* message);
static int cli_cmd_info(int  argc, char *argv[]);
static int cli_cmd_default(int  argc, char *argv[]);
static int cli_cmd_version(int  argc, char *argv[]);
static int cli_cmd_hash(int  argc, char *argv[]);
static int cli_cmd_qr(int  argc, char *argv[]);
static int cli_cmd_id(int  argc, char *argv[]);
static int cli_cmd_learn(int  argc, char *argv[]);
static int cli_cmd_reset_soft(int  argc, char *argv[]);
static int cli_cmd_reset_get(int  argc, char *argv[]);
static int cli_cmd_reset_clear(int  argc, char *argv[]);

extern TR_CLI_COMMAND_TABLE(app_specific_commands);

extern void cli_cmd_wait_for_buffer_write(void);

TR_CLI_COMMAND_TABLE(reset_commands) =
{
  { "soft",   cli_cmd_reset_soft,     "Perform a soft reset of the chip"             },
  { "get",    cli_cmd_reset_get,      "Get the number of resets since last POR"      },
  { "clear",  cli_cmd_reset_clear,    "Clear the resets count since last POR"        },
  TR_CLI_COMMAND_TABLE_END
};

// Commands for the common CLI
TR_CLI_COMMAND_TABLE(general_commands) =
{
    { "info",         cli_cmd_info,        "Print device information"                     },
    { "default",      cli_cmd_default,     "Reset to factory default"                     },
    { "version",      cli_cmd_version,     "Display the application version"              },
    { "hash",         cli_cmd_hash,        "Display git commit hash"                      },
    { "qr",           cli_cmd_qr,          "Show smart start QR code"                     },
    { "id",           cli_cmd_id,          "Get nodeID"                                   },
    { "learn",        cli_cmd_learn,       "Toggle learn mode"                            },
    { "reset",        TR_CLI_SUB_COMMANDS, TR_CLI_SUB_COMMAND_TABLE(reset_commands)       },
    { "application",  TR_CLI_SUB_COMMANDS, TR_CLI_SUB_COMMAND_TABLE(app_specific_commands)},
    TR_CLI_COMMAND_TABLE_END
};

static int cli_cmd_info(__attribute__((unused)) int argc, __attribute__((unused)) char *argv[])
{
  zpal_product_id_t product_id;
  zpal_get_product_id(&product_id);
  tr_cli_common_printf("Manufacturer ID : 0x%04x\n", product_id.app_manufacturer_id);
  tr_cli_common_printf("Product ID      : %u\n", product_id.app_product_id);
  tr_cli_common_printf("Product type ID : %u\n", product_id.app_product_type);
  tr_cli_common_printf("Generic type    : %u\n", product_id.generic_type);
  tr_cli_common_printf("Specific type   : %u\n", product_id.specyfic_type);
  return 0;
}

static int cli_cmd_reset_soft(__attribute__((unused)) int argc, __attribute__((unused)) char *argv[])
{
  tr_cli_common_print("Ok\n");
  cli_cmd_wait_for_buffer_write();
  zpal_reboot_with_info(0x462, ZPAL_RESET_INFO_DEFAULT);
  return 0;
}

static int cli_cmd_default(__attribute__((unused)) int  argc, __attribute__((unused)) char *argv[])
{
  tr_cli_common_print("Ok\n");
  zaf_event_distributor_enqueue_app_event_from_isr(EVENT_SYSTEM_RESET);
  return 0;
}

static int cli_cmd_version(__attribute__((unused)) int argc, __attribute__((unused)) char *argv[])
{
  tr_cli_common_printf("Application version : %u.%u.%u\n",
                      zpal_get_app_version_major(),
                      zpal_get_app_version_minor(),
                      zpal_get_app_version_patch());
  return 0;
}

static int cli_cmd_hash(__attribute__((unused)) int argc, __attribute__((unused)) char *argv[])
{
  tr_cli_common_printf("%s\n", GIT_HASH_ID);
  return 0;
}

static int cli_cmd_qr(__attribute__((unused)) int argc, __attribute__((unused)) char *argv[])
{
  static uint8_t qr_code[TOKEN_MFG_ZW_QR_CODE_SIZE] = {0};
  ZW_GetMfgTokenData(qr_code, TOKEN_MFG_ZW_QR_CODE_ID, TOKEN_MFG_ZW_QR_CODE_SIZE);
  tr_cli_common_printf((char*)qr_code);
  tr_cli_common_print("\n");
  return 0;
}

static int cli_cmd_id(__attribute__((unused)) int argc, __attribute__((unused)) char *argv[])
{
  node_id_t node_id = ZAF_GetNodeID();
  tr_cli_common_printf("%u\n", node_id);
  return 0;
}

static int cli_cmd_learn(__attribute__((unused)) int  argc, __attribute__((unused)) char *argv[])
{
  tr_cli_common_print("Ok\n");
  zaf_event_distributor_enqueue_app_event_from_isr(EVENT_SYSTEM_LEARNMODE_TOGGLE);
  return 0;
}

static int cli_cmd_reset_get(__attribute__((unused)) int argc, __attribute__((unused)) char *argv[])
{
  uint32_t restarts;

  restarts = zpal_get_restarts();
  tr_cli_common_printf("%u\n", restarts);
  return 0;
}

static int cli_cmd_reset_clear(__attribute__((unused)) int argc, __attribute__((unused)) char *argv[])
{
  zpal_clear_restarts();
  tr_cli_common_print("Ok\n");
  return 0;
}
