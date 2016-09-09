/* Copyright (c) 2016 Kaan Mertol
 * Licensed under the MIT License. See the accompanying LICENSE file */

#include <msp430.h>
#include "evm/include/event.h"
#include "evm/include/systimer.h"
#include "port_map.h"

// DCO_FREQ is hardcoded, don't change
#define DCO_FREQ                20971520
#define BAUD_RATE               115200
#define RECEPTION_TIMEOUT_MS    50

// Only use multiples of 2, we will use &2^n-1 for indexing
#define RX_BUFFER_SIZE              16
#define TX_BUFFER_SIZE              64

#define RX_BUFFER_SIZE_MASK         (RX_BUFFER_SIZE-1)
#define TX_BUFFER_SIZE_MASK         (TX_BUFFER_SIZE-1)

static u8 rx_buf[RX_BUFFER_SIZE];
static u8 tx_buf[TX_BUFFER_SIZE];

static volatile struct queue{
	u8 read_index;
	u8 write_index;
	u8 count;
} rx_queue, tx_queue;

static void tx_start(void)
{
	if (!(UCA2IE & UCTXIE)) {
		UCA2IFG |= UCTXIFG;
		UCA2IE |= UCTXIE;
	}
}

static void clear_rx_queue(void)
{
	rx_queue.write_index = 0;
	rx_queue.read_index = 0;
	rx_queue.count = 0;
}

static void clear_tx_queue(void)
{
	tx_queue.write_index = 0;
	tx_queue.read_index = 0;
	tx_queue.count = 0;
}

void serial_buffer_char(char ch)
{
	tx_buf[tx_queue.write_index] = ch;
	++tx_queue.write_index;
	tx_queue.write_index &= TX_BUFFER_SIZE_MASK;
	++tx_queue.count;
}

void serial_buffer_data(const void *data, u8 length)
{
	const u8 *bytes = data;

	while (length--) {
		serial_buffer_char(*bytes++);
	}
}

void serial_send_data(const void *data, u8 length)
{
	serial_buffer_data(data, length);
	tx_start();
}

int serial_read(void)
{
	int received;

	if (0 == rx_queue.count) {
		return -1;
	} else {
		received = rx_buf[rx_queue.read_index];
		++rx_queue.read_index;
		rx_queue.read_index &= RX_BUFFER_SIZE_MASK;
		--rx_queue.count;
		return received;
	}
}

void on_tx_end(void)
{
	_nop();
}

u8 buf[64];
uint index = 0;

void reception_timeout(void)
{
	index = 0;
}

void receiver(void)
{
	int rx;

	while (-1 != (rx = serial_read())) {
		buf[index++] = rx;
		if (rx == '\n') {
			serial_send_data(buf, index);
			index = 0;
			systimer_delete(reception_timeout);
		} else {
			if (index == 1) {
				/* If it is the first char, start reception timeout.
				 * Take notice that we are using renew instead of new, because
				 * the timer may have already been running. So we guarantee
				 * by using renew that there will be only one timer instance */
				systimer_renew(RECEPTION_TIMEOUT_MS, reception_timeout);
			}
			if (index >= 64) {
				index = 0;
			}
		}
	}
}

void init_clocks(void)
{
	uint timeout = 1000;

	UCSCTL6 &= ~(XT1OFF);

	do
	{
		UCSCTL7 &= ~(XT2OFFG | XT1LFOFFG | DCOFFG);
		SFRIFG1 &= ~OFIFG;
	} while ((SFRIFG1 & OFIFG) && --timeout);

	PMMCTL0 = PMMPW | PMMCOREV_3;

	// For clock 20971520 D*(N+1)*Aclk, D = 32, N = 19
	UCSCTL1 = DCORSEL2 | DCORSEL1;
	UCSCTL2 = 19 | FLLD0 | FLLD2;
	UCSCTL4 = SELA__XT1CLK | SELS__DCOCLK | SELM__DCOCLK;

	return;
}

#define val_UCBRX(clock, baud) (clock/baud/16)
#define val_UCBRFX(clock, baud) ((uint)(((double)clock/baud/16 - clock/baud/16) * 16 + 0.5))

void serial_init(void)
{
	UCA2CTLW0 |= UCSWRST;

	PORT_UART(SEL) |= PIN_UART_TX | PIN_UART_RX;

	UCA2BRW = val_UCBRX(DCO_FREQ, BAUD_RATE);
	UCA2MCTLW = val_UCBRFX(DCO_FREQ, BAUD_RATE) << 4 | UCOS16;

	// clock sourse SMCLK, no parity, 8 bits, 1 stop bit
	UCA2CTLW0 = UCSSEL__SMCLK;
	UCA2STATW = 0;

	clear_rx_queue();
	clear_tx_queue();

	event_register(EVENT_UART_RX, receiver);
	event_register(EVENT_UART_TX_END, on_tx_end);

	UCA2CTLW0 &= ~UCSWRST;
	UCA2IFG &= ~UCTXIFG;
	UCA2IE |= UCRXIE;
}

void main(void)
{
	WDTCTL = WDTPW | WDTHOLD;

	init_clocks();
	systimer_init();
	serial_init();

	event_machine();
}

#pragma vector = USCI_A2_VECTOR
__interrupt void USCI_A2_ISR(void)
{
	switch ( __even_in_range(UCA2IV, 4)) {
		case 0: break;
		case 2:
			rx_buf[rx_queue.write_index] = UCA2RXBUF;
			++rx_queue.write_index;
			rx_queue.write_index &= RX_BUFFER_SIZE_MASK;
			++rx_queue.count;
			event_set_isr(EVENT_UART_RX);
			break;

		case 4:
			if (0 == tx_queue.count) {
				UCA2IE &= ~UCTXIE;
				event_set_isr(EVENT_UART_TX_END);
				return;
			}
			UCA2TXBUF = tx_buf[tx_queue.read_index];
			++tx_queue.read_index;
			tx_queue.read_index &= TX_BUFFER_SIZE_MASK;
			--tx_queue.count;
			break;
		default:
			_never_executed( );
	}

}
