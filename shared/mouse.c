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
#include <unistd.h> // TODO: replace with better than usleep()
#include <stdio.h> // DEBUG
#include "../linux/src/include/version.h"
#include "../linux/src/include/serial.h"
#else 
#include "pico/stdlib.h"
#include "../pico/include/version.h"
#include "../pico/include/serial.h"
#endif 

#include "mouse.h"
#include "utils.h"

/*** Shared definitions ***/

#define CMD_BUFFER_LEN 512

const char amouse_title[] =
R"#( __ _   _ __  ___ _  _ ___ ___ 
/ _` | | '  \/ _ \ || (_-</ -_)
\__,_| |_|_|_\___/\_,_/__/\___=====_____)#";

const char amouse_menu[] =
R"#(1) Help/Usage
2) Show current settings
3) Set mouse wheel on/off
4) Set sensitivity (1-20)
5) Exit settings/Resume adapter
0) Read or write settings (Flash)
)#";

uint8_t init_mouse_state[] = "\x40\x00\x00\x00"; // Our basic mouse packet (We send 3 or 4 bytes of it)


/*** Global data / BSS (Avoid stack) ***/ 

// We should avoid calloc/malloc on embedded systems.
uint8_t cmd_buffer[CMD_BUFFER_LEN + 1] = {0};


/*** Shared mouse functions ***/

bool update_mouse_state(mouse_state_t *mouse) {
  if((mouse->update < 2) && (mouse->force_update == false)) { return(false); } // Minimum report size is 2 (3 bytes)
  int movement;

  // Set mouse button states    
  mouse->state[0] |= (mouse->lmb << MOUSE_LMB_BIT);
  mouse->state[0] |= (mouse->rmb << MOUSE_RMB_BIT);
  mouse->state[3] |= (mouse->mmb << MOUSE_MMB_BIT);

  // Clamp x, y, wheel inputs to values allowable by protocol.  
  mouse->x     = clampi(mouse->x, -127, 127);
  mouse->y     = clampi(mouse->y, -127, 127);
  mouse->wheel = clampi(mouse->wheel, -15, 15);

  // Update aggregated mouse movement state
  movement = mouse->x & 0xc0; // Get 2 upper bits of X movement
  mouse->state[0] = mouse->state[0] | (movement >> 6); // Set packet bits, 8th bit to 2nd bit (Discards bits)
  mouse->state[1] = mouse->state[1] | (mouse->x & 0x3f);

  movement = mouse->y & 0xc0; // Get 2 upper bits of Y movement
  mouse->state[0] = mouse->state[0] | (movement >> 4);
  mouse->state[2] = mouse->state[2] | (mouse->y & 0x3f);

  mouse->state[3] = mouse->state[3] | (-mouse->wheel & 0x0f); // 127(negatives) when scrolling up, 1(positives) when scrolling down.

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
      if(mouse->wheel < 0) { mouse->sensitivity -= 0.2; }
      else { mouse->sensitivity += 0.2; }
      mouse->sensitivity = clampf(mouse->sensitivity, 0.2, 2.0);
    }

    if(mouse->mmb) {
      // Toggle between mouse modes?
    }
  }
}

// Adjust mouse input based on sensitivity
void input_sensitivity(mouse_state_t *mouse) {
  mouse->x = mouse->x * mouse->sensitivity;
  mouse->y = mouse->y * mouse->sensitivity;
}


/*** Flow control functions ***/

// Make sure we don't clobber higher update requests with lower ones.
void push_update(mouse_state_t *mouse, bool full_packet) {
  if(full_packet || (mouse->update == 3)) { mouse->update = 3; }
  else { mouse->update = 2; }
}

void console(int fd) {

  uint write_pos = 0;
  uint read_len= 0;

  uint arg1, arg2;

  serial_write_terminal(fd, (uint8_t*)amouse_title, sizeof(amouse_title));
  serial_write_terminal(fd, (uint8_t*)"\n", 1);
  serial_write_terminal(fd, (uint8_t*)amouse_menu, sizeof(amouse_menu));
  serial_write_terminal(fd, (uint8_t*)"amouse> ", 8); 

  while(1) {
    write_pos += read_len;
    if(write_pos >= CMD_BUFFER_LEN) { 
      printf("Contents: #%.*s#\n", CMD_BUFFER_LEN, cmd_buffer);
      serial_write(fd, cmd_buffer, write_pos); // DEBUG
      write_pos = 0;
      usleep(1000000);
    }
    // Keep reading to buffer until buffer ends, shrink each allowed read length as more data is added.
    read_len = serial_read(fd, cmd_buffer + write_pos, (CMD_BUFFER_LEN - write_pos));
    serial_write_terminal(fd, cmd_buffer + write_pos, sizeof(uint8_t) * read_len); // Echo to terminal

    printf("%.*s\n", write_pos, cmd_buffer); // DEBUG
    usleep(50000); // TODO: Not arch independent.

    char* found_ptr; 
    found_ptr = strpbrk((char*)cmd_buffer, "\r\n"); // Check for end of line characters, \r or \n
    if(found_ptr) { 
      sscanf((char*)cmd_buffer, "%5u %5u", &arg1, &arg2); // sscanf will return 0 for int if not found.
      printf("arg1:%u arg2:%u\n", arg1, arg2);
      usleep(1000000);

      switch(arg1) {
	case 1:
	  printf("Selected help");
	  break;
	case 2:
	  printf("test\n");
	  break;
	default:
          printf("Unknown command.\n");
      }

      read_len = write_pos = 0;
      memset(cmd_buffer, 0, CMD_BUFFER_LEN);
    }

  }
}
