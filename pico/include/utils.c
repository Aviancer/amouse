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

#include <stdint.h>
#include "utils.h"

/*** Helper functions ****/

const char * byte_to_bitstring(uint8_t val) {
  static char buffer[9];

  for (int i = 0; 7 >= i; i++) {
    buffer[7-i] = 0x30 + ((val >> i) & 1); 
  }
 return buffer;
}

int clamp(int value, int min, int max) {
  if(value > max) { return max; }
  if(value < min) { return min; }
  return value;
}

/*void mouse_state_to_serial(mouse_state_t *mouse) {
  uint8_t buffer[64];
  ssize_t couldWriteSize = snprintf(buffer, 64, "x%d y%d w%d lmb%d rmb%d mmb%d upd%d force%d\n", 
                           (int)mouse->x, (int)mouse->y, (int)mouse->wheel, (int)mouse->lmb, (int)mouse->rmb,
                           (int)mouse->mmb, (int)mouse->update, (int)mouse->force_update);
  serial_write(uart0, buffer, couldWriteSize);
  for(int i; i<3; i++) {
    serial_write(uart0, (uint8_t*)byte_to_bitstring(mouse->state[i]), 8);
    serial_write(uart0, "\n", 1);
  }
}*/
