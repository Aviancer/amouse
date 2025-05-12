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

#include "mouse_defs.h"
#include "utils.h"

/*** Shared definitions ***/

extern const char amouse_title[];

extern uint8_t pkt_intellimouse_intro[];
extern int pkt_intellimouse_intro_len;

/* Functions */

void console(int fd);

bool update_mouse_state(mouse_state_t *mouse);

void reset_mouse_state(mouse_state_t *mouse);

void runtime_settings(mouse_state_t *mouse);

void input_sensitivity(mouse_state_t *mouse);

void set_sensitivity(scan_int_t scan_i);

void push_update(mouse_state_t *mouse, bool full_packet);

#endif // MOUSE_H_
