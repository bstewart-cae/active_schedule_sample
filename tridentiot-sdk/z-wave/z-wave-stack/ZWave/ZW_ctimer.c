/**
 * @file ZW_ctimer.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
/*
 * ctimers from contiki ported to Z-Wave.
 *
 *  Created on: 18/01/2013
 *      Author: jbu
 */

/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * $Id: ctimer.c,v 1.1 2010/06/14 07:34:36 adamdunkels Exp $
 */

/**
 * \file
 *         Callback timer implementation
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "linked_list.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "ZW_ctimer.h"
#include <ZW_timer.h>
#include <SwTimer.h>

LIST(ctimer_list);

static SSwTimer m_CTimerSwTimer;
static clock_time_t last_ticks;
static void ctimer_timerCallback(SSwTimer* pTimer);

static void adjust_timeout(void) {
  struct ctimer *c = list_head(ctimer_list);
  /*Math here assume that the compiler would always handle the variables as 32-bit*/
  clock_time_t current_ticks = xTaskGetTickCount();
  clock_time_t elapsed_ticks = current_ticks - last_ticks;
  last_ticks = current_ticks;

  if(c) {
    for (; c != NULL; c = c->next)
    {
      if (c->timeout > elapsed_ticks) {
        c->timeout -= elapsed_ticks;
      } else {
        c->timeout = 0;
      }
    }
  }
}

static void remove_expired_timers (void) {
  struct ctimer *elm = list_head(ctimer_list);
  for (; elm != NULL; elm = elm->next)
   {
     // f the timer alrady expired then call the callback function thne remove it from the list
     if (0 == elm->timeout) {
       /* call the timeout function */
       if (elm->f != NULL) elm->f(elm->ptr);
       list_remove( ctimer_list, elm);
     }
  }
}
/**
 * Update our timer to fire on the next timeout
 */
static void update_timer(void) {
  remove_expired_timers();
  struct ctimer *c = list_head(ctimer_list);
  if((NULL != c) && c->timeout) {
    TimerStart(&m_CTimerSwTimer, c->timeout );
  } else {
    TimerStop(&m_CTimerSwTimer);
  }
}

static void ctimer_timerCallback(__attribute__((unused)) SSwTimer* pTimer)
{
  struct ctimer *c = list_pop(ctimer_list);

  /* call the timeout function */
  if (c->f != NULL) c->f(c->ptr);
  adjust_timeout();
  update_timer();
}



void ctimer_init(void)
{
  list_init(ctimer_list);
  last_ticks = xTaskGetTickCount();
  // Initialize timer
  ZwTimerRegister(&m_CTimerSwTimer, false, ctimer_timerCallback);
}

void
ctimer_set(struct ctimer *new_timer, clock_time_t t,
           void (*f)(void *), void *ptr)
{
  struct ctimer *cur;

  /*Make sure that the timer is not already in the list*/
  list_remove( ctimer_list, new_timer);
  adjust_timeout();
  new_timer->timeout = t;
  new_timer->f = f;
  new_timer->ptr = ptr;

  if (!list_length(ctimer_list))
    // List is empty, simply push new elem
    list_push(ctimer_list, new_timer);
  else
  {
    // Try to find the right place to insert new_elem
    cur = list_head(ctimer_list);
    // new_elem has to be inserted in front of the list
    if (cur->timeout > t)
      list_push(ctimer_list, new_timer);
    else
    {
      while (cur)
      {
        if (!cur->next /* cur is the last elem of the queue */
            || cur->next->timeout > t /* new_elem has to be inserted between cur & cur->next */)
          break;
        cur = cur->next;
      };
      list_insert(ctimer_list, cur, new_timer);
    }
  }
  update_timer();
}

void
ctimer_stop(struct ctimer *c)
{
  list_remove(ctimer_list, c);
  adjust_timeout();
  update_timer();
}

/*---------------------------------------------------------------------------*/
int
ctimer_expired(struct ctimer *c)
{
  struct ctimer *t;
  for(t = list_head(ctimer_list); t != NULL; t = t->next) {
    if(t == c) {
      return 0;
    }
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
