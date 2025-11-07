// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
* @file
* @copyright Copyright 2020 Silicon Laboratories Inc. www.silabs.com, All Rights Reserved
*/

#ifndef ZW_FORCED_SHUTDOWN_H_
#define ZW_FORCED_SHUTDOWN_H_

/**
   Initiate graceful shutdown
   The API will stops the radio and cancel all timers and power locks.
   The protocol will notfiy the applciation when ready to go into deepsleep by the provided callback
* @param[in]     callback  Pointer to callback used to notify the application before ging into  deepsleep
*/
void
ZW_initiate_shutdown(void (*callback)(void));

#endif /* ZW_FORCED_SHUTDOWN_H_*/
