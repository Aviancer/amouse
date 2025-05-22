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

/*** console.c: Serial console for configuration ***/

#ifdef __linux__
#include <stdlib.h>
#include <stdint.h>
#include "../linux/src/include/version.h"
#include "../linux/src/include/serial.h"
#include "../linux/src/include/storage.h"
#include "../linux/src/include/wrappers.h"
#else 
#include "pico/stdlib.h"
#include "../pico/include/version.h"
#include "../pico/include/serial.h"
#include "../pico/include/storage.h"
#include "../pico/include/wrappers.h"
#endif 

#include "console.h"
#include "settings.h"
#include "utils.h"

/*** Shared definitions ***/

#define CMD_BUFFER_LEN 256
#define CTRL_L "\x0c"

const char g_amouse_title[] =
R"#( __ _   _ __  ___ _  _ ___ ___ 
/ _` | | '  \/ _ \ || (_-</ -_)
\__,_| |_|_|_\___/\_,_/__/\___=====_____)#";

const char help_menu[] =
R"#(1) Help/Usage
2) Show current settings
3) Set sensitivity (2-30)
4) Set mouse protocol (0-2)
   Proto(0:MS two-button 1: Logitech three-button 2: MS wheel)
5) Swap left/right buttons.
6) Read or write settings (Flash)
0) Exit settings/Resume adapter
   eg. to set sensitivity to 11, enter: 3 11
)#";

const char help_menu_flash[] =
R"#(1) Help/Usage
2) Load settings from flash
3) Write current settings to flash
0) Return to main menu
)#";

// Serial console menu contexts
console_menu_t console_menu[3] =
{
  // Prompt     Help text        Help size                Parent context
  { "exit",     "",              0,                       CONTEXT_EXIT_MENU }, // Dummy entry for exit
  { "amouse",   help_menu,       sizeof(help_menu),       CONTEXT_EXIT_MENU }, // Main menu
  { "flash",    help_menu_flash, sizeof(help_menu_flash), CONTEXT_MAIN_MENU }  // Flash menu
};
int console_context = CONTEXT_MAIN_MENU; // Default context

const char amouse_bye[] = "Bye!\n    Never too late to dream and frolic - fly!\n";

/*** Global data / BSS (Avoid stack) ***/ 

// TODO: Any?


// We should avoid calloc/malloc on embedded systems.
uint8_t cmd_buffer[CMD_BUFFER_LEN + 1] = {0};

void console_printvar(int fd, char* prefix, char* variable, char* suffix) {
  // Write until \0
  serial_write_terminal(fd, (uint8_t*)prefix, 1024);
  serial_write_terminal(fd, (uint8_t*)variable, 1024);
  serial_write_terminal(fd, (uint8_t*)suffix, 1024);
}

void console_prompt(int fd) {
  serial_write_terminal(fd,
    (uint8_t*)console_menu[console_context].prompt,
    sizeof(console_menu[console_context].prompt));
  serial_write_terminal(fd, (uint8_t*)"> ", 2); // Add prompt end
}

void console_help(int fd) {
  // Add context to which help we are printing
  console_printvar(fd, "[", (char*)console_menu[console_context].prompt, "]\r\n");
  serial_write_terminal(fd,
    (uint8_t*)console_menu[console_context].help_string, 
    console_menu[console_context].help_size
  );
}

// Helper to update context and display relevant help
void console_new_context(int fd, int context) {
  console_context = context;
  console_help(fd);
}

// Process command buffer for backspace
void console_backspace(uint8_t cmd_buffer[], char* found_ptr, uint* write_pos) {
  uint pread_pos = 0;
  uint pwrite_pos = 0;

  while((cmd_buffer + pread_pos < (uint8_t*)found_ptr) && (pread_pos < CMD_BUFFER_LEN)) { // Read until linebreak or end of buffer.
    // Handle backspace
    if(cmd_buffer[pread_pos] != '\b') {
      cmd_buffer[pwrite_pos] = cmd_buffer[pread_pos];
      pwrite_pos++;
    }
    // write_pos is a uint, so wraparound > than current value on subtraction. Avoid escaping buffer.
    else if(pwrite_pos > 0) {
      pwrite_pos--; 
    }
    pread_pos++;
  }
  cmd_buffer[pwrite_pos] = '\0'; // Zero the backspace from the command buffer.
  if(pwrite_pos > 0) { 
    pwrite_pos--;
    cmd_buffer[pwrite_pos] = '\0'; // Zero removed character
  }
  *write_pos = pwrite_pos;
}

void console_menu_main(int fd, scan_int_t* scan_i) {
  char itoa_buffer[6] = {0}; // Re-usable buffer for converting ints to char arr
  scan_int_t scan_ii;

  switch(scan_i->value) {
    case 1: // Help
      console_help(fd);
      break;
    case 2: // Settings
      serial_write_terminal(fd, (uint8_t*)"[Settings]\n", 11);
      console_printvar(fd, "  Mouse protocol: ", g_mouse_protocol[g_mouse_options.protocol].name, "\n");
      itoa((int)(g_mouse_options.sensitivity * 10), itoa_buffer, sizeof(itoa_buffer) - 1);
      console_printvar(fd, "  Mouse sensitivity: ", itoa_buffer, "\n");
      console_printvar(fd, "  Mouse buttons: ", (g_mouse_options.swap_buttons) ? "Swapped" : "Not swapped", "\n");
      break;
    case 3: // Sensitivity
      scan_ii = scan_int(cmd_buffer, scan_i->offset, CMD_BUFFER_LEN, 5);
      set_sensitivity(scan_ii);
      itoa((int)(g_mouse_options.sensitivity * 10), itoa_buffer, sizeof(itoa_buffer) - 1);
      console_printvar(fd, "Mouse sensitivity set to ", itoa_buffer, ".\n");
      break;
    case 4: // Mouse protocol
      scan_ii = scan_int(cmd_buffer, scan_i->offset, CMD_BUFFER_LEN, 1);
      if(scan_ii.found) { g_mouse_options.protocol = clampi(scan_ii.value, 0, 2); }
      console_printvar(fd, "Mouse protocol set to ", g_mouse_protocol[g_mouse_options.protocol].name, ". You may want to re-initialize OS mouse driver.\n");
      break;
    case 5: // Swap left/right buttons
      scan_ii = scan_int(cmd_buffer, scan_i->offset, CMD_BUFFER_LEN, 1);
      if(scan_ii.found) { g_mouse_options.swap_buttons = clampi(scan_ii.value, 0, 1); }
      else { g_mouse_options.swap_buttons = !g_mouse_options.swap_buttons; }
      console_printvar(fd, "Mouse buttons are now ", (g_mouse_options.swap_buttons) ? "swapped" : "unswapped", ".\n");
      break;
    case 6: // Menu: Write/load flash
      console_new_context(fd, CONTEXT_FLASH_MENU);
      break;
    case 0: // Exit
      console_new_context(fd, CONTEXT_EXIT_MENU);
      return;
    default:
      serial_write_terminal(fd, (uint8_t*)"Command not valid.\n", 19); 
  }
}

void console_menu_flash(int fd, scan_int_t* scan_i) {
  uint8_t binary_settings[SETTINGS_SIZE];

  switch(scan_i->value) {
    case 1: // Help
      console_help(fd);
      break;
    case 2: // Load binary settings from storage
      if(settings_decode(ptr_flash_settings(), &g_mouse_options)) {
        serial_write_terminal(fd, (uint8_t*)"Settings loaded.\n", 17);
      }
      else {
        serial_write_terminal(fd, (uint8_t*)"Error loading or corrupt data.\n", 34);
      }
      break;
    case 3: // Write binary settings to storage
      serial_write_terminal(fd, (uint8_t*)"Writing settings.. ", 19);
      settings_encode(&binary_settings[0], &g_mouse_options);
      write_flash_settings(&binary_settings[0], sizeof(binary_settings));
      serial_write_terminal(fd, (uint8_t*)"Done\n", 5);
      break;
    case 0: // Back to parent menu
      console_new_context(fd, console_menu[console_context].parent_menu);
      return;
    default:
      serial_write_terminal(fd, (uint8_t*)"Command not valid.\n", 19);
  }
}

// Serial console main
void console(int fd) {

  uint write_pos = 0;
  int read_len = 0;
  
  scan_int_t scan_i;
  
  memset(cmd_buffer, 0, CMD_BUFFER_LEN); // Clear command buffer if we get called multiple times.

  serial_write_terminal(fd, (uint8_t*)g_amouse_title, sizeof(g_amouse_title));
  serial_write_terminal(fd, (uint8_t*)"\nv", 2);
  serial_write_terminal(fd, (uint8_t*)V_FULL, sizeof(V_FULL));
  serial_write_terminal(fd, (uint8_t*)"\n", 1);
  console_new_context(fd, CONTEXT_MAIN_MENU);
  console_prompt(fd);

  while(1) {

    write_pos += read_len;
    // Enforce buffer length
    if(write_pos >= CMD_BUFFER_LEN) { 
      write_pos = CMD_BUFFER_LEN - 1; // Keep one byte of buffer free for linebreak or backspace.
    }
    // Keep reading to buffer until buffer ends, shrink each allowed read length as more data is added.
    if(write_pos < CMD_BUFFER_LEN) {
      read_len = serial_read(fd, cmd_buffer + write_pos, (CMD_BUFFER_LEN - write_pos));
    }
    else {
      serial_read(fd, cmd_buffer + write_pos, 1); // Keep reading for linebreak or backspace.
    }

    // Echo to console
    if(cmd_buffer[write_pos] != '\b') {
      if(write_pos < CMD_BUFFER_LEN - 1) {
        serial_write_terminal(fd, cmd_buffer + write_pos, sizeof(uint8_t) * read_len); 
      }
    }
    else if (write_pos > 0) { 
      serial_write_terminal(fd, cmd_buffer + write_pos, sizeof(uint8_t) * read_len); 
    }

    char* found_ptr; 

    // Handle ctrl+l (Redraw screen)
    found_ptr = strpbrk((char*)cmd_buffer, CTRL_L); // Check for end of line characters (\r or \n) or ctrl+l
    if(found_ptr) {
      memset(cmd_buffer + write_pos, 0, CMD_BUFFER_LEN - write_pos); // Clear bytes after ctrl+l from command line.
      write_pos = (uint8_t*)found_ptr - cmd_buffer;

      // Rewrite current line as seen by console
      serial_write_terminal(fd, (uint8_t*)CTRL_L, 1); // Ensure terminal screen gets reset if buffer full (Clear screen)
      serial_write_terminal(fd, (uint8_t*)"\r", 1); // In the case ctrl+l is not handled by terminal.
      console_prompt(fd);
      serial_write_terminal(fd, cmd_buffer, write_pos + 1); // Write prompt up to where ctrl+l was found.

      read_len = 0; // Ctrl+l and any additional input was discarded.
    }
    
    found_ptr = strpbrk((char*)cmd_buffer, "\b"); 
    if(found_ptr) {
      console_backspace(cmd_buffer, found_ptr, &write_pos);
      read_len = 0; // If write position is updated by backspace, leave it where it is now.
    }

    // Check for end of line characters (\r or \n) or ctrl+l
    // Full buffer will also be sent as a command even without linebreak.
    found_ptr = strpbrk((char*)cmd_buffer, "\r\n"); 
    if(found_ptr) { 
      serial_write_terminal(fd, (uint8_t*)"\n", 1); // Confirm client linebreak

      scan_i = scan_int(cmd_buffer, 0, CMD_BUFFER_LEN, 5);

      if(scan_i.found) {

        // Input handling in context to current menu
        // We could also use function pointer refs here but this is enough for now.
        switch(console_context) {
          case CONTEXT_FLASH_MENU:
            console_menu_flash(fd, &scan_i);
            break;
          default:
            console_menu_main(fd, &scan_i);
        }
      }

      read_len = write_pos = 0;
      memset(cmd_buffer, 0, CMD_BUFFER_LEN);

      // Exit handling
      if(console_context == CONTEXT_EXIT_MENU) {
        serial_write_terminal(fd, (uint8_t*)amouse_bye, sizeof(amouse_bye));
        return;
      }
 
      console_prompt(fd);
    }

    a_usleep(1);
  }
}
