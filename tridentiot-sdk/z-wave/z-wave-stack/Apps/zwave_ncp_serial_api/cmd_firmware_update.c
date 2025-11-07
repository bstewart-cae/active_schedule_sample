// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
// SPDX-License-Identifier: BSD-3-Clause
#include <stdint.h>
#include "cmd_handlers.h"
#include "zpal_bootloader.h"
#include "ZW_SerialAPI.h"
#include "app.h"

#define FIRMWARE_UPDATE_SUB_COMMAND_PREPARE       0x00
#define FIRMWARE_UPDATE_SUB_COMMAND_WRITE         0x01
#define FIRMWARE_UPDATE_SUB_COMMAND_UPDATE_TARGET 0x02

#define FIRMWARE_UPDATE_TARGET_FIRMWARE   0x01
#define FIRMWARE_UPDATE_TARGET_BOOTLOADER 0x02

const uint8_t FIRMWARE_UPDATE_RESPONSE_SUCCESS                          = 0x00;
const uint8_t FIRMWARE_UPDATE_RESPONSE_TOO_BIG                          = 0x01;
const uint8_t FIRMWARE_UPDATE_RESPONSE_FIRMWARE_UPDATE_NOT_SUPPORTED    = 0x02;
const uint8_t FIRMWARE_UPDATE_RESPONSE_BOOTLOADER_UPDATE_NOT_SUPPORTED  = 0x03;
const uint8_t FIRMWARE_UPDATE_RESPONSE_WRONG_CHECKSUM                   = 0x04;
const uint8_t FIRMWARE_UPDATE_RESPONSE_INVALID_FILE_HEADER              = 0x05;
const uint8_t FIRMWARE_UPDATE_RESPONSE_INVALID_SIGNATURE                = 0x06;
const uint8_t FIRMWARE_UPDATE_RESPONSE_FIRMWARE_DOES_NOT_MATCH          = 0x07;
const uint8_t FIRMWARE_UPDATE_RESPONSE_HARDWARE_VERSION_NOT_SUPPORTED   = 0x08;
const uint8_t FIRMWARE_UPDATE_RESPONSE_DOWNGRADE_NOT_SUPPORTED          = 0x09;
const uint8_t FIRMWARE_UPDATE_RESPONSE_TARGET_NOT_SUPPORTED             = 0x0A;
const uint8_t FIRMWARE_UPDATE_RESPONSE_PREPARE_FAILED                   = 0x0B;
const uint8_t FIRMWARE_UPDATE_RESPONSE_SUB_COMMAND_NOT_SUPPORTED        = 0xFF;

static void firmware_update_install_and_reboot(void)
{
  zpal_bootloader_reboot_and_install();
}

static void respond(const uint8_t sub_command_id, const uint8_t response)
{
  uint8_t i = 0;
  compl_workbuf[i++] = sub_command_id;
  compl_workbuf[i++] = response;
  DoRespond_workbuf(i);
}

ZW_ADD_CMD(FUNC_ID_FIRMWARE_UPDATE)
{
  uint8_t sub_command_id = frame->payload[0];

  switch (sub_command_id) {
    case FIRMWARE_UPDATE_SUB_COMMAND_PREPARE:
    {
      // Check for bootloader support.
      zpal_bootloader_info_t info;
      zpal_bootloader_get_info(&info);
      if (ZPAL_BOOTLOADER_NOT_PRESENT == info.type) {
        respond(sub_command_id, FIRMWARE_UPDATE_RESPONSE_SUB_COMMAND_NOT_SUPPORTED);
        return;
      }

      zpal_status_t status = zpal_bootloader_init();
      uint8_t result = FIRMWARE_UPDATE_RESPONSE_SUCCESS;
      if (ZPAL_STATUS_OK != status) {
        result = FIRMWARE_UPDATE_RESPONSE_PREPARE_FAILED;
      }

      respond(sub_command_id, result);
    }
    break;
    case FIRMWARE_UPDATE_SUB_COMMAND_WRITE:
    {
      uint32_t offset = 0;
      offset |= frame->payload[1]; // MSB
      offset <<= 8;
      offset |= frame->payload[2];
      offset <<= 8;
      offset |= frame->payload[3];
      offset <<= 8;
      offset |= frame->payload[4]; // LSB

      uint16_t data_length = (frame->payload[5] << 8) | frame->payload[6];

      zpal_status_t status = zpal_bootloader_write_data(offset, &(frame->payload[7]), data_length);

      uint8_t result = 0;
      if (ZPAL_STATUS_OK != status) {
        result = FIRMWARE_UPDATE_RESPONSE_TOO_BIG;
      }

      respond(sub_command_id, result);
    }
    break;

    case FIRMWARE_UPDATE_SUB_COMMAND_UPDATE_TARGET:
    {
      uint8_t target = frame->payload[1];

      switch (target) {
        case FIRMWARE_UPDATE_TARGET_FIRMWARE:
          // Do nothing in this case and continue past the switch-case.
          break;
        case FIRMWARE_UPDATE_TARGET_BOOTLOADER:
          respond(sub_command_id, FIRMWARE_UPDATE_RESPONSE_BOOTLOADER_UPDATE_NOT_SUPPORTED);
          return;
        default:
          respond(sub_command_id, FIRMWARE_UPDATE_RESPONSE_SUB_COMMAND_NOT_SUPPORTED);
          return;
      }

      zpal_status_t verify_status = zpal_bootloader_verify_image();

      switch (verify_status) {
        case ZPAL_STATUS_OK:
        {
          uint8_t i = 0;
          compl_workbuf[i++] = sub_command_id;
          compl_workbuf[i++] = FIRMWARE_UPDATE_RESPONSE_SUCCESS;
          DoRespond_workbuf_with_ack_cb(i, firmware_update_install_and_reboot);
          break;
        }
        case ZPAL_STATUS_BOOTLOADER_DOWNGRADE_NOT_SUPPORTED:
          respond(sub_command_id, FIRMWARE_UPDATE_RESPONSE_DOWNGRADE_NOT_SUPPORTED);
          break;
        case ZPAL_STATUS_BOOTLOADER_INVALID_CHECKSUM:
          respond(sub_command_id, FIRMWARE_UPDATE_RESPONSE_WRONG_CHECKSUM);
          break;
        default:
          respond(sub_command_id, FIRMWARE_UPDATE_RESPONSE_FIRMWARE_DOES_NOT_MATCH);
          break;
      }
    }
    break;

    default:
      respond(sub_command_id, FIRMWARE_UPDATE_RESPONSE_SUB_COMMAND_NOT_SUPPORTED);
      break;
  }
}
