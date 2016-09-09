/* Copyright (c) 2016 Kaan Mertol
 * Licensed under the MIT License. See the accompanying LICENSE file */

#include <msp430.h>
#include "evm/include/event.h"
#include "evm/include/systimer.h"

uint sec_tick = 0;

u16 feed_the_dog(int id, u16 latency)
{
	WDTCTL = WDT_ARST_250;
	return 200;
}

u16 one_sec_tick(int id, u16 latency)
{
	if (++sec_tick == 10) {
		WDTCTL = WDT_ARST_250;
		systimer_new_task(200, feed_the_dog, 0);
		return 0;
	}

	return SYS_TIME_OFFSET_LATENCY(SYS_TIME_SEC(1), latency);
}

void no_food_for_dog(void)
{
	systimer_delete_task(feed_the_dog, 0);
}

void boot_delay(void)
{
	systimer_new_task(SYS_TIME_SEC(1), one_sec_tick, 0);
}

void main(void)
{
	WDTCTL = WDTPW | WDTHOLD;

	systimer_init();
	systimer_new(5000, boot_delay);
	// Or better use SYS_TIME_SEC for the exact seconds
	systimer_new(SYS_TIME_SEC(30), no_food_for_dog);

	event_machine();
}
