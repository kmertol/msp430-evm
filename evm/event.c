/* Copyright (c) 2016 Kaan Mertol
 * Licensed under the MIT License. See the accompanying LICENSE file */

#include "include/event.h"
#include "include/debug.h"

#define EVENTS_USED_BITMASK	(((((event_reg_t)1 << (EVENT_COUNT - 1)) - 1) << 1) + 1)

volatile uint event_lpm = EVENT_LPM0;
volatile event_reg_t event_list = 0;
static pfn_t event_handlers[EVENT_COUNT] = {0};

/* Assuming most events that are defined are registered, it is faster to call
 * the function pointers directly rather than controlling everytime if is not
 * equal to Null */
static void no_handler(void) {}
static void init_empty_handlers(void)
{
	uint i;

	for (i = 0; i < EVENT_COUNT; i++) {
		if (Null == event_handlers[i])
			event_handlers[i] = no_handler;
	}
}

static inline void disable_interrupt(void) { __disable_interrupt(); }
static inline void enable_interrupt(void) { __enable_interrupt(); }
static inline void before_sleep(void) {}
static inline void after_sleep(void) {}
static inline void enter_sleep(void) { __bis_SR_register(event_lpm); }

static void _event_machine(void)
{
	event_reg_t current;
	event_reg_t bit;
	uint i;

	while (1) {
		do {
			for (i = 0, bit = 1, current = event_list; i < EVENT_COUNT; i++) {
				if (current & bit) {
					event_list &= ~bit;
					event_handlers[i]();
					current = event_list;
					if (!current)
						goto sleep;
				}
				bit <<= 1;
			}
			// Handle the below case in development
			assert(0 == (event_list & ~EVENTS_USED_BITMASK));
			// Erronuous event bit setting can cause the loop to got stuck
			event_list &= EVENTS_USED_BITMASK;
		} while (event_list);

		sleep:
		disable_interrupt();
		if (event_list) {
			enable_interrupt();
		} else {
			before_sleep();
			enter_sleep();
			after_sleep();
		}
	}
}

void event_machine(void)
{
	init_empty_handlers();
	_event_machine();
}

void event_register(event_id_t id, pfn_t handler)
{
	assert(id < EVENT_COUNT);
	event_handlers[id] = (Null == handler) ? no_handler : handler;
}
