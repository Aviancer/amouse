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

/* Protocol definitions */
#define MOUSE_LMB_BIT 5 // Defines << shift for bit position
#define MOUSE_RMB_BIT 4
#define MOUSE_MMB_BIT 4 // Shift 4 times in 4th byte

// Delay between data packets for 1200 baud
#define NS_FULL_SECOND 1000000000L   // 1s in nanoseconds
#define SERIALDELAY_3B   22700000    // 3 bytes
#define SERIALDELAY_4B   30000000    // 4 bytes

int serial_write(int fd, uint8_t *buffer, int size);

int get_pin(int fd, int flag);

int enable_pin(int fd, int flag);

int disable_pin(int fd, int flag);

int setup_tty(int fd, speed_t baudrate);

void wait_pin_state(int fd, int flag, int desired_state);

void mouse_ident(int fd, int wheel, int immediate);

void timespec_diff(struct timespec *ts1, struct timespec *ts2, struct timespec *result);

struct timespec get_target_time(uint32_t delay);

#endif // SERIAL_H_
