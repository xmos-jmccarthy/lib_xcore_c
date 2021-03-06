// Copyright (c) 2016, XMOS Ltd, All rights reserved

#include <platform.h>
#include <xs1.h>

void channel_example(chanend c, chanend d);

void send_data(chanend c, int v, int delta) {
  timer tmr;
  int time;
  tmr :> time;
  time += delta;
  for (int i = 0; i < 5; i++) {
    // Provide test data
    c <: v + i;
    time += 20000;
    select {
      case tmr when timerafter(time) :> void:
        break;
    }
  }
}

/*
 * Create a channel tester which receives data from two other cores
 * simultaneously
 */
int main()
{
  chan c, d;
  par {
    channel_example(c, d);
    send_data(c, 555, 0);
    send_data(d, 333, 10);
  }
  return 0;
}
