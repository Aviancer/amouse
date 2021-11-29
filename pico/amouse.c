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

#include "include/serial.h"
#include "../shared/utils.h"
#include "../shared/mouse.h"

#include "bsp/board.h"
#include "tusb.h"

// Technically could support multiple mice connected to the same system if we kept more mouse states in memory.


/*** Program parameters ***/ 

// Struct for storing pointers to dynamically allocated memory containing options.
typedef struct opts {
  int wheel;
} opts_t;

void set_opts(struct opts *options) {
  options->wheel = 1;
}


/*** Global state variables ****/

// Set default options, support mouse wheel.
opts_t options = { .wheel=1 };

extern mouse_state_t mouse; // Needs to be available for serial functions.
mouse_state_t mouse; // int values default to 0 

static uint32_t txtimer_target; // Serial transmit timer target time

// Aggregate movements before sending
CFG_TUSB_MEM_SECTION static hid_mouse_report_t usb_mouse_report;
CFG_TUSB_MEM_SECTION static hid_mouse_report_t usb_mouse_report_prev;

// DEBUG
const uint LED_PIN = PICO_DEFAULT_LED_PIN;
int led_state = 0;


/*** Timing ***/

void queue_tx(mouse_state_t *mouse) {
  // Update timer target for next transmit
  // Use variable send rate depending on whether a 3 or 4 byte update was sent
  if(mouse->update > 2) { txtimer_target = time_us_32() + U_SERIALDELAY_4B; }
  else                  { txtimer_target = time_us_32() + U_SERIALDELAY_3B; }
}


/*** USB comms ***/

void tuh_hid_mouse_mounted_cb(uint8_t dev_addr) {
  // printf("A Mouse device (address %d) is mounted\r\n", dev_addr);
}

void tuh_hid_mouse_unmounted_cb(uint8_t dev_addr) {
  // printf("A Mouse device (address %d) is unmounted\r\n", dev_addr);
}

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
    push_update(mouse, mouse->mmb);

    if(options.wheel && (button_changed_mask & MOUSE_BUTTON_MIDDLE)) {
      mouse->mmb = test_mouse_button(p_report->buttons, MOUSE_BUTTON_MIDDLE);
      push_update(mouse, true);
    }
  }
    
  // ### Handle relative movement ###
  // Clamp to larger than valid protocol output values to allow for sensitivity scaling.
  if(p_report->x) {
    mouse->x += p_report->x;
    mouse->x  = clampi(mouse->x, -36862, 36862); 
    push_update(mouse, mouse->mmb);
  }
  if(p_report->y) {
    mouse->y += p_report->y;
    mouse->y  = clampi(mouse->y, -36862, 36862);
    push_update(mouse, mouse->mmb);
  }
  if(options.wheel && p_report->wheel) {
      mouse->wheel += p_report->wheel;
      mouse->wheel  = clampi(mouse->wheel, -63, 63);
      push_update(mouse, true);
  }

  // Update previous mouse state
  usb_mouse_report_prev = *p_report;
}

// invoked ISR context
void tuh_hid_mouse_isr(uint8_t dev_addr, xfer_result_t event) {
  (void) dev_addr;
  (void) event;
}

// Process USB HID events.
void hid_task(void) {
  uint8_t const addr = 1;

  if(tuh_hid_mouse_is_mounted(addr)) {
    if(!tuh_hid_mouse_is_busy(addr)) {
      tuh_hid_mouse_get_report(addr, &usb_mouse_report);
      process_mouse_report(&mouse, &usb_mouse_report);
    }
  }
}


/*** Main init & loop ***/

int main() {
  // Initialize serial parameters 
  mouse_serial_init(uart0); 

  // Set up initial state 
  //enable_pins(UART_RTS_BIT | UART_DTR_BIT);
  reset_mouse_state(&mouse);
  mouse.pc_state = CTS_UNINIT;
  mouse.sensitivity = 1.0;

  // Initialize USB
  tusb_init();

  // Onboard LED
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  // CTS Pin
  gpio_init(UART_CTS_PIN); 
  gpio_set_dir(UART_CTS_PIN, GPIO_IN);

  // Set initial serial transmit timer target
  txtimer_target = time_us_32() + U_SERIALDELAY_3B; 

  while(1) {
    bool cts_pin = gpio_get(UART_CTS_PIN);

    // ### Check if mouse driver trying to initialize
    if(cts_pin) { // Computers RTS is low, with MAX3232 this shows reversed as high instead? Check spec.
      mouse.pc_state = CTS_LOW_INIT;
      gpio_put(LED_PIN, false);
    }

    // ### Mouse initiaizing request detected
    if(!cts_pin && mouse.pc_state == CTS_LOW_INIT) {
      mouse.pc_state = CTS_TOGGLED;
      mouse_ident(uart0, options.wheel);
    }

    /*** Mouse update loop ***/
    if(mouse.pc_state == CTS_TOGGLED) {
      //led_state ^= 1; // Flip state between 0/1 // DEBUG
      gpio_put(LED_PIN, true);

      tuh_task(); // tinyusb host task
      hid_task(); // hid/mouse handling
 
      runtime_settings(&mouse);

      if(time_reached(txtimer_target) || mouse.force_update) {
	input_sensitivity(&mouse);
	update_mouse_state(&mouse);

	queue_tx(&mouse); // Update next serial timing
        serial_write(uart0, mouse.state, mouse.update);
        reset_mouse_state(&mouse);
      }

    }
    //sleep_us(1);
  }

  return(0);
}
