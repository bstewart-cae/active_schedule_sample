// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_s2_inclusion_glue.h
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef _ZW_S2_INCLUSION_GLUE_H_
#define _ZW_S2_INCLUSION_GLUE_H_
#include <ZW_security_api.h>
#include <ZW_secure_learn_support.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
*  @param[in] p_context    Pointer to the context with information of peer to join.
*/
void s2_inclusion_glue_join_start(struct S2 *p_context);

void s2_inclusion_glue_set_securelearnCbFunc(sec_learn_complete_t psecureLearnCbFunc);

/**
 * Initiates the S2 inclusion engine.
 * @param keys Keys to request during inclusion.
 */
void ZW_s2_inclusion_init(uint8_t keys);

#endif /* _ZW_S2_INCLUSION_GLUE_H_ */
