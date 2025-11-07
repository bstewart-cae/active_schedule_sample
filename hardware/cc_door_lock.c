// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
// SPDX-License-Identifier: LicenseRef-TridentMSLA
/*
 * This file fakes the behavior of the hardware in a lock.
 */
#include <stdint.h>
#include "CC_DoorLock.h"
#include "CC_UserCredential.h"
#include "zaf_event_distributor_soc.h"

static door_lock_hw_data_t door_lock_hw = {0};

void cc_door_lock_bolt_set(bool locked)
{
  door_lock_hw.bolt_unlocked = !locked;
}

void cc_door_lock_latch_set(bool opened)
{
  door_lock_hw.latch_closed = !opened;
}

void cc_door_lock_handle_set(bool pressed)
{
  door_lock_hw.handle_pressed = pressed;
}

bool door_lock_hw_bolt_is_unlocked(void)
{
  return door_lock_hw.bolt_unlocked;
}

bool door_lock_hw_latch_is_closed(void)
{
  return door_lock_hw.latch_closed;
}

bool door_lock_hw_handle_is_pressed(void)
{
  return door_lock_hw.handle_pressed;
}

uint8_t cc_door_lock_mode_hw_change(door_lock_mode_t mode)
{
  cc_door_lock_bolt_set((mode == DOOR_MODE_SECURED));
  zaf_event_distributor_enqueue_cc_event(
      COMMAND_CLASS_DOOR_LOCK,
      CC_DOOR_LOCK_EVENT_HW_OPERATION_DONE,
      NULL);
  return 0;
}
