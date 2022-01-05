/*
 *   __ _   _ __  ___ _  _ ___ ___ 
 *  / _` | | '  \/ _ \ || (_-</ -_)
 *  \__,_| |_|_|_\___/\_,_/__/\___=====_____)
 *
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

#include <time.h>
#include "stdbool.h"
#include "pico/stdlib.h"

#include "include/version.h"
#include "include/serial.h"
#include "include/usb.h"
#include "../shared/utils.h"
#include "../shared/mouse.h"

#include "bsp/board.h"
#include "tusb.h"

// Technically could support multiple mice connected to the same system if we kept more mouse states in memory.


/*** Global state variables ****/

extern mouse_state_t mouse; // Needs to be available for serial functions.
mouse_state_t mouse;        // int values default to 0 

static uint32_t time_tx_target; // Serial transmit timers target time
static uint32_t time_rx_target; // Serial receive timers target time

uint8_t serial_buffer[2] = {0}; // Buffer for inputs from serial port.

// Aggregate movements before sending
CFG_TUSB_MEM_SECTION static hid_mouse_report_t usb_mouse_report_prev;

const uint LED_PIN = PICO_DEFAULT_LED_PIN;
bool led_state = false;


/*** Timing ***/

void queue_tx(mouse_state_t *mouse) {
  // Update timer target for next transmit
  // Use different send rate depending on protocol used (3 or 4 bytes)
  if(mouse_options.wheel) { time_tx_target = time_us_32() + U_SERIALDELAY_4B; }
  else                    { time_tx_target = time_us_32() + U_SERIALDELAY_3B; }
}


/*** Mouse specific USB handling ***/

int test_mouse_button(uint8_t buttons_state, uint8_t button) {
  if(buttons_state & button) { return 1; }
  return 0;
}

static inline void process_mouse_report(mouse_state_t *mouse, hid_mouse_report_t const *p_report) {
 
  uint8_t button_changed_mask = p_report->buttons ^usb_mouse_report_prev.buttons; // xor to set bits true if any state is different.
  //if(button_changed_mask & p_report->buttons) { // Could be used to act only on any button down press.
  if(button_changed_mask) { // If button pressed or released
    mouse->force_update = 1;

    mouse->lmb = test_mouse_button(p_report->buttons, MOUSE_BUTTON_LEFT);
    mouse->rmb = test_mouse_button(p_report->buttons, MOUSE_BUTTON_RIGHT);

    if((button_changed_mask & MOUSE_BUTTON_MIDDLE)) {
      mouse->mmb = test_mouse_button(p_report->buttons, MOUSE_BUTTON_MIDDLE);
    }
  }
    
  // ### Handle relative movement ###
  // Clamp to larger than valid protocol output values to allow for sensitivity scaling.
  if(p_report->x) {
    mouse->x += p_report->x;
    mouse->x  = clampi(mouse->x, -36862, 36862); 
  }
  if(p_report->y) {
    mouse->y += p_report->y;
    mouse->y  = clampi(mouse->y, -36862, 36862);
  }
  if(p_report->wheel) {
      mouse->wheel += p_report->wheel;
      mouse->wheel  = clampi(mouse->wheel, -63, 63);
  }

  push_update(mouse, mouse_options.wheel); // TODO

  // Update previous mouse state
  usb_mouse_report_prev = *p_report;
}

// External interface for delivering mouse reports to process_mouse_report()
// Allows keeping static context within amouse.c while tinyusb handling can be shifted to usb.c
extern void collect_mouse_report(hid_mouse_report_t* p_report) {
  process_mouse_report(&mouse, p_report); // Passes full context with mouse and report without having to make them external/non-static.
}


/*** Main init & loop ***/

int main() {
  // Initialize serial parameters 
  mouse_serial_init(0); // uart0

  // Set up initial state 
  //enable_pins(UART_RTS_BIT | UART_DTR_BIT);
  reset_mouse_state(&mouse);
  mouse.pc_state = CTS_UNINIT;

  // Set default options, support mouse wheel.
  mouse_options.wheel=1;
  mouse_options.sensitivity=1.0;

  // Initialize USB
  tusb_init();

  // Onboard LED
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  // CTS Pin
  gpio_init(UART_CTS_PIN); 
  gpio_set_dir(UART_CTS_PIN, GPIO_IN);

  // Set initial serial timer targets
  time_tx_target = time_us_32() + U_SERIALDELAY_3B; 
  time_rx_target = time_us_32() + U_FULL_SECOND; 

  while(1) {

    // Check for request for serial console  
    // Repeating non-blocking reads is slow so instead we queue checks every now and then with timer.
    if(time_reached(time_rx_target)) {
      if(serial_read(0, serial_buffer, 1) > 0) {
        if(serial_buffer[0] == '\r' || serial_buffer[0] == '\n') {
          console(0);
        }
      }
      time_rx_target = time_us_32() + U_FULL_SECOND;
    }

    bool cts_pin = gpio_get(UART_CTS_PIN);

    // ### Check if mouse driver trying to initialize
    if(cts_pin) { // Computers RTS is low, with MAX3232 this shows reversed as high instead? Check spec.
      if(mouse.pc_state == CTS_UNINIT) { mouse.pc_state = CTS_LOW_INIT; }
      else if(mouse.pc_state == CTS_TOGGLED) { mouse.pc_state = CTS_LOW_RUN; }
    }

    // ### Mouse initializing request detected
    if(!cts_pin && (mouse.pc_state != CTS_UNINIT && mouse.pc_state != CTS_TOGGLED)) {
      mouse.pc_state = CTS_TOGGLED;
      mouse_ident(0, mouse_options.wheel);
      gpio_put(LED_PIN, false);
      led_state = false;
    }

    /*** Mouse update loop ***/

    // Transmit only once we are initialized at least once. Unlike in DOS, Windows drivers will set CTS pin 
    // low after init which would inhibit transmitting. We will trust the driver to re-init if needed.
    if(mouse.pc_state > CTS_LOW_INIT) {
      if(!led_state) {
        gpio_put(LED_PIN, true);
	led_state = true;
      }
 
      if(time_reached(time_tx_target) || mouse.force_update) {
        runtime_settings(&mouse);
	input_sensitivity(&mouse);
	update_mouse_state(&mouse);

	queue_tx(&mouse); // Update next serial timing
        serial_write(0, mouse.state, mouse.update);
        reset_mouse_state(&mouse);
      }

      tuh_task(); // tinyusb host task // TODO: Test shifted from above to here.

    }
    //sleep_us(1);
  }

  return(0);
}
