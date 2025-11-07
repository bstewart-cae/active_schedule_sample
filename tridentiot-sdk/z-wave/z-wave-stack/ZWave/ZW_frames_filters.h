// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_frames_filters.h
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef ZW_FRAMES_FILTER_H_
#define ZW_FRAMES_FILTER_H_

#include <ZW_typedefs.h>
#include <ZW_protocol.h>
#include "ZW_Frame.h"

/*
 */
uint8_t IsFrameIlLegal (ZW_HeaderFormatType_t curHeader, frame* pRxFrame, uint8_t* pCmdClass, uint8_t *pCmd);
#ifdef ZW_CONTROLLER
/*
 */
uint8_t  DropPingTest (uint8_t bdestNode);

/*
 */
uint8_t IsFindNodeInRangeAllowed(uint8_t bSourceID);
#endif

#endif /* ZW_FRAMES_FILTER_H_ */
