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

#ifndef MOUSE_H_
#define MOUSE_H_

#include <stdbool.h>

/* Protocol definitions */
#define MOUSE_LMB_BIT 5 // Defines << shift for bit position
#define MOUSE_RMB_BIT 4
#define MOUSE_MMB_BIT 4 // Shift 4 times in 4th byte

// Delay between data packets for 1200 baud
#define U_FULL_SECOND 1000000L      // 1s in microseconds
#define SERIALDELAY_3B  22700       // 3 bytes (microseconds)
#define SERIALDELAY_4B  30000       // 4 bytes (microseconds)
// 1200 baud (bits/s) is 133.333333333... bytes/s
// 44.44.. updates per second with 3 bytes.
// 33.25.. updates per second with 4 bytes.
// ~0.0075 seconds per byte, target time calculated for 4 bytes.
#define NS_FULL_SECOND    1000000000L   // 1s in nanoseconds
#define NS_SERIALDELAY_3B   22700000 // 3 bytes
#define NS_SERIALDELAY_4B   30000000 // 4 bytes

// Struct for storing information about accumulated mouse state
typedef struct mouse_state {
  int pc_state; // Current state of mouse driver initialization on PC.
  uint8_t state[4]; // Mouse state
  int x, y, wheel;
  int update; // How many bytes to send
  bool lmb, rmb, mmb, force_update;
  float sensitivity; // Sensitivity coefficient
} mouse_state_t;

// States of mouse init request from PC
enum PC_INIT_STATES {
  CTS_UNINIT   = 0, // Initial state
  CTS_LOW_INIT = 1, // CTS pin has been set low, wait for high.
  CTS_TOGGLED  = 2  // CTS was low, now high -> do ident.
};

/* Functions */

bool update_mouse_state(mouse_state_t *mouse);

void reset_mouse_state(mouse_state_t *mouse);

void push_update(mouse_state_t *mouse, bool full_packet);

#endif // MOUSE_H_
