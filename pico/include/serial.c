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
#include "pico/multicore.h"

#include "serial.h"
#include "../shared/mouse.h"

// Map for iterating through each bit (index) for pin (value)  
// Should be updated to reflect UART_..._PIN values.
const int UART_BITS2PINS[] = {0,1,3,4,5,6};
const uint UART_BITS2PINS_LENGTH = 6;

#define BAUD_RATE 1200
#define DATA_BITS 7
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE


/*** Serial comms ***/

// Convert fd style number to uart 
uart_inst_t* get_uart(int uart_id) {
  if(uart_id == 0) { return uart0; }
  else if(uart_id == 1) { return uart1; }
  else { return NULL; }
}

void mouse_serial_init(int uart_id) {
  uart_inst_t* uart = get_uart(uart_id);
  if(uart != NULL) {
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
}

int serial_write(int uart_id, uint8_t *buffer, int size) {
  // For now uart is what gets set in Core 1 loop.
  int bytes=0;
  for(; bytes < size; bytes++) {
    // Offload serial write to Core 1
    multicore_fifo_push_blocking(buffer[bytes]);
  }
  return bytes;
}

// Closer to hardware write method, not working yet.
/*int serial_write(int uart_id, uint8_t *buffer, int size) {
  uart_inst_t* uart = get_uart(uart_id);
  for (size_t i = 0; i < size; ++i) {
      while (!uart_is_writable(uart))
          tight_loop_contents();
      uart_get_hw(uart)->dr = *buffer++;
  }
  return size;
}*/

/* Write to serial out with convert terminal characters */
int serial_write_terminal(int uart_id, uint8_t *buffer, int size) { 
  // For now uart is what gets set in Core 1 loop.
  //uart_inst_t* uart = get_uart(uart_id);
  int bytes=0;
  for(int pos=0; pos <= size; pos++) {
    if(buffer[pos] == '\0') { return bytes; }
    // Convert LF to CRLF
    else if(buffer[pos] == '\n') {
      multicore_fifo_push_blocking((uint8_t)'\r');
      bytes++;
    }
    // Offload serial write to Core 1
    multicore_fifo_push_blocking(buffer[pos]);
    bytes++;
  } 
  return bytes;
}

// Non-blocking read
int serial_read(int uart_id, uint8_t *buffer, int size) { 
  uart_inst_t* uart = get_uart(uart_id);
  int bytes=0;
  if(uart != NULL) {
    for(int i=0; i < size; i++) {
      if(uart_is_readable(uart)) {
	buffer[bytes] = uart_getc(uart);
	bytes++;
      }
      else { break; }
    } 
  }
  return bytes;
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

void mouse_ident(int uart_id, bool wheel_enabled) {
  /*** Mouse proto negotiation ***/
 
  sleep_us(14); 

  if(mouse_options.protocol == PROTO_MSWHEEL) {
    serial_write(uart_id, pkt_intellimouse_intro, pkt_intellimouse_intro_len); // Microsoft Intellimouse with wheel.
  }
  else {
    serial_write(
      uart_id,
      mouse_protocol[mouse_options.protocol].serial_ident,
      mouse_protocol[mouse_options.protocol].serial_ident_len
    );
  }

}
