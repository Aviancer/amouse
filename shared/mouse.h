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

extern mouse_opts_t mouse_options; // Global options

extern mouse_proto_t mouse_protocol[3]; // Global options
extern uint mouse_protocol_num;

extern const char amouse_title[];

extern uint8_t pkt_intellimouse_intro[];
extern int pkt_intellimouse_intro_len;

/*** Console definitions ***/

typedef struct console_menu {
  const char   prompt[12];
  const char*  help_string;
  size_t       help_size;
  uint         parent_menu;
} console_menu_t;

enum MENU_CONTEXT {
  CONTEXT_EXIT_MENU  = 0,
  CONTEXT_MAIN_MENU  = 1,
  CONTEXT_FLASH_MENU = 2
};

/* Functions */

void console(int fd);

bool update_mouse_state(mouse_state_t *mouse);

void reset_mouse_state(mouse_state_t *mouse);

void runtime_settings(mouse_state_t *mouse);

void input_sensitivity(mouse_state_t *mouse);

void set_sensitivity(scan_int_t scan_i);

void push_update(mouse_state_t *mouse, bool full_packet);

#endif // MOUSE_H_
