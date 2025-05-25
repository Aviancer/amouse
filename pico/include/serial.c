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

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "serial.h"
#include "../shared/mouse.h"
#include "include/wrappers.h"

// Map for iterating through each bit (index) for pin (value)  
// Should be updated to reflect UART_..._PIN values.
const int UART_BITS2PINS[] = {0,1,3,4,5,6};
const uint UART_BITS2PINS_LENGTH = 6;

// For queue purposes we need something that can be referred to with pointer
const uint8_t chr_carriage_return = (uint8_t)'\r';

#define BAUD_RATE 1200
#define DATA_BITS 7
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE

// Multi-core serial data queue 
queue_t g_serial_queue;

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
    queue_add_blocking(&g_serial_queue, &buffer[bytes]);
  }
  return bytes;
}

/* Write to serial out with convert terminal characters */
int serial_write_terminal(int uart_id, uint8_t *buffer, int size) { 
  // For now uart is what gets set in Core 1 loop.
  //uart_inst_t* uart = get_uart(uart_id);
  int bytes=0;
  for(int pos=0; pos <= size; pos++) {
    if(buffer[pos] == '\0') { return bytes; }
    // Convert LF to CRLF
    else if(buffer[pos] == '\n') {
      queue_add_blocking(&g_serial_queue, &chr_carriage_return);
      bytes++;
    }
    // Offload serial write to Core 1
    queue_add_blocking(&g_serial_queue, &buffer[pos]);
    bytes++;
  } 
  return bytes;
}

// Wait for any current serial transmission in the queue to be done
// Allows defining max_wait_us for timeout
bool serial_waitfor_tx(uint32_t max_wait_us) {
  bool time_rollover = false;
  static uint32_t time_timeout;
  time_timeout = time_us_32() + max_wait_us; // Can wrap

  // Check if timestamp will roll over before we reach timeout
  if(time_timeout < time_us_32()) {
    time_rollover = true;
  }

  do {
    a_usleep(10);

    if (time_rollover && (time_us_32() > time_timeout)) { continue; }
    else { time_rollover = false; }

    if (time_us_32() > time_timeout) { return false; } // Timed out
  } while(!queue_is_empty(&g_serial_queue));

  return true; // Finished within timeout
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

/* Pop the next entry from the serial data queue */
// Prefer less direct access to queue internals from rest of the program,
// for the moment just a wrap-around but easier to work with as a endpoint for changes later.
void serial_queue_pop(queue_t *queue, uint8_t *buffer) {
  queue_remove_blocking(queue, buffer); // TODO: Test if returns correct data now
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
 
  if(g_mouse_options.protocol == PROTO_MSWHEEL) {
    int bytes=0;
    for(; bytes < g_pkt_intellimouse_intro_len; bytes++) {
      // Interrupt long write if no longer requested to ident.
      if(gpio_get(UART_CTS_PIN)) { break; } 
      queue_add_blocking(&g_serial_queue, &g_pkt_intellimouse_intro[bytes]);
    }
  }
  else {
    serial_write(
      uart_id,
      g_mouse_protocol[g_mouse_options.protocol].serial_ident,
      g_mouse_protocol[g_mouse_options.protocol].serial_ident_len
    );
  }

}
