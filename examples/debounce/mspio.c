/* Copyright (c) 2016 Kaan Mertol
 * Licensed under the MIT License. See the accompanying LICENSE file */

/****************************************************************************
 * This implementation doesn't have pull-ups enabled, change it to use
 * pull-ups as you like
 ****************************************************************************/
#include <msp430.h>
#include "evm/include/event.h"
#include "evm/include/systimer.h"
#include "port_map.h"

#define P_IN_BASE   0x00
#define P_OUT_BASE  0x02
#define P_DIR_BASE  0x04
#define P_SEL0_BASE 0x0A
#define P_SEL1_BASE 0x0C
#define P_IES_BASE  0x18
#define P_IE_BASE   0x1A
#define P_IFG_BASE  0x1C

#define PxIN(base)      (*((volatile u8*)((base) + P_IN_BASE)))
#define PxOUT(base)     (*((volatile u8*)((base) + P_OUT_BASE)))
#define PxDIR(base)     (*((volatile u8*)((base) + P_DIR_BASE)))
#define PxSEL0(base)    (*((volatile u8*)((base) + P_SEL0_BASE)))
#define PxSEL1(base)    (*((volatile u8*)((base) + P_SEL1_BASE)))
#define PxIE(base)      (*((volatile u8*)((base) + P_IE_BASE)))
#define PxIES(base)     (*((volatile u8*)((base) + P_IES_BASE)))
#define PxIFG(base)     (*((volatile u8*)((base) + P_IFG_BASE)))

typedef void (*mspio_callback_t)(uint);

typedef struct mspio_input_table {
	volatile u8      *port_base;
	u8               pin;
	mspio_callback_t on_change;
	u16              debounce;
	const char       *name;
} mspio_input_table_t;

// The current state of the input pin is saved in the below state variable
typedef struct mspio_input {
	const mspio_input_table_t* table;
	u8 state;
} mspio_input_t;

static void on_input0_change(uint state);
static void on_input1_change(uint state);

#define NUM_INPUT_PORTS 2

static const mspio_input_table_t table[NUM_INPUT_PORTS] = {
	{&PORT_INPUT_0(IN), PIN_INPUT_0, on_input0_change, 100, "input_0"},
	{&PORT_INPUT_1(IN), PIN_INPUT_1, on_input1_change, 2000, "input_1"}
};

static mspio_input_t input[NUM_INPUT_PORTS] = {
	{&table[0], 1},
	{&table[1], 1},
};

static void mspio_debounce_end(mspio_input_t *input)
{
	volatile u8 *base = input->table->port_base;
	u8 pin = input->table->pin;

	if (PxIN(base) & pin) {
		PxIES(base) |= pin;
		PxIFG(base) &= ~pin;
		// if state changed during transition, leave it for the ISR to decide
		if (!(PxIN(base) & pin)) {
			PxIFG(base) |= pin;
			PxIE(base) |= pin;
			return;
		}
		// if (1 == waiting_edge)
		if (input->state ^ 1) {
			input->state = 1;
			if (Null != input->table->on_change)
				input->table->on_change(1);
		}
	} else {
		PxIES(base) &= ~pin;
		PxIFG(base) &= ~pin;
		// if state changed during transition, leave it for the ISR to decide
		if (PxIN(base) & pin) {
			PxIFG(base) |= pin;
			PxIE(base) |= pin;
			return;
		}
		// if (0 == waiting_edge)
		if (input->state ^ 0) {
			input->state = 0;
			if (Null != input->table->on_change)
				input->table->on_change(0);
		}
	}

	PxIE(base) |= pin;
}

static u16 on_debounce_end(int id, u16 latency)
{
	mspio_debounce_end(&input[id]);
	return 0;
}

static void mspio_init(void)
{
	uint i;

	// Configure input pins here
	P2DIR = 0xFF & ~PIN_INPUT_0 & ~PIN_INPUT_1;

	for (i = 0; i < NUM_INPUT_PORTS; i++) {
		input[i].state = PxIN(input[i].table->port_base) & input[i].table->pin ? 1 : 0;
		// We wiil debounce just 1ms for the initial state, the other
		// initializations will be done automatically on debounce end
		systimer_new_task(1, on_debounce_end, i);
	}
}

// These are your callbacks, put a breakpoint to test
static void on_input0_change(uint state)
{
	_nop();
}

static void on_input1_change(uint state)
{
	_nop();
}


void main(void)
{
	WDTCTL = WDTPW | WDTHOLD;

	systimer_init();
	mspio_init();
	event_lpm_set(EVENT_LPM4);

	event_machine();
}

#pragma vector = PORT2_VECTOR
__interrupt void PORT2_ISR(void)
{
	int id = -1;

	switch(__even_in_range(P2IV,0x10)){
		case 0x00: break;               // No interrupt
		case 0x02: break;               // P2.0 interrupt
		case 0x04: break;               // P2.1 interrupt
		case 0x06: break;               // P2.2 interrupt
		case 0x08: break;               // P2.3 interrupt
		case 0x0A:                      // P2.4 interrupt
			PORT_INPUT_0(IE) &= ~PIN_INPUT_0;
			id = 0;
			break;
		case 0x0C:                      // P2.5 interrupt
			PORT_INPUT_1(IE) &= ~PIN_INPUT_1;
			id = 1;
			break;
		case 0x0E: break;               // P2.6 interrupt
		case 0x10: break;               // P2.7 interrupt
		default:
			_never_executed( );
	}

	if (id != -1)
		systimer_new_task_isr(input[id].table->debounce, on_debounce_end, id);

}

