/* Copyright (c) 2016 Kaan Mertol
 * Licensed under the MIT License. See the accompanying LICENSE file */

/** @file port_map.h
 *  Includes port and pin mappings of connections, also include msp430.h
 *  Example usage:
 *      PORT_BACKLIGHT(OUT) &= ~PIN_BACKLIGHT
 *      PORT_BACKLIGHT(DIR) |= PIN_BACKLIGHT
 *                instead of
 *      P3OUT &= ~BIT3
 *      P3DIR |= BIT3
 */

#ifndef PORT_MAP_H
#define PORT_MAP_H

/* Inputs */
#define PORT_INPUT_0(reg)           P2##reg
#define PIN_INPUT_0                 BIT4
#define PORT_INPUT_1(reg)           P2##reg
#define PIN_INPUT_1                 BIT5

#endif /*PORT_MAP_H*/
