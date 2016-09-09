/* Copyright (c) 2016 Kaan Mertol
 * Licensed under the MIT License. See the accompanying LICENSE file */

/** @file port_map.h
 *  Includes port and pin mappings of connections, also include msp430.h
 *	Example usage:
 *	    PORT_BACKLIGHT(OUT) &= ~PIN_BACKLIGHT
 *		PORT_BACKLIGHT(DIR) |= PIN_BACKLIGHT
 *                instead of
 *      P3OUT &= ~BIT3
 *      P3DIR |= BIT3
 */

#ifndef PORT_MAP_H_
#define PORT_MAP_H_


/* UART (on UCA0)*/
#define PORT_UART(reg)				P2##reg
#define PORT_UART_RX(reg)			P2##reg
#define PORT_UART_TX(reg)			P2##reg
#define PIN_UART_RX					BIT2
#define PIN_UART_TX					BIT3


#endif /*PORT_MAP_H_*/
