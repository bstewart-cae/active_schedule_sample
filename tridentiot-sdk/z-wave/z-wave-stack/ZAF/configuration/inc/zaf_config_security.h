/**
 * @file
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef ZAF_CONFIG_SECURITY_H_
#define ZAF_CONFIG_SECURITY_H_

#include "zaf_config.h"
#include "ZW_security_api.h"

/**
 * @def S0_BIT
 * Contains the S0 bit if ZAF_CONFIG_REQUEST_KEY_S0 is set to 1.
 * @def S2_UNAUTHENTICATED_BIT
 * Contains the S2 Unauthenticated bit if ZAF_CONFIG_REQUEST_KEY_S2_UNAUTHENTICATED is set to 1.
 * @def S2_AUTHENTICATED_BIT
 * Contains the S2 Authenticated bit if ZAF_CONFIG_REQUEST_KEY_S2_AUTHENTICATED is set to 1.
 * @def S2_ACCESS_BIT
 * Contains the S2 Access bit if ZAF_CONFIG_REQUEST_KEY_S2_ACCESS is set to 1.
 */
#if ZAF_CONFIG_REQUEST_KEY_S0==1
#define S0_BIT                  SECURITY_KEY_S0_BIT
#else
#define S0_BIT                  0
#endif

#if ZAF_CONFIG_REQUEST_KEY_S2_UNAUTHENTICATED==1
#define S2_UNAUTHENTICATED_BIT  SECURITY_KEY_S2_UNAUTHENTICATED_BIT
#else
#define S2_UNAUTHENTICATED_BIT  0
#endif

#if ZAF_CONFIG_REQUEST_KEY_S2_AUTHENTICATED==1
#define S2_AUTHENTICATED_BIT    SECURITY_KEY_S2_AUTHENTICATED_BIT
#else
#define S2_AUTHENTICATED_BIT    0
#endif

#if ZAF_CONFIG_REQUEST_KEY_S2_ACCESS==1
#define S2_ACCESS_BIT           SECURITY_KEY_S2_ACCESS_BIT
#else
#define S2_ACCESS_BIT           0
#endif

/**
 * Holds the security keys configured by ZAF_CONFIG_REQUEST_KEY_S0,
 * ZAF_CONFIG_REQUEST_KEY_S2_UNAUTHENTICATED, ZAF_CONFIG_REQUEST_KEY_S2_AUTHENTICATED
 * and ZAF_CONFIG_REQUEST_KEY_S2_ACCESS.
 */
#define ZAF_CONFIG_REQUESTED_SECURITY_KEYS (S0_BIT | S2_UNAUTHENTICATED_BIT | S2_AUTHENTICATED_BIT | S2_ACCESS_BIT)

#endif /* ZAF_CONFIG_SECURITY_H_ */
