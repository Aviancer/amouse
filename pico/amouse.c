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

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "pico/stdlib.h"

#include "include/utils.h"
#include "include/serial.h"

#include "bsp/board.h"
#include "tusb.h"

// Technically could support multiple mice connected to the same system if we kept more mouse states in memory.


/*** Program parameters ***/ 

// Struct for storing pointers to dynamically allocated memory containing options.
typedef struct opts {
  int wheel;
} opts_t;

// Struct for storing information about accumulated mouse state
typedef struct mouse_state {
  int pc_state; // Current state of mouse driver initialization on PC.
  uint8_t state[4]; // Mouse state
  int x, y, wheel;
  int update; // How many bytes to send
  int lmb, rmb, mmb, force_update;
} mouse_state_t;

// States of mouse init request from PC
enum PC_INIT_STATES {
  CTS_UNINIT   = 0, // Initial state
  CTS_LOW_INIT = 1, // CTS pin has been set low, wait for high.
  CTS_TOGGLED  = 2  // CTS was low, now high -> do ident.
};

void set_opts(struct opts *options) {
  options->wheel = 1;
}


/*** Global state variables ****/

// Set default options, support mouse wheel.
opts_t options = { .wheel=1 };

static uint8_t init_mouse_state[] = "\x40\x00\x00\x00"; // Our basic mouse packet (We send 3 or 4 bytes of it)

extern mouse_state_t mouse; // Needs to be available for serial functions.
mouse_state_t mouse; // int values default to 0 

static uint32_t txtimer_target; // Serial transmit timer target time

// Aggregate movements before sending
CFG_TUSB_MEM_SECTION static hid_mouse_report_t usb_mouse_report;
CFG_TUSB_MEM_SECTION static hid_mouse_report_t usb_mouse_report_prev;

// DEBUG
const uint LED_PIN = PICO_DEFAULT_LED_PIN;
int led_state = 0;


/*** Flow control functions ***/

// Make sure we don't clobber higher update requests with lower ones.
int push_update(mouse_state_t *mouse, int full_packet) {
  if(full_packet || (mouse->update == 3)) { mouse->update = 3; }
  else { mouse->update = 2; }
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
  if(p_report->x) {
    mouse->x += p_report->x;
    mouse->x = clamp(mouse->x, -127, 127);
    push_update(mouse, mouse->mmb);
  }
  if(p_report->y) {
    mouse->y += p_report->y;
    mouse->y = clamp(mouse->y, -127, 127);
    push_update(mouse, mouse->mmb);
  }
  if(options.wheel && p_report->wheel) {
      mouse->wheel += p_report->wheel;
      mouse->wheel  = clamp(mouse->wheel, -15, 15);
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

void reset_mouse_state(mouse_state_t *mouse) {
  memcpy( mouse->state, init_mouse_state, sizeof(mouse->state) ); // Set packet memory to initial state
  mouse->update = -1;
  mouse->force_update = 0;
  mouse->x = mouse->y = mouse->wheel = 0;
  // Do not reset button states here, will be updated on release of buttons.
}

/*** Mainline mouse state logic ***/

bool serial_tx(mouse_state_t *mouse) {
  if((mouse->update < 2) && (mouse->force_update == false)) { return(false); } // Minimum report size is 2 (3 bytes)
  int movement;

  // Set mouse button states	
  mouse->state[0] |= (mouse->lmb << MOUSE_LMB_BIT);
  mouse->state[0] |= (mouse->rmb << MOUSE_RMB_BIT);
  mouse->state[3] |= (mouse->mmb << MOUSE_MMB_BIT);

  // Update aggregated mouse movement state
  movement = mouse->x & 0xc0; // Get 2 upper bits of X movement
  mouse->state[0] = mouse->state[0] | (movement >> 6); // Sets bit based on ev.value, 8th bit to 2nd bit (Discards bits)
  mouse->state[1] = mouse->state[1] | (mouse->x & 0x3f); 

  movement = mouse->y & 0xc0; // Get 2 upper bits of Y movement
  mouse->state[0] = mouse->state[0] | (movement >> 4);
  mouse->state[2] = mouse->state[2] | (mouse->y & 0x3f); 

  mouse->state[3] = mouse->state[3] | (-mouse->wheel & 0x0f); // 127(negatives) when scrolling up, 1(positives) when scrolling down.

  serial_write(uart0, mouse->state, mouse->update);
  reset_mouse_state(mouse);

  // Update timer target for next transmit
  // Use variable send rate depending on whether a 3 or 4 byte update was sent
  if(mouse->update > 2) { txtimer_target = time_us_32() + SERIALDELAY_4B; }
  else                  { txtimer_target = time_us_32() + SERIALDELAY_3B; }
}

/*void gpio_callback(uint gpio, uint32_t events) {
  //gpio_event_string(event_str, events);
  //printf("GPIO %d %s\n", gpio, event_str);
  if(events & 0x08) {
    led_state ^= 1; // Flip state between 0/1
    gpio_put(LED_PIN, led_state);
    mouse_ident(uart0, options.wheel);
  }
}*/


/*** Main init & loop ***/

int main() {
  // Initialize serial parameters 
  mouse_serial_init(uart0); 

  // Set up initial state 
  //enable_pins(UART_RTS_BIT | UART_DTR_BIT);
  reset_mouse_state(&mouse);

  // Initialize USB
  tusb_init();

  // Onboard LED
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  // CTS Pin
  gpio_init(UART_CTS_PIN); 
  gpio_set_dir(UART_CTS_PIN, GPIO_IN);

  // Button
  //gpio_init(3); // DEBUG
  //gpio_set_dir(3, GPIO_IN);
  //gpio_pull_down(3);
  //gpio_set_irq_enabled_with_callback(3, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

  // Set initial serial transmit timer target
  txtimer_target = time_us_32() + SERIALDELAY_3B; 

  mouse.pc_state = CTS_UNINIT;

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

      if(time_reached(txtimer_target) || mouse.force_update) {
	serial_tx(&mouse);
      }

    }
    //sleep_us(1);
  }

  return(0);
}
