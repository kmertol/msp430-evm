/* Copyright (c) 2016 Kaan Mertol
 * Licensed under the MIT License. See the accompanying LICENSE file */

#ifndef EVENT_H
#define EVENT_H

#include "types.h"
#include <msp430.h>

/* Best to use the native integer type unless you want to have more events */
typedef uint event_reg_t;
#define EVENT_COUNT_MAX (8 * sizeof(event_reg_t))

typedef enum lpm_modes {
	EVENT_LPM0 = LPM0_bits | GIE,
	EVENT_LPM1 = LPM1_bits | GIE,
	EVENT_LPM2 = LPM2_bits | GIE,
	EVENT_LPM3 = LPM3_bits | GIE,
	EVENT_LPM4 = LPM4_bits | GIE
} event_lpm_t;

#include "user_events.h"
/******************************************************
 * This is an example user_events.h, copy-paste-modify
 * - Put the most triggered events at the top, they
 *   will be dispatched faster
 ******************************************************/
#ifndef USER_EVENTS_H
#define USER_EVENTS_H

#define EVENT_COUNT 4
typedef enum user_events {
	EVENT_0_UNUSED = 0,
	EVENT_1_UNUSED,
	EVENT_2_UNUSED,
	EVENT_3_UNUSED
} event_id_t;

#endif
/******************************************************/

/* Pass Null as callback to unregister */
void event_register(event_id_t id, pfn_t event_callback);

/* The last function you should call from main, this will not return */
void event_machine(void);

/* NOTE: _isr functions should be called from the main body of a ISR */

/* The default(starting) lpm is LPM0. This lpm will be used at the
 * next sleep entry, when no events are remaining */
static inline void event_lpm_set(event_lpm_t lpm)
{
	extern volatile uint event_lpm;
	event_lpm = lpm;
}
/* If the cpu is already sleeping it needs to wake up first to change
 * into another lpm. This should be used to force(speed up) the change */
#define event_lpm_set_isr(lpm) do \
	{	event_lpm_set(lpm); \
		__bic_SR_register_on_exit(LPM4_bits); \
	} while (0)

static inline void event_set(event_id_t id)
{
	extern volatile event_reg_t event_list;
	event_list |= (event_reg_t)1 << id;
}

#define event_set_isr(id) do \
	{	event_set(id); \
		__bic_SR_register_on_exit(LPM4_bits); \
	} while (0)

/* No need to call this, the events are automatically cleared after
 * dispatched, but there might be a specific case for it */
static inline void event_clear(event_id_t id)
{
	extern volatile event_reg_t event_list;
	event_list &= ~((event_reg_t)1 << id);
}

#endif /* EVENT_H */
