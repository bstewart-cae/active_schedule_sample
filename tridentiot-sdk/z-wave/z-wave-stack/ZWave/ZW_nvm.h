// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
* @file ZW_nvm.h
* @brief Z-Wave protocol NVM top module.
* @details Owns the Z-Wave File System and keeps track of user modules
* through registration.
*
* The NVM module mounts the file system, and performs format + mount if
* mouting the FS fails.
* Mount procedure is performed on NVM init, or when first registration is received.
*
* A module can only receive the File system object pointer by registrering.
* When a format of the FS is performed, a callback is performed to all registered
* modules, allowing them to re-write default data etc.
* When registering, the registree is informed of if the file system has been formatted
* since boot, again allowing the registrees fill in any default data.
*
* @note A module cannot assume that any default data is present just because
* it has not received format callback or format information when registering,
* as power could have been lost while writing the default files, meaning FS is OK
* mountable etc. but the default files are not present.
*
* @copyright 2022 Silicon Laboratories Inc.
*/

#ifndef _ZW_NVM_H_
#define _ZW_NVM_H_

#include <stdbool.h>
#include "SyncEvent.h"
#include <zpal_nvm.h>

/**
* Status returned on Nvm Filesystem init.
*/
typedef enum ENvmFsInitStatus
{
  ENVMFSINITSTATUS_SUCCESS = 0,   /**< FS mounted successfully */
  ENVMFSINITSTATUS_FORMATTED,     /**< FS initially failed to mount. FS Formatted and mounted. */
  ENVMFSINITSTATUS_FAILED         /**< Failed to format and mount FS */
} ENvmFsInitStatus;


/**
* Initializes and mounts Z-Wave file system.
*
* If File system fails to mount, it will be formatted and then,
* attempted mounted. If file system fails to format or mount after format,
* an error code is returned.
*
* @return       Returns ENvmFsInitStatus. Its either success or error code.
*/
ENvmFsInitStatus NvmFileSystemInit(void);


/**
* Formats, re-initializes and mounts file system.
*
* Callbacks to registred FS users are performed after
* format and mount. Callbacks are skipped if format or mount fails.
*
* @retval       true      FS successfully formatted and mounted.
* @retval       false     Failed to either format or mount FS.
*/
bool NvmFileSystemFormat(void);


/**
* Acquire Pointer to Z-Wave file system, and register as user
* of file system.
*
* Method is the only way for FS users to acquire a pointer to
* the FS object. This ensures users are registered, and can be
* informed if the FS is formatted.
*
* @param[in]    pFsResetCb  Pointer to SyncEvent, invoked on
*                           FS format and remount. May be NULL if
*                           FS user does not require CB when FS is 
*                           formatted.
*
* @return       Returns     Pointer to Z-Wave file system object.
*/
zpal_nvm_handle_t NvmFileSystemRegister(const SSyncEvent* pFsResetCb);


/**
* Invokes all stored Cb`s for registered users.
*/
void NVM3_InvokeCbs(void);

#endif	// _ZW_NVM_H_
