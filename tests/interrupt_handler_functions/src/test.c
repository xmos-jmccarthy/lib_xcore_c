// Copyright (c) 2016, XMOS Ltd, All rights reserved

#include <stdio.h>
#include "xcore_c.h"
#include "debug_print.h"

// Timer period which allows enough time for the debug_printfs to complete
static const int period = 10000;

volatile int remaining_chan_c_reads = 30;
volatile int remaining_chan_d_reads = 30;

// For XS1 support all values passed to the interrupt setup function must have bit 16 set
typedef enum {
  CHAN_ID_1,
  CHAN_ID_2,
} chan_id_t;

typedef struct {
  chanend c;
  chan_id_t id;
} chan_data_t;

DEFINE_INTERRUPT_CALLBACK(groupA, channel_handler, data)
{
  chan_data_t *cd = (chan_data_t*)data;
  switch (cd->id) {
    case CHAN_ID_1: {
      uint32_t x;
      chan_in_word(cd->c, &x);
      remaining_chan_c_reads--;
      debug_printf("Received %d on channel c\n", x);
      break;
    }
    case CHAN_ID_2: {
      uint32_t x;
      chan_in_word(cd->c, &x);
      remaining_chan_d_reads--;
      debug_printf("Received %d on channel d\n", x);
      break;
    }
  }
}

void unmasked_chan_config(const chan_data_t *cd1, const chan_data_t *cd2)
{
  interrupt_unmask_all();
  chanend_enable_trigger(cd1->c);
  chanend_enable_trigger(cd2->c);
}

void masked_chan_config(const chan_data_t *cd1, const chan_data_t *cd2)
{
  interrupt_mask_all();
  chanend_enable_trigger(cd1->c);
  chanend_enable_trigger(cd2->c);
  interrupt_unmask_all();
}

void disable_chans_interrupts(const chan_data_t *cd1, const chan_data_t *cd2)
{
  interrupt_mask_all();
  chanend_disable_trigger(cd1->c);
  chanend_disable_trigger(cd2->c);
}

void test_interrupt_function(const chan_data_t *cd1, const chan_data_t *cd2, int n)
{
  chanend_setup_interrupt_callback(cd1->c, (void*)cd1, INTERRUPT_CALLBACK(channel_handler));
  chanend_setup_interrupt_callback(cd2->c, (void*)cd2, INTERRUPT_CALLBACK(channel_handler));
  unmasked_chan_config(cd1, cd2);
  // Wait for some interrupts, c1 and c2 might not have raised the same number
  while (remaining_chan_c_reads > (n + 5) && remaining_chan_d_reads > (n + 5));
  disable_chans_interrupts(cd1, cd2);

  masked_chan_config(cd1, cd2);
  // Wait for all of the remaining interrupts
  while (!(remaining_chan_c_reads <= n && remaining_chan_d_reads <= n));
  disable_chans_interrupts(cd1, cd2);
}

typedef struct {
  hwtimer_t t;
  int v;
} hwtimer_data_t;

/*
 * Interrupt handler function. Passed the timer that triggered the interrupt and
 * user data associated with that resource. This simply updates the timer to
 * fire after the period again.
 */
DEFINE_INTERRUPT_CALLBACK(groupA, hwtimer_handler, data)
{
  hwtimer_data_t *td = (hwtimer_data_t*)data;
  uint32_t time;
  hwtimer_get_time(td->t, &time);
  debug_printf("Timer interrupt data 0x%x\n", td->v);
  hwtimer_change_trigger_time(td->t, time + period);
}

DEFINE_INTERRUPT_PERMITTED(groupA, void, test, chanend c1, chanend c2)
{
  // Test enabling a timer outside of the interrupt loop with a function to handle
  // the interrupts.
  hwtimer_t t;
  hwtimer_alloc(&t);

  chan_data_t cd1 = {c1, CHAN_ID_1};
  chan_data_t cd2 = {c2, CHAN_ID_2};
  hwtimer_data_t td = {t, 0xfeedbeef};

  // Test 1: Run the test function with the timer enabled
  uint32_t time;
  hwtimer_get_time(t, &time);
  hwtimer_setup_interrupt_callback(t, time + period, &td, INTERRUPT_CALLBACK(hwtimer_handler));
  hwtimer_enable_trigger(t);
  test_interrupt_function(&cd1, &cd2, 20);
  hwtimer_disable_trigger(t);

  // Test 2: Run the test function again with the timer disabled
  test_interrupt_function(&cd1, &cd2, 10);

  // Test 3: Run the test function again with the timer enabled
  hwtimer_get_time(t, &time);
  hwtimer_setup_interrupt_callback(t, time + period, &td, INTERRUPT_CALLBACK(hwtimer_handler));
  hwtimer_enable_trigger(t);
  test_interrupt_function(&cd1, &cd2, 0);
  hwtimer_disable_trigger(t);
}
