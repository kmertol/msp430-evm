/* Copyright (c) 2016 Kaan Mertol
 * Licensed under the MIT License. See the accompanying LICENSE file */

#include "include/systimer.h"
#include "include/event.h"
#include "include/debug.h"
#include <msp430.h>

volatile u16 sys_tick = 0;
volatile u16 next_tick = 0;

// We need +1 timer space for the calls to systimer_new inside a timer
// callback and to also ensure thread safety
#define TIMER_MAX_COUNT (SYS_TIMER_MAX_COUNT + 1)
typedef struct timer_instance {
	u16        counter;
	tcb_noid_t call;
	int        id;
} timer_instance_t;

static timer_instance_t timer[TIMER_MAX_COUNT] = {{0}};

// This is for thread safety, -1 means unlocked, positive value means the
// correspending timer is locked for update
static volatile int timer_lock = -1;

// Called when adding a timer fails because all instances are occupied
static void default_fail_callback (void) {}
static pfn_t fail_callback = default_fail_callback;

static void systimer_sys_tick(void);

#ifdef SYS_TIMER_STOP_MODE
static inline void timer_start(void) { TA1CTL |= TACLR | MC_1; }
static inline void timer_stop(void) { TA1CTL &= ~(MC0 | MC1); TA1CCTL0 &= ~CCIFG;}
#else
static inline void timer_start(void) {}
static inline void timer_stop(void) {}
#endif

// Don't use with interrupts enabled
static inline void update_next_tick(u16 current_tick)
{
	assert(current_tick != 0);

	if (current_tick < next_tick) {
    	next_tick = current_tick;
    } else if (next_tick == 0) {
    	next_tick = current_tick;
    	timer_start();
    }
}

static inline void critical_update_next_tick(u16 current_tick)
{
	_uninterrupted(update_next_tick(current_tick));
}

void systimer_init(void)
{
	event_register(EVENT_SYS_TICK, systimer_sys_tick);

	TA1CTL = TACLR | TASSEL_1;
	TA1CCTL0 |= CCIE;
	TA1CCR0 = (32 * SYS_TICK_MS) - 1;

	#ifndef SYS_TIMER_STOP_MODE
	TA1CTL |= MC_1;
	#endif
}

void systimer_register_fail_callback(pfn_t callback)
{
	fail_callback = callback ? callback : default_fail_callback;
}

bool _systimer_new(u16 timeout_ms, tcb_noid_t callback, int id)
{
	int i;

	// No timeout, no registry
	if (timeout_ms == 0)
		return True;

	for (i = 0; i < TIMER_MAX_COUNT; i++) {
		timer_lock = i;
		if (0 == timer[i].counter) {
			timer[i].counter = timeout_ms;
			timer[i].call = callback;
			timer[i].id = id;
			timer_lock = -1;
			critical_update_next_tick(timeout_ms + sys_tick);
			return True;
		}
	}

	timer_lock = -1;
	// too many timers registered at once, maybe increase max count
	fail_callback();
	return False;
}

// Assumes interrupts are disabled
bool _systimer_new_isr(u16 timeout_ms, tcb_noid_t callback, int id)
{
	int i;

	if (timeout_ms == 0)
		return True;

	for (i = 0; i < TIMER_MAX_COUNT; i++) {
		if (0 == timer[i].counter && i != timer_lock) {
			timer[i].counter = timeout_ms;
			timer[i].call = callback;
			timer[i].id = id;
			update_next_tick(timeout_ms + sys_tick);
			return True;
		}
	}

	fail_callback();
	return False;
}

bool _systimer_renew(u16 timeout_ms, tcb_noid_t callback, int id)
{
	int i;

	for (i = 0; i < TIMER_MAX_COUNT; i++) {
		// The timer should be running also, not deprecated
		if (callback == timer[i].call && id == timer[i].id
		    && 0 != timer[i].counter) {
			// Since systimer_new does not touch a timer with counter != 0,
			// we are safe here
			timer[i].counter = timeout_ms;
			if (timeout_ms)
				critical_update_next_tick(timeout_ms + sys_tick);
			return True;
		}
	}

	// if not found register new
	if (timeout_ms) {
		return _systimer_new(timeout_ms, callback, id);
	} else {
		return True;
	}
}

static void systimer_update_tick(u16 tick_count)
{
	int i;
	u16 min_tick;
	u16 counter;

	min_tick = UINT16_MAX;
	// to prevent further sys_tick events, and to know
	// if a ISR registers a new timer during update
	next_tick = UINT16_MAX;

	for (i = 0; i < TIMER_MAX_COUNT; i++) {
		timer_lock = i;
		counter = timer[i].counter;
		if (0 != counter) {
			counter -= tick_count;
			if ((s16)counter <= 0) {
				if (-1 == timer[i].id) {
					timer[i].call();
					counter = 0;
				} else {
					u16 latency = -counter;
					counter = ((tcb_id_t)(timer[i].call))(timer[i].id, latency);
				}
			}

			if (counter && counter < min_tick)
				min_tick = counter;

			timer[i].counter = counter;
		}
	}

	timer_lock = -1;

	if (min_tick == UINT16_MAX && next_tick == UINT16_MAX) {
		_uninterrupted(
			if (next_tick == UINT16_MAX) {
				next_tick = 0;
				sys_tick = 0;
				timer_stop();
			}
		);
	} else {
		critical_update_next_tick(min_tick);
	}
}

static void systimer_sys_tick(void)
{
	u16 tick = sys_tick;

	do {
		sys_tick -= tick;
		systimer_update_tick(tick);
		event_clear(EVENT_SYS_TICK);
		tick = next_tick;
	} while (tick && sys_tick >= tick);
}

#pragma vector = TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void)
{
	#ifndef SYS_TIMER_STOP_MODE
	if (next_tick == 0)
		return;
	#endif

	sys_tick += SYS_TICK_MS;
	if (sys_tick >= next_tick)
		event_set_isr(EVENT_SYS_TICK);
}


