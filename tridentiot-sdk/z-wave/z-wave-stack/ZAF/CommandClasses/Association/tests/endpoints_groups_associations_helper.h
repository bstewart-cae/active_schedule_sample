#ifndef __GROUP_ENDPOINT_ASSOCIATIONS__
#define __GROUP_ENDPOINT_ASSOCIATIONS__


#include <stdint.h>
#include <stdbool.h>
#include <SizeOf.h>
#include <Assert.h>
#include <cc_association_config.h>
#include <ZAF_types.h>
#include <ZW_TransportEndpoint.h>
#include <SizeOf.h>
#include <cc_agi_config_api.h>

static const char GROUP_NAME_ENDPOINT_1_GROUP_2[] = "Button 1A";
static const char GROUP_NAME_ENDPOINT_1_GROUP_3[] = "Button 1B";
static const char GROUP_NAME_ENDPOINT_2_GROUP_2[] = "Button 2A";
static const char GROUP_NAME_ENDPOINT_2_GROUP_3[] = "Button 2B";
static const char GROUP_NAME_ENDPOINT_3_GROUP_2[] = "Button 3A";
static const char GROUP_NAME_ENDPOINT_3_GROUP_3[] = "Button 3B";
static const char GROUP_NAME_ENDPOINT_4_GROUP_2[] = "Button 4A";
static const char GROUP_NAME_ENDPOINT_4_GROUP_3[] = "Button 4B";

static const ccc_pair_t COMMANDS_ENDPOINT_1_GROUP_2[] = {
                                                          {
                                                            .cmdClass = COMMAND_CLASS_BASIC_V2,
                                                            .cmd = BASIC_SET_V2
                                                          },
};
static const ccc_pair_t COMMANDS_ENDPOINT_1_GROUP_3[] = {
                                                          {
                                                            .cmdClass = COMMAND_CLASS_CENTRAL_SCENE_V2,
                                                            .cmd = CENTRAL_SCENE_NOTIFICATION_V2
                                                          },
};
static const ccc_pair_t COMMANDS_ENDPOINT_2_GROUP_2[] = {
                                                          {
                                                            .cmdClass = COMMAND_CLASS_BASIC_V2,
                                                            .cmd = BASIC_SET_V2
                                                          },
};
static const ccc_pair_t COMMANDS_ENDPOINT_2_GROUP_3[] = {
                                                          {
                                                           .cmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL_V4,
                                                           .cmd = SWITCH_MULTILEVEL_START_LEVEL_CHANGE_V4
                                                          },
                                                          {
                                                           .cmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL_V4,
                                                           .cmd = SWITCH_MULTILEVEL_SET
                                                          },
};
static const ccc_pair_t COMMANDS_ENDPOINT_3_GROUP_2[] = {
                                                          {
                                                            .cmdClass = COMMAND_CLASS_BASIC_V2,
                                                            .cmd = BASIC_SET_V2
                                                          },
};
static const ccc_pair_t COMMANDS_ENDPOINT_3_GROUP_3[] = {
                                                          {
                                                           .cmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL_V4,
                                                           .cmd = SWITCH_MULTILEVEL_SET
                                                          },
};
static const ccc_pair_t COMMANDS_ENDPOINT_4_GROUP_2[] = {
                                                          {
                                                            .cmdClass = COMMAND_CLASS_BASIC_V2,
                                                            .cmd = BASIC_SET_V2
                                                          },
};
static const ccc_pair_t COMMANDS_ENDPOINT_4_GROUP_3[] = {
                                                          {
                                                           .cmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL_V4,
                                                           .cmd = SWITCH_MULTILEVEL_SET
                                                          },
};

static const cc_agi_group_t AGI_TABLE_ROOT_DEVICE_GROUPS[] =
{
 {
    .profile = {
     .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_CONTROL,
     .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_AGI_CONTROL_KEY01
    },
    .ccc_pair_count = sizeof_array(COMMANDS_ENDPOINT_1_GROUP_2),
    .ccc_pairs = COMMANDS_ENDPOINT_1_GROUP_2,
    .name = GROUP_NAME_ENDPOINT_1_GROUP_2,
    .name_length = sizeof(GROUP_NAME_ENDPOINT_1_GROUP_2),
 },
 {
    .profile = {
     .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_CONTROL,
     .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_AGI_CONTROL_KEY01
    },
    .ccc_pair_count = sizeof_array(COMMANDS_ENDPOINT_1_GROUP_3),
    .ccc_pairs = COMMANDS_ENDPOINT_1_GROUP_3,
    .name = GROUP_NAME_ENDPOINT_1_GROUP_3,
    .name_length = sizeof(GROUP_NAME_ENDPOINT_1_GROUP_3),
 },
 {
    .profile = {
     .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_CONTROL,
     .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_AGI_CONTROL_KEY02
    },
    .ccc_pair_count = sizeof_array(COMMANDS_ENDPOINT_2_GROUP_2),
    .ccc_pairs = COMMANDS_ENDPOINT_2_GROUP_2,
    .name = GROUP_NAME_ENDPOINT_2_GROUP_2,
    .name_length = sizeof(GROUP_NAME_ENDPOINT_2_GROUP_2),
 },
 {
    .profile = {
     .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_CONTROL,
     .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_AGI_CONTROL_KEY02
    },
    .ccc_pair_count = sizeof_array(COMMANDS_ENDPOINT_2_GROUP_3),
    .ccc_pairs = COMMANDS_ENDPOINT_2_GROUP_3,
    .name = GROUP_NAME_ENDPOINT_2_GROUP_3,
    .name_length = sizeof(GROUP_NAME_ENDPOINT_2_GROUP_3),
 },
 {
    .profile = {
     .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_CONTROL,
     .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_AGI_CONTROL_KEY03
    },
    .ccc_pair_count = sizeof_array(COMMANDS_ENDPOINT_3_GROUP_2),
    .ccc_pairs = COMMANDS_ENDPOINT_3_GROUP_2,
    .name = GROUP_NAME_ENDPOINT_3_GROUP_2,
    .name_length = sizeof(GROUP_NAME_ENDPOINT_3_GROUP_2),
 },
 {
    .profile = {
     .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_CONTROL,
     .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_AGI_CONTROL_KEY03
    },
    .ccc_pair_count = sizeof_array(COMMANDS_ENDPOINT_3_GROUP_3),
    .ccc_pairs = COMMANDS_ENDPOINT_3_GROUP_3,
    .name = GROUP_NAME_ENDPOINT_3_GROUP_3,
    .name_length = sizeof(GROUP_NAME_ENDPOINT_3_GROUP_3),
 },
 {
    .profile = {
     .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_CONTROL,
     .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_AGI_CONTROL_KEY04
    },
    .ccc_pair_count = sizeof_array(COMMANDS_ENDPOINT_4_GROUP_2),
    .ccc_pairs = COMMANDS_ENDPOINT_4_GROUP_2,
    .name = GROUP_NAME_ENDPOINT_4_GROUP_2,
    .name_length = sizeof(GROUP_NAME_ENDPOINT_4_GROUP_2),
 },
 {
    .profile = {
     .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_CONTROL,
     .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_AGI_CONTROL_KEY04
    },
    .ccc_pair_count = sizeof_array(COMMANDS_ENDPOINT_4_GROUP_3),
    .ccc_pairs = COMMANDS_ENDPOINT_4_GROUP_3,
    .name = GROUP_NAME_ENDPOINT_4_GROUP_3,
    .name_length = sizeof(GROUP_NAME_ENDPOINT_4_GROUP_3),
 },
};

static const cc_agi_group_t AGI_TABLE_ENDPOINT_1_GROUPS[] =
{
 {
    .profile = {
     .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_CONTROL,
     .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_AGI_CONTROL_KEY01
    },
    .ccc_pair_count = sizeof_array(COMMANDS_ENDPOINT_1_GROUP_2),
    .ccc_pairs = COMMANDS_ENDPOINT_1_GROUP_2,
    .name = GROUP_NAME_ENDPOINT_1_GROUP_2,
    .name_length = sizeof(GROUP_NAME_ENDPOINT_1_GROUP_2),
 },
 {
    .profile = {
     .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_CONTROL,
     .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_AGI_CONTROL_KEY01
    },
    .ccc_pair_count = sizeof_array(COMMANDS_ENDPOINT_1_GROUP_3),
    .ccc_pairs = COMMANDS_ENDPOINT_1_GROUP_3,
    .name = GROUP_NAME_ENDPOINT_1_GROUP_3,
    .name_length = sizeof(GROUP_NAME_ENDPOINT_1_GROUP_3),
 },
};

static const cc_agi_group_t AGI_TABLE_ENDPOINT_2_GROUPS[] =
{
 {
    .profile = {
     .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_CONTROL,
     .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_AGI_CONTROL_KEY02
    },
    .ccc_pair_count = sizeof_array(COMMANDS_ENDPOINT_2_GROUP_2),
    .ccc_pairs = COMMANDS_ENDPOINT_2_GROUP_2,
    .name = GROUP_NAME_ENDPOINT_2_GROUP_2,
    .name_length = sizeof(GROUP_NAME_ENDPOINT_2_GROUP_2),
 },
 {
    .profile = {
     .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_CONTROL,
     .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_AGI_CONTROL_KEY02
    },
    .ccc_pair_count = sizeof_array(COMMANDS_ENDPOINT_2_GROUP_3),
    .ccc_pairs = COMMANDS_ENDPOINT_2_GROUP_3,
    .name = GROUP_NAME_ENDPOINT_2_GROUP_3,
    .name_length = sizeof(GROUP_NAME_ENDPOINT_2_GROUP_3),
 },
};

static const cc_agi_group_t AGI_TABLE_ENDPOINT_3_GROUPS[] =
{
 {
    .profile = {
     .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_CONTROL,
     .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_AGI_CONTROL_KEY03
    },
    .ccc_pair_count = sizeof_array(COMMANDS_ENDPOINT_3_GROUP_2),
    .ccc_pairs = COMMANDS_ENDPOINT_3_GROUP_2,
    .name = GROUP_NAME_ENDPOINT_3_GROUP_2,
    .name_length = sizeof(GROUP_NAME_ENDPOINT_3_GROUP_2),
 },
 {
    .profile = {
     .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_CONTROL,
     .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_AGI_CONTROL_KEY03
    },
    .ccc_pair_count = sizeof_array(COMMANDS_ENDPOINT_3_GROUP_3),
    .ccc_pairs = COMMANDS_ENDPOINT_3_GROUP_3,
    .name = GROUP_NAME_ENDPOINT_3_GROUP_3,
    .name_length = sizeof(GROUP_NAME_ENDPOINT_3_GROUP_3),
 },
};

static const cc_agi_group_t AGI_TABLE_ENDPOINT_4_GROUPS[] =
{
 {
    .profile = {
     .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_CONTROL,
     .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_AGI_CONTROL_KEY04
    },
    .ccc_pair_count = sizeof_array(COMMANDS_ENDPOINT_4_GROUP_2),
    .ccc_pairs = COMMANDS_ENDPOINT_4_GROUP_2,
    .name = GROUP_NAME_ENDPOINT_4_GROUP_2,
    .name_length = sizeof(GROUP_NAME_ENDPOINT_4_GROUP_2),
 },
 {
    .profile = {
     .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_CONTROL,
     .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_AGI_CONTROL_KEY04
    },
    .ccc_pair_count = sizeof_array(COMMANDS_ENDPOINT_4_GROUP_3),
    .ccc_pairs = COMMANDS_ENDPOINT_4_GROUP_3,
    .name = GROUP_NAME_ENDPOINT_4_GROUP_3,
    .name_length = sizeof(GROUP_NAME_ENDPOINT_4_GROUP_3),
 },
};

cc_agi_config_t myAgi[] = {
                           {
                            .groups = AGI_TABLE_ROOT_DEVICE_GROUPS,
                            .group_count = sizeof_array(AGI_TABLE_ROOT_DEVICE_GROUPS)
                           },
                           {
                            .groups = AGI_TABLE_ENDPOINT_1_GROUPS,
                            .group_count = sizeof_array(AGI_TABLE_ENDPOINT_1_GROUPS)
                           },
                           {
                            .groups = AGI_TABLE_ENDPOINT_2_GROUPS,
                            .group_count = sizeof_array(AGI_TABLE_ENDPOINT_2_GROUPS)
                           },
                           {
                            .groups = AGI_TABLE_ENDPOINT_3_GROUPS,
                            .group_count = sizeof_array(AGI_TABLE_ENDPOINT_3_GROUPS)
                           },
                           {
                            .groups = AGI_TABLE_ENDPOINT_4_GROUPS,
                            .group_count = sizeof_array(AGI_TABLE_ENDPOINT_4_GROUPS)
                           },
};

/**
 * Endpoint config
 */
#define AGITABLE_LIFELINE_GROUP \
 {COMMAND_CLASS_CENTRAL_SCENE_V2, CENTRAL_SCENE_NOTIFICATION_V2}, \
 {COMMAND_CLASS_DEVICE_RESET_LOCALLY, DEVICE_RESET_LOCALLY_NOTIFICATION}


/**
 * Setup AGI lifeline table.
 */
CMD_CLASS_GRP  agiTableLifeLine[] = {AGITABLE_LIFELINE_GROUP};

/*
 * LifeLine group for all endpoints.
 * One define since they're all the same.
 */
#define AGITABLE_LIFELINE_GROUP_EP1_2_3_4 \
  {COMMAND_CLASS_CENTRAL_SCENE_V2, CENTRAL_SCENE_NOTIFICATION_V2}


/**
 * AGI table defining the lifeline groups for all four endpoints.
 */
CMD_CLASS_GRP  agiTableLifeLineEP1_2_3_4[] = {AGITABLE_LIFELINE_GROUP_EP1_2_3_4};


#endif
