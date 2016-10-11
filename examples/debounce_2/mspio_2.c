/* Copyright (c) 2016 Kaan Mertol
 * Licensed under the MIT License. See the accompanying LICENSE file */

#include <msp430.h>
#include "evm/include/event.h"
#include "evm/include/systimer.h"
#include "port_map.h"

#define PULLUP     1
#define PULLDOWN   0
#define FLOATING  -1

#define PIN_STATE FLOATING
// Add delay after enabling pullup/pulldown to wait for the voltage to stabilize
#define PULLUP_CHARGE_DELAY    0
#define PULLDOWN_CHARGE_DELAY  0

#define P_IN_BASE   0x00
#define P_OUT_BASE  0x02
#define P_DIR_BASE  0x04
#define P_REN_BASE  0x06
#define P_SEL0_BASE 0x0A
#define P_SEL1_BASE 0x0C
#define P_IES_BASE  0x18
#define P_IE_BASE   0x1A
#define P_IFG_BASE  0x1C

#define PxIN(base)      *((volatile u8*)(base) + P_IN_BASE)
#define PxOUT(base)     *((volatile u8*)(base) + P_OUT_BASE)
#define PxDIR(base)     *((volatile u8*)(base) + P_DIR_BASE)
#define PxREN(base)     *((volatile u8*)(base) + P_REN_BASE)
#define PxSEL0(base)    *((volatile u8*)(base) + P_SEL0_BASE)
#define PxSEL1(base)    *((volatile u8*)(base) + P_SEL1_BASE)
#define PxIES(base)     *((volatile u8*)(base) + P_IES_BASE)
#define PxIE(base)      *((volatile u8*)(base) + P_IE_BASE)
#define PxIFG(base)     *((volatile u8*)(base) + P_IFG_BASE)

#if PIN_STATE == PULLUP
static inline void pin_disable_pullup(volatile u8 *base, u8 pin) { PxREN(base) &= ~pin; }
static inline void pin_disable_pulldown(volatile u8 *base, u8 pin) {}
static inline void pin_disable_pull(volatile u8 *base, u8 pin) { PxREN(base) &= ~pin; }
static inline void pin_set_pull_dir(volatile u8 *base, u8 pin) { PxOUT(base) |= pin; }
static inline void pin_enable_pull(volatile u8 *base, u8 pin)
{
	PxREN(base) |= pin;
	__delay_cycles(PULLUP_CHARGE_DELAY);
}
#elif PIN_STATE == PULLDOWN
static inline void pin_disable_pullup(volatile u8 *base, u8 pin) {}
static inline void pin_disable_pulldown(volatile u8 *base, u8 pin) { PxREN(base) &= ~pin; }
static inline void pin_disable_pull(volatile u8 *base, u8 pin) { PxREN(base) &= ~pin; }
static inline void pin_set_pull_dir(volatile u8 *base, u8 pin) { PxOUT(base) &= ~pin; }
static inline void pin_enable_pull(volatile u8 *base, u8 pin)
{
	PxREN(base) |= pin;
	__delay_cycles(PULLDOWN_CHARGE_DELAY);
}
#elif PIN_STATE == FLOATING
static inline void pin_disable_pullup(volatile u8 *base, u8 pin) {}
static inline void pin_disable_pulldown(volatile u8 *base, u8 pin) {}
static inline void pin_disable_pull(volatile u8 *base, u8 pin) {}
static inline void pin_set_pull_dir(volatile u8 *base, u8 pin) {}
static inline void pin_enable_pull(volatile u8 *base, u8 pin) {}
#endif

static void mspio_on_change(uint id, uint state);
static void mspio_on_longpress(uint id, uint state);

/* port_base: base pointer to access the port, defaults to PxIN
 * pin: corresponding input pin as bitflag
 * debounce (in ms): should never be 0
 * longpress (in ms): set to 0 if you are not using
 */
typedef struct mspio_input_table {
	volatile u8 *port_base;
	u8           pin;
	u16          debounce;
	u16          longpress;
} mspio_input_table_t;

#define NUM_INPUT_PORTS 2

static const mspio_input_table_t input_table[NUM_INPUT_PORTS] = {
	{&PORT_INPUT_0(IN), PIN_INPUT_0, 100, 3000},
	{&PORT_INPUT_1(IN), PIN_INPUT_1, 100, 0}
};

static u8 input_states[NUM_INPUT_PORTS];

/****************************************************************************/

static u16 on_longpress(int id, u16 latency)
{
	mspio_on_longpress(id, input_states[id]);
	return 0;
}

static u16 on_debounce_end(int id, u16 latency)
{
	const mspio_input_table_t *current = &input_table[id];
	volatile u8 *base = current->port_base;
	u8 pin = current->pin;
	u8 state;

	pin_enable_pull(base, pin);

	if (PxIN(base) & pin) {
		state = 1;
		PxIES(base) |= pin;
	} else {
		state = 0;
		PxIES(base) &= ~pin;
	}

	PxIFG(base) &= ~pin;
	// If state has changed during edge transition, leave it for the ISR to decide
	if ((PxIN(base) & pin ? 1 : 0) != state) {
		PxIFG(base) |= pin;
		PxIE(base) |= pin;
		return 0;
	}
	// To decrease current consumption disable the pullup/pulldown register if the
	// input is pulling the reverse way
	if (state)
		pin_disable_pulldown(base, pin);
	else
		pin_disable_pullup(base, pin);

	// Only register change if it is different than the previous one
	if (input_states[id] != state) {
		input_states[id] = state;
		mspio_on_change(id, state);
		// The longpress works for both states: 0 and 1
		if (current->longpress)
			systimer_renew_task(current->longpress, on_longpress, id);
	}

	PxIE(base) |= pin;
	return 0;
}

static void mspio_init(void)
{
	uint i;

	for (i = 0; i < NUM_INPUT_PORTS; i++) {
		// Set defined port pins as inputs
		PxDIR(input_table[i].port_base) &= ~input_table[i].pin;
		// Select either pullup or pulldown or leave at floating
		pin_set_pull_dir(input_table[i].port_base, input_table[i].pin);
		// I don't think it is that necessary to enable pullup/pulldown here
		#if 0
		pin_enable_pull(input_table[i].port_base, input_table[i].pin);
		#endif
		// Sample input for the initial state
		input_states[i] = PxIN(input_table[i].port_base) & input_table[i].pin ? 1 : 0;
		// Debounce 1ms for the initial state, and let the other initializations be
		// done automatically after the end of debounce
		systimer_new_task(1, on_debounce_end, i);
	}
}

// These are your callbacks, put a breakpoint to test
void on_input_0_change(uint state)
{
	_nop();
}

void on_input_0_longpress(uint state)
{
	_nop();
}

void on_input_1_change(uint state)
{
	_nop();
}

static void mspio_on_change(uint id, uint state)
{
	switch (id) {
	case 0:
		on_input_0_change(state);
		break;
	case 1:
		on_input_1_change(state);
		break;
	default:
		break;
	}
}

static void mspio_on_longpress(uint id, uint state)
{
	switch (id) {
	case 0:
		on_input_0_longpress(state);
		break;
	default:
		break;
	}
}

void main(void)
{
	WDTCTL = WDTPW | WDTHOLD;

	systimer_init();
	mspio_init();
	event_lpm_set(EVENT_LPM4);

	event_machine();
}

/* During the debounce period:
 * - Disable pullup/pulldown to decrease current consumption
 * - Disable individual interrupt enable bit to prevent triggering
 *   of further interrupts
 */
static void enter_debounce_isr(uint id)
{
	const mspio_input_table_t *current = &input_table[id];

	pin_disable_pull(current->port_base, current->pin);
	PxIE(current->port_base) &= ~current->pin;
	systimer_new_task_isr(current->debounce, on_debounce_end, id);
}

#pragma vector = PORT2_VECTOR
__interrupt void PORT2_ISR(void)
{
	int id = -1;

	switch (__even_in_range(P2IV, 0x10)) {
	case 0x00: break;               // No interrupt
	case 0x02: break;               // P2.0 interrupt
	case 0x04: break;               // P2.1 interrupt
	case 0x06: break;               // P2.2 interrupt
	case 0x08: break;               // P2.3 interrupt
	case 0x0A:                      // P2.4 interrupt
		id = 0;
		break;
	case 0x0C:                      // P2.5 interrupt
		id = 1;
		break;
	case 0x0E: break;               // P2.6 interrupt
	case 0x10: break;               // P2.7 interrupt
	default:
		__never_executed();
	}

	if (id != -1)
		enter_debounce_isr(id);
}

