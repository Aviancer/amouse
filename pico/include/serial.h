/*
 * Anachro Mouse, a usb to serial mouse adaptor. Copyright (C) 2021-2025 Aviancer <oss+amouse@skyvian.me>
 *
 * This library is free software; you can redistribute it and/or modify it under the terms of the 
 * GNU Lesser General Public License as published by the Free Software Foundation; either version 
 * 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without 
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with this library; 
 * if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
*/

#ifndef SERIAL_H_
#define SERIAL_H_

#include "pico/util/queue.h"

// Which pin has which function
// Serial spec (Fem): TX(2), RX(3), DSR(4), DTR(6), CTS(7), RTS(8)
//                    GRN    YLW    ORN     BLU     WHI     BLK
// Pins are zero indexed
enum UART_PINS { 
  UART_TX_PIN  = 0,
  UART_RX_PIN  = 1,
  UART_CTS_PIN = 2,
//UART_GND_PIN 
  UART_DSR_PIN = 3,
  UART_DTR_PIN = 4,
  UART_RTS_PIN = 6
};

enum UART_BITS {
  UART_TX_BIT  = 0,
  UART_RX_BIT  = 1,
//UART_GND_PIN 
  UART_DSR_BIT = 3,
  UART_DTR_BIT = 4,
  UART_CTS_BIT = 5,
  UART_RTS_BIT = 6
};

extern queue_t serial_queue; // Global serial data queue

uart_inst_t* get_uart(int uart_id);

void mouse_serial_init(int uart_id);

int serial_write(int uart_id, uint8_t *buffer, int size);

int serial_write_terminal(int uart_id, uint8_t *buffer, int size);

int serial_read(int uart_id, uint8_t *buffer, int size);

uint8_t serial_queue_pop(uint8_t *buffer);

int get_pins(int flag);

void enable_pins(int flag);

void disable_pins(int flag);

void wait_pin_state(int flag, int desired_state);

void mouse_ident(int uart_id, bool wheel_enabled);

#endif // SERIAL_H_
