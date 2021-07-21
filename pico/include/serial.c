/*
 * Anachro Mouse, a usb to serial mouse adaptor. Copyright (C) 2021 Aviancer <oss+amouse@skyvian.me>
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

#include "pico/stdlib.h"

#include "serial.h"

// Map for iterating through each bit (index) for pin (value)  
// Should be updated to reflect UART_..._PIN values.
const int UART_BITS2PINS[] = {0,1,3,4,5,6};
const uint UART_BITS2PINS_LENGTH = 6;

uint8_t pkt_intellimouse_intro[] = {0x4D,0x5A};

#define BAUD_RATE 1200
#define DATA_BITS 7
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE

/*** Serial comms ***/

void mouse_serial_init(uart_inst_t* uart) {
    // Set baud for serial device 
    uart_init(uart, BAUD_RATE);

    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    // Set UART flow control CTS/RTS off 
    uart_set_hw_flow(uart, false, false);
    // Turn off crlf conversion, we want raw output
    uart_set_translate_crlf(uart, false);
    // 7n1
    uart_set_format(uart, DATA_BITS, STOP_BITS, PARITY);

    // Having the FIFOs on causes lag with 4 byte packets, this ensures better flow.
    uart_set_fifo_enabled(uart, false);
}

// TODO: Size does not always match if mixing null terminated and not packets.
int serial_write(uart_inst_t* uart, uint8_t *buffer, int size) { 
  int written=0;
  for(int i=0; i <= size; i++) {
    uart_putc_raw(uart, buffer[i]); 
    written++;
  } 
  return written;
}

int get_pins(int flag) {
  int serial_state = 0;
/*  serial_state |= (gpio_get(UART_TX_PIN)  << UART_TX_BIT);
  serial_state |= (gpio_get(UART_RX_PIN)  << UART_RX_BIT);
  serial_state |= (gpio_get(UART_RTS_PIN) << UART_RTS_BIT);
  serial_state |= (gpio_get(UART_DTR_PIN) << UART_DTR_BIT); */
  serial_state |= (gpio_get(UART_CTS_PIN) << UART_CTS_BIT);

  return (serial_state & flag) ? 1 : 0; // check bits set?
}

// Takes bitmask, pulls high relevant pins based on array map.
void enable_pins(int flag) {
  for (int i = 0; i < UART_BITS2PINS_LENGTH; i++) {
    if(flag & (1 << i)) { gpio_pull_up(UART_BITS2PINS[i]); }
  }
}

// Takes bitmask, pulls low relevant pins based on array map.
void disable_pins(int flag) {
  for (int i = 0; i < UART_BITS2PINS_LENGTH; i++) {
    if(flag & (1 << i)) { gpio_pull_down(UART_BITS2PINS[i]); }
  }
}

void wait_pin_state(int flag, int desired_state) {
  // Monitor
  int pin_state = -1;
  while(pin_state != desired_state) { 
    pin_state = get_pins(flag);
    sleep_us(1);
  }
}

void mouse_ident(uart_inst_t* uart, int wheel_enabled) {
  /*** Microsoft Mouse proto negotiation ***/
 
  sleep_us(14); 

  /* Byte1:Always M
   * Byte2:[None]=Microsoft 3=Logitech Z=MicrosoftWheel  */
  //uint8_t logitech[] = "\x4D\x33";
  //uint8_t microsoft[] = "\x4D";
  /* IntelliMouse: MZ@... */
  //uint8_t pkt_intellimouse_intro[] = "\x4D\x5A"; // MZ

  if(wheel_enabled) {
    serial_write(uart, pkt_intellimouse_intro, 2); // 2 byte intro is sufficient
  }
  else {
    serial_write(uart, pkt_intellimouse_intro, 1); // M for basic Microsoft proto. 
  }

  // sleep_us(63); // Simulate mouse init delay
  //write(fd, &logitech[1], 1); // Not enabled currently
}
