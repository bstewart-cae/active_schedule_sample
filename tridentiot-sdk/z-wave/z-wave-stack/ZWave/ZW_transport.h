// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_transport.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Z-Wave Transport Layer interface.
 */
#ifndef _ZW_TRANSPORT_H_
#define _ZW_TRANSPORT_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_lib_defines.h>
#include <ZW_protocol.h>
#include <ZW_protocol_commands.h>
#include <ZW_transport_commandclass.h>
#include <ZW_basis_api.h>
#include <ZW_transport_api.h>
#include <ZW_transport_transmit_cb.h>
#include "ZW_application_transport_interface.h"
#include <ZW_tx_queue.h>
#include <ZW_MAC.h>
#include <ZW_explore.h>
#include <zpal_radio.h>

/* Receive buffers */
#define RX_BUFFERS  2     /* number of receiver buffers */

typedef struct _return_route_
{
  /* same layout as ASSIGN_RETURN_ROUTE_FRAME excl. class and cmd */
  uint8_t  nodeID;           /* route destination node ID */
  uint8_t  routeNoNumHops;   /* Route number, number of hops*/
  uint8_t  repeaterList[MAX_REPEATERS]; /* List of repeaters */
  uint8_t  bReturnSpeed;
} RETURN_ROUTE;

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
#define FRAME_TYPE_SINGLE        HDRTYP_SINGLECAST   /* single cast */
#define FRAME_TYPE_MULTI         HDRTYP_MULTICAST    /* multicast   */
#define FRAME_TYPE_ACK           HDRTYP_TRANSFERACK  /* frame acknowledge */
#define FRAME_TYPE_ROUTED        HDRTYP_ROUTED       /* single cast routed */
#define FRAME_TYPE_FLOODED       HDRTYP_FLOODED      /* flooded sensor frame */
#define FRAME_TYPE_EXPLORE       HDRTYP_EXPLORE      /* explore frame */
#define FRAME_TYPE_INTERNAL_REPEATED  0x10           /* Internal frame type used for repeated frames */

/* Result of parsing received frame in ReceiveHandler() is indicated with these
 * status values */
#define STATUS_FRAME_IS_MINE    0x01  /* Is frame for me */
#define STATUS_TRANSFERACK      0x02  /* This is an ACK */
#define STATUS_DO_ACK           0x04  /* Must do an ACK in response to this frame */
#define STATUS_DO_ROUTING_ACK   0x08  /* Must do a Routed ACK in response to this frame */
#define STATUS_ROUTE_FRAME      0x10  /* This is a routed frame */
#define STATUS_ROUTE_ACK        0x20  /* This is a routed ACK*/
#define STATUS_ROUTE_ERR        0x40  /* This is a routed error */
#define STATUS_NOT_MY_HOMEID    0x80  /* This frame is from a different homeid */

/* Transmit options - following are currently defined and used in devkit libraries: */
/*
Application usable:
TRANSMIT_OPTION_NONE                                      0x00
TRANSMIT_OPTION_ACK                                       0x01
TRANSMIT_OPTION_LOW_POWER                                 0x02
TRANSMIT_OPTION_AUTO_ROUTE	TRANSMIT_OPTION_RETURN_ROUTE  0x04
TRANSMIT_OPTION_NO_ROUTE                                  0x10
TRANSMIT_OPTION_EXPLORE                                   0x20

Library used:
TRANSMIT_OPTION_APPLICATION                               0x20
TRANSMIT_OPTION_ROUTED                                    0x80
TRANSMIT_OPTION_EXPLORE_REPEAT                            0x40

UNUSED:
TRANSMIT_OPTION_FORCE_ROUTE                               0x08
*/

/* Allow Transport Service segmentation of long messages */
#define TRANSMIT_OPTION_2_TRANSPORT_SERVICE     0x01
#define TRANSMIT_OPTION_2_FOLLOWUP              0x08

/*
 * Conversions from multiple 8 bit TX option bit masks to one 32 bit TX option mask.
 *
 * The original TX options are located in the least significant byte in the 32 bit TX options.
 * TX options 2 are located as the next byte.
 */
#define TRANSMIT_OPTION_FOLLOWUP            (TRANSMIT_OPTION_2_FOLLOWUP << 8)
#define TRANSMIT_OPTION_LR_FORCE            0x00001000
#define TRANSMIT_OPTION_LR_SECONDARY_CH     0x00002000

#define TRANSMIT_OPTION_DELAYED_TX          0x00004000  //< Used to do delayed transmission

#define TRANSMIT_OPTION_NONE                0x00  /* No Transmit Options needed */
#define TRANSMIT_OPTION_APPLICATION         0x20  /* The transmission can be aborted using
                                                     ZW_SendDataAbort() */
#define TRANSMIT_OPTION_EXPLORE_REPEAT      0x40  /* Used when an explore frame is to be repeated */

#define TRANSMIT_OPTION_ROUTED              0x80  /* Routed frame */

/* Transmit frame option flag mask used for masking valid application option flags when transmitting explore frames */
#define TRANSMIT_EXPLORE_OPTION_MASK        (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_LOW_POWER)

#define TRANSMIT_APPLICATION_OPTION_MASK    (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_LOW_POWER | TRANSMIT_OPTION_NO_ROUTE | TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_EXPLORE)

/* bFrameOptions1 defines */
#define TRANSMIT_FRAME_OPTION_EXPLORE           0x01
#define TRANSMIT_FRAME_OPTION_SKIP_BEAM         0x02
#define TRANSMIT_FRAME_OPTION_SPEED_MODIFIED    0x04
#define TRANSMIT_FRAME_OPTION_USR_PTR_CB        0x08
#define TRANSMIT_FRAME_OPTION_NOBEAM            0x10
#define TRANSMIT_FRAME_OPTION_NO_BUILD_TX_HDR   0x20
#define TRANSMIT_FRAME_OPTION_TRANSPORT_SERVICE 0x40
#define TRANSMIT_FRAME_OPTION_FOLLOWUP          0x80

/* Received frame status flags */
#define RECEIVE_STATUS_FOREIGN_HOMEID   0x80   /* Not my HomeID */
#define RECEIVE_STATUS_SMART_NODE       0x20   /* Received frame may be a smart start node */


/* Dynamic route cache response/last working route definitions */
#define CACHED_ROUTE_LINE_DIRECT  0xFE

/* ZW_REDISCOVERY_NEEDED callback values. */
/* Note that they are different from ZW_REQUEST_NETWORK_UPDATE callbacks */
#define ZW_ROUTE_LOST_FAILED      0x04  /*Node Asked wont help us*/
#define ZW_ROUTE_LOST_ACCEPT      0x05  /*Accepted to help*/

/* TX_STATUS_TYPE Last Used Route array index definitions */
/* Unused
#define LAST_USED_ROUTE_REPEATER_0_INDEX        0
#define LAST_USED_ROUTE_REPEATER_1_INDEX        1
#define LAST_USED_ROUTE_REPEATER_2_INDEX        2
#define LAST_USED_ROUTE_REPEATER_3_INDEX        3
*/
#define LAST_USED_ROUTE_CONF_INDEX              4


/* predefined node ID's moved to ZW_protocol_def.h */

// Moved protocol commands to ZW_protocol_commands.h

/* Z-Wave protocol status */
#define ZWAVE_TRANSFER_FAIL       0x00
#define ZWAVE_TRANSFER_OK         0x01

/* These values are calculated from values in ZW_controller_api.h
 * and should not be changed without checking effects*/
#define ZWAVE_APPLICATION_VAL_OFFSET    2
#define ZWAVE_TRANSFER_UPDATE_DONE      (ZWAVE_APPLICATION_VAL_OFFSET + ZW_SUC_UPDATE_DONE)
#define ZWAVE_TRANSFER_UPDATE_ABORT     (ZWAVE_APPLICATION_VAL_OFFSET + ZW_SUC_UPDATE_ABORT)
#define ZWAVE_TRANSFER_UPDATE_WAIT      (ZWAVE_APPLICATION_VAL_OFFSET + ZW_SUC_UPDATE_WAIT)
#define ZWAVE_TRANSFER_UPDATE_DISABLED  (ZWAVE_APPLICATION_VAL_OFFSET + ZW_SUC_UPDATE_DISABLED)
#define ZWAVE_TRANSFER_UPDATE_OVERFLOW  (ZWAVE_APPLICATION_VAL_OFFSET + ZW_SUC_UPDATE_OVERFLOW)

// Moved Transport Command Class definitions to ZW_transport_commandclass.h

#define IS_PROTOCOL_CLASS(cmd)  (cmd == ZWAVE_CMD_CLASS_PROTOCOL)
#define IS_SENSOR_CLASS(cmd)    (cmd == ZWAVE_CMD_CLASS_SENSOR)
#define IS_EXPLORE_CLASS(cmd)    (cmd == ZWAVE_CMD_CLASS_EXPLORE)
#define IS_PROTOCOL_LR_CLASS(cmd) (cmd == ZWAVE_CMD_CLASS_PROTOCOL_LR)
#define IS_APPL_CLASS(cmd)    ((cmd >= ZWAVE_CMD_CLASS_APPL_MIN) && (cmd <= ZWAVE_CMD_CLASS_APPL_MAX))

/* Transmission states */
#define TX_STATE_IDLE             0x00 /* no transmissions in progress (RF in receive mode) */
#define TX_STATE_DELAY            0x01 /* delaying the RF tx mode (RF in receive mode) */
#define TX_STATE_TX_ACK           0x02 /* transmission of frame acknowledge in progress */
#define TX_STATE_TX_FRAME         0x03 /* transmission of a frame in progress */
#define TX_STATE_WAIT_ACK         0x04 /* waiting for frame acknowledge (RF in receive mode) */
#define TX_STATE_WAIT_ROUTE_ACK   0x05 /* waiting for routed frame acknowledge (RF in receive mode) */

/* Constants for RSSI feedback */
#define RSSI_MINIMUM_VALUE 40     /*  RSSI values below this level are reported as RSSI_BELOW_SENSITIVITY*/
                                  /* Values between and including RSSI_MINIMUM_VALUE
                                     and RSSI_MAXIMUM_VALUE increase linearly with
                                     the logarithm of the power received. */
#define RSSI_MAXIMUM_VALUE 80     /* Receiver saturated above this level. Higher values are reported to
                                     application as RSSI_MAX_POWER_SATURATED. */
/* Received frame */
typedef struct _RX_FRAME_
{
  ZW_ReceiveFrame_t *pReceiveFrame;
  frame *pFrame;        // Old raw frame, data is now accessible directly in pReceiveFrame.
  uint8_t  *pPayload;      // Old raw frame, data is now accessible directly in pReceiveFrame.
  uint8_t   payloadLength; // Old raw frame, data is now accessible directly in pReceiveFrame.
  uint8_t   status;
  node_id_t  ackNodeID;
  uint8_t   bChannelAndSpeed; //< Obsolete, now stored in mTransportRxCurrentSpeed and mTransportRxCurrentCh, see GH-643 for details
  uint8_t bTotalLength;
} RX_FRAME;

/************************************************************/
/* NOP frame*/
/************************************************************/
typedef struct _NOP_FRAME_
{
   uint8_t cmd;             /* The command is CMD_NOP, actually */
} NOP_FRAME;            /* it is Z-Wave protocol commmand class with no cmd */

/************************************************************/
/* NOP frame for Long Range */
/************************************************************/
typedef struct _NOP_FRAME_LR_
{
  uint8_t cmdClass;       /* The command class is Z-Wave LR protocol */
  uint8_t cmd;            /* The command is ZWAVE_LR_CMD_NOP */
} NOP_FRAME_LR;

/************************************************************/
/* Non-secure inclusion complete frame */
/************************************************************/
typedef struct _NON_SECURE_INCLUSION_COMPLETE_FRAME_LR_
{
  uint8_t cmdClass;       /* The command class is Z-Wave LR protocol */
  uint8_t cmd;            /* The command is ZWAVE_LR_CMD_NON_SECURE_INCLUSION_COMPLETE */
} NON_SECURE_INCLUSION_COMPLETE_FRAME_LR;

#ifdef ZW_SELF_HEAL
/************************************************************/
/* LOST frame*/
/************************************************************/
typedef struct _LOST_FRAME_
{
  uint8_t cmdClass;       /* The command class is Z-Wave protocol */
  uint8_t cmd;             /* The command is CMD_LOST, actually */
  uint8_t nodeID;          /*Node ID of the node that is lost*/
} LOST_FRAME;            /* it is Z-Wave protocol commmand class with no cmd */


/************************************************************/
/* ACCEPT_LOST frame*/
/************************************************************/
typedef struct _ACCEPT_LOST_FRAME_
{
  uint8_t cmdClass;       /* The command class is Z-Wave protocol */
  uint8_t cmd;             /* The command is CMD_ACCEPT_LOST, actually */
  uint8_t accepted;          /*true if node accepted to help a lost node*/
} ACCEPT_LOST_FRAME;    /* it is Z-Wave protocol commmand class with no cmd */
#endif /*ZW_SELF_HEAL*/


/************************************************************/
/* NOP_TEST frame*/
/************************************************************/
typedef struct _NOP_POWER_FRAME_
{
  uint8_t cmdClass;       /* The command class is Z-Wave protocol */
  uint8_t cmd;             /* the command in CMD_NOP_POWER */
  uint8_t parm;            /* the powerlevel frame should be acked with */
  uint8_t parm2;            /* the powerlevel index frame should be acked with */
} NOP_POWER_FRAME;


typedef struct _NODEINFO_FRAME_
{
  uint8_t      cmdClass;               /* The command class is Z-Wave protocol */
  uint8_t      cmd;                    /* The command is CMD_NODE_INFO */
  uint8_t      capability;             /* Network capabilities */
  uint8_t      security;               /* Network security */
  uint8_t      reserved;
  NODE_TYPE nodeType;               /* Basic, Generic and Specific Device Type */
  uint8_t      nodeInfo[NODEPARM_MAX]; /* Device status */
} NODEINFO_FRAME;

typedef struct
{
  uint8_t      cmdClass;               /* The command class is Z-Wave protocol */
  uint8_t      cmd;                    /* The command is CMD_NODE_INFO */
  uint8_t      capability;             /* Network capabilities */
  uint8_t      security;               /* Network security */
  uint8_t      reserved;
  NODE_TYPE    nodeType;               /* Basic, Generic and Specific Device Type */
  uint8_t      ccListLength;
  uint8_t      nodeInfo[NODEPARM_MAX]; /* Device status */
} NODEINFO_LR_FRAME;


typedef struct _NODEINFO_OLD_FRAME_
{
  uint8_t      cmdClass;               /* The command class is Z-Wave protocol */
  uint8_t      cmd;                    /* The command is CMD_NODE_INFO */
  uint8_t      capability;             /* Network capabilities */
  uint8_t      security;               /* Network security */
  uint8_t      reserved;
  uint8_t      nodeType;               /* Device Type */
  uint8_t      nodeInfo[NODEPARM_MAX]; /* Device status */
} NODEINFO_OLD_FRAME;


typedef struct _NODEINFO_BLOCK_
{
  uint8_t      capability;             /* Network capabilities */
  uint8_t      security;               /* Network security */
  uint8_t      reserved;
#ifdef ZW_CONTROLLER
  NODE_TYPE nodeType;               /* Basic, Generic and Specific Device Type */
#else
  /* Slaves do not transmit Basic Device Type in the nodeinfo frame only Generic and Specific */
  APPL_NODE_TYPE nodeType;          /* Generic and Specific Device Type */
#endif
  uint8_t      nodeInfo[NODEPARM_MAX]; /* Device status */
} NODEINFO_BLOCK;


typedef struct _NODEINFO_SLAVE_FRAME_
{
  uint8_t      cmdClass;               /* The command class is Z-Wave protocol */
  uint8_t      cmd;                    /* The command is CMD_NODE_INFO */
  uint8_t      capability;             /* Network capabilities */
  uint8_t      security;               /* Network security */
  uint8_t      reserved;
  /* Slaves do not transmit Basic Device Type in the nodeinfo frame only Generic and Specific */
  APPL_NODE_TYPE nodeType;          /* Generic and Specific Device Type */
  uint8_t      nodeInfo[NODEPARM_MAX]; /* Device status */
} NODEINFO_SLAVE_FRAME;

typedef struct
{
  uint8_t      cmdClass;               /* The command class is Z-Wave protocol */
  uint8_t      cmd;                    /* The command is CMD_NODE_INFO */
  uint8_t      capability;             /* Network capabilities */
  uint8_t      security;               /* Network security */
  uint8_t      reserved;
  /* Slaves do not transmit Basic Device Type in the nodeinfo frame only Generic and Specific */
  APPL_NODE_TYPE nodeType;          /* Generic and Specific Device Type */
  uint8_t      cc_list_length;
  uint8_t      nodeInfo[NODEPARM_MAX]; /* Device status */
} NODEINFO_LR_SLAVE_FRAME;

typedef struct _EXCLUDE_REQUEST_LR_FRAME_
{
  uint8_t      cmdClass;               /* The command class is Z-Wave LR protocol */
  uint8_t      cmd;                    /* The command is ZWAVE_LR_CMD_EXCLUDE_REQUEST */
} EXCLUDE_REQUEST_LR_FRAME;

/* Protocol version in nodeinfo capability */
#define ZWAVE_NODEINFO_PROTOCOL_VERSION_MASK    0x07
#define ZWAVE_NODEINFO_VERSION_2                0x01
#define ZWAVE_NODEINFO_VERSION_3                0x02
#define ZWAVE_NODEINFO_VERSION_4                0x03

/* Baud rate in nodeinfo capability */
#define ZWAVE_NODEINFO_BAUD_RATE_MASK           0x38
#define ZWAVE_NODEINFO_BAUD_9600                0x08
#define ZWAVE_NODEINFO_BAUD_40000               0x10

/* Routing support in nodeinfo capabilitity */
#define ZWAVE_NODEINFO_ROUTING_SUPPORTED_MASK   0x40
#define ZWAVE_NODEINFO_ROUTING_SUPPORT          0x40

/* Listening in nodeinfo capabilitity */
#define ZWAVE_NODEINFO_LISTENING_MASK            0x80
#define ZWAVE_NODEINFO_LISTENING_SUPPORT         0x80

/* Security bit in nodeinfo security field */
#define ZWAVE_NODEINFO_SECURITY_MASK             0x01
#define ZWAVE_NODEINFO_SECURITY_SUPPORT          0x01

/* Controller bit in nodeinfo security field */
#define ZWAVE_NODEINFO_CONTROLLER_MASK           0x02
#define ZWAVE_NODEINFO_CONTROLLER_NODE           0x02

/* Specific device type bit in nodeinfo security field */
#define ZWAVE_NODEINFO_SPECIFIC_DEVICE_MASK      0x04
#define ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE      0x04

/* Slave_routing bit in nodeinfo security field */
#define ZWAVE_NODEINFO_SLAVE_ROUTING_MASK        0x08
#define ZWAVE_NODEINFO_SLAVE_ROUTING             0x08

/* Beam capability bit in nodeinfo security field */
#define ZWAVE_NODEINFO_BEAM_CAPABILITY_MASK      0x10
#define ZWAVE_NODEINFO_BEAM_CAPABILITY           0x10

/* Sensor mode bits in nodeinfo security field */
#define ZWAVE_NODEINFO_SENSOR_MODE_MASK          0x60
#define ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_1000   0x40
#define ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_250    0x20
#define ZWAVE_NODEINFO_SENSOR_MODE_RESERVED      0x60

/* Optional Functionality bit in nodeinfo security field */
#define ZWAVE_NODEINFO_OPTIONAL_FUNC_MASK        0x80
#define ZWAVE_NODEINFO_OPTIONAL_FUNC             0x80

/* Speed extension in nodeinfo reserved field */
#define ZWAVE_NODEINFO_BAUD_100K                 0x01
#define ZWAVE_NODEINFO_BAUD_100KLR               0x02

/* Defines for Zensor wakeup time - how long must the beam be, so that the zensor */
/* is capable to hear it. */
/* Not a zensor findneighbor operation, but a plain standard findneighbor operation */
#define ZWAVE_SENSOR_WAKEUPTIME_NONE    0x00
/* The wakeup time for the zensor node in nodemask is 1000ms */
#define ZWAVE_SENSOR_WAKEUPTIME_1000MS  0x01
/* The wakeup time for the zensor node in nodemask is 250ms */
#define ZWAVE_SENSOR_WAKEUPTIME_250MS   0x02

/* Number of nodes we store for each response speed.
 * We store node IDs of the last MAX_RESPONSE_SPEED_NODES nodes
 * we received for each speed. */
#define MAX_RESPONSE_SPEED_NODES 2

/************************************************************/
/* Request node Information frame*/
/************************************************************/
typedef struct _REQ_NODE_INFO_FRAME_
{
  uint8_t  cmdClass;        /* The command class is Z-Wave protocol */
  uint8_t  cmd;             /* The command is CMD_REQUEST_NODE_INFO */
} REQ_NODE_INFO_FRAME;


/************************************************************/
/* Assign ID's command */
/************************************************************/
typedef struct _ASSIGN_IDS_FRAME_
{
  uint8_t  cmdClass;        /* The command class is Z-Wave protocol */
  uint8_t  cmd;             /* The command is CMD_ASSIGN_IDS */
  uint8_t  newNodeID;
  uint8_t  newHomeID[HOMEID_LENGTH];
} ASSIGN_IDS_FRAME;

/************************************************************/
/* Assign ID's command for Long Range*/
/************************************************************/
typedef struct _ASSIGN_IDS_FRAME_LR_
{
  uint8_t  cmdClass;        /* The command class is Z-Wave LR protocol */
  uint8_t  cmd;             /* The command is CMD_LR_ASSIGN_IDS */
  uint8_t  newNodeID[2];    /* LR node ID: [0]=MSB, [1]=LSB */
  uint8_t  newHomeID[HOMEID_LENGTH];
} ASSIGN_IDS_FRAME_LR;

/************************************************************/
/* Find nodes within range command*/
/************************************************************/
typedef struct _FIND_NODES_FRAME_
{
  uint8_t  cmdClass;                /* The command class is Z-Wave protocol */
  uint8_t  cmd;                     /* The command is CMD_FIND_NODES_IN_RANGE */
  uint8_t  numMaskBytes;            /* The number of mask bytes in the frame */
  uint8_t  maskBytes[MAX_NODEMASK_LENGTH];  /* The first mask byte in the frame */
  uint8_t  zensorWakeupTime;       /* The wakeuptime to be used if nodes are zensor nodes */
                                /* If this byte is missing or is ZERO then the find nodes */
                                /* is for a standard findneighbors sequence - If zensor- */
                                /* WakupTime is ZWAVE_ZENSOR_WAKEUPTIME_1000MS then the */
                                /* zensors has a wakeuptime at 1000ms, if zensorWakeupTime */
                                /* is ZWAVE_ZENSOR_WAKEUPTIME_250MS then the wakeup time */
                                /* for the zensors is 250ms */
  uint8_t  powerAndSpeed;          /* Transmit power and baud rate of the NOP frame */
} FIND_NODES_FRAME;

/* Extended speed info present in maskBytes */
#define ZWAVE_FIND_NODES_EXTENSION      0x80

/* Speed bits in powerAndSpeed */
#define ZWAVE_FIND_NODES_SPEED_MASK     0x07
#define ZWAVE_FIND_NODES_SPEED_9600     0x01
#define ZWAVE_FIND_NODES_SPEED_40K      0x02
#define ZWAVE_FIND_NODES_SPEED_100K     0x03
#define ZWAVE_FIND_NODES_SPEED_200K     0x04

/* Power bits in powerAndSpeed */
#define ZWAVE_FIND_NODES_POWER_MASK     0x38

#define ZWAVE_FIND_NODES_NUMMASKBYTES_MASK  0x1F


/************************************************************/
/* Get nodes within range frame*/
/************************************************************/
typedef struct _GET_NODE_RANGE_FRAME_
{
  uint8_t  cmdClass;        /* The command class is Z-Wave protocol */
  uint8_t  cmd;             /* The command is CMD_GET_NODES_IN_RANGE */
  uint8_t  zensorWakeupTime;       /* The wakeuptime to be used if nodes are zensor nodes */
                                /* If this byte is missing or is ZERO then the rangeinfo */
                                /* is for a standard findneighbors sequence - If zensor- */
                                /* WakupTime is ZWAVE_ZENSOR_WAKEUPTIME_1000MS then the */
                                /* zensors has a wakeuptime at 1000ms, if zensorWakeupTime */
                                /* is ZWAVE_ZENSOR_WAKEUPTIME_250MS then the wakeup time */
                                /* for the zensors is 250ms */
} GET_NODE_RANGE_FRAME;


/************************************************************/
/* Range Info frame*/
/************************************************************/
typedef struct _RANGEINFO_FRAME_
{
  uint8_t  cmdClass;               /* The class is Z-Wave protocol */
  uint8_t  cmd;                    /* The command is CMD_RANGE_INFO_RANGE */
  uint8_t  numMaskBytes;           /* The number of mask bytes in the frame */
  uint8_t  maskBytes[MAX_NODEMASK_LENGTH]; /* The first mask byte in the frame */
  uint8_t  zensorWakeupTime;       /* The wakeuptime to be used if nodes are zensor nodes */
                                /* If this byte is missing or is ZERO then the rangeinfo */
                                /* is for a standard findneighbors sequence - If zensor- */
                                /* WakupTime is ZWAVE_ZENSOR_WAKEUPTIME_1000MS then the */
                                /* zensors has a wakeuptime at 1000ms, if zensorWakeupTime */
                                /* is ZWAVE_ZENSOR_WAKEUPTIME_250MS then the wakeup time */
                                /* for the zensors is 250ms */
} RANGEINFO_FRAME;

#define ZWAVE_RANGE_INFO_NUMMASKBYTES_MASK  0x1F


/************************************************************/
/* Nodes exist frame*/
/************************************************************/
typedef struct _NODES_EXIST_FRAME_
{
  uint8_t  cmdClass;               /* The class is Z-Wave protocol */
  uint8_t  cmd;                    /* The command is CMD_NODES_EXIST */
  uint8_t  nodeMaskType;           /* The type of nodes which are present in nodeMask */
                                /* Type = 0 - All nodes */
                                /* Type = 1 - All repeater nodes */
                                /* Type = 2 - All listening nodes */
                                /* Type = 3 - All 1000ms beamable nodes */
                                /* Type = 4 - All 250ms beamable nodes */
  uint8_t  numNodeMask;            /* The number of bytes in the nodeMask */
  uint8_t  nodeMask[ZW_MAX_NODES/8]; /* The first mask byte in the frame */
} NODES_EXIST_FRAME;

/* Nodes Exist/Nodes Exist Reply nodeMaskType definitions */
#define NODES_EXIST_TYPE_ALL            0
#define NODES_EXIST_TYPE_REPEATER       1
#define NODES_EXIST_TYPE_LISTENING      2
#define NODES_EXIST_TYPE_WAKEUP_1000MS  3
#define NODES_EXIST_TYPE_WAKEUP_250MS   4

/* Nodes Exist Reply status definitions */
#define NODES_EXIST_REPLY_UNKNOWN_TYPE  0
#define NODES_EXIST_REPLY_DONE          1


/*******************************************************************/
/* Nodes exist reply frame - received whenever a Nodes Exist frame */
/*******************************************************************/
typedef struct _NODES_EXIST_REPLY_FRAME_
{
  uint8_t  cmdClass;               /* The class is Z-Wave protocol */
  uint8_t  cmd;                    /* The command is CMD_NODES_EXIST_REPLY */
  uint8_t  nodeMaskType;           /* The type of nodes which are to be present */
                                /* in the returning Nodes exist frame */
                                /* Type = 0 - All nodes */
                                /* Type = 1 - All repeater nodes */
                                /* Type = 2 - All listening nodes */
                                /* Type = 3 - All 1000ms beamable nodes */
                                /* Type = 4 - All 250ms beamable nodes */
  uint8_t  status;                 /* The result of the Nodes Exist frame received */
} NODES_EXIST_REPLY_FRAME;


/************************************************************/
/* Get Nodes exist frame*/
/************************************************************/
typedef struct _GET_NODES_EXIST_FRAME_
{
  uint8_t  cmdClass;               /* The class is Z-Wave protocol */
  uint8_t  cmd;                    /* The command is CMD_GET_NODES_EXIST */
  uint8_t  nodeMaskType;           /* The type of nodes which are to be present */
                                /* in the returning Nodes exist frame */
                                /* Type = 0 - All nodes */
                                /* Type = 1 - All repeater nodes */
                                /* Type = 2 - All listening nodes */
                                /* Type = 3 - All 1000ms beamable nodes */
                                /* Type = 4 - All 250ms beamable nodes */
} GET_NODES_EXIST_FRAME;

/* NWI mode definitions */
#define NWI_IDLE            0
#define NWI_REPEAT          1

/* NWI mode timeout default */
#define NWI_TIMEOUT_DEFAULT 0


/************************************************************/
/* Set NWI Mode frame*/
/************************************************************/
typedef struct _SET_NWI_MODE_FRAME_
{
  uint8_t  cmdClass;               /* The class is Z-Wave protocol */
  uint8_t  cmd;                    /* The command is CMD_SET_NWI_MODE */
  uint8_t  mode;                   /* The NWI Mode to be set */
  uint8_t  timeout;                /* Timeout on how long node should be in NWI mode */
                                /* If mode is NWI_IDLE then timeout is not used */
                                /* If ZERO then timeout is default (5min) */
} SET_NWI_MODE_FRAME;


/************************************************************/
/* Command complete frame*/
/************************************************************/
typedef struct _COMMAND_COMPLETE_FRAME_
{
  uint8_t  cmdClass;       /* The command class is Z-Wave protocol */
  uint8_t  cmd;            /* The command is CMD_CMD_COMPLETE */
  uint8_t  seqNo;          /* Optional sequence number.*/
}
COMMAND_COMPLETE_FRAME;


/************************************************************/
/* Assign return route frame*/
/************************************************************/
typedef struct _ASSIGN_RETURN_ROUTE_FRAME_
{
  uint8_t  cmdClass;         /* The command class is Z-Wave protocol */
  uint8_t  cmd;              /* The command is CMD_ASSIGN_RETURN_ROUTE */
  uint8_t  nodeID;           /* Static controller node ID */
  uint8_t  routeNoNumHops;   /* Route number, number of hops */
  uint8_t  repeaterList[MAX_REPEATERS]; /* List of repeaters */
  uint8_t  routeSpeed;       /* The speed supported by the routes */
                          /* Wakeup beam type for src and dst is in the routeSpeed byte : */
                          /* dst wakeup type mask = 00000110 (bit2 and bit1) */
                          /* src wakeup type mask = 11000000 (bit7 and bit6) */
                          /* The Route speed mask = 00111000 (bit5, bit4 and bit3) */
} ASSIGN_RETURN_ROUTE_FRAME;

/* Defines for masking source and destination wakeup beam type bits out */
/* which resides in the routeSpeed byte in the ASSIGN_RETURN_ROUTE_FRAME */
#define ROUTE_SPEED_SRC_WAKEUP_BEAM_MASK  0xC0
#define ROUTE_SPEED_SRC_WAKEUP_BEAM_1000  0x80
#define ROUTE_SPEED_SRC_WAKEUP_BEAM_250   0x40
#define ROUTE_SPEED_DST_WAKEUP_BEAM_MASK  0x06
#define ROUTE_SPEED_DST_WAKEUP_BEAM_1000  0x04
#define ROUTE_SPEED_DST_WAKEUP_BEAM_250   0x02

/* Defines for masking the baud rate of the route */
/* which resides in the routeSpeed byte in the ASSIGN_RETURN_ROUTE_FRAME */
/* Note that this is an enumeration, not a bitmask. */
/* Warning: Don't change these without changing NVM_RETURN_ROUTE_SPEED_* also */
#define ROUTE_SPEED_BAUD_RATE_MASK       0x38
#define ROUTE_SPEED_BAUD_9600            0x08
#define ROUTE_SPEED_BAUD_40000           0x10
#define ROUTE_SPEED_BAUD_100000          0x20
#define ROUTE_SPEED_BAUD_200000          0x18

/* Defines for masking source and destination wakeup beam type bits out */
/* which resides in the routeSpeed byte (one routeSpeed byte pr destination */
/* in the EEPROM_RETURN_ROUTE_SPEED_START structure in EEPROM */
#define MASK_RETURN_ROUTE_SPEED_BEAM_SRC  0xC0
#define MASK_RETURN_ROUTE_SPEED_BEAM_DST  0x30
#define MASK_RETURN_ROUTE_SPEED_DEST      0x0F

/* Defines for masking source and destination wakeup beam type bits out */
/* which resides in the bReturnSpeed byte in the RETURN_ROUTE structure */
#define RETURN_SPEED_SRC_WAKEUP_BEAM_MASK 0xC0
#define RETURN_SPEED_SRC_WAKEUP_BEAM_1000 0x80
#define RETURN_SPEED_SRC_WAKEUP_BEAM_250  0x40
#define RETURN_SPEED_DST_WAKEUP_BEAM_MASK 0x06
#define RETURN_SPEED_DST_WAKEUP_BEAM_1000 0x04
#define RETURN_SPEED_DST_WAKEUP_BEAM_250  0x02

/* Defines for masking the baud rate of the route */
/* which resides in the bReturnSpeed byte in the RETURN_ROUTE structure */
/* Note that this is an enumeration, not a bit mask. */
/* TODO: Delete these defines and use ROUTE_SPEED_* everywhere instead */
#define RETURN_ROUTE_BAUD_RATE_MASK       0x38
#define RETURN_ROUTE_BAUD_9600            0x08
#define RETURN_ROUTE_BAUD_40000           0x10
#define RETURN_ROUTE_BAUD_100000          0x20
#define RETURN_ROUTE_BAUD_200000          0x18

/************************************************************/
/* Assign return route priority frame*/
/************************************************************/
typedef struct _ASSIGN_RETURN_ROUTE_PRIORITY_FRAME_
{
  uint8_t  cmdClass;         /* The command class is Z-Wave protocol */
  uint8_t  cmd;              /* The command is CMD_ASSIGN_RETURN_ROUTE */
  uint8_t  nodeID;           /* Destination node ID of the route */
  uint8_t  routeNumber;      /* The number of the route that should be first priority */
} ASSIGN_RETURN_ROUTE_PRIORITY_FRAME;

#if defined(ZW_CONTROLLER) || defined(ZW_SLAVE_ROUTING)
/***********************************************************/
/* start the automatic controller updating command         */
/**********************************************************/
typedef struct _AUTOMATIC_CONTROLLER_UPDATE_START_FRAME_
{
  uint8_t cmdClass;         /* The class is Z-Wave protocol */
  uint8_t cmd;              /* The command is CMD_AUTOMATIC_CONTROLLER_UPDATE_START */
} AUTOMATIC_CONTROLLER_UPDATE_START_FRAME;


/***********************************************************/
/* providing the node ID of the SUC                        */
/**********************************************************/
typedef struct _SUC_NODE_ID_FRAME_
{
  uint8_t cmdClass;               /* The class is Z-Wave protocol */
  uint8_t cmd;                    /* The command is CMD_SUC_NODE_ID */
  uint8_t nodeID;                 /* the node ID of the SUC*/
  uint8_t SUCcapabilities;       /* The capabilities of the SUC */
} SUC_NODE_ID_FRAME;


/************************************************************/
/* static route request frame*/
/************************************************************/
typedef struct _STATIC_ROUTE_REQUEST_FRAME_
{
  uint8_t  cmdClass;         /* The command class is Z-Wave protocol */
  uint8_t  cmd;              /* The command is CMD_STATIC_ROUTE_REQUEST */
  uint8_t  desNodeIDList[ZW_MAX_CACHED_RETURN_ROUTE_DESTINATIONS];
} STATIC_ROUTE_REQUEST_FRAME;


typedef struct _FORCE_STATIC_ROUTE_REQUEST_FRAME_
{
  STATIC_ROUTE_REQUEST_FRAME routeFrame;
  uint8_t force;
} FORCE_STATIC_ROUTE_REQUEST_FRAME;

#endif


/* Transfer Presentation option bitmask definitions */
#define TRANSFER_PRESENTATION_OPTION_UNIQ_HOMEID    0x01
#define TRANSFER_PRESENTATION_OPTION_NOT_INCLUSION  0x02
#define TRANSFER_PRESENTATION_OPTION_NOT_EXCLUSION  0x04

/************************************************************/
/* Transfer presentation frame*/
/************************************************************/
typedef struct _TRANSFER_PRESENTATION_FRAME_
{
  uint8_t  cmdClass;        /* The command class is Z-Wave protocol */
  uint8_t  cmd;             /* The command is CMD_TRANSFER_PRESENTATION */
  uint8_t  option;         /* Command option bitmask */
} TRANSFER_PRESENTATION_FRAME;


#ifdef ZW_CONTROLLER
/************************************************************/
/* Transfer Node informatiom frame*/
/************************************************************/
typedef struct _TRANSFER_NODE_INFO_FRAME_
{
  uint8_t        cmdClass;  /* The command class is Z-Wave protocol */
  uint8_t        cmd;       /* The command is CMD_TRANSFER_NODE_INFO */
  uint8_t        seqNo;     /* sequence number*/
  uint8_t        nodeID;    /* The ID of the node */
  NODEINFO    nodeInfo;  /* NodeInfo as it is saved in the protocol */
} TRANSFER_NODE_INFO_FRAME;


/* Controller nodes is transfered with Basic, Generic and Specific Device types */
/* Slave nodes is transfered with Generic and Specific Device types */
typedef struct _NODEINFO_SLAVE_
{
  uint8_t        capability;     /* Network capabilities */
  uint8_t        security;       /* Network security */
  uint8_t        reserved;
  APPL_NODE_TYPE nodeType;       /* Generic and Specific Device type */
} NODEINFO_SLAVE;


typedef struct _TRANSFER_NODE_INFO_SLAVE_FRAME_
{
  uint8_t        cmdClass;  /* The command class is Z-Wave protocol */
  uint8_t        cmd;       /* The command is CMD_TRANSFER_NODE_INFO */
  uint8_t        seqNo;     /* sequence number*/
  uint8_t        nodeID;    /* The ID of the node */
  NODEINFO_SLAVE nodeInfo;  /* NodeInfo as it is saved in the protocol */
} TRANSFER_NODE_INFO_SLAVE_FRAME;


/* For old nodes only the Generic Device Type is transfered */
typedef struct _NODEINFO_OLD_
{
  uint8_t        capability;     /* Network capabilities */
  uint8_t        security;       /* Network security */
  uint8_t        reserved;
  uint8_t        nodeType;       /* Generic device type */
} NODEINFO_OLD;


typedef struct _TRANSFER_NODE_INFO_OLD_FRAME_
{
  uint8_t        cmdClass;  /* The command class is Z-Wave protocol */
  uint8_t        cmd;       /* The command is CMD_TRANSFER_NODE_INFO */
  uint8_t        seqNo;     /* sequence number*/
  uint8_t        nodeID;    /* The ID of the node */
  NODEINFO_OLD nodeInfo;  /* NodeInfo as it is saved in the protocol */
} TRANSFER_NODE_INFO_OLD_FRAME;


/************************************************************/
/* Transfer Range info frame*/
/************************************************************/
typedef struct _TRANSFER_RANGE_INFO_FRAME_
{
  uint8_t  cmdClass;               /* The command class is Z-Wave protocol */
  uint8_t  cmd;                    /* The command is CMD_TRANSFER_RANGE_INFO */
  uint8_t  seqNo;                  /* sequence number*/
  uint8_t  nodeID;                 /* The command is CMD_RANGE_INFO_RANGE */
  uint8_t  numMaskBytes;           /* The number of mask bytes in the frame */
  uint8_t  maskBytes[MAX_NODEMASK_LENGTH]; /* The first mask byte in the frame */
} TRANSFER_RANGE_INFO_FRAME;


/************************************************************/
/* Transfer Complete frame*/
/************************************************************/
typedef struct _TRANSFER_END_FRAME_
{
  uint8_t  cmdClass;     /* The command class is Z-Wave protocol */
  uint8_t  cmd;          /* The command is CMD_TRANSFER_COMPLETE */
  uint8_t  status;       /* Status of the previous command */
} TRANSFER_END_FRAME;


/************************************************************/
/* Transfer New primary Controller Complete frame*/
/************************************************************/
typedef struct _TRANSFER_NEW_PRIMARY_FRAME_
{
  uint8_t  cmdClass;     /* The command class is Z-Wave protocol */
  uint8_t  cmd;          /* The command is CMD_TRANSFER_NEW_PRIMARY_COMPLETE */
  uint8_t  sourceControllerType;     /* The type of the sender controller type*/
} TRANSFER_NEW_PRIMARY_FRAME;


/***********************************************************/
/* New node registered frame                               */
/**********************************************************/
typedef struct _NEW_NODE_REGISTERED_FRAME_
{
  uint8_t  cmdClass;       /* The command class is Z-Wave protocol */
  uint8_t  cmd;            /* The command is CMD_NEW_NODE_REGISTERED */
  uint8_t  nodeID;
  uint8_t  capability;     /* Network capabilities */
  uint8_t  security;       /* Network security */
  uint8_t  reserved;
  NODE_TYPE nodeType;   /* Basic, Generic and Specific Device type */
  uint8_t  nodeInfo[NODEPARM_MAX];    /* Device status */
} NEW_NODE_REGISTERED_FRAME;


typedef struct _NEW_NODE_REGISTERED_SLAVE_FRAME_
{
  uint8_t  cmdClass;       /* The command class is Z-Wave protocol */
  uint8_t  cmd;            /* The command is CMD_NEW_NODE_REGISTERED */
  uint8_t  nodeID;
  uint8_t  capability;     /* Network capabilities */
  uint8_t  security;       /* Network security */
  uint8_t  reserved;
  APPL_NODE_TYPE nodeType;  /* For Slaves we transmit Generic and Specific Device type */
  uint8_t  nodeInfo[NODEPARM_MAX];    /* Device status */
} NEW_NODE_REGISTERED_SLAVE_FRAME;


typedef struct _NEW_NODE_REGISTERED_OLD_FRAME_
{
  uint8_t  cmdClass;       /* The command class is Z-Wave protocol */
  uint8_t  cmd;            /* The command is CMD_NEW_NODE_REGISTERED */
  uint8_t  nodeID;
  uint8_t  capability;     /* Network capabilities */
  uint8_t  security;       /* Network security */
  uint8_t  reserved;
  uint8_t  nodeType;       /* Device type */
  uint8_t  nodeInfo[NODEPARM_MAX];    /* Device status */
} NEW_NODE_REGISTERED_OLD_FRAME;


typedef struct _NEW_RANGE_REGISTERED_FRAME_
{
  uint8_t  cmdClass;               /* The class is Z-Wave protocol */
  uint8_t  cmd;                    /* The command is CMD_NEW_RANGE_REGISTERED */
  uint8_t  nodeID;
  uint8_t  numMaskBytes;           /* The number of mask bytes in the frame */
  uint8_t  maskBytes[MAX_NODEMASK_LENGTH]; /* The first mask byte in the frame */
} NEW_RANGE_REGISTERED_FRAME;


/***********************************************************/
/* command to ask an static controller to be SUC           */
/**********************************************************/
typedef struct _SET_SUC_FRAME_
{
  uint8_t cmdClass;                /* The class is Z-Wave protocol */
  uint8_t cmd;                     /* The command is CMD_SET_SUC */
  uint8_t state;                   /*Disable or enable the functinalit */
  uint8_t SUCcapabilities;         /* The capabilities of the SUC */
} SET_SUC_FRAME;


/***********************************************************/
/* frame returning the result of the CMD_ST_SUC command    */
/**********************************************************/
typedef struct _SET_SUC_ACK_FRAME_
{
  uint8_t cmdClass;                /* The class is Z-Wave protocol */
  uint8_t cmd;                     /* The command is CMD_SET_SUC_ACK */
  uint8_t result;                  /* the result of the CMD_SET_SUC command; SUC_ENABLED_MASK: accepted, false: rejected*/
  uint8_t SUCcapabilities;         /* The capabilities of the SUC */
} SET_SUC_ACK_FRAME;


#define ID_SERVER_RUNNING_BIT   0x01

/***********************************************************/
/* Frame used for reserving a number of node ID in         */
/* the nodeID server.                                      */
/***********************************************************/
typedef struct _RESERVE_NODE_IDS_FRAME_
{
  uint8_t cmdClass;                /* The class is Z-Wave protocol */
  uint8_t cmd;                     /* The command is ZWAVE_CMD_RESERVE_NODE_IDS */
  uint8_t numberOfIDs;             /* The number of ID that should be reservedby the server */
} RESERVE_NODE_IDS_FRAME;

#define NUMBER_OF_IDS_MASK    0x0F  /* The number of IDS is a 4 bit field */

/***********************************************************/
/* Frame used for sending the reserved node IDs to         */
/* the controller that requested them.                     */
/***********************************************************/
typedef struct _RESERVED_IDS_FRAME_
{
  uint8_t cmdClass;                /* The class is Z-Wave protocol */
  uint8_t cmd;                     /* The command is ZWAVE_CMD_RESERVED_IDS */
  uint8_t numberOfIDs;             /* The number of ID that should be reservedby the server */
  uint8_t reservedID1;             /* First Reserved nodeID */
} RESERVED_IDS_FRAME;

#endif /* ZW_CONTROLLER */

/* common frame header layout */
typedef union _transmitHeader_
{
  frameHeader                 header;
  frameHeaderSinglecast       singlecast;
  frameHeaderSinglecastRouted singlecastRouted;
#if defined(ZW_CONTROLLER) || defined(ZW_SLAVE_ROUTING)
  frameHeaderMulticast        multicast;
#endif
  frameHeaderExplore          explore;
} transmitHeader;


/* Current active transmission */
typedef struct _activeTransmit_
{
  uint8_t  nodeIndex;        /* Current node within the Multicast node list */
#ifdef ZW_SLAVE_ROUTING
  uint8_t  routeIndex;       /* Current route within the Return Route list */
#endif
  /* TO#1627 fix - ZW_RequestNodeNeighborUpdate fails if questioned node is routed and first routing attempt fails. */
  uint8_t routeType; /* true if responseRoute */
} ACTIVE_TRANSMIT;


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

extern bool bUseExploreAsRouteResolution;

extern ACTIVE_TRANSMIT activeTransmit;   /* Current active transmission */

extern bool bLastTxFailed;               /* Status of the last transmission */

extern uint8_t bRestartAckTimerAllowed;  /* 1 - Restart of timer is allowed */

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 * Wrapper for EnQueueSingleData for the tranmission of the same frame on both LR channels.
 *
 * @param[in] rfSpeed Speed to be used for transmission.
 * @param[in] srcNodeID Source node ID.
 * @param[in] destNodeID Destination node ID.
 * @param[in] pData Pointer to data.
 * @param[in] dataLength Length of data to be transmitted.
 * @param[in] txPower Transmit power.
 * @param[in] pCompletedFunc Transmit completed call back function.
 * @return Returns false if transmitter is busy.
 */
bool
EnQueueSingleDataOnLRChannels(
  uint8_t    rfSpeed,
  node_id_t  srcNodeID,
  node_id_t  destNodeID,
  uint8_t   *pData,
  uint8_t    dataLength,
  uint32_t   delayedTxMs,
  uint8_t    txPower,
  const STransmitCallback* pCompletedFunc);

/**
 * Transmit data buffer to a single Z-Wave node or all Z-Wave nodes (broadcast).
 *
 * This is a compatibility wrapper for EnQueueSingleDataPtr().
 * @param[in] rfSpeed Speed to be used for transmission.
 * @param[in] srcNodeID Source node ID.
 * @param[in] destNodeID Destination node ID.
 * @param[in] pData Pointer to data.
 * @param[in] dataLength Length of data to be transmitted.
 * @param[in] txOptions Transmit option flags.
 * @param[in] delayedTxMs The delay of the transmission in ms, if needed.
 * @param[in] txPower Transmit power.
 * @param[in] pCompletedFunc Transmit completed call back function.
 * @return Returns false if transmitter is busy.
 */
bool
EnQueueSingleData(
  uint8_t      rfSpeed,
  node_id_t    srcNodeID,
  node_id_t    destNodeID,
  const uint8_t *pData,
  uint8_t      dataLength,
  TxOptions_t  txOptions,
  uint32_t     delayedTxMs,
  uint8_t      txPower,
  const STransmitCallback* pCompletedFunc);

/*===============================   SetSpeedOptions   ===========================
**    Set the speed options in a given value
**
**    Side effects:
**-------------
-------------------------------------------------------------*/
uint8_t
SetSpeedOptions(
  uint8_t bOldOptions,
  uint8_t bNewOption);

#ifdef ZW_SLAVE
uint8_t /*The baud rate of the frame.*/
ChooseSpeedForDestination(uint8_t bNodeID);
#endif

#ifdef ZW_RETURN_ROUTE_PRIORITY

#define RETURN_ROUTE_PRIORITY_SUC ZW_MAX_RETURN_ROUTE_DESTINATIONS

/*========================  ReturnRouteResetPriority  ========================
**    Set all the return route priority to default values
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void ReturnRouteResetPriority(void);

/*========================  ReturnRouteClearPriority  ==================
**    Set the return route priority to default values
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void ReturnRouteClearPriority(uint8_t bDestIndex);

/*========================  ReturnRouteFindPriority  ==================
**    Find the index of a route with a given priority
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t ZCB_ReturnRouteFindPriority(uint8_t bPriority, uint8_t bRouteIndex);


/*========================  ReturnRouteChangePriority  ==================
**    Change the priority of the return routes so the given index is the
**    new first priority.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void ReturnRouteChangePriority(uint8_t bRouteIndex);
#endif  /* ZW_RETURN_ROUTE_PRIORITY */

/*==========================   IsExploreRequested   ==========================
**    Build the specified frame header.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
bool
IsExploreRequested(void);


/*=========================   TransportEnqueueExploreFrame   =================
**    Transmit data buffer via a Z-Wave Explore frame to destination.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                                     /* RET false if transmitter busy             */
TransportEnQueueExploreFrame(
  frameExploreStruct     *pFrame,           /* IN Frame pointer                          */
  TxOptions_t             exploreTxOptions, /* IN Transmit option flags                  */
  ZW_SendData_Callback_t);                  /* IN Transmit completed call back function  */

#ifdef ZW_SLAVE
/*============================   ZW_LockRoute   ==============================
**    Function description
**      This function locks and unlocks any temporary route to a specific nodeID
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZW_LockRoute(
  uint8_t bNodeID);          /* IN if nonezero lock bNodeID entry, */
                          /*    zero unlock entry */

#endif  /* ZW_SLAVE */

#ifdef ZW_CONTROLLER
/*=============================   ZW_LockRoute   ============================
**    Function description
**      IF bLockRoute TRUE then any attempt to purge a LastWorkingRoute entry
**      is denied.
**
**    Side effects:
**
**
**--------------------------------------------------------------------------*/
void
ZW_LockRoute(
  bool bLockRoute);       /* IN TRUE lock LastWorkingRoute entry purging */
                          /*    FALSE unlock LastWorkingRoute entry purging */

/**
   * Return if current LastWorkingRoute is locked 
   * 
   * @return true if current LastWorkingRoute is locked for update/change
*/
bool
ZW_LockRouteGet();

#endif /* #ifdef ZW_CONTROLLER */

/*============================   RetransmitFail   ===========================
**    Retransmit frame failed. Try new routes or permanently give up.
**
**    Assumptions:
**      pFrameWaitingForACK points to the failed frame.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void                    /* RET Nothing */
RetransmitFail( void ); /* IN  Nothing */

/*===============================   TxRandom   ==============================
**  Transmit two bits pseudo-random number function
**
**
**--------------------------------------------------------------------------*/
uint8_t            /* RET Random number (0|1|2|3) */
TxRandom( void );  /* IN Nothing */


void
transportInitReceiveFilters(void);

/*============================   TransportInit   ============================
**    Initializes transport layer data.
**
**    Side effects: none
**
**--------------------------------------------------------------------------*/
void TransportInit(zpal_radio_profile_t* const pRfProfile);

#ifdef ZW_SLAVE
/*============================   FlushResponseSpeeds   =======================
**    Flush the stores response speeds.
**
**    Side effects:
**--------------------------------------------------------------------------*/
void
FlushResponseSpeeds();
#endif

#ifdef ZW_SLAVE
/*================   ChooseSpeedForDestination_slave   =======================
**    This function selects the speed to send a frame with, taking the
**    destination nodeid as input.
**
**    The following is used to select speed:
**     - use response speed saved from last 2 frames received at each speed.
**     - otherwise use 9.6k which everyone supports.
**
**    Speed for return routes and response routes are set up in ReturnRouteFindNext()
**    and GetResponseRoute().
**
**    Side effects:
**--------------------------------------------------------------------------*/
uint8_t
ChooseSpeedForDestination_slave(      /* RET Chosen speed RF_SPEED_* */
  node_id_t pNodeID                   /* IN  Destination nodeID      */
);
#endif /* ZW_SLAVE */


/*==========================   TransportSetCurrentRxCh  ======================
**    Set the RF channel last frame was received on.
**
**    Side effects:
**--------------------------------------------------------------------------*/
void
TransportSetCurrentRxChannel(
  uint8_t rfRxCurrentCh);     /* IN   RF channel last frame was received on */


/*==========================   TransportGetCurrentRxCh   ======================
**    Return the RF channel last frame was received on. Used for ACK tx.
**
**    Side effects:
**--------------------------------------------------------------------------*/
uint8_t                         /* RET  RF channel last frame was received on */
TransportGetCurrentRxChannel(void);


void
TransportSetCurrentRxSpeed(uint8_t rfRxCurrentSpeed);


void
TransportSetCurrentRxSpeedThroughProfile(uint32_t profile);


uint8_t
TransportGetCurrentRxSpeed(void);


/*==========================   TransportSetChannel   =========================
**    Set the RF channel the next frame should be send on
**
**    Side effects:
**--------------------------------------------------------------------------*/
void TransportSetChannel(uint8_t bChannel);


/*==========================   TransportGetChannel   =========================
**    Return the RF channel the next frame should be send on
**
**    Side effects:
**--------------------------------------------------------------------------*/
uint8_t                           /* RET  Channel frame should be send on */
TransportGetChannel(
  TxQueueElement *pFrameToSend);  /* IN Pointer to frame */


/*==========================   transportGetTxSpeed   =========================
**    Get the Tx power to be used with a given tx queue element
**
**    Side effects:
**--------------------------------------------------------------------------*/
int8_t
transportGetTxPower(TxQueueElement* element, ZW_HeaderFormatType_t headerFormat);


/*===========================   transportVarInit   ==========================
**    Function description.
**    	Initializes all global variables in transport module.
**
**    Side effects:
**     None
**--------------------------------------------------------------------------*/
void
transportVarInit(void);

/**
 * Send a beam ack
 *
 * The beam ACK is sent using LBT, if LBT fails the ACK is not sent.
 */
void
SendFragmentedBeamACK();


/*=========================   TransmitTimerStart   ==========================
**
**  Activate retransmit timeout timer.
**
**
**--------------------------------------------------------------------------*/
void                          /* RET Nothing */
TransmitTimerStart(
  VOID_CALLBACKFUNC(func)(),  /* IN Timeout function adddress         */
  uint32_t timerTicks);       /* IN Timeout value (value * 10 msec.)  */

/*=========================   TransmitTimerStop   ===========================
**
**  Stop retransmit timeout timer.
**
**
**--------------------------------------------------------------------------*/
void                       /* RET Nothing */
TransmitTimerStop( void ); /* IN  Nothing */

/*==================   ZW_EnableRoutedRssiFeedback   =======================
**
**  Enable or disable collection of routed rssi feedback on frames sent
**  to this node.
**
**-------------------------------------------------------------------------*/
void
ZW_EnableRoutedRssiFeedback(uint8_t bEnabled);

/*===========================   ZW_FollowUpReceived   ======================
**    A follow up frame was received, enable FLiRS broadcast in RF driver
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZW_FollowUpReceived(void);

enum ZW_SENDDATA_EX_RETURN_CODES
{
  ZW_TX_FAILED = 0,
  ZW_TX_IN_PROGRESS = 1
};

/*===============================   ZW_SendData   ===========================
**    Transmit data buffer to a single ZW-node or all ZW-nodes (broadcast).
**
**
**    txOptions:
**          TRANSMIT_OPTION_LOW_POWER     transmit at low output power level
**                                        (1/3 of normal RF range).
**          TRANSMIT_OPTION_ACK           request destination node for an acknowledge
**                                        that the frame has been received;
**                                        completedFunc (if NONE NULL) will return result.
**          TRANSMIT_OPTION_AUTO_ROUTE    request retransmission via repeater
**                                        nodes/return routes (at normal output power level).
**          TRANSMIT_OPTION_EXPLORE       Use explore frame route resolution if all else fails
**
** extern uint8_t              RET  false if transmitter queue overflow
** ZW_SendData(
** node_id_t    destNodeID,    IN  Destination node ID (0xFF == broadcast)
** uint8_t     *pData,         IN  Data buffer pointer
** uint8_t      dataLength,    IN  Data buffer length
** TxOptions_t  txOptions,     IN  Transmit option flags
** VOID_CALLBACKFUNC(completedFunc)( IN  Transmit completed call back function
**   uint8_t      txStatus,    IN  Transmit status
**   TX_STATUS_TYPE*));        IN  Transmit status report
**--------------------------------------------------------------------------*/
uint8_t                                     /* RET  false if transmitter busy      */
ZW_SendData(
  node_id_t                destNodeID,      /* IN  Destination node ID (0xFF == broadcast) */
  const uint8_t           *pData,           /* IN  Data buffer pointer           */
  uint8_t                  dataLength,      /* IN  Data buffer length            */
  TxOptions_t              txOptions,       /* IN  Transmit option flags         */
  const STransmitCallback* pCompletedFunc); /* IN  Transmit completed call back function  */


/*============================   ZW_SendData_Bridge   ========================
**    Transmit data buffer to a single ZW-node or all ZW-nodes (broadcast).
**
**
**    txOptions:
**          TRANSMIT_OPTION_LOW_POWER     transmit at low output power level
**                                        (1/3 of normal RF range).
**          TRANSMIT_OPTION_ACK           request destination node for an acknowledge
**                                        that the frame has been received;
**                                        completedFunc (if NONE NULL) will return result.
**          TRANSMIT_OPTION_AUTO_ROUTE    request retransmission via repeater
**                                        nodes (at normal output power level).
**
** extern uint8_t                     RET false if transmitter queue overflow
** ZW_SendData_Bridge(
**  node_id_t  bSrcNodeID,            IN Source NodeID - if 0xFF then controller ID is set as source
**  node_id_t  nodeID,                IN Destination node ID (0xFF == broadcast)
**  uint8_t *pData,                   IN Data buffer pointer
**  uint8_t  dataLength,              IN Data buffer length
**  TxOptions_t  txOptions,           IN Transmit option flags
**  VOID_CALLBACKFUNC(completedFunc)( IN  Transmit completed call back function
**    uint8_t txStatus,               IN  Transmit status
**    TX_STATUS_TYPE*));              IN  Transmit status report
**--------------------------------------------------------------------------*/
uint8_t                                     /* RET false if transmitter busy      */
ZW_SendData_Bridge(
  node_id_t                bSrcNodeID,      /* IN  Source NodeID - if 0xFF then controller ID is set as source */
  node_id_t                nodeID,          /* IN  Destination node ID (0xFF == broadcast) */
  uint8_t                 *pData,           /* IN  Data buffer pointer                     */
  uint8_t                  dataLength,      /* IN  Data buffer length                      */
  TxOptions_t              txOptions,       /* IN  Transmit option flags                   */
  const STransmitCallback* pCompletedFunc); /* IN  Transmit completed call back function   */

/*============================   ZW_SendDataAbort   ========================
**    Abort the ongoing application started transmission
**
**    This function aborts a transmission initiated by an application. The flag
**    TRANSMIT_OPTION_APPLICATION identifies the application initiated transmissions.
**    For the sake of this function, an ongoing transmission is one where the data
**    frame is waiting for an acknowledgment.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZW_SendDataAbort(void);


/*===============================   ZW_SendDataMulti   ======================
**    Transmit data buffer to a list of Z-Wave Nodes (multicast frame).
**
**
**    txOptions:
**          TRANSMIT_OPTION_LOW_POWER   transmit at low output power level (1/3 of
**                                      normal RF range).
**          TRANSMIT_OPTION_ACK         the multicast frame will be followed by a
**                                      singlecast frame to each of the destination nodes
**                                      and request acknowledge from each destination node.
**          TRANSMIT_OPTION_AUTO_ROUTE  request retransmission on singlecast frames via
**                                      repeater nodes/return routes (at normal output power level).
**
**--------------------------------------------------------------------------*/
uint8_t                                      /* RET  false if transmitter busy      */
ZW_SendDataMulti(
  uint8_t                  *pNodeIDList,     /* IN  List of destination node ID's */
  const uint8_t            *pData,           /* IN  Data buffer pointer           */
  uint8_t                   dataLength,      /* IN  Data buffer length            */
  TxOptions_t               txOptions,       /* IN  Transmit option flags         */
  const STransmitCallback*  pCompletedFunc); /* IN  Transmit completed call back function  */


/*===============================   ZW_SendDataMulti_Bridge   ================
**    Transmit data buffer to a list of Z-Wave Nodes (multicast frame).
**
**
**    txOptions:
**          TRANSMIT_OPTION_LOW_POWER   transmit at low output power level (1/3 of
**                                      normal RF range).
**          TRANSMIT_OPTION_ACK         the multicast frame will be followed by a
**                                      singlecast frame to each of the destination nodes
**                                      and request acknowledge from each destination node.
**          TRANSMIT_OPTION_AUTO_ROUTE  request retransmission on singlecast frames
**                                      via repeater nodes (at normal output power level).
**
**--------------------------------------------------------------------------*/
extern uint8_t                               /* RET  false if transmitter busy      */
ZW_SendDataMulti_Bridge(
  node_id_t                   bSrcNodeID,      /* IN Source nodeID - if 0xFF then controller is set as source */
  uint8_t                  *pNodeIDList,     /* IN List of destination node ID's */
  uint8_t                  *pData,           /* IN Data buffer pointer           */
  uint8_t                   dataLength,      /* IN Data buffer length            */
  TxOptions_t               txOptions,       /* IN Transmit option flags         */
  bool nodeid_list_lr,           /*IN type of the nodes in the node id list*/
  const STransmitCallback*  pCompletedFunc); /* IN  Transmit completed call back function  */

/**
* Send multicast security s2 encrypted frame.
* Only the MultiCast/Groupcast frame itself will be transmitted. There will be no single cast follow ups.
*
* \param pData             plaintext to which is going to be sent.
* \param dataLength        length of data to be sent.
* \param pTxOptionMultiEx  Transmit options structure containing the transmission source, transmit options and
*                          the groupID which is the connection handle for the mulicast group to use,
*
*/
enum ZW_SENDDATA_EX_RETURN_CODES                /* RET Return code      */
  ZW_SendDataMultiEx(
    uint8_t *pData,                             /* IN Data buffer pointer           */
    uint8_t  dataLength,                        /* IN Data buffer length            */
    TRANSMIT_MULTI_OPTIONS_TYPE *pTxOptionsMultiEx,
    const STransmitCallback* pCompletedFunc);   /* IN Transmit completed call back function */


/*===============================   ZW_SendDataEx   ===========================
**    Transmit data buffer to a single ZW-node or all ZW-nodes (broadcast).
**
**  This supersedes the old ZW_SendData and adds support for secure
**  transmissions.
**
**    pData                             Pointer to the payload data to be transmitted
**
**    dataLength                        Payload data length
**
**    pTxOptionsEx                      Points to Transmit options structure containing:
**
**      destNode
**        destination node id - 0xFF means broadcast to all nodes
**
**      bSrcNode
**        Reserved for future use.
**
**      txOptions:
**        TRANSMIT_OPTION_LOW_POWER     transmit at low output power level
**                                      (1/3 of normal RF range).
**        TRANSMIT_OPTION_ACK           the destination nodes
**                                      and request acknowledge from each
**                                      destination node.
**        TRANSMIT_OPTION_AUTO_ROUTE    request retransmission via return route.
**        TRANSMIT_OPTION_EXPLORE       Use explore frame route resolution if all else fails
**
**
**      securityKeys:
**
**
**      txOptions2
**
**
**--------------------------------------------------------------------------*/
enum ZW_SENDDATA_EX_RETURN_CODES             /* RET Return code      */
  ZW_SendDataEx(
    uint8_t const * const pData,             /* IN Data buffer pointer           */
    uint8_t  dataLength,                     /* IN Data buffer length            */
    TRANSMIT_OPTIONS_TYPE *pTxOptionsEx,
    const STransmitCallback* pCompletedFunc);

#ifdef ZW_CONTROLLER

/**
   * Update the list of nodes we received beam ack from them
   * @param[in] node_id  node id of a node that has send a beam ack
   * 
   * @return true if it the first time we received the beam ack else false
*/

bool
ZW_broadcast_beam_ack_id_update(node_id_t node_id);

#endif
#ifdef ZW_BEAM_RX_WAKEUP
/**
   * Check if we can send beam ack frame. 
   * if we already sent a beam ack then we are not send a new ackbefore a delay of
   * BEAM_TRAIN_DURATION_MS has past
   *
   * @return true if we can send beam ack frame, else true
*/
bool 
SendBeamAckAllowed(void);
#endif

typedef struct {
  uint8_t  *cmd;           /* IN  Frame payload pointer */
  uint8_t  cmdLength;      /* IN  Payload length        */
  RECEIVE_OPTIONS_TYPE *rxOpt;
#ifdef ZW_CONTROLLER_BRIDGE
  uint8_t *multi;      /* IN  Pointer to Multicast structure which indicates which nodes a received */
                     /*     multicast frame are destined for */
#endif
} CommandHandler_arg_t;

/**
 * Executes the received Z-Wave command or forwards it to the Application Layer.
 *
 * @param[in] pArgs Information related to the received command.
 */
extern void CommandHandler(CommandHandler_arg_t * pArgs);

#endif /* _ZW_TRANSPORT_H_ */
