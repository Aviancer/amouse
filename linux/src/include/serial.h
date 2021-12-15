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

#ifndef SERIAL_H_
#define SERIAL_H_

#include <termios.h> // POSIX terminal control defs

int serial_write(int fd, uint8_t *buffer, int size);

int serial_write_terminal(int fd, uint8_t *buffer, int size);

int serial_read(int fd, uint8_t *buffer, int size);

int get_pin(int fd, int flag);

int enable_pin(int fd, int flag);

int disable_pin(int fd, int flag);

int setup_tty(int fd, speed_t baudrate);

void wait_pin_state(int fd, int flag, int desired_state);

void mouse_ident(int fd, int wheel);

void timespec_diff(struct timespec *ts1, struct timespec *ts2, struct timespec *result);

struct timespec get_target_time(uint32_t delay);

#endif // SERIAL_H_
