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

/* mouse.c: Architecture independent mouse logic */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __linux__
#include <stdlib.h>
#include "../linux/src/include/version.h"
#include "../linux/src/include/serial.h"
#include "../linux/src/include/wrappers.h"
#else 
#include "pico/stdlib.h"
#include "../pico/include/version.h"
#include "../pico/include/serial.h"
#include "../pico/include/wrappers.h"
#endif 

#include "mouse.h"
#include "utils.h"

/*** Shared definitions ***/

#define CMD_BUFFER_LEN 256
#define CTRL_L "\x0c"

const char amouse_title[] =
R"#( __ _   _ __  ___ _  _ ___ ___ 
/ _` | | '  \/ _ \ || (_-</ -_)
\__,_| |_|_|_\___/\_,_/__/\___=====_____)#";

const char amouse_menu[] =
R"#(1) Help/Usage
2) Show current settings
3) Set sensitivity (1-25)
4) Set mouse protocol (0-2)
   Proto(0:MS two-button 1: Logitech three-button 2: MS wheel)
5) Swap left/right buttons.
6) Exit settings/Resume adapter
0) [TBD] Read or write settings (Flash)
   eg. to set sensitivity to 11, enter: 3 11
)#";

const char amouse_prompt[] = "amouse> ";
const char amouse_bye[] = "Bye!\n    Never too late for dreams and play - fly!\n";

uint8_t init_mouse_state[] = "\x40\x00\x00\x00"; // Our basic mouse packet (We send 3 or 4 bytes of it)

// Define available mouse protocols
mouse_proto_t mouse_protocol[3] =
{
// Name           Intro Len Btn Wheel  ReportLen
  {"MS 2-button", "M",  1,  2,  false, 3}, // MS_2BUTTON = 0
  {"Logitech",    "M3", 2,  3,  false, 3}, // LOGITECH   = 1, report is 3-4
  {"MS wheeled",  "MZ", 2,  3,  true,  4}  // MS_WHEELED = 2
};


// Full Serial Mouse intro with PnP information (Microsoft IntelliMouse)
uint8_t pkt_intellimouse_intro[] = {0x4D,0x5A,0x40,0x00,0x00,0x00,0x08,0x01,0x24,0x2d,0x33,0x28,0x10,0x10,0x10,0x11,
                                    0x3c,0x21,0x36,0x29,0x21,0x2e,0x23,0x25,0x32,0x3c,0x2d,0x2f,0x35,0x33,0x25,0x3c,
                                    0x30,0x2e,0x30,0x10,0x26,0x10,0x21,0x3c,0x2d,0x29,0x23,0x32,0x2f,0x33,0x2f,0x26,
                                    0x34,0x00,0x2d,0x2f,0x35,0x33,0x25,0x00,0x37,0x29,0x34,0x28,0x00,0x37,0x28,0x25,
                                    0x25,0x2c,0x12,0x16,0x09};
int pkt_intellimouse_intro_len = 69;

/*** Global data / BSS (Avoid stack) ***/ 

mouse_opts_t mouse_options; // Global user settable options.

// We should avoid calloc/malloc on embedded systems.
uint8_t cmd_buffer[CMD_BUFFER_LEN + 1] = {0};


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
      mouse_options.sensitivity = clampf(mouse_options.sensitivity, 0.2, 2.5);
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


/*** Flow control functions ***/

// Make sure we don't clobber higher update requests with lower ones.
void push_update(mouse_state_t *mouse, bool full_packet) {
  if(full_packet || mouse->update > 3) { mouse->update = 4; }
  else { mouse->update = 3; }
}


/*** Serial console ***/

// Character array to unsigned integer
uint atou(char* intbuffer, uint max_digits) {
  uint integer = 0;

  for(int i=0; i < max_digits && intbuffer[i] >= '0' && intbuffer[i] <= '9'; i++) {
    integer *= 10;
    integer += (intbuffer[i] - 0x30);
  }

  return integer;
}

// Integer to character array
void itoa(int integer, char* intbuffer, uint max_digits) {
  int i;
  
  memset(intbuffer, 0, max_digits); // Zero potential number space.

  // Figure out number of digits, clamped to max.
  int scratch = integer;
  for(i=0; scratch > 0 && i < max_digits; i++) {
    scratch /= 10;
  }
  if(integer == 0) { i = 1; }
  i--; // Shift to zero indexed

  for(; i >= 0 ; i--) { // Walk int backwards
    intbuffer[i] = 0x30 + (integer % 10); // 0x30 = 0
    integer /= 10;
  }  
}

// Return type for scan_int()
typedef struct scan_int_ret {
  bool found;
  int value;
  uint16_t offset; // How many bytes we have read into buffer.
} scan_int_t;

// Scan for number in character array
// Avoids sscanf() which tripled project size from including <stdio.h>
scan_int_t scan_int(uint8_t* buffer, uint i, uint scan_size, uint max_digits) {
  scan_int_t result;

  bool abort = false;
  char intbuffer[6] = {0};  
  uint j=0;
  uint usable_size=5;
 
  // Cap requested digits to buffer size.
  if(max_digits > sizeof(intbuffer) - 1) { max_digits = sizeof(intbuffer) - 1; }

  // Scan up to number
  for(; !abort && i <= scan_size && (buffer[i] < '0' || buffer[i] > '9'); i++) {
    // Abort if null byte found before number.
    if(buffer[i] == '\0') { 
      abort = true;
      result.found = false;
      result.value = -1;
    }
  }

  if(!abort) {
    if(scan_size - i > 5) { usable_size = i + 5; }
    // Copy until number end
    for(; i <= usable_size && buffer[i] >= '0' && buffer[i] <= '9' && j < max_digits; i++) {
      intbuffer[j] = buffer[i]; 
      j++;
    }

    result.found  = (j > 0);
    result.value  = atou(intbuffer, 5);
  }

  result.offset = i; 
  return(result);
}

void console_printvar(int fd, char* prefix, char* variable, char* suffix) {
  // Write until \0
  serial_write_terminal(fd, (uint8_t*)prefix, 1024);
  serial_write_terminal(fd, (uint8_t*)variable, 1024);
  serial_write_terminal(fd, (uint8_t*)suffix, 1024);
}

void console_prompt(int fd) {
  serial_write_terminal(fd, (uint8_t*)amouse_prompt, sizeof(amouse_prompt)); 
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

// Serial console main
void console(int fd) {

  uint write_pos = 0;
  int read_len = 0;

  scan_int_t scan_i;
  char itoa_buffer[6] = {0}; // Re-usable buffer for converting ints to char arr
  
  memset(cmd_buffer, 0, CMD_BUFFER_LEN); // Clear command buffer if we get called multiple times.

  serial_write_terminal(fd, (uint8_t*)amouse_title, sizeof(amouse_title));
  serial_write_terminal(fd, (uint8_t*)"\nv", 2);
  serial_write_terminal(fd, (uint8_t*)V_FULL, sizeof(V_FULL));
  serial_write_terminal(fd, (uint8_t*)"\n", 1);
  serial_write_terminal(fd, (uint8_t*)amouse_menu, sizeof(amouse_menu));
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
      serial_write_terminal(fd, (uint8_t*)amouse_prompt, sizeof(amouse_prompt));
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
	switch(scan_i.value) {
	  case 1: // Help
	    serial_write_terminal(fd, (uint8_t*)amouse_menu, sizeof(amouse_menu));
	    break;
	  case 2: // Settings
	    serial_write_terminal(fd, (uint8_t*)"[Settings]\n", 11);
	    console_printvar(fd, "  Mouse protocol: ", mouse_protocol[mouse_options.protocol].name, "\n");
	    itoa((int)(mouse_options.sensitivity * 10), itoa_buffer, sizeof(itoa_buffer) - 1);
	    console_printvar(fd, "  Mouse sensitivity: ", itoa_buffer, "\n");
	    console_printvar(fd, "  Mouse buttons: ", (mouse_options.swap_buttons) ? "Swapped" : "Not swapped", "\n");
	    break;
	  case 3: // Sensitivity
	    scan_i = scan_int(cmd_buffer, scan_i.offset, CMD_BUFFER_LEN, 5);
	    if(scan_i.found) { 
	      mouse_options.sensitivity = clampf(((float)scan_i.value / 10), 0.1, 2.5);
	    }
	    itoa((int)(mouse_options.sensitivity * 10), itoa_buffer, sizeof(itoa_buffer) - 1);
	    console_printvar(fd, "Mouse sensitivity set to ", itoa_buffer, ".\n");
	    break;
	  case 4: // Mouse protocol
	    scan_i = scan_int(cmd_buffer, scan_i.offset, CMD_BUFFER_LEN, 1);
	    if(scan_i.found) { mouse_options.protocol = clampi(scan_i.value, 0, 2); }
	    //else { mouse_options.wheel = !mouse_options.wheel; }
	    console_printvar(fd, "Mouse protocol set to ", mouse_protocol[mouse_options.protocol].name, ". You may want to re-initialize OS mouse driver.\n");
	    break;
	  case 5: // Swap left/right buttons
	    scan_i = scan_int(cmd_buffer, scan_i.offset, CMD_BUFFER_LEN, 1);
	    if(scan_i.found) { mouse_options.swap_buttons = clampi(scan_i.value, 0, 1); }
	    else { mouse_options.swap_buttons = !mouse_options.swap_buttons; }
	    console_printvar(fd, "Mouse buttons are now ", (mouse_options.swap_buttons) ? "swapped" : "unswapped", ".\n");
	    break;
	  case 6: // Exit
	    serial_write_terminal(fd, (uint8_t*)amouse_bye, sizeof(amouse_bye));
	    return;
	    break;
	  case 0: // Write/load flash
	    serial_write_terminal(fd, (uint8_t*)"Not yet implemented.\n", 22); 
	    break;
	  default:
	    serial_write_terminal(fd, (uint8_t*)"Command not valid.\n", 19); 
	}
      }

      read_len = write_pos = 0;
      memset(cmd_buffer, 0, CMD_BUFFER_LEN);
      console_prompt(fd);
    }

    a_usleep(1);
  }
}
