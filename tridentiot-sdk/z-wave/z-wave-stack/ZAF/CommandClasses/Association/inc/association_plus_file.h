/**
 * @file
 * Helper module for Command Class Association and Command Class Multi Channel Association.
 * @copyright 2022 Silicon Laboratories Inc.
 */

#ifndef _ASSOCIATION_PLUS_FILE_H_
#define _ASSOCIATION_PLUS_FILE_H_

#include "zaf_config.h"

/**
 * @addtogroup CC Command Classes
 * @{
 * @addtogroup Association
 * @{
 */

/**
 * @brief Contains associations for one group.
 */
typedef struct
{
  MULTICHAN_NODE_ID subGrp[CC_ASSOCIATION_MAX_NODES_IN_GROUP];
}
ASSOCIATION_GROUP;

/**
 * @brief Contains associations for one group packed for the file system.
 */
typedef struct
{
  MULTICHAN_NODE_ID_PACKED subGrp[CC_ASSOCIATION_MAX_NODES_IN_GROUP];
}
ASSOCIATION_GROUP_PACKED;

// Used to save association data to file system.
typedef struct SAssociationInfo
{
  ASSOCIATION_GROUP_PACKED Groups[ZAF_CONFIG_NUMBER_OF_END_POINTS + 1][CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT];  // +1 is for creating a rootdevice endpoint
} SAssociationInfo;

#define ZAF_FILE_SIZE_ASSOCIATIONINFO     (sizeof(SAssociationInfo))

/**
 * @}
 * @}
 */
#endif /* _ASSOCIATION_PLUS_FILE_H_ */
