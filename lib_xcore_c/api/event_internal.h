// Copyright (c) 2016, XMOS Ltd, All rights reserved

#ifndef __event_internal_h__
#define __event_internal_h__

#ifndef __XC__

#include <stdint.h>
#include <assert.h>
#include <xccompat.h>
#include "resource.h"
#include "port.h"

#ifndef EVENT_MAX_HANDLER_FUNCTIONS
/**
 * The default maximum number of event handler functions. This needs to be
 * a compile-time constant as space needs to be allocated for the handler data.
 */
#define EVENT_MAX_HANDLER_FUNCTIONS 20
#endif

/** The typedef for all function event handlers
 *
 * The ID of the resource that triggered the event is passed to the handler along
 * with the user data registered with that resource.
 */
typedef void (*event_handler_t)(resource, void*);

/**
 * The structure to register event function handlers
 */
typedef struct event_handler_function_state_t {
  resource owner;
  event_handler_t handler;
  void *data;
} event_handler_function_state_t;

/** Register an event handler for a resource
 *
 *  Find a free registry entry and register the function and data for the given
 *  resource.
 *
 *  \param r  The resource to de-register
 *  \returns  The index allocated for this resource or -1 if the register is full
 */
int event_register_function(resource r, event_handler_t handler, void *data);

/** Deregister an event handler for a resource
 *
 *  Determine whether there is an event handler function registered for a given
 *  resource. If so, remove the entry. If there is none registered then simply
 *  ignore it.
 *
 *  \param r  The resource to de-register
 */
void event_deregister_function(resource r);

#if defined(__XS2A__)
#define EVENT_ENUM_BASE 0
#else
// On XS1 the environment vectors (EVs) are only 16-bit and bit 16 will be set
// to 1 as it is expected to be used as a memory address.
#define EVENT_ENUM_BASE 0x10000
#endif

/** Enable events on a resource.
 *
 *  This is a shared function to be used by enable_events_chanend(),
 *  enable_events_port() and enable_events_timer().
 *
 *  \param r      The resource to enable events for
 *  \param value The value to be returned by event_select()/event_select_no_wait()
 *               when the timer event is triggered.
 */
inline void event_setup_resource(resource r, unsigned value)
{
  // Set the event vector
  asm volatile("ldap r11, __event_target");
  asm volatile("setv res[%0], r11" :: "r" (r));

  resource_set_ev(r, value);
  event_enable(r);
}

/** Enable events on a resource.
 *
 *  This is a shared function to be used by enable_events_chanend(),
 *  enable_events_port() and enable_events_timer().
 *
 *  \param r      The resource to enable events for
 *  \param value The value to be returned by event_select()/event_select_no_wait()
 *               when the timer event is triggered.
 */
inline void event_setup_resource_function(resource r, event_handler_t handler, void *data)
{
  int value = event_register_function(r, handler, data);
  // Set the event vector
  asm volatile("ldap r11, __event_function_wrapper");
  asm volatile("setv res[%0], r11" :: "r" (r));

  // Don't call resource_set_ev() function as that requires the value to have bit 16 set
  asm volatile("add r11, %0, 0" :: "r" (value));
  asm volatile("setev res[%0], r11" :: "r" (r));
  event_enable(r);
}

/** Basic setup function for timer.
 *
 *  Configure the 
 *  enable_events_port() and enable_events_timer().
 *
 *  \param r      The resource to enable events for
 *  \param value The value to be returned by event_select()/event_select_no_wait()
 *               when the timer event is triggered.
 */
inline void event_setup_timer_common(timer t, int time)
{
  // Set the condition to be AFTER
  asm volatile("setc res[%0], 0x9" :: "r" (t));

  // Set the time at which the event should fire
  asm volatile("setd res[%0], %1" :: "r" (t), "r" (time));
}

#endif // __XC__

#endif // __event_internal_h__
