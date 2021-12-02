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
*/

#include <stdio.h>    // Standard input / output
#include <stdlib.h>   // Standard input / output
#include <fcntl.h>    // File control defs, open()
#include <unistd.h>   // UNIX standard function defs, close()
#include <termios.h>  // POSIX terminal control defs
#include <errno.h>    // Error number definitions
#include <string.h>   // strerror()
#include <stdint.h>   // for uint8_t
#include <time.h>     // for time()

#include "include/version.h"
#include "include/serial.h"
#include "../../shared/mouse.h"
#include "../../shared/utils.h"

// Linux specific
#include <sys/ioctl.h> // ioctl (serial pins, mouse exclusive access)
#include <libevdev.h>  // input dev
#include <getopt.h>    // getopt


/*** Program parameters ***/ 

char title[] = 
R"#(  __ _   _ __  ___ _  _ ___ ___ 
 / _` | | '  \/ _ \ || (_-</ -_)
 \__,_| |_|_|_\___/\_,_/__/\___=====_____)#";

// Struct for storing pointers to dynamically allocated memory containing options.
struct opts {
  char *mousepath; // Pointers, memory is dynamically allocated.
  char *serialpath;
  int wheel;
  int exclusive;
  int immediate;
  int debug;
};


/*** Linux console ***/

void showhelp(char *argv[]) {
  printf("%s\n\n", title);
  printf("Anachro Mouse v%d.%d.%d, a usb to serial mouse adaptor.\n" \
         "Usage: %s -m <mouse_input> -s <serial_output>\n\n" \
         "  -m <File> to read mouse input from (/dev/input/*)\n" \
         "  -s <File> to write to serial port with (/dev/tty*)\n" \
	 "  -w Disable mousewheel, switch to basic MS protocol\n" \
	 "  -e Disable exclusive access to mouse\n" \
	 "  -i Immediate ident mode, disables waiting for CTS pin\n" \
	 "  -d Print out debug information on mouse state\n", V_MAJOR, V_MINOR, V_REVISION, argv[0]);
}

void parse_opts(int argc, char **argv, struct opts *options) {
  int option_index = 0;
  int quit = 0;

  while (( option_index = getopt(argc, argv, "hm:s:weid")) != -1) {
    // Defaults
    options->wheel = 1;
    options->exclusive = 1;

    switch(option_index) {
      case 'm':
        options->mousepath = strndup(optarg, 4096); // Max path size is 4095, plus a null byte
        break;
      case 's':
        options->serialpath = strndup(optarg, 4096);
        break;

      case 'h':
        showhelp(argv); exit(0);
        break;
      case 'w':
	options->wheel = 0;
	break;
      case 'e':
	options->exclusive = 0; // Computer will also get mouse inputs.
	break;
      case 'i':
	options->immediate = 1; // Don't wait for CTS pin to ident
	break;
      case 'd':
	options->debug = 1; // Enable debug prints
	break;
      default:
        fprintf(stderr, "Invalid option on commandline, ignoring.\n");
    }
  }

  if(options->mousepath == NULL) { 
    fprintf(stderr, "You must define a path with -m to your mouse /dev/input/* file.\n");
    quit = 1;
  }
  if(options->serialpath == NULL) { 
    fprintf(stderr, "You must define a path with -s to your serial port /dev/tty* file.\n");
    quit = 1;
  }
  if(quit != 0) { exit(0); }
}

void aprint(const char *message) {
    printf("amouse> %s\n", message);
}

/*** USB comms ***/

static int open_usbinput(const char* device, int exclusive) {
  int fd;
  int returncode = 1;
  struct libevdev* dev;

  fd = open(device, O_RDONLY | O_NONBLOCK);
  if (fd < 0) { return -1; }

  /* Check if it's a mouse */
  returncode = libevdev_new_from_fd(fd, &dev);
  if (returncode < 0) {
    fprintf(stderr, "Error: %d %s\n", -returncode, strerror(-returncode));
    return -1;
  }
  returncode = libevdev_has_event_type(dev, EV_REL) &&
               libevdev_has_event_code(dev, EV_REL, REL_X) &&
               libevdev_has_event_code(dev, EV_REL, REL_Y) &&
               libevdev_has_event_code(dev, EV_KEY, BTN_LEFT) &&
               libevdev_has_event_code(dev, EV_KEY, BTN_MIDDLE) &&
               libevdev_has_event_code(dev, EV_KEY, BTN_RIGHT);
  libevdev_free(dev);

  if (returncode) { 
    if(exclusive) { ioctl(fd, EVIOCGRAB, 1); } // Get exclusive mouse access
    return fd;
  }

  close(fd);
  return -1;
}

static inline void process_mouse_report(mouse_state_t *mouse, struct input_event const *ev, struct opts *options) {
  /** Handle mouse buttons ***/
  if(ev->type == EV_KEY) {
    switch(ev->code) {
      case BTN_LEFT:
	mouse->lmb = ev->value;
	mouse->force_update = true;
	push_update(mouse, mouse->mmb);
	break;
      case BTN_RIGHT:
	mouse->rmb = ev->value;
	mouse->force_update = true;
	push_update(mouse, mouse->mmb);
	break;
      case BTN_MIDDLE:
	if(options->wheel) {
	  mouse->mmb = ev->value;
	  mouse->force_update = true;
	  push_update(mouse, true); // Every time MMB changes (on or off), must send 4 bytes.
	}
	break;
    }
  }
  
  /*** Handle relative movement ***/
  else if (ev->type == EV_REL) {
    switch(ev->code) {
      case REL_X:
	mouse->x += ev->value;
	mouse->x = clampi(mouse->x, -36862, 36862);
	break;
      case REL_Y:
	mouse->y += ev->value;
	mouse->y = clampi(mouse->y, -36862, 36862);
	break;
      case REL_WHEEL:
	if(options->wheel) {
	  mouse->wheel += ev->value;
	  mouse->wheel = clampi(mouse->wheel, -63, 63);
	  push_update(mouse, true);
	}
	break;
    }
    push_update(mouse, mouse->mmb);
  }
}


/*** Main init & loop ***/

int main(int argc, char **argv) {
  struct termios old_tty;

  // Parse commandline options
  if(argc < 2) { showhelp(argv); exit(0); }
  struct opts *options = (struct opts*) calloc(1, sizeof(struct opts)); // Memory is zeroed by calloc
  if (options == NULL) {
    fprintf(stderr, "Failed calloc() for opts: %d: %s\n", errno, strerror(errno));
    exit(-1);
  }
  parse_opts(argc, argv, options);

  /*** USB mouse device input ***/
  int mouse_fd = open_usbinput(options->mousepath, options->exclusive);
  if(mouse_fd < 0) {
    fprintf(stderr, "Mouse device file open() failed: %d: %s\n", errno, strerror(errno));
    exit(-1);
  }

  struct input_event ev; // Input events
  int returncode;
  struct libevdev* mouse_dev;

  // Create mouse device
  returncode = libevdev_new_from_fd(mouse_fd, &mouse_dev);
  if (returncode < 0) {
    fprintf(stderr, "libedev_new failed: %d %s\n", -returncode, strerror(-returncode));
    exit(-1);
  }

  /*** Serial device ***/
  int fd;
  fd = open(options->serialpath, O_RDWR | O_NOCTTY | O_NONBLOCK); 
  if(fd < 0) {
    fprintf(stderr, "Serial device file open() failed: %d: %s\n", errno, strerror(errno));
    exit(-1);
  }

  if (tcgetattr(fd, &old_tty) != 0) {
    fprintf(stderr, "tcgetattr() failed: %d: %s\n", errno, strerror(errno));
  }
 
  // Initialize serial parameters 
  setup_tty(fd, (speed_t)B1200);
  enable_pin(fd, TIOCM_RTS | TIOCM_DTR);

  fcntl (0, F_SETFL, O_NONBLOCK); // Nonblock 0=stdin
  setvbuf(stdout, NULL, _IONBF, 0); // Unbuffer stdout
  
  // Aggregate movements before sending
  struct timespec time_now, time_target, time_diff;
  mouse_state_t mouse;
  mouse.pc_state = CTS_UNINIT;
  mouse.sensitivity = 1.0;
  reset_mouse_state(&mouse); // Set packet memory to initial state

  int i; // Allocate outside main loop instead of allocating every time.

  time_target = get_target_time(NS_SERIALDELAY_3B);
  
  printf("%s\n\n", title);
  aprint("Waiting for PC to initialize mouse driver..");

  // Ident immediately on program start up.
  if(options->immediate) {
    aprint("Performing immediate identification as mouse.");
    mouse_ident(fd, options->wheel);
    mouse.pc_state = CTS_TOGGLED; // Bypass CTS detection, send events straight away.
  }


  /*** Main loop ***/

  /* uint8_t *buffer; // DEBUG
  buffer = (uint8_t *)malloc(sizeof(uint8_t)*1024); */

  while(1) {
    bool pc_pins = get_pin(fd, TIOCM_CTS | TIOCM_DSR);

    // DEBUG
    /* int len = serial_read(fd, buffer, 1024);
    if(len > 0) {
      printf("%.*s", len, buffer);
      serial_write(fd, buffer, sizeof(uint8_t)*len);
    } */

    if(!pc_pins) { // Computers RTS & DTR low 
      mouse.pc_state = CTS_LOW_INIT;
    }

    // Mouse initiaizing request detected
    if(pc_pins && mouse.pc_state == CTS_LOW_INIT) {
      if(options->debug) {
	aprint("Computers RTS & DTR pins set low, identifying as mouse.");
      }
      mouse.pc_state = CTS_TOGGLED;
      mouse_ident(fd, options->wheel);
      /* Negotiate 2400 baud rate 
       *
       * Microsoft protocols may be limited to only 1200 baud.
       *
       * */
      //setup_tty(fd, (speed_t)B1200);

      /* setup_tty(fd, &tty, (speed_t)B2400);*/
      //serial_write(fd, "*o", 2); 
      //usleep(100);
      aprint("Mouse initialized. Good to go!");
    }

    if((mouse.pc_state == CTS_TOGGLED) && 
      (libevdev_next_event(mouse_dev, LIBEVDEV_READ_FLAG_NORMAL, &ev) == LIBEVDEV_READ_STATUS_SUCCESS)) {

      process_mouse_report(&mouse, &ev, options);
      runtime_settings(&mouse);

      /*** Send mouse state updates clamped to baud max rate ***/ 
      clock_gettime(CLOCK_MONOTONIC, &time_now);
      timespec_diff(&time_target, &time_now, &time_diff);

      if((time_diff.tv_sec < 0 && mouse.update > -1) || mouse.force_update) {
	input_sensitivity(&mouse);
        update_mouse_state(&mouse);
         
	// Send updates
	if(options->debug) { fprintf(stderr, "Sensitivity: %f\n", mouse.sensitivity); }
        for(i=0; i <= mouse.update; i++) {
          if(options->debug) {
	    fprintf(stderr, "Time: %d.%d\n", (int)time_diff.tv_sec, (int)time_diff.tv_nsec);
            fprintf(stderr, "Sent(ev:%d) %d: %x\n", ev.code, i, mouse.state[i]);
	    fprintf(stderr, "Mouse state(%d): %s\n", i, byte_to_bitstring(mouse.state[i]));
	  }
	  serial_write(fd, &mouse.state[i], sizeof(uint8_t));
        }
	if(options->debug) { printf("\n"); }

	// Use variable send rate depending on whether middle mouse button pressed or not (3 or 4 byte updates)
	if(mouse.mmb) { time_target = get_target_time(NS_SERIALDELAY_4B); }
	else          { time_target = get_target_time(NS_SERIALDELAY_3B); }

	reset_mouse_state(&mouse);
      }
    }
    usleep(1);
  }

  disable_pin(fd, TIOCM_RTS | TIOCM_DTR);

  if(options->exclusive) { ioctl(fd, EVIOCGRAB, 0); } // Release exclusive mouse access
  close(fd);

  free(options);

  return(0);
}
