// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_ctimer.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_ctimer.h"
#include "unity.h"
#include "mock_control.h"
#include <ZW_timer.h>
#include <SwTimer.h>

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

struct ctimer a,b,c;


void foo(void *a) {

}

void test_timer_order1(void) {

  mock_call_use_as_stub(TO_STR(ZwTimerRegister));
  mock_call_use_as_stub(TO_STR(TimerStart));
  mock_call_use_as_stub(TO_STR(xTaskGetTickCount));

  ctimer_init();

  ctimer_set(&a,10,foo,&a);
  ctimer_set(&b,20,foo,&b);
  ctimer_set(&c,30,foo,&c);

  TEST_ASSERT_EQUAL( a.next , &b );
  TEST_ASSERT_EQUAL( b.next , &c );
  TEST_ASSERT_EQUAL( c.next , 0 );

}


void test_timer_order2(void) {

  mock_call_use_as_stub(TO_STR(ZwTimerRegister));
  mock_call_use_as_stub(TO_STR(TimerStart));
  mock_call_use_as_stub(TO_STR(xTaskGetTickCount));

  ctimer_init();

  ctimer_set(&b,20,foo,&b);
  ctimer_set(&a,10,foo,&a);
  ctimer_set(&c,30,foo,&c);

  TEST_ASSERT_EQUAL( a.next , &b );
  TEST_ASSERT_EQUAL( b.next , &c );
  TEST_ASSERT_EQUAL( c.next , 0 );

}

void test_timer_order3(void) {

  mock_call_use_as_stub(TO_STR(ZwTimerRegister));
  mock_call_use_as_stub(TO_STR(TimerStart));
  mock_call_use_as_stub(TO_STR(xTaskGetTickCount));

  ctimer_init();

  ctimer_set(&a,10,foo,&a);
  ctimer_set(&c,30,foo,&c);
  ctimer_set(&b,20,foo,&b);

  TEST_ASSERT_EQUAL( a.next , &b );
  TEST_ASSERT_EQUAL( b.next , &c );
  TEST_ASSERT_EQUAL( c.next , 0 );

}


static bool timers_flags[3] = {false};

void timer_a(void *a) {
  timers_flags[0] = true;

}

void timer_b(void *a) {
  timers_flags[1] = true;
}

void timer_c(void *a) {
  timers_flags[2] = true;
}

typedef void(*timer_callback_t)(SSwTimer* pTimer);

void test_timer_overflow(void) {
  mock_t * pMockTimer;
  mock_t * pMockTick;  
  mock_t * pMock;
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(TimerStop));  

  mock_call_expect(TO_STR(ZwTimerRegister), &pMockTimer);
  pMockTimer->compare_rule_arg[0] = COMPARE_ANY;
  pMockTimer->compare_rule_arg[1] = COMPARE_ANY;
  pMockTimer->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMockTimer->return_code.v = true;


  mock_call_expect(TO_STR(xTaskGetTickCount), &pMockTick);
  pMockTick->return_code.v = 0;
  
  ctimer_init();
  
  timer_callback_t CallBack = pMockTimer->actual_arg[2].p;

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->expect_arg[1].v = 10;  

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMockTick);
  pMockTick->return_code.v = 0xFFFFFF00;
  ctimer_set(&a,10,timer_a,&a);

  TEST_ASSERT_EQUAL( timers_flags[0] , false);
  TEST_ASSERT_EQUAL( timers_flags[1] , false);
  TEST_ASSERT_EQUAL( timers_flags[2] , false);  
  //when we add timer b 5 ticks have passed then tmer a has 5 ticks remaining
  mock_call_expect(TO_STR(xTaskGetTickCount), &pMockTick);
  pMockTick->return_code.v = 0xFFFFFF05;
  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->expect_arg[1].v = 5;

  ctimer_set(&b,1000,timer_b,&b);

  TEST_ASSERT_EQUAL( timers_flags[0] , false);
  TEST_ASSERT_EQUAL( timers_flags[1] , false);
  TEST_ASSERT_EQUAL( timers_flags[2] , false);  

  // when we add timer c 8 ticks has passdd then timer a has 2 ticks remaining and tiemr a has 997 remaining
  mock_call_expect(TO_STR(xTaskGetTickCount), &pMockTick);
  pMockTick->return_code.v = 0xFFFFFF08;
  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->expect_arg[1].v = 2;

  ctimer_set(&c,2000,timer_c,&c);

  TEST_ASSERT_EQUAL( timers_flags[0] , false);
  TEST_ASSERT_EQUAL( timers_flags[1] , false);
  TEST_ASSERT_EQUAL( timers_flags[2] , false);  

  TEST_ASSERT_EQUAL( a.next , &b );
  TEST_ASSERT_EQUAL( b.next , &c );
  TEST_ASSERT_EQUAL( c.next , 0 );

  // if we assume the timer call back is called after 11 ticks then timer b has 994 ticks and timer c has 2997 ticks remaining
  mock_call_expect(TO_STR(xTaskGetTickCount), &pMockTick);
  pMockTick->return_code.v = 0xFFFFFF0B;

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->expect_arg[1].v = (1000 - 6);

  CallBack(NULL);
  TEST_ASSERT_EQUAL( timers_flags[0] , true );
  TEST_ASSERT_EQUAL( timers_flags[1] , false );
  TEST_ASSERT_EQUAL( timers_flags[2] , false );  
  
  timers_flags[0] = false;

  // if we assume the timer call back is called after 994 ticks timer c has 1003 ticks remaining
  mock_call_expect(TO_STR(xTaskGetTickCount), &pMockTick);
  pMockTick->return_code.v = (0xFFFFFF0B + (1000 -6));

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->expect_arg[1].v =  2000 - 994 -3; 

  CallBack(NULL);
  TEST_ASSERT_EQUAL( timers_flags[0] , false );
  TEST_ASSERT_EQUAL( timers_flags[1] , true );
  TEST_ASSERT_EQUAL( timers_flags[2] , false );  
  mock_calls_verify();
}
