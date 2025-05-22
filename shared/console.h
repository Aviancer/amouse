/*
 * Anachro Mouse, a usb to serial mouse adaptor. Copyright (C) 2025 Aviancer <oss+amouse@skyvian.me>
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

#ifndef CONSOLE_H_
#define CONSOLE_H_

/*** Shared definitions ***/

extern const char g_amouse_title[];

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

#endif // CONSOLE_H_
