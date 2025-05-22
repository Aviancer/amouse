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
#include "include/storage.h"
#include "../../shared/console.h"
#include "../../shared/mouse.h"
#include "../../shared/utils.h"
#include "../../shared/settings.h"

// Linux specific
#include <sys/ioctl.h> // ioctl (serial pins, mouse exclusive access)
#include <libevdev.h>  // input dev
#include <getopt.h>    // getopt


/*** Program parameters ***/ 

// Struct for storing pointers to dynamically allocated memory containing options.
struct linux_opts {
  char *mousepath; // Pointers, memory is dynamically allocated.
  char *serialpath;
  int exclusive;
  int immediate;
  int debug;
};


/*** Linux console ***/

void aprint(const char *message) {
  printf("amouse> %s", message);
}

void showhelp(char *argv[]) {
  printf("Anachro Mouse v%d.%d.%d, a usb to serial mouse adaptor.\n" \
    "Usage: %s -m <mouse_input> -s <serial_output>\n\n" \
    "  -m <File> to read mouse input from (/dev/input/*)\n" \
    "  -s <File> to write to serial port with (/dev/tty*)\n" \
    "  -p <Proto num> Select from available serial protocols (\'-p ?\' for list)\n" \
    "  -r <1-30> Set mouse responsiveness/sensitivity\n" \
    "  -e Disable exclusive access to mouse\n" \
    "  -i Immediate ident mode, disables waiting for CTS pin\n" \
    "  -l Swap left and right buttons\n" \
    "  -W Write mouse settings to ~/.amouse.conf file\n"
    "  -d Print out debug information on mouse state\n", V_MAJOR, V_MINOR, V_REVISION, argv[0]);
}

void linux_save_settings() {
    uint8_t binary_settings[SETTINGS_SIZE] = {0};

    aprint("Writing settings..\n");
    settings_encode(&binary_settings[0], &mouse_options);
    write_flash_settings(&binary_settings[0], sizeof(binary_settings));
}

void parse_opts(int argc, char **argv, struct linux_opts *options) {
  int option_index = 0;
  int quit = 0;
  scan_int_t scan_i;         // Re-usable ret type for char arr to int conversion

  // Safe defaults
  mouse_options.wheel = 1;
  mouse_options.protocol = PROTO_MSWHEEL;
  mouse_options.sensitivity = 1.0;
  options->exclusive = 1;

  // Attempt to load saved settings from storage
  uint8_t* flash_memory = ptr_flash_settings();
  if(flash_memory == NULL) {
    linux_save_settings(); // If no settings loaded, save defaults
  }
  else {
    settings_decode(&flash_memory[0], &mouse_options);
  }

  while (( option_index = getopt(argc, argv, "hm:s:p:r:ielWd")) != -1) {

    switch(option_index) {
      case '?':
      case 'h':
        showhelp(argv); exit(0);
        break;
      case 'm':
        options->mousepath = strndup(optarg, 4096); // Max path size is 4095, plus a null byte
        break;
      case 's':
        options->serialpath = strndup(optarg, 4096);
        break;
      case 'p':
        scan_i = scan_int((uint8_t*)optarg, 0, 2, 1); // Note: 0-9 only.
        if(scan_i.found && scan_i.value < mouse_protocol_num) {
          mouse_options.protocol = scan_i.value;
        }
        else {
          fprintf(stderr, "Available mouse protocols\n");
          for(int i=0; i < mouse_protocol_num; i++) { // < is 0-indexed
                  fprintf(stderr, "  %i: %s\n", i, mouse_protocol[i].name);
          }
          exit(1);
        }
    	  break;
      case 'r':
        scan_i = scan_int((uint8_t*)optarg, 0, 3, 2); // Note: 0-99 only.
        set_sensitivity(scan_i);
        break;
      case 'i':
      	options->immediate = 1; // Don't wait for CTS pin to ident
	      break;
      case 'e':
      	options->exclusive = 0; // Computer will also get mouse inputs.
	      break;
      case 'l':
      	mouse_options.swap_buttons = 1;
      	break;
      case 'd':
	      options->debug = 1; // Enable debug prints
	      break;
      case 'W':
        linux_save_settings();
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

static inline void process_mouse_report(mouse_state_t *mouse, struct input_event const *ev, struct linux_opts *options) {
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
        mouse->mmb = ev->value;
        mouse->force_update = true;
        if(mouse_protocol[mouse_options.protocol].buttons > 2) { 
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
              push_update(mouse, mouse->mmb);
        break;
      case REL_Y:
        mouse->y += ev->value;
        mouse->y = clampi(mouse->y, -36862, 36862);
              push_update(mouse, mouse->mmb);
        break;
      case REL_WHEEL:
        mouse->wheel += ev->value;
        mouse->wheel = clampi(mouse->wheel, -63, 63);
        if(mouse_protocol[mouse_options.protocol].wheel) {
          push_update(mouse, true);
        }
        break;
    }
  }
}


/*** Main init & loop ***/

int main(int argc, char **argv) {

  struct termios old_tty;
  char itoa_buffer[6] = {0}; // Re-usable buffer for converting ints to char arr

  printf("%s\n\n", amouse_title);

  // Parse commandline options
  if(argc < 2) { showhelp(argv); exit(0); }
  struct linux_opts *options = (struct linux_opts*) calloc(1, sizeof(struct linux_opts)); // Memory is zeroed by calloc
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
  int serial_fd;
  serial_fd = open(options->serialpath, O_RDWR | O_NOCTTY | O_NONBLOCK); 
  if(serial_fd < 0) {
    fprintf(stderr, "Serial device file open() failed: %d: %s\n", errno, strerror(errno));
    exit(-1);
  }

  if (tcgetattr(serial_fd, &old_tty) != 0) {
    fprintf(stderr, "tcgetattr() failed: %d: %s\n", errno, strerror(errno));
  }
 
  // Initialize serial parameters 
  setup_tty(serial_fd, (speed_t)B1200);
  disable_pin(serial_fd, TIOCM_RTS | TIOCM_DTR); // We're not a modem so make sure pins low.

  fcntl (0, F_SETFL, O_NONBLOCK); // Nonblock 0=stdin
  setvbuf(stdout, NULL, _IONBF, 0); // Unbuffer stdout

  // Buffer for checking for requests from serial port. 
  uint8_t serial_buffer[2] = {0}; 
  int i; // Allocate outside main loop instead of allocating every time.

  // Aggregate movements before sending
  struct timespec time_rx_target, time_tx_target;
  mouse_state_t mouse;
  mouse.pc_state = CTS_UNINIT;
  reset_mouse_state(&mouse); // Set packet memory to initial state

  // Set timers
  time_tx_target = get_target_time(0, NS_SERIALDELAY_3B);
  time_rx_target = get_target_time(1, 0);
  
  aprint("Selected mouse protocol: "); printf("%s\n", mouse_protocol[mouse_options.protocol].name);
  itoa((int)(mouse_options.sensitivity * 10), itoa_buffer, sizeof(itoa_buffer) - 1);
  aprint("Mouse sensitiviy set to "); printf("%s.\n", itoa_buffer);
  aprint("Waiting for PC to initialize mouse driver..\n");

  // Ident immediately on program start up.
  if(options->immediate) {
    aprint("Performing immediate identification as mouse.\n");
    mouse_ident(serial_fd, mouse_options.wheel);
    mouse.pc_state = CTS_TOGGLED; // Bypass CTS detection, send events straight away.
  }

  /*** Main loop ***/
  bool pc_cts = false;

  while(1) {

    // Check for request for serial console
    // Repeating non-blocking reads is slow so instead we queue checks every now and then with timer.
    if(timespec_reached(&time_rx_target)) {
      if(serial_read(serial_fd, serial_buffer, 1) > 0) {
        if(serial_buffer[0] == '\b') {
          aprint("Console requested from serial line, suspending adapter.\n");
          console(serial_fd);
          aprint("Serial console closed, resuming adapter.\n");
        }
      }
      time_rx_target = get_target_time(1, 0); 
    }


    // Mouse handling

    pc_cts = get_pin(serial_fd, TIOCM_CTS);

    if(!pc_cts) { // Computers RTS low, only pin we care about for MS drivers, etc.
      if(mouse.pc_state == CTS_UNINIT) { mouse.pc_state = CTS_LOW_INIT; }
      else if(mouse.pc_state == CTS_TOGGLED) { mouse.pc_state = CTS_LOW_RUN; }
    }

    // Mouse initiaizing request detected
    if(pc_cts && (mouse.pc_state != CTS_UNINIT && mouse.pc_state != CTS_TOGGLED)) {
      if(options->debug) {
      	aprint("Computers RTS pin toggled, identifying as mouse.\n");
      }
      mouse.pc_state = CTS_TOGGLED;
      mouse_ident(serial_fd, mouse_options.wheel);
      aprint("Mouse initialized. Good to go!\n");
    }

    // Transmit only once we are initialized at least once. Unlike in DOS, Windows drivers will set CTS pin 
    // low after init which would inhibit transmitting. We will trust the driver to re-init if needed.
    if((mouse.pc_state > CTS_LOW_INIT) && 
      (libevdev_next_event(mouse_dev, LIBEVDEV_READ_FLAG_NORMAL, &ev) == LIBEVDEV_READ_STATUS_SUCCESS)) {

      process_mouse_report(&mouse, &ev, options);
      runtime_settings(&mouse);

      /*** Send mouse state updates clamped to baud max rate ***/ 
      if((timespec_reached(&time_tx_target) && mouse.update > -1) || mouse.force_update) {
      	input_sensitivity(&mouse);
        update_mouse_state(&mouse);
         
        // Send updates
        if(options->debug) { fprintf(stderr, "Sensitivity: %f\n", mouse_options.sensitivity); }
          for(i=0; i < mouse.update; i++) {
            if(options->debug) {
              fprintf(stderr, "Time: %d.%d\n", (int)time_tx_target.tv_sec, (int)time_tx_target.tv_nsec);
              fprintf(stderr, "Sent(ev:%d) %d: %x\n", ev.code, i, mouse.state[i]);
              fprintf(stderr, "Mouse state(%d): %s\n", i, byte_to_bitstring(mouse.state[i]));
          }
          serial_write(serial_fd, &mouse.state[i], sizeof(uint8_t));
        }
        if(options->debug) { printf("\n"); }

        // Use different send rate depending on protocol used (3 or 4 byte)
        if(mouse.update > 3) { time_tx_target = get_target_time(0, NS_SERIALDELAY_4B); }
        else                 { time_tx_target = get_target_time(0, NS_SERIALDELAY_3B); }

        reset_mouse_state(&mouse);
      }
    }
    usleep(1);
  }

  disable_pin(serial_fd, TIOCM_RTS | TIOCM_DTR);

  if(options->exclusive) { ioctl(mouse_fd, EVIOCGRAB, 0); } // Release exclusive mouse access
  close(serial_fd);

  free(options);

  return(0);
}
