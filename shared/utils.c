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

const char* byte_to_bitstring(uint8_t val) {
  static char buffer[9];

  for (int i = 0; 7 >= i; i++) {
    buffer[7-i] = 0x30 + ((val >> i) & 1); 
  }
 return buffer;
}

int clampi(int value, int min, int max) {
  if(value > max) { return max; }
  if(value < min) { return min; }
  return value;
}

float clampf(float value, float min, float max) {
  if(value > max) { return max; }
  if(value < min) { return min; }
  return value;
}

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
