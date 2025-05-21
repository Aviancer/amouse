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

/* mouse.c: Architecture independent mouse logic */

#include <stdbool.h>
#include <stdint.h>

#ifdef __linux__
#include <stdlib.h>
//#include "../linux/src/include/version.h"
//#include "../linux/src/include/serial.h"
//#include "../linux/src/include/storage.h"
//#include "../linux/src/include/wrappers.h"
#else 
#include "pico/stdlib.h"
//#include "../pico/include/version.h"
//#include "../pico/include/serial.h"
//#include "../pico/include/storage.h"
//#include "../pico/include/wrappers.h"
#endif 

#include "mouse.h"
//#include "settings.h"
#include "utils.h"

// Define available mouse protocols
mouse_proto_t mouse_protocol[3] =
{
// Name           Intro Len Btn Wheel  ReportLen
  {"MS 2-button", "M",  1,  2,  false, 3}, // MS_2BUTTON = 0
  {"Logitech",    "M3", 2,  3,  false, 3}, // LOGITECH   = 1, report is 3-4
  {"MS wheeled",  "MZ", 2,  3,  true,  4}  // MS_WHEELED = 2
};
uint mouse_protocol_num = sizeof mouse_protocol / sizeof mouse_protocol[0];

// Full Serial Mouse intro with PnP information (Microsoft IntelliMouse)
uint8_t pkt_intellimouse_intro[] = {0x4D,0x5A,0x40,0x00,0x00,0x00,0x08,0x01,0x24,0x2d,0x33,0x28,0x10,0x10,0x10,0x11,
                                    0x3c,0x21,0x36,0x29,0x21,0x2e,0x23,0x25,0x32,0x3c,0x2d,0x2f,0x35,0x33,0x25,0x3c,
                                    0x30,0x2e,0x30,0x10,0x26,0x10,0x21,0x3c,0x2d,0x29,0x23,0x32,0x2f,0x33,0x2f,0x26,
                                    0x34,0x00,0x2d,0x2f,0x35,0x33,0x25,0x00,0x37,0x29,0x34,0x28,0x00,0x37,0x28,0x25,
                                    0x25,0x2c,0x12,0x16,0x09};
int pkt_intellimouse_intro_len = 69;

uint8_t init_mouse_state[] = "\x40\x00\x00\x00"; // Our basic mouse packet (We send 3 or 4 bytes of it)

/*** Global data / BSS (Avoid stack) ***/ 

mouse_opts_t mouse_options; // Global user settable options.


/*** Shared mouse functions ***/

bool update_mouse_state(mouse_state_t *mouse) {
  if((mouse->update < 3) && (mouse->force_update == false)) { return(false); } // Minimum report size is 3 bytes.
  int movement;

  // Set mouse button states    
  if(mouse_options.swap_buttons) {
    mouse->state[0] |= (mouse->rmb << MOUSE_LMB_BIT);
    mouse->state[0] |= (mouse->lmb << MOUSE_RMB_BIT);
  }
  else {
    mouse->state[0] |= (mouse->lmb << MOUSE_LMB_BIT);
    mouse->state[0] |= (mouse->rmb << MOUSE_RMB_BIT);
  }

  // Clamp x, y, wheel inputs to values allowable by protocol.  
  mouse->x = clampi(mouse->x, -127, 127);
  mouse->y = clampi(mouse->y, -127, 127);

  // Update aggregated mouse movement state
  movement = mouse->x & 0xc0; // Get 2 upper bits of X movement
  mouse->state[0] = mouse->state[0] | (movement >> 6); // Set packet bits, 8th bit to 2nd bit (Discards bits)
  mouse->state[1] = mouse->state[1] | (mouse->x & 0x3f);

  movement = mouse->y & 0xc0; // Get 2 upper bits of Y movement
  mouse->state[0] = mouse->state[0] | (movement >> 4);
  mouse->state[2] = mouse->state[2] | (mouse->y & 0x3f);

  // Protocol specific handling
  switch(mouse_options.protocol) {
    case PROTO_LOGITECH: 
      if(mouse->mmb) {
	      mouse->state[3] = 0x20;
      }
      // Note: Implicit, MMB release gets also sent as 4 byte packet (push_update on mmb change).
      break;
    case PROTO_MSWHEEL: 
      mouse->wheel = clampi(mouse->wheel, -15, 15);
      mouse->state[3] |= (mouse->mmb << MOUSE_MMB_BIT);
      mouse->state[3] = mouse->state[3] | (-mouse->wheel & 0x0f); // 127(negatives) when scrolling up, 1(positives) when scrolling down.
      mouse->update = mouse_protocol[mouse_options.protocol].report_len;
      break;
    default:
      // Get protocol default report length
      mouse->update = mouse_protocol[mouse_options.protocol].report_len;
  }

  return(true);
}

void reset_mouse_state(mouse_state_t *mouse) {
  memcpy( mouse->state, init_mouse_state, sizeof(mouse->state) ); // Set packet memory to initial state
  mouse->update = -1;
  mouse->force_update = 0;
  mouse->x = mouse->y = mouse->wheel = 0;
  // Do not reset button states here, will be updated on release of buttons.
}

// Changing settings based on user input
void runtime_settings(mouse_state_t *mouse) {

  // Sensitivity handling
  if(mouse->lmb && mouse->rmb) {
    // Handle sensitivity changes
    if(mouse->wheel != 0) {
      if(mouse->wheel < 0) { mouse_options.sensitivity -= 0.2; }
      else { mouse_options.sensitivity += 0.2; }
      mouse_options.sensitivity = clampf(mouse_options.sensitivity, 0.2, 3.0);
    }

    if(mouse->mmb) {
      // Toggle between mouse modes?
    }
  }
}

// Adjust mouse input based on sensitivity
void input_sensitivity(mouse_state_t *mouse) {
  mouse->x = mouse->x * mouse_options.sensitivity;
  mouse->y = mouse->y * mouse_options.sensitivity;
}

// Helper function for keeping mouse sensitivity setting consistent.
void set_sensitivity(scan_int_t scan_i) {
  if(scan_i.found) {
    mouse_options.sensitivity = clampf(((float)scan_i.value / 10), 0.2, 3.0);
  }
}

/*** Flow control functions ***/

// Make sure we don't clobber higher update requests with lower ones.
void push_update(mouse_state_t *mouse, bool full_packet) {
  if(full_packet || mouse->update > 3) { mouse->update = 4; }
  else { mouse->update = 3; }
}
