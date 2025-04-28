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

#ifndef UTILS_H_   /* Include guard */
#define UTILS_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __linux__
#include <stdlib.h>
#else
#include "pico/stdlib.h"
#endif

// Return type for scan_int()
typedef struct scan_int_ret {
  bool found;
  int value;
  uint16_t offset; // How many bytes we have read into buffer.
} scan_int_t;

const char * byte_to_bitstring(uint8_t val);

int clampi(int value, int min, int max);

float clampf(float value, float min, float max);

uint atou(char* intbuffer, uint max_digits);

void itoa(int integer, char* intbuffer, uint max_digits);

scan_int_t scan_int(uint8_t* buffer, uint i, uint scan_size, uint max_digits);

#endif // UTILS_H_
