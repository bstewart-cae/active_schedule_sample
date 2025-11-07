/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * @file events.h
 *
 * Definitions of events for Window/Door Sensor demo app.
 *
 */
#ifndef APPS_WINDOW_SENSOR_EVENTS_H_
#define APPS_WINDOW_SENSOR_EVENTS_H_

#include <ev_man.h>

/**
 * Defines events for the application.
 *
 * These events are not referred to anywhere else than in the application. Hence, they can be
 * altered to suit the application flow.
 *
 * The events are located in a separate file to make it possible to include them in other
 * application files. An example could be a peripheral driver that enqueues an event when something
 * specific happens, e.g. on motion detection.
 */
typedef enum EVENT_APP_SENSOR_PIR
{
  EVENT_EMPTY = DEFINE_EVENT_APP_NBR, ///< Empty event
  EVENT_APP_BATTERY_REPORT,           ///< Battery report event
  EVENT_APP_WINDOW_OPEN,              ///< Window open event
  EVENT_APP_WINDOW_CLOSE,             ///< Window close event
}
EVENT_APP;

#endif /* APPS_WINDOW_SENSOR_EVENTS_H_ */
